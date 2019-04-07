//
// Arduino code for a wheel and trigger style controller
//
// Copyright 2019 Jeff Argast
//
// Permission to use, copy, modify, and/or distribute this software 
// for any purpose with or without fee is hereby granted, provided 
// that the above copyright notice and this permission notice 
// appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL 
// WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE 
// AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR 
// CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
// NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
// CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//

// 
// Requires the joystick library
// https://github.com/MHeironimus/ArduinoJoystickLibrary
//
#include <Joystick.h>

//
// Defines
//

#define LOGGING 0

#if LOGGING
  #define LOG(X) (X)
#else
  #define LOG(X)
#endif

// 
// Set to 0 if just debugging log messages
//
#define ENABLE_JOYSTICK 1

//
// Overall defines
//

const byte WHEEL_APIN = 0;
const byte THROTTLE_APIN = 1;
const byte WHEEL_TRIM_APIN = 2;
const byte THROTTLE_TRIM_APIN = 3;

const byte BUTTON_0_PIN = 2;
const byte BUTTON_1_PIN = 3;
const byte BUTTON_2_PIN = 4;
const byte BUTTON_3_PIN = 5;

const byte NUM_BUTTONS = 4;

const byte BUTTON_TO_PIN[NUM_BUTTONS] = { 
  BUTTON_0_PIN, 
  BUTTON_1_PIN, 
  BUTTON_2_PIN, 
  BUTTON_3_PIN
};

const int WHEEL_RANGE = 1023;
const int THROTTLE_RANGE = 1023;
const int BRAKE_RANGE = 1023;

Joystick_ gJoystick(
  JOYSTICK_DEFAULT_REPORT_ID, 
  JOYSTICK_TYPE_MULTI_AXIS, 
  NUM_BUTTONS,
  0,      // Num hat switches
  false,  // No X Axis
  false,  // No Y Axis
  false,  // No Z Axis
  false,  // No X Rotation
  false,  // No Y Rotation
  false,  // No Z Rotation
  false,  // No Rudder
  false,  // No Throttle
  true,   // Yes Accelerator
  true,   // Yes Brake 
  true    // Yes Steering Wheel
 );
    


//
// Controller state
//

long gSample = 0;

struct {
  int wheelVal;
  int throttleVal;
  bool button[NUM_BUTTONS];
} gState;

//
// Setup
//

void setup() {
  #if LOGGING
    Serial.begin(9600);
  #endif

  gState.wheelVal = readWheel();
  gState.throttleVal = readThrottle();

  for (int n = 0; n < NUM_BUTTONS; n++ ) {
    gState.button[n] = false;
    pinMode(BUTTON_TO_PIN[n], INPUT_PULLUP);
  }

  gJoystick.setAcceleratorRange(0, THROTTLE_RANGE);
  gJoystick.setSteeringRange(0, WHEEL_RANGE);
  gJoystick.setBrakeRange(0, BRAKE_RANGE);
  
  #if ENABLE_JOYSTICK
    gJoystick.begin(false);
  #endif
}

//
// Logging
//

#if LOGGING

void logWheel() {
  Serial.print("gSample = ");
  Serial.print(gSample);
  Serial.print(", gState.wheelVal = ");
  Serial.println(gState.wheelVal);
}

void logThrottle() {
  Serial.print("gSample = ");
  Serial.print(gSample);
  Serial.print(", gState.throttleVal = ");
  Serial.println(gState.throttleVal);
}

void logButtonPressed(int aButton) {
  Serial.print("Pressing button ");
  Serial.println(aButton);
}

void logButtonReleased(int aButton) {
  Serial.print("Releasing button ");
  Serial.println(aButton);
}

void logWheelTrim(int trimVal) {
  Serial.print("gSample = ");
  Serial.print(gSample);
  Serial.print(", wheelTrimVal = ");
  Serial.println(trimVal);
}

void logThrottleTrim(int trimVal) {
  Serial.print("gSample = ");
  Serial.print(gSample);
  Serial.print(", throttleTrimVal = ");
  Serial.println(trimVal);
}

#endif

//
// utils
//

void updateButton(int aButton) {
  if ((aButton < 0) || (aButton >= NUM_BUTTONS))
    return;
    
  bool isPressed = !digitalRead(BUTTON_TO_PIN[aButton]);
  
  if (gState.button[aButton] != isPressed) {
    if (isPressed) {
      LOG(logButtonPressed(aButton));
      gJoystick.pressButton(aButton);
    } else {
      LOG(logButtonReleased(aButton));
      gJoystick.releaseButton(aButton);
    }

    gState.button[aButton] = isPressed;
  }
}

void updateButtons() {  
  for (int n = 0; n < NUM_BUTTONS; n++ ) {
    updateButton(n);
  }
}

int readWheelTrim() {
  int trimVal = ((analogRead(WHEEL_TRIM_APIN) - 512) / 512.0) * 100;

  LOG(logWheelTrim(trimVal));
  
  return trimVal;
}

int readWheel() {
  const int WHEEL_SPAN = 1000;
  const int WHEEL_MID = WHEEL_RANGE / 2;
  const int MIN_WHEEL = WHEEL_MID - WHEEL_SPAN / 2;
  const int MAX_WHEEL = WHEEL_MID + WHEEL_SPAN / 2;

  int wheelVal = analogRead(WHEEL_APIN);
  if (wheelVal < MIN_WHEEL)
    wheelVal = MIN_WHEEL;

  if (wheelVal > MAX_WHEEL)
    wheelVal = MAX_WHEEL;

  wheelVal = (((float) wheelVal - MIN_WHEEL) / (float) WHEEL_SPAN) * WHEEL_RANGE;
  wheelVal -= readWheelTrim();
  
  return wheelVal;
}

void updateWheel() {
  int newWheelVal = readWheel();

  if (gState.wheelVal != newWheelVal) {
    gState.wheelVal = newWheelVal;
    gJoystick.setSteering(newWheelVal);
    LOG(logWheel());
  }
}

int readThrottleTrim() {
  int trimVal = ((analogRead(THROTTLE_TRIM_APIN) - 512) / 512.0) * 100;

  LOG(logThrottleTrim(trimVal));
  
  return trimVal;
}

int readThrottle() {
  // These constants were found through experimentation of the specific 
  // device I used. These may need to be tweaked for a different trigger
  const int MAX_THROTTLE = 750;
  const int MIN_THROTTLE = 138;
  const int MID_THROTTLE = 430;

  int throttleVal = analogRead(THROTTLE_APIN);

  if (throttleVal > MAX_THROTTLE)
    throttleVal = MAX_THROTTLE;

  if (throttleVal < MIN_THROTTLE)
    throttleVal = MIN_THROTTLE;

  if (throttleVal < MID_THROTTLE) {
    int throttleRange = MID_THROTTLE - MIN_THROTTLE;
    
    throttleVal = (((float) throttleVal - MID_THROTTLE) / (float) throttleRange) * BRAKE_RANGE;
  } else {
    int throttleRange = MAX_THROTTLE - MID_THROTTLE;
    
    throttleVal = (((float) throttleVal - MID_THROTTLE) / (float) throttleRange) * THROTTLE_RANGE;
  }

  throttleVal -= readThrottleTrim();
  
  return throttleVal;
}

void updateThrottle() {
  int newThrottleVal = readThrottle();
  
  if (gState.throttleVal != newThrottleVal) {
    gState.throttleVal = newThrottleVal;
    
    if (newThrottleVal < 0 ) {
      gJoystick.setBrake(-newThrottleVal);
      gJoystick.setAccelerator(0);
    } else {
      gJoystick.setBrake(0);
      gJoystick.setAccelerator(newThrottleVal);
    }
    
    LOG(logThrottle());
  }
}

//
// loop
//

void loop() {
  updateButtons();
  updateWheel();
  updateThrottle();
  
  gSample++;

  #if ENABLE_JOYSTICK
    gJoystick.sendState();
  #endif
  
  #if LOGGING
    delay(10);
  #endif
}
