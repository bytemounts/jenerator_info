#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "D300Controller.h"
#include <WebSocketsServer.h>
//gen'e bağlanacak nihai program ancak sadece şuan veri gönderme yapılabiliyor.
//.net'den veri alma kısmı şuan çalışmıyor.


// WiFi ayarları
const char* ssid = "Xiaomi12";
const char* password = "genc4326";
String serverUrl = "http://10.82.134.173:5156/api/generator/add"; 
WebSocketsServer webSocket = WebSocketsServer(81); // WebSocket sunucu portu

// D-300 MK3 kontrol nesnesi
#define RS485_RX_PIN 5
#define RS485_TX_PIN 15
D300Controller genset(1, RS485_RX_PIN, RS485_TX_PIN);

// Program ayarları
const unsigned long POST_INTERVAL = 1000;  // 1 saniyede bir gönder
const unsigned long CHECK_INTERVAL = 1000; // 1 saniyede bir kontrol
const int HTTP_TIMEOUT = 10000;
const int MAX_RETRY_COUNT = 3;

unsigned long lastPostTime = 0;
unsigned long lastCheckTime = 0;
bool wifiConnected = false;

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n========================================");
  Serial.println("    D-300 MK3 Jeneratör Kontrol Sistemi");
  Serial.println("    API Entegrasyonu");
  Serial.println("========================================");
  
  // WiFi bağlantısını başlat
  connectToWiFi();
  
  // D-300 MK3 bağlantısını başlat
  Serial.println("Jeneratöre bağlanılıyor...");
  
  if (genset.begin(9600, 3000)) {
    Serial.println("✅ D-300 MK3 bağlantısı başarılı!");
    Serial.println("Cihaz Kimlik: 0x" + String(genset.Sistem.CihazKimlik, HEX));
    
    // Otomatik güncellemeyi etkinleştir
    genset.enableAutoUpdate(true);
    
    Serial.println("Sistem başlatıldı. API veri gönderimi başlıyor...\n");
  } else {
    Serial.println("❌ D-300 MK3 bağlantı hatası!");
  }

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server başladı (port 81)"); 
  randomSeed(esp_random());
}

  void loop() {
    unsigned long currentTime = millis();
    
    // WiFi bağlantı kontrolü
    if (WiFi.status() != WL_CONNECTED) {
      if (wifiConnected) {
        Serial.println("⚠️ WiFi bağlantısı kesildi!");
        wifiConnected = false;
      }
      connectToWiFi();
    }
    
    // D-300 veri güncelleme
    genset.handle();
    
    // Sistem kontrolü
    if (currentTime - lastCheckTime >= CHECK_INTERVAL) {
      performSystemCheck();
      lastCheckTime = currentTime;
    }
    
    // API veri gönderimi
    if (currentTime - lastPostTime >= POST_INTERVAL) {
      if (wifiConnected && genset.isConnected()) {
        sendGeneratorData();
      } else {
        Serial.println("⚠️ Veri gönderilemiyor: " + 
          String(!wifiConnected ? "WiFi yok" : "Jeneratör bağlantısı yok"));
      }
      lastPostTime = currentTime;
    }
    webSocket.loop();
    delay(100);
  }

void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiConnected) {
      Serial.println("✅ WiFi bağlandı! IP: " + WiFi.localIP().toString());
      wifiConnected = true;
    }
    return;
  }
  
  WiFi.begin(ssid, password);
  Serial.print("WiFi'ye bağlanıyor");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(250);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✅ WiFi bağlandı! IP: " + WiFi.localIP().toString());
    wifiConnected = true;
  } else {
    Serial.println("❌ WiFi bağlanamadı");
    wifiConnected = false;
  }
}

void performSystemCheck() {
  // Bağlantı kontrolü
  if (!genset.isConnected()) {
    Serial.println("⚠️ Jeneratör bağlantısı kesildi! Yeniden bağlanmaya çalışılıyor...");
    return;
  }
  
  // Kritik alarm kontrolü
  if (genset.isAlarmActive(AlarmTipi::Kapatma)) {
    Serial.println("🚨 KRİTİK ALARM: KAPATMA ALARMI AKTİF!");
  }
}

void sendGeneratorData() {
  
  String json = buildRealJson();
  
  bool success = httpPostSafe(json);
  
  if (success) {
    Serial.println("✅ Veri başarıyla gönderildi");
  } else {
    Serial.println("❌ Veri gönderilemedi");
  }
}

String buildRealJson() {
  // D-300'den güncel verileri al
  genset.updateBasicData();
  
  StaticJsonDocument<1024> doc; // JSON boyutunu artırdık
  
  // Temel durum bilgileri
  doc["CalismaDurumu"] = genset.getDurumAciklama();
  doc["OperationMode"] = static_cast<int>(genset.getUniteModu());
  doc["SistemCalismaSuresi"] = genset.Sayac.MotorCalismaSaati * 3600; // Saati saniyeye çevir
  
  // Şebeke verileri
  doc["SebekeVoltaj_l1"] = genset.ElektrikSistemi.Sebeke.L1.Voltaj;
  doc["SebekeVoltaj_l2"] = genset.ElektrikSistemi.Sebeke.L2.Voltaj;
  doc["SebekeVoltaj_l3"] = genset.ElektrikSistemi.Sebeke.L3.Voltaj;
  doc["SebekeHz"] = genset.getSebekeFrekans();
  doc["ToplamGuc"] = genset.ElektrikSistemi.Sebeke.Toplam.AktifGuc * 1000; // kW'tan W'a çevir
  doc["SebekeDurumu"] = genset.isSebekeMevcut();
  
  // Jeneratör verileri
  doc["GenVoltaj_l1"] = genset.ElektrikSistemi.Jenerator.L1.Voltaj;
  doc["GenVoltaj_l2"] = genset.ElektrikSistemi.Jenerator.L2.Voltaj;
  doc["GenVoltaj_l3"] = genset.ElektrikSistemi.Jenerator.L3.Voltaj;
  doc["GenHz"] = genset.getJeneratorFrekans();
  doc["GenUretilenGuc"] = genset.getJeneratorGuc() * 1000; // kW'tan W'a çevir
  
  // Güç faktörü hesaplama
  float gorunurGuc = sqrt(pow(genset.ElektrikSistemi.Jenerator.Toplam.AktifGuc, 2) + 
                         pow(genset.ElektrikSistemi.Jenerator.Toplam.ReaktifGuc, 2));
  float gucFaktoru = (gorunurGuc > 0) ? (genset.ElektrikSistemi.Jenerator.Toplam.AktifGuc / gorunurGuc) * 100 : 0;
  doc["GenGucFaktoru"] = gucFaktoru;
  
  // Motor verileri
  doc["MotorRpm"] = genset.getMotorRPM();
  doc["MotorSicaklik"] = genset.Motor.Sicaklik;
  doc["YagBasinci"] = genset.Motor.YagBasinci;
  doc["YakitSeviyesi"] = genset.getYakitSeviyesi();
  doc["BataryaVoltaji"] = genset.getBataryaVoltaji();
  doc["timestamp"] = millis();
  
  // Alarm durumu (opsiyonel - API'de ihtiyaç varsa)
  doc["KapatmaAlarmi"] = genset.isAlarmActive(AlarmTipi::Kapatma);
  doc["YukAtmaAlarmi"] = genset.isAlarmActive(AlarmTipi::YukAtma);
  doc["UyariAlarmi"] = genset.isAlarmActive(AlarmTipi::Uyari);
  doc["SistemSaglikli"] = genset.isSystemHealthy();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Debug için JSON'u yazdır
  //Serial.println("📄 Gönderilen JSON:");
  //Serial.println(jsonString);
  
  return jsonString;
}

bool httpPostSafe(const String &payload) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi bağlı değil, POST atılmadı.");
    return false;
  }

  HTTPClient http;
  WiFiClient client;
  http.setTimeout(HTTP_TIMEOUT);
  bool success = false;

  if (!http.begin(client, serverUrl)) {
    Serial.println("❌ http.begin() başarısız!");
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent", "ESP32-D300-Generator");

  int retry = 0;
  while (retry < MAX_RETRY_COUNT) {
    //Serial.printf("🔄 POST denemesi %d, payload length=%u\n", retry+1, payload.length());
    
    int httpCode = http.POST(payload);
    
    if (httpCode > 0) {
      //Serial.printf("📨 HTTP cevap kodu: %d\n", httpCode);
      String response = http.getString();
      //Serial.println("📨 Sunucu cevabı: " + response);
      
      success = (httpCode >= 200 && httpCode < 300);
      break;
    } else {
      Serial.printf("❌ HTTP POST hatası: %d -> %s\n", httpCode, http.errorToString(httpCode).c_str());
      retry++;
      delay(1000);
    }
  }

  http.end();
  return success;
}

// WebSocket mesajları yakalama
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    String msg = String((char*)payload);
    Serial.print("Gelen mesaj: ");
    Serial.println(msg);

    if (msg == "start") start_gen();
    else if (msg == "stop") stop_gen();
    else if(msg == "auto") auto_gen();
    else if(msg == "manuel") manuel_gen();
    else if(msg == "test") test_gen();
    else if(msg=="emergency") acilstop_gen();
  }
}

void start_gen() {
  genset.startGenerator();
  Serial.println("başlama moduna geçildi!");
}
void stop_gen() {
  genset.stopGenerator();
  Serial.println("durdurma moduna geçildi!");
}
void auto_gen(){
  genset.setAutoMode();
  Serial.println("auto mod geçildi!");
}
void manuel_gen(){
  genset.setManualMode();
  Serial.println("manuel mod geçildi!");
}
void test_gen(){
  genset.setTestMode();
  Serial.println("test mod geçildi!");
}
void acilstop_gen(){
  genset.emergencyStop();
  Serial.println("acilstop moduna geçildi!");
}