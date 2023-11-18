// Import Library ต่าง ๆ ที่จำเป็น
#include <WiFi.h>
#include <HTTPClient.h>
#include <TridentTD_LineNotify.h>
#include "DHT.h"

// คำสั่งเริ่มต้นการเชื่อมต่อของ WiFi และ LINE Token
#define WIFI_S3333TA_NAME "Mi 10T Pro"
#define WIFI_STA_PASS "DoDo1234567890"
#define LINE_TOKEN  "U1Ne6DL3NqTDXFOWCxdrmeyNGvMC7QljE3FPOo4OHy3"

// คำสั่งเริ่มต้นของ Blynk
#define BLYNK_TEMPLATE_ID "TMPL6TAch-Tzu"
#define BLYNK_TEMPLATE_NAME "dht"
#define BLYNK_FIRMWARE_VERSION "0.1.0"
#define BLYNK_PRINT Serial
#define APP_DEBUG
#include "BlynkEdgent.h"

#define AOUT_PIN 32
#define RELAY_PIN 21
#define DHTPIN 5     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // DHT 22 (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);
WidgetLCD lcd_showmsg(V10);

String url_root = "https://script.google.com/macros/s/AKfycbzu-vj3U9QeT1tRYH1KxQWh9wbZRRguZSWnxawpRw8RvBzZs4oldbFouB4BCgM42QSC/exec";

int is_sent = 0;
int moi_value;
int isManualMode = 0;

unsigned long previousMillis = 0; //กำหนด Unsigned Long ชื่อว่า previousMillis = 0 เพื่อเอาไว้จับเวลาของ Arduino
const long interval = 10000; // แก้ระยะเวลาตรวจสอบทุก 10 วินาที

void setup() {
  Serial.begin(115200);
  pinMode(DHTPIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  wifi_conected();
  dht.begin();
  BlynkEdgent.begin();
  lcd_showmsg.clear();
  lcd_showmsg.print(3, 0, "PUMP CONTROL");
  delay(2000);
}

void loop() {
  BlynkEdgent.run();
  unsigned long currentMillis = millis(); 

  if (currentMillis - previousMillis >= interval) {
    DHT_Read();
    analog_moi();
    google_sheet();

    Serial.print("Mode 1.auto / 0.Manual  = ");
    Serial.print(isManualMode);
    Serial.println();

    previousMillis = currentMillis;
  }

  moi_value = analogRead(AOUT_PIN);
  float moi_value2 = PercenTeam(moi_value, 0, 4095, 0, 100);

  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();

  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, moi_value2);

  String messagePUMP;
  messagePUMP = messagePUMP + "ค่าความชื่น : " + humidity + " H\n";
  messagePUMP = messagePUMP + "อุณหภูมิ :  " + temp + " °C\n";
  messagePUMP = messagePUMP + "ค่าความชื่นในผ้า : " + moi_value2 + "%\n";

  if (isManualMode == 1) {
    if (moi_value2 >= 45 && is_sent == 0) {
      is_sent = 1;
      messagePUMP = messagePUMP + "PUMP = ON";
      LINE.notify(messagePUMP.c_str());
      digitalWrite(RELAY_PIN, HIGH);
      lcd_showmsg.clear();
      lcd_showmsg.print(4, 0, "AUTO FARM");
      lcd_showmsg.print(4, 1, "PUMP = ON");
    } else if (moi_value2 <= 46 && is_sent == 1) {
      is_sent = 0;
      messagePUMP = messagePUMP + "PUMP = OFF";
      LINE.notify(messagePUMP.c_str());
      digitalWrite(RELAY_PIN, LOW);
      lcd_showmsg.clear();
      lcd_showmsg.print(4, 0, "AUTO FARM");
      lcd_showmsg.print(4, 1, "PUMP = OFF");
    }
  }
}

BLYNK_WRITE(V3) {
  isManualMode = param.asInt();
  if (isManualMode == 1) {
    // โหมด auto
  } else if (isManualMode == 0) {
    digitalWrite(RELAY_PIN, LOW);
    is_sent = 0;
    lcd_showmsg.clear();
    lcd_showmsg.print(4, 0, "FARM HAO");
    lcd_showmsg.print(4, 1, "PUMP = OFF");
  }
}

BLYNK_WRITE(V4) {
  int pumpControl = param.asInt();
  if (isManualMode == 0) { // ถ้าอยู่ในโหมด Manual ถึงจะใช้งานได้
    if (pumpControl == 1) { 
      lcd_showmsg.clear();
      lcd_showmsg.print(4, 0, "MANUAL FARM");
      lcd_showmsg.print(4, 1, "PUMP = ON");
      digitalWrite(RELAY_PIN, HIGH);
    } else if (pumpControl == 0) {
      lcd_showmsg.clear();
      lcd_showmsg.print(4, 0, "MANUAL FARM");
      lcd_showmsg.print(4, 1, "PUMP = OFF");
      digitalWrite(RELAY_PIN, LOW);
    }
  }
}

void wifi_conected() {
  WiFi.begin(WIFI_S3333TA_NAME, WIFI_STA_PASS);
  Serial.printf("WiFi connecting to %s\n", WIFI_S3333TA_NAME);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(400);
  }
  Serial.printf("\nWiFi connected\nIP : ");
  Serial.println(WiFi.localIP());
  Serial.println(LINE.getVersion());
  LINE.setToken(LINE_TOKEN);
}

void DHT_Read() {
  float humidity = dht.readHumidity();
  float temp = dht.readTemperature();

  Serial.printf("Humidity = %.2f \n", humidity, " %");
  Serial.printf("Temperature = %.2f \n", temp, " %");
}

void analog_moi() {
  moi_value = analogRead(AOUT_PIN);
  float moi_value2 = PercenTeam(moi_value, 0, 4095, 0, 100);
  Serial.printf("Moisture value = %.2f %% \n", moi_value2);
  Serial.printf("------------------------------------------------------------- \n");
}

void google_sheet() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  moi_value = analogRead(AOUT_PIN);
  float moi_value2 = PercenTeam(moi_value, 0, 4095, 0, 100);
  String url = url_root + "?temp=" + t + "&humidity=" + h + "&FMoisture=" + moi_value2;
  HTTPClient http;
  http.begin(url.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();

  if (httpCode == 200 || httpCode == 201) {
    Serial.println("Import Data to Google Sheet Success !!");
  } else {
    Serial.println("Tempfail ; " + String(httpCode));
  }
}

long PercenTeam(int x, int in_min, int in_max, int out_min, int out_max) {
  int formula = in_max - in_min;
  if (formula == 0) {
    log_e("map(): Invalid input range, min == max");
    return 2;
  }
  int Alpha = out_max - out_min;
  int Bravo = x - in_min;
  return (Bravo * Alpha) / formula + out_min;
}