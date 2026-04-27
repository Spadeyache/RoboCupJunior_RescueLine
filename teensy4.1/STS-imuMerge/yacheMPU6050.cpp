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

    // Apply stored calibration offsets AFTER dmpInitialize.
    loadOffsetsFromEEPROM();  // populates offset fields (or triggers calibrate())
    applyOffsets();

    _mpu.setDMPEnabled(true);
    _dmpReady = true;
    Serial.printf("IMU+DMP ready. FIFO packet: %u B\n", _mpu.dmpGetFIFOPacketSize());
}

FASTRUN void yacheMPU6050::update() {
    if (!_dmpReady) return;
    if (!_mpu.dmpGetCurrentFIFOPacket(_fifo)) return;  // 0 = no new packet

    Quaternion   q;
    VectorFloat  gravity;
    float        ypr[3];

    _mpu.dmpGetQuaternion(&q, _fifo);
    _mpu.dmpGetGravity(&gravity, &q);
    _mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

    // DMP returns radians; convert to degrees to match existing API.
    _yaw   = ypr[0] * (180.0f / (float)PI);
    _pitch = ypr[1] * (180.0f / (float)PI);
    _roll  = ypr[2] * (180.0f / (float)PI);
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
    Serial.println("Starting calibration...");
    ax_offset = ay_offset = az_offset = 0;
    gx_offset = gy_offset = gz_offset = 0;
    applyOffsets();
    delay(1000);
    meansensors();
    runAutoCalibration();
    meansensors();
    Serial.println("Calibration done.");
    saveOffsetsToEEPROM();
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

void yacheMPU6050::loadOffsetsFromEEPROM() {
    CalibrationData data;
    EEPROM.get(EEPROM_ADDR, data);
    if (data.magic == MAGIC_NUMBER) {
        ax_offset = data.ax; ay_offset = data.ay; az_offset = data.az;
        gx_offset = data.gx; gy_offset = data.gy; gz_offset = data.gz;
        Serial.println("IMU offsets loaded from EEPROM.");
    } else {
        Serial.println("No EEPROM offsets — calibrating...");
        calibrate();
    }
}

void yacheMPU6050::saveOffsetsToEEPROM() {
    CalibrationData data;
    data.magic = MAGIC_NUMBER;
    data.ax = ax_offset; data.ay = ay_offset; data.az = az_offset;
    data.gx = gx_offset; data.gy = gy_offset; data.gz = gz_offset;
    EEPROM.put(EEPROM_ADDR, data);
    Serial.println("IMU offsets saved to EEPROM.");
}

FLASHMEM void yacheMPU6050::printQuat() {
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint >= 250) {
        Serial.printf("Yaw: %.2f | Pitch: %.2f | Roll: %.2f\n", _yaw, _pitch, _roll);
        lastPrint = millis();
    }
}
