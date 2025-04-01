#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
#include "config.h"

RTC_DS3231 rtc;
Adafruit_7segment display = Adafruit_7segment();

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
     display.print(daysLeft);
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
  if (blinkState) {
    display.displaybuffer[4] |= 0x80;  // Punkt einschalten
  } else {
    display.displaybuffer[4] &= ~0x80; // Punkt ausschalten
  }

  if (update_needed)
    display.writeDisplay();

  delay(50);
}
