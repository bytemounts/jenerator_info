/*
 * D-300 MK3 Modbus RTU Library for ESP32
 * RS485 TTL Converter with Hardware Auto Flow Control
 * Datakom D-300 MK3 Jeneratör Kontrol Sistemi için
 * 
 * RS485 Module: UART TTL to RS485 Two-way Converter (Auto Flow Control)
 * 
 * Bağlantı Şeması:
 * ESP32        RS485 Module        D-300 MK3
 * GPIO16 ----> TXD               
 * GPIO17 ----> RXD               
 * 3.3V   ----> VCC               
 * GND    ----> GND               
 *              A+ -------------> A+ (Terminal 1)
 *              B- -------------> B- (Terminal 2)
 * 
 * Kullanım:
 * D300Controller genset(slaveID, rxPin, txPin);
 * genset.begin(baudRate);
 * genset.updateData(); // Timer ile otomatik çağırılabilir
 * 
 * Veri okuma:
 * float voltage = genset.ElektrikSistemi.Sebeke.L1.Voltaj;
 * float power = genset.ElektrikSistemi.Jenerator.Toplam.AktifGuc;
 */

#ifndef D300_CONTROLLER_H
#define D300_CONTROLLER_H

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
  MANUEL = 2,  // D-500'de RUN
  AUTO = 4,
  TEST = 8
};

enum class AlarmTipi {
  Kapatma,
  YukAtma,
  Uyari
};

// Buton maskeleri - PDF'den alınan bit tanımları
enum class ButonMaski : uint16_t {
  STOP = 0x0001,           // BIT 0
  MANUEL_RUN = 0x0002,     // BIT 1 - D700'de Manual, D500'de Run
  AUTO = 0x0004,           // BIT 2
  TEST = 0x0008,           // BIT 3
  RUN_D700 = 0x0010,       // BIT 4 - Sadece D700
  GCB = 0x0020,            // BIT 5
  MCB = 0x0040,            // BIT 6
  MENU_PLUS = 0x0080,      // BIT 7
  MENU_MINUS = 0x0100,     // BIT 8
  UP = 0x0200,             // BIT 9
  DOWN = 0x0400,           // BIT 10
  LONG_PRESS = 0x4000,     // BIT 14
  VERY_LONG_PRESS = 0x8000 // BIT 15
};

// Harmonik kanal seçimi
enum class HarmonikKanal : uint8_t {
  SEBEKE_L1_VOLTAJ = 0,
  SEBEKE_L2_VOLTAJ = 1,
  SEBEKE_L3_VOLTAJ = 2,
  JENERATOR_L1_VOLTAJ = 3,
  JENERATOR_L2_VOLTAJ = 4,
  JENERATOR_L3_VOLTAJ = 5,
  SEBEKE_L1L2_VOLTAJ = 6,
  SEBEKE_L2L3_VOLTAJ = 7,
  SEBEKE_L3L1_VOLTAJ = 8,
  JENERATOR_L1L2_VOLTAJ = 9,
  JENERATOR_L2L3_VOLTAJ = 10,
  JENERATOR_L3L1_VOLTAJ = 11,
  SEBEKE_L1_AKIM = 12,
  SEBEKE_L2_AKIM = 13,
  SEBEKE_L3_AKIM = 14,
  JENERATOR_L1_AKIM = 15,
  JENERATOR_L2_AKIM = 16,
  JENERATOR_L3_AKIM = 17,
  SEBEKE_NEUTRAL_AKIM = 18,
  JENERATOR_NEUTRAL_AKIM = 19
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
  float FlowMeter = 0.0;
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

struct HarmonikVerileri {
  float THD = 0.0;           // Total Harmonic Distortion
  float Fundamental = 0.0;   // Temel harmonik
  float H3 = 0.0, H5 = 0.0, H7 = 0.0, H9 = 0.0;
  float H11 = 0.0, H13 = 0.0, H15 = 0.0, H17 = 0.0;
  float H19 = 0.0, H21 = 0.0, H23 = 0.0, H25 = 0.0;
  float H27 = 0.0, H29 = 0.0, H31 = 0.0;
  HarmonikKanal SeciliKanal = HarmonikKanal::JENERATOR_L1_VOLTAJ;
};

struct GPS_Veriler {
  float Enlem = 0.0;        // Latitude
  float Boylam = 0.0;       // Longitude
  float Yukseklik = 0.0;    // Altitude
};

struct IletisimVerileri {
  uint16_t EthernetResetSayaci = 0;
  uint16_t EthernetTCPPaketSayaci = 0;
  String MAC_Adresi = "";
  String CihazKimlikNo = "";
  String ModemIMEI = "";
  uint32_t GPRS_IP = 0;
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
  
  // Veri okuma fonksiyonları
  bool read32BitValue(uint16_t address, uint32_t &value);
  bool read16BitValue(uint16_t address, uint16_t &value);
  bool readFloat32(uint16_t address, float &value, int coefficient);
  bool readFloat16(uint16_t address, float &value, int coefficient);
  bool readMultipleRegisters(uint16_t address, uint16_t quantity, uint16_t* buffer);
  
  // Sistem verilerini güncelleme fonksiyonları
  void updateElektrikselVeriler();
  void updateMotorVerileri();
  void updateSistemDurumu();
  void updateSayaclar();
  void updateAnalogGirisler();
  void updateAlarmDurumlari();
  void updateHarmonikVeriler();
  void updateGPSVeriler();
  void updateIletisimVeriler();
  
  // Hata yönetimi
  void handleError();
  void resetErrorCounter();

public:
  // Public veri yapıları - doğrudan erişim için
  struct {
    ElektrikselSistem Sebeke;
    ElektrikselSistem Jenerator;
  } ElektrikSistemi;
  
  MotorVerileri Motor;
  SistemDurumu Sistem;
  Sayaclar Sayac;
  AnalogGirisler AnalogGiris;
  HarmonikVerileri Harmonik;
  GPS_Veriler GPS;
  IletisimVerileri Iletisim;
  
  // Constructor
  D300Controller(uint8_t slaveID = 1, uint8_t rxPin = 16, uint8_t txPin = 17);
  
  // Sistem başlatma
  bool begin(uint32_t baudRate = 9600, unsigned long updateInterval = 5000);
  
  // Manuel veri güncelleme
  bool updateData();
  bool updateBasicData();     // Sadece temel veriler (daha hızlı)
  bool updateExtendedData();  // Genişletilmiş veriler
  
  // Otomatik güncelleme kontrolü
  void enableAutoUpdate(bool enable = true) { autoUpdate = enable; }
  void setUpdateInterval(unsigned long interval) { updateInterval = interval; }
  
  // Loop fonksiyonu - ana programda sürekli çağırılmalı
  void handle();
  
  // Kontrol komutları
  bool simulateButton(ButonMaski buton);
  bool simulateButtonCombination(uint16_t buttonMask);
  bool resetUnit();
  bool setHarmonicChannel(HarmonikKanal kanal);
  
  // Jeneratör kontrol komutları
  bool startGenerator();
  bool stopGenerator();
  bool setAutoMode();
  bool setManualMode();
  bool setTestMode();
  bool emergencyStop();
  
  // Durum sorgulama fonksiyonları
  String getDurumAciklama() const;
  String getModAciklama() const;
  String getAlarmAciklama(AlarmTipi tip) const;
  bool isAlarmActive(AlarmTipi tip) const;
  bool isJeneratorCalisir() const;
  bool isSebekeMevcut() const;
  bool isSystemHealthy() const;
  
  // Debug ve bilgi fonksiyonları
  void printAllData() const;
  void printElektrikselVeriler() const;
  void printMotorVerileri() const;
  void printSistemDurumu() const;
  void printAlarmlar() const;
  void printSayaclar() const;
  void printHarmonikVeriler() const;
  bool isConnected() const;
  
  // JSON çıktı fonksiyonları
  String getDataAsJSON() const;
  String getBasicDataAsJSON() const;
  
  // Getters - Hızlı erişim için
  float getJeneratorGuc() const { return ElektrikSistemi.Jenerator.Toplam.AktifGuc; }
  float getSebekeFrekans() const { return ElektrikSistemi.Sebeke.Frekans; }
  float getJeneratorFrekans() const { return ElektrikSistemi.Jenerator.Frekans; }
  float getMotorRPM() const { return Motor.RPM; }
  float getBataryaVoltaji() const { return Motor.BataryaVoltaji; }
  float getYakitSeviyesi() const { return Motor.YakitSeviyesi; }
  UniteDurumu getUniteDurumu() const { return Sistem.Durum; }
  UniteModu getUniteModu() const { return Sistem.Mod; }
  bool getBaglantiDurumu() const { return connectionStatus; }
  
  // Setters
  void setSlaveID(uint8_t newSlaveID);
  void setTimeout(unsigned long timeout);
};

// Constructor implementation
D300Controller::D300Controller(uint8_t slaveID, uint8_t rxPin, uint8_t txPin) 
  : slaveID(slaveID), lastUpdateTime(0), updateInterval(5000), autoUpdate(true), 
    connectionStatus(false), consecutiveErrors(0) {
  
  // RS485 TTL modülü otomatik flow control'e sahip olduğu için 
  // sadece RX ve TX pinlerini kullanıyoruz
  modbusSerial = new HardwareSerial(2);
  modbusSerial->begin(9600, SERIAL_8N1, rxPin, txPin);
}

bool D300Controller::begin(uint32_t baudRate, unsigned long updateInterval) {
  this->updateInterval = updateInterval;
  
  // Serial bağlantısını başlat
  modbusSerial->begin(baudRate, SERIAL_8N1);
  
  // Modbus konfigürasyonu - RS485 TTL modülü otomatik flow control
  // kullandığı için preTransmission ve postTransmission fonksiyonları gerekli değil
  node.begin(slaveID, *modbusSerial);
  
  // Timeout ayarları
  node.idle([]{});  // Boş idle fonksiyonu
  
  delay(100); // Modülün hazırlanması için kısa bekleme
  
  // İlk bağlantı testi
  connectionStatus = isConnected();
  if (connectionStatus) {
    resetErrorCounter();
    // İlk veri okuma
    return updateBasicData();
  }
  
  return false;
}

bool D300Controller::read32BitValue(uint16_t address, uint32_t &value) {
  uint8_t result = node.readHoldingRegisters(address, 2);
  if (result == node.ku8MBSuccess) {
    value = ((uint32_t)node.getResponseBuffer(0) << 16) | node.getResponseBuffer(1);
    resetErrorCounter();
    return true;
  }
  handleError();
  return false;
}

bool D300Controller::read16BitValue(uint16_t address, uint16_t &value) {
  uint8_t result = node.readHoldingRegisters(address, 1);
  if (result == node.ku8MBSuccess) {
    value = node.getResponseBuffer(0);
    resetErrorCounter();
    return true;
  }
  handleError();
  return false;
}

bool D300Controller::readFloat32(uint16_t address, float &value, int coefficient) {
  uint32_t rawValue;
  if (read32BitValue(address, rawValue)) {
    value = (float)rawValue / coefficient;
    return true;
  }
  return false;
}

bool D300Controller::readFloat16(uint16_t address, float &value, int coefficient) {
  uint16_t rawValue;
  if (read16BitValue(address, rawValue)) {
    value = (float)rawValue / coefficient;
    return true;
  }
  return false;
}

bool D300Controller::readMultipleRegisters(uint16_t address, uint16_t quantity, uint16_t* buffer) {
  uint8_t result = node.readHoldingRegisters(address, quantity);
  if (result == node.ku8MBSuccess) {
    for (uint16_t i = 0; i < quantity; i++) {
      buffer[i] = node.getResponseBuffer(i);
    }
    resetErrorCounter();
    return true;
  }
  handleError();
  return false;
}

void D300Controller::handleError() {
  consecutiveErrors++;
  if (consecutiveErrors >= MAX_ERRORS) {
    connectionStatus = false;
  }
}

void D300Controller::resetErrorCounter() {
  consecutiveErrors = 0;
  connectionStatus = true;
}

void D300Controller::updateElektrikselVeriler() {
  // Şebeke voltajları
  readFloat32(10240, ElektrikSistemi.Sebeke.L1.Voltaj, 10);
  readFloat32(10242, ElektrikSistemi.Sebeke.L2.Voltaj, 10);
  readFloat32(10244, ElektrikSistemi.Sebeke.L3.Voltaj, 10);
  
  // Jeneratör voltajları
  readFloat32(10246, ElektrikSistemi.Jenerator.L1.Voltaj, 10);
  readFloat32(10248, ElektrikSistemi.Jenerator.L2.Voltaj, 10);
  readFloat32(10250, ElektrikSistemi.Jenerator.L3.Voltaj, 10);
  
  // Faz voltajları
  readFloat32(10252, ElektrikSistemi.Sebeke.FazVoltajL1L2, 10);
  readFloat32(10254, ElektrikSistemi.Sebeke.FazVoltajL2L3, 10);
  readFloat32(10256, ElektrikSistemi.Sebeke.FazVoltajL3L1, 10);
  readFloat32(10258, ElektrikSistemi.Jenerator.FazVoltajL1L2, 10);
  readFloat32(10260, ElektrikSistemi.Jenerator.FazVoltajL2L3, 10);
  readFloat32(10262, ElektrikSistemi.Jenerator.FazVoltajL3L1, 10);
  
  // Akımlar
  readFloat32(10264, ElektrikSistemi.Sebeke.L1.Akim, 10);
  readFloat32(10266, ElektrikSistemi.Sebeke.L2.Akim, 10);
  readFloat32(10268, ElektrikSistemi.Sebeke.L3.Akim, 10);
  readFloat32(10270, ElektrikSistemi.Jenerator.L1.Akim, 10);
  readFloat32(10272, ElektrikSistemi.Jenerator.L2.Akim, 10);
  readFloat32(10274, ElektrikSistemi.Jenerator.L3.Akim, 10);
  readFloat32(10276, ElektrikSistemi.Sebeke.NeutralAkim, 10);
  readFloat32(10278, ElektrikSistemi.Jenerator.NeutralAkim, 10);
  
  // Güç verileri
  readFloat32(10292, ElektrikSistemi.Sebeke.Toplam.AktifGuc, 10);
  readFloat32(10294, ElektrikSistemi.Jenerator.Toplam.AktifGuc, 10);
  readFloat32(10308, ElektrikSistemi.Sebeke.Toplam.ReaktifGuc, 10);
  readFloat32(10310, ElektrikSistemi.Jenerator.Toplam.ReaktifGuc, 10);
  readFloat32(10324, ElektrikSistemi.Sebeke.Toplam.GorunurGuc, 10);
  readFloat32(10326, ElektrikSistemi.Jenerator.Toplam.GorunurGuc, 10);
  
  // Güç faktörleri
  readFloat16(10334, ElektrikSistemi.Sebeke.Toplam.GucFaktoru, 10);
  readFloat16(10335, ElektrikSistemi.Jenerator.Toplam.GucFaktoru, 10);
  
  // Frekanslar
  readFloat16(10338, ElektrikSistemi.Sebeke.Frekans, 100);
  readFloat16(10339, ElektrikSistemi.Jenerator.Frekans, 100);
  
  // Ortalama değerler
  readFloat32(10377, ElektrikSistemi.Jenerator.Toplam.OrtalamaVoltaj, 10);
  readFloat32(10379, ElektrikSistemi.Jenerator.Toplam.OrtalamaAkim, 10);
  readFloat32(10381, ElektrikSistemi.Sebeke.Toplam.OrtalamaVoltaj, 10);
  readFloat32(10383, ElektrikSistemi.Sebeke.Toplam.OrtalamaAkim, 10);
}

void D300Controller::updateMotorVerileri() {
  readFloat16(10376, Motor.RPM, 1);
  readFloat16(10362, Motor.Sicaklik, 10);
  readFloat16(10361, Motor.YagBasinci, 10);
  readFloat16(10363, Motor.YakitSeviyesi, 10);
  readFloat16(10364, Motor.YagSicakligi, 10);
  readFloat16(10365, Motor.KanopyicSicaklik, 10);
  readFloat16(10366, Motor.OrtamSicakligi, 10);
  readFloat16(10341, Motor.BataryaVoltaji, 100);
  readFloat16(10385, Motor.MinBataryaVoltaji, 100);
  readFloat16(10340, Motor.SarjVoltaji, 100);
  readFloat16(11173, Motor.SarjAkimi, 10);
}

void D300Controller::updateSistemDurumu() {
  uint16_t durum, mod;
  if (read16BitValue(10604, durum)) {
    Sistem.Durum = static_cast<UniteDurumu>(durum);
  }
  
  if (read16BitValue(10605, mod)) {
    Sistem.Mod = static_cast<UniteModu>(mod);
  }
  
  read16BitValue(10606, Sistem.OperasyonZamanlayici);
  readFloat16(10607, Sistem.GOVKontrolCikis, 10);
  readFloat16(10608, Sistem.AVRKontrolCikis, 10);
  read16BitValue(10609, Sistem.CihazKimlik);
  read16BitValue(10610, Sistem.DonanımVersiyon);
  read16BitValue(10611, Sistem.YazilimVersiyon);
}

void D300Controller::updateAlarmDurumlari() {
  uint16_t tempValue;
  
  // Kapatma alarmları kontrolü
  Sistem.KapatmaAlarmi = false;
  for (uint16_t addr = 10504; addr <= 10519; addr++) {
    if (read16BitValue(addr, tempValue) && tempValue != 0) {
      Sistem.KapatmaAlarmi = true;
      break;
    }
  }
  
  // Yük atma alarmları kontrolü
  Sistem.YukAtmaAlarmi = false;
  for (uint16_t addr = 10520; addr <= 10535; addr++) {
    if (read16BitValue(addr, tempValue) && tempValue != 0) {
      Sistem.YukAtmaAlarmi = true;
      break;
    }
  }
  
  // Uyarı alarmları kontrolü
  Sistem.UyariAlarmi = false;
  for (uint16_t addr = 10536; addr <= 10551; addr++) {
    if (read16BitValue(addr, tempValue) && tempValue != 0) {
      Sistem.UyariAlarmi = true;
      break;
    }
  }
}

void D300Controller::updateHarmonikVeriler() {
  readFloat16(10386, Harmonik.THD, 100);
  readFloat16(10387, Harmonik.Fundamental, 100);
  readFloat16(10388, Harmonik.H3, 100);
  readFloat16(10389, Harmonik.H5, 100);
  readFloat16(10390, Harmonik.H7, 100);
  readFloat16(10391, Harmonik.H9, 100);
  readFloat16(10392, Harmonik.H11, 100);
  readFloat16(10393, Harmonik.H13, 100);
  
  uint16_t kanal;
  if (read16BitValue(10403, kanal)) {
    Harmonik.SeciliKanal = static_cast<HarmonikKanal>(kanal);
  }
}

void D300Controller::updateGPSVeriler() {
  read32BitValue(10594, *(uint32_t*)&GPS.Enlem);
  read32BitValue(10596, *(uint32_t*)&GPS.Boylam);
  read32BitValue(10598, *(uint32_t*)&GPS.Yukseklik);
}

bool D300Controller::updateBasicData() {
  updateElektrikselVeriler();
  updateMotorVerileri();
  updateSistemDurumu();
  updateAlarmDurumlari();
  lastUpdateTime = millis();
  return connectionStatus;
}

bool D300Controller::updateExtendedData() {
  updateSayaclar();
  updateAnalogGirisler();
  updateHarmonikVeriler();
  updateGPSVeriler();
  return connectionStatus;
}

bool D300Controller::updateData() {
  bool basicResult = updateBasicData();
  bool extendedResult = updateExtendedData();
  return basicResult && extendedResult;
}

void D300Controller::handle() {
  if (autoUpdate && (millis() - lastUpdateTime >= updateInterval)) {
    updateBasicData();
  }
}

// Kontrol komutları
bool D300Controller::simulateButton(ButonMaski buton) {
  uint8_t result = node.writeSingleRegister(8193, static_cast<uint16_t>(buton));
  if (result == node.ku8MBSuccess) {
    resetErrorCounter();
    return true;
  }
  handleError();
  return false;
}

bool D300Controller::simulateButtonCombination(uint16_t buttonMask) {
  uint8_t result = node.writeSingleRegister(8193, buttonMask);
  if (result == node.ku8MBSuccess) {
    resetErrorCounter();
    return true;
  }
  handleError();
  return false;
}

bool D300Controller::resetUnit() {
  uint8_t result = node.writeSingleRegister(8210, 14536);
  if (result == node.ku8MBSuccess) {
    resetErrorCounter();
    return true;
  }
  handleError();
  return false;
}

bool D300Controller::setHarmonicChannel(HarmonikKanal kanal) {
  uint8_t result = node.writeSingleRegister(8194, static_cast<uint8_t>(kanal));
  if (result == node.ku8MBSuccess) {
    resetErrorCounter();
    return true;
  }
  handleError();
  return false;
}

// Jeneratör kontrol komutları
bool D300Controller::startGenerator() {
  return simulateButton(ButonMaski::AUTO);
}

bool D300Controller::stopGenerator() {
  return simulateButton(ButonMaski::STOP);
}

bool D300Controller::setAutoMode() {
  return simulateButton(ButonMaski::AUTO);
}

bool D300Controller::setManualMode() {
  return simulateButton(ButonMaski::MANUEL_RUN);
}

bool D300Controller::setTestMode() {
  return simulateButton(ButonMaski::TEST);
}

bool D300Controller::emergencyStop() {
  // Uzun basma ile acil durdurma
  uint16_t emergencyMask = static_cast<uint16_t>(ButonMaski::STOP) | 
                           static_cast<uint16_t>(ButonMaski::LONG_PRESS);
  return simulateButtonCombination(emergencyMask);
}

void D300Controller::updateSayaclar() {
  read32BitValue(10616, Sayac.JeneratorCalismaAdedi);
  read32BitValue(10618, Sayac.JeneratorMarsAdedi);
  read32BitValue(10620, Sayac.JeneratorYukluCalisma);
  readFloat32(10622, Sayac.MotorCalismaSaati, 100);
  readFloat32(10624, Sayac.SonServistenBeriSaat, 100);
  readFloat32(10626, Sayac.SonServistenBeriGun, 100);
  readFloat32(10628, Sayac.ToplamAktifEnerji, 10);
  readFloat32(10630, Sayac.ToplamReaktifEnerjiInd, 10);
  readFloat32(10632, Sayac.ToplamReaktifEnerjiCap, 10);
  readFloat32(11577, Sayac.YakitSayaci, 10);
  readFloat32(11680, Sayac.FlowMeter, 10);
}

void D300Controller::updateAnalogGirisler() {
  read16BitValue(10345, AnalogGiris.Analog1Ohm);
  read16BitValue(10346, AnalogGiris.Analog2Ohm);
  read16BitValue(10347, AnalogGiris.Analog3Ohm);
  read16BitValue(10348, AnalogGiris.Analog4Ohm);
  read16BitValue(10349, AnalogGiris.Analog5Ohm);
  read16BitValue(10350, AnalogGiris.Analog6Ohm);
  read16BitValue(10351, AnalogGiris.Analog7Deger);
  read16BitValue(10352, AnalogGiris.Analog8Deger);
}

void D300Controller::updateIletisimVeriler() {
  read16BitValue(11682, Iletisim.EthernetResetSayaci);
  read16BitValue(11683, Iletisim.EthernetTCPPaketSayaci);
  read32BitValue(10646, Iletisim.GPRS_IP);
  
  // MAC adresi okuma (48 bit = 3 register)
  uint16_t macRegs[3];
  if (readMultipleRegisters(11684, 3, macRegs)) {
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
            (macRegs[0] >> 8) & 0xFF, macRegs[0] & 0xFF,
            (macRegs[1] >> 8) & 0xFF, macRegs[1] & 0xFF,
            (macRegs[2] >> 8) & 0xFF, macRegs[2] & 0xFF);
    Iletisim.MAC_Adresi = String(macStr);
  }
}

String D300Controller::getDurumAciklama() const {
  switch (Sistem.Durum) {
    case UniteDurumu::JeneratorDinlenme: return "Jeneratör Dinlenme";
    case UniteDurumu::YakitOncesiBekleme: return "Yakıt Öncesi Bekleme";
    case UniteDurumu::MotorOnIsitma: return "Motor Ön Isıtma";
    case UniteDurumu::YagFlashBekleme: return "Yağ Flash Bekleme";
    case UniteDurumu::MarsDinlenmesi: return "Marş Dinlenmesi";
    case UniteDurumu::MarsAtma: return "Marş Atma";
    case UniteDurumu::MotorRolantiHizi: return "Motor Rölanti Hızı";
    case UniteDurumu::MotorIsinmasi: return "Motor Isınması";
    case UniteDurumu::YuksuzCalisma: return "Yüksüz Çalışma";
    case UniteDurumu::SebekeyeSenkronizasyon: return "Şebekeye Senkronizasyon";
    case UniteDurumu::YukTransferiJeneratoreL: return "Yük Transferi Jeneratöre";
    case UniteDurumu::GenCBAktivasyonu: return "Gen CB Aktivasyonu";
    case UniteDurumu::JeneratorCBZamanlayici: return "Jeneratör CB Zamanlayıcı";
    case UniteDurumu::MasterJeneratorYuklu: return "Master Jeneratör Yüklü";
    case UniteDurumu::PeakLopping: return "Peak Lopping";
    case UniteDurumu::PowerExporting: return "Güç İhracatı";
    case UniteDurumu::SlaveJeneratorYuklu: return "Slave Jeneratör Yüklü";
    case UniteDurumu::SenkronizasyonSebekey: return "Senkronizasyon Şebekeye";
    case UniteDurumu::YukTransferiSebekey: return "Yük Transferi Şebekeye";
    case UniteDurumu::MainsCBAktivasyonu: return "Mains CB Aktivasyonu";
    case UniteDurumu::MainsCBZamanlayici: return "Mains CB Zamanlayıcı";
    case UniteDurumu::SogutmaliDurdurma: return "Soğutmalı Durdurma";
    case UniteDurumu::Soguyor: return "Soğuyor";
    case UniteDurumu::MotorStopRolanti: return "Motor Stop Rölanti";
    case UniteDurumu::AcilDurdurma: return "Acil Durdurma";
    case UniteDurumu::MotorDuruyor: return "Motor Duruyor";
    default: return "Bilinmeyen (" + String(static_cast<uint16_t>(Sistem.Durum)) + ")";
  }
}

String D300Controller::getModAciklama() const {
  switch (Sistem.Mod) {
    case UniteModu::STOP: return "STOP";
    case UniteModu::MANUEL: return "MANUEL";
    case UniteModu::AUTO: return "AUTO";
    case UniteModu::TEST: return "TEST";
    default: return "Bilinmeyen (" + String(static_cast<uint16_t>(Sistem.Mod)) + ")";
  }
}

String D300Controller::getAlarmAciklama(AlarmTipi tip) const {
  switch (tip) {
    case AlarmTipi::Kapatma: return Sistem.KapatmaAlarmi ? "KAPATMA ALARMI AKTİF" : "Kapatma alarmı yok";
    case AlarmTipi::YukAtma: return Sistem.YukAtmaAlarmi ? "YÜK ATMA ALARMI AKTİF" : "Yük atma alarmı yok";
    case AlarmTipi::Uyari: return Sistem.UyariAlarmi ? "UYARI ALARMI AKTİF" : "Uyarı alarmı yok";
    default: return "Bilinmeyen alarm tipi";
  }
}

bool D300Controller::isAlarmActive(AlarmTipi tip) const {
  switch (tip) {
    case AlarmTipi::Kapatma: return Sistem.KapatmaAlarmi;
    case AlarmTipi::YukAtma: return Sistem.YukAtmaAlarmi;
    case AlarmTipi::Uyari: return Sistem.UyariAlarmi;
    default: return false;
  }
}

bool D300Controller::isJeneratorCalisir() const {
  return (Sistem.Durum >= UniteDurumu::MotorRolantiHizi && 
          Sistem.Durum <= UniteDurumu::SlaveJeneratorYuklu);
}

bool D300Controller::isSebekeMevcut() const {
  return (ElektrikSistemi.Sebeke.Toplam.OrtalamaVoltaj > 100.0 && 
          ElektrikSistemi.Sebeke.Frekans > 45.0 && 
          ElektrikSistemi.Sebeke.Frekans < 65.0);
}

bool D300Controller::isSystemHealthy() const {
  return connectionStatus && 
         !Sistem.KapatmaAlarmi && 
         Motor.BataryaVoltaji > 10.0 &&
         (Motor.YagBasinci > 0.5 || !isJeneratorCalisir());
}

void D300Controller::printAllData() const {
  Serial.println("\n========== D-300 MK3 TÜM VERİLER ==========");
  printElektrikselVeriler();
  printMotorVerileri();
  printSistemDurumu();
  printAlarmlar();
  printSayaclar();
  printHarmonikVeriler();
  Serial.println("==========================================");
}

void D300Controller::printElektrikselVeriler() const {
  Serial.println("\n--- ELEKTRİKSEL VERİLER ---");
  Serial.printf("Şebeke:\n");
  Serial.printf("  L1: %.1fV, %.1fA, %.1fkW\n", 
    ElektrikSistemi.Sebeke.L1.Voltaj, ElektrikSistemi.Sebeke.L1.Akim, ElektrikSistemi.Sebeke.L1.AktifGuc);
  Serial.printf("  L2: %.1fV, %.1fA, %.1fkW\n", 
    ElektrikSistemi.Sebeke.L2.Voltaj, ElektrikSistemi.Sebeke.L2.Akim, ElektrikSistemi.Sebeke.L2.AktifGuc);
  Serial.printf("  L3: %.1fV, %.1fA, %.1fkW\n", 
    ElektrikSistemi.Sebeke.L3.Voltaj, ElektrikSistemi.Sebeke.L3.Akim, ElektrikSistemi.Sebeke.L3.AktifGuc);
  Serial.printf("  Toplam: %.1fkW, %.1fHz, PF:%.2f\n", 
    ElektrikSistemi.Sebeke.Toplam.AktifGuc, ElektrikSistemi.Sebeke.Frekans, ElektrikSistemi.Sebeke.Toplam.GucFaktoru);
  
  Serial.printf("Jeneratör:\n");
  Serial.printf("  L1: %.1fV, %.1fA, %.1fkW\n", 
    ElektrikSistemi.Jenerator.L1.Voltaj, ElektrikSistemi.Jenerator.L1.Akim, ElektrikSistemi.Jenerator.L1.AktifGuc);
  Serial.printf("  L2: %.1fV, %.1fA, %.1fkW\n", 
    ElektrikSistemi.Jenerator.L2.Voltaj, ElektrikSistemi.Jenerator.L2.Akim, ElektrikSistemi.Jenerator.L2.AktifGuc);
  Serial.printf("  L3: %.1fV, %.1fA, %.1fkW\n", 
    ElektrikSistemi.Jenerator.L3.Voltaj, ElektrikSistemi.Jenerator.L3.Akim, ElektrikSistemi.Jenerator.L3.AktifGuc);
  Serial.printf("  Toplam: %.1fkW, %.1fHz, PF:%.2f\n", 
    ElektrikSistemi.Jenerator.Toplam.AktifGuc, ElektrikSistemi.Jenerator.Frekans, ElektrikSistemi.Jenerator.Toplam.GucFaktoru);
}

void D300Controller::printMotorVerileri() const {
  Serial.println("\n--- MOTOR VERİLERİ ---");
  Serial.printf("RPM: %.0f | Sıcaklık: %.1f°C | Yağ Basıncı: %.1f bar\n", 
    Motor.RPM, Motor.Sicaklik, Motor.YagBasinci);
  Serial.printf("Yakıt: %.1f%% | Batarya: %.1fV | Şarj: %.1fV\n", 
    Motor.YakitSeviyesi, Motor.BataryaVoltaji, Motor.SarjVoltaji);
  Serial.printf("Yağ Sıcaklık: %.1f°C | Kanopi: %.1f°C | Ortam: %.1f°C\n", 
    Motor.YagSicakligi, Motor.KanopyicSicaklik, Motor.OrtamSicakligi);
}

void D300Controller::printSistemDurumu() const {
  Serial.println("\n--- SİSTEM DURUMU ---");
  Serial.printf("Durum: %s\n", getDurumAciklama().c_str());
  Serial.printf("Mod: %s\n", getModAciklama().c_str());
  Serial.printf("Operasyon Timer: %d saniye\n", Sistem.OperasyonZamanlayici);
  Serial.printf("GOV Kontrol: %.1f%% | AVR Kontrol: %.1f%%\n", 
    Sistem.GOVKontrolCikis, Sistem.AVRKontrolCikis);
  Serial.printf("Cihaz: 0x%04X | HW: v%d | SW: v%d\n", 
    Sistem.CihazKimlik, Sistem.DonanımVersiyon, Sistem.YazilimVersiyon);
  Serial.printf("Bağlantı: %s | Hata Sayısı: %d\n", 
    connectionStatus ? "OK" : "HATA", consecutiveErrors);
}

void D300Controller::printAlarmlar() const {
  Serial.println("\n--- ALARMLAR ---");
  Serial.printf("Kapatma: %s\n", getAlarmAciklama(AlarmTipi::Kapatma).c_str());
  Serial.printf("Yük Atma: %s\n", getAlarmAciklama(AlarmTipi::YukAtma).c_str());
  Serial.printf("Uyarı: %s\n", getAlarmAciklama(AlarmTipi::Uyari).c_str());
  Serial.printf("Sistem Sağlığı: %s\n", isSystemHealthy() ? "SAĞLIKLI" : "SORUNLU");
}

void D300Controller::printSayaclar() const {
  Serial.println("\n--- SAYAÇLAR ---");
  Serial.printf("Jeneratör Çalışma: %lu kez\n", Sayac.JeneratorCalismaAdedi);
  Serial.printf("Marş Sayısı: %lu kez\n", Sayac.JeneratorMarsAdedi);
  Serial.printf("Yüklü Çalışma: %lu kez\n", Sayac.JeneratorYukluCalisma);
  Serial.printf("Çalışma Saati: %.1f saat\n", Sayac.MotorCalismaSaati);
  Serial.printf("Son Servisten Beri: %.1f saat (%.1f gün)\n", 
    Sayac.SonServistenBeriSaat, Sayac.SonServistenBeriGun);
  Serial.printf("Toplam Enerji: %.1f kWh\n", Sayac.ToplamAktifEnerji);
  Serial.printf("Yakıt Sayacı: %.1f L\n", Sayac.YakitSayaci);
}

void D300Controller::printHarmonikVeriler() const {
  Serial.println("\n--- HARMONİK VERİLER ---");
  Serial.printf("Kanal: %d | THD: %.2f%% | Fundamental: %.2f%%\n", 
    static_cast<int>(Harmonik.SeciliKanal), Harmonik.THD, Harmonik.Fundamental);
  Serial.printf("H3: %.2f%% | H5: %.2f%% | H7: %.2f%% | H9: %.2f%%\n", 
    Harmonik.H3, Harmonik.H5, Harmonik.H7, Harmonik.H9);
}

String D300Controller::getDataAsJSON() const {
  String json = "{";
  json += "\"timestamp\":" + String(millis()) + ",";
  json += "\"connection\":" + String(connectionStatus ? "true" : "false") + ",";
  
  // Elektriksel sistem
  json += "\"electrical\":{";
  json += "\"mains\":{";
  json += "\"voltage\":{\"l1\":" + String(ElektrikSistemi.Sebeke.L1.Voltaj, 1) + 
          ",\"l2\":" + String(ElektrikSistemi.Sebeke.L2.Voltaj, 1) + 
          ",\"l3\":" + String(ElektrikSistemi.Sebeke.L3.Voltaj, 1) + "},";
  json += "\"frequency\":" + String(ElektrikSistemi.Sebeke.Frekans, 1) + ",";
  json += "\"power\":" + String(ElektrikSistemi.Sebeke.Toplam.AktifGuc, 1) + "},";
  
  json += "\"generator\":{";
  json += "\"voltage\":{\"l1\":" + String(ElektrikSistemi.Jenerator.L1.Voltaj, 1) + 
          ",\"l2\":" + String(ElektrikSistemi.Jenerator.L2.Voltaj, 1) + 
          ",\"l3\":" + String(ElektrikSistemi.Jenerator.L3.Voltaj, 1) + "},";
  json += "\"frequency\":" + String(ElektrikSistemi.Jenerator.Frekans, 1) + ",";
  json += "\"power\":" + String(ElektrikSistemi.Jenerator.Toplam.AktifGuc, 1) + "}},";
  
  // Motor verileri
  json += "\"engine\":{";
  json += "\"rpm\":" + String(Motor.RPM, 0) + ",";
  json += "\"temperature\":" + String(Motor.Sicaklik, 1) + ",";
  json += "\"oil_pressure\":" + String(Motor.YagBasinci, 1) + ",";
  json += "\"fuel_level\":" + String(Motor.YakitSeviyesi, 1) + ",";
  json += "\"battery_voltage\":" + String(Motor.BataryaVoltaji, 1) + "},";
  
  // Sistem durumu
  json += "\"system\":{";
  json += "\"status\":" + String(static_cast<int>(Sistem.Durum)) + ",";
  json += "\"mode\":" + String(static_cast<int>(Sistem.Mod)) + ",";
  json += "\"alarms\":{";
  json += "\"shutdown\":" + String(Sistem.KapatmaAlarmi ? "true" : "false") + ",";
  json += "\"loaddump\":" + String(Sistem.YukAtmaAlarmi ? "true" : "false") + ",";
  json += "\"warning\":" + String(Sistem.UyariAlarmi ? "true" : "false") + "}},";
  
  json += "\"healthy\":" + String(isSystemHealthy() ? "true" : "false");
  json += "}";
  
  return json;
}

String D300Controller::getBasicDataAsJSON() const {
  String json = "{";
  json += "\"gen_power\":" + String(ElektrikSistemi.Jenerator.Toplam.AktifGuc, 1) + ",";
  json += "\"gen_freq\":" + String(ElektrikSistemi.Jenerator.Frekans, 1) + ",";
  json += "\"mains_freq\":" + String(ElektrikSistemi.Sebeke.Frekans, 1) + ",";
  json += "\"rpm\":" + String(Motor.RPM, 0) + ",";
  json += "\"battery\":" + String(Motor.BataryaVoltaji, 1) + ",";
  json += "\"fuel\":" + String(Motor.YakitSeviyesi, 1) + ",";
  json += "\"status\":" + String(static_cast<int>(Sistem.Durum)) + ",";
  json += "\"mode\":" + String(static_cast<int>(Sistem.Mod)) + ",";
  json += "\"alarms\":" + String((Sistem.KapatmaAlarmi || Sistem.YukAtmaAlarmi || Sistem.UyariAlarmi) ? "true" : "false") + ",";
  json += "\"connected\":" + String(connectionStatus ? "true" : "false");
  json += "}";
  return json;
}

bool D300Controller::isConnected() const {
  // Cihaz kimlik register'ını okuyarak bağlantı testi yap
  uint16_t testValue;
  D300Controller* nonConstThis = const_cast<D300Controller*>(this);
  bool result = nonConstThis->read16BitValue(10609, testValue);
  
  // D-300 cihazları için beklenen kimlik değeri 0xD300
  if (result && (testValue == 0xD300 || testValue == 0xD500 || testValue == 0xD700)) {
    return true;
  }
  return false;
}

void D300Controller::setSlaveID(uint8_t newSlaveID) {
  if (newSlaveID >= 1 && newSlaveID <= 240) {
    slaveID = newSlaveID;
    node.begin(slaveID, *modbusSerial);
  }
}

void D300Controller::setTimeout(unsigned long timeout) {
  // ModbusMaster kütüphanesi için timeout ayarı
  // Bu fonksiyon kütüphane versiyonuna göre değişebilir
}

#endif // D300_CONTROLLER_H

/*
 * KULLANIM ÖRNEĞİ:
 * 
 * #include "D300Controller.h"
 * 
 * D300Controller genset(1, 16, 17); // SlaveID=1, RX=GPIO16, TX=GPIO17
 * 
 * void setup() {
 *   Serial.begin(115200);
 *   
 *   if (genset.begin(9600, 3000)) { // 9600 baud, 3 saniye güncelleme
 *     Serial.println("D-300 MK3 bağlantısı başarılı!");
 *     genset.enableAutoUpdate(true);
 *   } else {
 *     Serial.println("Bağlantı hatası!");
 *   }
 * }
 * 
 * void loop() {
 *   genset.handle(); // Otomatik güncelleme için
 *   
 *   // Veri okuma örnekleri:
 *   if (genset.isConnected()) {
 *     float power = genset.ElektrikSistemi.Jenerator.Toplam.AktifGuc;
 *     float voltage = genset.ElektrikSistemi.Sebeke.L1.Voltaj;
 *     float rpm = genset.Motor.RPM;
 *     float fuel = genset.Motor.YakitSeviyesi;
 *     
 *     Serial.printf("Güç: %.1f kW, RPM: %.0f, Yakıt: %.1f%%\n", 
 *                   power, rpm, fuel);
 *     
 *     // JSON çıktı
 *     Serial.println(genset.getBasicDataAsJSON());
 *     
 *     // Kontrol örnekleri:
 *     if (genset.Motor.YakitSeviyesi < 20.0) {
 *       Serial.println("DÜŞÜK YAKIT UYARISI!");
 *     }
 *     
 *     if (genset.isAlarmActive(AlarmTipi::Kapatma)) {
 *       Serial.println("KAPATMA ALARMI!");
 *       // genset.emergencyStop();
 *     }
 *   }
 *   
 *   delay(1000);
 * }
 */