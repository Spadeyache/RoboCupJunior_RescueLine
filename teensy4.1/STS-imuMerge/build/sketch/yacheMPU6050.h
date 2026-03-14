#line 1 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\STS-imuMerge\\yacheMPU6050.h"
#ifndef yacheMPU6050_h
#define yacheMPU6050_h

/*
  25 Feb, 2025
*/

#include "Arduino.h"
#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050.h"
#include <MadgwickAHRS.h>
#include <EEPROM.h>
#include <arm_math.h>

// Structure to save the data as a group
struct CalibrationData {
    int32_t  magic;      // 32bit: EEPROM key
    int16_t  ax, ay, az; // 16bit: acceleration offset
    int16_t  gx, gy, gz; // 16bit: gyro offset
};

class yacheMPU6050 {
public:
    yacheMPU6050(); 
    

    void begin(TwoWire &w = Wire, float32_t sampleRate = 25.0); // paramter to spesify the I2C port and data samplerate
    void update() FASTRUN; // to update the quaternion.
    void calibrate();
    void loadOffsetsFromEEPROM();
    void printQuat() FLASHMEM;
    

    float32_t getPitch() { return pitch; }
    float32_t getRoll()  { return roll; }
    float32_t getYaw()   { return yaw; }

private:
    TwoWire *_wire; // Pointer to the I2C bus
    MPU6050 mpu;
    Madgwick filter;

    uint32_t microsPerReading;
    uint32_t microsPrevious;

    int16_t axRaw, ayRaw, azRaw, gxRaw, gyRaw, gzRaw;
    float32_t ax, ay, az, gx, gy, gz;
    float32_t pitch, roll, yaw;

    // Calibration settings
    int32_t buffersize = 1000;
    int16_t acel_deadzone = 8;
    int16_t giro_deadzone = 1;
    
    int16_t ax_offset = -4737, ay_offset = -374, az_offset = 631;
    int16_t gx_offset = 19, gy_offset = 54, gz_offset = 2;
    int32_t mean_ax, mean_ay, mean_az, mean_gx, mean_gy, mean_gz;

    const uint16_t EEPROM_ADDR = 0;
    const int32_t MAGIC_NUMBER = 12345;

    void meansensors();
    void runAutoCalibration();
    inline float32_t convertRawAcceleration(int16_t aRaw) { return ((float32_t)aRaw) / 16384.0f; }
    inline float32_t convertRawGyro(int16_t gRaw) { return ((float32_t)gRaw) / 131.0f; }
    void applyOffsets();
    void saveOffsetsToEEPROM();
};

#endif