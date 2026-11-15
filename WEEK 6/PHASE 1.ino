/*
  Smart Surveillance System Using ESP32-CAM
  Arduino IDE 2.3.6
  ESP32 Arduino Core 3.x compatible (the error log shows version 3.3.12)

  Board: AI Thinker ESP32-CAM
  Servo signal: GPIO 2
  Servo power: External regulated 5V supply
  IMPORTANT: Connect external supply GND to ESP32-CAM GND.
*/

#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"
#include "esp_timer.h"
#include <stdio.h>
#include <string.h>

// Replace these with your Wi-Fi network details.
const char* ssid = "JasmineMilkTea";
const char* password = "Syuhada2023";

#define PART_BOUNDARY "123456789000000000000987654321"

static const char* _STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;

static const char* _STREAM_BOUNDARY =
    "\r\n--" PART_BOUNDARY "\r\n";

static const char* _STREAM_PART =
    "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

int stream_port_number = 81;

const int servo_pin = 2;

// Original PWM values from the supplied Week 6 code.
const int servo_min_position = 3700;
const int servo_max_position = 7000;
const int servo_speed = 10;

int servo_position = servo_min_position;
bool servo_direction = true;  // true = forward, false = backward


// ---------------------------------------------------------
// Handles the MJPEG video stream
// ---------------------------------------------------------
static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t* fb = NULL;
  esp_err_t res = ESP_OK;
  size_t jpg_buf_len = 0;
  uint8_t* jpg_buf = NULL;
  char part_buf[64];

  static int64_t last_frame = 0;
  if (last_frame == 0) {
    last_frame = esp_timer_get_time();
  }

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    return res;
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  while (true) {
    fb = esp_camera_fb_get();

    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      if (fb->format == PIXFORMAT_JPEG) {
        jpg_buf_len = fb->len;
        jpg_buf = fb->buf;
      } else {
        bool jpeg_converted = frame2jpg(fb, 80, &jpg_buf, &jpg_buf_len);
        esp_camera_fb_return(fb);
        fb = NULL;

        if (!jpeg_converted) {
          Serial.println("JPEG compression failed");
          res = ESP_FAIL;
        }
      }
    }

    if (res == ESP_OK) {
      size_t header_len = snprintf(
          part_buf, sizeof(part_buf), _STREAM_PART, (unsigned int)jpg_buf_len);

      res = httpd_resp_send_chunk(req, part_buf, header_len);
    }

    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(
          req, (const char*)jpg_buf, jpg_buf_len);
    }

    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(
          req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }

    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      jpg_buf = NULL;
    } else if (jpg_buf) {
      free(jpg_buf);
      jpg_buf = NULL;
    }

    if (res != ESP_OK) {
      break;
    }

    // Automatic horizontal servo panning.
    ledcWrite(servo_pin, servo_position);

    if (servo_direction) {
      servo_position += servo_speed;
      if (servo_position > servo_max_position) {
        servo_direction = false;
      }
    } else {
      servo_position -= servo_speed;
      if (servo_position < servo_min_position) {
        servo_direction = true;
      }
    }

    last_frame = esp_timer_get_time();
  }

  last_frame = 0;
  return res;
}


// ---------------------------------------------------------
// Builds the main webpage
// ---------------------------------------------------------
static esp_err_t web_handler(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Content-Encoding", "identity");

  char webPage[256];

  snprintf(
      webPage,
      sizeof(webPage),
      "<!doctype html><html><head><title>ESP32-CAM</title></head>"
      "<body><img id='stream' src='http://%s:%d/stream'></body></html>",
      WiFi.localIP().toString().c_str(),
      stream_port_number
  );

  return httpd_resp_send(req, webPage, HTTPD_RESP_USE_STRLEN);
}


// ---------------------------------------------------------
// Starts the camera web server and video stream server
// ---------------------------------------------------------
void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  httpd_uri_t index_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = web_handler,
    .user_ctx = NULL
  };

  httpd_uri_t stream_uri = {
    .uri = "/stream",
    .method = HTTP_GET,
    .handler = stream_handler,
    .user_ctx = NULL
  };

  Serial.printf("Starting web server on port: '%d'\n", config.server_port);

  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
  } else {
    Serial.println("Failed to start camera web server");
  }

  config.server_port += 1;
  config.ctrl_port += 1;
  stream_port_number = config.server_port;

  Serial.printf("Starting stream server on port: '%d'\n", config.server_port);

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  } else {
    Serial.println("Failed to start stream server");
  }
}


// ---------------------------------------------------------
// Initializes the camera, servo PWM and Wi-Fi
// ---------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("==== Smart Surveillance System ====");

  // AI Thinker ESP32-CAM pin configuration
  camera_config_t config = {};

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;

  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;

  // ESP32 Arduino Core 3.x uses the pin_sccb_* field names.
  config.pin_sccb_sda = 26;
  config.pin_sccb_scl = 27;

  config.pin_pwdn = 32;
  config.pin_reset = -1;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 10;
  config.fb_count = 2;

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  // Initialize servo PWM.
  // ESP32 Arduino Core 3.x: ledcAttach(pin, frequency, resolution).
  // ledcWrite() then uses the pin number, not the old channel number.
  if (!ledcAttach(servo_pin, 50, 16)) {
    Serial.println("Servo PWM attach failed");
    return;
  }

  ledcWrite(servo_pin, servo_position);

  Serial.print("Connecting to Wi-Fi network: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wi-Fi connected");

  startCameraServer();

  Serial.print("Camera ready. Open: http://");
  Serial.println(WiFi.localIP());
}

// ---------------------------------------------------------
// Main loop
// ---------------------------------------------------------
void loop() {
  delay(10000);
}