const float X_SPEED = 3.0f;   // sensitivity
const float Y_SPEED = 3.0f;   // sensitivity

// Sigmoid curve parameters
int T1 = 511; // amplitude
float T2 = 0.015f; //growth
int T3 = 255; // midpoint
float XI = 0.5; //shape parameter

const int ACCEL_SIZE = 513;
int X_RESPONSE[ACCEL_SIZE];
int Y_RESPONSE[ACCEL_SIZE];

int MOUSE_DIR_X = 1;
int MOUSE_DIR_Y = -1; //flip mouse y axis



//***UPDATE RESPONSE FUNCTION**//
// Function   : updateResponse
//
// Description: This function calculates the response curve table for the joystick.
//
// Parameters :  deadzone
//
// Return     : Void
//*********************************//

void updateResponse(int deadzone) {
  for (int i = 0; i < ACCEL_SIZE; i++ ) {
    if (i < deadzone) {
      X_RESPONSE[i] = 0;
      Y_RESPONSE[i] = 0;
    } else {
      //X_RESPONSE[i] = max(1, int((pow(i / 511.0f, X_SPEED) * 511) + 0.5f)); // maps from 1 to 511
      //Y_RESPONSE[i] = max(1, int((pow(i / 511.0f, Y_SPEED) * 511) + 0.5f));

      X_RESPONSE[i] = max(1, int(T1 / pow((1 + XI * exp(-T2 * (i - T3))), (1.0f / XI)) ));
      Y_RESPONSE[i] = max(1, int(T1 / pow((1 + XI * exp(-T2 * (i - T3))), (1.0f / XI)) ));
    }
  }
  X_RESPONSE[512] = X_RESPONSE[511];
  Y_RESPONSE[512] = Y_RESPONSE[511];

  for (int i = 0; i < ACCEL_SIZE; i++ ) {
    Serial.print(i); Serial.print("\t"); Serial.println(X_RESPONSE[i]);
  }
}
