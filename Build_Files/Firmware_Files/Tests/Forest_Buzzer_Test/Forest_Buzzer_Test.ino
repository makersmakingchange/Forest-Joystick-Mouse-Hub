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

#define NOTE_F5 698                                                     //frequency for f5
#define NOTE_A5 880                                                     //frequency for a5
#define NOTE_C6 1047                                                    //frequency for c6
#define NOTE_E6 1319                                                    //frequency for e6

void setup() {
  tone(PIN_BUZZER, NOTE_A5, 500);
  delay(500);
  tone(PIN_BUZZER, NOTE_C6, 375);
  delay(375);
  tone(PIN_BUZZER, NOTE_C6, 125);
  delay(125);
  tone(PIN_BUZZER, NOTE_F5, 750);
  delay(750);
  noTone(PIN_BUZZER);

}

void loop() {
  

}
