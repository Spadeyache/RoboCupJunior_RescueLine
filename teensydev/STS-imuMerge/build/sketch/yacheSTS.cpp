#line 1 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\STS-imuMerge\\yacheSTS.cpp"
#include "yacheSTS.h"

yacheSTS::yacheSTS() {}

void yacheSTS::begin(HardwareSerial &serialPort, int enablePin, unsigned long baud) {
// byte ID[4] = {4, 1, 2, 3};      // Servo IDs
// s16 Speed[4] = {0, 0, 0, 0};    // Target speeds 0-4000
// byte ACC[4] = {150, 150, 150, 150};     // Acceleration values 0-254
    _enPin = enablePin;
    pinMode(_enPin, OUTPUT);
    digitalWrite(_enPin, HIGH); // Power up the bus
    
    serialPort.begin(baud);
    _sts.pSerial = &serialPort;
    
    // delay(500); // Wait for servos to boot

    // // Initialize all servos into Wheel Mode immediately
    // for(int i = 0; i < 4; i++) {
    //     _sts.WheelMode(_ids[i]);
    // }
}


void yacheSTS::setWheelMode() {
    for(int i = 0; i < 4; i++) {
        _sts.WheelMode(_ids[i]);
    }
}

void yacheSTS::setWheelMode(bool enable) {
    stop(); // Safety: Stop moving before switching modes
    delay(10); 
    for(int i = 0; i < 4; i++) {
        if(enable) _sts.WheelMode(_ids[i]);
        else _sts.ServoMode(_ids[i]); // unWheelMode = Position/Step Mode
    }
}

void yacheSTS::power(int8_t lf, int8_t rf, int8_t lb, int8_t rb) {
    // linear mapping of -100, 100 to -4000, 4000
    // Use int32_t for the middle calculation to prevent overflow before assignment
    _speeds[0] = (int16_t)lf * 40;
    _speeds[1] = (int16_t)rf * -40; // Inverted
    _speeds[2] = (int16_t)lb * 40;
    _speeds[3] = (int16_t)rb * -40; // Inverted

    _sts.SyncWriteSpe(_ids, 4, _speeds, _accs);
}

void yacheSTS::stop() {
    power(0, 0, 0, 0);
}