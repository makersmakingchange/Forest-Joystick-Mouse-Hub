

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

void setup() {
  leds.begin();
  leds.setBrightness(ledsBrightness);
  leds.setPixelColor(0,leds.Color(0,255,0)); //red g r b
  leds.setPixelColor(1,leds.Color(165,255,0)); //orange
  leds.setPixelColor(2,leds.Color(255,255,0)); //yellow
  leds.setPixelColor(3,leds.Color(255,0,0)); //green
  leds.setPixelColor(4,leds.Color(0,0,255)); //blue
  leds.show();

}

void loop() {
  // put your main code here, to run repeatedly:

}
