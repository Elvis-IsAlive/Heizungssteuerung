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
const uint8_t TMP_LIMIT_LOWER = 30; // Threshold fuer Rampup
const uint8_t TMP_LIMIT_UPPER = 60; // Threshold fuer Rampup

const uint16_t MIN_ON_TIME_S = 2 * 60; // 2 minutes

// Messwerte
typedef float tmp_t;

#if CYCLE_PERIOD_MS % 1000 != 0
#error "Invalid cycle period"
#endif 

const uint8_t DIVISOR_EXPONENTIAL_FILTER = 16 / (CYCLE_PERIOD_MS / 1000); // Running average over last n s

#define LCD_CURSORPOS_VALUE 12
// #define DEBUG

void setup()
{
	// IO-Modes
	pinMode(PIN_RELAIS_PUMP, OUTPUT);
	pinMode(PIN_LED, OUTPUT);
	pinMode(PIN_DISPLAY_SWITCH, INPUT_PULLUP);

	// lcd
	lcd.init();

	lcd.setCursor(0, 0);
	lcd.print("Toff <");
	lcd.print(TMP_LIMIT_LOWER);
	lcd.print("  P ");
	lcd.println();

	lcd.setCursor(0, 1);
	lcd.print("TReg <");
	lcd.print(TMP_LIMIT_UPPER);
	lcd.print("  T ");

#ifdef DEBUG
	Serial.begin(9600);
#endif
}

void loop()
{
	// Timing
	static unsigned long int timePrev;   		
	unsigned long int timeNow = millis();  

	// Temperature
	static tmp_t tmp;			// Durchschnittstemperatur
	static tmp_t tmpMin;		// Mindesttemperatur
	static tmp_t tmpMax;		// Maximaltemperatur

	// Pump
	static bool pumpOff = false;	

	// Pump control @1s
	if ((timeNow - timePrev) >= CYCLE_PERIOD_MS)
	{
		// Read and smooth temperature
		tmp_t tmpRead = analogRead(PIN_TEMP_SENSOR);
		tmpRead = (tmpRead * 5.0 * 150.0) / 1024 / 1.5;

		tmp = (tmpRead + tmp * (DIVISOR_EXPONENTIAL_FILTER - 1)) / DIVISOR_EXPONENTIAL_FILTER; 

		static uint16_t minOnTime;		// Forced pump on time on activation
		static bool peakDetected = false;
		pumpOff = false;				// Default on
		const tmp_t TMP_TOLERANCE = 3;	// Degrees


		if ( TMP_LIMIT_LOWER > tmp)
		{
			// OFF
			pumpOff = true;
			minOnTime = 0;
			tmpMax = TMP_LIMIT_LOWER;	// Set maximum to lower limit to follow warm up
			tmpMin = TMP_LIMIT_LOWER;
			peakDetected = false;
		}
		else 
		{
			// Control temperature 

			if ( (tmpMax - tmp) > TMP_TOLERANCE)
			{
				// Cool down/after peak --> check for rising temp

				if (!peakDetected)
				{
					tmpMin = tmp;	
					minOnTime = 0;
					peakDetected = true;
				}

				if ( (tmp - tmpMin ) > TMP_TOLERANCE)
				{
					// Temp rising after peak --> ON
					minOnTime = MIN_ON_TIME_S;	// reactivate minOnTime
					tmpMax = tmp;
					peakDetected = false;
				}		
				else if (tmp < TMP_LIMIT_UPPER)
				{
					// Temp below upper limit and falling --> OFF
					pumpOff = true;
				}	
				else
				{
					// ON
				}
			}
			else
			{
				// Before peak --> ON
				if (minOnTime == 0) 
				{ 
					minOnTime = MIN_ON_TIME_S;	// reactivate minOnTime
				}
			}

			if (tmp > tmpMax)
			{
				tmpMax = tmp;
			}

			if (tmp < tmpMin)
			{
				tmpMin = tmp;
			}
		}

		if ( minOnTime > 0)
		{
			// Min on time active --> ON
			--minOnTime;
			pumpOff = false;
		}

		digitalWrite(PIN_RELAIS_PUMP, pumpOff);
		timePrev = timeNow;

#ifdef DEBUG
		Serial.print("t: ");
		Serial.print(tmpRead);
		Serial.print("\tavg: ");
		Serial.print(tmp);
		Serial.print("\tt_min: ");
		Serial.print(tmpMin);
		Serial.print("\tt_max: ");
		Serial.print(tmpMax);
		Serial.print("\tpeakDetected: ");
		Serial.print(peakDetected ? "YES" : "NO");
		Serial.print("\tpump: ");
		Serial.print(pumpOff ? "OFF" : "ON");
		Serial.print("\tminOnTime: ");
		Serial.println(minOnTime);
#endif
	} // Intervallende

	// Display
	if (digitalRead(PIN_DISPLAY_SWITCH))
	{
		lcd.setCursor(LCD_CURSORPOS_VALUE, 0);
		lcd.print(pumpOff == true ? "OFF" : "ON ");
		
		lcd.setCursor(LCD_CURSORPOS_VALUE, 1);
		lcd.print(tmp, 1);
				
		lcd.backlight();
	}
	else
	{
		lcd.noBacklight();
	}
}
