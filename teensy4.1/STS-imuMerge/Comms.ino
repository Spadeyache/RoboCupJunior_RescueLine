#include "globals.h"

// =============================================================================
//  Comms.ino — XIAO serial link + K230D AI processor UART
//  Exposes: initComms(), updateComms(), initK230(), updateK230()
//
//  XIAO register map:
//    0x01 : command  0=idle 1=U-turn 2=left 3=right 4=red 5=silver
//    0x02 : line position 0-254, 127 = dead-centre
//
//  K230D link (Serial5, TX5=pin20 → K230D RX, RX5=pin21 ← K230D TX):
//    Teensy→K230D  1 byte every K230_CMD_INTERVAL ms:
//      0x00 = IDLE   — outside evacuation zone
//      0x01 = DETECT — inside evacuation zone, run inference
//    K230D→Teensy  [0xAA][COUNT]([TYPE][X][Y])×COUNT[CHKSUM]
// =============================================================================

// --- XIAO variable definitions (declared extern in globals.h) ---
YacheEncodedSerial xiao(Serial3);
uint8_t            xiaoCommand   = 0;
float              xiaoLineError = 127.0f;
CommandFilter      cmdFilter;

// --- K230D variable definitions (declared extern in globals.h) ---
Detection detections[K230_MAX_DETECTIONS];
uint8_t   detectionCount = 0;
bool      k230Running    = false;

// ---------------------------------------------------------------------------
//  K230D frame parser — 4-state byte-by-byte state machine 
// ---------------------------------------------------------------------------
namespace {
    enum ParseState : uint8_t { WAIT_HEADER, WAIT_COUNT, WAIT_DATA, WAIT_CHKSUM };

    ParseState _state  = WAIT_HEADER;
    uint8_t    _count  = 0;
    uint8_t    _idx    = 0;
    // Buffer: [COUNT, TYPE0,X0,Y0, TYPE1,X1,Y1, ..., TYPE7,X7,Y7]
    uint8_t    _buf[1 + K230_MAX_DETECTIONS * 3];
}

static void _parseK230() {
    while (Serial5.available()) {
        uint8_t b = (uint8_t)Serial5.read();

        switch (_state) {
            case WAIT_HEADER:
                if (b == 0xAA) _state = WAIT_COUNT;
                break;

            case WAIT_COUNT:
                if (b > K230_MAX_DETECTIONS) { _state = WAIT_HEADER; break; }
                _count  = b;
                _buf[0] = b;
                _idx    = 0;
                _state  = (_count == 0) ? WAIT_CHKSUM : WAIT_DATA;
                break;

            case WAIT_DATA:
                _buf[1 + _idx++] = b;
                if (_idx >= (uint8_t)(_count * 3)) _state = WAIT_CHKSUM;
                break;

            case WAIT_CHKSUM: {
                uint8_t chk = 0;
                for (uint8_t i = 0; i <= _count * 3; i++) chk ^= _buf[i];

                if (chk == b) {
                    detectionCount = _count;
                    for (uint8_t i = 0; i < _count; i++) {
                        detections[i].type = (K230ObjectType)_buf[1 + i * 3];
                        detections[i].x    =                 _buf[2 + i * 3];
                        detections[i].y    =                 _buf[3 + i * 3];
                    }
                }
                _state = WAIT_HEADER;
                break;
            }
        }
    }
}

// ---------------------------------------------------------------------------

void initComms() {
    xiao.begin(XIAO_BAUD);
    Serial5.begin(K230_BAUD);
}

// Call every loop iteration.
// Parses all pending XIAO packets, then re-runs the vote filter every 20 ms.
void updateComms() {

    xiao.update();
    // Line error is time-critical; update every loop without debounce.
    xiaoLineError = (float)xiao.get(0x02);
    // Filter at 50 Hz — responsive but immune to single-frame glitches.
    static unsigned long lastFilter = 0;
    if (millis() - lastFilter >= 20) {
        uint8_t raw = xiao.get(0x01);
        xiaoCommand = cmdFilter.update(raw);
        lastFilter  = millis();
    }

// Sends run/idle command to K230D at K230_CMD_INTERVAL ms; always parses incoming frames.
    k230Running = (robotState == EVACUATION_ZONE);
    static unsigned long _lastCmd = 0;
    if (millis() - _lastCmd >= K230_CMD_INTERVAL) {
        Serial5.write(k230Running ? (uint8_t)0x01 : (uint8_t)0x00);
        _lastCmd = millis();
    }
    _parseK230(); // Decode the K230 Message
}

