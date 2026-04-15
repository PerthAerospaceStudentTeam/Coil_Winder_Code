// Motor Pins
const int spoolStep = 8;
const int spoolDir = 7;
const int feederStep = 11;
const int feederDir = 10;

// Limit Switch Pins
const int leftLimit = 4;
const int rightLimit = 9;

// Physical Hardware Variables
float wireDiameter = 0.25;    // Diameter in mm
float threadPitch = 1.25;     // M8 rod = 1.25mm per rotation
int stepsPerRev = 200;        // NEMA 17 standard

// Logic Variables
int stepRatio;                
int spoolStepCount = 0;
bool isWinding = false;
bool currentFeederDir = HIGH; 

void setup() {
  Serial.begin(9600);
  
  pinMode(spoolStep, OUTPUT);
  pinMode(spoolDir, OUTPUT);
  pinMode(feederStep, OUTPUT);
  pinMode(feederDir, OUTPUT);
  pinMode(leftLimit, INPUT_PULLUP);
  pinMode(rightLimit, INPUT_PULLUP);

  // Calculate ratio
  float mmPerFeederStep = threadPitch / (float)stepsPerRev;
  float feederStepsPerWire = wireDiameter / mmPerFeederStep;
  stepRatio = (int)((float)stepsPerRev / feederStepsPerWire);

  // Safety floor: stepRatio must be at least 1
  if (stepRatio < 1) stepRatio = 1;

  Serial.println("Calculated Step Ratio: "); Serial.println(stepRatio);
  Serial.println("Commands: f (Start), s (Stop)");
}

void loop() {
  // Serial Inputs
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 'f') { isWinding = true; Serial.println("Winding Started..."); }
    if (cmd == 's') { isWinding = false; Serial.println("Stopped."); }
  }

  // Modified version of the mainloop
  if (isWinding) {
    // Constantly apply the current direction to the feeder motor
    digitalWrite(feederDir, currentFeederDir);

    // Check boundaries and flip state, not motor pulse
    if (digitalRead(leftLimit) == LOW && currentFeederDir == LOW) {
      currentFeederDir = HIGH;
      Serial.println(">> Reversing Right");
    }
    if (digitalRead(rightLimit) == LOW && currentFeederDir == HIGH) {
      currentFeederDir = LOW;
      Serial.println("<< Reversing Left");
    }

    // Spool Movement
    digitalWrite(spoolStep, HIGH);

    // Feeder Syncing
    spoolStepCount++;
    if (spoolStepCount >= stepRatio) {
      digitalWrite(feederStep, HIGH);
      delayMicroseconds(10);
      digitalWrite(feederStep, LOW);
      spoolStepCount = 0;
    }

    digitalWrite(spoolStep, LOW);
    delayMicroseconds(2000); // Main speed control
  }

  /* Old version of the winding sequence: spool would halt after hitting limit switch
  if (isWinding) {
    // Check if we hit a boundary and flip direction
    if (digitalRead(leftLimit) == LOW) {
      currentFeederDir = HIGH;
      digitalWrite(feederDir, currentFeederDir);
      Serial.println(">> Reversing Right");
      delay(300); // Give it time to move away from the switch
    }
    
    if (digitalRead(rightLimit) == LOW) {
      currentFeederDir = LOW;
      digitalWrite(feederDir, currentFeederDir);
      Serial.println("<< Reversing Left");
      delay(300);
    }

    // Spool Movement
    digitalWrite(spoolStep, HIGH);
    
    // Feeder Syncing
    spoolStepCount++;
    if (spoolStepCount >= stepRatio) {
      digitalWrite(feederStep, HIGH);
      delayMicroseconds(10);
      digitalWrite(feederStep, LOW);
      spoolStepCount = 0;
    } else {
      delayMicroseconds(10); 
    }

    digitalWrite(spoolStep, LOW);
    delayMicroseconds(2000); // Main speed control
  }
  */
}