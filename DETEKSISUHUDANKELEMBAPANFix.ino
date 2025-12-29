#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ================= PIN SETUP =================
#define DHTPIN 19
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define MQ2_AO 23
#define MQ2_DO 18

#define LED_HIJAU 25
#define LED_KUNING 15
#define LED_MERAH 33
#define BUZZER 26

// SENSOR DEBU GP2Y
#define DUST_WHITE 34
#define DUST_BLACK 35

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= THRESHOLD =================
int asapThreshold = 300;
float suhuThreshold = 34.0;
int debuThreshold = 1800;

// ================= WIFI =================
const char* ssid = "A";
const char* password = "12345678";

// ================= THINGSPEAK =================
unsigned long channelID = 3190391;
String writeAPIKey = "LNL46ZKEO4RGUP5M";
const char* server = "https://api.thingspeak.com/update";

// =================================================
void setup() {
  Serial.begin(115200);
  dht.begin();

  // I2C ESP32
  Wire.begin(17, 22);

  // LCD Init
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(1500);
  lcd.clear();

  // Pin Mode
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(LED_KUNING, OUTPUT);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  pinMode(MQ2_DO, INPUT);
  pinMode(DUST_WHITE, INPUT);
  pinMode(DUST_BLACK, INPUT);

  digitalWrite(LED_HIJAU, HIGH);
  digitalWrite(LED_KUNING, LOW);
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(BUZZER, LOW);

  // WiFi Connect
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
}

// =================================================
void loop() {

  // Reconnect WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    WiFi.reconnect();
    delay(3000);
  }

  // Read Sensor
  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();
  int asapAnalog = analogRead(MQ2_AO);
  int asapDigital = digitalRead(MQ2_DO);
  int debuWhite = analogRead(DUST_WHITE);
  int debuBlack = analogRead(DUST_BLACK);

  if (isnan(suhu) || isnan(kelembapan)) {
    Serial.println("DHT Sensor Error");
    return;
  }

  // Serial Monitor
  Serial.print("Suhu: "); Serial.print(suhu);
  Serial.print(" | Hum: "); Serial.print(kelembapan);
  Serial.print(" | MQ2 AO: "); Serial.print(asapAnalog);
  Serial.print(" | MQ2 DO: "); Serial.print(asapDigital);
  Serial.print(" | DebuW: "); Serial.print(debuWhite);
  Serial.print(" | DebuB: "); Serial.println(debuBlack);

  // LCD Display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("S:");
  lcd.print(suhu);
  lcd.print(" H:");
  lcd.print(kelembapan);

  lcd.setCursor(0, 1);
  lcd.print("DW:");
  lcd.print(debuWhite);
  lcd.print(" DB:");
  lcd.print(debuBlack);

  // Logika Kualitas Udara
  bool kondisiBuruk = false;

  if (suhu > suhuThreshold) kondisiBuruk = true;
  if (asapAnalog > asapThreshold) kondisiBuruk = true;
  if (asapDigital == LOW) kondisiBuruk = true;
  if (debuWhite > debuThreshold) kondisiBuruk = true;

  // Output Indikator
  if (kondisiBuruk) {
    digitalWrite(LED_HIJAU, LOW);
    digitalWrite(LED_KUNING, HIGH);
    digitalWrite(LED_MERAH, HIGH);

    digitalWrite(BUZZER, HIGH);
    delay(300);
    digitalWrite(BUZZER, LOW);

    Serial.println("Kualitas Udara BURUK");
  } else {
    digitalWrite(LED_HIJAU, HIGH);
    digitalWrite(LED_KUNING, LOW);
    digitalWrite(LED_MERAH, LOW);
    digitalWrite(BUZZER, LOW);

    Serial.println("Kualitas Udara BAIK");
  }

  // Kirim ke ThingSpeak
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(server) +
      "?api_key=" + writeAPIKey +
      "&field1=" + String(suhu) +
      "&field2=" + String(kelembapan) +
      "&field3=" + String(asapAnalog) +
      "&field4=" + String(asapDigital) +
      "&field5=" + String(debuWhite) +
      "&field6=" + String(debuBlack);

    http.begin(url);
    http.GET();
    http.end();
  }

  delay(20000); // interval kirim data
}
