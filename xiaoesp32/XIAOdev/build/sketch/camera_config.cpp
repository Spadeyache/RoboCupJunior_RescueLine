#line 1 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\xiaoesp32\\XIAOdev\\camera_config.cpp"
#include "camera_config.h"
#include "esp_camera.h"
#include <Arduino.h>

bool Camera_Init() {
    if (!psramFound()) {
        Serial.println("PSRAM not found! Check board settings.");
        return false;
    }
    
    camera_config_t config;

    config.pin_pwdn     = PWDN_GPIO_NUM;
    config.pin_reset    = RESET_GPIO_NUM;
    config.pin_xclk     = XCLK_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;

    config.pin_d7    = Y9_GPIO_NUM;
    config.pin_d6    = Y8_GPIO_NUM;
    config.pin_d5    = Y7_GPIO_NUM;
    config.pin_d4    = Y6_GPIO_NUM;
    config.pin_d3    = Y5_GPIO_NUM;
    config.pin_d2    = Y4_GPIO_NUM;
    config.pin_d1    = Y3_GPIO_NUM;
    config.pin_d0    = Y2_GPIO_NUM;

    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href  = HREF_GPIO_NUM;
    config.pin_pclk  = PCLK_GPIO_NUM;

    config.xclk_freq_hz = 20000000;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;

    config.pixel_format = PIXFORMAT_RGB565;
    config.frame_size   = FRAMESIZE_QVGA;  // 320x240
    config.jpeg_quality = 12;              // unused for RGB565, but required
    config.fb_count     = 2;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }

    // Optional: tweak sensor settings after init
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_framesize(s, FRAMESIZE_QVGA);
        s->set_quality(s, 12);
        s->set_brightness(s, 0);
        s->set_contrast(s, 0);
        s->set_saturation(s, 0);
        s->set_whitebal(s, 1);      // auto white balance
        s->set_awb_gain(s, 1);
        s->set_exposure_ctrl(s, 1); // auto exposure
        s->set_aec2(s, 1);
        s->set_gain_ctrl(s, 1);     // auto gain
    }

    return true;
}

camera_fb_t* Camera_Grab() {
    return esp_camera_fb_get();
}

void Camera_Return(camera_fb_t* fb) {
    esp_camera_fb_return(fb);
}