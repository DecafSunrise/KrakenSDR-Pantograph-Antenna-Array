#define MIN_WAVELENGTH     153.0 // millimeters
#define MAX_WAVELENGTH     900.0 // millimeters

// Stepper controller details
#define X_STEP_BIT         5  // Nano Digital Pin 5
#define X_DIRECTION_BIT    2  // Nano Digital Pin 2
#define X_LIMIT_BIT        9  // Digital Pin 9
#define X_MICROSTEPPING    1  // No microstepping

// Leadscrew details
#define rotation           1.41111111  // 25.4mm / 18 (threads per inch) in millimeters
#define stepsPerRotation   200

// Serial variables
char incomingByte;                    // a variable to read incoming serial data into
String incomingFrequency;               // Varible to store the command

// Position variables
volatile long int position = -1;               // -1 means 'we havent homed'. There's no negative positions in our arrangement here.


void setup() {
  Serial.begin(115200);
  pinMode(X_STEP_BIT, OUTPUT);
  pinMode(X_DIRECTION_BIT, OUTPUT);
  pinMode(X_LIMIT_BIT, INPUT_PULLUP); // inverted logic. connected or 1 = not triggered. disconnected or 0 = triggered

  homeStepper();

  Serial.println("Please enter freq in MHz.");
}

void loop() {
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    incomingFrequency.concat(String(incomingByte));
  }
  if ( incomingFrequency.endsWith("\r") || incomingFrequency.endsWith("\n") ) {
    Serial.print("wavelength: "); Serial.print(1000*(299792458 / (incomingFrequency.toFloat() * 1000000.0))); Serial.println(" mm");
    executeCode(incomingFrequency);
    incomingFrequency.remove(0);
  }
  if ( incomingFrequency.length() > 10 ) {
    Serial.println("Length of command exceeded. Please enter freq in MHz.");
    incomingFrequency.remove(0);
  }

}

void executeCode(String freq) {
  float targetWavelength = frequencyToWavelength(freq.toFloat() * 1000000.0); // returns millimeters of frequency
  float offset =  targetWavelength - (MIN_WAVELENGTH + (position*rotation)/200.0);                        // returns offset of millimeters, positive or negative
  Serial.print("start     : "); Serial.println((MIN_WAVELENGTH + (position*rotation)/200.0));
  Serial.print("travel    : "); Serial.println(offset);
  if (offset < 0){                                                            // if negative...
    digitalWrite(X_DIRECTION_BIT,LOW);                                        // reverse direction
    for (long int i=0; i>millimetersToSteps(offset); --i){ incrementStepper(); position = position - 1; }   // and go backwards that many steps
  }
  if (offset > 0){  
    digitalWrite(X_DIRECTION_BIT,HIGH);
    for (long int i=0; i<millimetersToSteps(offset); ++i){ incrementStepper(); position = position + 1; }
  }
}

void incrementStepper() {
  if ((digitalRead(X_DIRECTION_BIT) == 0) && (digitalRead(X_LIMIT_BIT) == 1)) {
    return; // Failsafe if the direction is 'bad' AND limit switch is triggered
  }
  //if (MIN_WAVELENGTH + (position / stepsPerRotation)*rotation >= MAX_WAVELENGTH) {
  //  return; // Failsafe on max end
  //}
  digitalWrite(X_STEP_BIT, HIGH);
  delayMicroseconds(500);
  digitalWrite(X_STEP_BIT, LOW);
  delayMicroseconds(500);
}

void homeStepper() {
  Serial.println("Homing...");
  digitalWrite(X_DIRECTION_BIT, LOW); // go in reverse direction
  while (digitalRead(X_LIMIT_BIT) == LOW) { // When we dont hit the endstop, move towards endstop
    incrementStepper();
  }
  digitalWrite(X_DIRECTION_BIT, HIGH); // clean up direction flag
  position = 0 ; // We're at home, so set position to 0.
  Serial.println("Done homing...");
}

// receives hertz, returns in millimeters
float frequencyToWavelength(float frequency){
  float wavelength = (299792458.0/frequency)*1000;
  return wavelength;
}

// receives meters, returns in megahertz
float wavelengthToFrequency(float wavelength){
  float frequency = (299792458.0/wavelength)*0.000001;
  return frequency;
}

//// current position in millimeters
//float currentPosition(){
//  float solution =  (MIN_WAVELENGTH + (position*rotation)/200.0);
//  return solution;
//}

// returns steps for x mm distace
long int millimetersToSteps(float millimeters){
  return long(stepsPerRotation*millimeters/rotation);
}
