/*
  File: OpenAT_Joystick_Software.ino
  Software: OpenAT Joystick Plus 4 Switches, with both USB gamepad and mouse funtionality.
  Developed by: Makers Making Change
  Version: (25 September 2023)
  License: GPL v3

  Copyright (C) 2023 Neil Squire Society
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
#include <TinyUSB_Mouse_and_Keyboard.h> // Convenience library for 
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


#define LED_SLOT1         0
#define LED_SLOT2         1
#define LED_SLOT3         2
#define LED_GAMEPAD       3
#define LED_MOUSE         4
#define LED_DEFAULT_BRIGHTNESS 50


#define MODE_MOUSE 1
#define MODE_GAMEPAD 0
#define DEFAULT_MODE MODE_GAMEPAD

#define UPDATE_INTERVAL   5 // TBD Update interval for perfoming HID actions (in milliseconds)
#define DEFAULT_DEBOUNCING_TIME 5

#define JOYSTICK_DEFAULT_DEADZONE_LEVEL      1              //Joystick deadzone
#define JOYSTICK_MIN_DEADZONE_LEVEL          1
#define JOYSTICK_MAX_DEADZONE_LEVEL          10
#define JOYSTICK_MAX_DEADZONE_VALUE          64             //Out of 127
#define JOYSTICK_MAX_VALUE                   127

#define MOUSE_DEFAULT_CURSOR_SPEED_LEVEL     5              // Default cursor speed level
#define MOUSE_MIN_CURSOR_SPEED_LEVEL         1              //Minimum cursor speed level
#define MOUSE_MAX_CURSOR_SPEED_LEVEL         10             //Maxium cursor speed level
#define MOUSE_MIN_CURSOR_SPEED_VALUE         1              //Minimum cursor speed value [pixels per update]
#define MOUSE_MAX_CURSOR_SPEED_VALUE         10

#define JOYSTICK_REACTION_TIME               30             //Minimum time between each action in ms
#define SWITCH_REACTION_TIME                 100            //Minimum time between each switch action in ms
#define STARTUP_DELAY_TIME                   5000           //Time to wait on startup
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
int ledBrightness;



FlashStorage(isConfiguredFlash, int);
FlashStorage(deviceNumberFlash,int);
FlashStorage(modelNumberFlash, int);
FlashStorage(versionNumberFlash, int);
FlashStorage(deadzoneLevelFlash, int);
FlashStorage(cursorSpeedLevelFlash, int);
FlashStorage(operatingModeFlash, int);
FlashStorage(ledBrightnessFlash,int);
FlashStorage(currentSlotFlash,int);  // Track index of current settings slot

long lastInteractionUpdate;

//Declare joystick input and output variables
int inputX;
int inputY;
int outputX;
int outputY;

//Declare switch state variables for each switch
bool switchS1State;           // Mouse mode = left click
bool switchS2State;           // Mouse mode = scroll mode
bool switchS3State;           // Mouse mode = right click
bool switchS4State;           // Mouse mode = middle click
bool switchSMState;
bool buttonCState;
bool buttonMState;

//Previous status of switches
bool switchS1PrevState = HIGH;
bool switchS2PrevState = HIGH;
bool switchS3PrevState = HIGH;
bool switchS4PrevState = HIGH;
bool switchSMPrevState = HIGH;
bool buttonCPrevState  = HIGH;
bool buttonMPrevState  = HIGH;

// Magic numbers for multi-button analog resistor network.
// SM_Switch - Mode Switch - Calib Switch
// voltages v000 = voltage when off-off-off
const int v000 = 644;
const int v001 = 610;
const int v010 = 563;
const int v011 = 513;
const int v100 = 419;
const int v101 = 327;
const int v110 = 185;
const int v111 = 1;
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


// Declare individual API functions with command, parameter, and corresponding function
_functionList getDeviceNumberFunction =           {"DN", "0", "0", &getDeviceNumber};
_functionList getModelNumberFunction =            {"MN", "0", "0", &getModelNumber};
_functionList getVersionNumberFunction =          {"VN", "0", "0", &getVersionNumber};
_functionList getJoystickDeadZoneFunction =       {"DZ", "0", "0", &getJoystickDeadZone};
_functionList setJoystickDeadZoneFunction =       {"DZ", "1", "",  &setJoystickDeadZone};
_functionList getMouseCursorSpeedFunction =       {"SS", "0", "0", &getMouseCursorSpeed};
_functionList setMouseCursorSpeedFunction =       {"SS", "1", "",  &setMouseCursorSpeed};
//_functionList getLEDBrightnessFunction =          {"LB", "0", "",  &getLEDBrightness};
//_functionList setLEDBrightnessFunction =          {"LB", "1", "",  &setLEDBrightness};

// Declare array of API functions
_functionList apiFunction[8] = {
  getDeviceNumberFunction,
  getModelNumberFunction,
  getVersionNumberFunction,
  getJoystickDeadZoneFunction,
  setJoystickDeadZoneFunction,
  getMouseCursorSpeedFunction,
  setMouseCursorSpeedFunction
};

//Switch properties
const switchStruct switchProperty[] {
  {1, "S1", 1},
  {2, "S2", 2},
  {3, "S3", 3},
  {4, "S4", 4}

};

Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL);  // Create a pixel strand with 1 pixel on QtPy
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
    // Initialize Memory
  initMemory();
  delay(FLASH_DELAY_TIME);
  
  pixels.begin(); // Initiate pixel on microcontroller
  pixels.clear();
  pixels.setBrightness(ledBrightness);
  pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // Turn LED red to start
  pixels.show();

  leds.begin();
  leds.clear();
  leds.setBrightness(ledBrightness);
  leds.show();
 
  
    // Begin HID gamepad or mouse, depending on mode selection
  switch (operatingMode) {
    case MODE_MOUSE:
      initMouse();
      break;
    case MODE_GAMEPAD:
      gamepad.begin();
      break;
  }

  // Initialize Joystick
  initJoystick();

  // Begin serial
  Serial.begin(115200);

  //Initialize the switch pins as inputs
  pinMode(PIN_SW_S1, INPUT_PULLUP);
  pinMode(PIN_SW_S2, INPUT_PULLUP);
  pinMode(PIN_SW_S3, INPUT_PULLUP);
  pinMode(PIN_SW_S4, INPUT_PULLUP);
  pinMode(PIN_BUTTON, INPUT);


  checkSetupMode(); // Check to see if operating mode change
  delay(STARTUP_DELAY_TIME);

 


  // Turn on indicator light, depending on mode selection
  switch (operatingMode) {
    case MODE_MOUSE:
      pixels.setPixelColor(0, pixels.Color(255, 255, 0)); // Turn LED yellow
      pixels.show();
      leds.setPixelColor(LED_MOUSE,pixels.Color(255,255,0));// Turn LED yellow
      leds.show();
      break;
    case MODE_GAMEPAD:
      pixels.setPixelColor(0, pixels.Color(0, 0, 255)); // Turn LED blue
      pixels.show();
      leds.setPixelColor(LED_GAMEPAD, pixels.Color(0, 0, 255)); // Turn LED blue
      leds.show();
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

    switch (operatingMode) {
      case MODE_MOUSE:
        switchesMouseActions();
        break;
      case MODE_GAMEPAD:
        switchesJoystickActions();
        break;
    }
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
    cursorSpeedLevel = MOUSE_DEFAULT_CURSOR_SPEED_LEVEL;
    operatingMode = DEFAULT_MODE;
    ledBrightness = LED_DEFAULT_BRIGHTNESS;
    isConfigured = 1;

    //Write default settings to flash storage
    modelNumberFlash.write(modelNumber);
    versionNumberFlash.write(versionNumber);
    deadzoneLevelFlash.write(deadzoneLevel);
    cursorSpeedLevelFlash.write(cursorSpeedLevel);
    operatingModeFlash.write(operatingMode);
    ledBrightnessFlash.write(ledBrightness);

    isConfiguredFlash.write(isConfigured);
    delay(FLASH_DELAY_TIME);

  } else {
    //Load settings from flash storage
    modelNumber = modelNumberFlash.read();
    versionNumber = versionNumberFlash.read();
    deadzoneLevel = deadzoneLevelFlash.read();
    cursorSpeedLevel = cursorSpeedLevelFlash.read();
    operatingMode = operatingModeFlash.read();
    ledBrightness = ledBrightnessFlash.read();
    delay(FLASH_DELAY_TIME);
  }

  //Serial print settings
  Serial.print("Model Number: ");
  Serial.println(modelNumber);

  Serial.print("Version Number: ");
  Serial.println(versionNumber);

  Serial.print("Operating Mode: ");
  Serial.println(operatingMode);

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
  getJoystickDeadZone(true, false);                                         //Get joystick deadzone stored in memory                                      //Get joystick calibration points stored in flash memory
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

  //Read analog raw value using ADC
  inputX = analogRead(PIN_JOYSTICK_X);
  inputY = analogRead(PIN_JOYSTICK_Y);

  //Map joystick x and y move values
  outputX = map(inputX, 0, 1023, -127, 127);
  outputY = map(inputY, 0, 1023, -127, 127);

  double outputMag = calcMag(outputX, outputY);

  //Apply radial deadzone *********************************************************************
  if (outputMag <= currentDeadzoneValue)
  {
    outputX = 0;
    outputY = 0;
  }

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
  switchS1PrevState = switchS1State;
  switchS2PrevState = switchS2State;
  switchS3PrevState = switchS3State;
  switchS4PrevState = switchS4State;
  switchSMPrevState = switchSMState;
  buttonCPrevState  = buttonCState;
  buttonMPrevState  = buttonMState;

  //Update status of switch inputs
  switchS1State = digitalRead(PIN_SW_S1);
  switchS2State = digitalRead(PIN_SW_S2);
  switchS3State = digitalRead(PIN_SW_S3);
  switchS4State = digitalRead(PIN_SW_S4);

  int buttonState   = analogRead(PIN_BUTTON); // read the button array

   if (buttonState < t111)
 {
  switchSMState = true;
  buttonMState = true;
  buttonCState = true;
 }
 else if (buttonState < t110)
 {
  switchSMState = true;
  buttonMState = true;
  buttonCState = false;
 }
 else if (buttonState < t101)
  {
  switchSMState = true;
  buttonMState = false;
  buttonCState = true;
 }
 else if (buttonState < t100)
  {
  switchSMState = true;
  buttonMState = false;
  buttonCState = false;
 }
  else if (buttonState < t011)
  {
  switchSMState = false;
  buttonMState = true;
  buttonCState = true;
 }
  else if (buttonState < t010)
  {
  switchSMState = false;
  buttonMState = true;
  buttonCState = false;
 }
  else if (buttonState < t001)
  {
  switchSMState = false;
  buttonMState = false;
  buttonCState = true;
 }
  else 
  {
  switchSMState = false;
  buttonMState = false;
  buttonCState = false;
 }


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
  if (!switchS1State) {
    gamepadButtonPress(switchProperty[0].switchButtonNumber);
  }
  else if (switchS1State && !switchS1PrevState) {
    gamepadButtonRelease(switchProperty[0].switchButtonNumber);
  }

  if (!switchS2State) {
    gamepadButtonPress(switchProperty[1].switchButtonNumber);
  } else if (switchS2State && !switchS2PrevState) {
    gamepadButtonRelease(switchProperty[1].switchButtonNumber);
  }

  if (!switchS3State) {
    gamepadButtonPress(switchProperty[2].switchButtonNumber);
  } else if (switchS3State && !switchS3PrevState) {
    gamepadButtonRelease(switchProperty[2].switchButtonNumber);
  }

  if (!switchS4State) {
    gamepadButtonPress(switchProperty[3].switchButtonNumber);
  } else if (switchS4State && !switchS4PrevState) {
    gamepadButtonRelease(switchProperty[3].switchButtonNumber);
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
  getMouseCursorSpeed(true,false);        //Get mouse cursor speed stored in memory
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
  if (!switchS1State) {
    Mouse.press(MOUSE_LEFT);
  }
  else if (switchS1State && !switchS1PrevState) {
    Mouse.release(MOUSE_LEFT);
  }

  if (!switchS2State) {
    Mouse.press(MOUSE_MIDDLE);
  } else if (switchS2State && !switchS2PrevState) {
    Mouse.release(MOUSE_MIDDLE);
  }

  if (!switchS3State) {
    Mouse.press(MOUSE_RIGHT);
  } else if (switchS3State && !switchS3PrevState) {
    Mouse.release(MOUSE_RIGHT);
  }

  if (!switchS4State) {
    int counter = 0;
    while (!switchS4State) {

      readJoystick();
      readSwitches();

      if (outputY > (currentDeadzoneValue)) {
        Mouse.move(0, 0, 1);                  // Scroll up
        counter++;
      }
      else if (outputY < -currentDeadzoneValue) {
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
  }
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
  switchS1State = digitalRead(PIN_SW_S1);
  switchS2State = digitalRead(PIN_SW_S2);
  switchS3State = digitalRead(PIN_SW_S3);
  switchS4State = digitalRead(PIN_SW_S4);

  int mode = 0;

  if (!switchS1State && !switchS2State) {
    Serial.println("Mode selection: Press Button SW1 to Switch Mode, Button SW2 to Confirm selection");
    pixels.setPixelColor(0, pixels.Color(0, 255, 0)); //green
    pixels.show();
    delay(3000);
    pixels.clear();
    pixels.show(); //turn off LEDs

    switch (mode) {
      case MODE_MOUSE:
        pixels.setPixelColor(0, pixels.Color(255, 255, 0)); //yellow
        pixels.show();
        leds.setPixelColor(LED_MOUSE,pixels.Color(255,255,0)); //yellow
        leds.setPixelColor(LED_GAMEPAD,pixels.Color(0,0,0)); //off
        leds.show();
        break;
      case MODE_GAMEPAD:
        pixels.setPixelColor(0, pixels.Color(0, 0, 255)); //blue
        pixels.show();
        leds.setPixelColor(LED_GAMEPAD, pixels.Color(0, 0, 255)); //blue
        leds.setPixelColor(LED_MOUSE, pixels.Color(0, 0, 0)); //off
        leds.show();
        break;
      default:
        break;
    }

    boolean continueLoop = true;

    while (continueLoop) {
      switchS1PrevState = switchS1State;
      switchS2PrevState = switchS2State;
      
      switchS1State = digitalRead(PIN_SW_S1);
      switchS2State = digitalRead(PIN_SW_S2);

      if (!switchS1State && switchS1PrevState) { // If button 1 pushed, change mode
        mode += 1;
        if (mode > 1) {
          mode = 0;
        }
        switch (mode) { //Update color
          case MODE_MOUSE:
            Serial.println("Mouse mode selected.");
            pixels.setPixelColor(0, pixels.Color(255, 255, 0)); //yellow
            pixels.show();
            break;
          case MODE_GAMEPAD:
            Serial.println("Gamepad mode selected.");
            pixels.setPixelColor(0, pixels.Color(0, 0, 255)); //blue
            pixels.show();
            break;
          default:
            break;
        }
      }

      if (!switchS2State && switchS2PrevState) { // If button 2 pushed, exit loop
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
          pixels.setPixelColor(0, pixels.Color(255, 255, 0)); // Turn LED yellow
          pixels.show();
          break;
        case MODE_GAMEPAD:
          pixels.setPixelColor(0, pixels.Color(0, 0, 255)); // Turn LED blue
          pixels.show();
          break;
      }
      delay(1000);
      pixels.clear();
      pixels.show();
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
  Mouse.move(currentMouseCursorSpeedValue * outputX / 127, -currentMouseCursorSpeedValue * outputY / 127, 0); 
  
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
