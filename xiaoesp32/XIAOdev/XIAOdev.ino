#include <Arduino.h>
#include "YacheEncodedSerial.h"
#include "camera_config.h"
#include "vision.h"
// #include "wifi_config.h"


YacheEncodedSerial teensy(Serial1);

// --- PID Parameters ---
const float kP = 2.5;
const float kI = 0.0;
const float kD = 1.5;

float lastError = 0, integral = 0;

float pid(float error);
void stream(camera_fb_t* fb);

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, D7, D6);
    while (!Serial) delay(10);

    Serial.println("\n=== XIAO ESP32-S3 Sense Vision System ===");

    if (!Camera_Init()) Serial.println("Camera initialization failed!");
    else Serial.println("Camera initialized successfully");

    delay(500);
}

void loop() {
    static unsigned long lastTime = 0;
    if (millis() - lastTime < 10) return;
    lastTime = millis();

    camera_fb_t* fb = Camera_Grab();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }

    int32_t weightedSum = 0;
    uint8_t blackCount  = 0;
    uint8_t redCount    = 0;
    uint8_t silverCount = 0;

    const uint8_t greenWidth = 40;
    const uint8_t greenGap   = 8;

    // Cache all pixel data in one pass
    cameraData pixels[160];
    for (uint8_t pixel = 0; pixel < 160; pixel++) {
        pixels[pixel] = updateRawGrayHSV(fb, pixel, (uint8_t)30);
        if (isBlack(pixels[pixel]))       { weightedSum += pixel; blackCount++; }
        if (isSilver(pixels[pixel]))      { silverCount++; }
        else if (isRed(pixels[pixel]))    { redCount++; }
    }

    float centerOfMass = (blackCount > 0) ? (float)weightedSum / blackCount : 79.5f;

    // Fixed: Added definitions and types
    int16_t comInt = (int16_t)centerOfMass;
    const uint8_t lineHalfWidth = 10; 

    // Green detection window around line center
    int16_t leftEndSigned    = comInt - lineHalfWidth - greenGap;
    int16_t rightStartSigned = comInt + lineHalfWidth + greenGap;

    uint8_t leftEnd    = (leftEndSigned   > 0)   ? (uint8_t)leftEndSigned    : 0;
    uint8_t leftStart  = (leftEnd > greenWidth)   ? leftEnd - greenWidth      : 0;
    uint8_t rightStart = (rightStartSigned < 159) ? (uint8_t)rightStartSigned : 159;
    uint8_t rightEnd   = (rightStart + greenWidth > 159) ? 159 : (uint8_t)(rightStart + greenWidth);

    uint8_t greenLeft = 0, greenRight = 0;
    for (uint8_t pixel = leftStart;  pixel < leftEnd;  pixel++) if (isGreen(pixels[pixel])) greenLeft++;
    for (uint8_t pixel = rightStart; pixel < rightEnd; pixel++) if (isGreen(pixels[pixel])) greenRight++;



    // if(greenLeft > 10 && greenRight > 10){
    //     Serial.print("both green - ");
    //     delay(200); return;
    // }
    // else if(greenLeft > 10){
    //     Serial.print("leftgreen - ");
    //     delay(200);
    // }
    // else if(greenRight > 10){
    //     Serial.print("right green - ");
    //     delay(200);
    // }

    // if(redCount > 50){
    //     Serial.print("red found - ");
    //     delay(200);
    // }
    // if(silverCount > 20){
    //     Serial.print("silver found - ");
    //     delay(200);
    // }







    // --- PID ---
    float error   = centerOfMass - 79.5f;
    float pidGain = pid(error);

    // Scale PID to 0-254 for UART
    pidGain = constrain(pidGain, -500.0f, 500.0f);
    uint8_t pidByte = (uint8_t)map((long)pidGain, -500, 500, 0, 254);

    // teensy.send(0x01, (uint8_t)0);
    // teensy.send(0x02, pidByte);

    Serial.printf("COM: %5.1f | pid: %3d | black: %3d | red: %3d | silver: %3d | gL: %3d [%3d-%3d] | gR: %3d [%3d-%3d]\n",
        centerOfMass, pidByte, blackCount, redCount, silverCount,
        greenLeft, leftStart, leftEnd,
        greenRight, rightStart, rightEnd);

    // Serial.print(millis() - lastTime);
    // Serial.print(" ");
    stream(fb);
    Camera_Return(fb);
    // Serial.println(millis() - lastTime);
}

// --- Color classifiers ---
#define BLACK_GRAY_MIN    35
#define SILVER_RED_MIN   240
#define SILVER_GREEN_MIN 240
#define SILVER_BLUE_MIN  240

bool isBlack(const cameraData& d) {
    return d.gray <= BLACK_GRAY_MIN;
}
bool isSilver(const cameraData& d) {
    return (d.avgR > SILVER_RED_MIN) && (d.avgG > SILVER_GREEN_MIN) && (d.avgB > SILVER_BLUE_MIN);
}

#define GREEN_HUE_MIN  80
#define GREEN_HUE_MAX 160
#define GREEN_SAT_MIN  60
#define GREEN_VAL_MIN  40

#define RED_SAT_MIN    80
#define RED_VAL_MIN    40

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

// --- PID ---
float pid(float error) {
    float P = kP * error;
    integral += error;
    integral = constrain(integral, -1000.0f, 1000.0f);
    float I = kI * integral;
    float D = kD * (error - lastError);
    lastError = error;
    return (P + I + D);
}

// --- Stream frame over Serial ---
void stream(camera_fb_t* fb) {
    static const uint8_t FRAME_MAGIC[4] = {0xAA, 0x55, 0xBB, 0x44};
    if (!fb) { delay(10); return; }

    uint16_t w = fb->width;
    uint16_t h = fb->height;
    int32_t errVal = 0;

    Serial.write(FRAME_MAGIC, 4);
    Serial.write((uint8_t*)&w, 2);
    Serial.write((uint8_t*)&h, 2);
    Serial.write((uint8_t*)&errVal, 4);
    Serial.write(fb->buf, fb->len);
}