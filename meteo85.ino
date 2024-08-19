#include "GyverOLED.h"
#include "GyverBME280.h"
#include "GyverPower.h"
#include "microDS3231.h"

MicroDS3231 rtc;
GyverOLED<SSH1106_128x64, OLED_NO_BUFFER> oled;
GyverBME280 bme;

#define X 128
#define Y 64

#define STANDBY_PERIOD 10000

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
