#include <Arduino.h>
#include "YacheEncodedSerial.h"
#include "camera_config.h"
#include "vision.h"
// #include "wifi_config.h"

YacheEncodedSerial teensy(Serial1); // define UART

// --- PID Parameters ---
const float kP = 2.5;
const float kI = 0.0; // we generally run with PD
const float kD = 1.5;

float error = 0, lastError = 0, integral = 0; // Global vars for pid


float pid(float error);

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, D7, D6); //teensy UART begin
    while (!Serial) {
        delay(10); // Wait for serial port to connect
    }

    Serial.println("\n=== XIAO ESP32-S3 Sense Vision System ===");

    // Initialize camera
    if (!Camera_Init()) Serial.println("Camera initialization failed!");
    else Serial.println("Camera initialized successfully");
    // remember calibration in vision.cpp

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
    if (millis() - lastTime < 10) {
        return; // Run every ↑ms
    }
    lastTime = millis();

    // --- Capture frame ---
    camera_fb_t* fb = Camera_Grab();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }

    // get the pixel data
    uint8_t redCount = 0;
    int16_t blackCount = 0; //-32768 ~ 32767
    uint8_t silverCount = 0;

    for(uint8_t pixel = 0; pixel < 160; pixel++){
        cameraData pixelData = updateRawGrayHSV(fb, pixel, (uint8_t)20);
        if(isSilver(pixelData)){              //check silver
            silverCount++;
        }
        else if(isRed(pixelData)){            // check red
            redCount++;
        }
        if(isBlack(pixelData)){               // check black
            if(160 - pixel >= 80){blackCount--;}
            else{blackCount++;}
        }
    }

    // assume green marker (25mm) about 30 pixels with some erro margin
    // we use 25 becuse the line COM might not be exact
    uint8_t greenLeft = 0;
    uint8_t greenRight = 0;
    for(uint8_t pixel = (80 - (blackCount/2) - 25); pixel < 80; pixel++){      // left marker
        if(pixel >= 160){break;}
        cameraData pixelData = updateRawGrayHSV(fb, pixel, (uint8_t)20);
        if(isGreen(pixelData)){
            greenLeft++;
        }
    }
    for(uint8_t pixel = 80; pixel < (80 + (blackCount/2) + 25); pixel++){      // right marker
        if(pixel >= 160){break;}
        cameraData pixelData = updateRawGrayHSV(fb, pixel, (uint8_t)20);
        if(isGreen(pixelData)){
            greenRight++;
        }
    }
    // print greeennnnnjdsfknjektnjkhest
    if(greenLeft > 8 && greenRight > 8){
        Serial.print("both green - ");
        delay(200); return;
    }
    else if(greenLeft > 8){
        Serial.print("leftgreen - ");
        delay(200);
    }
    else if(greenRight > 8){
        Serial.print("right green - ");
        delay(200);
    }

    if(redCount > 30){
        Serial.print("red found - ");
        delay(200);
    }
    if(silverCount > 30){
        Serial.print("silver found - ");
        delay(200);
    }

    Serial.print("blackCount: ");
    Serial.print(blackCount);


    // // cameraData dataLeft = updateRawGrayHSV(fb, (uint8_t)20, (uint8_t)60, true); // 160x120
    // cameraData dataLeft = updateRawGrayHSV(fb, (uint8_t)94, (uint8_t)20);
    // cameraData dataRight = updateRawGrayHSV(fb, (uint8_t)66, (uint8_t)20);

    // float pidGain = pid((float)dataLeft.gray, (float)dataRight.gray); //compute PID error

    float pidGain = pid((float)blackCount);

    Serial.print("prescale pidgain: ");
    Serial.print(pidGain);

    // scale PID gain to 8bit number to send viw UART
    pidGain = constrain(pidGain, -500, 500);
    pidGain = (uint8_t) map(pidGain, -500, 500, 0, 254); //temporally range -500 ~ 500

    // teensy.send(0x01, (uint8_t)0);
    // teensy.send(0x02, pidGain);
    Serial.print("SCpidgain: ");
    Serial.println(pidGain);

    stream(fb);






    // Return frame buffer
    Camera_Return(fb);
}


#define BLACK_GRAY_MIN 35

#define SILVER_RED_MIN 240
#define SILVER_GREEN_MIN 240
#define SILVER_BLUE_MIN 240

bool isBlack(const cameraData& d){
    return d.gray <= BLACK_GRAY_MIN;
}
bool isSilver(const cameraData& d) {
    return (d.avgR > SILVER_RED_MIN) && (d.avgG > SILVER_GREEN_MIN) && (d.avgB > SILVER_BLUE_MIN);
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

// bool isWhite(const cameraData& d) {
//     return d.gray >= WHITE_GRAY_MIN;
// }



// // With const — communicates "read only, I won't touch it"  
// uint8_t classifyScene(const cameraData& left, const cameraData& right) {

//     bool leftGreen  = isGreen(left);
//     bool rightGreen = isGreen(right);

//     if (leftGreen && rightGreen)  return 0;           // Case 0: Both green takes highest priority
//     if (leftGreen && !rightGreen) return 1;           // Case 1: Left only green
//     if (!leftGreen && rightGreen) return 2;           // Case 2: Right only green
//     if (isRed(left) || isRed(right)) return 3;        // Case 3: Red stop
//     if (isSilver(left) || isSilver(right)) return 5;  //Case 5: Silver
//     if (isWhite(left) && isWhite(right)) return 4;    // Case 4: lost line

//     return 4;
// }
// uint8_t classifyMode()





// --- Dont touch ---
float pid(float error /*, float left, float right*/){
    // error = (float)left - (float)right; // calculate error

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