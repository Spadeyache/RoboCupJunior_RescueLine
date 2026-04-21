// /*
// 4th Apr 2026
// Serial xiao test
// */

#include "YacheEncodedSerial.h"
#include "yacheMPU6050.h"
#include "yacheSTS.h"
#include "IcsHardSerialClass.h"
#include <Servo.h>

#define max 100 //max motor speed


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

class CommandFilter {
private:
    static const int QUEUE_SIZE = 15;
    static const int THRESHOLD = 9;
    uint8_t queue[QUEUE_SIZE];
    uint8_t head = 0;

public:
    uint8_t votesUturn = 0;
    uint8_t votesLeft = 0;
    uint8_t votesRight = 0;
    uint8_t votesRed = 0;
    uint8_t votesSilver = 0;

    CommandFilter() {
        clear();
    }

    void clear() {
        for (int i = 0; i < QUEUE_SIZE; i++) {
            queue[i] = 0;
        }
        head = 0;
        votesUturn = 0;
        votesLeft = 0;
        votesRight = 0;
        votesRed = 0;
        votesSilver = 0;
    }

    uint8_t update(uint8_t rawCmd) {
        queue[head] = rawCmd;
        head = (head + 1) % QUEUE_SIZE;

        votesUturn = 0;
        votesLeft = 0;
        votesRight = 0;
        votesRed = 0;
        votesSilver = 0;

        for (int i = 0; i < QUEUE_SIZE; i++) {
            uint8_t cmd = queue[i];
            if (cmd == 1) { // U-Turn
                votesUturn++;
                votesLeft++;
                votesRight++;
            } else if (cmd == 2) { // Left
                votesLeft++;
            } else if (cmd == 3) { // Right
                votesRight++;
            } else if (cmd == 4) { // Red
                votesRed++;
            } else if (cmd == 5) { // Silver
                votesSilver++;
            }
        }

        // Higher priority ones checked first
        if (votesRed >= THRESHOLD) return 4;
        if (votesSilver >= THRESHOLD) return 5;

        // U-turn rule: We require strong evidence of one side and EVEN TRIVIAL evidence of the other.
        // Because pure Left/Right turns will mathematically NEVER have the other color blob.
        bool isUturn = false;
        if (votesUturn >= 2) isUturn = true; // 2 pure U-turn signals
        else if (votesLeft >= 6 && votesRight >= 1) isUturn = true; // mostly left, but glimpsed right
        else if (votesRight >= 6 && votesLeft >= 1) isUturn = true; // mostly right, but glimpsed left
        else if (votesLeft >= 4 && votesRight >= 4) isUturn = true; // balanced

        if (isUturn) return 1;

        if (votesLeft >= THRESHOLD) return 2;
        if (votesRight >= THRESHOLD) return 3;

        return 0; // PID / Idle
    }
};

CommandFilter cmdFilter;

// A funciton that has priority in precicly updating the imu & motorGains
FASTRUN void motorOutput(){

    // _imu.update();
    // pitch = _imu.getPitch();
    // roll = _imu.getRoll();

    // ADD LOGIC to encoporate the pitch and roll.
    _sts.power(frontLeftGain, frontRightGain, backLeftGain, backRightGain);
    // a note here if _sts.power takes more than 5ms we are in a infinite loop. Since we run 4 STS3032 at 1Mbps should take arround 100 microsec so we are chill.

    // Serial.print("L: "); Serial.print(frontLeftGain);
    // Serial.print(" | RMot: "); Serial.println(frontRightGain);
  }

FLASHMEM void setup() {
  Serial.begin(115200);
  _sts.begin(Serial2);
  // _imu.begin(Wire, 200.0f);
  xiao.begin(4000000); //XIAO
  //   Serial5.begin(115200); //K230


  
  // _imu.loadOffsetsFromEEPROM();
  // imu.calibrate(); 

  pinMode(_74HTC126EN, OUTPUT);
  digitalWrite(_74HTC126EN, HIGH);

  // // ---HS45HB servo---
  // // Start conservative: 1000–2000 us
  _HS45HB0.attach(_HS45HB0PIN, 1000, 2000);
  _HS45HB1.attach(_HS45HB1PIN, 1000, 2000);
  // krs.begin();  //サーボモータの通信初期設定
  // krs.setSpd(_KRSID, 60); //speed 1-127
  // liftARM(true);
  grabARM(true);

// 
  pinMode(buzzerPin, OUTPUT);
  analogWrite(buzzerPin, 30); //30
  delay(50);

  _sts.setWheelMode(true);
  _HS45HB0.detach();
  _HS45HB1.detach();

  analogWrite(buzzerPin,0);
  analogWrite(buzzerPin,160);
  delay(40);
  analogWrite(buzzerPin,0);

  controlTimer.begin(motorOutput, 9000); // 9ms = 9000 microsecond

  motorTimer = 0;
}

void loop() {
    // do not call _sts.power in loop
    

    xiao.update();
    
    static unsigned long lastFilterUpdate = 0;
    if (millis() - lastFilterUpdate >= 20) {
        uint8_t rawXiaoCommand = xiao.get(0x01);
        xiaoCommand = cmdFilter.update(rawXiaoCommand); 
        lastFilterUpdate = millis();
    }

    pidgain = (float32_t)xiao.get(0x02);
    // updateBuzzer(xiaoCommand);


    pidgain = map(pidgain, 0, 254, -200, 200);
    
    motor(100+pidgain * 1.7,100-pidgain);   // ****** x1.3 is for the unbalance of lighting for camera



  // デバッグ表示（500msごと）
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 10) {
    // pidgain = (pidgain < 0) ? (pidgain * 7) : (pidgain * 3);


    

    // Serial.print("Cmd: "); Serial.print(xiaoCommand);
    // Serial.print(" | Gain: "); Serial.println(pidgain);
      
    //   // テスト送信: 相手に現在のgainをそのまま送り返す例
    //   xiao.send(0x02, pidgain);
      
      lastPrint = millis();
  }


  // ADD CASE FOR BLACK BLACK with green makers on the other end ----

  static uint8_t lastExecutedCommand = 0;
  if (xiaoCommand != 0 && xiaoCommand != lastExecutedCommand) {

    Serial.println("===============================================");
    Serial.print("ACTION TRIGGERED (Cmd: "); Serial.print(xiaoCommand); Serial.println(")");
    Serial.print("Queue state -> U-Turn: "); Serial.print(cmdFilter.votesUturn);
    Serial.print(" | Left: "); Serial.print(cmdFilter.votesLeft);
    Serial.print(" | Right: "); Serial.print(cmdFilter.votesRight);
    Serial.print(" | Red: "); Serial.print(cmdFilter.votesRed);
    Serial.print(" | Silver: "); Serial.println(cmdFilter.votesSilver);
    Serial.println("===============================================");

    switch (xiaoCommand) {
      
      case 0: // PID / Idle
        // Serial.println("State 0: Maintaining PID / No Action");
        // motor(0.5, 0.5); // Example: Base speed if 0 means "keep going"
        break;

      case 1: // U-Turn
        Serial.println("Action: Executing U-Turn");
        // motor(0,0);
        // analogWrite(buzzerPin,160);
        // delay(20);
        // analogWrite(buzzerPin,0);
        motor(-10, 10); // Spin in place
        delay(1000);      // Adjust timing based on your robot's speed
        motor(0, 0);
        break;

      case 2: // Left Turn
        Serial.println("Action: Executing Left Turn");
        motor(0,0);
        analogWrite(buzzerPin,160);
        delay(20);
        analogWrite(buzzerPin,0);
        motor(-70, 100); // Pivot left
        delay(600);       // Adjust for 90 degrees
        motor(0, 0);
        break;

      case 3: // Right Turn
        Serial.println("Action: Executing Right Turn");
        motor(0,0);
        analogWrite(buzzerPin,160);
        delay(20);
        analogWrite(buzzerPin,0);
        motor(100, -70); // Pivot right
        delay(600);       // Adjust for 90 degrees
        motor(0, 0);
        break;

      case 4: // Red Detection
        Serial.println("Action: RED Detected - Stopping");
        motor(0,0);
        analogWrite(buzzerPin,160);
        delay(20);
        analogWrite(buzzerPin,0);
        motor(0, 0);
        delay(500);
        break;

      case 5: // Silver Detection
        Serial.println("Action: SILVER Detected - Checking/Beeping");
        motor(0,0);
        analogWrite(buzzerPin,160);
        delay(20);
        analogWrite(buzzerPin,0);
        motor(0, 0);
        // Continuous check loop for Silver
        while(true) {
          Serial.println("Beep... Checking Silver status");
          // beep(); // Call your beep function here
          delay(500); 
          
          /* Note: You'll need a way to break this loop, 
              perhaps by reading a new command from the XIAO
          */
        }
        break;

      default:
        Serial.print("Unknown Command: ");
        Serial.println(xiaoCommand);
        break;
    }
    
    lastExecutedCommand = xiaoCommand;
    cmdFilter.clear(); // Set all past readings to 0 after movement
  } else if (xiaoCommand == 0) {
    lastExecutedCommand = 0; // reset so we can trigger again if needed
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