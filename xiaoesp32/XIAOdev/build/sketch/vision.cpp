#line 1 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\xiaoesp32\\XIAOdev\\vision.cpp"
#include "vision.h"
#include <Arduino.h>
// PLAN 1: separate pixel procccesing and image procces in two libraries 
// PLAN 2: integration calibration for the scalar. 

#define boxlength 5 //must be odd

static float r_scale = 1.0, g_scale = 1.0, b_scale = 1.0;

// Function to set software gains based on WB Mode  // WILL CHANGE TO AUTO CALIBRATION
// void set_manual_wb_compensation(int mode) {
//     // These are starting guesses; you'll tweak these during calibration
//     switch(mode) {
//         case 3: // Office (Fluorescent) - Usually needs a boost in Red
//             r_scale = 1.2; g_scale = 0.9; b_scale = 1.1;
//             break;
//         case 2: // Cloudy - Needs more Blue
//             r_scale = 1.0; g_scale = 1.0; b_scale = 1.3;
//             break;
//         case 1: // Sunny
//             r_scale = 1.0; g_scale = 1.0; b_scale = 1.0;
//             break;
//         default: // Manual/None
//             r_scale = 1.0; g_scale = 1.0; b_scale = 1.0;
//             break;
//     }
// }

void perform_white_balance_calibration(camera_fb_t* fb) {
    if (!fb) return;

    // 1. Sample the center of the image (5x5 box)
    // We pass 'false' to print because we will print our own results here
    cameraData data = updateRawGrayHSV(fb, fb->width / 2, fb->height / 2, false);

    if (data.avgR == 0 || data.avgG == 0 || data.avgB == 0) {
        Serial.println("Calibration Failed: Image too dark.");
        return;
    }

    // 2. Calculate new scales. 
    // We target the Green channel as the baseline (1.0)
    r_scale = (float)data.avgG / (float)data.avgR;
    g_scale = 1.0; // Green is our reference
    b_scale = (float)data.avgG / (float)data.avgB;

    Serial.println("--- Calibration Complete ---");
    Serial.printf("Target (Green): %d\n", data.avgG);
    Serial.printf("New Scales -> R: %.2f, G: %.2f, B: %.2f\n", r_scale, g_scale, b_scale);
}

HSV rgb888_to_hsv(uint8_t r8, uint8_t g8, uint8_t b8) {
    // Apply Virtual White Balance Compensation
    float r = r8 * r_scale;
    float g = g8 * g_scale;
    float b = b8 * b_scale;

    // Constrain to 8-bit bounds
    if (r > 255) r = 255; 
    if (g > 255) g = 255; 
    if (b > 255) b = 255;

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
    uint8_t gray = rgbToGray(avgR, avgG, avgB);

    // 3. Convert averaged RGB to HSV (Applying software WB inside)
    HSV hsv = rgb888_to_hsv(avgR, avgG, avgB);

    if (print){
        // 4. Print unified line
        Serial.printf(
            "AVG_BOX:[R:%3d G:%3d B:%3d Gray:%3d] | HSV:[H:%3d S:%3d V:%3d]\n",
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
