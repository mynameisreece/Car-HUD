#include <M5Core2.h>
#include "BluetoothSerial.h"
#include "ELMduino.h"

BluetoothSerial SerialBT;
#define ELM_PORT SerialBT
#define DEBUG_PORT Serial

ELM327 myELM327;

typedef enum { ENG_RPM, SPEED } obd_pid_states;
obd_pid_states obd_state = ENG_RPM;

float rpm = 0;
float kph = 0;

void setup(){
  M5.begin();
  M5.Lcd.setTextSize(3);
  M5.Lcd.print("Initializing...");
  
  // Set up Bluetooth connection
  ELM_PORT.begin("OBDII", true);

  // Increase LCD brightness for better visibility
  M5.Axp.SetLcdVoltage(3300);

  if (!myELM327.begin(ELM_PORT, true, 2000)) {
    M5.Lcd.clear();
    M5.Lcd.print("Couldn't connect to OBD scanner.");
    while (1); // Halt if connection fails
  }

  M5.Lcd.clear();
  M5.Lcd.print("Connected to ELM327");
  delay(1000); // Display connection success for a moment
  M5.Lcd.clear();
}

void loop() {
  // Only set rotation and text properties once
  M5.Lcd.setRotation(1);
  M5.Lcd.setTextSize(7);
  M5.Lcd.setTextColor(CYAN, BLACK);

  switch (obd_state) {
    case ENG_RPM: {
      float tempRPM = myELM327.rpm();

      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        rpm = tempRPM;
        M5.Lcd.fillRect(20, 40, 150, 60, BLACK); // Clear area without full screen clear
        M5.Lcd.drawNumber(rpm, 20, 40);
        obd_state = SPEED;
      } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        M5.Lcd.print("Error getting RPM");
        obd_state = SPEED;
      }
      break;
    }

    case SPEED: {
      float tempKPH = myELM327.kph();

      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        kph = tempKPH;
        M5.Lcd.fillRect(180, 160, 150, 60, BLACK); // Clear area without full screen clear
        M5.Lcd.drawNumber(kph, 180, 160);
        obd_state = ENG_RPM;
      } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        M5.Lcd.print("Error getting Speed");
        obd_state = ENG_RPM;
      }
      break;
    }
  }
}
