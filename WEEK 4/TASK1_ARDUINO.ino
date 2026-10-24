#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

unsigned long lastRead = 0;
const unsigned long READ_INTERVAL = 100;

bool sensorReady = false;

bool startSensor() {
  if (mpu.begin()) {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    return true;
  }
  return false;
}

void setup() {
  Serial.begin(9600);

  Wire.begin();
  Wire.setClock(100000);

  // Prevent the I2C bus from waiting indefinitely.
  Wire.setWireTimeout(25000, true);

  Serial.println("Starting MPU6050...");

  sensorReady = startSensor();

  if (sensorReady) {
    Serial.println("MPU6050 READY");
  } else {
    Serial.println("MPU6050 NOT FOUND");
  }
}

void loop() {
  if (millis() - lastRead < READ_INTERVAL) {
    return;
  }
  lastRead = millis();

  if (!sensorReady) {
    Serial.println("Retrying sensor...");
    sensorReady = startSensor();

    if (!sensorReady) {
      return;
    }

    Serial.println("Sensor reconnected");
  }

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Check whether the I2C peripheral timed out.
  if (Wire.getWireTimeoutFlag()) {
    Wire.clearWireTimeoutFlag();
    Serial.println("I2C TIMEOUT - CHECK SENSOR WIRING");
    sensorReady = false;
    return;
  }

  Serial.print(a.acceleration.x, 3);
  Serial.print(",");
  Serial.print(a.acceleration.y, 3);
  Serial.print(",");
  Serial.print(a.acceleration.z, 3);
  Serial.print(",");
  Serial.print(g.gyro.x, 3);
  Serial.print(",");
  Serial.print(g.gyro.y, 3);
  Serial.print(",");
  Serial.println(g.gyro.z, 3);
}