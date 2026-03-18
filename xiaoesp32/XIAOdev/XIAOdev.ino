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
    

    // calculate error
    error = (float)dataLeft.gray - (float)dataRight.gray;

    float P = kP * error;                 // Proportional 
    integral += error;                    // Integral (with Windup Guard)
    integral = constrain(integral, -1000, 1000); // Prevent "Integral Windup"
    float I = kI * integral;
    float D = kD * (error - lastError);   // Derivative
    
    // Output
    float pidGain = P + I + D;
    lastError = error;

    Serial1.print("Hello from XIAO: ");
    Serial1.println(pidGain);


    // Serial.println(dataLeft.gray - dataRight.gray);

    // Print results
    // Vision_Print(result);
    // Serial.println((int32_t)(result.line.error));

    stream(fb);

    // Return frame buffer
    Camera_Return(fb);

    // TODO: Add color reaction logic here based on result.color flags

    
}




void stream(camera_fb_t* fb){
    // code snippet to stream the camera image via serial
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

