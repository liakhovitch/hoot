#include "batt.h"

float get_batt(float correction){
  digitalWrite(21, LOW);
  delay(100);
  pinMode(37, INPUT);
  unsigned int val = analogRead(37);
  //return (float)val * (3.3 / 4096);
  digitalWrite(21, HIGH);
  return (float)val * correction * 0.002578125;
}
