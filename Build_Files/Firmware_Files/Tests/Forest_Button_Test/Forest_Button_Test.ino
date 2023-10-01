

#include <Adafruit_NeoPixel.h> //Lights on QtPy

// ==================== Pin Assignment =================

const int PIN_JOYSTICK_X =  A0;
const int PIN_JOYSTICK_Y =  A1;
const int PIN_SW_S1 =       A2;
const int PIN_SW_S2 =       A3;
const int PIN_SW_S3 =       A6; //TX
const int PIN_SW_S4 =       A10; //MO
const int PIN_BUTTON =      A9; //MI
const int PIN_LEDS =        A7; //RX
const int  PIN_BUZZER =     A8; //SCK

Adafruit_NeoPixel leds(5, PIN_LEDS,NEO_GRB + NEO_KHZ800);  // Create a pixel strand with 5 pixels on QtPy
int ledsBrightness = 50;

bool switchS1State;
bool switchS2State;
bool switchS3State;
bool switchS4State;
int buttonState;
bool switchSMState;
bool buttonCState;
bool buttonMState;

#define BUTTON_CAL_MAX
#define BUTTON_CAL_MIN
#define BUTTON_MODE_MAX
#define BUTTON_MODE_MIN

const int thresholdCount = 7;
const int t001 = (644+610)/2;
const int t010 = (610+563)/2;
const int t011 = (563+513)/2;
const int t100 = (513+419)/2;
const int t101 = (419+327)/2;
const int t110 = (327+185)/2;
const int t111 = (185+1)/2;
const int thresholds[thresholdCount]={t111,t110,t101,t100,t011,t010,t001};



void setup() {
  
// Begin serial
  Serial.begin(115200);
  
  leds.begin();
  leds.setBrightness(ledsBrightness);
  leds.clear();

    //Initialize the switch pins as inputs
  pinMode(PIN_SW_S1,  INPUT_PULLUP);
  pinMode(PIN_SW_S2,  INPUT_PULLUP);
  pinMode(PIN_SW_S3,  INPUT_PULLUP);
  pinMode(PIN_SW_S4,  INPUT_PULLUP);
  pinMode(PIN_BUTTON, INPUT);  

}

void loop() {
  
  switchS1State = !digitalRead(PIN_SW_S1);
  switchS2State = !digitalRead(PIN_SW_S2);
  switchS3State = !digitalRead(PIN_SW_S3);
  switchS4State = !digitalRead(PIN_SW_S4);

  buttonState = analogRead(PIN_BUTTON);

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
 
  

  Serial.print("S1: ");
  Serial.print(switchS1State);
  
  Serial.print("\t S2: ");
  Serial.print(switchS2State);

  Serial.print("\t S3: ");
  Serial.print(switchS3State);

  Serial.print("\t S4: ");
  Serial.print(switchS4State);

  Serial.print("\t SM: ");
  Serial.print(buttonState);

  Serial.print("\t SM: ");
  Serial.print(switchSMState);

  Serial.print("\t M: ");
  Serial.print(buttonMState);

  Serial.print("\t C: ");
  Serial.println(buttonCState);

  

  delay(200);
  

}
