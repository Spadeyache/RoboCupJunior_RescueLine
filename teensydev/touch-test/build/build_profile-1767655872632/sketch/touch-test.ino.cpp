#line 1 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\touch-test\\touch-test.ino"
#include <Arduino.h>
const int buttonPin = 2;  // The pin the button is connected to
const int ledPin = 13;    // The built-in LED on the Teensy 4.1

#line 5 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\touch-test\\touch-test.ino"
void setup();
#line 14 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\touch-test\\touch-test.ino"
void loop();
#line 5 "C:\\Users\\magic\\Documents\\robocup\\RoboCupJunior_RescueLine\\touch-test\\touch-test.ino"
void setup() {
  Serial.begin(9600);
  
  // ACTIVATES the internal resistor (High Power comes from inside)
  pinMode(buttonPin, INPUT_PULLUP); 
  
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // Read the button state
  int buttonState = digitalRead(buttonPin);

  // If the button reads LOW, that means it is PRESSED
  if (buttonState == LOW) {
    Serial.println("Touched / Pressed!");
    // digitalWrite(ledPin, HIGH); // Turn the orange light ON
  } 
  else {
    // digitalWrite(ledPin, LOW);  // Turn the orange light OFF
    Serial.println("Not touched / Pressed!");
  }
  
  delay(50); // Small delay to clean up the signal
}

