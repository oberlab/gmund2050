#define ROTATE_DISPLAY true

#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
#include "config.h"

RTC_DS3231 rtc;
Adafruit_7segment display = Adafruit_7segment();

// --- wir wollen das display 180 grad drehen damit der doppelpunkt hinten ist! ---
// A=bit0 … G=bit6, DP=bit7
#ifdef ROTATE_DISPLAY
static const uint8_t ROTATED_SEGMENTS[10] = {
  0x3F, // 0 -> 0x3F (symmetrisch)
  0x30, // 1 -> 0x06  wird zu 0x30
  0x5B, // 2 -> 0x5B  wird zu 0x6D
  0x79, // 3 -> 0x4F  wird zu 0x79
  0x74, // 4 -> 0x66  wird zu 0x72
  0x6D, // 5 -> 0x6D  wird zu 0x5B
  0x6F, // 6 -> 0x7D  wird zu 0x6F
  0x38, // 7 -> 0x07  wird zu 0x38
  0x7F, // 8 -> 0x7F  (symmetrisch)
  0x7D  // 9 -> 0x6F  wird zu 0x7D
};

void print_inverted(int value) {
  display.displaybuffer[0] = 0;
  display.displaybuffer[1] = 0;
  display.displaybuffer[3] = 0;
  display.displaybuffer[4] = 0;

  for (uint8_t pos = 0; pos < 4; pos++) {
    int digit = value % 10;
    value -= digit;
    value /= 10;
    uint8_t inv = ROTATED_SEGMENTS[digit];
    uint8_t pos_array = pos;
    if (pos >= 2)
      pos_array++;
    display.displaybuffer[pos_array] = inv;
    if (value == 0)
      return;
  }
}
#endif

inline void update_number(int num){
#ifdef ROTATE_DISPLAY
  print_inverted(num);
#else
  display.print(num);
#endif
}

DateTime start(2000, 1, 1, 0, 0, 0);

// Globale Variablen für den Blink-State
unsigned long lastBlinkTime = 0;
bool blinkState = false;

void setup() {
  pinMode(BUTTON_PLUS, INPUT_PULLUP);
  pinMode(BUTTON_MINUS, INPUT_PULLUP);

  Wire.begin();
  rtc.begin();
  display.begin(DISPLAY_I2C_ADDRESS);
  display.setBrightness(DISPLAY_BRIGHTNESS);

  delay(100);

#ifdef DEBUG_NUMBERS
  update_number(1234);
  display.writeDisplay();
  delay(5000);
   update_number(5678);
  display.writeDisplay();

  delay(5000);
   update_number(90);
  display.writeDisplay();
  delay(5000);
#endif

  bool plusPressed = digitalRead(BUTTON_PLUS) == LOW;
  bool minusPressed = digitalRead(BUTTON_MINUS) == LOW;

  if (plusPressed && minusPressed) {
    rtc.adjust(start); // Setze auf "Tag 0"
    update_number(MAX_DAYS);
    display.writeDisplay();
    delay(2000);
  }
}

// --- Helferfunktion zur Berechnung ---
int calculateDaysLeft() {
  DateTime now = rtc.now();
  TimeSpan elapsed = now - start;

  int daysElapsed = elapsed.days();
  int daysLeft = MAX_DAYS - daysElapsed;

  return (daysLeft < 0) ? 0 : daysLeft;
}

void loop() {
  static int hw_access_minimizer_count = 100;
  bool update_needed = false;
  bool plusPressed = digitalRead(BUTTON_PLUS) == LOW;
  bool minusPressed = digitalRead(BUTTON_MINUS) == LOW;

  // --- Buttonaktionen ---
  if (plusPressed) {
    DateTime now = rtc.now();
    rtc.adjust(now - TimeSpan(1, 0, 0, 0)); // -1 Tag
    update_needed = true;
  } else if (minusPressed) {
    DateTime now = rtc.now();
    rtc.adjust(now + TimeSpan(1, 0, 0, 0)); // +1 Tag
    update_needed = true;
  }

  if ((hw_access_minimizer_count==100) || update_needed){
     // --- Anzeige aktualisieren ---
     int daysLeft = calculateDaysLeft();
     update_number(daysLeft);
     update_needed = true;
     hw_access_minimizer_count = 0;
  } else hw_access_minimizer_count++;

  // Blink-Status: Alle 1000 ms umschalten
  unsigned long currentMillis = millis();
  if (currentMillis - lastBlinkTime >= 500) {
    lastBlinkTime = currentMillis;
    blinkState = !blinkState;
    update_needed = true;
  }

  // Setze oder lösche den Punkt im ersten Ziffernfeld abhängig vom Blink-State.
  // Annahme: Bit 0x80 in display.displaybuffer[0] entspricht dem Punkt.
#ifdef ROTATE_DISPLAY
  if (blinkState) {
    display.displaybuffer[2] = 4;  // Punkt einschalten
  } else {
    display.displaybuffer[2] = 0; // Punkt ausschalten
  }
#else
  if (blinkState) {
    display.displaybuffer[4] |= 0x80;  // Punkt einschalten
  } else {
    display.displaybuffer[4] &= ~0x80; // Punkt ausschalten
  }
#endif
  if (update_needed)
    display.writeDisplay();

  delay(50);
}
