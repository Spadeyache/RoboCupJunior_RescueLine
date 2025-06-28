/*
  yacheVL53L0X.h - Library for operating, calibrating and reading the VL53L0X sensor.
  Created by Kent Nakai, June 28, 2025.
  Released into the private domain.
*/
#ifndef yacheVL53L1X_h
#define yacheVL53L1X_h

#include "Arduino.h"
#include "SparkFun_VL53L1X.h"

class yacheVL53L1X {
    public:
        yacheVL53L1X(TwoWire* wire, int leftShutdownPin, int rightShutdownPin);
        void init();
        int getDistanceLeft();
        int getDistanceRight();
    private:
        int _leftPin;
        int _rightPin;
        TwoWire* _wire;
        SFEVL53L1X _leftTofSensor;
        SFEVL53L1X _rightTofSensor;
};

#endif
