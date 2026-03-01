/*
28 Feb 2026
This code tested the HS45HB servo.
*/

#include <Arduino.h>
#include <Servo.h>

static const uint8_t SERVO_PIN = 3;   // choose a PWM-capable pin
Servo servo;

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
}3   