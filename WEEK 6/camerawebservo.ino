/*
  ESP32-CAM Face Detection + Servo Panning
  Board: AI Thinker ESP32-CAM
  ESP32 Arduino core: 2.0.17
  Required library: ESP32Servo (install from Library Manager)

  Camera uses RGB565 at QVGA because the built-in face detector needs
  an uncompressed image. Open the IP printed in Serial Monitor.
  Live stream: http://ESP32_IP:81/stream
*/

#include "esp_camera.h"
#include <WiFi.h>
#include <ESP32Servo.h>
#include "board_config.h"
#include "camera_pins.h"

const char *WIFI_SSID = "nurin";
const char *WIFI_PASSWORD = "nurinnajwa";

#define SERVO_PIN 2
#define SERVO_CENTER 90
#define SERVO_LEFT 60
#define SERVO_RIGHT 120

Servo panServo;
volatile bool faceCurrentlyDetected = false;

static bool cameraReady = false;

void onFaceDetected() {
  static unsigned long lastMove = 0;
  static bool moveRight = false;
  const unsigned long now = millis();

  // Avoid repeatedly moving the servo on every video frame.
  if (now - lastMove >= 1200) {
    moveRight = !moveRight;
    panServo.write(moveRight ? SERVO_RIGHT : SERVO_LEFT);
    lastMove = now;
    Serial.println("FACE DETECTED - SERVO PANNING");
  }
}

void onFaceLost() {
  static unsigned long lastReturn = 0;
  if (millis() - lastReturn >= 700) {
    panServo.write(SERVO_CENTER);
    lastReturn = millis();
    Serial.println("NO FACE - SERVO CENTRED");
  }
}

void startCameraServer();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  delay(1000);
  Serial.println();
  Serial.println("ESP32-CAM FACE DETECTION + SERVO");

  // Servo setup
  panServo.setPeriodHertz(50);
  panServo.attach(SERVO_PIN, 500, 2400);
  panServo.write(SERVO_CENTER);

  // Camera configuration for face detection
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  if (psramFound()) {
    Serial.println("PSRAM detected");
  } else {
    Serial.println("WARNING: PSRAM not detected. Face detection may fail.");
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return;
  }
  cameraReady = true;
  Serial.println("Camera initialized successfully");

  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor) {
    sensor->set_framesize(sensor, FRAMESIZE_QVGA);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("Open this address: http://");
  Serial.println(WiFi.localIP());
  Serial.println("Stream address: http://" + WiFi.localIP().toString() + ":81/stream");

  startCameraServer();
  Serial.println("Camera web server started");
}

void loop() {
  if (!cameraReady) {
    delay(1000);
    return;
  }
  delay(20);
}
