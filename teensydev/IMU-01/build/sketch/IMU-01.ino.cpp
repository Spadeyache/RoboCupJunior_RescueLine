#include <Arduino.h>
#line 1 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\IMU-01\\IMU-01.ino"
#include "RobotIMU.h"

// Select the I2C port here: Wire, Wire1, or Wire2
// Wire  -> SDA: 18, SCL: 19
// Wire1 -> SDA: 17, SCL: 16
// Wire2 -> SDA: 25, SCL: 24

// Create the IMU object with the desired I2C port
RobotIMU IMU(Wire); 

#line 11 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\IMU-01\\IMU-01.ino"
void setup();
#line 18 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\IMU-01\\IMU-01.ino"
void loop();
#line 11 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\IMU-01\\IMU-01.ino"
void setup() {
  Serial.begin(9600);

  // Initialize the IMU
  IMU.begin();
}

void loop() {
  // Update the IMU reading (handles timing internally)
  IMU.update();

  // Print data
  Serial.print("Orientation: ");
  Serial.print(IMU.getHeading());
  Serial.print(" ");
  Serial.print(IMU.getPitch());
  Serial.print(" ");
  Serial.println(IMU.getRoll());
}

