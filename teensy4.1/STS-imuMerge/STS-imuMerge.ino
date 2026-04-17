// /*
// 4th Apr 2026
// Serial xiao test
// */

#include "YacheEncodedSerial.h"
#include "yacheMPU6050.h"
#include "yacheSTS.h"
#include "IcsHardSerialClass.h"
#include <Servo.h>



#define buzzerPin 37
// static constexpr uint8_t BUZZER_DUTY = 116;      // matches setup() beep
// static constexpr uint16_t BUZZER_ON_MS = 40;     // matches setup() beep
// static constexpr uint16_t BUZZER_STEP_MS = 140; // 1s between beeps (easier to count)
// static constexpr uint16_t STATE_STABLE_MS = 50;  // filter flicker from serial updates

// --- ArmServo ---
#define _HS45HB0PIN 3    // Teensy pinnumber  
#define _HS45HB1PIN 4      // Teensy pinnumber
Servo _HS45HB0;
Servo _HS45HB1;

#define _KRSID 1                // Serial Servo ID
// #define _74HTC126EN 2
const byte _74HTC126EN = 2;   // Teensy pinnumber
const long KRSBAUDRATE = 115200; //serial servo baud rate
const int KRSTIMEOUT = 400;     //サーボとのシリアル通信に設定する応答待ち時間
IcsHardSerialClass krs(&Serial1,_74HTC126EN,KRSBAUDRATE,KRSTIMEOUT);




// --- Motor Controol ---
yacheSTS _sts;
// yacheMPU6050 _imu;
elapsedMillis motorTimer;
IntervalTimer controlTimer;
volatile float32_t frontLeftGain = 0.0f;
volatile float32_t frontRightGain = 0.0f;
volatile float32_t backLeftGain = 0.0f;
volatile float32_t backRightGain = 0.0f;
volatile float32_t pitch = 0;
volatile float32_t roll = 0;

// --- Serial ---
YacheEncodedSerial xiao(Serial3);
uint8_t xiaoCommand = 0; // 0x01: 0-linefollow 1-silver 2-red 3-intersection
float32_t pidgain = 127; // 0x02: center of 0-254 

// // --- Xiao state filter (debounce / stability) ---
// static uint8_t rawStateLast = 0;
// static uint32_t rawStateChangedAtMs = 0;
// static uint8_t stableState = 0;

// static uint8_t filterStableState(uint8_t raw) {
//   const uint32_t now = millis();
//   if (raw != rawStateLast) {
//     rawStateLast = raw;
//     rawStateChangedAtMs = now;
//   }
//   if (raw != stableState && (uint32_t)(now - rawStateChangedAtMs) >= STATE_STABLE_MS) {
//     stableState = raw;
//   }
//   return stableState;
// }

// // --- Buzzer (non-blocking, beep only on state change) ---
// static uint8_t buzzerState = 0;
// static uint8_t buzzerBeepIdx = 0;
// static bool buzzerOn = false;
// static bool buzzerBurstActive = false;
// static uint32_t buzzerNextStepAtMs = 0;
// static uint32_t buzzerOffAtMs = 0;

// static inline void buzzerSet_(bool on) {
//   if (on) analogWrite(buzzerPin, BUZZER_DUTY);
//   else analogWrite(buzzerPin, 0);
// }

// // Pattern:
// // - state = 0: silent
// // - state = N (>0): emit N short beeps (1s apart) ONCE when state changes.
// static void updateBuzzer(uint8_t state) {
//   const uint32_t now = millis();

//   if (buzzerOn && (int32_t)(now - buzzerOffAtMs) >= 0) {
//     buzzerOn = false;
//     buzzerSet_(false);
//   }

//   if (state != buzzerState) {
//     buzzerState = state;
//     buzzerBeepIdx = 0;
//     buzzerNextStepAtMs = now; // allow immediate start
//     buzzerBurstActive = (buzzerState != 0);
//     if (buzzerOn) {
//       buzzerOn = false;
//       buzzerSet_(false);
//     }
//   }

//   if (!buzzerBurstActive) return;

//   if ((int32_t)(now - buzzerNextStepAtMs) >= 0) {
//     buzzerOn = true;
//     buzzerSet_(true);
//     buzzerOffAtMs = now + BUZZER_ON_MS;
//     buzzerNextStepAtMs = now + BUZZER_STEP_MS;
//     buzzerBeepIdx++;
//     if (buzzerBeepIdx >= buzzerState) {
//       buzzerBurstActive = false; // done until next state change
//     }
//   }
// }


// A funciton that has priority in precicly updating the imu & motorGains
FASTRUN void motorOutput(){
    // _imu.update();
    // pitch = _imu.getPitch();
    // roll = _imu.getRoll();

    // ADD LOGIC to encoporate the pitch and roll.
    _sts.power(frontLeftGain, frontRightGain, backLeftGain, backRightGain);
    // a note here if _sts.power takes more than 5ms we are in a infinite loop. Since we run 4 STS3032 at 1Mbps should take arround 100 microsec so we are chill.
}

FLASHMEM void setup() {
  Serial.begin(115200);
  _sts.begin(Serial2);
  // _imu.begin(Wire, 200.0f);
  xiao.begin(115200); //XIAO
  //   Serial5.begin(115200); //K230

  pinMode(buzzerPin, OUTPUT);
  analogWrite(buzzerPin, 21);

  // ---HS45HB servo---
  // Start conservative: 1000–2000 us
  _HS45HB0.attach(_HS45HB0PIN, 1000, 2000);
  _HS45HB1.attach(_HS45HB1PIN, 1000, 2000);

  pinMode(_74HTC126EN, OUTPUT);
  digitalWrite(_74HTC126EN, HIGH);
  delay(50);
  _sts.setWheelMode(true);
  krs.begin();  //サーボモータの通信初期設定
  krs.setSpd(_KRSID, 60); //speed 1-127

  delay(5);

  grabARM(true);
  liftARM(true);
  // delay(1759);
  _HS45HB0.detach();
  _HS45HB1.detach();

  // _imu.loadOffsetsFromEEPROM();
  // imu.calibrate(); 

  analogWrite(buzzerPin,0);

  controlTimer.begin(motorOutput, 20000); // 5ms = 5000 microsecond

  motorTimer = 0;
}

void loop() {
    // do not call _sts.power in loop

    

    xiao.update();

    xiaoCommand = xiao.get(0x01); 
    pidgain = (float32_t)xiao.get(0x02);
    // updateBuzzer(filterStableState(xiaoCommand));

  // デバッグ表示（500msごと）
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 20) {
    pidgain = map(pidgain, 0, 254, -220, 220); // motor output constained (-30 ~ 100)
  
  // Serial.print("L: "); Serial.print(40+pidgain);
  // Serial.print(" | RMot: "); Serial.println(40-pidgain);
    motor(100+pidgain,100-pidgain);

    Serial.print("Cmd: "); Serial.print(xiaoCommand);
    Serial.print(" | Gain: "); Serial.println(pidgain);
      
    //   // テスト送信: 相手に現在のgainをそのまま送り返す例
    //   xiao.send(0x02, pidgain);
      
      lastPrint = millis();
  }

  
  
  // --- ここにPID制御などを書く ---
  // ------------------------------------------------------------------
//   delay(20);
  
  // Serial3.print("H");
  // char latestByte = 0;

  // // バッファにある分を全部回して、最後に上書きされたものだけ残す
  // while (Serial3.available() > 0) {
  //     latestByte = Serial3.read(); 
  // }

  // // command of PID gain を見る

  // Serial.println(latestByte);
  // delay(500);
    

    // if (motorTimer >= 200) {
    //     motor(20,20);
    //     motorTimer -= 200; 
    // }

// best for upward front COM
    // _sts.power(30.0f, -15.0f, 55.0f, -25.0f);
    // delay(3000);
    // _sts.power(-15.0f, 30.0f, -25.0f, 55.0f);
    // delay(3000);


    // _imu.printQuat(); //prints every 250ms
    
}

// line follow speed && intersections

#define max 80
FASTRUN void motor(float32_t left, float32_t right){
    noInterrupts(); // Safety: update all 4 at once
    frontLeftGain = constrain(left, -max, max); frontRightGain = constrain(right, -max, max);
    backLeftGain = constrain(left, -max, max);  backRightGain = constrain(right, -max, max);
    interrupts();
}

void grabARM(bool closed){
    if(!closed){
        _HS45HB0.writeMicroseconds(1000);
        _HS45HB1.writeMicroseconds(2000);
    }else{
        _HS45HB0.writeMicroseconds(2000);
        _HS45HB1.writeMicroseconds(1000);
    }
}
void liftARM(bool lift){
    if(lift){
        krs.setPos(_KRSID,11050);  //setPose range: 3500〜11500 11500 は今だけ高めにしてる
    }
    else{
      krs.setPos(_KRSID,4400);  //位置指令　ID:0サーボを7500へ 中央
    }
    delay(800);
    krs.setFree(_KRSID);
}

// make a struct and when arm is droped withingh any command arm will drop and open the hs45hb if its not holding a ball