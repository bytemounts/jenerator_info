/*
 * D300Controller.h
 * D-300 MK3 Modbus RTU Library for ESP32
 * Header File - Sınıf tanımlamaları ve enum'lar
 */

#ifndef D300_CONTROLLER_H
#define D300_CONTROLLER_H

#ifdef ARDUINO
#include <Arduino.h>
#endif

#include <ModbusMaster.h>
#include <HardwareSerial.h>

// Sistem durumu enum'ları
enum class UniteDurumu : uint16_t {
  JeneratorDinlenme = 0,
  YakitOncesiBekleme = 1,
  MotorOnIsitma = 2,
  YagFlashBekleme = 3,
  MarsDinlenmesi = 4,
  MarsAtma = 5,
  MotorRolantiHizi = 6,
  MotorIsinmasi = 7,
  YuksuzCalisma = 8,
  SebekeyeSenkronizasyon = 9,
  YukTransferiJeneratoreL = 10,
  GenCBAktivasyonu = 11,
  JeneratorCBZamanlayici = 12,
  MasterJeneratorYuklu = 13,
  PeakLopping = 14,
  PowerExporting = 15,
  SlaveJeneratorYuklu = 16,
  SenkronizasyonSebekey = 17,
  YukTransferiSebekey = 18,
  MainsCBAktivasyonu = 19,
  MainsCBZamanlayici = 20,
  SogutmaliDurdurma = 21,
  Soguyor = 22,
  MotorStopRolanti = 23,
  AcilDurdurma = 24,
  MotorDuruyor = 25
};

enum class UniteModu : uint16_t {
  STOP = 1,
  MANUEL = 2,
  AUTO = 4,
  TEST = 8
};

enum class AlarmTipi {
  Kapatma,
  YukAtma,
  Uyari
};

// Buton maskeleri
enum class ButonMaski : uint16_t {
  STOP = 0x0001,
  MANUEL_RUN = 0x0002,
  AUTO = 0x0004,
  TEST = 0x0008,
  RUN_D700 = 0x0010,
  GCB = 0x0020,
  MCB = 0x0040,
  MENU_PLUS = 0x0080,
  MENU_MINUS = 0x0100,
  UP = 0x0200,
  DOWN = 0x0400,
  LONG_PRESS = 0x4000,
  VERY_LONG_PRESS = 0x8000
};

// Veri yapıları
struct FazVerisi {
  float Voltaj = 0.0;
  float Akim = 0.0;
  float AktifGuc = 0.0;
  float ReaktifGuc = 0.0;
  float GorunurGuc = 0.0;
  float GucFaktoru = 0.0;
};

struct ToplamVerisi {
  float AktifGuc = 0.0;
  float ReaktifGuc = 0.0;
  float GorunurGuc = 0.0;
  float GucFaktoru = 0.0;
  float OrtalamaVoltaj = 0.0;
  float OrtalamaAkim = 0.0;
};

struct ElektrikselSistem {
  FazVerisi L1;
  FazVerisi L2;
  FazVerisi L3;
  ToplamVerisi Toplam;
  float Frekans = 0.0;
  float FazVoltajL1L2 = 0.0;
  float FazVoltajL2L3 = 0.0;
  float FazVoltajL3L1 = 0.0;
  float NeutralAkim = 0.0;
};

struct MotorVerileri {
  float RPM = 0.0;
  float Sicaklik = 0.0;
  float YagBasinci = 0.0;
  float YakitSeviyesi = 0.0;
  float HariciYakitSeviyesi = 0.0;  // <<<< YENİ: Harici şamandra
  float YagSicakligi = 0.0;
  float KanopyicSicaklik = 0.0;
  float OrtamSicakligi = 0.0;
  float BataryaVoltaji = 0.0;
  float MinBataryaVoltaji = 0.0;
  float SarjVoltaji = 0.0;
  float SarjAkimi = 0.0;
};

struct SistemDurumu {
  UniteDurumu Durum = UniteDurumu::JeneratorDinlenme;
  UniteModu Mod = UniteModu::STOP;
  uint16_t OperasyonZamanlayici = 0;
  float GOVKontrolCikis = 0.0;
  float AVRKontrolCikis = 0.0;
  bool KapatmaAlarmi = false;
  bool YukAtmaAlarmi = false;
  bool UyariAlarmi = false;
  uint16_t CihazKimlik = 0;
  uint16_t DonanımVersiyon = 0;
  uint16_t YazilimVersiyon = 0;
};

struct Sayaclar {
  uint32_t JeneratorCalismaAdedi = 0;
  uint32_t JeneratorMarsAdedi = 0;
  uint32_t JeneratorYukluCalisma = 0;
  float MotorCalismaSaati = 0.0;
  float SonServistenBeriSaat = 0.0;
  float SonServistenBeriGun = 0.0;
  float ToplamAktifEnerji = 0.0;
  float ToplamReaktifEnerjiInd = 0.0;
  float ToplamReaktifEnerjiCap = 0.0;
  float YakitSayaci = 0.0;
};

struct AnalogGirisler {
  uint16_t Analog1Ohm = 0;
  uint16_t Analog2Ohm = 0;
  uint16_t Analog3Ohm = 0;
  uint16_t Analog4Ohm = 0;
  uint16_t Analog5Ohm = 0;
  uint16_t Analog6Ohm = 0;
  uint16_t Analog7Deger = 0;
  uint16_t Analog8Deger = 0;
};

class D300Controller {
private:
  ModbusMaster node;
  HardwareSerial* modbusSerial;
  uint8_t slaveID;
  unsigned long lastUpdateTime;
  unsigned long updateInterval;
  bool autoUpdate;
  bool connectionStatus;
  uint8_t consecutiveErrors;
  static const uint8_t MAX_ERRORS = 5;
  
  // Private fonksiyonlar
  bool read32BitValue(uint16_t address, uint32_t &value);
  bool read16BitValue(uint16_t address, uint16_t &value);
  bool readFloat32(uint16_t address, float &value, int coefficient);
  bool readFloat16(uint16_t address, float &value, int coefficient);
  
  void updateElektrikselVeriler();
  void updateMotorVerileri();
  void updateSistemDurumu();
  void updateSayaclar();
  void updateAnalogGirisler();
  void updateAlarmDurumlari();
  
  void handleError();
  void resetErrorCounter();
/*
*/
  // ADC yakıt sensörü için yeni değişkenler
  const uint8_t FUEL_ADC_PIN = 34;
  const int ADC_SAMPLES = 10;          // Ortalama alma için sample sayısı
  const float R1 = 680.0;             // Alt direnç (ohm)
  const float R2 = 4700.0;            // Üst direnç (ohm)
  const float VCC = 5.0;              // Besleme voltajı
  const float ADC_MAX = 4095.0;       // 12-bit ADC maksimum değeri
  const float ESP32_VREF = 3.3;       // ESP32 referans voltajı
  
  // Şamandra kalibrasyonu (bu değerleri yakıt tankınıza göre ayarlayın)
  const float FUEL_EMPTY_RESISTANCE = 10.0;    // Boş tank direnci (ohm)
  const float FUEL_FULL_RESISTANCE = 180.0;    // Dolu tank direnci (ohm)
  
  // ADC fonksiyonları
  float readFuelADC();
  float calculateFuelLevel(float adcVoltage);
  float resistanceToFuelLevel(float resistance);


public:
  // Public veri yapıları
  struct {
    ElektrikselSistem Sebeke;
    ElektrikselSistem Jenerator;
  } ElektrikSistemi;
  
  MotorVerileri Motor;
  SistemDurumu Sistem;
  Sayaclar Sayac;
  AnalogGirisler AnalogGiris;
  
  /**/
 float getHariciYakitSeviyesi() const { return Motor.HariciYakitSeviyesi; }
  
  // Kalibasyon fonksiyonları
  void calibrateFuelSensor(float emptyResistance, float fullResistance);
  void setFuelSensorPin(uint8_t pin);



  // Constructor
  D300Controller(uint8_t slaveID = 1, uint8_t rxPin = 16, uint8_t txPin = 17);
  
  // Ana fonksiyonlar
  bool begin(uint32_t baudRate = 9600, unsigned long updateInterval = 5000);
  bool updateData();
  bool updateBasicData();
  void handle();
  
  // Kontrol fonksiyonları
  bool simulateButton(ButonMaski buton);
  bool startGenerator();
  bool stopGenerator();
  bool setAutoMode();
  bool setManualMode();
  bool setTestMode();
  bool emergencyStop();
  bool resetUnit();
  // JSON veri oluşturma
  String buildJson() const;
  // Durum kontrol fonksiyonları
  String getDurumAciklama() const;
  String getModAciklama() const;
  bool isAlarmActive(AlarmTipi tip) const;
  bool isJeneratorCalisir() const;
  bool isSebekeMevcut() const;
  bool isSystemHealthy() const;
  bool isConnected() const;
  
  // Debug fonksiyonları
  void printAllData() const;
  void printElektrikselVeriler() const;
  void printMotorVerileri() const;
  void printSistemDurumu() const;
  
  // JSON çıktı
  String getDataAsJSON() const;
  String getBasicDataAsJSON() const;
  
  // Hızlı erişim getters
  float getJeneratorGuc() const { return ElektrikSistemi.Jenerator.Toplam.AktifGuc; }
  float getSebekeFrekans() const { return ElektrikSistemi.Sebeke.Frekans; }
  float getJeneratorFrekans() const { return ElektrikSistemi.Jenerator.Frekans; }
  float getMotorRPM() const { return Motor.RPM; }
  float getBataryaVoltaji() const { return Motor.BataryaVoltaji; }
  float getYakitSeviyesi() const { return Motor.YakitSeviyesi; }
  UniteDurumu getUniteDurumu() const { return Sistem.Durum; }
  UniteModu getUniteModu() const { return Sistem.Mod; }
  bool getBaglantiDurumu() const { return connectionStatus; }
  
  // Ayar fonksiyonları
  void enableAutoUpdate(bool enable = true) { autoUpdate = enable; }
  void setUpdateInterval(unsigned long interval) { updateInterval = interval; }
  void setSlaveID(uint8_t newSlaveID);
};

#endif // D300_CONTROLLER_H
