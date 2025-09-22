/*
 * main.ino
 * D-300 MK3 Jeneratör Kontrol Sistemi - Ana Program
 * ESP32 + RS485 TTL Auto-Flow Control Module
 * 
 * Dosya yapısı:
 * - main.ino           (bu dosya)
 * - D300Controller.h   (header dosyası)
 * - D300Controller.cpp (implementation dosyası)
 * 
 * Bağlantı:
 * ESP32 GPIO16 -> RS485 RXD
 * ESP32 GPIO17 -> RS485 TXD
 * ESP32 3.3V   -> RS485 VCC
 * ESP32 GND    -> RS485 GND
 * RS485 A+     -> D-300 A+
 * RS485 B-     -> D-300 B-
 */

#include "D300Controller.h"

// D-300 MK3 controller nesnesi
// Parametreler: slaveID, rxPin, txPin
#define RS485_RX_PIN 5
#define RS485_TX_PIN 15
D300Controller genset(1, RS485_RX_PIN, RS485_TX_PIN);

// Program ayarları
const unsigned long PRINT_INTERVAL = 5000;  // 5 saniye aralıklarla yazdır
const unsigned long CHECK_INTERVAL = 1000;  // 1 saniye aralıklarla kontrol et
unsigned long lastPrintTime = 0;
unsigned long lastCheckTime = 0;

void setup() {
  Serial.begin(115200);
  
  Serial.println("\n========================================");
  Serial.println("    D-300 MK3 Jeneratör Kontrol");
  Serial.println("    ESP32 + RS485 TTL Auto-Flow");
  Serial.println("========================================");
  
  // D-300 MK3 bağlantısını başlat
  Serial.println("Jeneratöre bağlanılıyor...");
  
  if (genset.begin(9600, 3000)) {  // 9600 baud, 3 saniye güncelleme aralığı
    Serial.println("✅ D-300 MK3 bağlantısı başarılı!");
    Serial.println("Cihaz Kimlik: 0x" + String(genset.Sistem.CihazKimlik, HEX));
    Serial.println("Donanım Versiyon: " + String(genset.Sistem.DonanımVersiyon));
    Serial.println("Yazılım Versiyon: " + String(genset.Sistem.YazilimVersiyon));
    
    // Otomatik güncellemeyi etkinleştir
    genset.enableAutoUpdate(true);
    
    Serial.println("Program başlatıldı. Veri akışı başlıyor...\n");
  } else {
    Serial.println("❌ D-300 MK3 bağlantı hatası!");
    Serial.println("Lütfen bağlantıları kontrol edin:");
    Serial.println("- RS485 modülü ESP32'ye doğru bağlı mı?");
    Serial.println("- A+/B- kabloları D-300'e doğru bağlı mı?");
    Serial.println("- D-300 açık ve Modbus etkin mi?");
    Serial.println("- Slave ID doğru mu? (varsayılan: 1)");
    Serial.println("\nYeniden başlatmayı deneyin...");
  }
}

void loop() {
  unsigned long currentTime = millis();
  
  // Otomatik veri güncelleme (D300Controller sınıfı tarafından yönetilir)
  genset.handle();
  
  // Periyodik sistem kontrolü
  if (currentTime - lastCheckTime >= CHECK_INTERVAL) {
    performSystemCheck();
    lastCheckTime = currentTime;
  }
  
  // Periyodik veri yazdırma
  if (currentTime - lastPrintTime >= PRINT_INTERVAL) {
    printSystemStatus();
    lastPrintTime = currentTime;
  }
  
  // Serial komutları için kontrol
  handleSerialCommands();
  
  delay(100); // Ana döngü gecikmesi
}

void performSystemCheck() {
  // Bağlantı kontrolü
  if (!genset.isConnected()) {
    Serial.println("⚠️ Bağlantı kesildi! Yeniden bağlanmaya çalışılıyor...");
    return;
  }
  
  // Kritik alarm kontrolü
  if (genset.isAlarmActive(AlarmTipi::Kapatma)) {
    Serial.println("🚨 KRİTİK ALARM: KAPATMA ALARMI AKTİF!");
    // Kritik durumlarda acil durdurma yapılabilir
    // genset.emergencyStop();
  }
  
  // Yük atma alarmı kontrolü
  if (genset.isAlarmActive(AlarmTipi::YukAtma)) {
    Serial.println("⚠️ YÜK ATMA ALARMI AKTİF!");
  }
  
  // Düşük yakıt kontrolü
  if (genset.getYakitSeviyesi() < 20.0 && genset.getYakitSeviyesi() > 0) {
    Serial.println("⛽ DÜŞÜK YAKIT UYARISI: %" + String(genset.getYakitSeviyesi(), 1));
  }
  
  // Düşük batarya kontrolü
  if (genset.getBataryaVoltaji() < 11.0 && genset.getBataryaVoltaji() > 0) {
    Serial.println("🔋 DÜŞÜK BATARYA UYARISI: " + String(genset.getBataryaVoltaji(), 1) + "V");
  }
  
  // Yüksek sıcaklık kontrolü
  if (genset.Motor.Sicaklik > 90.0 && genset.isJeneratorCalisir()) {
    Serial.println("🌡️ YÜKSEK MOTOR SICAKLIĞI: " + String(genset.Motor.Sicaklik, 1) + "°C");
  }
  
  // Şebeke durumu kontrolü
  static bool lastMainsStatus = false;
  bool currentMainsStatus = genset.isSebekeMevcut();
  
  if (currentMainsStatus != lastMainsStatus) {
    if (currentMainsStatus) {
      Serial.println("🔌 ŞEBEKEYİ DÖNDÜ - Jeneratör otomatik modda durabilir");
    } else {
      Serial.println("🔌 ŞEBEKE KESİLDİ - Jeneratör otomatik modda başlayabilir");
    }
    lastMainsStatus = currentMainsStatus;
  }
}

void printSystemStatus() {
  if (!genset.isConnected()) {
    Serial.println("❌ Jeneratöre bağlantı yok!");
    return;
  }
  
  Serial.println("\n" + String("=").substring(0, 50));
  Serial.println("D-300 MK3 DURUM RAPORU - " + String(millis() / 1000) + "s");
  Serial.println(String("=").substring(0, 50));
  
  // Temel durum bilgileri
  Serial.printf("🔧 Durum: %s | Mod: %s\n", 
    genset.getDurumAciklama().c_str(), genset.getModAciklama().c_str());
  
  // Elektriksel değerler
  Serial.println("\n📊 ELEKTRİKSEL VERİLER:");
  Serial.printf("   Şebeke  : %.1fV, %.1fHz, %.1fkW\n", 
    genset.ElektrikSistemi.Sebeke.Toplam.OrtalamaVoltaj,
    genset.getSebekeFrekans(), 
    genset.ElektrikSistemi.Sebeke.Toplam.AktifGuc);
  
  Serial.printf("   Jeneratör: %.1fV, %.1fHz, %.1fkW\n", 
    genset.ElektrikSistemi.Jenerator.Toplam.OrtalamaVoltaj,
    genset.getJeneratorFrekans(), 
    genset.getJeneratorGuc());
  
  // Motor verileri
  Serial.println("\n🔩 MOTOR VERİLERİ:");
  Serial.printf("   RPM: %.0f | Sıcaklık: %.1f°C | Yağ: %.1f bar\n", 
    genset.getMotorRPM(), genset.Motor.Sicaklik, genset.Motor.YagBasinci);
  
  Serial.printf("   Yakıt: %.1f%% | Batarya: %.1fV\n", 
    genset.getYakitSeviyesi(), genset.getBataryaVoltaji());
  
  // Alarm durumu
  Serial.println("\n🚨 ALARM DURUMU:");
  String alarmStatus = "   ";
  if (genset.isAlarmActive(AlarmTipi::Kapatma)) alarmStatus += "🔴 KAPATMA ";
  if (genset.isAlarmActive(AlarmTipi::YukAtma)) alarmStatus += "🟡 YÜK-ATMA ";
  if (genset.isAlarmActive(AlarmTipi::Uyari)) alarmStatus += "🟠 UYARI ";
  if (alarmStatus == "   ") alarmStatus += "✅ ALARM YOK";
  Serial.println(alarmStatus);
  
  // Sistem sağlığı
  Serial.printf("\n💚 Sistem Durumu: %s\n", 
    genset.isSystemHealthy() ? "SAĞLIKLI" : "SORUNLU");
  
  // JSON veri (API'ler için)
  Serial.println("\n📱 JSON Veri:");
  Serial.println("   " + genset.getBasicDataAsJSON());
  
  Serial.println(String("-").substring(0, 50));
  Serial.println("Komutlar: 'h' yardım, 's' start, 'p' stop, 'a' auto, 'm' manual, 't' test");
  Serial.println(String("-").substring(0, 50) + "\n");
}

void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    
    Serial.println("\n💻 Komut alındı: " + command);
    
    if (command == "h" || command == "help") {
      printHelp();
    }
    else if (command == "s" || command == "start") {
      Serial.println("🚀 Jeneratör başlatılıyor...");
      if (genset.startGenerator()) {
        Serial.println("✅ Başlatma komutu gönderildi");
      } else {
        Serial.println("❌ Başlatma komutu gönderilemedi");
      }
    }
    else if (command == "p" || command == "stop") {
      Serial.println("🛑 Jeneratör durduruluyor...");
      if (genset.stopGenerator()) {
        Serial.println("✅ Durdurma komutu gönderildi");
      } else {
        Serial.println("❌ Durdurma komutu gönderilemedi");
      }
    }
    else if (command == "a" || command == "auto") {
      Serial.println("🤖 AUTO moduna geçiliyor...");
      if (genset.setAutoMode()) {
        Serial.println("✅ AUTO mod komutu gönderildi");
      } else {
        Serial.println("❌ AUTO mod komutu gönderilemedi");
      }
    }
    else if (command == "m" || command == "manual") {
      Serial.println("👨‍🔧 MANUEL moduna geçiliyor...");
      if (genset.setManualMode()) {
        Serial.println("✅ MANUEL mod komutu gönderildi");
      } else {
        Serial.println("❌ MANUEL mod komutu gönderilemedi");
      }
    }
    else if (command == "t" || command == "test") {
      Serial.println("🧪 TEST moduna geçiliyor...");
      if (genset.setTestMode()) {
        Serial.println("✅ TEST mod komutu gönderildi");
      } else {
        Serial.println("❌ TEST mod komutu gönderilemedi");
      }
    }
    else if (command == "e" || command == "emergency") {
      Serial.println("🚨 ACİL DURDURMA!");
      if (genset.emergencyStop()) {
        Serial.println("✅ Acil durdurma komutu gönderildi");
      } else {
        Serial.println("❌ Acil durdurma komutu gönderilemedi");
      }
    }
    else if (command == "i" || command == "info") {
      genset.printAllData();
    }
    else if (command == "r" || command == "reset") {
      Serial.println("⚠️ ÜNİTE RESET EDİLİYOR!");
      Serial.println("Bu işlem cihazı yeniden başlatacak. Emin misiniz? (y/n)");
      delay(5000); // 5 saniye bekle
      if (Serial.available() && Serial.readString().startsWith("y")) {
        if (genset.resetUnit()) {
          Serial.println("✅ Reset komutu gönderildi");
        } else {
          Serial.println("❌ Reset komutu gönderilemedi");
        }
      } else {
        Serial.println("Reset iptal edildi");
      }
    }
    else if (command == "j" || command == "json") {
      Serial.println("📄 Tam JSON Veri:");
      Serial.println(genset.getDataAsJSON());
    }
    else {
      Serial.println("❓ Bilinmeyen komut. Yardım için 'h' yazın.");
    }
    
    Serial.println(); // Boş satır
  }
}

void printHelp() {
  Serial.println("\n📖 KOMUT LİSTESİ:");
  Serial.println("════════════════════════════════════════");
  Serial.println("🔧 KONTROL KOMUTLARI:");
  Serial.println("   s, start    - Jeneratörü başlat");
  Serial.println("   p, stop     - Jeneratörü durdur");
  Serial.println("   a, auto     - AUTO moduna geç");
  Serial.println("   m, manual   - MANUEL moduna geç");
  Serial.println("   t, test     - TEST moduna geç");
  Serial.println("   e, emergency- Acil durdurma");
  Serial.println("");
  Serial.println("📊 BİLGİ KOMUTLARI:");
  Serial.println("   i, info     - Detaylı sistem bilgisi");
  Serial.println("   j, json     - Tam JSON veri");
  Serial.println("   h, help     - Bu yardım menüsü");
  Serial.println("");
  Serial.println("⚙️ SİSTEM KOMUTLARI:");
  Serial.println("   r, reset    - Ünite reset (DİKKAT!)");
  Serial.println("════════════════════════════════════════");
  Serial.println("💡 İpucu: Sistem otomatik olarak 5 saniyede bir");
  Serial.println("   durum raporunu yazdırır ve 1 saniyede bir");
  Serial.println("   alarm kontrolü yapar.\n");
}
