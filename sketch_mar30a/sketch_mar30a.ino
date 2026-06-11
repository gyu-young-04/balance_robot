/*#include <M5Unified.h>

#define LED_PIN 1   // GPIO (노랑선)
#define BTN_PIN 2   // GPIO (흰색선)

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP); // 버튼
}

void loop() {
  int buttonState = digitalRead(BTN_PIN);

  if (buttonState == LOW) { // 버튼 눌림
    digitalWrite(LED_PIN, HIGH);  // LED ON
    Serial.println("Button Pressed → LED ON");
  } else {
    digitalWrite(LED_PIN, LOW);   // LED OFF
    Serial.println("Button Released → LED OFF");
  }

  delay(100);
}
*/