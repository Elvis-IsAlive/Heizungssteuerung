#include "Arduino.h"
#include <pins_arduino.h>

// #include "../hs/Average.h"
// #include "../hs/Measurement.h"


#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
	//MEGA-CONFIG
	const byte PIN_TEMP = A0; //Pin fuer TempInput
	const byte PIN_RELAIS_1// #include "../hs/Average.h"
// #include "../hs/Measurement.h" = 44;	//Pin fuer Reilais
	// const byte PIN_PIEZO = 53;	//Pin fuer Piezo
	const byte PIN_LED = 52;
#endif


#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
	//NANO-CONFIG
	const byte PIN_TEMP = A0; //Pin fuer TempInput
	const byte PIN_RELAIS_1 = 2;	//Pin fuer Reilais
	// const byte PIN_RELAIS_2 = 3;
	// const byte PIN_PIEZO = 4;	//Pin fuer Piezo
	const byte PIN_LED = LED_BUILTIN;		//Pin fuer StatusLED
#endif





const uint8_t T_THR_ON = 30;      // Threshold fuer Rampup
const uint8_t T_SOLL = 60;     // Fuehrungsgroesse fuer Regelbetrieb

bool ramp_up = true;    // Ablaufsteuerung
bool pump_on = false;   // Pumpenvariable


// PID-Regler
const uint8_t ANZ_MESSWERTE = 3;




void setup() {

	//communication
	Serial.begin(115200);

	//io
	pinMode(PIN_RELAIS_1, OUTPUT);
	// pinMode(PIN_RELAIS_2, OUTPUT);
	// pinMode(PIN_PIEZO, OUTPUT);
	pinMode(PIN_LED, OUTPUT);

	//Relais im ausgeschaltetem Zustand
	digitalWrite(PIN_RELAIS_1, HIGH);		//Relais off
	// digitalWrite(PIN_RELAIS_2, HIGH);


	Serial.print("TempOn: ");
	Serial.println(T_THR_ON);
	// Serial.print("TempOff: ");
	// Serial.println(tempThr2);
	Serial.println("Setup done...");

}


void loop() {

  //Messintervall 1s
  if (millis() % 1000 == 0){

    // Messwert erfassen
    // Sensor Messbereich 0 - 150 °C, Ausgangsspannung 0 bis 1,5V
    // 10-Bit-Aufloesung für 5V -> 0 bis 1,5V entspricht 0 bis 1024 / 5V * 1,5V = 307

    // Array fuer Regeldiff
    // uint16_t mw[ANZ_MESSWERTE] = {};  // Messwerte/Rueckfuehrungsgroessen
    // int8_t diff[ANZ_MESSWERTE] = {};  // Regeldifferenzen
    // uint8_t c = 2;


    // ############################## in Klasse auslagern
    // Regeldiffeichung ermitteln
    uint16_t val_roh;
    float val, val_prev, err, err_sum;

    val_roh = analogRead(PIN_TEMP);
    val = round(val_roh / 1024 * 5.0);    // Temperatur-Messwert

    // val *= 10;      // Auf keine Kommastelle gerundet
    // if ((int)val % 10 >= 5){
    //   val = ((int)val + 1) / 10.0;
    // }else{
    //   val = ((int) val) / 10.0;
    // }

    err = val_prev - val;   // Regeldifferenz
    err_sum += err;         // Fehlersummeval = round(t)


    // Implementierung PID-Regler

    // Parametrierung
    float Kp = 0.1;
    float Ki = 0.1;
    float Kd = 0.1;

    float t = val + Kp * err +
      Kd * (val - val_prev) +     // val - val_prev  =  err - err_prev
      Ki * (err_sum);



    // Stellgroesse
    if (t > T_THR_ON){
      if (ramp_up){
        // Bei Rampup laeuft Pumpe bereits am T_THR_ON um Wasser umzuwaelzen
        // um gleichmaessige Temperaturverteilung zu erreichen
        pump_on = true;
        if (t >= T_SOLL){
          ramp_up = false;    //Reset
        }

      }else{
        // Regelbetrieb
        if ( t < T_SOLL){
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

    // Relais ansteuern
    digitalWrite(PIN_RELAIS_1, pump_on);


    Serial.print("T_OFF:");
    Serial.println(T_SOLL);
    Serial.print("T_IST:");
    Serial.println();
  }
}


  //
	// //Messung ausfuehren**
	// msm.run();
  //
	// if(millis()%2000 == 0){
	// 	avg = msm.getNewAvg();
	// 	diff = msm.getDiffAvg();
  //
  //
  //
	// 	//safe on
	// 	if (avg > tempThr2){
	// 		r1_off = false;		//on
	// 		runningHigh = false;
  //
	// 	//within temperature band
	// 	}else if(avg > tempThr1 && avg < tempThr2){
  //
	// 		//low to high
	// 		if(runningHigh){
	// 			r1_off = false;		//on
  //
	// 		//high to low
	// 		}else if(!runningHigh){
	// 			r1_off = true;		//off
	// 		}
  //
	// 	}else if(avg < tempThr1){
	// 		r1_off = true;		//off
	// 		runningHigh = true;
	// 	}
  //
	// 	digitalWrite(PIN_RELAIS_1, r1_off);
  //
	// 	//Serielle �bertragung
	// 	Serial.print("TempAvg: ");
	// 	Serial.print(avg);
	// 	Serial.print("    Diff: ");
	// 	Serial.println(diff);
	// }
