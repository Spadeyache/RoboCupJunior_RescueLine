#include <Arduino.h>
#include "YacheEncodedSerial.h"
#include "camera_config.h"
#include "vision.h"
// #include "wifi_config.h"


YacheEncodedSerial teensy(Serial1);

#define minimumBlackforLine 1 // Ignore anything smaller than 5 pixels


void stream(camera_fb_t* fb);

void setup() {
    Serial.begin(115200);
    Serial1.begin(4000000, SERIAL_8N1, D7, D6);
    while (!Serial1) delay(10);

    Serial.println("\n=== XIAO ESP32-S3 Sense Vision System ===");

    if (!Camera_Init()) Serial.println("Camera initialization failed!");
    else Serial.println("Camera initialized successfully");

    delay(500);
}

void loop() {      // --------- LOOP ----------

    // Serial.print("Start: ");
    // Serial.print(millis());

    camera_fb_t* fb = Camera_Grab();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }
    

    // updateRawGrayHSV(fb, 5/*40*/, (uint8_t)15, true);
    // updateRawGrayHSV(fb, 38/*40*/, (uint8_t)15, true);
    // updateRawGrayHSV(fb, 79/*40*/, (uint8_t)15, true);
    // updateRawGrayHSV(fb, 100/*40*/, (uint8_t)45, true);
    // updateRawGrayHSV(fb, 155/*40*/, (uint8_t)15, true);
    // stream(fb);
    // Camera_Return(fb);
    // return;

    
    // Serial.print(" getHGRAYHSV 1px: ");
    // Serial.print(millis());


    int32_t weightedSum = 0;
    uint8_t blackCount  = 0;
    uint8_t redCount    = 0;
    uint8_t silverCount = 0;

    const uint8_t greenWidth = 25;
    const uint8_t greenGap   = 0;

    // Cache all pixel data in one pass
    cameraData pixels[160];
    for (uint8_t pixel = 50; pixel < 110/*120 is also in the range*/; pixel++) { //pixel range 50.5 ~ 109.5
        pixels[pixel] = updateRawGrayHSV(fb, pixel, (uint8_t)55);//45
        if (isBlack(pixels[pixel]))       { weightedSum += pixel; blackCount++; }
        if (isSilver(pixels[pixel]))      { silverCount++; }
        else if (isRed(pixels[pixel]))    { redCount++; }
    }

    float centerOfMass = (blackCount > minimumBlackforLine) ? (float)weightedSum / blackCount : 79.5f;
    
    // Serial.print(" getCOM: ");
    // Serial.print(millis());

    // Fixed: Added definitions and types
    int16_t comInt = (int16_t)centerOfMass;
    const uint8_t lineHalfWidth = 4; 

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

    
    // Serial.print("datacollced: ");
    // Serial.print(millis());


    // scale the center of mass to 0-254
    float scaledCenterOfMass = map(centerOfMass, 50.5, 109.5, 0, 254);


    teensy.send(0x01, (uint8_t)0);
    teensy.send(0x02, (uint8_t)scaledCenterOfMass);
    

    // --- address defenition (0x02) ---
    // 0.centerofmass   1.U-T   2.Leftgreen 3.RightGreen 4.Red 5.Silver
    if(greenLeft > 5 && greenRight > 5) {Serial.print("Detected U-turn"); teensy.send(0x01, (uint8_t) 1);}  //u turn
    else if(greenLeft> 5) {Serial.print("Detected leftGreen"); teensy.send(0x01, (uint8_t) 2);}             // left  g
    else if(greenRight> 5) {Serial.print("Detected rightGreen"); teensy.send(0x01, (uint8_t) 3);}           // right g

    if(redCount > 30) {Serial.print("Detected Red"); teensy.send(0x01, (uint8_t) 4);}                        // red

    if(silverCount > 9) {Serial.print("Detected Silver"); teensy.send(0x01, (uint8_t) 5);}  // silver reed must be updated depending on the angle the row dim apear differs  
    



    // Serial.print("computationDone: ");
    // Serial.print(millis());

    Serial.printf("COM: %5.1f | black: %3d | red: %3d | silver: %3d | gL: %3d [%3d-%3d] | gR: %3d [%3d-%3d]\n",
        centerOfMass, blackCount, redCount, silverCount,
        greenLeft, leftStart, leftEnd,
        greenRight, rightStart, rightEnd);

    stream(fb);
    Camera_Return(fb);
    teensy.update();
    
    // Serial.print("end: ");
    // Serial.println(millis());
}

// --- Color classifiers ---
#define BLACK_GRAY_MIN    15
#define SILVER_RAW_RED_MIN   250
#define SILVER_RAW_GREEN_MIN 252  // remember theres two ways to detect green. check bellow
#define SILVER_RAW_BLUE_MIN  252

bool isBlack(const cameraData& d) {
    return d.gray <= BLACK_GRAY_MIN;
}
bool isSilver(const cameraData& d) {
    return (d.avgR > SILVER_RAW_RED_MIN) && (d.avgG > SILVER_RAW_GREEN_MIN) && (d.avgB > SILVER_RAW_BLUE_MIN);
}// avg is raw

#define GREEN_HUE_MIN  80
#define GREEN_HUE_MAX 165
#define GREEN_SAT_MIN  150
#define GREEN_VAL_MIN  135

#define RED_SAT_MIN    80
#define RED_VAL_MIN    40

bool isGreen(const cameraData& d) {
    return ((d.hsv.h >= GREEN_HUE_MIN && d.hsv.h <= GREEN_HUE_MAX))
        && (((d.hsv.s >= GREEN_SAT_MIN) && (d.hsv.v >= GREEN_VAL_MIN)) || ((d.hsv.s >= 215) && (d.hsv.v >= 5)));
}

bool isRed(const cameraData& d) {
    return (d.hsv.h <= 20 || d.hsv.h >= 340)
        && (d.hsv.s >= RED_SAT_MIN)
        && (d.hsv.v >= RED_VAL_MIN);
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