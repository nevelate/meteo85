#include <GyverPower.h>
#include <GyverBME280.h>
#include <microDS3231.h>
#include <tiny1106.h>

MicroDS3231 rtc;
Oled oled;
GyverBME280 bme;

#define GATE 1  // MOSFET transistor Gate pin

#define X 128
#define Y 64

#define DISPLAY_MODE 1  // 1, 2 or 3

#define UPDATE_PERIOD 1000
#define STANDBY_PERIOD 15000

bool isSleeping;

uint8_t temp, humidity;
uint16_t pressurePa;
uint32_t standbyTimer, updateTimer;

void setup() {
  pinMode(3, INPUT);      // Interrupt pin
  pinMode(GATE, OUTPUT);  // MOSFET transistor Gate pin
  digitalWrite(GATE, HIGH);

  GIMSK = 0b00100000;  // Enable pin change interrupt
  PCMSK = 0b00001000;  // Pin change interrupt for PB3

  isSleeping = true;
  power.sleep(SLEEP_FOREVER);
}

void loop() {
  if ((long)millis() - updateTimer > UPDATE_PERIOD) {  // update every UPDATE_PERIOD
    updateTimer = millis();

    temp = (uint8_t)bme.readTemperature();
    humidity = (uint8_t)bme.readHumidity();

    pressurePa = (uint8_t)bme.readPressure();

    if (readVcc() < 2800) showLowBatteryAlert();
    else display();
  }

  if ((long)millis() - standbyTimer > STANDBY_PERIOD) {
    isSleeping = true;
    digitalWrite(GATE, LOW);
    power.sleep(SLEEP_FOREVER);  // sleep after STANDBY_PERIOD
  }
}

#if (DISPLAY_MODE == 1)
void display() {
  oled.clear();
  oled.drawLineH(32, 10, 110);
  oled.drawLineV(64, 20, 50);
  oled.printFast("F");
  oled.printCharFast('F');
}

#elif (DISPLAY_MODE == 2)
void display() {
}

#elif (DISPLAY_MODE == 3)
void display() {
}
#endif

void showLowBatteryAlert() {
}

ISR(PCINT0_VECT) {
  if (isSleeping) {
    power.wakeUp();
    digitalWrite(GATE, HIGH);
    standbyTimer = millis();
    isSleeping = false;

    //--------Initializing parts----------
    rtc.begin();
    bme.begin();
    oled.init();
  }
}

long readVcc() {
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif
  delay(2);             // Wait for Vref to settle
  ADCSRA |= _BV(ADSC);  // Start conversion
  while (bit_is_set(ADCSRA, ADSC))
    ;                   // measuring
  uint8_t low = ADCL;   // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH;  // unlocks both
  long result = (high << 8) | low;
  result = 1.1 * 1023 * 1000 / result;  // расчёт реального VCC
  return result;                        // возвращает VCC
}