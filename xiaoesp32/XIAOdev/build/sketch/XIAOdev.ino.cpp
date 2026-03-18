#line 1 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\xiaoesp32\\XIAOdev\\XIAOdev.ino"
#include <Arduino.h>
#include "camera_config.h"
#include "vision.h"
#include "wifi_config.h"

#line 6 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\xiaoesp32\\XIAOdev\\XIAOdev.ino"
void setup();
#line 56 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\xiaoesp32\\XIAOdev\\XIAOdev.ino"
void loop();
#line 99 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\xiaoesp32\\XIAOdev\\XIAOdev.ino"
void stream(camera_fb_t* fb);
#line 6 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\xiaoesp32\\XIAOdev\\XIAOdev.ino"
void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10); // Wait for serial port to connect
    }

    Serial.println("\n=== XIAO ESP32-S3 Sense Vision System ===");

    // Initialize camera
    if (!Camera_Init()) {
        Serial.println("Camera initialization failed!");
        delay(100);
    } else{Serial.println("Camera initialized successfully");}

    // // --- White balance calibration ---
    // // Point camera at white surface before this runs
    // Serial.println("Calibrating white balance... point at white surface");
    // delay(2000);  // give time to position camera

    // camera_fb_t* cal_fb = Camera_Grab();
    // if (cal_fb) {
    //     Vision_CalibrateWB(cal_fb);
    //     Camera_Return(cal_fb);
    //     Serial.println("WB calibration done.");
    // } else {
    //     Serial.println("WB calibration failed — using default gains");
    // }


    // // Initialize WiFi
    // Serial.printf("Connecting to %s ", WIFI_SSID);
    // if (WiFi_Init()) {
    //     Serial.printf("Connected! IP address: %s\n", WiFi.localIP().toString().c_str());
    //     delay(100);
    // } else {Serial.println("Connection failed or timed out");}
    
    delay(500); //wait for camera to settle
    
    // // Camera Calibration
    // camera_fb_t* fb = Camera_Grab();
    // if (fb) {
    //     perform_white_balance_calibration(fb);
    //     Camera_Return(fb);
    //     Camera_Grab(); // Optional: capture another to clear buffer
    //     Camera_Return(fb);
    // } else {
    //     Serial.println("Calibration Error: Could not capture frame.");
    // }
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

    // Process frame for vision data
    // FrameResult result = Line_Vision_Process(fb);
    // Debug_PrintGrayPatch(fb, fb->width / 2, fb->height / 2);

    cameraData dataLeft = updateRawGrayHSV(fb, (uint8_t)80, (uint8_t)60, true);

    // cameraData dataLeft = updateRawGrayHSV(fb, (uint8_t)90, (uint8_t)60, true);
    // cameraData dataRight = updateRawGrayHSV(fb, (uint8_t)70, (uint8_t)60, true);
    // Serial.print("gray");
    // Serial.println(dataLeft.gray - dataRight.gray);

    // Print results
    // Vision_Print(result);
    // Serial.println((int32_t)(result.line.error));

    stream(fb);

    // Return frame buffer
    Camera_Return(fb);

    // TODO: Add line steering logic here based on result.line.offset
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


