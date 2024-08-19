#include "src/GyverPower/src/GyverPower.h"
#include "src/GyverOLED/src/GyverOLED.h"
#include "src/GyverBME280/src/GyverBME280.h"
#include "src/microDS3231/src/microDS3231.h"

MicroDS3231 rtc;
GyverOLED<SSH1106_128x64, OLED_NO_BUFFER> oled;
GyverBME280 bme;

#define PWR 1 // MOSFET transistor Gate pin

#define X 128
#define Y 64

#define DISPLAY_MODE 1 // 1, 2 or 3

#define UPDATE_PERIOD 1000
#define STANDBY_PERIOD 10000

uint8_t temp, humidity;
uint16_t pressureMmHg, pressurePa;
uint32_t standbyTimer, updateTimer;
DateTime now;

void setup() {
  pinMode(3, INPUT);  // Interrupt pin

  GIMSK = 0b00100000; // Enable pin change interrupt
  PCMSK = 0b00001000; // Pin change interrupt for PB3

  power.sleep(SLEEP_FOREVER);
}

void loop() {
  standbyTimer = millis();

  //--------Initializing parts----------
  rtc.begin();
  oled.init();
  bme.begin();

  if ((long)millis() - updateTimer > UPDATE_PERIOD) { // update every UPDATE_PERIOD
    updateTimer = millis();
    // do smth  
    now = rtc.getTime();
    
    temp = (uint8_t)bme.readTemperature();
    humidity = (uint8_t)bme.readHumidity();

    pressurePa = (uint8_t)bme.readPressure();
    pressureMmHg = (uint8_t)pressureToMmHg(pressurePa);

    display(); 
  }

  if ((long)millis() - standbyTimer > STANDBY_PERIOD)
  {
     power.sleep(SLEEP_FOREVER); // sleep after STANDBY_PERIOD
  }
}

#if (DISPLAY_MODE == 1)
void display() {
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
}