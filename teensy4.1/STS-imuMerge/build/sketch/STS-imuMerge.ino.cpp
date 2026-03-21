#include <Arduino.h>
#line 1 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensy4.1\\STS-imuMerge\\STS-imuMerge.ino"
// /*
// 26 Feb 2026
// This code tested 
// */
#include "yacheMPU6050.h"
#include "yacheSTS.h"

#define _74HTC126EN 2

yacheMPU6050 _imu;
yacheSTS _sts;

elapsedMillis motorTimer;

IntervalTimer controlTimer;

// GLOBAL VARS in (DTCM / RAM1)
volatile float32_t frontLeftGain = 0.0f;
volatile float32_t frontRightGain = 0.0f;
volatile float32_t backLeftGain = 0.0f;
volatile float32_t backRightGain = 0.0f;
volatile float32_t pitch = 0;
volatile float32_t roll = 0;


// A funciton that has priority in precicly updating the imu & motorGains
#line 27 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensy4.1\\STS-imuMerge\\STS-imuMerge.ino"
void motorOutput();
#line 37 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensy4.1\\STS-imuMerge\\STS-imuMerge.ino"
void setup();
#line 58 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensy4.1\\STS-imuMerge\\STS-imuMerge.ino"
void loop();
#line 80 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensy4.1\\STS-imuMerge\\STS-imuMerge.ino"
void motor(float32_t left, float32_t right);
#line 27 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensy4.1\\STS-imuMerge\\STS-imuMerge.ino"
FASTRUN void motorOutput(){
    _imu.update();
    pitch = _imu.getPitch();
    roll = _imu.getRoll();

    // ADD LOGIC to encoporate the pitch and roll.
    _sts.power(frontLeftGain, frontRightGain, backLeftGain, backRightGain);
    // a note here if _sts.power takes more than 5ms we are in a infinite loop. Since we run 4 STS3032 at 1Mbps should take arround 100 microsec so we are chill.
}

FLASHMEM void setup() {
  Serial.begin(115200);
  _sts.begin(Serial1);
//   _imu.begin(Wire, 200.0f);
  Serial4.begin(115200); //XIAO
  Serial5.begin(115200); //K230

  pinMode(_74HTC126EN, OUTPUT);
  digitalWrite(_74HTC126EN, HIGH);
  delay(200);

  _sts.setWheelMode(true);

  // _imu.loadOffsetsFromEEPROM();
  // imu.calibrate(); 

  controlTimer.begin(motorOutput, 5000); // 5ms = 5000 microsecond

  motorTimer = 0;
}

void loop() {
    // do not call _sts.power in loop

    
    if (motorTimer >= 200) {
        motor(20,20);
        motorTimer -= 200; 
    }

// best for upward front COM
    // _sts.power(30.0f, -15.0f, 55.0f, -25.0f);
    // delay(3000);
    // _sts.power(-15.0f, 30.0f, -25.0f, 55.0f);
    // delay(3000);


    // _imu.printQuat(); //prints every 250ms
    
}



FASTRUN void motor(float32_t left, float32_t right){
    noInterrupts(); // Safety: update all 4 at once
    frontLeftGain = left; frontRightGain = right;
    backLeftGain = left;  backRightGain = right;
    interrupts();
}
