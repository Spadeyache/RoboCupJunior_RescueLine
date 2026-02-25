#include <Arduino.h>
#line 1 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\STS-imuMerge\\STS-imuMerge.ino"
#include "yacheMPU6050.h"

yacheMPU6050 imu;

#line 5 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\STS-imuMerge\\STS-imuMerge.ino"
void setup();
#line 17 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\STS-imuMerge\\STS-imuMerge.ino"
void loop();
#line 5 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\STS-imuMerge\\STS-imuMerge.ino"
void setup() {
    Serial.begin(115200);
    
    imu.begin(Wire, 100.0f);



    // imu.calibrate(); 
    imu.loadOffsetsFromEEPROM(); 
    Serial.println("IMU Ready.");
}

void loop() {
    imu.update();

    // Standard output
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint >= 50) {
        Serial.printf("Pitch: %.2f | Roll: %.2f | Yaw: %.2f\n", 
                      imu.getPitch(), imu.getRoll(), imu.getYaw());
        lastPrint = millis();
    }
}
