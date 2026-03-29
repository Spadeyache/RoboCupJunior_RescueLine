#line 1 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensy4.1\\STS-imuMerge\\STS-imuMerge.ino"
// // /*
// // 26 Feb 2026
// // This code tested 
// // */
// #include "yacheMPU6050.h"
// #include "yacheSTS.h"

// #define _74HTC126EN 2

// // yacheMPU6050 _imu;
// yacheSTS _sts;

// elapsedMillis motorTimer;

// IntervalTimer controlTimer;

// // GLOBAL VARS in (DTCM / RAM1)
// volatile float32_t frontLeftGain = 0.0f;
// volatile float32_t frontRightGain = 0.0f;
// volatile float32_t backLeftGain = 0.0f;
// volatile float32_t backRightGain = 0.0f;
// volatile float32_t pitch = 0;
// volatile float32_t roll = 0;


// // A funciton that has priority in precicly updating the imu & motorGains
// FASTRUN void motorOutput(){
//     // _imu.update();
//     // pitch = _imu.getPitch();
//     // roll = _imu.getRoll();

//     // ADD LOGIC to encoporate the pitch and roll.
//     _sts.power(frontLeftGain, frontRightGain, backLeftGain, backRightGain);
//     // a note here if _sts.power takes more than 5ms we are in a infinite loop. Since we run 4 STS3032 at 1Mbps should take arround 100 microsec so we are chill.
// }

// FLASHMEM void setup() {
//   Serial.begin(115200);
//   _sts.begin(Serial1);
// //   _imu.begin(Wire, 200.0f);
// //   Serial4.begin(115200); //XIAO
// //   Serial5.begin(115200); //K230

//   pinMode(_74HTC126EN, OUTPUT);
//   digitalWrite(_74HTC126EN, HIGH);
//   delay(200);

//   _sts.setWheelMode(true);

//   // _imu.loadOffsetsFromEEPROM();
//   // imu.calibrate(); 

//   controlTimer.begin(motorOutput, 5000); // 5ms = 5000 microsecond

//   motorTimer = 0;
// }

// void loop() {
//     // do not call _sts.power in loop

//     motor(60,60);
//     // if (motorTimer >= 200) {
//     //     motor(20,20);
//     //     motorTimer -= 200; 
//     // }

// // best for upward front COM
//     // _sts.power(30.0f, -15.0f, 55.0f, -25.0f);
//     // delay(3000);
//     // _sts.power(-15.0f, 30.0f, -25.0f, 55.0f);
//     // delay(3000);


//     // _imu.printQuat(); //prints every 250ms
    
// }



// FASTRUN void motor(float32_t left, float32_t right){
//     noInterrupts(); // Safety: update all 4 at once
//     frontLeftGain = left; frontRightGain = right;
//     backLeftGain = left;  backRightGain = right;
//     interrupts();
// }












/*
28 Feb 2026
This code tested the HS45HB servo.
*/

#include <Arduino.h>
#include <Servo.h>

#define _74HTC126EN 2
#define _servo0Pin 3
#define _servo1Pin 4

Servo hs45hb0;
Servo hs45hb1;

#line 113 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensy4.1\\STS-imuMerge\\STS-imuMerge.ino"
void setup();
#line 128 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensy4.1\\STS-imuMerge\\STS-imuMerge.ino"
void loop();
#line 136 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensy4.1\\STS-imuMerge\\STS-imuMerge.ino"
void armGripper(bool closed);
#line 145 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensy4.1\\STS-imuMerge\\STS-imuMerge.ino"
void armLift(bool lift);
#line 153 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensy4.1\\STS-imuMerge\\STS-imuMerge.ino"
void dropper(bool drop);
#line 113 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensy4.1\\STS-imuMerge\\STS-imuMerge.ino"
void setup() {
  // min/max are pulse widths in microseconds
  // Start conservative: 1000–2000 us
  hs45hb0.attach(_servo0Pin, 1000, 2000);
  hs45hb1.attach(_servo1Pin, 1000, 2000);

//   servo.writeMicroseconds(1500); // center

  // Enable 74HCT126
  pinMode(_74HTC126EN, OUTPUT);
  digitalWrite(_74HTC126EN, HIGH);

  delay(500);
}

void loop() {
    armGripper(true);
    delay(2000);
    
    armGripper(false);
    delay(2000);
}

void armGripper(bool closed){
    if(closed){
        hs45hb0.writeMicroseconds(1000);
        hs45hb1.writeMicroseconds(1000);
    }else{
        hs45hb0.writeMicroseconds(2000);
        hs45hb1.writeMicroseconds(2000);
    }
}
void armLift(bool lift){
    if(lift){
        // KRS3301 control code
    }
    else{
        // KRS3301 control code
    }
}
void dropper(bool drop){
    if(drop){
        // KRS3301 control code
    }
    else{
        // KRS3301 control code
    }
}
