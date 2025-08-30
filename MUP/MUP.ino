#include <Wire.h>
#include <MadgwickAHRS.h>

Madgwick filter;
unsigned long microsPerReading, microsPrevious;
float accelScale, gyroScale;

void setup() {
  Serial.begin(9600);

  // start the IMU and filter
  Ini_MPU6050();
  

  // initialize variables to pace updates to correct rate
  microsPerReading = 1000000 / 25;
  microsPrevious = micros();
}

void loop() {
  int aix, aiy, aiz;
  int gix, giy, giz;
  float ax, ay, az;
  float gx, gy, gz;
  float roll, pitch, heading;
  unsigned long microsNow;

  // check if it's time to read data and update the filter
  microsNow = micros();
  if (microsNow - microsPrevious >= microsPerReading) {

    // read raw data from CurieIMU
 

    // convert from raw data to gravity and degrees/second units
    ax = convertRawAcceleration(Get_AX());
    ay = convertRawAcceleration(Get_AY());
    az = convertRawAcceleration(Get_AZ());
    gx = convertRawGyro(Get_GX());
    gy = convertRawGyro(Get_GY());
    gz = convertRawGyro(Get_GZ());

    // print the heading, pitch and roll
    roll = filter.getRoll();
    pitch = filter.getPitch();
    heading = filter.getYaw();
    Serial.print("Orientation: ");
    Serial.print(heading);
    Serial.print(" ");
    Serial.print(pitch);
    Serial.print(" ");
    Serial.println(roll);

    // increment previous time, so we keep proper pace
    microsPrevious = microsPrevious + microsPerReading;
  }
}

float convertRawAcceleration(int aRaw) {
  // since we are using 2 g range
  // -2 g maps to a raw value of -32768
  // +2 g maps to a raw value of 32767
  
  float a = (aRaw * 2.0) / 32768.0;
  return a;
}

float convertRawGyro(int gRaw) {
  // since we are using 250 degrees/seconds range
  // -250 maps to a raw value of -32768
  // +250 maps to a raw value of 32767
  
  float g = (gRaw * 250.0) / 32768.0;
  return g;
}


int Get_AX()
{
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 2);

  while (Wire.available() < 2);
  int AX = Wire.read() << 8 | Wire.read();        // 0x3B
  return AX;
}

int Get_AY()
{
  Wire.beginTransmission(0x68);
  Wire.write(0x3D);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 2);

  while (Wire.available() < 2);
  int AY = Wire.read() << 8 | Wire.read();        // 0x3D
  return AY;
}

int Get_AZ()
{
  Wire.beginTransmission(0x68);
  Wire.write(0x3F);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 2);

  while (Wire.available() < 2);
  int AZ = Wire.read() << 8 | Wire.read();        // 0x3F
  return AZ;
}

int Get_GX()
{
  Wire.beginTransmission(0x68);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 2);

  while (Wire.available() < 2);
  int GX = Wire.read() << 8 | Wire.read();        // 0x43
  return GX;
}

int Get_GY()
{
  Wire.beginTransmission(0x68);
  Wire.write(0x45);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 2);

  while (Wire.available() < 2);
  int GY = Wire.read() << 8 | Wire.read();        // 0x45
  return GY;
}

int Get_GZ()
{
  Wire.beginTransmission(0x68);
  Wire.write(0x47);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 2);

  while (Wire.available() < 2);
  int GZ = Wire.read() << 8 | Wire.read();        // 0x47
  return GZ;
}

void Ini_MPU6050()
{
  Wire.beginTransmission(0x68);             // MPU6050を選択
  Wire.write(0x6B);                   // MPU6050_PWR_MGMT_1レジスタのアドレス指定
  Wire.write(0x00);                   // 0を書き込むことでセンサ動作開始
  Wire.endTransmission();                 // スレーブデバイスに対する送信を完了

  Wire.beginTransmission(0x68);             // LPF設定(Delayは加速度とジャイロ異なるが近い値なのでジャイロ側を記載)
  Wire.write(0x1A);                   // 外部同期、ローパスフィルタ設定
  Wire.write(0x06);                   // --,000(外部同期禁止),110(L.P.F.最大効果)
  Wire.endTransmission();                 // Delay(ms)06:18.6 ,05:13.4 ,04:8.3 ,03:4.8 ,02:2.8 ,01:1.9 ,00:0.98

  Wire.beginTransmission(0x68);             // Gyro初期設定
  Wire.write(0x1B);                   // ジャイロ3軸セルフテスト,フルスケール設定
  Wire.write(0x18);                   // 000(セルフテスト無効 ),xx(フルスケール),---
  Wire.endTransmission();                 // 18:2000°/s ,10:1000°/s ,08:500°/s ,00:250°/s

  Wire.beginTransmission(0x68);             // 加速度センサー初期設定
  Wire.write(0x1C);                   // 加速度3軸セルフテスト,フルスケール設定
  Wire.write(0x00);                   // 000(セルフテスト無効 ),xx(フルスケール),---
  Wire.endTransmission();                 // 18:16G ,10:8G ,08:4G ,00:2G
}
