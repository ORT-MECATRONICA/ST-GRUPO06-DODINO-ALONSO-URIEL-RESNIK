  //Grupo 6 Lara Alonso, Luca Resnik, Ezequiel Dodino y Uriel Laufer
#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>


// Pines
#define BTN1 35
#define BTN2 34
#define SENSOR_TEMP 23
#define LED 25

// Variables
float temperatura;
unsigned long Time = 0;
int Intervalo;
int Intervalito;



DHT dht(SENSOR_TEMP, DHT11);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

int Maquinola;
enum Estados { //enumera est.
  Inicio,
  Espera1,
  Tiempo,
  suma,
  resta,
  Espera2
};

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials datos firease
#define WIFI_SSID "ORT-IoT"
#define WIFI_PASSWORD "OrtIOT24"

// Insert Firebase project API Key
#define API_KEY "AIzaSyCBIb2rAkr4gkCL0NOmWRUOKB1wbVBhltw"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "ezedodi@gmail.com"
#define USER_PASSWORD "123456"

// Insert RTDB URL
#define DATABASE_URL "https://tp-firebase-ezd-default-rtdb.firebaseio.com/"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String tempPath = "/temperature";


// Parent Node (to be updated in every loop)
String parentPath;

int timestamp;
FirebaseJson json;

const char* ntpServer = "pool.ntp.org";

unsigned long sendDataPrevMillis = 0;

//wifi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return 0;
  }
  time(&now);
  return now;
}

void setup() {
  Serial.begin(115200);
  initWiFi();
  configTime(0, 0, ntpServer);

  // Assign the API key (required)
  config.api_key = API_KEY;

  // Assign the user sign-in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long-running token generation task
  config.token_status_callback = tokenStatusCallback;  // see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authentication and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";

  // Initial setup
  Intervalo = 30000;
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  u8g2.begin();
  dht.begin();
  Maquinola = Inicio;
}

void loop() {
  Serial.println(millis() - sendDataPrevMillis); // el tiempo del monitor que seteas en el estado tiempo
  Intervalito = Intervalo / 1000;

  if (Firebase.ready() && (millis() - sendDataPrevMillis > Intervalo || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    // Get current timestamp
    timestamp = getTime();
    Serial.print("time: ");
    Serial.println(timestamp);

    parentPath = databasePath + "/" + String(timestamp);


    json.set(tempPath.c_str(), dht.readTemperature());// lee dht
    Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json);// manda valor
  }

  switch (Maquinola) {
    case Inicio:
      Pantalla_INIT();
      if (digitalRead(BTN1) == LOW && digitalRead(BTN2) == LOW)
        Maquinola = Espera1;
      break;
    case Espera1:
      Pantalla_INIT();
      if (digitalRead(BTN1) == HIGH && digitalRead(BTN2) == HIGH) {
        Maquinola = Tiempo;
      }
      break;
    case Tiempo:
      Pantalla_Tiempr();
      if (digitalRead(BTN1) == LOW) {
        Maquinola = suma;
      }
      if (digitalRead(BTN2) == LOW) {
        Maquinola = resta;
      }
      if (digitalRead(BTN1) == LOW && digitalRead(BTN2) == LOW) {
        Maquinola = Espera2;
      }

      break;
    case suma:
      Pantalla_Tiempr();
      if (digitalRead(BTN1) == HIGH) {
        Intervalo = Intervalo + 30000;
        Maquinola = Tiempo;
      }
      if (digitalRead(BTN1) == LOW && digitalRead(BTN2) == LOW) {
        Maquinola = Espera2;
      }
      break;
    case resta:
      Pantalla_Tiempr();
      if (digitalRead(BTN2) == HIGH) {
        Maquinola = Tiempo;
        if (Intervalo > 30000) {
          Intervalo = Intervalo - 30000;
        }
      }

      if (digitalRead(BTN1) == LOW && digitalRead(BTN2) == LOW) {
        Maquinola = Espera2;
      }
      break;
    case Espera2:
      Pantalla_Tiempr();
      if (digitalRead(BTN1) == HIGH && digitalRead(BTN2) == HIGH) {
        Maquinola = Inicio;
      }
      break;
  }
}

void Pantalla_Tiempr() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.setCursor(18, 30);
  u8g2.print(Intervalito);
  u8g2.drawStr(55, 30, "Time");
  u8g2.drawStr(15, 50, "-");
  u8g2.drawStr(100, 50, "+");

  u8g2.sendBuffer();
}

void Pantalla_INIT() {
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_ncenB12_tr);
  u8g2.drawStr(0, 15, "TP FIREBASE");

  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 30, "Temperatura: ");
  u8g2.setCursor(80, 30);
  temperatura = dht.readTemperature();  // Actualiza la temperatura
  u8g2.print(temperatura);
  u8g2.drawStr(110, 30, "°C");

  u8g2.sendBuffer();
}
