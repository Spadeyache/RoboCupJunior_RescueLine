#include <Arduino.h>
#line 1 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\STS-imuMerge\\STS-imuMerge.ino"
#include "yacheMPU6050.h"
#include "yacheSTS.h"

yacheMPU6050 _imu;
yacheSTS _sts;

elapsedMillis imuTimer;

#line 9 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\STS-imuMerge\\STS-imuMerge.ino"
void setup();
#line 29 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\STS-imuMerge\\STS-imuMerge.ino"
void loop();
#line 9 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\STS-imuMerge\\STS-imuMerge.ino"
void setup() {
    Serial.begin(115200);

    _sts.begin(Serial1); 
    
    _imu.begin(Wire, 200.0f);

    int _enPin = 2;
    pinMode(_enPin, OUTPUT);
    digitalWrite(_enPin, HIGH); // Power up the bus

    delay(500); // Allow servos to power up

    _sts.setWheelMode(true);
    _imu.loadOffsetsFromEEPROM();
    
    Serial.println("System Ready.");
    imuTimer = 0;
}

void loop() {
    // 200Hz Control Loop

    
    if (imuTimer >= 5) {
        // Now using optimized float inputs
        _sts.power(20.0f, 20.0f, 20.0f, 20.0f); 
        _imu.update(); 
        imuTimer -= 5; 
    }

    _imu.printQuat();
    
}


