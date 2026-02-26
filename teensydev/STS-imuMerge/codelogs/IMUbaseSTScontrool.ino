#include "yacheMPU6050.h"
#include "yacheSTS.h"


yacheMPU6050 _imu;
yacheSTS _sts;


void setup() {
    Serial.begin(115200);
    _sts.begin(Serial1, 2);    // Start communication
    _imu.begin(Wire, 100.0f);


    delay(500);


    _sts.setWheelMode(true);   // Set servo to "WheelMode"

    // imu.calibrate(); 
    _imu.loadOffsetsFromEEPROM(); 
    Serial.println("IMU Ready.");
}

void loop() {
    _imu.update();
    _imu.printQuat();


    // Drive forward at 50% power
    _sts.power(-50, -50, -50, -50); 
    delay(200);
    _sts.stop();
    delay(200);
}