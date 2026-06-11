/*
#include <M5Unified.h> // M5Stack의 모든 기기를 하나로 통합(Unified)하여 다루겠다 (최신 호환성 높인 라이브러리)

void setup() {
    auto cfg = M5.config(); // 부팅될때 필요한 수십가지 기본값들 설정
    M5.begin(cfg);  // 하드웨어에 전기를 넣고 통신을 시작하는 명령어
    // 세밀한 초기상태를 설정하기위해선 config 후 필요로 하는 작업 추가하고 시작하기
    // 나는 전력 소모를 줄이기 위해 화면을 아예 안 켜고 센서만 쓸 거야!" 할 때
    //begin을 하기 전에 cfg.external_display = false; 같은 식으로 세밀하게 튜닝

    Serial.begin(115200);  // PC 출력용 (컴퓨터랑 통신을 한다)


    // LCD 설정
    M5.Lcd.setRotation(1); // 화면 방향 설정
    M5.Lcd.setTextSize(2); // 글자 크기
    M5.Lcd.setTextColor(WHITE, BLACK); // 글자색, 배경색
    
    // IMU 초기화 확인
    if (!M5.Imu.begin()) {
        M5.Lcd.println("IMU 초기화 실패!");
    }
}

void loop() {
    M5.update(); // 내부 상태 업데이트

    // 데이터 변수 선언
    float ax, ay, az; // 가속도 (Accelerometer)
    float gx, gy, gz; // 각속도 (Gyroscope)
    float pitch, roll, yaw; // 기울기 및 각도 (Pose)

    // IMU 데이터 읽기
    M5.Imu.getAccel(&ax, &ay, &az);
    M5.Imu.getGyro(&gx, &gy, &gz);
    M5.Imu.getAhrs(&pitch, &roll, &yaw); // 내부 필터를 거친 안정적인 각도

    // 화면 출력 (커서를 맨 위로 고정하여 계속 덮어씌움)
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("[ IMU Data - CoreS3 ]");
    M5.Lcd.println("---------------------");

    // 1. 기울기 (Degree)
    M5.Lcd.printf("Pitch: %6.2f\n", pitch);
    M5.Lcd.printf("Roll:  %6.2f\n", roll);
    M5.Lcd.printf("Yaw:   %6.2f\n", yaw);
    M5.Lcd.println("---------------------");

    // 2. 가속도/속도 변화 (g)
    M5.Lcd.printf("Acc X: %6.2f\n", ax);
    M5.Lcd.printf("Acc Y: %6.2f\n", ay);
    M5.Lcd.printf("Acc Z: %6.2f\n", az);
    M5.Lcd.println("---------------------");

    // 3. 회전 속도 (deg/s)
    M5.Lcd.printf("Gyro X: %6.2f\n", gx);
    M5.Lcd.printf("Gyro Y: %6.2f\n", gy);
    M5.Lcd.printf("Gyro Z: %6.2f\n", gz);

    delay(100); // 0.1초마다 갱신
}
*/

#include <M5Unified.h>
#include <math.h>
void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);

    Serial.begin(115200);

    // 🔥 핵심 (이거 없으면 안 잡히는 경우 많음)
    while (!Serial) delay(10);

    delay(2000);

    Serial.println("START");

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
/*
void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);

    Serial.begin(115200);  // PC 출력용 (컴퓨터랑 통신을 한다)

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
}*/

void loop() {
    M5.update();    // 기기상태를 업데이트하는것

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
    M5.Lcd.printf("Pitch: %6.2f\n", pitch);
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
