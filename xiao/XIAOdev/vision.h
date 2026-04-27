#ifndef VISION_H
#define VISION_H

// All color proccesing happes here.

#include <Arduino.h>
#include "esp_camera.h"
#define LINE_SCAN_ROW       0.5    // Scan 75% down from top for line detection
#define LINE_DARK_THRESH    80      // Grayscale threshold for dark pixels (0-255)
#define LINE_TRIGGER_PCT    0.35    // Minimum % of dark pixels to trigger line detection
#define COLOR_SCAN_ROW      0.40    // Scan 40% down from top for color detection
#define COLOR_SAMPLE_STEP   4       // Sample every Nth pixel for color averaging
#define GREEN_DOMINANCE     20      // Minimum % green dominance for green detection
#define RED_DOMINANCE       25      // Minimum % red dominance for red detection
#define BLUE_DOMINANCE      25      // Minimum % blue dominance for blue detection

// Result structures
typedef struct {
    uint16_t h; // 0-359
    uint8_t s;  // 0-255
    uint8_t v;  // 0-255
} HSV;
typedef struct {
    uint8_t avgR;
    uint8_t avgG;
    uint8_t avgB;

    uint8_t gray;
    HSV hsv;
} cameraData;

// Function declarations
uint16_t unpackRGB565(const uint8_t* data, size_t index);
void rgb565To888(uint16_t px, uint8_t& r8, uint8_t& g8, uint8_t& b8);
void rgb888Calibration(uint8_t& r8, uint8_t& g8, uint8_t& b8);

uint8_t rgbToGray(uint8_t r, uint8_t g, uint8_t b);
HSV rgb888_to_hsv(uint8_t r8, uint8_t g8, uint8_t b8);
cameraData updateRawGrayHSV(camera_fb_t* fb, uint8_t x, uint8_t y, bool = false);

// void debugGray(camera_fb_t* fb);

#endif // VISION_H