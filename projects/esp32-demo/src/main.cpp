#include <Arduino.h>

#define ledpin 2

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(ledpin, OUTPUT);
}

void loop()
{
  // put your main code here, to run repeatedly:
  digitalWrite(ledpin, HIGH);
  Serial.printf("%s - LED HIGH\n", __func__);
  delay(1000);
  digitalWrite(ledpin, LOW);
  Serial.printf("%s - LED LOW\n", __func__);
  delay(1000);
}
