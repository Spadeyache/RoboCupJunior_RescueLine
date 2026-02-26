#line 1 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\STS-imuMerge\\yacheMPU6050.cpp"
#include "yacheMPU6050.h"

// Initialize with the chosen Wire bus for furute development we have it able to spesify the wire bus
yacheMPU6050::yacheMPU6050() : _wire(&Wire), mpu() {}

void yacheMPU6050::begin(TwoWire &w, float32_t sampleRate) {
    _wire = &w;
    _wire->begin();
    // Teensy can handle 400kHz or even 1MHz I2C clocks but MPU6050 cant
    _wire->setClock(400000); 

    // the libraly I2Cdev only supports wire so ther is no point setting begin(Wire)
    mpu.initialize();
    
    if (!mpu.testConnection()) {
        Serial.println("MPU6050 connection failed! Check wiring.");
        while (1);
    }

    // Load from EEPROM. If magic number fails, it triggers calibrate()
    loadOffsetsFromEEPROM();
    applyOffsets();

    filter.begin(sampleRate);
    microsPerReading = (uint32_t)(1000000.0f / sampleRate);
    microsPrevious = micros();
}

void yacheMPU6050::applyOffsets() {
    mpu.setXAccelOffset(ax_offset);
    mpu.setYAccelOffset(ay_offset);
    mpu.setZAccelOffset(az_offset);
    mpu.setXGyroOffset(gx_offset);
    mpu.setYGyroOffset(gy_offset);
    mpu.setZGyroOffset(gz_offset);
}



FASTRUN void yacheMPU6050::update() {
    uint32_t microsNow = micros();
    if (microsNow - microsPrevious >= microsPerReading) {
        
        // 1. Get raw data from I2C
        mpu.getMotion6(&axRaw, &ayRaw, &azRaw, &gxRaw, &gyRaw, &gzRaw);

        // 2. Optimization: Multiplication is faster than division on FPU
        // Using 'f' suffix ensures 32-bit hardware math
        static const float32_t accelScale = 1.0f / 16384.0f;
        static const float32_t gyroScale = 1.0f / 131.0f;

        ax = (float32_t)axRaw * accelScale;
        ay = (float32_t)ayRaw * accelScale;
        az = (float32_t)azRaw * accelScale;
        gx = (float32_t)gxRaw * gyroScale;
        gy = (float32_t)gyRaw * gyroScale;
        gz = (float32_t)gzRaw * gyroScale;

        // 3. Madgwick filter (Heavy Math - benefits from FASTRUN)
        // Ensure your axis mapping is correct for your mounting
        filter.updateIMU(gy, gz, gx, ay, az, ax);

        roll = filter.getRoll();
        pitch = filter.getPitch();
        yaw = filter.getYaw();

        microsPrevious += microsPerReading;
    }
}

// float32_t yacheMPU6050::convertRawAcceleration(int16_t aRaw) {
//     return ((float32_t)aRaw) / 16384.0f;
// }

// float32_t yacheMPU6050::convertRawGyro(int16_t gRaw) {
//     return ((float32_t)gRaw) / 131.0f;
// }

void yacheMPU6050::calibrate() {
    Serial.println("Starting calibration...");
    ax_offset = 0; ay_offset = 0; az_offset = 0;
    gx_offset = 0; gy_offset = 0; gz_offset = 0;
    applyOffsets();
    Serial.println("aplied null offsets");
    
    delay(1000);
    meansensors();

    Serial.println("Running Autocalibration");
    runAutoCalibration();
    Serial.println("finish autocalib");
    meansensors();

    Serial.println("Calibration Finished!");
    saveOffsetsToEEPROM();
}

void yacheMPU6050::runAutoCalibration() {
    // Initial coarse guess
    az_offset = (int16_t)(-mean_az / 8);
    ay_offset = (int16_t)(-mean_ay / 8);
    ax_offset = (int16_t)((16384 - mean_ax) / 8);
    gx_offset = (int16_t)(-mean_gx / 4);
    gy_offset = (int16_t)(-mean_gy / 4);
    gz_offset = (int16_t)(-mean_gz / 4);

    uint16_t cycles = 0;
    while (cycles < 100) { // Safety exit for Teensy high speed
        int8_t ready = 0;
        applyOffsets();
        meansensors();

        if (abs(mean_az) <= acel_deadzone) ready++; else az_offset -= mean_az / acel_deadzone;
        if (abs(mean_ay) <= acel_deadzone) ready++; else ay_offset -= mean_ay / acel_deadzone;
        if (abs(16384 - mean_ax) <= acel_deadzone) ready++; else ax_offset += (16384 - mean_ax) / acel_deadzone;
        if (abs(mean_gx) <= giro_deadzone) ready++; else gx_offset -= mean_gx / (giro_deadzone + 1);
        if (abs(mean_gy) <= giro_deadzone) ready++; else gy_offset -= mean_gy / (giro_deadzone + 1);
        if (abs(mean_gz) <= giro_deadzone) ready++; else gz_offset -= mean_gz / (giro_deadzone + 1);

        if (ready == 6) break;
        cycles++;
        delay(2); // Small rest for I2C bus
    }
}

void yacheMPU6050::meansensors() {
    // Use 64-bit accumulators to prevent overflow during summing
    int64_t buff_ax = 0, buff_ay = 0, buff_az = 0;
    int64_t buff_gx = 0, buff_gy = 0, buff_gz = 0;

    for (int32_t i = 0; i < (buffersize + 100); i++) {
        mpu.getMotion6(&axRaw, &ayRaw, &azRaw, &gxRaw, &gyRaw, &gzRaw);
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
        ax_offset = data.ax;
        ay_offset = data.ay;
        az_offset = data.az;
        gx_offset = data.gx;
        gy_offset = data.gy;
        gz_offset = data.gz;
        Serial.println("Offsets loaded from EEPROM.");
    } else {
        Serial.println("EEPROM magic mismatch. Calibrating...");
        calibrate();
    }
}

void yacheMPU6050::saveOffsetsToEEPROM() {
    CalibrationData data;
    data.magic = MAGIC_NUMBER;
    data.ax = ax_offset;
    data.ay = ay_offset;
    data.az = az_offset;
    data.gx = gx_offset;
    data.gy = gy_offset;
    data.gz = gz_offset;
    
    // EEPROM.put uses update() internally to save flash wear
    EEPROM.put(EEPROM_ADDR, data);
    Serial.println("Offsets saved to EEPROM.");
}

void yacheMPU6050::printQuat() {
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint >= 200) {
        Serial.printf("Yaw: %.2f | Pitch: %.2f | Roll: %.2f\n",
                      yaw, pitch, roll);
        lastPrint = millis();
    }
}