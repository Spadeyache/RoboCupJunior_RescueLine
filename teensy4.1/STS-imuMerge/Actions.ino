#include "globals.h"

// =============================================================================
//  Actions.ino — High-level non-blocking manoeuvres + arm control
//  Exposes: initActions()
//           executeTurn(angle, blocking)  — 90° left/right intersection turn
//           doUTurn()                    — 180° spin
//           enterEvacuationZone()        — start evacuation-zone sub-loop
//           handleEvacuationZone()       — called each loop tick in EVACUATION_ZONE
//           handleTurnTick()             — called each loop tick in EXECUTING_TURN
//           grabARM(closed)
//           liftARM(lift)               — blocking; only call from setup() or evac sequence
// =============================================================================

// --- Hobby Servos (grab arm) ---
Servo _HS45HB0;
Servo _HS45HB1;

// --- KRS Smart Servo (lift arm) ---
IcsHardSerialClass krs(&Serial1, PIN_74HCT126_EN, KRS_BAUD, KRS_TIMEOUT);

// ---------------------------------------------------------------------------
//  Turn state — shared with the EXECUTING_TURN state handler in Main
// ---------------------------------------------------------------------------
static unsigned long _turnStart    = 0;
static unsigned long _turnDuration = 0;

// ---------------------------------------------------------------------------
//  Evacuation-zone beeper state
// ---------------------------------------------------------------------------
static unsigned long _lastEvacBeep  = 0;
static unsigned long _beepOnStart   = 0;
static bool          _beepActive    = false;

// ---------------------------------------------------------------------------
//  Init
// ---------------------------------------------------------------------------
void initActions() {
    pinMode(BUZZER_PIN, OUTPUT);
    analogWrite(BUZZER_PIN, 0);

    _HS45HB0.attach(HS45HB0_PIN, 1000, 2000);
    _HS45HB1.attach(HS45HB1_PIN, 1000, 2000);

    grabARM(true); // Close gripper on boot

    _HS45HB0.detach();
    _HS45HB1.detach();
}

// ---------------------------------------------------------------------------
//  executeTurn — non-blocking intersection turn
//
//  angle > 0 → right turn   angle < 0 → left turn
//  blocking: if true the call busy-waits (only use from non-loop contexts).
//  Sets robotState = EXECUTING_TURN and isBusyTurning = true.
// ---------------------------------------------------------------------------
void executeTurn(float angle, bool blocking = false) {
    if (angle > 0) {
        motor(TURN_RIGHT_L, TURN_RIGHT_R);
        Serial.println("Action: Right turn");
    } else {
        motor(TURN_LEFT_L, TURN_LEFT_R);
        Serial.println("Action: Left turn");
    }
    _turnDuration = (unsigned long)(fabsf(angle) * TURN_MS_PER_DEG);

    _turnStart      = millis();
    isBusyTurning   = true;
    robotState      = EXECUTING_TURN;

    if (blocking) {
        while (millis() - _turnStart < _turnDuration) { /* spin-wait */ }
        handleTurnTick(); // Finalise immediately
    }
}

// ---------------------------------------------------------------------------
//  doUTurn — non-blocking 180° spin
// ---------------------------------------------------------------------------
void doUTurn() {
    Serial.println("Action: U-Turn");
    motor(TURN_UTURN_L, TURN_UTURN_R);
    _turnStart      = millis();
    _turnDuration   = TURN_UTURN_MS;
    isBusyTurning   = true;
    robotState      = EXECUTING_TURN;
}

// ---------------------------------------------------------------------------
//  execForward — drive straight for a set distance
//
//  speed        : motor power 0–MAX_MOTOR_SPEED
//  distance_mm  : how far to travel; converted to time via FORWARD_MS_PER_MM
//                 (calibrated at MAX_MOTOR_SPEED, scaled linearly by speed)
//  usePID       : false → open-loop blocking drive (default)
//                 true  → yaw-hold loop using IMU; corrects drift each tick
//                         (requires IMU enabled in Sensors.ino)
// ---------------------------------------------------------------------------
void execForward(float speed, float distance_mm, bool usePID = false) {
    // Scale duration by speed ratio so the same mm always gives the same distance
    unsigned long duration = (unsigned long)(distance_mm * FORWARD_MS_PER_MM
                                            * MAX_MOTOR_SPEED / speed);

    Serial.printf("Forward: %.0f mm @ speed %.0f -> %lu ms\n",
                  distance_mm, speed, duration);

    if (!usePID) {
        // Open-loop: drive straight, wait, stop
        motor(speed, speed);
        delay(duration);
        motor(0, 0);

    } else {
        // Yaw-hold PID: sample IMU every tick, correct left/right to stay straight
        float         startYaw = yaw;
        unsigned long start    = millis();

        while (millis() - start < duration) {
            updateSensors(); // refresh yaw (no-op until IMU is enabled)

            float yawError   = yaw - startYaw;           // +ve = drifted right
            float correction = FORWARD_YAW_KP * yawError;

            // Reduce speed on the side we drifted towards, boost the other
            motor(speed - correction, speed + correction);
        }
        motor(0, 0);
    }
}

// ---------------------------------------------------------------------------
//  handleTurnTick — call every loop tick while in EXECUTING_TURN state.
//  Transitions back to FOLLOWING_LINE once the timed duration elapses.
// ---------------------------------------------------------------------------
void handleTurnTick() {
    if (millis() - _turnStart >= _turnDuration) {
        motor(0, 0);
        analogWrite(BUZZER_PIN, 0); // turn off buzzer that was set when the turn started
        isBusyTurning = false;
        cmdFilter.clear();
        robotState = FOLLOWING_LINE;
        Serial.println("Turn complete → FOLLOWING_LINE");
    }
}

// ---------------------------------------------------------------------------
//  enterEvacuationZone — trigger transition into EVACUATION_ZONE state
// ---------------------------------------------------------------------------
void enterEvacuationZone() {
    Serial.println("Action: Entering evacuation zone");
    motor(0, 0);
    robotState    = EVACUATION_ZONE;
}

// ---------------------------------------------------------------------------
//  handleEvacuationZone — call every loop tick while in EVACUATION_ZONE.
//  Beeps periodically and exits back to FOLLOWING_LINE once XIAO stops
//  reporting silver.
// ---------------------------------------------------------------------------
void handleEvacuationZone() {
    unsigned long now = millis();

    motor(0,0);
    analogWrite(BUZZER_PIN, 30); delay(50); analogWrite(BUZZER_PIN, 160); delay(40);analogWrite(BUZZER_PIN, 0);
    delay(1000);

    // position adjustment
    // state map init

    // go to center of evac

    // // Start a new beep pulse when the period has elapsed
    // if (!_beepActive && (now - _lastEvacBeep >= EVAC_BEEP_PERIOD_MS)) {
    //     analogWrite(BUZZER_PIN, 160);
    //     _beepOnStart  = now;
    //     _beepActive   = true;
    //     _lastEvacBeep = now;
    //     Serial.println("Evac: beep");
    // }

    // // End the beep pulse after EVAC_BEEP_ON_MS
    // if (_beepActive && (now - _beepOnStart >= EVAC_BEEP_ON_MS)) {
    //     analogWrite(BUZZER_PIN, 0);
    //     _beepActive = false;
    // }

    // // Exit when XIAO no longer sees silver
    // if (xiaoCommand != 5) {
    //     cmdFilter.clear();
    //     robotState = FOLLOWING_LINE;
    //     Serial.println("Evac: silver cleared → FOLLOWING_LINE");
    // }
}


// ---------------------------------------------------------------------------
//  Arm helpers
// ---------------------------------------------------------------------------
void grabARM(bool closed) {
    if (closed) {
        _HS45HB0.writeMicroseconds(2000);
        _HS45HB1.writeMicroseconds(1000);
    } else {
        _HS45HB0.writeMicroseconds(1000);
        _HS45HB1.writeMicroseconds(2000);
    }
}

// Blocking — only call during setup() or a controlled stop sequence.
void liftARM(bool lift) {
    krs.setPos(KRS_ID, lift ? 11050 : 4400);
    delay(800);
    krs.setFree(KRS_ID);
}
