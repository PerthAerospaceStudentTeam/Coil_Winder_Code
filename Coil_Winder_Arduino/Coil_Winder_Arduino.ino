// Motor Pins
const int spoolStep = 3;
const int spoolDir = 2;
const int feederStep = 11;
const int feederDir = 10;

// Limit Switch Pins
const int leftLimit = 5;
const int rightLimit = 4;

const int sleepPin = 7;

// Physical Hardware Variables
float wireDiameter = 0.20;    // Diameter in mm
float threadPitch = 1.25;     // M8 rod = 1.25mm per rotation
int stepsPerRev = 200;        // NEMA 17 standard

// Logic Variables
int stepRatio;                
int spoolStepCount = 0;
bool isWinding = false;
bool goingHome = false;
bool currentFeederDir = HIGH;

// Variables for tracking number of windings
long targetTotalSteps = 0;
long totalSpoolStepsTaken = 0;

// Accumulator variables for more precise steps
float feederStepsPerSpoolStep;
float feederStepAccumulator = 0.0;

void setup() {
  Serial.begin(9600);
  
  pinMode(spoolStep, OUTPUT);
  pinMode(spoolDir, OUTPUT);
  pinMode(feederStep, OUTPUT);
  pinMode(feederDir, OUTPUT);
  pinMode(leftLimit, INPUT_PULLUP);
  pinMode(rightLimit, INPUT_PULLUP);

  // Calculate decimal ratio
  float mmPerFeederStep = threadPitch / (float)stepsPerRev;
  float feederStepsPerWire = wireDiameter / mmPerFeederStep;
  feederStepsPerSpoolStep = feederStepsPerWire / (float)stepsPerRev;

  // stepRatio must be at least 1
  if (stepRatio < 1) stepRatio = 1;

  Serial.println("Calculated Feeder Step Ratio: ");
  Serial.println(feederStepsPerSpoolStep, 4);
  Serial.println("Enter the number of windings required to start.");
  Serial.println("Press 's' for manual/emergency stop");
  Serial.println("Press 'i' to home the feeder (to right limit switch)");
}

void loop() {
  // Serial Inputs
  if (Serial.available() > 0) {
    String inputStr = Serial.readStringUntil('\n');
    inputStr.trim();

    if (inputStr == "s") {
      isWinding = false;
      Serial.println("Winding stopped.");
    }

    if (inputStr == "i") { // Initialise winder by going all the way to right limit
      isWinding = false;
      goingHome = true;
      Serial.println("Going home");

      currentFeederDir = HIGH;
      digitalWrite(feederDir, currentFeederDir);

      while (goingHome == true) {
        // Pulse feeder motor
        digitalWrite(feederStep, HIGH);
        delayMicroseconds(5);
        digitalWrite(feederStep, LOW);
        // Allow time to step
        delayMicroseconds(2000);

        // Check if reached home pos (right limit switch)
        if (digitalRead(rightLimit) == LOW && currentFeederDir == HIGH) {
          currentFeederDir = LOW;
          goingHome = false;
          Serial.println("Home");
      }
    }
    }

    else if (!isWinding) {
      long requestedTurns = inputStr.toInt();

      if (requestedTurns > 0) {
        targetTotalSteps = requestedTurns * stepsPerRev; // Calculate number of spool steps required
        totalSpoolStepsTaken = 0; // Reset counter for new run
        feederStepAccumulator = 0.0; // Reset accumulator for new run
        isWinding = true;

        Serial.print("Winding started for ");
        Serial.print(requestedTurns);
        Serial.println(" turns...");
      }
    }
  }

  // Main loop
  if (isWinding) {
    // Constantly apply the current direction to the feeder motor
    digitalWrite(feederDir, currentFeederDir);

    // Check limit switches
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
    delayMicroseconds(5);

    // Feeder syncing
    totalSpoolStepsTaken++;
    feederStepAccumulator += feederStepsPerSpoolStep;

    if (feederStepAccumulator >= 1.0) {
      digitalWrite(feederStep, HIGH);
      delayMicroseconds(5);
      digitalWrite(feederStep, LOW);
      feederStepAccumulator -= 1.0; // Subtract 1 so we keep remainder
    }

    digitalWrite(spoolStep, LOW);
    delayMicroseconds(2000); // Main speed control

    // Check if winding complete
    if (totalSpoolStepsTaken >= targetTotalSteps) {
      isWinding = false;
      Serial.println("WINDING COMPLETED");
      Serial.println("Enter a new number of turns to continue or start new coil...");
    }
  }
}