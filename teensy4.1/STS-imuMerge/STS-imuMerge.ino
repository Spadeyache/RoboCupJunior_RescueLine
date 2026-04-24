// =============================================================================
//  STS-imuMerge.ino — Main entry point
//  State machine: FOLLOWING_LINE → EXECUTING_TURN / EVACUATION_ZONE / STALLED_RED
//
//  File layout:
//    config.h      — all tunable constants
//    globals.h     — CommandFilter class + extern declarations for shared vars
//    Sensors.ino   — owns pitch/roll/yaw; IMU init + update
//    Comms.ino     — owns xiao/xiaoCommand/xiaoLineError/cmdFilter; XIAO link
//    Drive.ino     — STS motors + IntervalTimer ISR + runLinePID()
//    Actions.ino   — executeTurn(), doUTurn(), enterEvacuationZone(), ARM helpers
// =============================================================================

#include "globals.h"   // all types, enums, and extern declarations
#include "yacheMPU6050.h"

// ---------------------------------------------------------------------------
//  Forward declarations — listed here as a readable module API reference.
//  (Arduino's auto-prototype generator can fail on default parameters.)
// ---------------------------------------------------------------------------

// Sensors.ino
void initSensors();
void updateSensors();

// Comms.ino
void initComms();
void updateComms();

// Drive.ino
void initDrive();
void motor(float32_t left, float32_t right);
void runLinePID();

// Actions.ino
void initActions();
void executeTurn(float angle, bool blocking = false);
void doUTurn();
void enterEvacuationZone();
void handleTurnTick();
void handleEvacuationZone();
void grabARM(bool closed);
void liftARM(bool lift);

// RobotState enum is defined in globals.h
RobotState robotState    = FOLLOWING_LINE;
bool       isBusyTurning = false; // Set true during turns; blocks new command dispatch

// ---------------------------------------------------------------------------
//  setup
// ---------------------------------------------------------------------------
FLASHMEM void setup() {
    Serial.begin(115200);

    // Startup beep sequence
    analogWrite(BUZZER_PIN, 30); delay(50); analogWrite(BUZZER_PIN, 0);

    pinMode(PIN_74HCT126_EN , OUTPUT);
    digitalWrite(PIN_74HCT126_EN , HIGH);


    initActions();  // Servos, buzzer, KRS, startup beep  (Actions.ino)
    initDrive();    // STS motors + control timer          (Drive.ino)
    initSensors();  // IMU                                 (Sensors.ino)
    initComms();    // XIAO serial                         (Comms.ino)

    // Confirmation beep after motors are ready
    analogWrite(BUZZER_PIN, 160); delay(40);analogWrite(BUZZER_PIN, 0);
}

// ---------------------------------------------------------------------------
//  loop
// ---------------------------------------------------------------------------
void loop() {
    // delay(20);
    updateComms();    // Parse XIAO packets → xiaoCommand, xiaoLineError
    updateSensors();  // Update pitch / roll / yaw from IMU

    switch (robotState) {

        // ------------------------------------------------------------------
        case FOLLOWING_LINE:
        // ------------------------------------------------------------------
            if (!isBusyTurning) {
                // Priority: U-Turn > Intersection (L/R) > Red > Silver > PID
                if      (xiaoCommand == 1) { doUTurn();                    }
                else if (xiaoCommand == 2) { executeTurn(-90.0f);          }
                else if (xiaoCommand == 3) { executeTurn( 90.0f);          }
                else if (xiaoCommand == 4) { robotState = STALLED_RED;     }
                else if (xiaoCommand == 5) { enterEvacuationZone();        }
                else                       { runLinePID(); } // no command → follow line
            }
            break;

        // ------------------------------------------------------------------
        case EXECUTING_TURN:
        // ------------------------------------------------------------------
            handleTurnTick(); // Returns to FOLLOWING_LINE when duration elapses
            break;

        // ------------------------------------------------------------------
        case EVACUATION_ZONE:
        // ------------------------------------------------------------------
            handleEvacuationZone(); // Beeps; exits when XIAO stops seeing silver
            break;

        // ------------------------------------------------------------------
        case STALLED_RED:
        // ------------------------------------------------------------------
            motor(0, 0);
            if (xiaoCommand != 4) {
                cmdFilter.clear();
                robotState = FOLLOWING_LINE;
                Serial.println("Red cleared → FOLLOWING_LINE");
            }
            break;
    }

    // Debug at ~10 Hz
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug >= 100) {
        Serial.printf("State:%d Cmd:%u Err:%.1f Busy:%d | U:%u L:%u R:%u Red:%u Slv:%u\n",
                      (int)robotState, xiaoCommand, xiaoLineError, (int)isBusyTurning,
                      cmdFilter.votesUturn, cmdFilter.votesLeft,
                      cmdFilter.votesRight, cmdFilter.votesRed, cmdFilter.votesSilver);
        lastDebug = millis();
    }
}
