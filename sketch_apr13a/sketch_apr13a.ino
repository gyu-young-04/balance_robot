/*
#include <M5Unified.h> // M5Stack의 모든 기기를 하나로 통합(Unified)하여 다루겠다 (최신 호환성 높인 라이브러리)

void setup() {
    auto cfg = M5.config(); // 부팅될때 필요한 수십가지 기본값들 설정
    M5.begin(cfg);  // 하드웨어에 전기를 넣고 통신을 시작하는 명령어
    // 세밀한 초기상태를 설정하기위해선 config 후 필요로 하는 작업 추가하고 시작하기
    // 나는 전력 소모를 줄이기 위해 화면을 아예 안 켜고 센서만 쓸 거야!" 할 때
    //begin을 하기 전에 cfg.external_display = false; 같은 식으로 세밀하게 튜닝
*/

#include <M5Unified.h>
#include <math.h>
#define MODE 3  // 1: IMU, 2: 지자기 센서 3: 소리 인식(마이크)

// 1번 imu sensor (기기의 기울기, 움직임, 회전을 감지)
void imu_setup() {
    Serial.begin(115200);
    Serial.println("START");
    /*  없어도 된다길래 일단 없애봄
    // 🔥 핵심 (이거 없으면 안 잡히는 경우 많음)
    while (!Serial) delay(10);

    delay(2000);
    */

    // LCD 설정
    M5.Lcd.setRotation(1);  // 화면방향
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setCursor(0, 0); // 출력 시작 위치

    // IMU 초기화
    if (!M5.Imu.begin()) {    //IMU 센서를 실제로 시작해봤는데 실패했냐?
        M5.Lcd.println("IMU Init Failed!");
        Serial.println("IMU Init Failed!");
        while (1) delay(1000);
    }

    M5.Lcd.println("IMU Ready!");
    Serial.println("IMU Ready!");   // usb랑 연결끊으면 실행안됨
    delay(1000);
}

void imu_loop() {
    float ax, ay, az;
    float gx, gy, gz;

    // 센서 데이터 읽기
    M5.Imu.getAccel(&ax, &ay, &az); // getAccel - 가속도센서 (좌우,앞뒤,위아래)
    //얼마나 움직였는지 기울여졌는지,중력방향 정보를 제공
    M5.Imu.getGyro(&gx, &gy, &gz); // getGyro- 자이로 센서 (x축회전속도,y축회전속도,z축회전속도)
    //(회전속도를 의미)

    // 🔥 기울기 계산 (getAhrs 대신)
    float pitch = atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
    float roll  = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;

    // 화면 출력 (덮어쓰기 방식)
    M5.Lcd.setCursor(0, 0);

    M5.Lcd.println("[ IMU Data - CoreS3 ]");
    M5.Lcd.println("---------------------");

    // 기울기
    M5.Lcd.printf("Pitch: %6.2f\n", pitch); // 소수점 두자리 출력 
    // 3.14 처럼 되면 4자리는 사용중이니 나머지 2자리는 공백으로
    // 숫자 줄을 깔끔하게 맞추기 위해서 사용
    M5.Lcd.printf("Roll : %6.2f\n", roll);

    M5.Lcd.println("---------------------");

    // 가속도
    M5.Lcd.printf("Acc X: %6.2f\n", ax);
    M5.Lcd.printf("Acc Y: %6.2f\n", ay);
    M5.Lcd.printf("Acc Z: %6.2f\n", az);

    M5.Lcd.println("---------------------");

    // 회전 속도
    M5.Lcd.printf("Gyro X: %6.2f\n", gx);
    M5.Lcd.printf("Gyro Y: %6.2f\n", gy);
    M5.Lcd.printf("Gyro Z: %6.2f\n", gz);

    // 🔥 PC(시리얼 모니터) 출력
    Serial.printf("Pitch: %.2f, Roll: %.2f | ", pitch, roll);
    Serial.printf("Acc: %.2f %.2f %.2f | ", ax, ay, az);
    Serial.printf("Gyro: %.2f %.2f %.2f\n", gx, gy, gz);

    delay(100);
}


// 2번 지자기 센서 이용 (0 - 북     90 - 동     180 - 남    270 - 서)



void mag_setup() {
    M5.Lcd.setRotation(1);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE, BLACK);

    if (!M5.Imu.begin()) {
        M5.Lcd.println("IMU Init Failed!");
        while (1);
    }

    M5.Lcd.println("Mag Ready!");
    delay(1000);
}
// 센서 보정안된거같음
void mag_loop() {
    float mx, my, mz;

    // 자기장 값 읽기
    M5.Imu.getMag(&mx, &my, &mz);

    // 방향 계산
    float heading = atan2(my, mx) * 180.0 / PI;

    if (heading < 0) heading += 360;

    // 화면 출력
    M5.Lcd.setCursor(0, 0);

    M5.Lcd.println("[ Compass ]");
    M5.Lcd.println("----------------");

    M5.Lcd.printf("Heading: %6.2f\n", heading);

    delay(200);
}



// 3 마이크로 소리받아서 출력하기  (볼륨높으면 스피커로 경고음)
// 스피커는 너무 크다 소리가
// 진동하기 기능이 빠져있음


unsigned long lastAlertTime = 0; // 마지막으로 경고음이 울린 시간 저장
const int alertInterval = 500;   // 경고음 간격 (0.5초)
void mic_setup() {

    M5.Display.setTextSize(2);
    M5.Display.println("M5Unified Mic Test");

    // 마이크 설정을 명시적으로 시작 (M5Unified가 알아서 핀을 잡음)
    if (!M5.Mic.begin()) {
        M5.Display.println("Mic Init Failed!");
        while (1) delay(1);
    }
    if (!M5.Speaker.begin()) {
        M5.Display.println("Speaker Init Failed!");
    }
    M5.Speaker.setVolume(5);
    M5.Speaker.setChannelVolume(0, 5);  // 한번더 소리를 깎음
}

void mic_loop() {
    // 마이크로부터 소리 데이터를 읽어오기 위한 버퍼
    static int16_t sBuffer[64];
    // static 은 이 함수가 끝나도 기억하겠다는 뜻 
    // int16_t 의 경우 여기있는 마이크가 소리를 디지털로 바꿀때 16비트 숫자로 표현함
    
    // 마이크 데이터 읽기 (내부적으로 I2S 처리)
    if (M5.Mic.record(sBuffer, 64, 16000)) { 
        // 16000은 헤르츠로서 초당 샘플링 횟수다
        // 64개가 될때마다 if문 안의 로직이 실행됨
        long sum = 0;
        for (int i = 0; i < 64; i++) {
            sum += abs(sBuffer[i]); // 진폭의 절댓값 합산
        }
        float volume = (float)sum / 64;

        // 화면 출력
        M5.Display.setCursor(0, 40);
        M5.Display.printf("Volume: %6.2f\n", volume);

        // 막대 그래프 시각화
        int barWidth = map((int)volume, 0, 1000, 0, M5.Display.width());
        M5.Display.fillRect(0, 100, barWidth, 50, GREEN);
        M5.Display.fillRect(barWidth, 100, M5.Display.width() - barWidth, 50, BLACK);
        
        // 목소리가 클 때 반응
        if (volume > 600 && (millis() - lastAlertTime > alertInterval)) {
            // M5.Speaker.tone(100, 50); // 경고음 100Hz로 5ms 동안
            lastAlertTime = millis();
            M5.Display.setCursor(0, 180);
            M5.Display.setTextColor(RED, BLACK);
            M5.Display.print("DETECTED!");
        } 
        if (millis() - lastAlertTime > alertInterval) {
            M5.Display.fillRect(0, 180, 200, 30, BLACK);// 글자 지우기
            // 계속 변할때는 덮어쓰면 되는데 
            // 특정조건에만 나오는 것들은 다른위치에 나올수있기에 안보이게 하는 코드가 필요
        }
    }
    M5.Display.setCursor(0, 0);
    M5.Display.printf("Current Vol: %d", M5.Speaker.getVolume());
    
    delay(10); // 너무 빠르면 화면 잔상이 생기니 약간의 딜레이
}





void setup() {
    auto cfg = M5.config();
    cfg.external_speaker.atomic_spk = true; // 외부 스피커 설정을 건드리지 않도록 함
    M5.begin(cfg);

#if MODE == 1
    imu_setup();
#elif MODE == 2
    mag_setup();
#elif MODE == 3
    mic_setup();
#endif
}

void loop() {
    M5.update();

#if MODE == 1
    imu_loop();
#elif MODE == 2
    mag_loop();
#elif MODE == 3
    mic_loop();
#endif
}
