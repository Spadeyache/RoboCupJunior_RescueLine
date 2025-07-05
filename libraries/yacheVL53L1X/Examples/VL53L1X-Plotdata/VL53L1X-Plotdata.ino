/*
  Reading distance from the laser based VL53L1X and plot the data
  By: Kent Nakai
  Date: July 5th, 2025

  This example prints and plots the distance to an object.

  Are you getting weird readings? Be sure the vacuum tape has been removed from the sensor.
*/

#include <Wire.h>
#include "SparkFun_VL53L1X.h" //Click here to get the library: http://librarymanager/All#SparkFun_VL53L1X
#include "yacheVL53L1X.h"

//Optional interrupt and shutdown pins.
#define SHUTDOWN_PIN_LEFT 9
#define SHUTDOWN_PIN_RIGHT 10

yacheVL53L1X distanceSensor(&Wire, SHUTDOWN_PIN_LEFT, SHUTDOWN_PIN_RIGHT);

void setup(void)
{
  Wire.begin();
  Serial.begin(115200);
  
  distanceSensor.init();
  
}

void loop(void)
{
  distanceSensor.plotToFData();
}
