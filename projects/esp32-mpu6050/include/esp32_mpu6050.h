#include <Arduino.h>
#include "I2Cdev.h"
#include <MPU6050.h>

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 accelgyro;
//MPU6050 accelgyro(0x69); // <-- use for AD0 high

int16_t ax, ay, az;
int16_t gx, gy, gz;
float dpsX, dpsY, dpsZ;
float interval, preInterval;
float acc_x, acc_y, acc_z, acc_angle_x, acc_angle_y;
float angleX, angleY, angleZ;
double gyro_angle_x = 0, gyro_angle_y = 0, gyro_angle_z = 0;
double offsetX = 0, offsetY = 0, offsetZ = 0;

void calcRotation();
void calibration();
#define OUTPUT_READABLE_ACCELGYRO
