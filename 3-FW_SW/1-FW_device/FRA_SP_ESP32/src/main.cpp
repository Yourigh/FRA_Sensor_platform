#include <Arduino.h>

void setup() {
  // put your setup code here, to run once:
  pinMode(33,OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  int a = 0;
  a++;
  if (a==200) {
    digitalWrite(33,0);
  }
  digitalWrite(33,1);
  delay(1000);
  digitalWrite(33,0);
  delay(1000);
}