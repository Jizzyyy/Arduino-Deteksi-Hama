#include <FB_Const.h>
#include <FB_Error.h>
#include <FB_Network.h>
#include <FB_Utils.h>
#include <Firebase.h>
#include <FirebaseFS.h>
#include <Firebase_ESP_Client.h>

#include "Arduino.h"
#include "WiFi.h"
#include "esp_camera.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include "esp_log.h"
#include <LittleFS.h>
#include <FS.h>
#include <addons/RTDBHelper.h>
//Provide the token generation process info.
#include <addons/TokenHelper.h>


//Replace with your network credentials
const char* ssid = "jizzy";
const char* password = "nopalkadapi";

// Insert Firebase project API Key
#define API_KEY "AIzaSyA3jgPQYs6WAI_12EF6ANxhglzS4OR4HuM"
#define USER_EMAIL "naufalkadhafi513@gmail.com"
#define USER_PASSWORD "nflkdhf1292"
#define DATABASE_URL "https://dataset-projectt-default-rtdb.asia-southeast1.firebasedatabase.app/"

#define STORAGE_BUCKET_ID "dataset-projectt.appspot.com"
#define FILE_PHOTO_PATH "/gambar.jpg"
#define BUCKET_PHOTO "/temp_photo/gambar.jpg"

// OV2640 camera module pins (CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

boolean takeNewPhoto = true;
int pir = 2;
int ldrDO = 14;
int nilai = 0;
int buzzerPin = 15;
int LedKuning1 = 13;
// int LedKuning2 = 12;
int a;
int delayPhoto = 1;

//Define Firebase Data objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig configF;

void fcsUploadCallback(FCS_UploadStatusInfo info);
bool taskCompleted = false;

void capturePhotoSaveLittleFS(void) {
  camera_fb_t* fb = NULL;
  for (int i = 0; i < 4; i++) {
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = NULL;
  }

  fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }

  // Serial.printf("Picture file name: %s\n", FILE_PHOTO_PATH);
  File file = LittleFS.open(FILE_PHOTO_PATH, FILE_WRITE);

  if (!file) {
    // Serial.println("Failed to open file in writing mode");
  } else {
    file.write(fb->buf, fb->len);
    // Serial.print("The picture has been saved in ");
    // Serial.print(FILE_PHOTO_PATH);
    // Serial.print(" - Size: ");
    // Serial.print(fb->len);
    // Serial.println(" bytes");
  }
  // Close the file
  file.close();
  esp_camera_fb_return(fb);
}

void initWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
}

void initLittleFS() {
  if (!LittleFS.begin(true)) {
    // Serial.println("An Error has occurred while mounting LittleFS");
    ESP.restart();
  } else {
    delay(500);
    // Serial.println("LittleFS mounted successfully");
  }
}

void initCamera() {
  // OV2640 camera module
  ledc_channel_config_t ledc_channel;
  ledc_channel.channel = LEDC_CHANNEL_0;
  ledc_channel.gpio_num = 18;  // Ganti dengan nomor GPIO yang diinginkan
  ledc_channel.speed_mode = LEDC_HIGH_SPEED_MODE;
  ledc_channel.timer_sel = LEDC_TIMER_0;
  ledc_channel.duty = 0;  // Nilai tugas awal
  ledc_channel.hpoint = 0;
  ledc_channel.intr_type = LEDC_INTR_DISABLE;
  ledc_channel_config(&ledc_channel);

  ledc_timer_config_t ledc_timer;
  ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;
  ledc_timer.duty_resolution = LEDC_TIMER_13_BIT;
  ledc_timer.timer_num = LEDC_TIMER_0;
  ledc_timer.freq_hz = 5000;  // Ganti dengan frekuensi yang diinginkan
  ledc_timer.clk_cfg = LEDC_AUTO_CLK;
  ledc_timer_config(&ledc_timer);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_1;
  config.ledc_timer = LEDC_TIMER_1;
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    // Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  esp_log_level_set("*", ESP_LOG_NONE);
  initWiFi();
  initLittleFS();
  // Turn-off the 'brownout detector'
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  initCamera();
  pinMode(pir, INPUT);
  pinMode(ldrDO, INPUT);
  pinMode(LedKuning1, OUTPUT);
  digitalWrite(LedKuning1, HIGH);
  pinMode(buzzerPin, OUTPUT);
  noTone(buzzerPin);  // Matikan buzzer

  // delay(1000);

  // Firebase
  configF.api_key = API_KEY;
  configF.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  configF.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  Firebase.begin(&configF, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  int statusPir = digitalRead(pir);
  if (statusPir == HIGH) {
    Firebase.RTDB.setString(&fbdo, "/sensor/pir", "Ada");
    Serial.println("Gerakan: Ada");
    // digitalWrite(LedGreen, HIGH);
    if (takeNewPhoto) {
      capturePhotoSaveLittleFS();
      takeNewPhoto = true;
    }
    if (Firebase.ready() && !taskCompleted) {
      // Serial.print("Uploading picture... ");
      if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID, FILE_PHOTO_PATH, mem_storage_type_flash, BUCKET_PHOTO, "image/jpeg", fcsUploadCallback)) {
        // Serial.printf("\nDownload URL: %s\n", fbdo.downloadURL().c_str());
      } else {
        // Serial.println(fbdo.errorReason());
      }
      taskCompleted = false;
    }

  } else {
    Firebase.RTDB.setString(&fbdo, "/sensor/pir", "Tidak Ada");
    Firebase.RTDB.setInt(&fbdo, "/ada_tikus", 0);
    Serial.println("Gerakan: Tidak Ada");
    // digitalWrite(LedGreen, HIGH);
    noTone(buzzerPin);  // Matikan buzzer
  }

  nilai = digitalRead(ldrDO);
  if (nilai == HIGH) {
    Firebase.RTDB.setString(&fbdo, "/sensor/ldr", "Gelap");
    Serial.println("Keadaan gelap");
    // digitalWrite(LedGreen, HIGH);

  } else {
    Firebase.RTDB.setString(&fbdo, "/sensor/ldr", "Terang");
    Serial.println("Keadaan terang");
    // digitalWrite(LedGreen, HIGH);
  }

  delay(1000);  // Menunggu 3 detik sebelum membaca lagi

  if (Firebase.ready()) {
    Firebase.RTDB.getInt(&fbdo, "/ada_tikus") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str();
    a = fbdo.to<int>();
    if (a == 1) {             // Hanya aktifkan buzzer jika belum menyala
      tone(buzzerPin, 2000);  // Frekuensi 5 kHz
      delay(1000);            // Mematikan buzzer;    // Mengatur status buzzer menjadi menyala
      Serial.println("Ada Tikus");
    } else {
      Serial.println("Tidak Ada Tikus");
      noTone(buzzerPin);  // Mengatur status buzzer menjadi mati
    }
    delay(3000);  // Menunggu 3 detik sebelum membaca lagi
  }

  if (delayPhoto != 30) {
    delayPhoto++;
  } else {
    if (takeNewPhoto) {
      capturePhotoSaveLittleFS();
      takeNewPhoto = true;
    }
    delay(100);
    if (Firebase.ready() && !taskCompleted) {
      // Serial.print("Uploading picture... ");
      if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID, FILE_PHOTO_PATH, mem_storage_type_flash, BUCKET_PHOTO, "image/jpeg", fcsUploadCallback)) {
        // Serial.printf("\nDownload URL: %s\n", fbdo.downloadURL().c_str());
      } else {
        // Serial.println(fbdo.errorReason());
      }
      taskCompleted = false;
    }
    delayPhoto = 1;
  }
}

// The Firebase Storage upload callback function
void fcsUploadCallback(FCS_UploadStatusInfo info) {
  if (info.status == firebase_fcs_upload_status_init) {
    // Serial.printf("Uploading file %s (%d) to %s\n", info.localFileName.c_str(), info.fileSize, info.remoteFileName.c_str());
  } else if (info.status == firebase_fcs_upload_status_upload) {
    // Serial.printf("Uploaded %d%s, Elapsed time %d ms\n", (int)info.progress, "%", info.elapsedTime);
  } else if (info.status == firebase_fcs_upload_status_complete) {
    // Serial.println("Upload completed\n");
    FileMetaInfo meta = fbdo.metaData();
    // Serial.printf("Name: %s\n", meta.name.c_str());
    // Serial.printf("Bucket: %s\n", meta.bucket.c_str());
    // Serial.printf("contentType: %s\n", meta.contentType.c_str());
    // Serial.printf("Size: %d\n", meta.size);
    // Serial.printf("Generation: %lu\n", meta.generation);
    // Serial.printf("Metageneration: %lu\n", meta.metageneration);
    // Serial.printf("ETag: %s\n", meta.etag.c_str());
    // Serial.printf("CRC32: %s\n", meta.crc32.c_str());
    // Serial.printf("Tokens: %s\n", meta.downloadTokens.c_str());
    // Serial.printf("Download URL: %s\n\n", fbdo.downloadURL().c_str());
  } else if (info.status == firebase_fcs_upload_status_error) {
    // Serial.printf("Upload failed, %s\n", info.errorMsg.c_str());
  }
}