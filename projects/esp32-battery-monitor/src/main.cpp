#include <Arduino.h>

const int adcPin = 34;
const int R1 = 10000;
const int R2 = 10000;

void setup()
{
// put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();
}

void loop()
{
// put your main code here, to run repeatedly:
  float value = analogRead(adcPin);
  float voltage = value * (R1 + R2) / R2 * (3.6 / 4095); // 3.6 -> 3.9?

  Serial.print("ADC:");
  Serial.print(voltage);
  Serial.print(" Value:");
  Serial.println(value);

  delay(500);
}
