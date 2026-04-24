#pragma once

#include "config.h"
#include "CommandFilter.h"
#include "YacheEncodedSerial.h"
#include "yacheSTS.h"
#include "IcsHardSerialClass.h"
#include <Servo.h>
#include <arm_math.h>

// =============================================================================
//  globals.h — Every cross-file global in one place
//
//  Include this in EVERY .ino file.  Variables are DEFINED in their logical
//  .ino file (noted in comments); this header only declares them as extern so
//  IntelliSense and the compiler can see them everywhere.
// =============================================================================

// --- State machine (STS-imuMerge.ino) ---
enum RobotState {
    FOLLOWING_LINE,  // Normal PID line follow
    EXECUTING_TURN,  // Mid-turn, ignoring new commands
    EVACUATION_ZONE, // Silver tape — stopped, periodic beep
    STALLED_RED      // Red tape — stopped until XIAO clears
};
extern RobotState robotState;
extern bool       isBusyTurning; // True while a turn is running; blocks new commands

// --- XIAO comms (Comms.ino) ---
extern YacheEncodedSerial xiao;
extern uint8_t            xiaoCommand;   // 0=idle 1=U-turn 2=left 3=right 4=red 5=silver
extern float              xiaoLineError; // Raw line position 0-254 (127 = centre)
extern CommandFilter      cmdFilter;

// --- IMU (Sensors.ino) ---
extern volatile float32_t pitch; // Positive = nose-up
extern volatile float32_t roll;
extern volatile float32_t yaw;

// --- Motors (Drive.ino) ---
extern yacheSTS            _sts;
extern IntervalTimer        controlTimer;
extern volatile float32_t  frontLeftGain;
extern volatile float32_t  frontRightGain;
extern volatile float32_t  backLeftGain;
extern volatile float32_t  backRightGain;

// --- Arm / Servos (Actions.ino) ---
extern Servo             _HS45HB0;
extern Servo             _HS45HB1;
extern IcsHardSerialClass krs;
