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
const uint8_t TMP_LIMIT_UPPER = 55; // still hot ambers at 53.5 degrees
#define MIN_ON_TIME_MINUTES 5
const uint16_t MIN_ON_TIME_S = MIN_ON_TIME_MINUTES * 60; // 2 minutes

// Messwerte
typedef float tmp_t;
const tmp_t TMP_DELTA_ON = 3;	 // Degrees
const tmp_t TMP_DELTA_OFF = -10; // Degrees

const uint8_t LIMIT = 30;

#if CYCLE_PERIOD_MS % 1000 != 0
#error "Invalid cycle period"
#endif

const uint8_t DIVISOR_EXPONENTIAL_FILTER = 32 / (CYCLE_PERIOD_MS / 1000); // Running average over last n s

#define LCD_CURSORPOS_TMP 3
#define LCD_CURSORPOS_PUMP 12
#define LCD_CURSORPOS_MINONTIME 10
#define DEBUG

void setup()
{
	// IO-Modes
	pinMode(PIN_RELAIS_PUMP, OUTPUT);
	pinMode(PIN_LED, OUTPUT);
	pinMode(PIN_DISPLAY_SWITCH, INPUT_PULLUP);

	// lcd
	lcd.init();

	lcd.setCursor(0, 0);
	lcd.print("T: ");
	lcd.setCursor(LCD_CURSORPOS_TMP, 0);
	lcd.print(" 0.0");
	lcd.print("  P: ");
	lcd.setCursor(LCD_CURSORPOS_PUMP, 0);
	lcd.print("OFF");

	lcd.setCursor(0, 1);
	lcd.print(TMP_DELTA_ON, 1);
	lcd.print("/");
	lcd.print(TMP_DELTA_OFF, 1);
	lcd.print(" ");
	// lcd.setCursor(LCD_CURSORPOS_MINONTIME, 1);
	// lcd.print(MIN_ON_TIME_S);
	// lcd.print("s");

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
	static tmp_t tmp;	 // Durchschnittstemperatur
	static tmp_t tmpMin; // Mindesttemperatur
	static tmp_t tmpMax; // Maximaltemperatur

	// Pump
	bool pumpOff;
	static bool pumpOffPrev = false;
	static uint16_t minOnTime = 0; // Forced pump on time on activation
	static bool peakDetected = false;
	tmp_t tmpRead;

	// Pump control @1s
	if ((timeNow - timePrev) >= CYCLE_PERIOD_MS)
	{
		// Read and smooth temperature
		tmpRead = analogRead(PIN_TEMP_SENSOR);
		tmpRead = (tmpRead * 5.0 * 150.0) / 1024 / 1.5;
		tmp = (tmpRead + tmp * (DIVISOR_EXPONENTIAL_FILTER - 1)) / DIVISOR_EXPONENTIAL_FILTER;

		pumpOff = false;

		if (TMP_LIMIT_LOWER > tmp)
		{
			// OFF
			pumpOff = true;
			tmpMax = TMP_LIMIT_LOWER; // Set maximum to lower limit to follow warm up
			tmpMin = TMP_LIMIT_LOWER;
			peakDetected = false;
		}
		else
		{
			// Control temperature

			if ((tmp - tmpMax) <= TMP_DELTA_OFF)
			{
				// Cool down/after peak --> check for rising temp
				static uint8_t cnt = 0;

				if (!peakDetected)
				{
					tmpMin = tmp;
					cnt = 0;
					peakDetected = true;
				}

				if ((tmp - tmpMin) > TMP_DELTA_ON)
				{
					// Temp rising after peak --> ON
					tmpMax = tmp;
					peakDetected = false;
				}
				else if (tmp < TMP_LIMIT_UPPER)
				{
					if ( LIMIT > cnt)
					{
						++cnt;
						// Keep ON
					}
					else
					{
						// Temp below upper limit and falling --> OFF
						pumpOff = true;
					}
				}
				else
				{
					// ON
				}
			}
			else
			{
				// Before peak --> ON
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

		if (pumpOffPrev == true && pumpOff == false)
		{
			// Pump is turned on
			minOnTime = MIN_ON_TIME_S; // reactivate minOnTime
		}

		if (minOnTime > 0)
		{
			// Min on time active --> ON
			--minOnTime;
			pumpOff = false;
		}

		digitalWrite(PIN_RELAIS_PUMP, pumpOff);
		timePrev = timeNow;
		pumpOffPrev = pumpOff;

#ifdef DEBUG
		if (digitalRead(PIN_DISPLAY_SWITCH))
		{
			Serial.print("t: ");
			Serial.print(tmpRead);
			Serial.print(" \tavg: ");
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
		}
#endif
	} // Intervallende

	// Display
	if (digitalRead(PIN_DISPLAY_SWITCH))
	{
		lcd.setCursor(LCD_CURSORPOS_TMP, 0);

		if ((uint8_t)tmp < 10)
		{
			lcd.print(" ");
		}

		lcd.print(tmp, 1);
		lcd.print(" ");

		lcd.setCursor(LCD_CURSORPOS_PUMP, 0);
		lcd.print(pumpOff ? "OFF" : "ON ");

		lcd.setCursor(LCD_CURSORPOS_MINONTIME, 1);
		if (minOnTime < 100)
		{
			lcd.print(" ");
		}
		if (minOnTime < 10)
		{
			lcd.print(" ");
		}
		lcd.print(minOnTime);
		lcd.print("s");

		lcd.backlight();
	}
	else
	{
		lcd.noBacklight();
	}
}
