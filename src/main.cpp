#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
#include "config.h"

RTC_DS3231 rtc;
Adafruit_7segment display = Adafruit_7segment();

DateTime start(2000, 1, 1, 0, 0, 0);

void setup() {
  pinMode(BUTTON_PLUS, INPUT_PULLUP);
  pinMode(BUTTON_MINUS, INPUT_PULLUP);

  Wire.begin();
  rtc.begin();
  display.begin(DISPLAY_I2C_ADDRESS);
  display.setBrightness(DISPLAY_BRIGHTNESS);

  delay(100);

  bool plusPressed = digitalRead(BUTTON_PLUS) == LOW;
  bool minusPressed = digitalRead(BUTTON_MINUS) == LOW;

  if (plusPressed && minusPressed) {
    rtc.adjust(start); // Setze auf "Tag 0"
    display.print(MAX_DAYS);
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
  bool plusPressed = digitalRead(BUTTON_PLUS) == LOW;
  bool minusPressed = digitalRead(BUTTON_MINUS) == LOW;

  // --- Buttonaktionen ---
  if (plusPressed) {
    DateTime now = rtc.now();
    rtc.adjust(now - TimeSpan(1, 0, 0, 0)); // -1 Tag
  } else if (minusPressed) {
    DateTime now = rtc.now();
    rtc.adjust(now + TimeSpan(1, 0, 0, 0)); // +1 Tag
  }

  // --- Anzeige aktualisieren ---
  int daysLeft = calculateDaysLeft();
  display.print(daysLeft);
  display.writeDisplay();

  // --- Verzögerung je nach Zustand ---
  if (plusPressed || minusPressed) {
    delay(500);  // Wenn Knopf gedrückt, kurze Reaktionszeit
  } else {
    delay(60000); // Kein Knopf gedrückt → 1x pro Minute aktualisieren
  }
}
