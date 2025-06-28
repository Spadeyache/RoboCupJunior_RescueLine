/*
  Reading distance from the laser based VL53L1X
  By: Nathan Seidle
  Revised by: Andy England
  SparkFun Electronics
  Date: April 4th, 2018
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  SparkFun labored with love to create this code. Feel like supporting open source hardware? 
  Buy a board from SparkFun! https://www.sparkfun.com/products/14667

  This example prints the distance to an object.

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
  distanceSensor.init();
  Serial.begin(115200);
}

void loop(void)
{
  distanceSensor.getDistanceLeft();
}
