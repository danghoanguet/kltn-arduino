#include "ESP8266WiFi.h"
// #include <FirebaseESP8266.h>
#include <Firebase_ESP_Client.h>

#include "DHT.h"
#include <ArduinoJson.h>

#define DATABASE_URL "https://kltn-hethongtuoitieu-87ff7-default-rtdb.asia-southeast1.firebasedatabase.app/"  // địa chỉ tên dự án từ firebase id
#define API_KEY "AIzaSyAnOmt_-fgq0fwABrfvyN0HQo-Yi3ZcB04"                                                     //mã bí mật của project từ firebase cung cấp

//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;

FirebaseConfig config;

#define WIFI_SSID "MinhHoang"     // tên wifi
#define WIFI_PASSWORD "11111111"  // mật khẩu wifi
// #define WIFI_SSID "Phong Khach"     // tên wifi
// #define WIFI_PASSWORD "hung190969"  // mật khẩu wifi

int OutPin = A0;  //Lưu chân ra của cảm biến độ ẩm đất

#define DHTPIN D3      //Lưu chân ra của cảm biến nhiệt độ độ ẩm
#define DHTTYPE DHT22  //chọn loại dht DHT22
DHT dht(DHTPIN, DHTTYPE);

#define RelayBom D5   //Lưu chân vào của relay
#define RelayQuat D6  //Lưu chân vào của relay
#define RelayPhun D7  //Lưu chân vào của relay
#define RelaySuong D8 //Lưu chân vào của relay suong
unsigned long sendDataPrevMillis = 0;

void setup() {
  Serial.begin(9600);
  delay(1000);
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  myNetwork();  //try to connect with wifi

  dht.begin();  //Đọc giá trị DHT độ ẩm, nhiệt độ


  pinMode(RelayQuat, OUTPUT);
  digitalWrite(RelayQuat, LOW);
  pinMode(RelayBom, OUTPUT);
  digitalWrite(RelayBom, HIGH );
  pinMode(RelayPhun, OUTPUT);
  digitalWrite(RelayPhun, LOW);
  pinMode(RelaySuong, OUTPUT);
  digitalWrite(RelaySuong, LOW);
}
void myNetwork() {
  Serial.println("Searching Wifi");

  int numberOfNetworks = WiFi.scanNetworks();

  if (Firebase.ready()) {
    for (int i = 0; i < numberOfNetworks; i++) {
      Serial.println(WiFi.SSID(i));
      if (WiFi.SSID(i) == WIFI_SSID) {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        Serial.println("-------------Wifi found-------------");
        Serial.print("-------------Reconnecting to ");
        Serial.println(WIFI_SSID);
        delay(6000);

        break;
      }
    }
  } else {
    for (int i = 0; i < numberOfNetworks; i++) {
      Serial.println(WiFi.SSID(i));
      if (WiFi.SSID(i) == WIFI_SSID) {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        Serial.println("-------------Wifi found-------------");
        Serial.print("-------------Connecting to ");
        Serial.println(WIFI_SSID);
        delay(6000);

        auth.user.email = "dmhoang8420@gmail.com";
        auth.user.password = "hoang8420@";
        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);

        Firebase.RTDB.setString(&fbdo, "/Wifi/IP", WiFi.localIP().toString());
        Firebase.RTDB.setString(&fbdo, "/Wifi/Name", String(WIFI_SSID));
        Firebase.RTDB.setString(&fbdo, "/Wifi/Password", String(WIFI_PASSWORD));
        Firebase.RTDB.setString(&fbdo, "/Wifi/Status", "Connected");
        break;
      }
    }
  }
}
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("No internet");
    myNetwork();
  } else {
    Serial.println("-------------Internet Access-------------");
    String vpdMin = "0";
    String vpdMax = "0";
    if (Firebase.RTDB.getString(&fbdo, "/Threshold/vpdMin")) {
      Serial.print("Firebase - get threshold - vpdMin: ");
      vpdMin = fbdo.to<String>();
      Serial.println(vpdMin);
    } else {
      Serial.println("get /Threshold/vpdMin failed");
      return;
    }
    //thresholdHum = Firebase.getString(firebaseData, "/Threshold/HUM");
    if (Firebase.RTDB.getString(&fbdo, "/Threshold/vpdMax")) {
      Serial.print("Firebase - get threshold - vpdMax: ");
      vpdMax = fbdo.to<String>();
      Serial.println(vpdMax);

    } else {
      Serial.println("get /Threshold/vpdMax failed");
      return;
    }

    // float h = round(dht.readHumidity());     // Reading temperature or humidity takes about 250 milliseconds!
    // float t = round(dht.readTemperature());  // Read temperature as Celsius(the default)
    float doc_cb_h = 0;
    float doc_cb_t = 0;

    for (int i = 0; i < 10; i++) {
      doc_cb_h += dht.readHumidity();
      doc_cb_t += dht.readTemperature();
    }
    float h = doc_cb_h / 10;  // Reading temperature or humidity takes about 250 milliseconds!
    float t = doc_cb_t / 10;  // Read temperature as Celsius(the default)

    if (isnan(h) || isnan(t)) {  // Check if any reads failed and exit early (to try again)
      Serial.println(F(" Failed to read from DHT sensor!"));
      Firebase.RTDB.setString(&fbdo, "/DHT/Nhiệt Độ", "0");
      Firebase.RTDB.setString(&fbdo, "/DHT/Độ Ẩm", "0");
      return;
    }

    double vpd = calculateVPD(t, h);
    Serial.print("VPD: ");
    Serial.println(vpd);
    if (vpd < vpdMin.toFloat()) {
      Serial.println("Quat: ");
      digitalWrite(RelayQuat, HIGH);
      digitalWrite(RelayPhun, LOW);
      digitalWrite(RelaySuong, LOW);

    } else if (vpd > vpdMax.toFloat()) {
      Serial.println("Phun: ");
      digitalWrite(RelayPhun, HIGH);
      digitalWrite(RelaySuong, HIGH);

      digitalWrite(RelayQuat, LOW);
    } else {
      Serial.println("Khong: ");
      digitalWrite(RelayQuat, LOW);
      digitalWrite(RelayPhun, LOW);
      digitalWrite(RelaySuong, LOW);
    }

    //Đọc giá trị từ cảm biến đo độ ẩm đất
    int doc_cb = 0;
    for (int i = 0; i < 10; i++) {
      doc_cb += analogRead(OutPin);
    }

    int s = doc_cb / 10;  // Ta sẽ đọc giá trị hiệu điện thế của cảm biến độ ẩm đất giá trị
    Serial.print("Soil sensor analog value: ");
    Serial.println(s);
    // Giá trị được số hóa thành 1 số nguyên có
    // trong khoảng từ 0 đến 1023

    if (isnan(s)) {  // Check if any reads failed and exit early (to try again)
      Serial.println(F("Failed to read from soil moisture sensor!"));
      Firebase.RTDB.setString(&fbdo, "/DHT/Độ Ẩm Đất", "0");
      return;
    }

    int s_percent_before = map(s, 0, 1024, 0, 100);
    int s_percent = 100 - s_percent_before;
    Serial.print("Soil moisture: ");
    Serial.print(s_percent);
    Serial.print("%      ");
    String fireSoilMoi = String(s_percent) + String("%");  //convert integer humidity to string humidity


    Serial.print("Humidity: ");
    Serial.print(h);
    String fireHumid = String(h) + String("%");  //convert integer humidity to string humidity
    Serial.print("%      Temperature: ");
    Serial.print(t);
    Serial.println(" C");
    String fireTemp = String(t) + String(" C");  //convert integer temperature to string temperature

    //Đẩy giá trị đo được lên Firebase
    String ss = String(s_percent);
    String hh = String(h);
    String tt = String(t);
    Firebase.RTDB.setString(&fbdo, "/DHT/Độ Ẩm Đất", ss);
    Firebase.RTDB.setString(&fbdo, "/DHT/Độ Ẩm", hh);
    Firebase.RTDB.setString(&fbdo, "/DHT/Nhiệt Độ", tt);

    ////////////////////////////////////////////////////
    String myPump = "0";
    String myPumpMode = "0";  //1 = manual; 0 = auto
    String thresholdTemp = "0";
    String thresholdHum = "0";
    String thresholdSoil = "0";

    int myPumpValue = 1024;

    if (Firebase.RTDB.getString(&fbdo, "/CONTROL/Manual")) {
      Serial.print("Firebase - get PUMP - Manual: ");
      myPumpMode = fbdo.to<String>();
      Serial.println(myPumpMode);
      //Check chế độ bơm thủ công hay tự động
      if (myPumpMode == "1") {
        Serial.println("- Bat tat bom thu cong");

        if (Firebase.RTDB.getString(&fbdo, "/CONTROL/State")) {
          myPump = fbdo.to<String>();
          if (myPump != "0")  // Pump On
          {
            Serial.println("- Bat may bom bang tay");
            digitalWrite(RelayBom, LOW);
            delay(3000);  // Tưới trong 3s
            digitalWrite(RelayBom, HIGH);
            Firebase.RTDB.setString(&fbdo, "/CONTROL/State", "0");
          } else  // Pump Off
          {
            Serial.println("- Tat may bom bang tay");
            digitalWrite(RelayBom, HIGH);
          }
          Serial.println("-------------Ket thuc 1 chu ki-------------");
          unsigned long time = millis() - sendDataPrevMillis;
          Serial.print("Thoi gian chay het 1 chu ki: ");
          Serial.println(time);
          sendDataPrevMillis = millis();
          delay(5000);
        } else {
          Serial.println("get /CONTROL/State failed");
          return;
        }
        return;
      } else {
        //thresholdTemp = Firebase.getString(firebaseData, "/Threshold/TEMP");

        //thresholdSoil = Firebase.getString(firebaseData, "/Threshold/SOIL");
        if (Firebase.RTDB.getString(&fbdo, "/Threshold/SOIL")) {
          Serial.print("Firebase - get threshold - Soil: ");
          thresholdSoil = fbdo.to<String>();
          Serial.println(thresholdSoil);
        } else {
          Serial.println("get /Threshold/SOIL failed");
          return;
        }

        BomTuDong(t, h, s_percent, vpdMin.toFloat(), vpdMax.toFloat(), thresholdSoil.toInt());
        Serial.println("-------------Ket thuc 1 chu ki-------------");
        unsigned long time = millis() - sendDataPrevMillis;
        Serial.print("Thoi gian chay het 1 chu ki: ");
        Serial.println(time);
        sendDataPrevMillis = millis();
        delay(5000);
      }
      return;

    } else {
      Serial.println("get /CONTROL/Manual failed");
      Serial.println("REASON: " + fbdo.errorReason());

      return;
    }
  }
}

// FirebaseData pumpData;
void BomTuDong(float temp, float hum, int soil, float vpdMin, float vpdMax, int _thresholdSoil) {
  // digitalWrite(Relay, HIGH);
  double vpd = calculateVPD(temp, hum);
  Serial.print("VPD: ");
  Serial.println(vpd);
  if (vpdMin <= vpd && vpd <= vpdMax) {
    if (soil < _thresholdSoil) {
      Firebase.RTDB.setString(&fbdo, "/CONTROL/State", "100");
      Serial.println("Function BomTuDong()1 - Bat may bom tu dong");

      digitalWrite(RelayBom, LOW);
      delay(3000);
      digitalWrite(RelayBom, HIGH);
      Firebase.RTDB.setString(&fbdo, "/CONTROL/State", "0");
      Serial.println("Function BomTuDong() - Tat may bom tu dong");
    } else {
      Serial.println("void BomTuDong() - Tat may bom tu dong");
      digitalWrite(RelayBom, HIGH);
      Firebase.RTDB.setString(&fbdo, "/CONTROL/State", "0");
    }
  } else {
    if (vpd < vpdMin && soil < _thresholdSoil || vpd > vpdMax) {
      Firebase.RTDB.setString(&fbdo, "/CONTROL/State", "100");
      Serial.println("Function BomTuDong()2 - Bat may bom tu dong");
      digitalWrite(RelayBom, LOW);
      delay(3000);
      digitalWrite(RelayBom, HIGH);
      Firebase.RTDB.setString(&fbdo, "/CONTROL/State", "0");
      Serial.println("Function BomTuDong() - Tat may bom tu dong");
    } else {
      Serial.println("void BomTuDong() - Tat may bom tu dong");
      digitalWrite(RelayBom, HIGH);
      Firebase.RTDB.setString(&fbdo, "/CONTROL/State", "0");
    }
  }
}

double calculateVPD(double temp, double rh) {
  return ((610.7 * pow(10, (7.5 * temp / (237.3 + temp)))) / 1000 * (1 - rh / 100));
}