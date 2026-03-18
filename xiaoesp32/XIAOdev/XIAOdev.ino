#include <Arduino.h>
#include "camera_config.h"
#include "vision.h"
// #include "wifi_config.h"

// --- PID Parameters ---
const float kP = 2.5;
const float kI = 0.0; // we generally run with PD
const float kD = 0.5;

float error = 0, lastError = 0, integral = 0; // Global vars for pid

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, D7, D6); //UART
    while (!Serial) {
        delay(10); // Wait for serial port to connect
    }

    Serial.println("\n=== XIAO ESP32-S3 Sense Vision System ===");

    // Initialize camera
    if (!Camera_Init()) Serial.println("Camera initialization failed!");
    else Serial.println("Camera initialized successfully");
    // remember to calibration in vision.cpp : follow the steps in reamme.md

    // // Initialize WiFi
    // Serial.printf("Connecting to %s ", WIFI_SSID);
    // if (WiFi_Init()) {
    //     Serial.printf("Connected! IP address: %s\n", WiFi.localIP().toString().c_str());
    //     delay(100);
    // } else {Serial.println("Connection failed or timed out");}
    
    delay(500); //wait for camera to settle take about 2 seconds so 500ms not enough
}

void loop() {
    static unsigned long lastTime = 0;
    if (millis() - lastTime < 240) {
        return; // Run every ↑ms
    }
    lastTime = millis();

    // Capture frame
    camera_fb_t* fb = Camera_Grab();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }

    // get the pixel data
    // cameraData dataLeft = updateRawGrayHSV(fb, (uint8_t)80, (uint8_t)60, true);
    cameraData dataLeft = updateRawGrayHSV(fb, (uint8_t)90, (uint8_t)60);
    cameraData dataRight = updateRawGrayHSV(fb, (uint8_t)70, (uint8_t)60);

    float pidGain= pid((float)dataLeft.gray, (float)dataRight.gray);

    Serial1.print("Hello from XIAO: ");
    Serial1.println(pidGain);


    // Identify green markers, red goal tape, silver and gaps(white / lost line).
    uint8_t sceneCase = classifyScene(dataLeft, dataRight);
    switch (sceneCase) {
        case 0: Serial.println("BOTH GREEN");   break;
        case 1: Serial.println("LEFT GREEN");   break;
        case 2: Serial.println("RIGHT GREEN");  break;
        case 3: Serial.println("RED DETECTED"); break;
        case 4: Serial.println("ALL WHITE");    break;
    }

    stream(fb);

    // Return frame buffer
    Camera_Return(fb);
}


float pid(float left, float right){
    error = (float)left - (float)right; // calculate error

    float P = kP * error;                 // Proportional 
    integral += error;                    // Integral (with Windup Guard)
    integral = constrain(integral, -1000, 1000); // Prevent "Integral Windup"
    float I = kI * integral;
    float D = kD * (error - lastError);   // Derivative

    lastError = error;
    return (P + I + D);
}

// --- code to stream the camera image via serial ---
void stream(camera_fb_t* fb){
    static const uint8_t FRAME_MAGIC[4] = {0xAA, 0x55, 0xBB, 0x44};
    // camera_fb_t* fb = Camera_Grab();
    if (!fb) { delay(10); return; }
    
    uint16_t w = fb->width;
    uint16_t h = fb->height;
    int32_t errVal = 0;  // dummy, viewer needs this to find pixel data
    
    Serial.write(FRAME_MAGIC, 4);
    Serial.write((uint8_t*)&w, 2);
    Serial.write((uint8_t*)&h, 2);
    Serial.write((uint8_t*)&errVal, 4);
    Serial.write(fb->buf, fb->len);
    
    // Camera_Return(fb);
}


// --- Identify green markers, red goal tape, silver and gaps(white / lost line) ---
/**
 * Case separation based on left/right camera data:
 * 0 - Both green
 * 1 - Left only green
 * 2 - Right only green
 * 3 - No black / all white (both gray >= 225)
 * 4 - Red detected (left or right or both)
 */

// Tunable thresholds
#define GREEN_HUE_MIN     80
#define GREEN_HUE_MAX     160
#define GREEN_SAT_MIN     60
#define GREEN_VAL_MIN     40

#define RED_SAT_MIN       80
#define RED_VAL_MIN       40

#define WHITE_GRAY_MIN    225

bool isGreen(const cameraData& d) {
    return (d.hsv.h >= GREEN_HUE_MIN && d.hsv.h <= GREEN_HUE_MAX)
        && (d.hsv.s >= GREEN_SAT_MIN)
        && (d.hsv.v >= GREEN_VAL_MIN);
}

bool isRed(const cameraData& d) {
    return (d.hsv.h <= 20 || d.hsv.h >= 340)
        && (d.hsv.s >= RED_SAT_MIN)
        && (d.hsv.v >= RED_VAL_MIN);
}

bool isWhite(const cameraData& d) {
    return d.gray >= WHITE_GRAY_MIN;
}

// With const — communicates "read only, I won't touch it"  
uint8_t classifyScene(const cameraData& left, const cameraData& right) {

    bool leftGreen  = isGreen(left);
    bool rightGreen = isGreen(right);

    // Case 0: Both green takes highest priority
    if (leftGreen && rightGreen)  return 0;
    // Case 1: Left only green
    if (leftGreen && !rightGreen) return 1;
    // Case 2: Right only green
    if (!leftGreen && rightGreen) return 2;
    // Case 3: Red stop
    if (isRed(left) || isRed(right)) return 3;
    // Case 4: No green, no red → likely all white / silver
    if (isWhite(left) && isWhite(right)) return 4;

    // Fallback: treat as white/no-line if neither sensor sees anything meaningful
    return 4;
}