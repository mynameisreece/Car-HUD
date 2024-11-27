#include <M5Core2.h>
#include "BluetoothSerial.h"
#include "ELMduino.h"

BluetoothSerial SerialBT;
#define ELM_PORT SerialBT
#define DEBUG_PORT Serial

ELM327 myELM327;

// OBD-II states and data
typedef enum { ENG_RPM, SPEED } obd_pid_states;
obd_pid_states obd_state = ENG_RPM;

float rpm = 0;
float kph = 0;

// Button Colors and Definitions
ButtonColors on_clrs  = {RED, WHITE, WHITE};
ButtonColors off_clrs = {BLACK, WHITE, WHITE};
Button leftButton(0, 0, 0, 0, false, "ENGINE RPM", off_clrs, on_clrs, MC_DATUM);
Button rightButton(0, 0, 0, 0, false, "SPEED", off_clrs, on_clrs, MC_DATUM);

// Runtime states
bool isRunningRpm = false;
bool isRunningKph = false;
unsigned long lastActionTime = 0;

// LCD Constants for positions
const int RPM_DISPLAY_X = 20, RPM_DISPLAY_Y = 40;
const int KPH_DISPLAY_X = 180, KPH_DISPLAY_Y = 160;
const int TIMEOUT_DURATION = 30000; // 30 seconds

void setup() {
  M5.begin();
  M5.Lcd.setTextDatum(MC_DATUM);
  M5.Lcd.setTextSize(1);
  M5.Axp.SetLcdVoltage(3300); // Increase LCD brightness
  M5.Lcd.setCursor(0, 24);
  M5.Lcd.print("Initializing...");
  delay(500);

  // Initialize Serial and Bluetooth
  DEBUG_PORT.begin(115200);
  SerialBT.setPin("1234"); // Set user-configurable PIN here
  ELM_PORT.begin("OBDII", true);

  if (!ELM_PORT.connect("OBDII")) {
    M5.Lcd.setCursor(0, 24);
    M5.Lcd.clear();
    M5.Lcd.print("Couldn't connect to OBD scanner - Phase 1");
    while (1);
  }

  if (!myELM327.begin(ELM_PORT, true, 2000)) {
    M5.Lcd.setCursor(0, 24);
    M5.Lcd.clear();
    M5.Lcd.print("Couldn't connect to OBD scanner - Phase 2");
    while (1);
  }

  M5.Lcd.print("Connected to ELM327");
  delay(1000);

  M5.Lcd.clear();

  // Add handlers for button taps with debouncing
  leftButton.addHandler(toggleRpm, E_TAP);
  rightButton.addHandler(toggleKph, E_TAP);
  doButtons();
  M5.Lcd.setRotation(1);
  M5.Lcd.setTextSize(7);
  M5.Lcd.setTextColor(CYAN, BLACK);
}

void loop() {
  M5.update();

  // Handle timeout
  if (millis() - lastActionTime > TIMEOUT_DURATION) {
    isRunningRpm = false;
    isRunningKph = false;
  }

  if (isRunningRpm) {
    fetchObdData(myELM327.rpm(), rpm, RPM_DISPLAY_X, RPM_DISPLAY_Y, "RPM");
  }
  if (isRunningKph) {
    fetchObdData(myELM327.kph(), kph, KPH_DISPLAY_X, KPH_DISPLAY_Y, "KPH");
  }
}

void doButtons() {
  int16_t hw = M5.Lcd.width() / 2;
  int16_t h = M5.Lcd.height() / 8;
  leftButton.set(0, 0, hw, h);
  rightButton.set(hw, 0, hw, h);
  M5.Buttons.draw();
}

void toggleRpm(Event& e) {
  debounceButton([&]() {
    isRunningRpm = !isRunningRpm;
    lastActionTime = millis();
    Serial.printf("getRpm is now %s\n", isRunningRpm ? "ON" : "OFF");
    leftButton.off.bg = isRunningRpm ? GREEN : BLACK;
    leftButton.draw();
  });
}

void toggleKph(Event& e) {
  debounceButton([&]() {
    isRunningKph = !isRunningKph;
    lastActionTime = millis();
    Serial.printf("getKph is now %s\n", isRunningKph ? "ON" : "OFF");
    rightButton.off.bg = isRunningKph ? GREEN : BLACK;
    rightButton.draw();
  });
}

void fetchObdData(float value, float& target, int x, int y, const char* label) {
  float tempValue = value;
  if (myELM327.nb_rx_state == ELM_SUCCESS) {
    target = (uint32_t)tempValue;
    M5.Lcd.fillRect(x, y, 150, 60, BLACK);
    M5.Lcd.drawNumber(target, x, y);
  } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
    M5.Lcd.print(String("Error getting ") + label);
    myELM327.printError();
  }
}

void debounceButton(std::function<void()> action) {
  static unsigned long lastPress = 0;
  unsigned long now = millis();
  if (now - lastPress > 300) { // 300ms debounce interval
    action();
    lastPress = now;
  }
}
