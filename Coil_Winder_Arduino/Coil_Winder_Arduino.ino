#include <AccelStepper.h>

// Motor Pins
const int spoolStep = 3;
const int spoolDir = 2;
const int feederStep = 11;
const int feederDir = 10;

// Limit Switch Pins
const int leftLimit = 5;
const int rightLimit = 4;

const int sleepPin = 7;

// AccelStepper instances
AccelStepper spoolMotor(1, spoolStep, spoolDir);
AccelStepper feederMotor(1, feederStep, feederDir);

// Physical Hardware Variables
float wireDiameter = 0.20;    // Diameter in mm
float threadPitch = 1.25;     // M8 rod = 1.25mm per rotation
int stepsPerRev = 200;        // NEMA 17 standard

// Logic Variables
int stepRatio;                
int spoolStepCount = 0;
bool isWinding = false;
bool goingHome = false;
int currentFeederDir = 1;

// Variables for tracking number of windings
long targetTotalSteps = 0;

// Accumulator variables for more precise steps
long lastSpoolPosition = 0;
float feederTargetAccumulator = 0.0;
float feederStepsPerSpoolStep;

void setup() {
  Serial.begin(9600);

  pinMode(leftLimit, INPUT_PULLUP);
  pinMode(rightLimit, INPUT_PULLUP);

  // Configure AccelStepper max speed/acceleration
  spoolMotor.setMaxSpeed(1000);
  spoolMotor.setAcceleration(500);

  feederMotor.setMaxSpeed(1000);
  feederMotor.setAcceleration(1000);

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
      spoolMotor.stop();
      feederMotor.stop();
      Serial.println("Winding stopped.");
    }

    if (inputStr == "i") { // Initialise winder by going all the way to right limit
      isWinding = false;
      goingHome = true;
      Serial.println("Going home...");

      feederMotor.setSpeed(400);
    }

    else if (!isWinding && !goingHome) {
      long requestedTurns = inputStr.toInt();

      if (requestedTurns > 0) {
        targetTotalSteps = requestedTurns * stepsPerRev; // Calculate number of spool steps required
        
        // Reset absolute positions to 0 before starting a new run
        spoolMotor.setCurrentPosition(0);
        feederMotor.setCurrentPosition(0);

        lastSpoolPosition = 0;
        feederTargetAccumulator = 0.0;
        
        spoolMotor.moveTo(targetTotalSteps);

        currentFeederDir = 1;
        isWinding = true;

        Serial.print("Winding started for ");
        Serial.print(requestedTurns);
        Serial.println(" turns...");
      }
    }
  }


  // Homing Routine
  if(goingHome) {
    if(digitalRead(rightLimit) == LOW) {
      feederMotor.stop();
      feederMotor.setCurrentPosition(0);
      goingHome = false;
      Serial.println("Home.");
    } else {
      feederMotor.runSpeed();
    }
  }
  // Main loop
  if (isWinding) {

    // Check limit switches
    if (digitalRead(leftLimit) == LOW && currentFeederDir == -1) {
      currentFeederDir = 1;
      Serial.println(">> Reversing Right");
    }
    if (digitalRead(rightLimit) == LOW && currentFeederDir == 1) {
      currentFeederDir = -1;
      Serial.println("<< Reversing Left");
    }

    if (spoolMotor.distanceToGo() != 0) {
      spoolMotor.run();

      long currentSpoolPos = spoolMotor.currentPosition();
      long spoolDelta = currentSpoolPos - lastSpoolPosition;

      if (spoolDelta != 0) {
        feederTargetAccumulator += (spoolDelta * feederStepsPerSpoolStep * currentFeederDir);
        lastSpoolPosition = currentSpoolPos;
        feederMotor.moveTo((long)feederTargetAccumulator);
      }

      float dynamicFeederSpeed = spoolMotor.speed() * feederStepsPerSpoolStep * currentFeederDir;
      feederMotor.setSpeed(dynamicFeederSpeed);
      feederMotor.runSpeedToPosition();
    }
    // Check if winding complete
    else {
      isWinding = false;
      Serial.println("WINDING COMPLETED");
      Serial.println("Enter a new number of turns to continue or start new coil...");
    }
  }
}