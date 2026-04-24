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
        _turnDuration = TURN_RIGHT_MS;
        Serial.println("Action: Right turn");
    } else {
        motor(TURN_LEFT_L, TURN_LEFT_R);
        _turnDuration = TURN_LEFT_MS;
        Serial.println("Action: Left turn");
    }

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
//  handleTurnTick — call every loop tick while in EXECUTING_TURN state.
//  Transitions back to FOLLOWING_LINE once the timed duration elapses.
// ---------------------------------------------------------------------------
void handleTurnTick() {
    if (millis() - _turnStart >= _turnDuration) {
        motor(0, 0);
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
    _lastEvacBeep = millis() - EVAC_BEEP_PERIOD_MS; // Force immediate first beep
    _beepActive   = false;
    robotState    = EVACUATION_ZONE;
}

// ---------------------------------------------------------------------------
//  handleEvacuationZone — call every loop tick while in EVACUATION_ZONE.
//  Beeps periodically and exits back to FOLLOWING_LINE once XIAO stops
//  reporting silver.
// ---------------------------------------------------------------------------
void handleEvacuationZone() {
    unsigned long now = millis();

    // Start a new beep pulse when the period has elapsed
    if (!_beepActive && (now - _lastEvacBeep >= EVAC_BEEP_PERIOD_MS)) {
        analogWrite(BUZZER_PIN, 160);
        _beepOnStart  = now;
        _beepActive   = true;
        _lastEvacBeep = now;
        Serial.println("Evac: beep");
    }

    // End the beep pulse after EVAC_BEEP_ON_MS
    if (_beepActive && (now - _beepOnStart >= EVAC_BEEP_ON_MS)) {
        analogWrite(BUZZER_PIN, 0);
        _beepActive = false;
    }

    // Exit when XIAO no longer sees silver
    if (xiaoCommand != 5) {
        cmdFilter.clear();
        robotState = FOLLOWING_LINE;
        Serial.println("Evac: silver cleared → FOLLOWING_LINE");
    }
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
