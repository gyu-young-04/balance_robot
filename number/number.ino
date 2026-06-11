#include <M5Unified.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h"

#define SENSOR_SDA  17
#define SENSOR_SCL  18

Adafruit_TCS34725 tcs = Adafruit_TCS34725(
  TCS34725_INTEGRATIONTIME_154MS,
  TCS34725_GAIN_4X
);

// ── 화이트밸런스 보정값 (흰 종이 기준) ──
float WB_R = 2.325f;
float WB_G = 1.00f;
float WB_B = 0.727f;

String getColorName(uint8_t r, uint8_t g, uint8_t b, uint16_t c_raw) {
  float rf = r / 255.0f;
  float gf = g / 255.0f;
  float bf = b / 255.0f;

  float maxC  = max(rf, max(gf, bf));
  float minC  = min(rf, min(gf, bf));
  float delta = maxC - minC;

  float s = (maxC > 0) ? delta / maxC : 0;
  float v = maxC;

  if (c_raw < 30)  return "BLACK";
  if (v < 0.20f)   return "BLACK";
  if (s < 0.03f)   return "WHITE";
  if (s < 0.10f)   return "GRAY";

  float h = 0;
  if (delta > 0) {
    if      (maxC == rf) h = 60.0f * fmod((gf - bf) / delta, 6.0f);
    else if (maxC == gf) h = 60.0f * ((bf - rf) / delta + 2.0f);
    else                 h = 60.0f * ((rf - gf) / delta + 4.0f);
    if (h < 0) h += 360.0f;
  }

  if      (h <  20 || h >= 340) return (s < 0.50f) ? "PINK"   : "RED";
  else if (h <  85)              return "YELLOW";
  else if (h < 150)              return "GREEN";
  else if (h < 200)              return "CYAN";
  else if (h < 265)              return "BLUE";
  else if (h < 295)              return "PURPLE";
  else                           return (s < 0.50f) ? "PINK"   : "PURPLE";
}

uint32_t getBgColor(const String& name) {
  if (name == "RED")    return M5.Display.color565(220,  40,  40);
  if (name == "GREEN")  return M5.Display.color565( 40, 200,  40);
  if (name == "BLUE")   return M5.Display.color565( 40,  40, 220);
  if (name == "YELLOW") return M5.Display.color565(240, 220,  20);
  if (name == "CYAN")   return M5.Display.color565( 20, 210, 210);
  if (name == "PURPLE") return M5.Display.color565(160,  40, 200);
  if (name == "PINK")   return M5.Display.color565(230,  80, 160);
  if (name == "WHITE")  return M5.Display.color565(240, 240, 240);
  if (name == "GRAY")   return M5.Display.color565(120, 120, 120);
  return TFT_BLACK;
}

uint32_t getTextColor(const String& name) {
  if (name == "WHITE" || name == "YELLOW" || name == "CYAN" || name == "GREEN")
    return TFT_BLACK;
  return TFT_WHITE;
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  Serial.begin(115200);

  M5.Power.setExtOutput(true);
  delay(200);

  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(10, 10);
  M5.Display.println("Color Sensor Init...");

  Wire.end();
  delay(50);
  Wire.begin(SENSOR_SDA, SENSOR_SCL);

  if (!tcs.begin(TCS34725_ADDRESS, &Wire)) {
    M5.Display.fillScreen(TFT_RED);
    M5.Display.setCursor(10, 80);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.println("Sensor NOT FOUND!");
    while (1) delay(100);
  }

  M5.Display.println("Sensor OK!");
  Serial.println("[OK] TCS34725 found!");
  delay(800);
}

void loop() {
  M5.update();

  uint16_t r_raw, g_raw, b_raw, c_raw;
  tcs.getRawData(&r_raw, &g_raw, &b_raw, &c_raw);

  // ── 1단계: WB 보정을 raw에 먼저 적용 ──────────────
  float r_wb = r_raw * WB_R;
  float g_wb = g_raw * WB_G;
  float b_wb = b_raw * WB_B;

  // ── 2단계: 보정된 값의 최댓값으로 정규화 ──────────
  float maxWB = max(r_wb, max(g_wb, b_wb));
  uint8_t rc = 0, gc = 0, bc = 0;
  if (maxWB > 0) {
    rc = (uint8_t)min(255.0f, r_wb * 255.0f / maxWB);
    gc = (uint8_t)min(255.0f, g_wb * 255.0f / maxWB);
    bc = (uint8_t)min(255.0f, b_wb * 255.0f / maxWB);
  }

  String   colorName = getColorName(rc, gc, bc, c_raw);
  uint32_t bgColor   = getBgColor(colorName);
  uint32_t textColor = getTextColor(colorName);

  M5.Display.fillScreen(bgColor);
  M5.Display.setTextColor(textColor);

  // 색상 이름
  M5.Display.setTextSize(5);
  int textW   = (int)colorName.length() * 30;
  int cursorX = (M5.Display.width() - textW) / 2;
  if (cursorX < 0) cursorX = 0;
  M5.Display.setCursor(cursorX, 50);
  M5.Display.print(colorName);

  // RGB 수치 (보정 후)
  M5.Display.setTextSize(2);
  M5.Display.setCursor(15, 145);
  M5.Display.printf("R:%3d  G:%3d  B:%3d", rc, gc, bc);

  // 원본 raw
  M5.Display.setCursor(15, 168);
  M5.Display.printf("raw %5d %5d %5d", r_raw, g_raw, b_raw);

  // C값
  M5.Display.setCursor(15, 191);
  M5.Display.printf("C:%5d", c_raw);

  // 미리보기
  uint32_t previewColor = M5.Display.color565(rc, gc, bc);
  M5.Display.fillRect(15, 210, 110, 50, previewColor);
  M5.Display.drawRect(15, 210, 110, 50, textColor);
  M5.Display.setCursor(135, 228);
  M5.Display.print("Preview");

  // 시리얼 디버그
  float rf2 = rc/255.0f, gf2 = gc/255.0f, bf2 = bc/255.0f;
  float mx = max(rf2, max(gf2, bf2));
  float mn = min(rf2, min(gf2, bf2));
  float d  = mx - mn;
  float h2 = 0, s2 = (mx > 0) ? d/mx : 0;
  if (d > 0) {
    if      (mx == rf2) h2 = 60.0f * fmod((gf2-bf2)/d, 6.0f);
    else if (mx == gf2) h2 = 60.0f * ((bf2-rf2)/d + 2.0f);
    else                h2 = 60.0f * ((rf2-gf2)/d + 4.0f);
    if (h2 < 0) h2 += 360.0f;
  }

  // 포화 경고
  bool saturated = (r_raw >= 60000 || g_raw >= 60000 || b_raw >= 60000);
  Serial.printf("%-8s | Rc:%3d Gc:%3d Bc:%3d | raw R:%5d G:%5d B:%5d | C:%5d | H:%6.1f S:%.2f V:%.2f%s\n",
                colorName.c_str(), rc, gc, bc,
                r_raw, g_raw, b_raw, c_raw,
                h2, s2, mx,
                saturated ? " [SAT!]" : "");

  delay(300);
}