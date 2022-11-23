#include "Arduino.h"
#include <pins_arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pinbelegung
const byte PIN_DISPLAY_SWITCH = A1;
const byte PIN_TEMP_SENSOR = A3;  // Pin fuer TempInput
const byte PIN_RELAIS_PUMP = A2;  // Pin fuer Reilais
const byte PIN_LED = LED_BUILTIN; // Pin fuer StatusLED

// Display
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Zeit
const uint16_t CYCLE_PERIOD_MS = 1000; // Zeitintervall

// Temperatur-Grenzwerte
const uint8_t TMP_LIMIT_LOWER = 28; // Threshold fuer Rampup
const uint8_t TMP_LIMIT_UPPER = 60; // Threshold fuer Rampup

const uint16_t MIN_ON_TIME_S = 5 * 60; // 5 minutes

// Messwerte
typedef float tmp_t;

const uint8_t DIVISOR_EXPONENTIAL_FILTER = 16;

#define LCD_CURSORPOS_VALUE 13


void setup()
{

	// Serielle Kommunikation
	// Serial.begin(115200);

	// IO-Modes
	pinMode(PIN_RELAIS_PUMP, OUTPUT);
	pinMode(PIN_LED, OUTPUT);
	pinMode(PIN_DISPLAY_SWITCH, INPUT_PULLUP);

	// lcd
	lcd.init();

	lcd.setCursor(0, 0);
	lcd.print("Toff: <");
	lcd.print(TMP_LIMIT_LOWER);
	lcd.print(" P: ");
	lcd.println();

	lcd.setCursor(0, 1);
	lcd.print("TReg: <");
	lcd.print(TMP_LIMIT_UPPER);
	lcd.print(" T: ");

	// Serial.begin(9600);
}

void loop()
{
	// Timing
	static unsigned long int timePrev;   		
	unsigned long int timeNow = millis();  

	// Temperature
	static tmp_t tmp;			// Durchschnittstemperatur
	static tmp_t tmpDiff;		// Durchschnittstemperaturänderung
	static tmp_t tmpPrev; 		// Vorherige Temperatur

	// Pump
	static bool pumpOff = false;	

	// Pump control @1s
	if ((timeNow - timePrev) >= CYCLE_PERIOD_MS)
	{
		// Read and smooth temperature
		tmp_t tmpRead = analogRead(PIN_TEMP_SENSOR);
		tmpRead = tmpRead / 1024 * 5.0 / 1.5 * 150;

		tmp = (tmpRead + tmp * (DIVISOR_EXPONENTIAL_FILTER - 1)) / DIVISOR_EXPONENTIAL_FILTER;
		tmpDiff = ((tmp - tmpPrev) + tmpDiff * (DIVISOR_EXPONENTIAL_FILTER - 1)) / DIVISOR_EXPONENTIAL_FILTER;

		// Serial.print("avg: ");
		// Serial.print(tmp);

		static uint16_t minOnTime;		// Forced pump on time on activation
		pumpOff = false;				// Default on/active off

		// check hard limits
		if (TMP_LIMIT_LOWER > tmp)
		{
			// pump off
			pumpOff = true;
			minOnTime = 0; // deactivate min on time if pump is off
		}
		else if (TMP_LIMIT_UPPER <= tmp)
		{
			// leave pump on
			minOnTime = 0; // deactivate min on time to start at 0 when entering mid temperature band from high band
		}
		else
		{
			/* check gradient for temperature between hard limits
			 * Due to minOnTime, if a rising temperature has been detected and pump was activated,
			 * the next evaluation of tmpDiff is after minOnTime.
			 */
			
			if (0 < minOnTime)
			{
				// min on time active
				minOnTime--;
			}
			else if (tmpDiff > 0)
			{
				// minimal on time passed and rising temperature --> leave pump on at least for minimal on time
				minOnTime = MIN_ON_TIME_S;
			}
			else
			{
				// minimal on time passed and temperature unchanged or falling --> turn pump off
				pumpOff = true;
			}
		}

		// Serial.print("   minOnTime: ");
		// Serial.println(minOnTime);

		// Write output pin
		digitalWrite(PIN_RELAIS_PUMP, pumpOff);

		timePrev = timeNow;
		tmpPrev = tmp;
	} // Intervallende

	// Display
	if (digitalRead(PIN_DISPLAY_SWITCH))
	{
		lcd.setCursor(LCD_CURSORPOS_VALUE, 0);
		lcd.print(pumpOff == true ? "OFF" : "ON ");
		
		lcd.setCursor(LCD_CURSORPOS_VALUE, 1);
		lcd.print(round(tmp));
				
		lcd.backlight();
	}
	else
	{
		lcd.noBacklight();
	}
}
