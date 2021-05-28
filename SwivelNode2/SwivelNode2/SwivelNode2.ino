#define USE_USBCON
#include <ros.h>
#include <std_msgs/Empty.h>
#include <std_msgs/Int8.h>
#include <std_msgs/Bool.h>
#include <Servo.h>
#include <AccelStepper.h>

// ---------------------------------------------------
// CONSTANTS

// Yaw Swivel states
#define SWIVEL_IDLE 0                   // The swivel is idle.
#define SWIVEL_CALIBRATE_BOUNDS_LOW 1   // Swivel is calibrating a lower bound.
#define SWIVEL_CALIBRATE_BOUNDS_HIGH 2  // Swivel is calibrating an upper bound. 
#define SWIVEL_MOVING 3                 // Swivel moving towards a position.
#define SWIVEL_ERROR 4                  // Swivel encountered an error/abort state.

// Yaw Swivel constants
#define SWIVEL_STEP_ENABLE 7
#define SWIVEL_STEP_PIN 9
#define SWIVEL_DIRECTION_PIN 8
#define SWIVEL_MICROSTEP_PIN_1 4
#define SWIVEL_MICROSTEP_PIN_2 5
#define SWIVEL_MICROSTEP_PIN_3 6
#define SWIVEL_LIMIT_LOW_PIN 2
#define SWIVEL_LIMIT_HIGH_PIN 4

// Launcher states
#define LAUNCHER_IDLE 0               // The launcher is idle.
#define LAUNCHER_EXTENDING 1          // Launcher is extending, to release a ball.
#define LAUNCHER_RETRACTING 2         // Launcher is retracting/resetting.

// Launcher Constants
#define LAUNCHER_EXTENDED_POS 10      // "Extended" servo position
#define LAUNCHER_RETRACTED_POS 155    // "Retracted" servo position
#define LAUNCHER_SERVO_PIN 10         // What pin the launcher servo is on
#define LAUNCHER_EXTEND_DURATION 500  // Time it takes the launcher servo to extend
#define LAUNCHER_RETRACT_DURATION 500 // Time it takes the launcher servo to retract

// ---------------------------------------------------
// GLOBAL VARIABLES

// Launcher variables & state 
int iLauncherState = LAUNCHER_IDLE;
unsigned long lLauncherLastAction = 0;
bool bWantsLaunch = false;
Servo launcherServo; 

// Swivel variables & state
int iSwivelState = SWIVEL_CALIBRATE_BOUNDS_LOW;
long lSwivelHighPos = 0;
long lSwivelTargetPos = 0;
bool bSwivelClearedEndStop = false;
AccelStepper stepper(AccelStepper::DRIVER, SWIVEL_STEP_PIN, SWIVEL_DIRECTION_PIN);

unsigned long lLastHeartbeatMsg = 0;

// ---------------------------------------------------
// ROS SETUP

ros::NodeHandle nh;

void triggerCommandCallback(const std_msgs::Empty& launch_msg) {
  bWantsLaunch = true;
}

void yawCommandCallback(const std_msgs::Int8& yaw_msg) {
  // Yaw message will be a value from -90 to 90 inclusive
  // Convert this to a target position.
  lSwivelTargetPos = constrain(map(yaw_msg.data, -90, 90, 0, lSwivelHighPos),0, lSwivelHighPos);
}

ros::Subscriber<std_msgs::Int8> yawSubscriber("yaw_cmd", &yawCommandCallback);
ros::Subscriber<std_msgs::Empty> triggerSubscriber("trigger", &triggerCommandCallback);

std_msgs::Bool ready_msg;
ros::Publisher readyPublisher("yaw_ready", &ready_msg);

//std_msgs::Int8 swivel_state_msg;
//std_msgs::Int8 launcher_state_msg;
//ros::Publisher swivelStatePublisher("swivel_state", &swivel_state_msg);
//ros::Publisher launcherStatePublisher("launcher_state", &launcher_state_msg);

// ---------------------------------------------------
// ROS SETUP

void sendStatusMsg() {
  //swivel_state_msg.data = iSwivelState;
  //launcher_state_msg.data = iLauncherState;
  //swivelStatePublisher.publish(&swivel_state_msg);
  //launcherStatePublisher.publish(&launcher_state_msg);
  //nh.spinOnce();
}

void sendReadyMsg(bool isReady) {
  ready_msg.data = isReady;
  readyPublisher.publish(&ready_msg);
  nh.spinOnce();
}

void setup() {
  // Initialize launcher servo
  launcherServo.attach(LAUNCHER_SERVO_PIN);
  launcherServo.write(LAUNCHER_RETRACTED_POS);

  // Initialize stepper motor
  pinMode(SWIVEL_STEP_ENABLE, OUTPUT);
  digitalWrite(SWIVEL_STEP_ENABLE, LOW);
  
  // Set to use full microstepping
  pinMode(SWIVEL_MICROSTEP_PIN_1, OUTPUT);
  pinMode(SWIVEL_MICROSTEP_PIN_2, OUTPUT);
  pinMode(SWIVEL_MICROSTEP_PIN_3, OUTPUT);
  digitalWrite(SWIVEL_MICROSTEP_PIN_1, HIGH);
  digitalWrite(SWIVEL_MICROSTEP_PIN_2, HIGH);
  digitalWrite(SWIVEL_MICROSTEP_PIN_3, HIGH);
  
  // Stepper limit switches
  pinMode(SWIVEL_LIMIT_LOW_PIN, INPUT_PULLUP);
  pinMode(SWIVEL_LIMIT_HIGH_PIN, INPUT_PULLUP);
  
  // Set max speed and acceleration for the stepper
  stepper.setSpeed(1000);
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(2000);

  // Start calibration mode
  iSwivelState = SWIVEL_CALIBRATE_BOUNDS_LOW;
  
  // Init ROS node and setup topics
  nh.initNode();
  //nh.advertise(swivelStatePublisher);
  //nh.advertise(launcherStatePublisher);
  nh.subscribe(yawSubscriber);
  nh.subscribe(triggerSubscriber);
  nh.advertise(readyPublisher);

}

// ---------------------------------------------------
// MAIN LOOP

void loop() {
  if (!nh.connected()) {
    digitalWrite(SWIVEL_STEP_ENABLE, LOW);
    nh.spinOnce();

    if (nh.connected()) {
      digitalWrite(SWIVEL_STEP_ENABLE, HIGH);
      iSwivelState = SWIVEL_CALIBRATE_BOUNDS_LOW;
    }
    return;
  }

  // Check limit switches
  bool limitLow = !digitalRead(2);
  bool limitHigh = !digitalRead(4);

  
  switch (iSwivelState) {
    case SWIVEL_IDLE: {
      nh.spinOnce();
      stepper.moveTo(lSwivelTargetPos);
      stepper.run();
      if (stepper.isRunning()) {
        iSwivelState = SWIVEL_MOVING;
      }
      break;
    }
    case SWIVEL_CALIBRATE_BOUNDS_LOW: {
      stepper.moveTo(-10000000);
      stepper.run();
      if (limitLow) {
        stepper.stop();
        iSwivelState = SWIVEL_CALIBRATE_BOUNDS_HIGH;
        stepper.setCurrentPosition(0);
      }
      break;
    }
    case SWIVEL_CALIBRATE_BOUNDS_HIGH: {
      stepper.moveTo(10000000);
      stepper.run();
      if (limitHigh) {
        stepper.stop();
        iSwivelState = SWIVEL_IDLE;
        lSwivelHighPos = stepper.currentPosition();
        stepper.setCurrentPosition(lSwivelHighPos);
        stepper.run();
        
        // set target pos to middle of range
        lSwivelTargetPos = lSwivelHighPos / 2;
        bSwivelClearedEndStop = false;
      }
      break;
    }
    case SWIVEL_MOVING: {
      stepper.run();
      if (!stepper.isRunning()) {
        iSwivelState = SWIVEL_IDLE;
      }
      
      if (limitLow || limitHigh) {
        if (bSwivelClearedEndStop) {
          stepper.stop();
          iSwivelState = SWIVEL_ERROR;
        }
      } else {
        bSwivelClearedEndStop = true;
      }
      break;
    }
    case SWIVEL_ERROR: {
      // Disable the stepper. Something went wrong.
      stepper.stop();
      digitalWrite(SWIVEL_STEP_ENABLE, LOW);
      break;
    }
  }
  
  switch (iLauncherState) {
    case LAUNCHER_IDLE: {
      launcherServo.write(LAUNCHER_RETRACTED_POS);
      if (bWantsLaunch) {
        launcherServo.write(LAUNCHER_EXTENDED_POS);
        iLauncherState = LAUNCHER_EXTENDING;
        lLauncherLastAction = millis();
      }
      break;
    }
    case LAUNCHER_EXTENDING: {
      // Wait while the launcher is extending, then start retraction.
      if (millis() - lLauncherLastAction > LAUNCHER_EXTEND_DURATION) {
        launcherServo.write(LAUNCHER_RETRACTED_POS);
        iLauncherState = LAUNCHER_RETRACTING;
        lLauncherLastAction = millis();
      }
      break;
    }
    case LAUNCHER_RETRACTING: {
      // Wait while the launcher is retracting, then return to idle.
      if (millis() - lLauncherLastAction > LAUNCHER_RETRACT_DURATION) {
        iLauncherState = LAUNCHER_IDLE;
        bWantsLaunch = false;
      }
      break;
    }
  }
}
