#include <MessagePack.h>
#include <TelemetryJet.h>

#include <AccelStepper.h>

AccelStepper stepper(AccelStepper::DRIVER, 9, 8);
TelemetryJet telemetry(&Serial, 100);

int state = 0;
long highPos = 0;
long targetPos = 0;
bool clearedEndStop = false;

Dimension targetPosInput = telemetry.createDimension(1);
Dimension currentPosOutput = telemetry.createDimension(2);
Dimension targetPosOutput = telemetry.createDimension(4);
Dimension stateOutput = telemetry.createDimension(3);

void setup() {  
  // Pin 7: Enable the stepper driver
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);

  // Set microstepping
  pinMode(6, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
  digitalWrite(6, HIGH);
  digitalWrite(5, HIGH);
  digitalWrite(4, HIGH);

  // Limit switches
  pinMode(2, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  
  Serial.begin(115200);

  stepper.setSpeed(1000);
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(2000);
}

void loop() {
  // Update the telemetry stream
  telemetry.update();
  
  bool limitLow = !digitalRead(2);
  bool limitHigh = !digitalRead(4);
  
  if (state == 0) {
    // State 0: Find lower bound of range
    // This will always be position "0"
    // Run to left until we hit end stop
    stepper.moveTo(-10000000);
    
    if (limitLow) {
      stepper.stop();
      state = 1;
      stepper.setCurrentPosition(0);
    }
  } else if (state == 1) {
    // State 1: Find upper bound of range
    // Run to right until we hit end stop
    // record position 
    stepper.moveTo(10000000);
    
    if (limitHigh) {
      stepper.stop();
      state = 2;
      highPos = stepper.currentPosition();
      stepper.setCurrentPosition(highPos);
      stepper.run();
      
      // set target pos to middle of range
      stepper.moveTo(highPos / 2);
      targetPos = highPos / 2;
      clearedEndStop = false;
    }
  } else {
    // Handle commands to a new position
    if (targetPosInput.hasNewValue()) {
      if (newPos > 0 && newPos < highPos) {
        stepper.moveTo(targetPos + targetPosInput.getFloat32());
      }
      targetPosInput.clearValue();
    }

    // If we hit an end stop, immediately go into an error state
    if (limitLow || limitHigh) {
      if (clearedEndStop) {
        stepper.stop();
      }
    } else {
      clearedEndStop = true;
    }
  }

  currentPosOutput.setInt32(stepper.currentPosition());
  targetPosOutput.setInt32(stepper.targetPosition());
  stateOutput.setInt32(state);
  stepper.run();
}
