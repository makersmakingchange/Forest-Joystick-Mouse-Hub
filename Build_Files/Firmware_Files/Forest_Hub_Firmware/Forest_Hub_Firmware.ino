/*
  File: OpenAT_Joystick_Software.ino
  Software: OpenAT Joystick Plus 4 Switches, with both USB gamepad and mouse funtionality.
  Developed by: Makers Making Change
  Version: V2.0(20 February 2024)
  License: GPL v3

  Copyright (C) 2023-2024 Neil Squire Society
  This program is free software: you can redistribute it and/or modify it under the terms of
  the GNU General Public License as published by the Free Software Foundation,
  either version 3 of the License, or (at your option) any later version.
  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU General Public License for more details.
  You should have received a copy of the GNU General Public License along with this program.
  If not, see <http://www.gnu.org/licenses/>
*/
#include "XACGamepad.h"
#include "OpenAT_Joystick_Response.h"
#include <FlashStorage.h> // Non-volatile memory for storing settings
#include <TinyUSB_Mouse_and_Keyboard.h> // Library to allow mouse and keyboard to work using TinyUSB
#include <WiiChuck.h> //Nunchuck communication
#include <Adafruit_NeoPixel.h> //Lights on QtPy


#define USB_DEBUG  0 // Set this to 0 for best performance.

// ==================== Pin Assignment =================

#define PIN_JOYSTICK_X    A0
#define PIN_JOYSTICK_Y    A1
#define PIN_SW_S1         A2
#define PIN_SW_S2         A3
#define PIN_SW_S3         A6 //TX
#define PIN_SW_S4         A10 //MO
#define PIN_BUTTON        A9 //MI
#define PIN_LEDS          A7 //RX
#define PIN_BUZZER        A8 //SCK


// 5 Neopixels connected in series
#define LED_SLOT0         5 // Non-existent Neopixel for no light
#define LED_SLOT1         2 // Neopixel LED number 2 //L3
#define LED_SLOT2         1 // Neopixel LED number 1 //L2
#define LED_SLOT3         0 // Neopixel LED number 0 //L1
#define LED_GAMEPAD       4 // Neopixel LED number
#define LED_MOUSE         3 // Neopixel LED number
#define LED_DEFAULT_BRIGHTNESS 50


#define MODE_MOUSE 1
#define MODE_GAMEPAD 0
#define DEFAULT_MODE MODE_MOUSE

#define CONF_OPERATING_MODE_MIN 0
#define CONF_OPERATING_MODE_MAX 1
#define CONF_OPERATING_MODE_DEFAULT DEFAULT_MODE

#define DEFAULT_SLOT      0

#define UPDATE_INTERVAL   5 // TBD Update interval for perfoming HID actions (in milliseconds)
#define DEFAULT_DEBOUNCING_TIME 5

#define LONG_PRESS_MILLIS  3000 // time, in milliseconds, for a mode change to occur

#define SLOT_DEFAULT_NUMBER                   0             // Default slot number
#define SLOT_MIN_NUMBER                       0             // Minimum slot number
#define SLOT_MAX_NUMBER                       3             // Maxium slot number


#define JOYSTICK_DEFAULT_DEADZONE_LEVEL      2              //Joystick deadzone
#define JOYSTICK_MIN_DEADZONE_LEVEL          1
#define JOYSTICK_MAX_DEADZONE_LEVEL          10
#define JOYSTICK_MAX_DEADZONE_VALUE          64             //Out of 127
#define JOYSTICK_MAX_VALUE                   128

#define JOYSTICK_INPUT_XY_MAX               1023

#define MOUSE_DEFAULT_CURSOR_SPEED_LEVEL     5              // Default cursor speed level
#define MOUSE_MIN_CURSOR_SPEED_LEVEL         1              //Minimum cursor speed level
#define MOUSE_MAX_CURSOR_SPEED_LEVEL         10             //Maxium cursor speed level
#define MOUSE_MIN_CURSOR_SPEED_VALUE         1              //Minimum cursor speed value [pixels per update]
#define MOUSE_MAX_CURSOR_SPEED_VALUE         10

#define JOYSTICK_REACTION_TIME               30             //Minimum time between each action in ms
#define SWITCH_REACTION_TIME                 100            //Minimum time between each switch action in ms
#define STARTUP_DELAY_TIME                   1000           //Time to wait on startup
#define FLASH_DELAY_TIME                     5              //Time to wait after reading/writing flash memory

#define SLOW_SCROLL_DELAY                    200            //Minimum time, in ms, between each slow scroll action (number of slow scroll actions defined below)
#define FAST_SCROLL_DELAY                    60             //Minimum time, in ms, between each fast scroll action (higher delay = slower scroll speed)
#define SLOW_SCROLL_NUM                      10             //Number of times to scroll at the slow scroll rate


//Define model number and version number
#define JOYSTICK_DEVICE                       1                
#define JOYSTICK_MODEL                        1
#define JOYSTICK_VERSION                      2


XACGamepad gamepad;   //Starts an instance of the USB gamepad object

//Declare variables for settings
int isConfigured;
int deviceNumber;
int modelNumber;
int versionNumber;
int deadzoneLevel;
int cursorSpeedLevel; // 1-10 cursor speed levels
int operatingMode;   // 1 = Mouse mode, 0 = Joystick Mode 
int slotNumber;      // Slots numbered 1-3 with different settings
int ledBrightness;

// Variables stored in Flash Memory
FlashStorage(isConfiguredFlash, int);
FlashStorage(deviceNumberFlash,int);
FlashStorage(modelNumberFlash, int);
FlashStorage(versionNumberFlash, int);
FlashStorage(deadzoneLevelFlash, int);
FlashStorage(cursorSpeedLevelFlash, int);
FlashStorage(operatingModeFlash, int);
FlashStorage(ledBrightnessFlash,int);
FlashStorage(slotNumberFlash,int);  // Track index of current settings slot
FlashStorage(xOutputMinimumFlash,int);
FlashStorage(xOutputMaximumFlash,int);
FlashStorage(yOutputMinimumFlash,int);
FlashStorage(yOutputMaximumFlash,int);
FlashStorage(xNeutralFlash,int);
FlashStorage(yNeutralFlash,int);

// Timing Variables
long lastInteractionUpdate;
long mPressStartMillis = 0;
long cPressStartMillis = 0;

//Declare joystick input and output variables
int rawX;
int rawY;
int inputX;
int inputY;
int centeredX;
int centeredY;
int outputX;
int outputY;

/*
//Declare joystick calibration variables
int xOutputMinimum = 0;
int xOutputMaximum = JOYSTICK_INPUT_XY_MAX;
int yOutputMinimum = 0;
int yOutputMaximum = JOYSTICK_INPUT_XY_MAX;
int xNeutral = 512;
int yNeutral = 512;
*/

//Declare joystick calibration variables
int xOutputMinimum;
int xOutputMaximum;
int yOutputMinimum;
int yOutputMaximum;
int xNeutral;
int yNeutral;

float minRange;    // TODO: change this name

//Declare switch state variables for each switch
bool switchS1Pressed;           // Mouse mode = left click
bool switchS2Pressed;           // Mouse mode = scroll mode
bool switchS3Pressed;           // Mouse mode = right click
bool switchS4Pressed;           // Mouse mode = middle click
bool switchSMPressed;
bool buttonCPressed;
bool buttonMPressed;

bool isModeChanging;

//Previous status of switches
bool switchS1PrevPressed = false;
bool switchS2PrevPressed = false;
bool switchS3PrevPressed = false;
bool switchS4PrevPressed = false;
bool switchSMPrevPressed = false;
bool buttonCPrevPressed  = false;
bool buttonMPrevPressed  = false;

// Theoretical voltages for multi-button analog resistor network
// SM_Switch - Mode Switch - Calib Switch
// voltages v000 = voltage when off-off-off
const int v000 = 643;
const int v001 = 609;
const int v010 = 563;
const int v011 = 512;
const int v100 = 418;
const int v101 = 327;
const int v110 = 185;
const int v111 = 0;
const int t001 = (v000+v001)/2;
const int t010 = (v001+v010)/2;
const int t011 = (v010+v011)/2;
const int t100 = (v011+v100)/2;
const int t101 = (v100+v101)/2;
const int t110 = (v101+v110)/2;
const int t111 = (v110+v111)/2;

int currentDeadzoneValue;
int currentMouseCursorSpeedValue;

bool settingsEnabled = false;

//***API FUNCTIONS***// - DO NOT CHANGE
typedef void (*FunctionPointer)(bool, bool, String);

//Type definition for API function pointer

typedef struct {                                  //Type definition for API function list
  String endpoint;                                //Unique two character end point
  String code;                                    //Unique one character command code
  String parameter;                               //Parameter that is passed to function
  FunctionPointer function;                       //API function pointer
} _functionList;

//Switch structure
typedef struct {
  uint8_t switchNumber;
  String switchButtonName;
  uint8_t switchButtonNumber;
} switchStruct;

// Slot structure
typedef struct {
  uint8_t slotNumber;
  uint8_t slotLEDNumber;
  String mouseSlotName;
  int slotCursorSpeedLevel;
} slotStruct; 
/*
 * To add to Slot Structure later:
 * - mouse acceleration
 * - mouse switch functions (so these can be remapped)
 * - gamepad slot name
 * - gamepad switch functions
 * - deadzone??
 */

// Declare individual API functions with command, parameter, and corresponding function
_functionList getDeviceNumberFunction =           {"DN", "0", "0", &getDeviceNumber};
_functionList getModelNumberFunction =            {"MN", "0", "0", &getModelNumber};
_functionList getVersionNumberFunction =          {"VN", "0", "0", &getVersionNumber};
_functionList getOperatingModeFunction =          {"OM", "0", "0", &getOperatingMode};
_functionList setOperatingModeFunction =          {"OM", "1", "",  &setOperatingMode};
_functionList getSlotNumberFunction =             {"SN", "0", "0", &getSlotNumber};
_functionList setSlotNumberFunction =             {"SN", "1", "",  &setSlotNumber};
_functionList getJoystickInitializationFunction = {"IN", "0", "0", &getJoystickInitialization};
_functionList setJoystickInitializationFunction = {"IN", "1", "1", &setJoystickInitialization};
_functionList getJoystickCalibrationFunction =    {"CA", "0", "0", &getJoystickCalibration};
_functionList setJoystickCalibrationFunction =    {"CA", "1", "1", &setJoystickCalibration};
_functionList getJoystickDeadZoneFunction =       {"DZ", "0", "0", &getJoystickDeadZone};
_functionList setJoystickDeadZoneFunction =       {"DZ", "1", "",  &setJoystickDeadZone};
_functionList getMouseCursorSpeedFunction =       {"SS", "0", "0", &getMouseCursorSpeed};
_functionList setMouseCursorSpeedFunction =       {"SS", "1", "",  &setMouseCursorSpeed};
_functionList getJoystickValueFunction =          {"JV", "0", "0", &getJoystickValue};
_functionList softResetFunction =                 {"SR", "1", "1", &softReset};
//_functionList getLEDBrightnessFunction =          {"LB", "0", "",  &getLEDBrightness};
//_functionList setLEDBrightnessFunction =          {"LB", "1", "",  &setLEDBrightness};

// Declare array of API functions
_functionList apiFunction[17] = {
  getDeviceNumberFunction,
  getModelNumberFunction,
  getVersionNumberFunction,
  getOperatingModeFunction,
  setOperatingModeFunction,
  getSlotNumberFunction,
  setSlotNumberFunction,
  getJoystickInitializationFunction,
  setJoystickInitializationFunction,
  getJoystickCalibrationFunction,
  setJoystickCalibrationFunction,
  getJoystickDeadZoneFunction,
  setJoystickDeadZoneFunction,
  getMouseCursorSpeedFunction,
  setMouseCursorSpeedFunction,
  getJoystickValueFunction,
  softResetFunction
};

//Switch properties
const switchStruct joystickMapping[] {
  {1, "S1", 1},
  {2, "S2", 2},
  {3, "S3", 3},
  {4, "S4", 4}

};

//Slot properties   **CHANGE THESE WITH VARIABLES AND HAVE THEM BE LOADED IN SETUP DEPENDING IF INIT OR NO
slotStruct mouseSlots[] {
  {0, LED_SLOT0, "Default", 5}, // slot number, Slot LED Number, Slot name, cursor speed
  {1, LED_SLOT1, "Slow",    1},
  {2, LED_SLOT2, "Medium",  5},
  {3, LED_SLOT3, "Fast",   10}
};

Adafruit_NeoPixel led_microcontroller(1, PIN_NEOPIXEL);  // Create a pixel strand with 1 pixel on QtPy
Adafruit_NeoPixel leds(5, PIN_LEDS);  // Create a pixel strand with 5 NeoPixels

//***MICROCONTROLLER AND PERIPHERAL CONFIGURATION***//
// Function   : setup
//
// Description: This function handles the initialization of variables, pins, methods, libraries. This function only runs once at powerup or reset.
//
// Parameters :  void
//
// Return     : void
//*********************************//
void setup() {
    // Begin serial
  Serial.begin(115200);
  if (USB_DEBUG) { Serial.println("DEBUG: Serial Started.");}
    
    // Initialize Memory
  initMemory();
  delay(FLASH_DELAY_TIME);
  if (USB_DEBUG) { Serial.println("DEBUG: Memory initialized.");}
  
  led_microcontroller.begin(); // Initiate pixel on microcontroller
  led_microcontroller.clear();
  led_microcontroller.setBrightness(ledBrightness);
  // led_microcontroller.setPixelColor(0, led_microcontroller.Color(255, 0, 0)); // Turn LED red to start
  led_microcontroller.setPixelColor(0, led_microcontroller.Color(255, 255, 255)); // Turn LED white to start
  led_microcontroller.show();
  if (USB_DEBUG) { Serial.println("DEBUG: Microcontroller LED on.");}

  leds.begin();
  leds.clear();
  leds.setBrightness(ledBrightness);
  leds.show();
 if (USB_DEBUG) { Serial.println("DEBUG: Neopixel started.");}
  
    // Begin HID gamepad or mouse, depending on mode selection
  switch (operatingMode) {
    case MODE_MOUSE:
      initMouse();
      if (USB_DEBUG) { Serial.println("DEBUG: Mouse started.");}
      break;
    case MODE_GAMEPAD:
      gamepad.begin();
      if (USB_DEBUG) { Serial.println("DEBUG: Gamepad Started.");}
      break;
  }

  // Initialize Joystick
  initJoystick();
  if (USB_DEBUG) { Serial.println("DEBUG: Joystick initialized.");}

  //Initialize the switch pins as inputs
  pinMode(PIN_SW_S1, INPUT_PULLUP);
  pinMode(PIN_SW_S2, INPUT_PULLUP);
  pinMode(PIN_SW_S3, INPUT_PULLUP);
  pinMode(PIN_SW_S4, INPUT_PULLUP);
  pinMode(PIN_BUTTON, INPUT);


  checkSetupMode(); // Check to see if operating mode change
  delay(STARTUP_DELAY_TIME);

 
  // Turn on indicator light, depending on mode selection
  showModeLED();
  updateSlot(slotNumber);

  tone(PIN_BUZZER, 1047, 500); 

  switch (operatingMode) {
    case MODE_MOUSE:
        tone(PIN_BUZZER, 523, 400); 
      break;
    case MODE_GAMEPAD:
        tone(PIN_BUZZER, 1047, 600); 
      break;
  }
 
  lastInteractionUpdate = millis();  // get first timestamp

}

void loop() {

  settingsEnabled = serialSettings(settingsEnabled); //Check to see if setting option is enabled


  if (millis() >= lastInteractionUpdate + UPDATE_INTERVAL) {
    lastInteractionUpdate = millis();  // get timestamp
    readJoystick();
    readSwitches();

    joystickActions();

    switchesActions();

    // mode change function

    // calibration function
    
  }
}

//*********************************//
// Joystick Functions
//*********************************//

//***INITIALIZE Memory FUNCTION***//
// Function   : initMemory
//
// Description: This function initializes Memory
//
// Parameters : void
//
// Return     : void
//****************************************//
void initMemory() {
  //Check if it's first time running the code
  isConfigured = isConfiguredFlash.read();
  delay(FLASH_DELAY_TIME);

  if (isConfigured == 0) {
    //Define default settings if it's first time running the code
    modelNumber = JOYSTICK_MODEL;
    deviceNumber = JOYSTICK_DEVICE;
    versionNumber = JOYSTICK_VERSION;
    deadzoneLevel = JOYSTICK_DEFAULT_DEADZONE_LEVEL;
    operatingMode = DEFAULT_MODE;
    slotNumber = DEFAULT_SLOT;
    ledBrightness = LED_DEFAULT_BRIGHTNESS;
    xOutputMinimum = -JOYSTICK_MAX_VALUE;
    xNeutral = 0;
    xOutputMaximum = JOYSTICK_MAX_VALUE;
    yOutputMinimum = -JOYSTICK_MAX_VALUE;
    yNeutral = 0;
    yOutputMaximum = JOYSTICK_MAX_VALUE;

    isConfigured = 1;

    //cursorSpeedLevel = MOUSE_DEFAULT_CURSOR_SPEED_LEVEL;    //Load each slot cursor speed level here
    cursorSpeedLevel = mouseSlots[slotNumber].slotCursorSpeedLevel;  

    //Write default settings to flash storage
    modelNumberFlash.write(modelNumber);
    versionNumberFlash.write(versionNumber);
    deadzoneLevelFlash.write(deadzoneLevel);
    cursorSpeedLevelFlash.write(cursorSpeedLevel);          //Save each slot cursor speed level here
    operatingModeFlash.write(operatingMode);
    slotNumberFlash.write(slotNumber);
    ledBrightnessFlash.write(ledBrightness);
    xOutputMinimumFlash.write(xOutputMinimum);
    xNeutralFlash.write(xNeutral);
    xOutputMaximumFlash.write(xOutputMaximum);
    yOutputMinimumFlash.write(yOutputMinimum);
    yNeutralFlash.write(yNeutral);
    yOutputMaximumFlash.write(yOutputMaximum);

    isConfiguredFlash.write(isConfigured);
    delay(FLASH_DELAY_TIME);

  } else {
    //Load settings from flash storage
    modelNumber = modelNumberFlash.read();
    versionNumber = versionNumberFlash.read();
    deadzoneLevel = deadzoneLevelFlash.read(); 
    operatingMode = operatingModeFlash.read();
    slotNumber = slotNumberFlash.read();
    //cursorSpeedLevel = cursorSpeedLevelFlash.read();
    cursorSpeedLevel = mouseSlots[slotNumber].slotCursorSpeedLevel;  
    ledBrightness = ledBrightnessFlash.read();

    xOutputMinimum = xOutputMinimumFlash.read();
    xOutputMaximum = xOutputMaximumFlash.read();
    yOutputMinimum = yOutputMinimumFlash.read();
    yOutputMaximum = yOutputMaximumFlash.read();

    xNeutral = xNeutralFlash.read();
    yNeutral = yNeutralFlash.read();

    delay(FLASH_DELAY_TIME);
  }

  
  //Serial print settings
  Serial.print("Model Number: ");
  Serial.println(modelNumber);

  Serial.print("Version Number: ");
  Serial.println(versionNumber);

  Serial.print("Operating Mode: ");
  Serial.println(operatingMode);

  Serial.print("Current Slot: ");
  Serial.println(slotNumber);

  Serial.print("Deadzone Level: ");
  Serial.println(deadzoneLevel);

  Serial.print("Cursor Speed Level: ");
  Serial.println(cursorSpeedLevel);

}

//***INITIALIZE JOYSTICK FUNCTION***//
// Function   : initJoystick
//
// Description: This function initializes joystick as input.
//
// Parameters : void
//
// Return     : void
//****************************************//
void initJoystick()
{
  joystickNeutralCalibration();
  getJoystickDeadZone(true, false);                                         //Get joystick deadzone stored in memory                                      //Get joystick calibration points stored in flash memory
}

void showModeLED(){
  leds.clear();
  led_microcontroller.clear();
  switch (operatingMode) {
    case MODE_MOUSE:
      led_microcontroller.setPixelColor(0, led_microcontroller.Color(255, 255, 0)); // Turn LED yellow
      leds.setPixelColor(LED_MOUSE, leds.Color(255,255,0));// Turn LED yellow
      break;
    case MODE_GAMEPAD:
      led_microcontroller.setPixelColor(0, led_microcontroller.Color(0, 0, 255)); // Turn LED blue
      leds.setPixelColor(LED_GAMEPAD, leds.Color(0, 0, 255)); // Turn LED blue
      break;
  }
  led_microcontroller.show();
  leds.show();
}


void joystickNeutralCalibration() {

  leds.setPixelColor(LED_MOUSE, leds.Color(255,255,255));  // Turn LED white
  leds.setPixelColor(LED_GAMEPAD, leds.Color(255,255,255));  // Turn LED white
  leds.show();

  int tempXNeutral = 0;
  int tempYNeutral = 0;

  const int JOYSTICK_NEUTRAL_READINGS = 10;

  for (int i = 0; i < JOYSTICK_NEUTRAL_READINGS ; i++){
     // Read analog raw value using ADC
  tempXNeutral += analogRead(PIN_JOYSTICK_X);
  tempYNeutral += analogRead(PIN_JOYSTICK_Y);
  delay(50);
  }
  
  int xNeutralRaw = tempXNeutral / JOYSTICK_NEUTRAL_READINGS;
  int yNeutralRaw = tempYNeutral / JOYSTICK_NEUTRAL_READINGS;

  xNeutral = map(xNeutralRaw, 0, JOYSTICK_INPUT_XY_MAX, -JOYSTICK_MAX_VALUE, JOYSTICK_MAX_VALUE);
  yNeutral = map(yNeutralRaw, 0, JOYSTICK_INPUT_XY_MAX, -JOYSTICK_MAX_VALUE, JOYSTICK_MAX_VALUE);

  // Calculate radius values from diagonals
  //int rad1 = calcMag((xOutputMaximum - xNeutral), (yOutputMaximum - yNeutral))/sqrt(2.0);
  //int rad2 = calcMag((xOutputMaximum - xNeutral), (yOutputMinimum - yNeutral))/sqrt(2.0);
  //int rad3 = calcMag((xOutputMinimum - xNeutral), (yOutputMaximum - yNeutral))/sqrt(2.0);
  //int rad4 = calcMag((xOutputMinimum - xNeutral), (yOutputMinimum - yNeutral))/sqrt(2.0);

  // Calculate radius values from cardinal directions
  int rad1 = abs(xOutputMaximum - xNeutral);
  int rad2 = abs(xOutputMinimum - xNeutral);
  int rad3 = abs(yOutputMaximum - yNeutral);
  int rad4 = abs(yOutputMinimum - yNeutral);
  
  minRange = min(min(rad1, rad2), min(rad3, rad4)) * 0.95;
  
  /*
  Serial.println(rad1);
  Serial.println(rad2);
  Serial.println(rad3);
  Serial.println(rad4);
  Serial.println(minRange);
  */
  
  // Calibration Complete, show mode LED
  showModeLED();

}


//***READ JOYSTICK FUNCTION**//
// Function   : readJoystick
//
// Description: This function reads the current raw values of potentiometers, checks if values exceed deadband, and calculates
//              joystick movements.
//
// Parameters :  Void
//
// Return     : Void
//*********************************//

void readJoystick() {

  // Read analog raw value using ADC
  rawX = analogRead(PIN_JOYSTICK_X);
  rawY = analogRead(PIN_JOYSTICK_Y);

  // Map joystick x and y move values
  inputX = map(rawX, 0, JOYSTICK_INPUT_XY_MAX, -JOYSTICK_MAX_VALUE, JOYSTICK_MAX_VALUE);   
  inputY = map(rawY, 0, JOYSTICK_INPUT_XY_MAX, -JOYSTICK_MAX_VALUE, JOYSTICK_MAX_VALUE);

  // Center point and implement neutral calibration
  centeredX = inputX - xNeutral;
  centeredY = inputY - yNeutral;

  float outputMag = calcMag(centeredX, centeredY);

  // Apply radial deadzone ************************
  if (outputMag <= currentDeadzoneValue)
  {
    centeredX = 0;
    centeredY = 0;
  } else if (outputMag >= minRange)
  {
    float thetaVal = atan2(centeredY, centeredX);
    centeredX = sgn(centeredX) * abs(cos(thetaVal)*minRange);
    centeredY = sgn(centeredY) * abs(sin(thetaVal)*minRange);
  }

  outputX = map(centeredX, -minRange, minRange, -JOYSTICK_MAX_VALUE, JOYSTICK_MAX_VALUE);
  outputY = map(centeredY, -minRange, minRange, -JOYSTICK_MAX_VALUE, JOYSTICK_MAX_VALUE); //TODO: Clean this up with more clear variable names

  //Serial.print(outputX); Serial.print("\t"); Serial.println(outputY);
}

//*********************************//
// Function   : sgn 
// 
// Description: The sign of float value ( -1 or +1 )
// 
// Arguments :  val : float : float input value
// 
// Return     : sign : int : sign of float value ( -1 or +1 )
//*********************************//
int sgn(float val) {
  return (0.0 < val) - (val < 0.0);
}

//***JOYSTICK ACTIONS FUNCTION**//
// Function   : joystickActions
//
// Description: This function executes joystick or mouse movements.
//
// Parameters :  Void
//
// Return     : Void
//*********************************//

void joystickActions() {

  switch (operatingMode) {
    case MODE_MOUSE:
      //mouse action   
      mouseJoystickMove(outputX, outputY);  
      break;
    case MODE_GAMEPAD:
      //Perform joystick HID action
      gamepadJoystickMove(outputX, outputY);
      break;
  }

  //delay(JOYSTICK_REACTION_TIME);
}

//***READ SWITCH FUNCTION**//
// Function   : readSwitches
//
// Description: This function reads the current digital values of pull-up pins.
//
// Parameters :  Void
//
// Return     : Void
//*********************************//

void readSwitches() {
  // Update Prev Switch States
  switchS1PrevPressed = switchS1Pressed;
  switchS2PrevPressed = switchS2Pressed;
  switchS3PrevPressed = switchS3Pressed;
  switchS4PrevPressed = switchS4Pressed;
  switchSMPrevPressed = switchSMPressed;
  buttonCPrevPressed  = buttonCPressed;
  buttonMPrevPressed  = buttonMPressed;

  //Update status of switch inputs
  switchS1Pressed = !digitalRead(PIN_SW_S1); // Switches are wired in a pull up, so digital read needs to be inverted
  switchS2Pressed = !digitalRead(PIN_SW_S2); 
  switchS3Pressed = !digitalRead(PIN_SW_S3); 
  switchS4Pressed = !digitalRead(PIN_SW_S4); 

  int buttonState   = analogRead(PIN_BUTTON); // read the button array

  if (buttonState < t111)
  {
  switchSMPressed = true;
  buttonMPressed = true;
  buttonCPressed = true;
  }
  else if (buttonState < t110)
  {
    switchSMPressed = true;
    buttonMPressed = true;
    buttonCPressed = false;
  }
  else if (buttonState < t101)
  {
    switchSMPressed = true;
    buttonMPressed = false;
    buttonCPressed = true;
  }
  else if (buttonState < t100)
  {
    switchSMPressed = true;
    buttonMPressed = false;
    buttonCPressed = false;
  }
  else if (buttonState < t011)
  {
    switchSMPressed = false;
    buttonMPressed = true;
    buttonCPressed = true;
  }
  else if (buttonState < t010)
  {
    switchSMPressed = false;
    buttonMPressed = true;
    buttonCPressed = false;
  }
  else if (buttonState < t001)
  {
    switchSMPressed = false;
    buttonMPressed = false;
    buttonCPressed = true;
  }
  else 
  {
    switchSMPressed = false;
    buttonMPressed = false;
    buttonCPressed = false;
  }

}

//***ALL SWITCH ACTIONS FUNCTION**//
// Function   : switchesActions
//
// Description: This function executes the functions of the switches.
//
// Parameters :  Void
//
// Return     : Void
//*********************************//
void switchesActions(){
        
    switch (operatingMode) {
      case MODE_MOUSE:
        switchesMouseActions();
        break;
      case MODE_GAMEPAD:
        switchesJoystickActions();
        break;
    }

    calibrationActions();
    slotModeChangeActions();
    
}

//***JOYSTICK MODE - SWITCH ACTIONS FUNCTION**//
// Function   : switchesJoystickActions
//
// Description: This function executes joystick functions of the switches.
//
// Parameters :  Void
//
// Return     : Void
//*********************************//

void switchesJoystickActions() {

  //Perform button actions
  if (switchS1Pressed) {
    gamepadButtonPress(joystickMapping[0].switchButtonNumber);
  }
  else if (!switchS1Pressed && switchS1PrevPressed) {
    gamepadButtonRelease(joystickMapping[0].switchButtonNumber);
  }

  if (switchS2Pressed) {
    gamepadButtonPress(joystickMapping[1].switchButtonNumber);
  } else if (!switchS2Pressed && switchS2PrevPressed) {
    gamepadButtonRelease(joystickMapping[1].switchButtonNumber);
  }

  if (switchS3Pressed) {
    gamepadButtonPress(joystickMapping[2].switchButtonNumber);
  } else if (!switchS3Pressed && switchS3PrevPressed) {
    gamepadButtonRelease(joystickMapping[2].switchButtonNumber);
  }

  if (switchS4Pressed) {
    gamepadButtonPress(joystickMapping[3].switchButtonNumber);
  } else if (!switchS4Pressed && switchS4PrevPressed) {
    gamepadButtonRelease(joystickMapping[3].switchButtonNumber);
  }

}

//***INITIALIZE MOUSE FUNCTION***//
// Function   : initMouse
//
// Description: This function initializes mouse as an output.
//
// Parameters : void
//
// Return     : void
//****************************************//
void initMouse()
{
  Mouse.begin(); // Initialize USB HID Mouse
}




//***MOUSE MODE - SWITCH ACTIONS FUNCTION**//
// Function   : switchesMouseActions
//
// Description: This function executes mouse functions of the switches.
//
// Parameters :  Void
//
// Return     : Void
//*********************************//

void switchesMouseActions() {

  //Perform button actions
  if (switchS1Pressed) {
    Mouse.press(MOUSE_LEFT);
  }
  else if (!switchS1Pressed && switchS1PrevPressed) {
    Mouse.release(MOUSE_LEFT);
  }

  if (switchS2Pressed) {
    Mouse.press(MOUSE_MIDDLE);
  } else if (!switchS2Pressed && switchS2PrevPressed) {
    Mouse.release(MOUSE_MIDDLE);
  }

  if (switchS3Pressed) {
    Mouse.press(MOUSE_RIGHT);
  } else if (!switchS3Pressed && switchS3PrevPressed) {
    Mouse.release(MOUSE_RIGHT);
  }

  if (switchS4Pressed) {
    int counter = 0;
    //change colour of neopixel to indicate entering scroll mode?
    while (switchS4Pressed) {

      readJoystick();
      readSwitches();

      if (outputY > 0) {
        Mouse.move(0, 0, 1);                  // Scroll up
        counter++;
      }
      else if (outputY < 0) {
        Mouse.move(0, 0, -1);                 //Scroll down
        counter++;
      } else if (outputY == 0) {
        counter = 0;
      }

      if (counter == 0) {
        delay(2);                             // low delay before any scroll actions have been completed, continue to read switches and joystick
      } else if (counter == 1) {
        delay(500);                           // long delay after first scroll action, to allow user to do single scroll
      } else if (counter < SLOW_SCROLL_NUM) {
        delay(SLOW_SCROLL_DELAY);             // first 10 scroll actions are slower (have longer delay) to allow precise scrolling
      } else {
        delay(FAST_SCROLL_DELAY);             // scroll actions after first 10 are faster (have less delay) to allow fast scrolling
      }

    }
  } else if (!switchS4Pressed && switchS4PrevPressed) {
    //action on release
    //change colour of neopixel to indicate exiting scroll mode?
  }
}

//***CALIBRATION ACTIONS FUNCTION**//
// Function   : calibrationActions
//
// Description: This function checks the calibration button to determine if a neutral calibration or extents calibration should take place. 
//
// Parameters :  Void
//
// Return     : Void
//*********************************//

void calibrationActions(){
  if (buttonCPressed){
   
    if (!buttonCPrevPressed){ 
      // button pressed for the first time
      cPressStartMillis = millis();
    }

  } else if (buttonCPrevPressed){
    if ((millis()-cPressStartMillis)>= LONG_PRESS_MILLIS){
      //TODO: Add full calibration here
    } else{
      joystickNeutralCalibration();
    }
  }
}

//***SLOT AND MODE CHANGE FUNCTION**//
// Function   : slotModeChangeActions
//
// Description: This function checks the mode button and switch to determine if a slot change or mode change should take place. 
//
// Parameters :  Void
//
// Return     : Void
//*********************************//

void slotModeChangeActions() {
  
  
  if (switchSMPressed || buttonMPressed){
   
    if (!switchSMPrevPressed && !buttonMPrevPressed){ 
      // button pressed for the first time
      isModeChanging = false;
      mPressStartMillis = millis();
    }
  
    if ((millis()-mPressStartMillis)>= LONG_PRESS_MILLIS){
      modeChange();
      isModeChanging = true;
    } else{
      isModeChanging = false;
    }

  } else if (switchSMPrevPressed || buttonMPrevPressed){
    // button released
    if (!isModeChanging){
      // a mode change is not occuring, therefore it was a short press -> change slots
      incrementSlot();
    }
  }
  
}

//***MODE CHANGE FUNCTION**//
// Function   : modeChange
//
// Description: This function changes the output mode to either gamepad or mouse.
//
// Parameters :  Void
//
// Return     : Void
//*********************************//

void modeChange(){
  // Change mode
  operatingMode += 1;
  if (operatingMode > 1) {
    operatingMode = 0;
  }

  // Display corresponding mode LED
  switch (operatingMode) {
    case MODE_MOUSE:
      led_microcontroller.setPixelColor(0, led_microcontroller.Color(255, 255, 0)); // Turn LED yellow
      led_microcontroller.show();
      leds.setPixelColor(LED_GAMEPAD, leds.Color(0,0,0)); // Turn off Gamepad LED
      leds.setPixelColor(LED_MOUSE, leds.Color(255,255,0));// Turn LED yellow
      leds.show();
      break;
    case MODE_GAMEPAD:
      led_microcontroller.setPixelColor(0, led_microcontroller.Color(0, 0, 255)); // Turn LED blue
      led_microcontroller.show();
      leds.setPixelColor(LED_MOUSE, leds.Color(0,0,0)); // Turn off Mouse LED
      leds.setPixelColor(LED_GAMEPAD, leds.Color(0, 0, 255)); // Turn LED blue
      leds.show();
      break;
  }

  // write current mode to flash
  operatingModeFlash.write(operatingMode);
  delay(FLASH_DELAY_TIME);
  
  // load slot settings that correspond to mode
  // perform solftware reset
  softwareReset();
}

//***INCREMENT SLOT FUNCTION**//
// Function   : incrementSlot
//
// Description: This function increments the current slot to the next and loads all settings.
//
// Parameters :  Void
//
// Return     : Void
//*********************************//

void incrementSlot(){

  // increase slot by 1 (if slot 3 go to 1)
 
  int newSlotNumber = slotNumber+1;

  if (newSlotNumber == SLOT_MAX_NUMBER+1){
    newSlotNumber = SLOT_MIN_NUMBER;
  }

  setSlotNumber(false,false,newSlotNumber);
}

//***UPDATE SLOT NUMBER FUNCTION**//
// Function   : updateSlot
//
// Description: This function loads the settings of the current slot.
//
// Parameters :  int tempSlotNumber
//
// Return     : Void
//*********************************//

void updateSlot(int newSlotNumber){


  // Update cursor speed    
  cursorSpeedLevel = mouseSlots[newSlotNumber].slotCursorSpeedLevel; 
  updateCursorSpeed(cursorSpeedLevel);  
    
  // Update LEDs
      // Turn off previous slot LED
  leds.setPixelColor(mouseSlots[slotNumber].slotLEDNumber, leds.Color(0, 0, 0));
    // Turn on indicator light for current slot
  leds.setPixelColor(mouseSlots[newSlotNumber].slotLEDNumber, leds.Color(0, 255, 0)); // Turn Slot LED red
  leds.show();

  //Update global variable.
  slotNumber = newSlotNumber;
 
}

//***CHECK SETUP MODE FUNCTION**//
// Function   : checkSetupMode
//
// Description: This function sets the operating mode
//
// Parameters :  Void
//
// Return     : void
//*********************************//

void checkSetupMode() {

  //Update status of switch inputs
  switchS1Pressed = !digitalRead(PIN_SW_S1);
  switchS2Pressed = !digitalRead(PIN_SW_S2);
  switchS3Pressed = !digitalRead(PIN_SW_S3);
  switchS4Pressed = !digitalRead(PIN_SW_S4);

  int mode = 0;

  if (switchS1Pressed && switchS2Pressed) {
    Serial.println("Mode selection: Press Button SW1 to Switch Mode, Button SW2 to Confirm selection");
    led_microcontroller.setPixelColor(0, led_microcontroller.Color(0, 255, 0)); //green
    led_microcontroller.show();
    delay(3000);
    led_microcontroller.clear();
    led_microcontroller.show(); //turn off LEDs

    switch (mode) {
      case MODE_MOUSE:
        led_microcontroller.setPixelColor(0, led_microcontroller.Color(255, 255, 0)); //yellow
        led_microcontroller.show();
        leds.setPixelColor(LED_MOUSE,led_microcontroller.Color(255,255,0)); //yellow
        leds.setPixelColor(LED_GAMEPAD,led_microcontroller.Color(0,0,0)); //off
        leds.show();
        break;
      case MODE_GAMEPAD:
        led_microcontroller.setPixelColor(0, led_microcontroller.Color(0, 0, 255)); //blue
        led_microcontroller.show();
        leds.setPixelColor(LED_GAMEPAD, led_microcontroller.Color(0, 0, 255)); //blue
        leds.setPixelColor(LED_MOUSE, led_microcontroller.Color(0, 0, 0)); //off
        leds.show();
        break;
      default:
        break;
    }

    boolean continueLoop = true;

    while (continueLoop) {
      switchS1PrevPressed = switchS1Pressed;
      switchS2PrevPressed = switchS2Pressed;
      
      switchS1Pressed = !digitalRead(PIN_SW_S1);
      switchS2Pressed = !digitalRead(PIN_SW_S2);

      if (switchS1Pressed && !switchS1PrevPressed) { // If button 1 pushed, change mode
        mode += 1;
        if (mode > 1) {
          mode = 0;
        }
        switch (mode) { //Update color
          case MODE_MOUSE:
            Serial.println("Mouse mode selected.");
            led_microcontroller.setPixelColor(0, led_microcontroller.Color(255, 255, 0)); //yellow
            led_microcontroller.show();
            break;
          case MODE_GAMEPAD:
            Serial.println("Gamepad mode selected.");
            led_microcontroller.setPixelColor(0, led_microcontroller.Color(0, 0, 255)); //blue
            led_microcontroller.show();
            break;
          default:
            break;
        }
      }

      if (switchS2Pressed && !switchS2PrevPressed) { // If button 2 pushed, exit loop
        continueLoop = false;
      }

      delay(100);
    } // end while loop

    //
    if (mode != operatingMode) { // If operating mode has changed
      operatingMode = mode;
      operatingModeFlash.write(operatingMode); //write update mode to memory
      delay(5);
    }

    if (USB_DEBUG) {
      Serial.println("Release all buttons and reset joystick...");
    }

    while (1) { //Blink operating mode color until reset
      switch (mode) {
        case MODE_MOUSE:
          led_microcontroller.setPixelColor(0, led_microcontroller.Color(255, 255, 0)); // Turn LED yellow
          led_microcontroller.show();
          break;
        case MODE_GAMEPAD:
          led_microcontroller.setPixelColor(0, led_microcontroller.Color(0, 0, 255)); // Turn LED blue
          led_microcontroller.show();
          break;
      }
      delay(1000);
      led_microcontroller.clear();
      led_microcontroller.show();
      delay(1000);
    }
    //resetJoystick();  //call reset regardless, as too much time has elapsed to enable USB
  } // end if both switches detected

}

//***UPDATE JOYSTICK DEADZONE FUNCTION***//
// Function   : updateDeadzone
//
// Description: This function updates deadzone value based on deadzone level.
//
// Parameters :  inputDeadzoneLevel : int : The input deadzone level
//
// Return     : void
//****************************************//

void updateDeadzone(int inputDeadzoneLevel) {
  if ((inputDeadzoneLevel >= JOYSTICK_MIN_DEADZONE_LEVEL) && (inputDeadzoneLevel <= JOYSTICK_MAX_DEADZONE_LEVEL)) {
    currentDeadzoneValue = int ((inputDeadzoneLevel * JOYSTICK_MAX_DEADZONE_VALUE) / JOYSTICK_MAX_DEADZONE_LEVEL);
    currentDeadzoneValue = constrain(currentDeadzoneValue, 0, JOYSTICK_MAX_DEADZONE_VALUE);
  }
}

//***UPDATE CURSOR SPEED FUNCTION***//
// Function   : updateCursorSpeed
//
// Description: This function updates cursor speed value based on cursor speed level.
//
// Parameters :  inputCursorSpeedLevel : int : The input cursor speed level
//
// Return     : void
//****************************************//

void updateCursorSpeed(int inputCursorSpeedLevel) {
  if ((inputCursorSpeedLevel >= MOUSE_MIN_CURSOR_SPEED_LEVEL) && (inputCursorSpeedLevel <= MOUSE_MAX_CURSOR_SPEED_LEVEL)) { //todo
    currentMouseCursorSpeedValue = int ((inputCursorSpeedLevel * MOUSE_MAX_CURSOR_SPEED_VALUE) / MOUSE_MAX_CURSOR_SPEED_LEVEL);
    currentMouseCursorSpeedValue = constrain(currentMouseCursorSpeedValue, 0, MOUSE_MAX_CURSOR_SPEED_VALUE);
  }
}


//***GAMEPAD BUTTON PRESS FUNCTION***//
// Function   : gamepadButtonPress
//
// Description: This function performs button press action.
//
// Parameters : int : buttonNumber
//
// Return     : void
//****************************************//
void gamepadButtonPress(int buttonNumber)
{
  //Serial.println("Button Press");
  if (buttonNumber > 0 && buttonNumber <= 8 )
  {
    gamepad.press(buttonNumber - 1);
    gamepad.send();
  }
}

//***GAMEPAD BUTTON CLICK FUNCTION***//
// Function   : gamepadButtonClick
//
// Description: This function performs button click action.
//
// Parameters : int : buttonNumber
//
// Return     : void
//****************************************//
void gamepadButtonClick(int buttonNumber)
{
  //Serial.println("Button click");
  if (buttonNumber > 0 && buttonNumber <= 8 )
  {
    gamepad.press(buttonNumber - 1);
    gamepad.send();
    delay(SWITCH_REACTION_TIME);
    gamepadButtonRelease(buttonNumber);
  }

}

//***GAMEPAD BUTTON RELEASE FUNCTION***//
// Function   : gamepadButtonRelease
//
// Description: This function performs button release action.
//
// Parameters : int* : args
//
// Return     : void
//****************************************//
void gamepadButtonRelease(int buttonNumber)
{
  //Serial.println("Button Release");
  if (buttonNumber > 0 && buttonNumber <= 8) {
    gamepad.release(buttonNumber - 1);
    gamepad.send();
  }

}

//***GAMEPAD BUTTON RELEASE FUNCTION***//
// Function   : gamepadButtonReleaseAll
//
// Description: This function performs button release action.
//
// Parameters : void
//
// Return     : void
//****************************************//
void gamepadButtonReleaseAll()
{
  //Serial.println("Button Release");
  gamepad.releaseAll();
  gamepad.send();
}


//***PERFORM GAMEPAD MOVE FUNCTION***//
// Function   : gamepadJoystickMove
//
// Description: This function performs joystick move
//
// Parameters : int : x and y : The output gamepad x and y
//
// Return     : void
//****************************************//
void gamepadJoystickMove(int x, int y)
{
  gamepad.move(x, -y);
  gamepad.send();
}

//***PERFORM MOUSE MOVE FUNCTION***//
// Function   : mouseJoystickMove
//
// Description: This function performs joystick move
//
// Parameters : int : x and y : The output gamepad x and y
//
// Return     : void
//****************************************//
void mouseJoystickMove(int x, int y)
{
  // Move the mouse based on current cursor speed
  Mouse.move(currentMouseCursorSpeedValue * outputX / JOYSTICK_MAX_VALUE, -currentMouseCursorSpeedValue * outputY / JOYSTICK_MAX_VALUE, 0); 
  
}


//***SERIAL SETTINGS FUNCTION TO CHANGE SPEED AND COMMUNICATION MODE USING SOFTWARE***//
// Function   : serialSettings
//
// Description: This function confirms if serial settings should be enabled.
//              It returns true if it's in the settings mode and is waiting for a command.
//              It returns false if it's not in the settings mode or it needs to exit the settings mode.
//
// Parameters :  enabled : bool : The input flag
//
// Return     : bool
//*************************************************************************************//
bool serialSettings(bool enabled) {

  String commandString = "";
  bool settingsFlag = enabled;

  //Set the input parameter to the flag returned. This will help to detect that the settings actions should be performed.
  if (Serial.available() > 0)
  {
    //Check if serial has received or read input string and word "SETTINGS" is in input string.
    commandString = Serial.readString();
    if (settingsFlag == false && commandString == "SETTINGS") {
      //SETTING received
      //Set the return flag to true so settings actions can be performed in the next call to the function
      printResponseInt(true, true, true, 0, commandString, false, 0);
      settingsFlag = true;
    } else if (settingsFlag == true && commandString == "EXIT") {
      //EXIT Recieved
      //Set the return flag to false so settings actions can be exited
      printResponseInt(true, true, true, 0, commandString, false, 0);
      settingsFlag = false;
    } else if (settingsFlag == true && isValidCommandFormat(commandString)) { //Check if command's format is correct and it's in settings mode
      performCommand(commandString);                  //Sub function to process valid strings
      settingsFlag = false;
    } else {
      printResponseInt(true, true, false, 0, commandString, false, 0);
      settingsFlag = false;
    }
    Serial.flush();
  }
  return settingsFlag;
}

//***PERFORM COMMAND FUNCTION TO CHANGE SETTINGS USING SOFTWARE***//
// Function   : performCommand
//
// Description: This function takes processes an input string from the serial and calls the
//              corresponding API function, or outputs an error.
//
// Parameters :  inputString : String : The input command as a string.
//
// Return     : void
//*********************************//
void performCommand(String inputString) {
  int inputIndex = inputString.indexOf(':');

  //Extract command string from input string
  String inputCommandString = inputString.substring(0, inputIndex);

  int inputCommandIndex = inputCommandString.indexOf(',');

  String inputEndpointString = inputCommandString.substring(0, inputCommandIndex);

  String inputCodeString = inputCommandString.substring(inputCommandIndex + 1);

  //Extract parameter string from input string
  String inputParameterString = inputString.substring(inputIndex + 1);

  // Determine total number of API commands
  int apiTotalNumber = sizeof(apiFunction) / sizeof(apiFunction[0]);

  //Iterate through each API command
  for (int apiIndex = 0; apiIndex < apiTotalNumber; apiIndex++) {

    // Test if input command string matches API command and input parameter string matches API parameter string
    if (inputEndpointString == apiFunction[apiIndex].endpoint &&
        inputCodeString == apiFunction[apiIndex].code) {

      // Matching Command String found
      if (!isValidCommandParameter( inputParameterString )) {
        printResponseInt(true, true, false, 2, inputString, false, 0);
      }
      else if (inputParameterString == apiFunction[apiIndex].parameter || apiFunction[apiIndex].parameter == "") {
        apiFunction[apiIndex].function(true, true, inputParameterString);
      }
      else { // Invalid input parameter

        // Outut error message
        printResponseInt(true, true, false, 3, inputString, false, 0);
      }
      break;
    } else if (apiIndex == (apiTotalNumber - 1)) { // api doesn’t exist

      //Output error message
      printResponseInt(true, true, false, 1, inputString, false, 0);

      //delay(5);
      break;
    }
  } //end iterate through API functions
}



//***CHECK IF CHAR IS A VALID DELIMITER FUNCTION***//
// Function   : isValidDelimiter
//
// Description: This function checks if the input char is a valid delimiter.
//              It returns true if the character is a valid delimiter.
//              It returns false if the character is not a valid delimiter.
//
// Parameters :  inputDelimiter : char : The input char delimiter
//
// Return     : boolean
//******************************************//
bool isValidDelimiter(char inputDelimiter) {
  bool validOutput;

  (inputDelimiter == ',' || inputDelimiter == ':' || inputDelimiter == '-') ? validOutput = true : validOutput = false;

  return validOutput;
}

//***VALIDATE INPUT COMMAND FORMAT FUNCTION***//
// Function   : isValidCommandFormat
//
// Description: This function confirms command string has correct format.
//              It returns true if the string has a correct format.
//              It returns false if the string doesn't have a correct format.
//
// Parameters :  inputCommandString : String : The input string
//
// Return     : boolean
//***********************************************//
bool isValidCommandFormat(String inputCommandString) {
  bool isValidFormat = false;
  if ((inputCommandString.length() == (6) || //XX,d:d
       inputCommandString.length() == (7)) //XX,d:dd
      && inputCommandString.charAt(2) == ',' && inputCommandString.charAt(4) == ':') {
    isValidFormat = true;
  }
  return isValidFormat;
}

//***VALIDATE INPUT COMMAND PARAMETER FUNCTION***//
// Function   : isValidCommandParameter
//
// Description: This function checks if the input string is a valid command parameters.
//              It returns true if the string includes valid parameters.
//              It returns false if the string includes invalid parameters.
//
// Parameters :  inputParamterString : String : The input string
//
// Return     : boolean
//*************************************************//
bool isValidCommandParameter(String inputParamterString) {
  if (isStrNumber(inputParamterString)) {
    return true;
  }
  return false;
}

//***CHECK IF STRING IS A NUMBER FUNCTION***//
// Function   : isStrNumber
//
// Description: This function checks if the input string is a number.
//              It returns true if the string includes all numeric characters.
//              It returns false if the string includes a non numeric character.
//
// Parameters :  str : String : The input string
//
// Return     : boolean
//******************************************//
boolean isStrNumber(String str) {
  boolean isNumber = false;
  for (byte i = 0; i < str.length(); i++)
  {
    isNumber = isDigit(str.charAt(i)) || str.charAt(i) == '+' || str.charAt(i) == '.' || str.charAt(i) == '-';
    if (!isNumber) return false;
  }
  return true;
}

//***SERIAL PRINT OUT COMMAND RESPONSE WITH STRING PARAMETER FUNCTION***//
// Function   : printResponseString
//
// Description: Serial Print output of the responses from APIs with string parameter as the output
//
// Parameters :  responseEnabled : bool : Print the response if it's set to true, and skip the response if it's set to false.
//               apiEnabled : bool : Print the response and indicate if the the function was called via the API if it's set to true.
//                                   Print Manual response if the function wasn't called via API.
//               responseStatus : bool : The response status (SUCCESS,FAIL)
//               responseNumber : int : 0,1,2 (Different meanings depending on the responseStatus)
//               responseCommand : String : The End-Point command which is returned as output.
//               responseParameterEnabled : bool : Print the parameter if it's set to true, and skip the parameter if it's set to false.
//               responseParameter : String : The response parameters printed as output.
//
// Return     : void
//***********************************************************************//
void printResponseString(bool responseEnabled, bool apiEnabled, bool responseStatus, int responseNumber, String responseCommand, bool responseParameterEnabled, String responseParameter) {
  if (responseEnabled) {

    if (responseStatus) {
      (apiEnabled) ? Serial.print("SUCCESS") : Serial.print("MANUAL");
    } else {
      Serial.print("FAIL");
    }
    Serial.print(",");
    Serial.print(responseNumber);
    Serial.print(":");
    Serial.print(responseCommand);
    if (responseParameterEnabled) {
      Serial.print(":");
      Serial.println(responseParameter);
    } else {
      Serial.println("");
    }
  }
}

//***SERIAL PRINT OUT COMMAND RESPONSE WITH INT PARAMETER FUNCTION***//
// Function   : printResponseInt
//
// Description: Serial Print output of the responses from APIs with int parameter as the output
//
// Parameters :  responseEnabled : bool : Print the response if it's set to true, and skip the response if it's set to false.
//               apiEnabled : bool : Print the response and indicate if the the function was called via the API if it's set to true.
//                                   Print Manual response if the function wasn't called via API.
//               responseStatus : bool : The response status (SUCCESS,FAIL)
//               responseNumber : int : 0,1,2 (Different meanings depending on the responseStatus)
//               responseCommand : String : The End-Point command which is returned as output.
//               responseParameterEnabled : bool : Print the parameter if it's set to true, and skip the parameter if it's set to false.
//               responseParameter : int : The response parameter printed as output.
//
// Return     : void
//***********************************************************************//
void printResponseInt(bool responseEnabled, bool apiEnabled, bool responseStatus, int responseNumber, String responseCommand, bool responseParameterEnabled, int responseParameter) {
  printResponseString(responseEnabled, apiEnabled, responseStatus, responseNumber, responseCommand, responseParameterEnabled, String(responseParameter));
}

//***SERIAL PRINT OUT COMMAND RESPONSE WITH FLOAT PARAMETER FUNCTION***//
// Function   : printResponseFloat
//
// Description: Serial Print output of the responses from APIs with float parameter as the output
//
// Parameters :  responseEnabled : bool : Print the response if it's set to true, and skip the response if it's set to false.
//               apiEnabled : bool : Print the response and indicate if the the function was called via the API if it's set to true.
//                                   Print Manual response if the function wasn't called via API.
//               responseStatus : bool : The response status (SUCCESS,FAIL)
//               responseNumber : int : 0,1,2 (Different meanings depending on the responseStatus)
//               responseCommand : String : The End-Point command which is returned as output.
//               responseParameterEnabled : bool : Print the parameter if it's set to true, and skip the parameter if it's set to false.
//               responseParameter : float : The response parameter printed as output.
//
// Return     : void
//***********************************************************************//
void printResponseFloat(bool responseEnabled, bool apiEnabled, bool responseStatus, int responseNumber, String responseCommand, bool responseParameterEnabled, float responseParameter) {
  printResponseString(responseEnabled, apiEnabled, responseStatus, responseNumber, responseCommand, responseParameterEnabled, String(responseParameter));

}


void printResponseIntArray(bool responseEnabled, bool apiEnabled, bool responseStatus, int responseNumber, String responseCommand, bool responseParameterEnabled, String responsePrefix, int responseParameterSize, char responseParameterDelimiter, int responseParameter[]) {
  char tempParameterDelimiter[1];

  (isValidDelimiter(responseParameterDelimiter)) ? tempParameterDelimiter[0] = {responseParameterDelimiter} : tempParameterDelimiter[0] = {'\0'};

  String responseParameterString = String(responsePrefix);
  for (int parameterIndex = 0; parameterIndex < responseParameterSize; parameterIndex++) {
    responseParameterString.concat(responseParameter[parameterIndex]);
    if (parameterIndex < (responseParameterSize - 1)) {
      responseParameterString.concat(tempParameterDelimiter[0]);
    };
  }

  printResponseString(responseEnabled, apiEnabled, responseStatus, responseNumber, responseCommand, responseParameterEnabled, responseParameterString);

}

//***GET DEVICE NUMBER FUNCTION***//
// Function   : getDeviceNumber
//
// Description: This function retrieves the current device number.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//
// Return     : void
//*********************************//
void getDeviceNumber(bool responseEnabled, bool apiEnabled) {
  int tempDeviceNumber = JOYSTICK_DEVICE;

  if (USB_DEBUG) { Serial.println("DEBUG: getDeviceNumber");}

  printResponseInt(responseEnabled, apiEnabled, true, 0, "DN,0", true, tempDeviceNumber);

}
//***GET DEVICE NUMBER API FUNCTION***//
// Function   : getDeviceNumber
//
// Description: This function is redefinition of main getDeviceNumber function to match the types of API function arguments.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               optionalParameter : String : The input parameter string should contain one element with value of zero.
//
// Return     : void
void getDeviceNumber(bool responseEnabled, bool apiEnabled, String optionalParameter) {
  if (optionalParameter.length() == 1 && optionalParameter.toInt() == 0) {
    getDeviceNumber(responseEnabled, apiEnabled);
  }
}


//***GET MODEL NUMBER FUNCTION***//
// Function   : getModelNumber
//
// Description: This function retrieves the current device model number.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//
// Return     : void
//*********************************//
void getModelNumber(bool responseEnabled, bool apiEnabled) {
  int tempModelNumber = JOYSTICK_MODEL;
  printResponseInt(responseEnabled, apiEnabled, true, 0, "MN,0", true, tempModelNumber);

}
//***GET MODEL NUMBER API FUNCTION***//
// Function   : getModelNumber
//
// Description: This function is redefinition of main getModelNumber function to match the types of API function arguments.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               optionalParameter : String : The input parameter string should contain one element with value of zero.
//
// Return     : void
void getModelNumber(bool responseEnabled, bool apiEnabled, String optionalParameter) {
  if (optionalParameter.length() == 1 && optionalParameter.toInt() == 0) {
    getModelNumber(responseEnabled, apiEnabled);
  }
}

//***GET VERSION FUNCTION***//
// Function   : getVersionNumber
//
// Description: This function retrieves the current device firmware version number.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//
// Return     : void
//*********************************//
void getVersionNumber(bool responseEnabled, bool apiEnabled) {
  float tempVersionNumber = JOYSTICK_VERSION;
  printResponseFloat(responseEnabled, apiEnabled, true, 0, "VN,0", true, tempVersionNumber);
}

//***GET VERSION API FUNCTION***//
// Function   : getVersionNumber
//
// Description: This function is redefinition of main getVersionNumber function to match the types of API function arguments.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               optionalParameter : String : The input parameter string should contain one element with value of zero.
//
// Return     : void
void getVersionNumber(bool responseEnabled, bool apiEnabled, String optionalParameter) {
  if (optionalParameter.length() == 1 && optionalParameter.toInt() == 0) {
    getVersionNumber(responseEnabled, apiEnabled);
  }
}

//*** GET SLOT NUMBER FUNCTION***//
/// Function   : getSlotNumber
//
// Description: This function retrieves the slot number.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//
// Return     : void
//*********************************//
int getSlotNumber(bool responseEnabled, bool apiEnabled) {
  String slotNumberCommand = "SN";
  int tempSlotNumber;
  tempSlotNumber = slotNumberFlash.read();

  if ((tempSlotNumber <= SLOT_MIN_NUMBER) || (tempSlotNumber >= SLOT_MAX_NUMBER)) {
    tempSlotNumber = SLOT_DEFAULT_NUMBER;
    slotNumberFlash.write(tempSlotNumber);
  }
  updateSlot(tempSlotNumber);

  printResponseInt(responseEnabled, apiEnabled, true, 0, "SN,0", true, tempSlotNumber);
  return tempSlotNumber;
}

//***GET SLOT NUMBER API FUNCTION***//
// Function   : getSlotNumber
//
// Description: This function is redefinition of main getSlotNumber function to match the types of API function arguments.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               optionalParameter : String : The input parameter string should contain one element with value of zero.
//
// Return     : void
void getSlotNumber(bool responseEnabled, bool apiEnabled, String optionalParameter) {
  if (optionalParameter.length() == 1 && optionalParameter.toInt() == 0) {
    getSlotNumber(responseEnabled, apiEnabled);
  }
}

//*** SET SLOT NUMBER FUNCTION***//
/// Function   : setSlotNumber
//
// Description: This function sets the slot number.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               inputSlotNumber : int : The input cursor speed level
//
// Return     : void
//*********************************//
void setSlotNumber(bool responseEnabled, bool apiEnabled, int inputSlotNumber) {
  String cursorSpeedCommand = "SN";
  if ((inputSlotNumber >= SLOT_MIN_NUMBER) && (inputSlotNumber <= SLOT_MAX_NUMBER)) {
    slotNumberFlash.write(inputSlotNumber);
    updateSlot(inputSlotNumber);
    printResponseInt(responseEnabled, apiEnabled, true, 0, "SN,1", true, inputSlotNumber); // Success output
  }
  else {
    printResponseInt(responseEnabled, apiEnabled, false, 3, "SN,1", true, inputSlotNumber); //failure output
  }
}

//***SET SLOT NUMBER CURSOR SPEED API FUNCTION***//
// Function   : setSlotNumber
//
// Description: This function is redefinition of main setSlotNumber function to match the types of API function arguments.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               optionalParameter : String : The input parameter string should contain one element with value of zero.
//
// Return     : void
void setSlotNumber(bool responseEnabled, bool apiEnabled, String optionalParameter) {
  setSlotNumber(responseEnabled, apiEnabled, optionalParameter.toInt());
}




//***GET JOYSTICK INITIALIZATION FUNCTION***//
/// Function   : getJoystickInitialization
//
// Description: This function retrieves the neutral position from the joystick initialization.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//
// Return     : void
//*********************************//
void getJoystickInitialization(bool responseEnabled, bool apiEnabled) {
  //pointFloatType tempCenterPoint = js.getInputCenter();
  //js.setMinimumRadius();                                                                    //Update the minimum cursor operating radius 
  int tempCenterPoint[2];
  tempCenterPoint[0]=xNeutral;
  tempCenterPoint[1]=yNeutral;


  printResponseIntArray(responseEnabled,        // pass through responseEnabled
                        apiEnabled,             // pass through apiEnabled
                        true,                   // responseStatus, 
                        0,                      // responseNumber
                        "IN,0",                 // responseCommand
                        true,                   // responseParameterEnabled,
                        "",                     // responsePrefix
                        2,                      // responseParameterSize
                        ',',                    // responseParameterDelimiter
                        tempCenterPoint);       // responseParameter[]


}
//***GET JOYSTICK INITIALIZATION API FUNCTION***//
// Function   : getJoystickInitialization
//
// Description: This function is redefinition of main getJoystickInitialization function to match the types of API function arguments.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               optionalParameter : String : The input parameter string should contain one element with value of zero.
//
// Return     : void
void getJoystickInitialization(bool responseEnabled, bool apiEnabled, String optionalParameter) {
  if (optionalParameter.length() == 1 && optionalParameter.toInt() == 0) {
    getJoystickInitialization(responseEnabled, apiEnabled);
  }
}

//***SET JOYSTICK INITIALIZATION FUNCTION***//
/// Function   : setJoystickInitialization
//
// Description: This function performs joystick Initialization.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//
// Return     : void
//*********************************//
void setJoystickInitialization(bool responseEnabled, bool apiEnabled) {
 // int stepNumber = 0;
 // canOutputAction = false;
 // calibTimerId[0] = calibTimer.setTimeout(CONF_JOY_INIT_START_DELAY, performJoystickCenter, (int *)stepNumber);  
  joystickNeutralCalibration();
  int tempCenterPoint[2];
  tempCenterPoint[0]=xNeutral;
  tempCenterPoint[1]=yNeutral;


  printResponseIntArray(responseEnabled,        // pass through responseEnabled
                        apiEnabled,             // pass through apiEnabled
                        true,                   // responseStatus, 
                        0,                      // responseNumber
                        "IN,1",                 // responseCommand
                        true,                   // responseParameterEnabled,
                        "",                     // responsePrefix
                        2,                      // responseParameterSize
                        ',',                    // responseParameterDelimiter
                        tempCenterPoint);       // responseParameter[]
}

//***SET JOYSTICK INITIALIZATION API FUNCTION***//
// Function   : setJoystickInitialization
//
// Description: This function is redefinition of main setJoystickInitialization function to match the types of API function arguments.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               optionalParameter : String : The input parameter string should contain one element with value of zero.
//
// Return     : void
void setJoystickInitialization(bool responseEnabled, bool apiEnabled, String optionalParameter) {
  if (optionalParameter.length() == 1 && optionalParameter.toInt() == 1) {
    setJoystickInitialization(responseEnabled, apiEnabled);
  }
}

//*** GET JOYSTICK CALIBRATION FUNCTION***//
/// Function   : getJoystickCalibration
//
// Description: This function retrieves minimum and maximum values from joystick calibration.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//
// Return     : void
//*********************************//
void getJoystickCalibration(bool responseEnabled, bool apiEnabled) {
  // String commandKey;
  // pointFloatType calibrationPointArray[5];
  // calibrationPointArray[0] = js.getInputCenter();
  // for (int i = 1; i < 5; i++)
  // {
  //   commandKey = "CA" + String(i);
  //   calibrationPointArray[i] = mem.readPoint(CONF_SETTINGS_FILE, commandKey);
  //   js.setInputMax(i, calibrationPointArray[i]);
  // }
  //js.setMinimumRadius();                                                                              //Update the minimum cursor operating radius 
  
  int calibrationPointArray[6];

  calibrationPointArray[0]=xOutputMinimum;
  calibrationPointArray[1]=xNeutral;
  calibrationPointArray[2]=xOutputMaximum;
  calibrationPointArray[3]=yOutputMinimum;
  calibrationPointArray[4]=yNeutral;
  calibrationPointArray[5]=yOutputMaximum;

  
  printResponseIntArray(responseEnabled,        // pass through responseEnabled
                        apiEnabled,             // pass through apiEnabled
                        true,                   // responseStatus, 
                        0,                      // responseNumber
                        "CA,1",                 // responseCommand
                        true,                   // responseParameterEnabled,
                        "",                     // responsePrefix
                        6,                      // responseParameterSize
                        ',',                    // responseParameterDelimiter
                        calibrationPointArray);       // responseParameter[]

}
//***GET JOYSTICK CALIBRATION API FUNCTION***//
// Function   : getJoystickCalibration
//
// Description: This function is redefinition of main getJoystickCalibration function to match the types of API function arguments.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               optionalParameter : String : The input parameter string should contain one element with value of zero.
//
// Return     : void
void getJoystickCalibration(bool responseEnabled, bool apiEnabled, String optionalParameter) {
  if (optionalParameter.length() == 1 && optionalParameter.toInt() == 0) {
    getJoystickCalibration(responseEnabled, apiEnabled);
  }
}

//*** SET JOYSTICK CALIBRATION FUNCTION***//
/// Function   : setJoystickCalibration
//
// Description: This function starts the joystick Calibration.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//
// Return     : void
//*********************************//
void setJoystickCalibration(bool responseEnabled, bool apiEnabled) {
  //todo
  //js.clear();                                                                                           //Clear previous calibration values
  //int stepNumber = 0;
  //canOutputAction = false;
  //calibTimerId[0] = calibTimer.setTimeout(CONF_JOY_CALIB_START_DELAY, performJoystickCalibration, (int *)stepNumber);  //Start the process
}
//***SET JOYSTICK CALIBRATION API FUNCTION***//
// Function   : setJoystickCalibration
//
// Description: This function is redefinition of main setJoystickCalibration function to match the types of API function arguments.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               optionalParameter : String : The input parameter string should contain one element with value of zero.
//
// Return     : void
void setJoystickCalibration(bool responseEnabled, bool apiEnabled, String optionalParameter) {
  if (optionalParameter.length() == 1 && optionalParameter.toInt() == 1) {
    setJoystickCalibration(responseEnabled, apiEnabled);
  }
}


//*** GET JOYSTICK DEADZONE FUNCTION***//
/// Function   : getJoystickDeadZone
//
// Description: This function retrieves the joystick deadzone.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//
// Return     : void
//*********************************//
int getJoystickDeadZone(bool responseEnabled, bool apiEnabled) {
  String deadZoneCommand = "DZ";
  int tempDeadzoneLevel;
  tempDeadzoneLevel = deadzoneLevelFlash.read();

  if ((tempDeadzoneLevel <= JOYSTICK_MIN_DEADZONE_LEVEL) || (tempDeadzoneLevel >= JOYSTICK_MAX_DEADZONE_LEVEL)) {
    tempDeadzoneLevel = JOYSTICK_DEFAULT_DEADZONE_LEVEL;
    deadzoneLevelFlash.write(tempDeadzoneLevel);
  }
  updateDeadzone(tempDeadzoneLevel);
  printResponseInt(responseEnabled, apiEnabled, true, 0, "DZ,0", true, tempDeadzoneLevel);
  return tempDeadzoneLevel;
}
//***GET JOYSTICK DEADZONE API FUNCTION***//
// Function   : getJoystickDeadZone
//
// Description: This function is redefinition of main getJoystickDeadZone function to match the types of API function arguments.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               optionalParameter : String : The input parameter string should contain one element with value of zero.
//
// Return     : void
void getJoystickDeadZone(bool responseEnabled, bool apiEnabled, String optionalParameter) {
  if (optionalParameter.length() == 1 && optionalParameter.toInt() == 0) {
    getJoystickDeadZone(responseEnabled, apiEnabled);
  }
}

//*** SET JOYSTICK DEADZONE FUNCTION***//
/// Function   : setJoystickDeadZone
//
// Description: This function starts the joystick deadzone.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               inputDeadzoneLevel : int : The input deadzone level
//
// Return     : void
//*********************************//
void setJoystickDeadZone(bool responseEnabled, bool apiEnabled, int inputDeadzoneLevel) {
  String deadZoneCommand = "DZ";
  if ((inputDeadzoneLevel >= JOYSTICK_MIN_DEADZONE_LEVEL) && (inputDeadzoneLevel <= JOYSTICK_MAX_DEADZONE_LEVEL)) {
    deadzoneLevelFlash.write(inputDeadzoneLevel);
    updateDeadzone(inputDeadzoneLevel);
    printResponseInt(responseEnabled, apiEnabled, true, 0, "DZ,1", true, inputDeadzoneLevel);
  }
  else {
    printResponseInt(responseEnabled, apiEnabled, false, 3, "DZ,1", true, inputDeadzoneLevel);
  }
}
//***SET JOYSTICK DEADZONE API FUNCTION***//
// Function   : setJoystickDeadZone
//
// Description: This function is redefinition of main setJoystickDeadZone function to match the types of API function arguments.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               optionalParameter : String : The input parameter string should contain one element with value of zero.
//
// Return     : void
void setJoystickDeadZone(bool responseEnabled, bool apiEnabled, String optionalParameter) {
  setJoystickDeadZone(responseEnabled, apiEnabled, optionalParameter.toInt());
}

 
//*** GET MOUSER CURSOR SPEEDE FUNCTION***//
/// Function   : getMouseCursorSpeed
//
// Description: This function retrieves the mouse cursor speed.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//
// Return     : void
//*********************************//
int getMouseCursorSpeed(bool responseEnabled, bool apiEnabled) {
  String cursorSpeedCommand = "SS";
  int tempCursorSpeedLevel;
  tempCursorSpeedLevel = cursorSpeedLevelFlash.read();

  if ((tempCursorSpeedLevel <= MOUSE_MIN_CURSOR_SPEED_LEVEL) || (tempCursorSpeedLevel >= MOUSE_MAX_CURSOR_SPEED_LEVEL)) {
    tempCursorSpeedLevel = MOUSE_DEFAULT_CURSOR_SPEED_LEVEL;
    cursorSpeedLevelFlash.write(tempCursorSpeedLevel);
  }
  updateCursorSpeed(tempCursorSpeedLevel);
  printResponseInt(responseEnabled, apiEnabled, true, 0, "SS,0", true, tempCursorSpeedLevel);
  return tempCursorSpeedLevel;
}

//***GET MOUSE CURSOR SPEED API FUNCTION***//
// Function   : getMouseCursorSpeed
//
// Description: This function is redefinition of main getMouseCursorSpeed function to match the types of API function arguments.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               optionalParameter : String : The input parameter string should contain one element with value of zero.
//
// Return     : void
void getMouseCursorSpeed(bool responseEnabled, bool apiEnabled, String optionalParameter) {
  if (optionalParameter.length() == 1 && optionalParameter.toInt() == 0) {
    getMouseCursorSpeed(responseEnabled, apiEnabled);
  }
}

//*** SET MOUSE CURSOR SPEED FUNCTION***//
/// Function   : setMouseCursorSpeed
//
// Description: This function sets the mouse cursor speed.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               inputCursorSpeedLevel : int : The input cursor speed level
//
// Return     : void
//*********************************//
void setMouseCursorSpeed(bool responseEnabled, bool apiEnabled, int inputCursorSpeedLevel) {
  String cursorSpeedCommand = "SS";
  if ((inputCursorSpeedLevel >= MOUSE_MIN_CURSOR_SPEED_LEVEL) && (inputCursorSpeedLevel <= MOUSE_MAX_CURSOR_SPEED_LEVEL)) {
    cursorSpeedLevelFlash.write(inputCursorSpeedLevel);
    updateCursorSpeed(inputCursorSpeedLevel);
    printResponseInt(responseEnabled, apiEnabled, true, 0, "SS,1", true, inputCursorSpeedLevel);
  }
  else {
    printResponseInt(responseEnabled, apiEnabled, false, 3, "SS,1", true, inputCursorSpeedLevel);
  }
}

//***SET MOUSE CURSOR SPEED API FUNCTION***//
// Function   : setMouseCursorSpeed
//
// Description: This function is redefinition of main setMouseCursorSpeed function to match the types of API function arguments.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               optionalParameter : String : The input parameter string should contain one element with value of zero.
//
// Return     : void
void setMouseCursorSpeed(bool responseEnabled, bool apiEnabled, String optionalParameter) {
  setMouseCursorSpeed(responseEnabled, apiEnabled, optionalParameter.toInt());
}

//***CALCULATE THE MAGNITUDE OF A VECTOR***//
// Function   : calcMag
//
// Description: This function calculates the magntidue of a vector using the x and y values.
//              It returns a double containing the magnitude of the vector.
//
// Parameters :  x : double : The x component of the vector
//               y : double : The y component of the vector
//
// Return     : double
//******************************************//
double calcMag(double x, double y) {
  double magnitude = sqrt(x * x + y * y);

  return magnitude;
}

//***GET OPERATING MODE STATE FUNCTION***//
// Function   : getOperatingMode
//
// Description: This function retrieves the state of operating mode.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//
// Return     : operatingState : intol : The current state of operating mode.
//*********************************//
int getOperatingMode(bool responseEnabled, bool apiEnabled) {
  String commandKey = "OM";

    int tempOperatingMode;
  // tempOperatingMode = mem.readInt(CONF_SETTINGS_FILE, commandKey);
  tempOperatingMode = operatingModeFlash.read();

  if ((tempOperatingMode < CONF_OPERATING_MODE_MIN) || (tempOperatingMode > CONF_OPERATING_MODE_MAX)) {
    tempOperatingMode = CONF_OPERATING_MODE_DEFAULT;
    // mem.writeInt(CONF_SETTINGS_FILE, commandKey, tempOperatingMode);
    operatingModeFlash.write(tempOperatingMode);
  }

  printResponseInt(responseEnabled, apiEnabled, true, 0, "OM,0", true, tempOperatingMode);

  return tempOperatingMode;
}
//***GET OPERATING MODE STATE API FUNCTION***//
// Function   : getOperatingMode
//
// Description: This function is redefinition of main getOperatingMode function to match the types of API function arguments.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               optionalParameter : String : The input parameter string should contain one element with value of zero.
//
// Return     : void
void getOperatingMode(bool responseEnabled, bool apiEnabled, String optionalParameter) {
  if (optionalParameter.length() == 1 && optionalParameter.toInt() == 0) {
    getOperatingMode(responseEnabled, apiEnabled);
  }
}

//***SET OPERATING MODE STATE FUNCTION***//
// Function   : setOperatingMode
//
// Description: This function sets the state of operating mode.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               inputOperatingMode : int : The new operating mode state 
//
// Return     : void
//*********************************//
void setOperatingMode(bool responseEnabled, bool apiEnabled, int inputOperatingMode) {
  String commandKey = "OM";
  
  if (USB_DEBUG) { Serial.println("DEBUG: setOperatingMode");}
   
  if ((inputOperatingMode >= CONF_OPERATING_MODE_MIN) && (inputOperatingMode <= CONF_OPERATING_MODE_MAX)) {   
    // mem.writeInt(CONF_SETTINGS_FILE, commandKey, inputOperatingMode);
    if (inputOperatingMode != operatingMode){
      operatingModeFlash.write(inputOperatingMode);
      delay(FLASH_DELAY_TIME);
      operatingMode = inputOperatingMode;
      //changeOperatingMode(inputOperatingMode);
      softwareReset(); // perform reset if mode changes
    }
    printResponseInt(responseEnabled, apiEnabled, true, 0, "OM,1", true, inputOperatingMode);
  
  }
  else {
    printResponseInt(responseEnabled, apiEnabled, false, 3, "OM,1", true, inputOperatingMode);
  }

  

}
//***SET OPERATING MODE STATE API FUNCTION***//
// Function   : setOperatingMode
//
// Description: This function is redefinition of main setOperatingMode function to match the types of API function arguments.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               optionalParameter : String : The input parameter string should contain one element with value of zero.
//
// Return     : void
void setOperatingMode(bool responseEnabled, bool apiEnabled, String optionalParameter) {
  setOperatingMode(responseEnabled, apiEnabled, optionalParameter.toInt());
}

//***GET JOYSTICK VALUE FUNCTION***//
// Function   : getJoystickValue
//
// Description: This function returns raw joystick value.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//
// Return     : void
//*********************************//
void getJoystickValue(bool responseEnabled, bool apiEnabled) {
  readJoystick(); // Measure and calculate new joystick reading

  const int outputArraySize = 8;
  int tempJoystickArray[outputArraySize];
  tempJoystickArray[0] = rawX;
  tempJoystickArray[1] = rawY;

  tempJoystickArray[2] = inputX;
  tempJoystickArray[3] = inputY;

  tempJoystickArray[4] = centeredX;
  tempJoystickArray[5] = centeredY;

  tempJoystickArray[6] = outputX;
  tempJoystickArray[7] = outputY;

  printResponseIntArray(responseEnabled, apiEnabled, true, 0, "JV,0", true, "", outputArraySize, ',', tempJoystickArray);
  
}
//***GET JOYSTICK VALUE API FUNCTION***//
// Function   : getJoystickValue
//
// Description: This function is redefinition of main getJoystickValue function to match the types of API function arguments.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               optionalParameter : String : The input parameter string should contain one element with value of zero.
//
// Return     : void
void getJoystickValue(bool responseEnabled, bool apiEnabled, String optionalParameter) {
  if (optionalParameter.length() == 1 && optionalParameter.toInt() == 0) {
    getJoystickValue(responseEnabled, apiEnabled);
  }
}

//***SOFT RESET FUNCTION***//
// Function   : softReset
//
// Description: This function performs a software reset.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//
// Return     : void
//***************************//
void softReset(bool responseEnabled, bool apiEnabled) {

  printResponseInt(responseEnabled, apiEnabled, true, 0, "SR,1", true, 1);
  softwareReset();

}
//***SOFT RESET API FUNCTION***//
// Function   : softReset
//
// Description: This function is redefinition of main softReset function to match the types of API function arguments.
//
// Parameters :  responseEnabled : bool : The response for serial printing is enabled if it's set to true.
//                                        The serial printing is ignored if it's set to false.
//               apiEnabled : bool : The api response is sent if it's set to true.
//                                   Manual response is sent if it's set to false.
//               optionalParameter : String : The input parameter string should contain one element with value of zero.
//
// Return     : void
void softReset(bool responseEnabled, bool apiEnabled, String optionalParameter) {
  if (optionalParameter.length() == 1 && optionalParameter.toInt() == 1) {
    softReset(responseEnabled, apiEnabled);
  }
}

//***INITIATE SOFTWARE RESET***//
// Function   : softwareReset
//
// Description: This function initiates a software reset.
//
// Parameters :  none
//
// Return     : none
//******************************************//
void softwareReset() {
  if (USB_DEBUG) { Serial.println("DEBUG: softwareReset");}
  
  switch (operatingMode) {
    case MODE_GAMEPAD:            // New mode is Gamepad, active mode is Mouse
      if (USB_DEBUG) { Serial.println("DEBUG: End mouse");}
      Mouse.release(MOUSE_LEFT);
      Mouse.release(MOUSE_MIDDLE);
      Mouse.release(MOUSE_RIGHT);
      Mouse.end();
      delay(10);
      break;
    case MODE_MOUSE:              // New mode is Mouse, active mode is Gamepad
    if (USB_DEBUG) { Serial.println("DEBUG: End gamepad");}
      gamepadButtonReleaseAll();
      gamepad.end();
      delay(10);
      break;
  }
  if (USB_DEBUG) { Serial.println("DEBUG: Attempt reset");}
  delay(1000);
  
  NVIC_SystemReset();  // Software reset
  delay(10);
}
