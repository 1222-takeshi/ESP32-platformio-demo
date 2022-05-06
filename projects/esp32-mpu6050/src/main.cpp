#include <esp32_mpu6050.h>

void setup()
{
  // join I2C bus (I2Cdev library doesn't do this automatically)
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
  Wire.begin();
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
  Fastwire::setup(400, true);
    #endif

  Serial.begin(115200);

  // initialize device
  Serial.println("Initializing I2C devices...");
  accelgyro.initialize();

  // verify connection
  Serial.println("Testing device connections...");
  Serial.println(
    accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");
  calibration();
}

void calibration()
{
  Serial.print("Calculate Calibration");
  for (int i = 0; i < 3000; i++) {
    int16_t raw_acc_x, raw_acc_y, raw_acc_z, raw_t, raw_gyro_x, raw_gyro_y, raw_gyro_z;
    accelgyro.getMotion6(&raw_acc_x, &raw_acc_y, &raw_acc_z, &raw_gyro_x, &raw_gyro_y, &raw_gyro_z);

    dpsX = ((float)raw_gyro_x) / 65.5;
    dpsY = ((float)raw_gyro_y) / 65.5;
    dpsZ = ((float)raw_gyro_z) / 65.5;
    offsetX += dpsX;
    offsetY += dpsY;
    offsetZ += dpsZ;
    if (i % 1000 == 0) {
      Serial.print(".");
    }
  }
  Serial.println();

  offsetX /= 3000;
  offsetY /= 3000;
  offsetZ /= 3000;

  Serial.print("offsetX : ");
  Serial.println(offsetX);
  Serial.print("offsetY : ");
  Serial.println(offsetY);
  Serial.print("offsetZ : ");
  Serial.println(offsetZ);
}

void calcRotation()
{
  int16_t raw_acc_x, raw_acc_y, raw_acc_z, raw_t, raw_gyro_x, raw_gyro_y, raw_gyro_z;
  accelgyro.getMotion6(&raw_acc_x, &raw_acc_y, &raw_acc_z, &raw_gyro_x, &raw_gyro_y, &raw_gyro_z);

  //単位Gへ変換
  acc_x = ((float)raw_acc_x) / 16384.0;
  acc_y = ((float)raw_acc_y) / 16384.0;
  acc_z = ((float)raw_acc_z) / 16384.0;

  //加速度センサーから角度を算出
  acc_angle_y = atan2(acc_x, acc_z + abs(acc_y)) * 360 / -2.0 / PI;
  acc_angle_x = atan2(acc_y, acc_z + abs(acc_x)) * 360 / 2.0 / PI;

  dpsX = ((float)raw_gyro_x) / 65.5; // LSB sensitivity: 65.5 LSB/dps @ ±500dps
  dpsY = ((float)raw_gyro_y) / 65.5;
  dpsZ = ((float)raw_gyro_z) / 65.5;

  //前回計算した時から今までの経過時間を算出
  interval = millis() - preInterval;
  preInterval = millis();

  //数値積分
  gyro_angle_x += (dpsX - offsetX) * (interval * 0.001);
  gyro_angle_y += (dpsY - offsetY) * (interval * 0.001);
  gyro_angle_z += (dpsZ - offsetZ) * (interval * 0.001);

  //相補フィルター
  angleX = (0.996 * gyro_angle_x) + (0.004 * acc_angle_x);
  angleY = (0.996 * gyro_angle_y) + (0.004 * acc_angle_y);
  angleZ = gyro_angle_z;
  gyro_angle_x = angleX;
  gyro_angle_y = angleY;
  gyro_angle_z = angleZ;
}


void loop()
{
  // read raw accel/gyro measurements from device
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // these methods (and a few others) are also available
  //accelgyro.getAcceleration(&ax, &ay, &az);
  //accelgyro.getRotation(&gx, &gy, &gz);

    #ifdef OUTPUT_READABLE_ACCELGYRO
  // display tab-separated accel/gyro x/y/z values
  // Serial.print("a/g:\t");
  // Serial.print(ax); Serial.print("\t");
  // Serial.print(ay); Serial.print("\t");
  // Serial.print(az); Serial.print("\t");
  // Serial.print(gx); Serial.print("\t");
  // Serial.print(gy); Serial.print("\t");
  // Serial.println(gz);

  calcRotation();

  Serial.print("angleX : ");
  Serial.print(angleX);
  Serial.print("    angleY : ");
  Serial.print(angleY);
  Serial.print("    angleZ : ");
  Serial.println(angleZ);
    #endif

    #ifdef OUTPUT_BINARY_ACCELGYRO
  Serial.write((uint8_t)(ax >> 8)); Serial.write((uint8_t)(ax & 0xFF));
  Serial.write((uint8_t)(ay >> 8)); Serial.write((uint8_t)(ay & 0xFF));
  Serial.write((uint8_t)(az >> 8)); Serial.write((uint8_t)(az & 0xFF));
  Serial.write((uint8_t)(gx >> 8)); Serial.write((uint8_t)(gx & 0xFF));
  Serial.write((uint8_t)(gy >> 8)); Serial.write((uint8_t)(gy & 0xFF));
  Serial.write((uint8_t)(gz >> 8)); Serial.write((uint8_t)(gz & 0xFF));
    #endif
}
