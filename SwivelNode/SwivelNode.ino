#include <MessagePack.h>
#include <TelemetryJet.h>

#include <AccelStepper.h>

AccelStepper stepper(AccelStepper::DRIVER, 9, 8);

// Swivel state machine 
#define SWIVEL_IDLE 0
#define SWIVEL_CALIBRATE_BOUNDS_LOW 1
#define SWIVEL_CALIBRATE_BOUNDS_HIGH 2
#define SWIVEL_MOVING 3
#define SWIVEL_ERROR 4

// Launcher state machine
#define LAUNCHER_IDLE 0
#define LAUNCHER_EXTENDING 1
#define LAUNCHER_RETRACTING 2

int state = 0;
long highPos = 0;
long targetPos = 0;
bool clearedEndStop = false;

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

  // Set max speed and acceleration for the stepper
  stepper.setSpeed(1000);
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(2000);
}

void loop() {
  // Check limit switches
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

  // Update the stepper driver
  stepper.run();
}
