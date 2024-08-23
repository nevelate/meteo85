#include "src/GyverPower/src/GyverPower.h"
#include "src/GyverOLED/src/GyverOLED.h"
#include "src/GyverBME280/src/GyverBME280.h"
#include "src/microDS3231/src/microDS3231.h"

MicroDS3231 rtc;
GyverOLED oled;
GyverBME280 bme;

#define PWR 1  // MOSFET transistor Gate pin

#define X 128
#define Y 64

#define DISPLAY_MODE 1  // 1, 2 or 3

#define UPDATE_PERIOD 1000
#define STANDBY_PERIOD 10000

uint8_t temp, humidity;
uint16_t pressurePa;
uint32_t standbyTimer, updateTimer;

void setup() {
  pinMode(3, INPUT);     // Interrupt pin
  pinMode(PWR, OUTPUT);  // MOSFET transistor Gate pin
  digitalWrite(PWR, HIGH);

  GIMSK = 0b00100000;  // Enable pin change interrupt
  PCMSK = 0b00001000;  // Pin change interrupt for PB3

  power.sleep(SLEEP_FOREVER);
}

void loop() {
  if ((long)millis() - updateTimer > UPDATE_PERIOD) {  // update every UPDATE_PERIOD
    updateTimer = millis();

    temp = (uint8_t)bme.readTemperature();
    humidity = (uint8_t)bme.readHumidity();

    pressurePa = (uint8_t)bme.readPressure();

    display();
  }

  if ((long)millis() - standbyTimer > STANDBY_PERIOD) {
    digitalWrite(PWR, LOW);
    power.sleep(SLEEP_FOREVER);  // sleep after STANDBY_PERIOD
  }
}

#if (DISPLAY_MODE == 1)
void display() {
  oled.fastLineH(Y / 2, 0, X);
  oled.fastLineV(X / 2, 0, Y);

  oled.setScale(2);
  oled.setCursorXY(20, 3 >> 3);
  oled.print(temp);
  oled.print(F(" C"));

  oled.setCursorXY(20, 7 >> 3);
  oled.print(humidity);
  oled.print(F(" %"));

  oled.setCursorXY(84, 7 >> 3);
  oled.print(pressurePa);
  oled.print(F(" PA"));

  oled.setScale(1);
  oled.setCursorXY(84, 2 >> 3);
  oled.print(rtc.getHours());
  oled.print(":");
  oled.print(rtc.getMinutes());

  oled.setCursorXY(84, 3 >> 3);
  oled.print(rtc.getDate());
  oled.print(":");
  oled.print(rtc.getMonth());  
  oled.print(":");
  oled.print(rtc.getYear());
}

#elif (DISPLAY_MODE == 2)
void display() {
}

#elif (DISPLAY_MODE == 3)
void display() {
}
#endif

ISR(PCINT0_VECT) {
  power.wakeUp();
  digitalWrite(PWR, HIGH);
  standbyTimer = millis();

  //--------Initializing parts----------
  rtc.begin();
  oled.init();
  bme.begin();
}