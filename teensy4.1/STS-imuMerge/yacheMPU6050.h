#ifndef yacheMPU6050_h
#define yacheMPU6050_h

#include "Arduino.h"
#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <EEPROM.h>
#include <arm_math.h>

struct CalibrationData {
    int32_t  magic;
    int16_t  ax, ay, az;
    int16_t  gx, gy, gz;
};

class yacheMPU6050 {
public:
    // Pass the Wire bus at construction — DMP needs it at object creation time.
    explicit yacheMPU6050(TwoWire &w = Wire);

    void begin();              // DMP init + EEPROM offset load
    void update() FASTRUN;     // Poll DMP FIFO; updates pitch/roll/yaw if new packet ready
    void calibrate();
    void loadOffsetsFromEEPROM();
    void printQuat() FLASHMEM;

    float32_t getPitch() { return _pitch; }
    float32_t getRoll()  { return _roll; }
    float32_t getYaw()   { return _yaw; }

private:
    TwoWire                    *_wire;
    MPU6050_6Axis_MotionApps20  _mpu;
    bool                        _dmpReady = false;
    uint8_t                     _fifo[64];

    float32_t _pitch = 0.0f, _roll = 0.0f, _yaw = 0.0f;

    int16_t ax_offset = -4737, ay_offset = -374, az_offset = 631;
    int16_t gx_offset = 19,    gy_offset = 54,   gz_offset = 2;

    int32_t buffersize    = 1000;
    int16_t acel_deadzone = 8;
    int16_t giro_deadzone = 1;
    int32_t mean_ax, mean_ay, mean_az, mean_gx, mean_gy, mean_gz;
    int16_t axRaw, ayRaw, azRaw, gxRaw, gyRaw, gzRaw;

    const uint16_t EEPROM_ADDR  = 0;
    const int32_t  MAGIC_NUMBER = 12345;

    void applyOffsets();
    void saveOffsetsToEEPROM();
    void meansensors();
    void runAutoCalibration();
};

#endif
