#pragma once

// =============================================================================
//  config.h — Tune everything from here
// =============================================================================

// --- Hardware Pins ---
#define BUZZER_PIN          37
#define HS45HB0_PIN         3      // arm servo A
#define HS45HB1_PIN         4      // arm servo B
#define PIN_74HCT126_EN     2      // Direction pin for KRS half-duplex serial

// --- Serial Baud Rates ---
#define XIAO_BAUD           4000000UL
#define KRS_BAUD            115200UL
#define KRS_TIMEOUT         400      // ms
#define KRS_ID              1

// --- Motor Speed Limits ---
#define MAX_MOTOR_SPEED     100      // Absolute cap applied to all wheel commands

// --- Line-follow PID (Teensy-side) ---
// Error is derived from xiaoLineError (0–254, centre = 127), scaled to ±200
#define PID_KP              2.5f     // Proportional gain
#define PID_KI              0.0f     // Integral gain
#define PID_KD              3.0f     // Derivative gain
#define PID_BASE_SPEED      100.0f   // Straight-ahead wheel speed
#define PID_LEFT_SCALE      1.7f     // Extra gain on left motor (compensates camera lighting bias)
#define PID_INTEGRAL_LIMIT  500.0f   // Anti-windup clamp on the I-term accumulator

// --- IMU Pitch Compensation ---
// Speed added per degree of forward pitch.  Set to 0 to disable.
#define IMU_SAMPLE_RATE     200.0f   // Hz for Madgwick filter
#define IMU_PITCH_GAIN      0.0f

// --- Turn Motor Speeds ---
#define TURN_UTURN_L       -10.0f
#define TURN_UTURN_R        10.0f
#define TURN_LEFT_L        -70.0f
#define TURN_LEFT_R        100.0f
#define TURN_RIGHT_L       100.0f
#define TURN_RIGHT_R       -70.0f

// --- Turn Durations (ms) ---
#define TURN_UTURN_MS       1000
#define TURN_LEFT_MS        600
#define TURN_RIGHT_MS       600

// --- CommandFilter ---
#define FILTER_QUEUE_SIZE   15       // Rolling window length
#define FILTER_THRESHOLD    9        // Votes needed to confirm a command

// --- Evacuation Zone ---
#define EVAC_BEEP_ON_MS     20       // Buzzer on-time per beep
#define EVAC_BEEP_PERIOD_MS 500      // Time between beep starts
