

#include <Adafruit_NeoPixel.h> //Lights on QtPy

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

Adafruit_NeoPixel leds(5, PIN_LEDS,NEO_GRB + NEO_KHZ800);  // Create a pixel strand with 5 pixels on QtPy
int ledsBrightness = 50;

int switchS1State;
int switchS2State;
int switchS3State;
int switchS4State;
int buttonState;

#define BUTTON_CAL_MAX
#define BUTTON_CAL_MIN
#define BUTTON_MODE_MAX
#define BUTTON_MODE_MIN


void setup() {
  
// Begin serial
  Serial.begin(115200);
  
  leds.begin();
  leds.setBrightness(ledsBrightness);
  leds.clear();

    //Initialize the switch pins as inputs
  pinMode(PIN_SW_S1, INPUT_PULLUP);
  pinMode(PIN_SW_S2, INPUT_PULLUP);
  pinMode(PIN_SW_S3, INPUT_PULLUP);
  pinMode(PIN_SW_S4, INPUT_PULLUP);
  pinMode(PIN_BUTTON, INPUT_PULLUP);  

}

void loop() {
  
  switchS1State = !digitalRead(PIN_SW_S1);
  switchS2State = !digitalRead(PIN_SW_S2);
  switchS3State = !digitalRead(PIN_SW_S3);
  switchS4State = !digitalRead(PIN_SW_S4);

  buttonState = analogRead(PIN_BUTTON);

  Serial.print("S1: ");
  Serial.print(switchS1State);
  
  Serial.print("\t S2: ");
  Serial.print(switchS2State);

  Serial.print("\t S3: ");
  Serial.print(switchS3State);

  Serial.print("\t S4: ");
  Serial.print(switchS4State);

  Serial.print("\t SM: ");
  Serial.println(buttonState);

  delay(200);
  

}
