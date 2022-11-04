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
unsigned long int now = 0, then = 0;   // Zeitvariablen
const uint16_t CYCLE_PERIOD_MS = 1000; // Zeitintervall

// Temperatur-Grenzwerte
const uint8_t TMP_LIMIT_LOWER = 28; // Threshold fuer Rampup
const uint8_t TMP_LIMIT_UPPER = 60; // Threshold fuer Rampup
const float T_ROOM = 23;			// Raumtemperatur

bool pumpOff = false;				   // Pumpenvariable
const uint16_t MIN_ON_TIME_S = 5 * 60; // 5 minutes

// Messwerte
typedef float tmp_t;
tmp_t tmp; // Durchschnittstemperatur
const uint8_t DIVISOR_EXPONENTIAL_FILTER = 16;

typedef enum
{
	COLD,
	WARMUP,
	HOT,
	COOLDOWN
} ePhase_t;

ePhase_t phase = COLD;

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

	// Serial.begin(9600);
}

void loop()
{
	now = millis();

	// Pump control @1s
	if ((now - then) >= CYCLE_PERIOD_MS)
	{
		static tmp_t tmpPrev = TMP_LIMIT_LOWER; // on first run and if in mid temperature band, provoke pump on

		// Read and smooth temperature
		tmp_t tmpRead = analogRead(PIN_TEMP_SENSOR);
		tmpRead = round(tmpRead / 1024 * 5.0 / 1.5 * 150);

		tmp = (tmpRead + tmp * (DIVISOR_EXPONENTIAL_FILTER - 1)) / DIVISOR_EXPONENTIAL_FILTER;

		// Serial.print("avg: ");
		// Serial.print(tmp);

		pumpOff = false; // Default on/active off
		static uint16_t minOnTime;

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
			// check gradient for temperature between hard limits

			if (0 < minOnTime)
			{
				// min on time active
				minOnTime--;
			}
			else if (tmpPrev < tmp)
			{
				// rising temperature --> leave pump on at least for minimal on time
				minOnTime = MIN_ON_TIME_S;
			}
			else
			{
				// unchanged or falling temperature --> turn pump off
				pumpOff = true;
			}
		}

		// Serial.print("   minOnTime: ");
		// Serial.println(minOnTime);

		// Write output pin
		digitalWrite(PIN_RELAIS_PUMP, pumpOff);

		then = now;
		tmpPrev = tmp;
	} // Intervallende

	// Display
	if (digitalRead(PIN_DISPLAY_SWITCH))
	{
		lcd.backlight();

		lcd.setCursor(0, 0);
		lcd.print("Toff: <");
		lcd.print(TMP_LIMIT_LOWER);
		lcd.print(" P: ");
		if (pumpOff)
		{
			lcd.print("OFF");
		}
		else
		{
			lcd.print("ON ");
		}
		lcd.println();

		lcd.setCursor(0, 1);
		lcd.print("TReg: <");
		lcd.print(TMP_LIMIT_UPPER);
		lcd.print(" T: ");
		lcd.print(round(tmp));
	}
	else
	{
		lcd.noBacklight();
		lcd.clear();
	}
}
