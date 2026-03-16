#include "vision.h"
#include <Arduino.h>

// Calibration gains — defined in vision.cpp
static float wb_r_gain = 1.0f;
static float wb_b_gain = 1.0f;

uint16_t Vision_UnpackRGB565(const uint8_t* data, size_t index) {
    // ESP32 is little-endian, so bytes need to be swapped
    uint8_t lowByte = data[index * 2];
    uint8_t highByte = data[index * 2 + 1];
    return (highByte << 8) | lowByte;
}

uint8_t Vision_Grayscale(uint8_t r, uint8_t g, uint8_t b) {
    return (uint8_t)(
        (0.299f * r * wb_r_gain) +
        (0.587f * g) +
        (0.114f * b * wb_b_gain)
    );
}
void Vision_CalibrateWB(camera_fb_t* fb) {
    long r_sum = 0, g_sum = 0, b_sum = 0;
    int count = fb->width * fb->height;

    for (int i = 0; i < count; i++) {
        uint16_t px = Vision_UnpackRGB565(fb->buf, i);

        uint8_t r = (px >> 11) & 0x1F;
        uint8_t g = (px >> 5)  & 0x3F;
        uint8_t b =  px        & 0x1F;

        r_sum += (r << 3) | (r >> 2);
        g_sum += (g << 2) | (g >> 4);
        b_sum += (b << 3) | (b >> 2);
    }

    wb_r_gain = (g_sum / (float)count) / (r_sum / (float)count);
    wb_b_gain = (g_sum / (float)count) / (b_sum / (float)count);
}


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
        // int darkCount = 0;
        // int totalPixels = width;
        // int leftEdge = width;
        // int rightEdge = 0;
        // bool foundEdge = false;

        //access the rgb for every element in the lineY row
        int32_t err = 0;
        
        for (int x = 0; x < width; x++) {
            uint16_t pixel = Vision_UnpackRGB565(buffer, lineY * width + x);

            // Extract RGB565 components
            uint8_t r = (pixel >> 11) & 0x1F;
            uint8_t g = (pixel >> 5) & 0x3F;
            uint8_t b = pixel & 0x1F;

            // Convert to 8-bit for grayscale calculation
            uint8_t r8 = (r << 3) | (r >> 2);
            uint8_t g8 = (g << 2) | (g >> 4);
            uint8_t b8 = (b << 3) | (b >> 2);

            uint8_t gray = Vision_Grayscale(r8, g8, b8);

            // left half of the pixels as "-" and right as "+"
            if(x < (int)(width / 2)){
                err -= gray;
            }
            else {
                err += gray;
            }



        //     if (gray < LINE_DARK_THRESH) {
        //         darkCount++;
        //         if (x < leftEdge) leftEdge = x;
        //         if (x > rightEdge) rightEdge = x;
        //         foundEdge = true;
        //     }
        // }

        // result.line.darkPixels = darkCount;
        // result.line.darkPercent = (float)darkCount / totalPixels * 100.0;

        // if (foundEdge && result.line.darkPercent >= (LINE_TRIGGER_PCT * 100.0)) {
        //     result.line.detected = true;
        //     // Calculate centroid offset normalized to -1.0 to +1.0
        //     int centroid = (leftEdge + rightEdge) / 2;
        //     result.line.offset = (2.0 * centroid / (width - 1)) - 1.0;
        }
        result.line.error = err;
    }
    

    // // COLOR DETECTION - sample row at COLOR_SCAN_ROW every COLOR_SAMPLE_STEP pixels
    // int colorY = (int)(height * COLOR_SCAN_ROW);
    // if (colorY >= 0 && colorY < height) {
    //     int rSum = 0, gSum = 0, bSum = 0;
    //     int sampleCount = 0;

    //     for (int x = 0; x < width; x += COLOR_SAMPLE_STEP) {
    //         uint16_t pixel = Vision_UnpackRGB565(buffer, colorY * width + x);

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