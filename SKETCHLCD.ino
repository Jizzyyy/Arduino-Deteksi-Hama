#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Inisialisasi alamat LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  // Inisialisasi LCD
  lcd.begin(0x27, 16, 2);
  lcd.backlight();  // Aktifkan pencahayaan belakang LCD
  lcd.clear();      // Bersihkan layar LCD
  Serial.begin(115200);
}

void loop() {
  // Tampilkan tulisan di LCD
  lcd.setCursor(0, 0);
  String coba = Serial.readStringUntil("\n");
  lcd.println(coba);
}
