#line 1 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\xiaoesp32\\XIAOdev\\vision.cpp"
#include "vision.h"
#include <Arduino.h>
// duplicated rgb565 to 888 convertion which I would like to only see once

// PLAN integration calibration for the scalar. 
//  clean up the code structure and allow efficient implementation without duplicate calclutions


static float r_scale = 1.0, g_scale = 1.0, b_scale = 1.0;

// Function to set software gains based on WB Mode  // WILL CHANGE TO AUTO CALIBRATION
void set_manual_wb_compensation(int mode) {
    // These are starting guesses; you'll tweak these during calibration
    switch(mode) {
        case 3: // Office (Fluorescent) - Usually needs a boost in Red
            r_scale = 1.2; g_scale = 0.9; b_scale = 1.1;
            break;
        case 2: // Cloudy - Needs more Blue
            r_scale = 1.0; g_scale = 1.0; b_scale = 1.3;
            break;
        case 1: // Sunny
            r_scale = 1.0; g_scale = 1.0; b_scale = 1.0;
            break;
        default: // Manual/None
            r_scale = 1.0; g_scale = 1.0; b_scale = 1.0;
            break;
    }
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







void printRawGrayHSV(camera_fb_t* fb) {
    if (!fb || !fb->buf) return;

    int w = fb->width;
    int h = fb->height;
    int cx = w / 2;
    int cy = h / 2;

    long rSum = 0, gSum = 0, bSum = 0;
    int count = 0;

    // 1. Average a 5x5 box around center (identical logic to debugGray)
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            int x = cx + dx;
            int y = cy + dy;
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

    if (count == 0) return;

    // 2. Calculate averages
    uint8_t avgR = (uint8_t)(rSum / count);
    uint8_t avgG = (uint8_t)(gSum / count);
    uint8_t avgB = (uint8_t)(bSum / count);
    uint8_t gray = rgbToGray(avgR, avgG, avgB);

    // 3. Convert averaged RGB to HSV (Applying software WB inside)
    HSV res = rgb888_to_hsv(avgR, avgG, avgB);

    // 4. Print unified line
    Serial.printf(
        "AVG_BOX:[R:%3d G:%3d B:%3d Gray:%3d] | HSV:[H:%3d S:%3d V:%3d]\n",
        avgR, avgG, avgB, gray,
        res.h, res.s, res.v
    );
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


FrameResult Line_Vision_Process(camera_fb_t* fb) {
    FrameResult result;
    result.frameWidth = fb->width;
    result.frameHeight = fb->height;

    // Initialize results
    result.line.error = 0.0;
    result.line.isSilver = false;
    result.line.isRed = false;
    result.line.isWhite = false;
    result.line.intersection = false;

    result.intersect.isLeftGreen = false;
    result.intersect.isRightGreen = false;

    if (!fb->buf || fb->width == 0 || fb->height == 0) {
        return result;
    }

    const uint8_t* buffer = fb->buf;
    int32_t width = fb->width;
    int32_t height = fb->height;

    // LINE DETECTION - scan row at LINE_SCAN_ROW depth
    int32_t lineY = (int)(height * LINE_SCAN_ROW);
    if (lineY >= 0 && lineY < height) {


        //access the rgb for every element in the lineY row
        int32_t err = 0;
        for (int x = 0; x < width; x++) {
        }
        result.line.error = err;
    }
    

    // // COLOR DETECTION - sample row at COLOR_SCAN_ROW every COLOR_SAMPLE_STEP pixels
    // int colorY = (int)(height * COLOR_SCAN_ROW);
    // if (colorY >= 0 && colorY < height) {
    //     int rSum = 0, gSum = 0, bSum = 0;
    //     int sampleCount = 0;

    //     for (int x = 0; x < width; x += COLOR_SAMPLE_STEP) {
    //         uint16_t pixel = unpackRGB565(buffer, colorY * width + x);

    //         // Extract RGB565 components
    //         uint8_t r = (pixel >> 11) & 0x1F;
    //         uint8_t g = (pixel >> 5) & 0x3F;
    //         uint8_t b = pixel & 0x1F;

    //         // Convert to 8-bit
    //         uint8_t r8 = (r << 3) | (r >> 2);
    //         uint8_t g8 = (g << 2) | (g >> 4);
    //         uint8_t b8 = (b << 3) | (b >> 2);

    //         rSum += r8;
    //         gSum += g8;
    //         bSum += b8;
    //         sampleCount++;
    //     }

    //     if (sampleCount > 0) {
    //         result.color.r = rSum / sampleCount;
    //         result.color.g = gSum / sampleCount;
    //         result.color.b = bSum / sampleCount;

    //         // Calculate color dominance percentages
    //         int maxVal = max(result.color.r, max(result.color.g, result.color.b));
    //         if (maxVal > 0) {
    //             result.color.isGreen = (result.color.g * 100 / maxVal) >= (100 + GREEN_DOMINANCE);
    //             result.color.isRed = (result.color.r * 100 / maxVal) >= (100 + RED_DOMINANCE);
    //             result.color.isBlue = (result.color.b * 100 / maxVal) >= (100 + BLUE_DOMINANCE);
    //         }

    //         // Simple white/black detection
    //         int brightness = (result.color.r + result.color.g + result.color.b) / 3;
    //         result.color.isWhite = brightness > 200;
    //         result.color.isBlack = brightness < 30;
    //     }
    // }

    return result;
}

void Vision_Print(const FrameResult& result) {
    // Serial.printf("LINE: detected=%s, offset=%.2f, darkPixels=%d, darkPercent=%.1f%%\n",
    //               result.line.detected ? "YES" : "NO",
    //               result.line.offset,
    //               result.line.darkPixels,
    //               result.line.darkPercent);

    // Serial.printf("COLOR: R=%d, G=%d, B=%d, Green=%s, Red=%s, Blue=%s, White=%s, Black=%s\n",
    //               result.color.r,
    //               result.color.g,
    //               result.color.b,
    //               result.color.isGreen ? "YES" : "NO",
    //               result.color.isRed ? "YES" : "NO",
    //               result.color.isBlue ? "YES" : "NO",
    //               result.color.isWhite ? "YES" : "NO",
    //               result.color.isBlack ? "YES" : "NO");
}
















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
