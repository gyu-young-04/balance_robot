#include "M5Unified.h"
#include "M5Module4EncoderMotor.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

M5Module4EncoderMotor driver;

#define LEFT_MOTOR  2
#define RIGHT_MOTOR 3
#define LEFT_DIR    (1)
#define RIGHT_DIR   (-1)
#define LEFT_ENC_DIR  (1)
#define RIGHT_ENC_DIR (1)

// BLE UUID (고정값, 그냥 사용)
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

float Kp = 11.0;
float Ki = 0.1;
float Kd = 0.6;
float Ks = -0.23;
#define PWM_LIMIT 80

float target = -1.45;
float g_pitch = 0;
int g_pwm = 0;
float g_speed = 0;
float left_trim = 1.0;
float right_trim = 1.01;

// 명령어 정의
#define CMD_STOP      '0'
#define CMD_FORWARD   'F'
#define CMD_BACKWARD  'B'
#define CMD_LEFT      'L'
#define CMD_RIGHT     'R'

volatile char g_cmd = CMD_STOP;

// BLE 수신 콜백
class CmdCallback : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pChar) {
        String val = String(pChar->getValue().c_str());
        if (val.length() > 0) {
            g_cmd = val[0];
        }
    }
};

void setupBLE() {
    BLEDevice::init("M5Balance");  // 폰에서 보이는 이름
    BLEServer* pServer = BLEDevice::createServer();
    BLEService* pService = pServer->createService(SERVICE_UUID);
    BLECharacteristic* pChar = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pChar->setCallbacks(new CmdCallback());
    pService->start();
    BLEAdvertising* pAdv = BLEDevice::getAdvertising();
    pAdv->addServiceUUID(SERVICE_UUID);
    pAdv->setScanResponse(true);        // ← 추가
    pAdv->setMinPreferred(0x06);        // ← 추가
    pAdv->setMinPreferred(0x12);        // ← 추가
    BLEDevice::startAdvertising();      // ← 이걸로 변경
    Serial.println("BLE started");
}

void PIDtask(void* pv) {
    float pitch = 0;
    float integral = 0;
    int32_t prevEncL = 0, prevEncR = 0;
    unsigned long lastTime = micros();
    static float filtered_speed = 0;

    while (1) {
        unsigned long now = micros();
        float dt = (now - lastTime) / 1000000.0;
        lastTime = now;
        if (dt <= 0 || dt > 0.05) { vTaskDelay(1); continue; }

        // 명령에 따른 target 조정
        float drive_target = target;
        float turn_trim = 0.0;

        switch (g_cmd) {
            case CMD_FORWARD:  drive_target = target - 2.5; break;  // 앞으로 기울게
            case CMD_BACKWARD: drive_target = target + 2.5; break;  // 뒤로 기울게
            case CMD_LEFT:     turn_trim =  0.3; break;
            case CMD_RIGHT:    turn_trim = -0.3; break;
            default: break;
        }

        float ax, ay, az, gx, gy, gz;
        M5.Imu.getAccel(&ax, &ay, &az);
        M5.Imu.getGyro(&gx, &gy, &gz);
        float pitchAcc = atan2(az, -ax) * 180.0 / PI;
        pitch = 0.985 * (pitch + (-gy) * dt) + 0.015 * pitchAcc;

        int32_t encL = LEFT_ENC_DIR * driver.getEncoderValue(LEFT_MOTOR);
        int32_t encR = RIGHT_ENC_DIR * driver.getEncoderValue(RIGHT_MOTOR);
        float raw_speed = ((encL - prevEncL) + (encR - prevEncR)) / 2.0 / dt;
        prevEncL = encL;
        prevEncR = encR;
        filtered_speed = 0.8 * filtered_speed + 0.2 * raw_speed;

        float error = pitch - drive_target;
        integral += error * dt;
        integral = constrain(integral, -5, 5);

        float pwm_f = (-Kp * error) + (-Ki * integral) + (Kd * gy) + (-Ks * filtered_speed);

        int motor_pwm = 0;
        if (abs(pwm_f) > 0.5) {
            motor_pwm = (pwm_f > 0) ? (int)pwm_f + 3 : (int)pwm_f - 3;
        }
        motor_pwm = constrain(motor_pwm, -PWM_LIMIT, PWM_LIMIT);

        if (abs(pitch) > 45) {
            motor_pwm = 0;
            integral = 0;
        }

        // 좌우 회전: 양쪽 모터 속도 차이로 회전
        driver.setMotorSpeed(LEFT_MOTOR,  LEFT_DIR  * (motor_pwm + turn_trim * PWM_LIMIT) * left_trim);
        driver.setMotorSpeed(RIGHT_MOTOR, RIGHT_DIR * (motor_pwm - turn_trim * PWM_LIMIT) * right_trim);

        g_pitch = pitch;
        g_pwm = motor_pwm;
        g_speed = filtered_speed;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);
    M5.Imu.begin();
    Wire1.setClock(400000);
    if (!driver.begin(&Wire1, MODULE_4ENCODERMOTOR_ADDR, 11, 12)) {
        Serial.println("Motor Module Error");
        while (1);
    }
    driver.setMode(LEFT_MOTOR, NORMAL_MODE);
    driver.setMode(RIGHT_MOTOR, NORMAL_MODE);
    setupBLE();
    xTaskCreatePinnedToCore(PIDtask, "PIDtask", 4096, NULL, 5, NULL, 1);
}

void loop() {
    Serial.printf("Pitch:%.2f | PWM:%d | Speed:%.2f | CMD:%c\n",
                  g_pitch, g_pwm, g_speed, g_cmd);
    delay(100);
}