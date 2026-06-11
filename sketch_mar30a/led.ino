/*#include <M5Unified.h>

#define LED_PIN 1   // GPIO (노랑선)
#define BTN_PIN 2   // GPIO (흰색선)

void setup() {
  M5.begin();
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextSize(2);

  // 화면에 버튼 영역 표시
  M5.Lcd.fillRect(50, 100, 100, 50, TFT_BLUE);   // 파란 버튼
  M5.Lcd.fillRect(200, 100, 100, 50, TFT_RED);   // 빨간 버튼
  M5.Lcd.drawString("Blue", 70, 120);
  M5.Lcd.drawString("Red", 220, 120);
}

void loop() {
  M5.update();  // 터치 상태 갱신

  if (M5.Touch.ispressed()) {
    auto point = M5.Touch.getPressPoint();  // 좌표 읽기
    int x = point.x;
    int y = point.y;

    // 파란 버튼 영역 확인
    if (x > 50 && x < 150 && y > 100 && y < 150) {
      M5.Lcd.fillScreen(TFT_BLUE);
      M5.Lcd.setTextColor(TFT_WHITE);
      M5.Lcd.drawString("Blue Button Pressed!", 60, 200);
    }

    // 빨간 버튼 영역 확인
    if (x > 200 && x < 300 && y > 100 && y < 150) {
      M5.Lcd.fillScreen(TFT_RED);
      M5.Lcd.setTextColor(TFT_WHITE);
      M5.Lcd.drawString("Red Button Pressed!", 80, 200);
    }
  }
}*/