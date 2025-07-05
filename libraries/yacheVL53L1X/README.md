This libraly if for operating VL53L1X efficiently. Simplyfied the code by the assumption 3 VL53L1X sensors will operate at the same time indipendently(you can easilly modify the code to change the number of sensors.

It  wraps arround the Sparkfun VL53L1X Libraly and has the following functions: 

init() //initialize the sensor
getDistanceLeft()
getDistanceRight()
plotTOFData() //plots left ToF data
