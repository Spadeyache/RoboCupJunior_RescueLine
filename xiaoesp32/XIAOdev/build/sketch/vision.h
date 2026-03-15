#line 1 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\xiaoesp32\\XIAOdev\\vision.h"
#ifndef VISION_H
#define VISION_H

#include <Arduino.h>
#include "esp_camera.h"
#define LINE_SCAN_ROW       0.75    // Scan 75% down from top for line detection
#define LINE_DARK_THRESH    80      // Grayscale threshold for dark pixels (0-255)
#define LINE_TRIGGER_PCT    0.35    // Minimum % of dark pixels to trigger line detection
#define COLOR_SCAN_ROW      0.40    // Scan 40% down from top for color detection
#define COLOR_SAMPLE_STEP   4       // Sample every Nth pixel for color averaging
#define GREEN_DOMINANCE     20      // Minimum % green dominance for green detection
#define RED_DOMINANCE       25      // Minimum % red dominance for red detection
#define BLUE_DOMINANCE      25      // Minimum % blue dominance for blue detection

// Result structures
typedef struct {
    bool detected;
    float offset;           // -1.0 (full left) to +1.0 (full right)
    int darkPixels;
    float darkPercent;
} LineResult;

typedef struct {
    uint8_t r, g, b;
    bool isGreen;
    bool isRed;
    bool isBlue;
    bool isWhite;
    bool isBlack;
} ColorResult;

typedef struct {
    LineResult line;
    ColorResult color;
    int frameWidth;
    int frameHeight;
} FrameResult;

// Function declarations
FrameResult Vision_Process(camera_fb_t* fb);
void Vision_Print(const FrameResult& result);
uint16_t Vision_UnpackRGB565(const uint8_t* data, size_t index);
uint8_t Vision_Grayscale(uint8_t r, uint8_t g, uint8_t b);

#endif // VISION_H