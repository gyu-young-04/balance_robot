#include "M5Unified.h"
#include "M5Module4EncoderMotor.h"
#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <gyu-young-04-project-2_inferencing.h>

M5Module4EncoderMotor driver;

// ======================
// Motor
// ======================
#define LEFT_MOTOR  2
#define RIGHT_MOTOR 3
#define LEFT_DIR     (1)
#define RIGHT_DIR    (-1)
#define LEFT_ENC_DIR  (1)
#define RIGHT_ENC_DIR (1)
#define PWM_LIMIT    55
#define DEAD_BAND     3

// ======================
// PID 밸런싱 파라미터
// ======================
float Kp     = 11.0;
float Ki     =  0.1;
float Kd     =  0.6;
float Ks     = -0.20;
float target =  5.05;

// ======================
// 라인트레이싱 파라미터
// 흰색 C≈280, 검정 C≈120 → 중간값 200
// ======================
const float LINE_SETPOINT = 200.0f;
const float KP_LINE       =  0.40f;  // 0.12 → 0.40 (조향력 강화)
const float KD_LINE       =  0.05f;  // 변화량 보조
const int   MAX_STEER     = 18;

// ======================
// 전역 상태
// ======================
volatile float g_pitch    = 0;
volatile int   g_pwm      = 0;
volatile int   g_steering = 0;
volatile float g_forward  = 0.0f;
float target_g_forward = 0.0f; // 목표 속도
float current_g_forward = 0.0f; // 현재 속도 (PID에 사용)

enum RobotState { STOPPED, RUNNING };
volatile RobotState g_state = STOPPED;

// ======================
// 컬러 센서
// ======================
TwoWire ColorWire = TwoWire(0);
Adafruit_TCS34725 tcs(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

static float filtered_c = 200.0f;  // 초기값을 중간값으로
static float prev_c     = 200.0f;

// ======================
// 음성인식 버퍼
// ======================
static constexpr size_t record_samplerate = 16000;
static int16_t inference_buffer[EI_CLASSIFIER_RAW_SAMPLE_COUNT];
static int16_t half_buffer[EI_CLASSIFIER_RAW_SAMPLE_COUNT / 2];

unsigned long last_trigger = 0;
const unsigned long DEBOUNCE_MS = 2000;

void updateScreenBackground() {
  if (g_state == RUNNING) {
    M5.Display.fillScreen(TFT_GREEN); // 출발 -> 초록
  } else {
    M5.Display.fillScreen(TFT_RED);   // 정지 -> 빨강
  }
}

// ===================================================
// PID TASK (Core 1)
// ===================================================
void PIDtask(void* pv) {
  float pitch = 0;
  float integral = 0;
  int32_t prevEncL = 0;
  int32_t prevEncR = 0;
  unsigned long lastTime = micros();
  float filtered_speed = 0;

  while (1) {
    unsigned long now = micros();
    float dt = (now - lastTime) / 1000000.0f;
    lastTime = now;

    if (dt <= 0.0f || dt > 0.05f) dt = 0.005f;

    float ax, ay, az, gx, gy, gz;
    M5.Imu.getAccel(&ax, &ay, &az);
    M5.Imu.getGyro(&gx, &gy, &gz);

    pitch = 0.985f * (pitch + (-gy) * dt)
          + 0.015f * (atan2(az, -ax) * 180.0f / PI);

    int32_t encL = LEFT_ENC_DIR  * driver.getEncoderValue(LEFT_MOTOR);
    int32_t encR = RIGHT_ENC_DIR * driver.getEncoderValue(RIGHT_MOTOR);
    float raw_speed = ((encL - prevEncL) + (encR - prevEncR)) / 2.0f / dt;
    prevEncL = encL;
    prevEncR = encR;
    filtered_speed = 0.6f * filtered_speed + 0.4f * raw_speed;

    float error = pitch - (target + g_forward);
    integral += error * dt;
    integral = constrain(integral, -5, 5);

    float pwm_f = (-Kp * error) + (-Ki * integral) + (Kd * gy) + (-Ks * filtered_speed);

    int motor_pwm = 0;
    if (abs(pwm_f) > 0.5f)
      motor_pwm = (pwm_f > 0) ? (int)pwm_f + DEAD_BAND : (int)pwm_f - DEAD_BAND;

    motor_pwm = constrain(motor_pwm, -PWM_LIMIT, PWM_LIMIT);

    if (abs(pitch) > 45) { motor_pwm = 0; integral = 0; }

    static unsigned long start_time = 0;
    if (g_state == RUNNING && start_time == 0) start_time = millis(); // 처음 출발 시간 기록
    if (g_state == STOPPED) start_time = 0; // 정지 시 초기화

    int steer = g_steering;
    // 출발 후 800ms 동안은 조향을 0으로 고정하여 균형 잡을 시간을 줌
    if (g_state == RUNNING && (millis() - start_time < 800)) {
        steer = 0;
    }
    int leftOut  = constrain(motor_pwm + steer, -PWM_LIMIT, PWM_LIMIT);
    int rightOut = constrain(motor_pwm - steer, -PWM_LIMIT, PWM_LIMIT);

    driver.setMotorSpeed(LEFT_MOTOR,  LEFT_DIR  * leftOut  * 1.05f);
    driver.setMotorSpeed(RIGHT_MOTOR, RIGHT_DIR * rightOut);

    g_pitch = pitch;
    g_pwm   = motor_pwm;

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// ===================================================
// SETUP
// ===================================================
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  Serial.begin(115200);
  M5.Imu.begin();

  Wire1.begin(11, 12, 400000);
  driver.begin(&Wire1, MODULE_4ENCODERMOTOR_ADDR, 11, 12);
  driver.setMode(LEFT_MOTOR,  NORMAL_MODE);
  driver.setMode(RIGHT_MOTOR, NORMAL_MODE);

  ColorWire.begin(17, 18, 100000);
  M5.Power.setExtOutput(true);
  delay(300);
  if (!tcs.begin(TCS34725_ADDRESS, &ColorWire))
    Serial.println("Color Sensor NOT FOUND");
  else
    Serial.println("Color Sensor OK");

  auto mic_cfg = M5.Mic.config();
  mic_cfg.sample_rate = record_samplerate;
  mic_cfg.magnification = 8;
  mic_cfg.noise_filter_level = 0;
  M5.Mic.config(mic_cfg);
  M5.Speaker.end();
  M5.Mic.begin();
  memset(inference_buffer, 0, sizeof(inference_buffer));

  updateScreenBackground();
  xTaskCreatePinnedToCore(PIDtask, "PIDtask", 4096, NULL, 5, NULL, 1);
}

// ===================================================
// LOOP (Core 0)
// ===================================================
void loop() {
  M5.update();

  static uint32_t lastColor = 0;
  static uint32_t lastPrint = 0;

  // 1. 라인트레이싱 PD (20ms)
  if (millis() - lastColor > 20) {
    lastColor = millis();

    uint16_t r, g, b, c;
    tcs.getRawData(&r, &g, &b, &c);

    // 필터 업데이트
    if (c > 0) {
      filtered_c = 0.6f * filtered_c + 0.4f * (float)c;
    }

    // [수정] g_state 관계없이 항상 조향 계산 수행
    float error = filtered_c - LINE_SETPOINT;
    float delta = filtered_c - prev_c;
    prev_c = filtered_c;

    int target_steer = (int)(KP_LINE * error + KD_LINE * delta);
    target_steer = constrain(target_steer, -MAX_STEER, MAX_STEER);
    
    // 기울어질 때 조향 약화
    if (abs(g_pitch) > 15) target_steer = (int)(target_steer * 0.7f);
    
    // 조향값 업데이트
    g_steering = target_steer;

    // 만약 STOPPED 상태라면, 속도(g_forward)는 0이므로 제자리에서 방향만 잡게 됨
  }

  // 2. 음성인식 (sliding window)
  if (M5.Mic.isEnabled()) {
    size_t half = EI_CLASSIFIER_RAW_SAMPLE_COUNT / 2;
    if (M5.Mic.record(half_buffer, half, record_samplerate)) {

      memmove(inference_buffer, inference_buffer + half, half * sizeof(int16_t));
      memcpy(inference_buffer + half, half_buffer, half * sizeof(int16_t));

      signal_t signal;
      signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
      signal.get_data = [](size_t offset, size_t length, float *out_ptr) -> int {
        for (size_t i = 0; i < length; i++)
          out_ptr[i] = (float)inference_buffer[offset + i];
        return 0;
      };

      ei_impulse_result_t result = {0};
      run_classifier(&signal, &result, false);

      int best_idx = 0;
      float best_val = 0;
      for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > best_val) {
          best_val = result.classification[i].value;
          best_idx = i;
        }
      }

      String label = String(result.classification[best_idx].label);
      label.toUpperCase();

      if (best_val > 0.65f && label != "NOISE"
          && millis() - last_trigger > DEBOUNCE_MS) {
        last_trigger = millis();

        if (label == "START") {
            g_state = RUNNING;
            target_g_forward = 0.5f; // 바로 적용하지 않고 목표치만 설정
            // 출발 시 조향 0으로 초기화
            g_steering = 0; 
            updateScreenBackground();
            Serial.println("=== START (Ramping) ===");
        } else if (label == "STOP") {
          g_state   = STOPPED;
          g_forward = 0.0f;
          updateScreenBackground();
          Serial.println("=== STOP ===");
        }
      }
    }
  }

  // 3. 시리얼 출력 (200ms)
  if (millis() - lastPrint > 200) {
    lastPrint = millis();
    Serial.printf("[%s] C:%4.0f Err:%6.1f | Pitch:%6.2f | PWM:%4d | Steer:%3d\n",
      g_state == RUNNING ? "RUN " : "STOP",
      filtered_c,
      filtered_c - LINE_SETPOINT,
      g_pitch, g_pwm, g_steering);
  }

    static uint32_t last_ramp = 0;
    if (millis() - last_ramp > 50) {
        last_ramp = millis();
        if (g_state == RUNNING && current_g_forward < target_g_forward) {
            current_g_forward += 0.02f; // 조금씩 증가 (0.02씩)
        } else if (g_state == STOPPED && current_g_forward > 0) {
            current_g_forward -= 0.05f; // 정지 시엔 빠르게 감소
            if (current_g_forward < 0) current_g_forward = 0;
        }
        g_forward = current_g_forward; // 실제 PID에 반영
    }
  delay(1);
}