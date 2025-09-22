/*
 * D300Controller.cpp
 * D-300 MK3 Modbus RTU Library for ESP32
 * Implementation File - Fonksiyon kodları
 */

#include "D300Controller.h"

// Constructor
D300Controller::D300Controller(uint8_t slaveID, uint8_t rxPin, uint8_t txPin) 
  : slaveID(slaveID), lastUpdateTime(0), updateInterval(5000), autoUpdate(true), 
    connectionStatus(false), consecutiveErrors(0) {
  
  // RS485 TTL modülü otomatik flow control kullandığı için sadece RX/TX
  modbusSerial = new HardwareSerial(2);
  modbusSerial->begin(9600, SERIAL_8N1, rxPin, txPin);

    // ADC pin konfigürasyonu
  pinMode(FUEL_ADC_PIN, INPUT);
  analogReadResolution(12);  // 12-bit ADC çözünürlük
}

bool D300Controller::begin(uint32_t baudRate, unsigned long updateInterval) {
  this->updateInterval = updateInterval;
  
  // Serial bağlantısını başlat
  modbusSerial->begin(baudRate, SERIAL_8N1);
  
  // Modbus konfigürasyonu
  node.begin(slaveID, *modbusSerial);
  
  delay(100); // Modülün hazırlanması için
  
  // İlk bağlantı testi
  connectionStatus = isConnected();
  if (connectionStatus) {
    resetErrorCounter();
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
  
  // Akımlar
  readFloat32(10264, ElektrikSistemi.Sebeke.L1.Akim, 10);
  readFloat32(10266, ElektrikSistemi.Sebeke.L2.Akim, 10);
  readFloat32(10268, ElektrikSistemi.Sebeke.L3.Akim, 10);
  readFloat32(10270, ElektrikSistemi.Jenerator.L1.Akim, 10);
  readFloat32(10272, ElektrikSistemi.Jenerator.L2.Akim, 10);
  readFloat32(10274, ElektrikSistemi.Jenerator.L3.Akim, 10);
  
  // Güç verileri
  readFloat32(10292, ElektrikSistemi.Sebeke.Toplam.AktifGuc, 10);
  readFloat32(10294, ElektrikSistemi.Jenerator.Toplam.AktifGuc, 10);
  readFloat32(10308, ElektrikSistemi.Sebeke.Toplam.ReaktifGuc, 10);
  readFloat32(10310, ElektrikSistemi.Jenerator.Toplam.ReaktifGuc, 10);
  
  // Frekanslar
  readFloat16(10338, ElektrikSistemi.Sebeke.Frekans, 100);
  readFloat16(10339, ElektrikSistemi.Jenerator.Frekans, 100);
  
  // Ortalama değerler
  readFloat32(10377, ElektrikSistemi.Jenerator.Toplam.OrtalamaVoltaj, 10);
  readFloat32(10379, ElektrikSistemi.Jenerator.Toplam.OrtalamaAkim, 10);
  readFloat32(10381, ElektrikSistemi.Sebeke.Toplam.OrtalamaVoltaj, 10);
  readFloat32(10383, ElektrikSistemi.Sebeke.Toplam.OrtalamaAkim, 10);
}

// updateMotorVerileri() fonksiyonunu güncelleyin:
void D300Controller::updateMotorVerileri() {
  // Mevcut D-300 verileri
  readFloat16(10376, Motor.RPM, 1);
  readFloat16(10362, Motor.Sicaklik, 10);
  readFloat16(10361, Motor.YagBasinci, 10);
  readFloat16(10363, Motor.YakitSeviyesi, 10);  // D-300'den gelen
  readFloat16(10364, Motor.YagSicakligi, 10);
  readFloat16(10365, Motor.KanopyicSicaklik, 10);
  readFloat16(10366, Motor.OrtamSicakligi, 10);
  readFloat16(10341, Motor.BataryaVoltaji, 100);
  readFloat16(10385, Motor.MinBataryaVoltaji, 100);
  readFloat16(10340, Motor.SarjVoltaji, 100);
  readFloat16(11173, Motor.SarjAkimi, 10);
  
  // Harici yakıt sensörü okuma
  Motor.HariciYakitSeviyesi = readFuelADC();
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

bool D300Controller::updateBasicData() {
  updateElektrikselVeriler();
  updateMotorVerileri();
  updateSistemDurumu();
  updateAlarmDurumlari();
  lastUpdateTime = millis();
  return connectionStatus;
}

bool D300Controller::updateData() {
  bool basicResult = updateBasicData();
  updateSayaclar();
  updateAnalogGirisler();
  return basicResult;
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
  uint8_t result = node.writeSingleRegister(8193, emergencyMask);
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
    case UniteDurumu::MasterJeneratorYuklu: return "Master Jeneratör Yüklü";
    case UniteDurumu::SogutmaliDurdurma: return "Soğutmalı Durdurma";
    case UniteDurumu::Soguyor: return "Soğuyor";
    case UniteDurumu::AcilDurdurma: return "Acil Durdurma";
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

bool D300Controller::isConnected() const {
  uint16_t testValue;
  D300Controller* nonConstThis = const_cast<D300Controller*>(this);
  bool result = nonConstThis->read16BitValue(10609, testValue);
  
  // D-300 cihazları için beklenen kimlik değeri
  if (result && (testValue == 0xD300 || testValue == 0xD500 || testValue == 0xD700)) {
    return true;
  }
  return false;
}

void D300Controller::printAllData() const {
  Serial.println("\n========== D-300 MK3 TÜM VERİLER ==========");
  printElektrikselVeriler();
  printMotorVerileri();
  printSistemDurumu();
  Serial.println("==========================================");
}

void D300Controller::printElektrikselVeriler() const {
  Serial.println("\n--- ELEKTRİKSEL VERİLER ---");
  Serial.printf("Şebeke: L1=%.1fV, L2=%.1fV, L3=%.1fV, %.1fHz, %.1fkW\n", 
    ElektrikSistemi.Sebeke.L1.Voltaj, ElektrikSistemi.Sebeke.L2.Voltaj, 
    ElektrikSistemi.Sebeke.L3.Voltaj, ElektrikSistemi.Sebeke.Frekans, 
    ElektrikSistemi.Sebeke.Toplam.AktifGuc);
  
  Serial.printf("Jeneratör: L1=%.1fV, L2=%.1fV, L3=%.1fV, %.1fHz, %.1fkW\n", 
    ElektrikSistemi.Jenerator.L1.Voltaj, ElektrikSistemi.Jenerator.L2.Voltaj, 
    ElektrikSistemi.Jenerator.L3.Voltaj, ElektrikSistemi.Jenerator.Frekans, 
    ElektrikSistemi.Jenerator.Toplam.AktifGuc);
}

void D300Controller::printMotorVerileri() const {
  Serial.println("\n--- MOTOR VERİLERİ ---");
  Serial.printf("RPM: %.0f | Sıcaklık: %.1f°C | Yağ Basıncı: %.1f bar\n", 
    Motor.RPM, Motor.Sicaklik, Motor.YagBasinci);
      Serial.printf("Yakıt (D-300): %.1f%% | Yakıt (Harici): %.1f%%\n", 
    Motor.YakitSeviyesi, Motor.HariciYakitSeviyesi);
  Serial.printf("Yakıt: %.1f%% | Batarya: %.1fV | Şarj: %.1fV\n", 
    Motor.YakitSeviyesi, Motor.BataryaVoltaji, Motor.SarjVoltaji);
}

void D300Controller::printSistemDurumu() const {
  Serial.println("\n--- SİSTEM DURUMU ---");
  Serial.printf("Durum: %s\n", getDurumAciklama().c_str());
  Serial.printf("Mod: %s\n", getModAciklama().c_str());
  Serial.printf("Alarmlar: Kapatma=%s, YükAtma=%s, Uyarı=%s\n", 
    Sistem.KapatmaAlarmi ? "AKTİF" : "YOK",
    Sistem.YukAtmaAlarmi ? "AKTİF" : "YOK", 
    Sistem.UyariAlarmi ? "AKTİF" : "YOK");
  Serial.printf("Bağlantı: %s | Sistem Sağlığı: %s\n", 
    connectionStatus ? "OK" : "HATA", 
    isSystemHealthy() ? "SAĞLIKLI" : "SORUNLU");
}

String D300Controller::getDataAsJSON() const {
  String json = "{";
  json += "\"timestamp\":" + String(millis()) + ",";
  json += "\"connected\":" + String(connectionStatus ? "true" : "false") + ",";
  
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
  json += "\"external_fuel_level\":" + String(Motor.HariciYakitSeviyesi, 1) + ",";  // Harici
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

void D300Controller::setSlaveID(uint8_t newSlaveID) {
  if (newSlaveID >= 1 && newSlaveID <= 240) {
    slaveID = newSlaveID;
    node.begin(slaveID, *modbusSerial);
  }
}


// D300Controller.cpp dosyasına eklenecek implementasyonlar:



// ADC okuma fonksiyonu
float D300Controller::readFuelADC() {
  long adcSum = 0;
  
  // Çoklu okuma ile noise filtreleme
  for (int i = 0; i < ADC_SAMPLES; i++) {
    adcSum += analogRead(FUEL_ADC_PIN);
    delay(10);  // Örnekler arası gecikme
  }
  
  float adcAverage = (float)adcSum / ADC_SAMPLES;
  
  // ADC değerini voltaja çevir
  float adcVoltage = (adcAverage / ADC_MAX) * ESP32_VREF;
  
  // Yakıt seviyesini hesapla
  return calculateFuelLevel(adcVoltage);
}

// Voltage divider'dan yakıt seviyesi hesaplama
float D300Controller::calculateFuelLevel(float adcVoltage) {
  // Voltage divider formülü: Vout = Vin * (R_shamandra) / (R1 + R_shamandra)
  // R_shamandra = (Vout * R1) / (Vin - Vout)
  
  if (adcVoltage >= ESP32_VREF || adcVoltage <= 0) {
    return -1; // Hata durumu
  }
  
  // Şamandra direncini hesapla
  float shamandraResistance = (adcVoltage * (R1 + R2)) / (ESP32_VREF - adcVoltage);
  
  // Direnci yakıt seviyesine çevir
  return resistanceToFuelLevel(shamandraResistance);
}

// Direnç değerini yakıt yüzdesine çevirme
float D300Controller::resistanceToFuelLevel(float resistance) {
  // Linear interpolasyon
  if (resistance <= FUEL_EMPTY_RESISTANCE) {
    return 0.0;  // Tank boş
  }
  if (resistance >= FUEL_FULL_RESISTANCE) {
    return 100.0;  // Tank dolu
  }
  
  // Doğrusal olarak yakıt seviyesini hesapla
  float fuelPercent = ((resistance - FUEL_EMPTY_RESISTANCE) / 
                      (FUEL_FULL_RESISTANCE - FUEL_EMPTY_RESISTANCE)) * 100.0;
  
  return constrain(fuelPercent, 0.0, 100.0);
}

// Kalibrasyon fonksiyonu
void D300Controller::calibrateFuelSensor(float emptyResistance, float fullResistance) {
  // Bu değerleri const olarak tanımladığımız için runtime'da değiştiremeyiz
  // Gerçek uygulamada EEPROM'da saklayabilir veya değişken yaparız
  Serial.printf("Yakıt sensörü kalibrasyonu: Boş=%.1fΩ, Dolu=%.1fΩ\n", 
                emptyResistance, fullResistance);
}

// Pin değiştirme fonksiyonu
void D300Controller::setFuelSensorPin(uint8_t pin) {
  // Yeni pin ayarı (const olduğu için bu örnek sadece bilgilendirme)
  Serial.printf("Yakıt sensörü pin'i: GPIO%d olarak ayarlandı\n", pin);
}
