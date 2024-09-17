#include <avr/sleep.h>
#include <avr/interrupt.h>

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

bool isLowBattery;

uint8_t temp, humidity, pressure;
uint32_t standbyTimer, updateTimer;

void setup() {
  pinMode(3, INPUT_PULLUP);      // Interrupt pin
  pinMode(GATE, OUTPUT);  // MOSFET transistor Gate pin
  digitalWrite(GATE, LOW);

  sleep();
}

void loop() {
  if ((long)millis() - updateTimer > UPDATE_PERIOD) {  // update every UPDATE_PERIOD
    updateTimer = millis();

    temp = (uint8_t)bme.readTemperature();
    humidity = (uint8_t)bme.readHumidity();

    pressure = bme.readPressure() / 1000;

    if (isLowBattery ^ readVcc() < 2800) oled.clear();
    if (readVcc() < 2800) showLowBatteryAlert();
    else display();
  }

  if ((long)millis() - standbyTimer > STANDBY_PERIOD) {
    digitalWrite(GATE, LOW);
    sleep();  // sleep after STANDBY_PERIOD
  }
}

#if (DISPLAY_MODE == 1)
void display() {
  isLowBattery = false;

  oled.drawLineH(Y / 2, 0, X);
  oled.drawLineV(X / 2, 0, Y);

  // Temperature
  oled.setTextScale(2);
  oled.setCursor(9, 10);
  printNumber(temp);

  oled.setCursor(45, 10);
  oled.printCharFast('C');

  // Humidity
  oled.setCursor(9, 42);
  printNumber(humidity);

  oled.setCursor(45, 42);
  oled.printCharFast('%');

  // Date / Time
  oled.setTextScale(1);
  oled.setCursor(81, 10);
  printNumber(rtc.getHours());
  oled.printCharFast(':');
  printNumber(rtc.getMinutes());

  char date[10];
  rtc.getDateChar(date);
  oled.setCursor(66, 18);
  oled.printFast(date);

  // Pressure
  oled.setTextScale(2);
  oled.setCursor(70, 42);
  printNumber(pressure);
  oled.printFast(" k");
}

#elif (DISPLAY_MODE == 2)
void display() {
}

#elif (DISPLAY_MODE == 3)
void display() {
}
#endif

void showLowBatteryAlert() {
  isLowBattery = true;

  oled.setTextScale(3);
  oled.setCursor(37, 20);
  oled.printFast("LOW");

  oled.setTextScale(1);
  oled.setCursor(40, 34);
  oled.printFast("BATTERY!");
  oled.setCursor(52, 42);
  printNumber(readVcc());
}

void printNumber(unsigned int number) {
  uint8_t digits[10];
  uint8_t amount;
  for (uint8_t i = 0; i < 10; i++) {
    digits[i] = number % 10;
    number /= 10;
    if (number == 0) {
      amount = i;
      break;
    }
  }
  for (uint8_t i = amount; i >= 0; i--) {
    oled.printCharFast(digits[i] + 48);
  }
}

ISR(PCINT0_vect) {
  digitalWrite(GATE, HIGH);
  standbyTimer = millis();

  //--------Initializing parts----------
  rtc.begin();
  bme.begin();
  oled.init();
}

void sleep() {
  GIMSK |= _BV(PCIE);    // Enable Pin Change Interrupts
  PCMSK |= _BV(PCINT3);  // Use PB3 as interrupt pin
  ADCSRA &= ~_BV(ADEN);  // ADC off (saves power)
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  sleep_enable();  // Sets the Sleep Enable bit in the MCUCR Register (SE BIT)
  sei();           // Enable interrupts
  sleep_cpu();     // Put the microcontroller to sleep

  // Code execution stops here since the microcontroller is now asleep.  When the button connected to PB3
  // is pressed the microcontroller will awake, run the code in the interrupt service routine (ISR),
  // and then continue executing the code starting from here.

  cli();  // Disable interrupts

  PCMSK &= ~_BV(PCINT3);  // Turn off PB3 as interrupt pin
  sleep_disable();        // Clear SE bit
  ADCSRA |= _BV(ADEN);    // ADC on (we turned it off the save power)

  sei();  // Enable interrupts
}

long readVcc() {
  ADMUX = _BV(MUX3) | _BV(MUX2); // Attiny 25/45/85

  delay(2);             // Wait for Vref to settle
  ADCSRA |= _BV(ADSC);  // Start conversion
  while (bit_is_set(ADCSRA, ADSC))
    ;                   // measuring
  uint8_t low = ADCL;   // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH;  // unlocks both
  long result = (high << 8) | low;
  result = 1.1 * 1023 * 1000 / result;
  return result;
}