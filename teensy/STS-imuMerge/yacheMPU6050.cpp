#include "yacheMPU6050.h"

// Pass &w as the I2Cdev wireObj so all DMP transactions go through the chosen bus.
yacheMPU6050::yacheMPU6050(TwoWire &w)
    : _wire(&w), _mpu(MPU6050_DEFAULT_ADDRESS, (void*)&w) {}

void yacheMPU6050::begin() {
    _wire->begin();
    _wire->setClock(400000);

    _mpu.initialize();
    if (!_mpu.testConnection()) {
        Serial.println("MPU6050 connection failed! Check wiring.");
        while (1);
    }

    // DMP firmware upload (~1900 bytes over I2C — takes ~1 s at 400 kHz).
    uint8_t devStatus = _mpu.dmpInitialize();
    if (devStatus != 0) {
        Serial.printf("DMP init failed (code %u)\n", devStatus);
        while (1);
    }

    _mpu.setFullScaleAccelRange(0);     // Sets accelerometer to ±2g (highest resolution: 16,384 LSB/g) Free-test also set to 0
    _mpu.setFullScaleGyroRange(0);     // Sets gyroscope to ±250 dps (highest resolution: 131 LSB/dps)

    _mpu.setDLPFMode(MPU6050_DLPF_BW_188);  // 188 Hz — ~1ms group delay
    _mpu.setRate(4);                          // explicit 200 Hz (1000/(1+4))

#if CALIBRATE_IMU
    calibrate();  // prints offsets to Serial; copy them into yacheMPU6050.h then set CALIBRATE_IMU=0
#else
    applyOffsets();
#endif

    _mpu.setDMPEnabled(true);
    _dmpReady = true;
    Serial.printf("IMU+DMP ready. FIFO packet: %u B\n", _mpu.dmpGetFIFOPacketSize());
}

FASTRUN void yacheMPU6050::update() {
    if (!_dmpReady) return;
    if (!_mpu.dmpGetCurrentFIFOPacket(_fifo)) return;  // 0 = no new packet

    Quaternion q_raw;
    _mpu.dmpGetQuaternion(&q_raw, _fifo);

    
    // -90° rotation around Y: maps sensor X(down)→DMP -Z(up), sensor Z(forward)→DMP X(forward).
    // Sensor mounting: X=down, Y=right, Z=forward.
    Quaternion q;
    // const float s = 0.7071f;
    // const float c = 0.7071f;

    // q.w =  c * q_raw.w + s * q_raw.y;
    // q.x =  c * q_raw.x - s * q_raw.z;
    // q.y =  c * q_raw.y - s * q_raw.w;
    // q.z =  c * q_raw.z + s * q_raw.x;

    // Mathematical 90-degree rotation around Y
    q.w = 0.7071f * (q_raw.w - q_raw.y);
    q.x = 0.7071f * (q_raw.x + q_raw.z);
    q.y = 0.7071f * (q_raw.y + q_raw.w);
    q.z = 0.7071f * (q_raw.z - q_raw.x);


    // --- CALCULATE GRAVITY ---
    // The library now calculates gravity based on our "faked" Z axis.
    VectorFloat gravity;
    _mpu.dmpGetGravity(&gravity, &q);

    // --- STEP 3: EXTRACT YPR ---
    // Since we remapped 'q', ypr[0] is now automatically rotating around your X-axis.
    float ypr[3];
    _mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

    // --- STEP 4: CONVERT TO DEGREES ---
    _yaw   =  ypr[0] * (180.0f / (float)M_PI);
    _pitch =  ypr[1] * (180.0f / (float)M_PI);
    _roll  = -ypr[2] * (180.0f / (float)M_PI);  // negated: sensor Y=right vs DMP Y=left
}

void yacheMPU6050::applyOffsets() {
    _mpu.setXAccelOffset(ax_offset);
    _mpu.setYAccelOffset(ay_offset);
    _mpu.setZAccelOffset(az_offset);
    _mpu.setXGyroOffset(gx_offset);
    _mpu.setYGyroOffset(gy_offset);
    _mpu.setZGyroOffset(gz_offset);
}

void yacheMPU6050::calibrate() {
    Serial.println("=== IMU CALIBRATION START — keep robot flat and still ===");
    ax_offset = ay_offset = az_offset = 0;
    gx_offset = gy_offset = gz_offset = 0;
    applyOffsets();
    delay(1000);

    meansensors();
    Serial.printf("Raw means  ax=%d ay=%d az=%d | gx=%d gy=%d gz=%d\n",
                  mean_ax, mean_ay, mean_az, mean_gx, mean_gy, mean_gz);

    runAutoCalibration();
    meansensors();

    Serial.println("=== PASTE THESE INTO yacheMPU6050.h, then set CALIBRATE_IMU 0 ===");
    Serial.printf("    int16_t ax_offset = %d, ay_offset = %d, az_offset = %d;\n",
                  ax_offset, ay_offset, az_offset);
    Serial.printf("    int16_t gx_offset = %d, gy_offset = %d, gz_offset = %d;\n",
                  gx_offset, gy_offset, gz_offset);
    Serial.println("=== Final check means (should be near 0 / 16384 for az) ===");
    Serial.printf("    ax=%d ay=%d az=%d | gx=%d gy=%d gz=%d\n",
                  mean_ax, mean_ay, mean_az, mean_gx, mean_gy, mean_gz);
}

void yacheMPU6050::runAutoCalibration() {
    az_offset = (int16_t)(-mean_az / 8);
    ay_offset = (int16_t)(-mean_ay / 8);
    ax_offset = (int16_t)((16384 - mean_ax) / 8);
    gx_offset = (int16_t)(-mean_gx / 4);
    gy_offset = (int16_t)(-mean_gy / 4);
    gz_offset = (int16_t)(-mean_gz / 4);

    for (uint16_t cycles = 0; cycles < 100; ++cycles) {
        int8_t ready = 0;
        applyOffsets();
        meansensors();
        if (abs(mean_az)           <= acel_deadzone) ready++; else az_offset -= mean_az / acel_deadzone;
        if (abs(mean_ay)           <= acel_deadzone) ready++; else ay_offset -= mean_ay / acel_deadzone;
        if (abs(16384 - mean_ax)   <= acel_deadzone) ready++; else ax_offset += (16384 - mean_ax) / acel_deadzone;
        if (abs(mean_gx)           <= giro_deadzone) ready++; else gx_offset -= mean_gx / (giro_deadzone + 1);
        if (abs(mean_gy)           <= giro_deadzone) ready++; else gy_offset -= mean_gy / (giro_deadzone + 1);
        if (abs(mean_gz)           <= giro_deadzone) ready++; else gz_offset -= mean_gz / (giro_deadzone + 1);
        if (ready == 6) break;
        delay(2);
    }
}

void yacheMPU6050::meansensors() {
    int64_t buff_ax=0, buff_ay=0, buff_az=0;
    int64_t buff_gx=0, buff_gy=0, buff_gz=0;
    for (int32_t i = 0; i < buffersize + 100; ++i) {
        _mpu.getMotion6(&axRaw, &ayRaw, &azRaw, &gxRaw, &gyRaw, &gzRaw);
        if (i > 100) {
            buff_ax += axRaw; buff_ay += ayRaw; buff_az += azRaw;
            buff_gx += gxRaw; buff_gy += gyRaw; buff_gz += gzRaw;
        }
        delay(2);
    }
    mean_ax = (int32_t)(buff_ax / buffersize);
    mean_ay = (int32_t)(buff_ay / buffersize);
    mean_az = (int32_t)(buff_az / buffersize);
    mean_gx = (int32_t)(buff_gx / buffersize);
    mean_gy = (int32_t)(buff_gy / buffersize);
    mean_gz = (int32_t)(buff_gz / buffersize);
}


FLASHMEM void yacheMPU6050::printQuat() {
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint >= 250) {
        Serial.printf("Yaw:%7.2f  Pit:%7.2f  Rol:%7.2f\n", _yaw, _pitch, _roll);
        lastPrint = millis();
    }
}
