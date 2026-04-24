#include "globals.h"

// =============================================================================
//  Comms.ino — XIAO serial link + command filter
//  Exposes: initComms(), updateComms()
//
//  XIAO register map:
//    0x01 : command  0=idle 1=U-turn 2=left 3=right 4=red 5=silver
//    0x02 : line position 0-254, 127 = dead-centre
// =============================================================================

// --- Variable definitions (declared as extern in globals.h) ---
YacheEncodedSerial xiao(Serial3);
uint8_t            xiaoCommand   = 0;
float              xiaoLineError = 127.0f;
CommandFilter      cmdFilter;

// ---------------------------------------------------------------------------

void initComms() {
    xiao.begin(XIAO_BAUD);
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
}
