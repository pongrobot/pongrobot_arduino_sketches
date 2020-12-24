#include <A4988.h>
#include <BasicStepperDriver.h>
#include <DRV8825.h>
#include <DRV8834.h>
#include <DRV8880.h>
#include <MultiDriver.h>
#include <SyncDriver.h>

DRV8825 stepper(200, 8, 9, 3, 5, 6);

void setup() {
  stepper.begin(50, 32);
  stepper.enable();
  stepper.setSpeedProfile(BasicStepperDriver::LINEAR_SPEED, 200, 200);
  stepper.startMove(200 * 32);
}

void loop() {
  stepper.nextAction();
  if (stepper.getCurrentState() == 0) {
    delay(5000);
    stepper.startMove(200 * 32);
  }
}
