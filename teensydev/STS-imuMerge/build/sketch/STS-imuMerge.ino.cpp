#line 1 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\STS-imuMerge\\STS-imuMerge.ino"
#include <Arduino.h>
#include <Servo.h>

static const uint8_t SERVO_PIN = 3;   // choose a PWM-capable pin
Servo servo;

#line 7 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\STS-imuMerge\\STS-imuMerge.ino"
void setup();
#line 16 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\STS-imuMerge\\STS-imuMerge.ino"
void loop();
#line 7 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\teensydev\\STS-imuMerge\\STS-imuMerge.ino"
void setup() {
  // min/max are pulse widths in microseconds
  // Start conservative: 1000–2000 us
  servo.attach(SERVO_PIN, 1000, 2000);

  servo.writeMicroseconds(1500); // center
  delay(500);
}

void loop() {
  // Sweep using microseconds (more explicit than degrees)
  for (int us = 1000; us <= 2000; us += 10) {
    servo.writeMicroseconds(us);
    delay(10);
  }
  for (int us = 2000; us >= 1000; us -= 10) {
    servo.writeMicroseconds(us);
    delay(10);
  }
}
