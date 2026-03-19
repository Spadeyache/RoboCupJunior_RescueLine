#include "vision.h"
#include <Arduino.h>

#define boxlength 5 //must be odd

// 255 / (avgR_White - avgR_Black)
const float R_Gain = 1.98598130841;     // 1.98598
const float G_Gain = 2.25265017668;     // 2.25265
const float B_Gain = 4.0;     // 4.84791

const uint8_t R_D = 12.4 * 0.8;       // avgR_Black : 0.8 is the safety margin
const uint8_t G_D = 25.2 * 0.8;       // 
const uint8_t B_D = 17.4 * 0.8;        // 

// Measure Calib constants with gain 1 and D 0
// White avgR=140.8 avgG=138.4 avgB=70
// AVG_BOX:[R:140 G:138 B: 69 Gray:131] | HSV:[H: 58 S:129 V:140]
// AVG_BOX:[R:139 G:140 B: 71 Gray:132] | HSV:[H: 60 S:125 V:140]
// AVG_BOX:[R:142 G:137 B: 71 Gray:131] | HSV:[H: 55 S:127 V:142]
// AVG_BOX:[R:144 G:137 B: 67 Gray:131] | HSV:[H: 54 S:136 V:144]
// AVG_BOX:[R:139 G:140 B: 72 Gray:132] | HSV:[H: 60 S:123 V:140]

// Black avgR=12.4 avgG=25.2 avgB=17.4 // carefull for using gray to get avgB
// AVG_BOX:[R: 12 G: 28 B: 13 Gray: 22] | HSV:[H:123 S:145 V: 28]
// AVG_BOX:[R: 13 G: 23 B: 17 Gray: 19] | HSV:[H:144 S:110 V: 23]
// AVG_BOX:[R: 14 G: 26 B: 17 Gray: 21] | HSV:[H:135 S:117 V: 26]
// AVG_BOX:[R: 12 G: 25 B: 20 Gray: 21] | HSV:[H:156 S:132 V: 25]
// AVG_BOX:[R: 11 G: 24 B: 20 Gray: 20] | HSV:[H:161 S:138 V: 24]

// result
// PRE_CALIB:[R:185 G:201 B:118] | AVG_BOX:[R:255 G:255 B:255 Gray:255] | HSV:[H:  0 S:  0 V:255] : White
// PRE_CALIB:[R: 10 G: 26 B: 27] | AVG_BOX:[R:  1 G: 13 B: 67 Gray: 16] | HSV:[H:229 S:251 V: 67] : Black
// PRE_CALIB:[R: 93 G: 48 B: 44] | AVG_BOX:[R:166 G: 63 B:150 Gray:104] | HSV:[H:309 S:158 V:166] : Red
// PRE_CALIB:[R: 21 G: 62 B: 45] | AVG_BOX:[R: 23 G: 94 B:155 Gray: 80] | HSV:[H:207 S:217 V:155] : Green
// PRE_CALIB:[R: 86 G:105 B: 74] | AVG_BOX:[R:152 G:191 B:244 Gray:185] | HSV:[H:214 S: 96 V:244] : Silver
// PRE_CALIB:[R:150 G:180 B:104] | AVG_BOX:[R:255 G:255 B:255 Gray:255] | HSV:[H:  0 S:  0 V:255] : Silver2

// Findings: the blue channel is very inconsistent and easily effected by the change in light. meaning colors tend to be have a strong or weak blue. So we 


void rgb888Calibration(uint8_t& r8, uint8_t& g8, uint8_t& b8) {
    float calR = ((float)r8 - R_D) * R_Gain;
    float calG = ((float)g8 - G_D) * G_Gain;
    float calB = ((float)b8 - B_D) * B_Gain;

    // costrain to keep in 0-255 bound
    r8 = (uint8_t)constrain(calR, 0, 255);
    g8 = (uint8_t)constrain(calG, 0, 255);
    b8 = (uint8_t)constrain(calB, 0, 255);
}

HSV rgb888_to_hsv(uint8_t r8, uint8_t g8, uint8_t b8) {
    float r = r8;
    float g = g8;
    float b = b8;

    float min = fmin(fmin(r, g), b);
    float max = fmax(fmax(r, g), b);
    float delta = max - min;

    HSV res;
    res.v = (uint8_t)max; // Value

    if (max == 0 || delta == 0) {
        res.h = 0;
        res.s = 0;
        return res;
    }

    res.s = (uint8_t)((delta / max) * 255); // Saturation

    // Hue Calculation
    float h;
    if (r == max) h = (g - b) / delta;
    else if (g == max) h = 2 + (b - r) / delta;
    else h = 4 + (r - g) / delta;

    h *= 60; // Scale to 0-360 range
    if (h < 0) h += 360;

    res.h = (uint16_t)h;
    return res;
}

// remember it returns the 5x5box(maybe I need to add edge case)
cameraData updateRawGrayHSV(camera_fb_t* fb, uint8_t coordX, uint8_t coordY, bool print) {

    cameraData res = {0};

    if (!fb || !fb->buf) return res;

    int w = fb->width;
    int h = fb->height;
    
    // edge case: add later if needed
    if (coordX <= (boxlength - 1)/2) return res;
    if (coordX >= w - (boxlength - 1)/2) return res;
    if (coordY <= (boxlength - 1)/2) return res;
    if (coordY >= h - (boxlength - 1)/2) return res;

    long rSum = 0, gSum = 0, bSum = 0;
    int count = 0;
    // 1. Average a 5x5 box around center (identical logic to debugGray)
    for (int dy = -(boxlength - 1)/2; dy <= (boxlength - 1)/2; dy++) {
        for (int dx = -(boxlength - 1)/2; dx <= (boxlength - 1)/2; dx++) {
            int x = coordX + dx;
            int y = coordY + dy;
            if (x < 0 || x >= w || y < 0 || y >= h) continue;

            uint16_t px = unpackRGB565(fb->buf, y * w + x);
            uint8_t r, g, b;
            rgb565To888(px, r, g, b);

            rSum += r;
            gSum += g;
            bSum += b;
            count++;
        }
    }

    if (count == 0) return res;

    // 2. Calculate averages
    uint8_t avgR = (uint8_t)(rSum / count);
    uint8_t avgG = (uint8_t)(gSum / count);
    uint8_t avgB = (uint8_t)(bSum / count);

    // save the values to print later
    uint8_t preR = avgR;
    uint8_t preG = avgG;
    uint8_t preB = avgB;
    
    // apply the calibration
    rgb888Calibration(avgR, avgG, avgB);

    uint8_t gray = rgbToGray(avgR, avgG, avgB);

    // 3. Convert averaged RGB to HSV (Applying software WB inside)
    HSV hsv = rgb888_to_hsv(avgR, avgG, avgB);

    if (print){
        // 4. Print unified line
        Serial.printf(
            "PRE_CALIB:[R:%3d G:%3d B:%3d] | AVG_BOX:[R:%3d G:%3d B:%3d Gray:%3d] | HSV:[H:%3d S:%3d V:%3d]\n",
            preR, preG, preB,
            avgR, avgG, avgB, gray,
            hsv.h, hsv.s, hsv.v
        );
    }

    res.avgR = avgR;
    res.avgG = avgG;
    res.avgB = avgB;
    res.gray = gray;
    res.hsv = hsv;
    return res;
}

// void debugGray(camera_fb_t* fb) {
//     int w = fb->width;
//     int h = fb->height;
//     int cx = w / 2;
//     int cy = h / 2;

//     long rSum = 0, gSum = 0, bSum = 0, graySum = 0;
//     int count = 0;

//     for (int dy = -2; dy <= 2; dy++) {
//         for (int dx = -2; dx <= 2; dx++) {
//             int x = cx + dx;
//             int y = cy + dy;
//             if (x < 0 || x >= w || y < 0 || y >= h) continue;

//             size_t idx = y * w + x;
//             uint16_t px = unpackRGB565(fb->buf, idx);

//             uint8_t r8, g8, b8;
//             rgb565To888(px, r8, g8, b8);

//             uint8_t gray = rgbToGray(r8, g8, b8);

//             rSum += r8;
//             gSum += g8;
//             bSum += b8;
//             graySum += gray;
//             count++;
//         }
//     }

//     Serial.printf(
//         "r=%d g=%d b=%d gray=%d\n",
//         (int)(rSum / count),
//         (int)(gSum / count),
//         (int)(bSum / count),
//         (int)(graySum / count)
//     );
// }




// void Vision_Print(const FrameResult& result) {
//     // Serial.printf("LINE: detected=%s, offset=%.2f, darkPixels=%d, darkPercent=%.1f%%\n",
//     //               result.line.detected ? "YES" : "NO",
//     //               result.line.offset,
//     //               result.line.darkPixels,
//     //               result.line.darkPercent);

//     // Serial.printf("COLOR: R=%d, G=%d, B=%d, Green=%s, Red=%s, Blue=%s, White=%s, Black=%s\n",
//     //               result.color.r,
//     //               result.color.g,
//     //               result.color.b,
//     //               result.color.isGreen ? "YES" : "NO",
//     //               result.color.isRed ? "YES" : "NO",
//     //               result.color.isBlue ? "YES" : "NO",
//     //               result.color.isWhite ? "YES" : "NO",
//     //               result.color.isBlack ? "YES" : "NO");
// }

// ov2640 sends hibyte first
uint16_t unpackRGB565(const uint8_t* data, size_t index) {
    uint8_t highByte = data[index * 2];
    uint8_t lowByte  = data[index * 2 + 1];
    return (highByte << 8) | lowByte;
}
void rgb565To888(uint16_t px, uint8_t& r8, uint8_t& g8, uint8_t& b8) {
    uint8_t r = (px >> 11) & 0x1F;
    uint8_t g = (px >> 5)  & 0x3F;
    uint8_t b =  px        & 0x1F;

    r8 = (r << 3) | (r >> 2);
    g8 = (g << 2) | (g >> 4);
    b8 = (b << 3) | (b >> 2);
}

uint8_t rgbToGray(uint8_t r, uint8_t g, uint8_t b) {
    float gray = 0.299f * r + 0.587f * g + 0.114f * b;
    if (gray < 0.0f) gray = 0.0f;
    if (gray > 255.0f) gray = 255.0f;
    return (uint8_t)(gray + 0.5f);
}

// Since the camra produces a green biased image, we have the gray scale leaning more to green