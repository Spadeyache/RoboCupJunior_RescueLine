/*
  yacheVL53L0X.h - Library for operating, calibrating and reading the VL53L0X sensor.
  Created by Kent Nakai, June 28, 2025.
  Released into the private domain.
*/
#include "Arduino.h"
#include "SparkFun_VL53L1X.h"
#include "yacheVL53L1X.h"

yacheVL53L1X::yacheVL53L1X(TwoWire* wire, int leftShutdownPin, int rightShutdownPin)
    : _leftPin(leftShutdownPin), _rightPin(rightShutdownPin), _wire(wire),
      _leftTofSensor(*wire, leftShutdownPin, -1), _rightTofSensor(*wire, rightShutdownPin, -1) {
}

void yacheVL53L1X::init() {
    _wire->begin();
    pinMode(_leftPin, OUTPUT);
    pinMode(_rightPin, OUTPUT);
    digitalWrite(_leftPin, LOW);
    digitalWrite(_rightPin, HIGH);
    _rightTofSensor.setI2CAddress(0x30);
    if(_rightTofSensor.begin() != 0) {
        Serial.println("Right ToF failed to begin. Please check wiring. Freezing...");
    }
    digitalWrite(_leftPin, HIGH);
    if(_leftTofSensor.begin() != 0) {//Begin returns 0 on a good init
        Serial.println("Left ToF failed to begin. Please check wiring. Freezing...");
    }
    Serial.println("ToF online!");
    // Short mode max distance is limited to 1.3 m but has a better ambient immunity.
    // Above 1.3 meter error 4 is thrown (wrap around).
    _leftTofSensor.setDistanceModeShort();
    _rightTofSensor.setDistanceModeShort();
    //distanceSensor.setDistanceModeLong(); // default

    Serial.println("VL53L0X Signal-Rate/100 Ambient-Rate/100");
    /*
     * The minimum timing budget is 20 ms for the short distance mode and 33 ms for the medium and long distance modes.
     * Predefined values = 15, 20, 33, 50, 100(default), 200, 500.
     * This function must be called after SetDistanceMode.
     */
    _leftTofSensor.setTimingBudgetInMs(50);
    _rightTofSensor.setTimingBudgetInMs(50);
    /*
     * The intermeasurement period is the time between measurements.
     * Predefined values = 10, 20, 50, 100, 200, 500, 1000.
     * This function must be called after SetTimingBudget.
     */
     // measure periodically. Intermeasurement period must be >/= timing budget.
    _leftTofSensor.setIntermeasurementPeriod(100);
    _rightTofSensor.setIntermeasurementPeriod(100);
    _leftTofSensor.startRanging();
    _rightTofSensor.startRanging();
}

int yacheVL53L1X::getDistanceLeft() {
  _leftTofSensor.startRanging();
  while (!_leftTofSensor.checkForDataReady())
  {
    delay(1);
  }
  int distance = _leftTofSensor.getDistance(); //Get the result of the measurement from the sensor
  _leftTofSensor.clearInterrupt();
  _leftTofSensor.stopRanging();

  Serial.print("Distance(mm): ");
  Serial.print(distance);

  Serial.println();

  return distance;
}

int yacheVL53L1X::getDistanceRight() {
  _rightTofSensor.startRanging();
  while (!_rightTofSensor.checkForDataReady())
  {
    delay(1);
  }
  int distance = _rightTofSensor.getDistance(); //Get the result of the measurement from the sensor
  _rightTofSensor.clearInterrupt();
  _rightTofSensor.stopRanging();

  Serial.print("Distance(mm): ");
  Serial.print(distance);

  Serial.println();

  return distance;
}

void yacheVL53L1X::plotToFData() {

  _leftTofSensor.startRanging();
  while (!_leftTofSensor.checkForDataReady())
  {
    delay(1);
  }
  byte rangeStatusLeft = _leftTofSensor.getRangeStatus();
  unsigned int distanceLeft = _leftTofSensor.getDistance(); //Get the result of the measurement from the sensor
  _leftTofSensor.clearInterrupt();

  /*
     * With signed int we get overflow at short distances.
     * With unsigned we get an overflow below around 2.5 cm.
     */
  unsigned int tSignalRateLeft = _leftTofSensor.getSignalRate();
  unsigned int tAmbientRateLeft = _leftTofSensor.getAmbientRate();

  if (rangeStatusLeft == 0)
  {
#if !defined(ESP32) && !defined(ARDUINO_SAM_DUE) && !defined(__SAM3X8E__)
    tone(11, distanceLeft + 500);
#endif
  }
  else
  {
    // if tAmbientRate > tSignalRate we likely get a signal fail error condition
    // in Distance mode short we get error 4 (out of bounds) or 7 (wrap around) if the distance is greater than 1.3 meter.
    distanceLeft = rangeStatusLeft;
#if !defined(ESP32) && !defined(ARDUINO_SAM_DUE) && !defined(__SAM3X8E__)
    noTone(11);
#endif
  }

  Serial.print("DistanceLeft(mm):");
  Serial.print(distanceLeft);
  Serial.print(' ');
  Serial.print("SignalRate:");
  Serial.print(tSignalRateLeft / 100);//受信する光量
  Serial.print(' ');
  Serial.print("AmbientRate:");
  Serial.println(tAmbientRateLeft / 100);//センサーが発光した光ではない光の受信光量　sunlight, etc.
}