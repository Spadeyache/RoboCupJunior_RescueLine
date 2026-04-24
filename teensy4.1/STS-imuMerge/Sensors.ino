#include "globals.h"

// =============================================================================
//  Sensors.ino — MPU-6050 IMU init and update
//  Exposes: initSensors(), updateSensors()
// =============================================================================

// yacheMPU6050 _imu;  // Uncomment when IMU wiring is connected

// --- Variable definitions (declared as extern in globals.h) ---
volatile float32_t pitch = 0.0f; // Positive = nose-up
volatile float32_t roll  = 0.0f;
volatile float32_t yaw   = 0.0f;

// ---------------------------------------------------------------------------

void initSensors() {
    // _imu.begin(Wire, IMU_SAMPLE_RATE);
    // _imu.loadOffsetsFromEEPROM();
    // Serial.println("IMU ready.");
}

// Called every loop iteration — keep fast.
void updateSensors() {
    // _imu.update();
    // pitch = _imu.getPitch();
    // roll  = _imu.getRoll();
    // yaw   = _imu.getYaw();
}
