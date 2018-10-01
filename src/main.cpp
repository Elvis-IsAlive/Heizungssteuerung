#include "Arduino.h"
#include <pins_arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pinbelegung
const byte PIN_SCHALTER = A1;
const byte PIN_TEMP = A3; //Pin fuer TempInput
const byte PIN_RELAIS_1 = A2;	//Pin fuer Reilais
const byte PIN_LED = LED_BUILTIN;		//Pin fuer StatusLED


// Display
LiquidCrystal_I2C lcd(0x27,16,2);
bool s_disp_an = false;				// Schalterstatus
bool lcd_backlight_on = true;	// Hintergrundbeleuchtung Status

// Zeit
unsigned long int now = 0, then = 0;	// Zeitvariablen
const uint16_t T_INTERVALL = 1000;			// Zeitintervall

// Temperatur-Grenzwerte
const uint8_t T_THR_ON = 30;   	   // Threshold fuer Rampup
const uint8_t T_SOLL = 60;    		 // Fuehrungsgroesse fuer Regelbetrieb

bool ramp_up = true;    // Ablaufsteuerung
bool pump_on = false;   // Pumpenvariable

// Messwerte
const uint8_t ANZ_DS_LAUFEND = 10;	// Anzahl der Messwerte für Berechnung laufender Durchschnitt
float temperaturen[ANZ_DS_LAUFEND]; 	// = {150, 150, 150, 150, 150, 150, 150, 150, 150, 150};
float ds = 0, val = 0, val_roh = 0;	// laufender Durchschnitt aus Array temperaturen, Messwert, Roh-Messwert

uint16_t counter = 0;		// Laufindex für Temperatur-Array




void setup() {

	// Serielle Kommunikation
	Serial.begin(115200);

	// IO-Modes
	pinMode(PIN_RELAIS_1, OUTPUT);
	pinMode(PIN_LED, OUTPUT);
	pinMode(PIN_SCHALTER, INPUT);

	//Relais im ausgeschaltetem Zustand
	digitalWrite(PIN_RELAIS_1, HIGH);		// active low > bei Ausfall schaltet Relais die Pumpe auf Dauerbetrieb an

	// lcd
	lcd.init();
	lcd.backlight();

	// Setup message
	lcd.clear();
	lcd.println("Starting...");
	lcd.clear();

}






void loop() {

	now = millis();
	s_disp_an = digitalRead(PIN_SCHALTER);		// Schalterstatus ständig überwachen


	//Messintervall 1s
  if ((now - then) >= T_INTERVALL){
		then = now;

		// Messwert einlesen
    val_roh = analogRead(PIN_TEMP);
		// if(Serial.available() > 0){
		// 	val_roh = Serial.parseInt();
		// }
    val = round(val_roh / 1024 * 5.0 / 1.5 * 150);    // Temperatur-Messwert umrechnen
		temperaturen[counter] = val;	// schreibe Temperatur in array für Berechnung lfnd Durchschnitt

		// counter ggf. resetten
		if (counter >= ANZ_DS_LAUFEND - 1){
			counter = 0;
		}else{
			counter++;		// increment
		}

		// Durchschnitt berechnen
		float sum = 0;
		for (int i = 0; i < ANZ_DS_LAUFEND; i++){
			sum += temperaturen[i];
		}
		ds = round(sum / ANZ_DS_LAUFEND);


    // Logik
    if (ds > T_THR_ON){
      if (ramp_up){
        // Bei Rampup laeuft Pumpe bereits am T_THR_ON um Wasser umzuwaelzen
        // um gleichmaessige Temperaturverteilung zu erreichen
        pump_on = true;
        if (ds >= T_SOLL){
          ramp_up = false;    //Reset
        }

      }else{
        // Regelbetrieb
        if ( ds < T_SOLL){
          // Pumpe fruehzeitig aus. VermT_SOLLeidet unnoetiges Pumpen und Waermeverlust durch Kamin
          pump_on = false;
        }else{
          pump_on = true;
        }
      }

    // unterhalb T_THR_ON
    }else{
      pump_on = false;
      ramp_up = true;
    }


    // Ausgänge setzen/Relais ansteuern
    digitalWrite(PIN_RELAIS_1, pump_on);



	}	// Intervallende


	// Kürzere Intervallzeit für Schalter bzw. Display
	if ((now - then) >= 500){
		// LCD-Ausgabe
		if (s_disp_an == true){
			// if (!lcd_backlight_on){
			// 	lcd.backlight();
			// 	lcd_backlight_on = true;
			// }
			lcd.setCursor(0, 0);
			lcd.print("Ton:  ");
			lcd.print(T_THR_ON);
			lcd.print(" P: ");
			if (pump_on){
				lcd.print("ON  ");
			}else{
				lcd.print("OFF ");
			}
			lcd.println(pump_on);
			lcd.setCursor(0, 1);
			lcd.print("Toff: ");
			lcd.print(T_SOLL);
			lcd.print(" T: ");
			lcd.print((int) ds);
			// lcd.println("  ");
			// if(lcd_backlight_on){
			// 	lcd.noBacklight();
			// 	lcd_backlight_on = false;
			// 	lcd.clear();
			// }


		}
	}


}
