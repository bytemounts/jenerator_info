/*
 * D300_Simulator.ino
 * D-300 MK3 Modbus RTU Master Simulator
 * ESP32 üzerinde D-300 cihazını simüle eder
 * 
 * Bu simulator gerçek D-300 cihazı gibi davranır ve
 * Modbus RTU slave olarak çalışır
 * 
 * Bağlantı:
 * Simulator ESP32    Test ESP32 (D300Controller)
 * GPIO16 (RX) -----> GPIO17 (TX)
 * GPIO17 (TX) -----> GPIO16 (RX)
 * GND -------------- GND
 * 
 * Simulator Slave ID: 1
 * Test Controller Master olarak bağlanır
 */
//#include <SoftwareSerial.h>
#include <ModbusMaster.h>

#define RS485_RX_PIN 5
#define RS485_TX_PIN 15

/*
SoftwareSerial modbusSerial(RX_PIN, TX_PIN); // RX, TX

// RS485 bağlantı pinleri
#define RS485_RX_PIN 22
#define RS485_TX_PIN 23
#define RS485_DE_PIN 4
#define RS485_RE_PIN 2 // MAX485 RE pin
//#define RS485_RE_PIN 2
*/
// Modbus Slave emülasyonu için
#define SLAVE_ID 1

// Simülasyon parametreleri
bool simulatorRunning = true;
bool generatorRunning = false;
bool mainsAvailable = true;
uint16_t systemStatus = 0;  // 0=dinlenme, 13=yüklü çalışma
uint16_t systemMode = 4;    // 4=AUTO mod
unsigned long lastUpdate = 0;
float simulationTime = 0;

// Simüle edilecek veriler
struct SimulatedData {
  // Voltajlar (x10)
  uint32_t mainsL1Voltage = 2300;     // 230.0V
  uint32_t mainsL2Voltage = 2310;     // 231.0V  
  uint32_t mainsL3Voltage = 2290;     // 229.0V
  uint32_t genL1Voltage = 2300;       // 230.0V
  uint32_t genL2Voltage = 2300;       // 230.0V
  uint32_t genL3Voltage = 2300;       // 230.0V
  
  // Akımlar (x10)
  uint32_t mainsL1Current = 0;        // 0.0A
  uint32_t mainsL2Current = 0;        // 0.0A
  uint32_t mainsL3Current = 0;        // 0.0A
  uint32_t genL1Current = 250;        // 25.0A
  uint32_t genL2Current = 240;        // 24.0A
  uint32_t genL3Current = 260;        // 26.0A
  
  // Güçler (x10)
  uint32_t mainsTotalPower = 0;       // 0.0kW
  uint32_t genTotalPower = 175;       // 17.5kW
  
  // Frekanslar (x100)
  uint16_t mainsFrequency = 5000;     // 50.00Hz
  uint16_t genFrequency = 5000;       // 50.00Hz
  
  // Motor verileri
  uint16_t motorRPM = 1500;           // 1500 rpm
  uint16_t motorTemp = 750;           // 75.0°C (x10)
  uint16_t oilPressure = 35;          // 3.5 bar (x10)
  uint16_t fuelLevel = 750;           // 75.0% (x10)
  uint16_t batteryVoltage = 1280;     // 12.80V (x100)
  
  // Sistem bilgileri
  uint16_t deviceID = 0xD300;
  uint16_t hwVersion = 1;
  uint16_t swVersion = 56;
  
  // Alarmlar (16 register = 256 bit)
  uint16_t shutdownAlarms[16] = {0};
  uint16_t loaddumpAlarms[16] = {0};
  uint16_t warningAlarms[16] = {0};
} simData;

HardwareSerial modbusSerial(2);

void setup() {
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("    D-300 MK3 Modbus RTU Simulator");
  Serial.println("    Slave ID: 1");
  Serial.println("========================================");
  
  // RS485 serial başlat
  modbusSerial.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  //modbusSerial.begin(9600);
  Serial.println("Simulator başlatıldı!");
  Serial.println("Test cihazı bağlanmayı bekliyor...");
  Serial.println("Komutlar:");
  Serial.println("  's' - Jeneratör başlat");
  Serial.println("  'x' - Jeneratör durdur");
  Serial.println("  'm' - Şebekeyi kes/bağla");
  Serial.println("  'a' - Alarm simüle et");
  Serial.println("  'f' - Yakıt seviyesi değiştir");
  Serial.println("  'r' - RPM değiştir");
  Serial.println("========================================\n");
  
  // İlk veri güncellemesi
  updateSimulationData();
}

void loop() {
  // Modbus isteklerini işle
  handleModbusRequests();
  
  // Simülasyon verilerini güncelle
  if (millis() - lastUpdate > 1000) {
    updateSimulationData();
    lastUpdate = millis();
  }
  
  // Serial komutları
  handleSerialCommands();
  
  delay(10);
}

void handleModbusRequests() {
  if (modbusSerial.available()) {
    uint8_t buffer[256];
    int len = 0;
    
    // Veri oku
    while (modbusSerial.available() && len < 256) {
      buffer[len++] = modbusSerial.read();
      delay(1);
    }
    
    if (len >= 8) {  // Minimum Modbus frame
      processModbusFrame(buffer, len);
    }
  }
}

void processModbusFrame(uint8_t* frame, int len) {
  if (frame[0] != SLAVE_ID) return;  // Bizim slave ID'miz değil
  
  uint8_t function = frame[1];
  uint16_t address = (frame[2] << 8) | frame[3];
  uint16_t quantity = (frame[4] << 8) | frame[5];
  
  Serial.printf("Modbus istek: Func=%d, Addr=%d, Qty=%d\n", function, address, quantity);
  
  switch (function) {
    case 3:  // Read Holding Registers
      handleReadHoldingRegisters(address, quantity);
      break;
      
    case 6:  // Write Single Register
      {
        uint16_t value = (frame[4] << 8) | frame[5];
        handleWriteSingleRegister(address, value);
      }
      break;
      
    default:
      sendExceptionResponse(function, 1);  // Illegal function
      break;
  }
}

void handleReadHoldingRegisters(uint16_t address, uint16_t quantity) {
  uint8_t response[256];
  int responseLen = 0;
  
  // Modbus frame başlangıcı
  response[responseLen++] = SLAVE_ID;
  response[responseLen++] = 3;  // Function code
  response[responseLen++] = quantity * 2;  // Byte count
  
  // Register verilerini ekle
  for (int i = 0; i < quantity; i++) {
    uint16_t value = getRegisterValue(address + i);
    response[responseLen++] = (value >> 8) & 0xFF;  // High byte
    response[responseLen++] = value & 0xFF;         // Low byte
  }
  
  // CRC ekle
  uint16_t crc = calculateCRC(response, responseLen);
  response[responseLen++] = crc & 0xFF;
  response[responseLen++] = (crc >> 8) & 0xFF;
  
  // Gönder
  modbusSerial.write(response, responseLen);
  Serial.printf("Yanıt gönderildi: %d byte\n", responseLen);
}

void handleWriteSingleRegister(uint16_t address, uint16_t value) {
  Serial.printf("Write Register: Addr=%d, Value=0x%04X\n", address, value);
  
  switch (address) {
    case 8193:  // Buton simülasyonu
      handleButtonSimulation(value);
      break;
      
    case 8210:  // Reset komutu
      if (value == 14536) {
        Serial.println("RESET komutu alındı!");
        // Reset simülasyonu
        systemStatus = 0;
        generatorRunning = false;
      }
      break;
  }
  
  // Echo response gönder
  uint8_t response[8];
  response[0] = SLAVE_ID;
  response[1] = 6;
  response[2] = (address >> 8) & 0xFF;
  response[3] = address & 0xFF;
  response[4] = (value >> 8) & 0xFF;
  response[5] = value & 0xFF;
  
  uint16_t crc = calculateCRC(response, 6);
  response[6] = crc & 0xFF;
  response[7] = (crc >> 8) & 0xFF;
  
  modbusSerial.write(response, 8);
}

void handleButtonSimulation(uint16_t buttonMask) {
  Serial.printf("Buton simülasyonu: 0x%04X\n", buttonMask);
  
  if (buttonMask & 0x0001) {  // STOP
    Serial.println("STOP butonu - Jeneratör durduruluyor");
    generatorRunning = false;
    systemStatus = 0;  // Dinlenme
    systemMode = 1;    // STOP mode
  }
  
  if (buttonMask & 0x0004) {  // AUTO
    Serial.println("AUTO butonu - AUTO mod");
    systemMode = 4;  // AUTO mode
    if (mainsAvailable) {
      systemStatus = 0;  // Dinlenme
      generatorRunning = false;
    } else {
      systemStatus = 13; // Yüklü çalışma
      generatorRunning = true;
    }
  }
  
  if (buttonMask & 0x0002) {  // MANUEL/RUN
    Serial.println("MANUEL/RUN butonu - Jeneratör başlatılıyor");
    generatorRunning = true;
    systemStatus = 13;  // Yüklü çalışma
    systemMode = 2;     // MANUEL mode
  }
  
  if (buttonMask & 0x0008) {  // TEST
    Serial.println("TEST butonu - Test modu");
    systemMode = 8;  // TEST mode
    generatorRunning = true;
    systemStatus = 13;
  }
}

uint16_t getRegisterValue(uint16_t address) {
  switch (address) {
    // Şebeke voltajları (32-bit, high word)
    case 10240: return (simData.mainsL1Voltage >> 16) & 0xFFFF;
    case 10241: return simData.mainsL1Voltage & 0xFFFF;
    case 10242: return (simData.mainsL2Voltage >> 16) & 0xFFFF;
    case 10243: return simData.mainsL2Voltage & 0xFFFF;
    case 10244: return (simData.mainsL3Voltage >> 16) & 0xFFFF;
    case 10245: return simData.mainsL3Voltage & 0xFFFF;
    
    // Jeneratör voltajları (32-bit)
    case 10246: return (simData.genL1Voltage >> 16) & 0xFFFF;
    case 10247: return simData.genL1Voltage & 0xFFFF;
    case 10248: return (simData.genL2Voltage >> 16) & 0xFFFF;
    case 10249: return simData.genL2Voltage & 0xFFFF;
    case 10250: return (simData.genL3Voltage >> 16) & 0xFFFF;
    case 10251: return simData.genL3Voltage & 0xFFFF;
    
    // Akımlar (32-bit)
    case 10264: return (simData.mainsL1Current >> 16) & 0xFFFF;
    case 10265: return simData.mainsL1Current & 0xFFFF;
    case 10266: return (simData.mainsL2Current >> 16) & 0xFFFF;
    case 10267: return simData.mainsL2Current & 0xFFFF;
    case 10268: return (simData.mainsL3Current >> 16) & 0xFFFF;
    case 10269: return simData.mainsL3Current & 0xFFFF;
    
    case 10270: return (simData.genL1Current >> 16) & 0xFFFF;
    case 10271: return simData.genL1Current & 0xFFFF;
    case 10272: return (simData.genL2Current >> 16) & 0xFFFF;
    case 10273: return simData.genL2Current & 0xFFFF;
    case 10274: return (simData.genL3Current >> 16) & 0xFFFF;
    case 10275: return simData.genL3Current & 0xFFFF;
    
    // Güçler (32-bit)
    case 10292: return (simData.mainsTotalPower >> 16) & 0xFFFF;
    case 10293: return simData.mainsTotalPower & 0xFFFF;
    case 10294: return (simData.genTotalPower >> 16) & 0xFFFF;
    case 10295: return simData.genTotalPower & 0xFFFF;
    
    // Frekanslar (16-bit)
    case 10338: return simData.mainsFrequency;
    case 10339: return simData.genFrequency;
    
    // Motor verileri
    case 10361: return simData.oilPressure;      // Yağ basıncı
    case 10362: return simData.motorTemp;        // Motor sıcaklık
    case 10363: return simData.fuelLevel;        // Yakıt seviyesi
    case 10376: return simData.motorRPM;         // RPM
    case 10341: return simData.batteryVoltage;   // Batarya voltajı
    
    // Sistem durumu
    case 10604: return systemStatus;
    case 10605: return systemMode;
    case 10606: return 0;  // Operasyon zamanlayıcı
    case 10607: return generatorRunning ? 500 : 0;  // GOV kontrol
    case 10608: return generatorRunning ? 750 : 0;  // AVR kontrol
    case 10609: return simData.deviceID;
    case 10610: return simData.hwVersion;
    case 10611: return simData.swVersion;
    
    // Alarm registerleri (10504-10551 arası tümü 0)
    default:
      if (address >= 10504 && address <= 10551) {
        return 0;  // Alarm yok
      }
      return 0;
  }
}

void updateSimulationData() {
  simulationTime += 1.0;
  
  if (generatorRunning) {
    // Jeneratör çalışıyor
    simData.genTotalPower = 150 + (sin(simulationTime * 0.1) * 25);  // 12.5-17.5 kW
    simData.genL1Current = 200 + (sin(simulationTime * 0.15) * 50);
    simData.genL2Current = 210 + (sin(simulationTime * 0.12) * 45);
    simData.genL3Current = 220 + (sin(simulationTime * 0.18) * 40);
    
    simData.motorRPM = 1500 + (sin(simulationTime * 0.05) * 10);
    simData.motorTemp = 750 + (sin(simulationTime * 0.02) * 50);  // 70-80°C
    simData.oilPressure = 35 + (sin(simulationTime * 0.08) * 5);   // 3.0-4.0 bar
    
    // Yakıt azalması
    if (simData.fuelLevel > 0) {
      simData.fuelLevel -= 1;  // Her saniye %0.1 azalır
    }
  } else {
    // Jeneratör durmuş
    simData.genTotalPower = 0;
    simData.genL1Current = 0;
    simData.genL2Current = 0;
    simData.genL3Current = 0;
    simData.motorRPM = 0;
    simData.motorTemp = 250 + (sin(simulationTime * 0.01) * 50); // Soğuyor
    simData.oilPressure = 0;
  }
  
  // Şebeke durumu
  if (mainsAvailable) {
    simData.mainsL1Voltage = 2300 + (sin(simulationTime * 0.2) * 20);
    simData.mainsL2Voltage = 2310 + (sin(simulationTime * 0.25) * 15);
    simData.mainsL3Voltage = 2290 + (sin(simulationTime * 0.22) * 18);
    simData.mainsFrequency = 5000 + (sin(simulationTime * 0.1) * 10);
  } else {
    simData.mainsL1Voltage = 0;
    simData.mainsL2Voltage = 0;
    simData.mainsL3Voltage = 0;
    simData.mainsFrequency = 0;
  }
  
  // Batarya durumu
  if (generatorRunning) {
    simData.batteryVoltage = min(1400, simData.batteryVoltage + 2); // Şarj oluyor
  } else {
    simData.batteryVoltage = max(1100, simData.batteryVoltage - 1); // Boşalıyor
  }
}

void handleSerialCommands() {
  if (Serial.available()) {
    char cmd = Serial.read();
    
    switch (cmd) {
      case 's':
        Serial.println("Manuel komut: Jeneratör başlatıldı");
        generatorRunning = true;
        systemStatus = 13;
        break;
        
      case 'x':
        Serial.println("Manuel komut: Jeneratör durduruldu");
        generatorRunning = false;
        systemStatus = 0;
        break;
        
      case 'm':
        mainsAvailable = !mainsAvailable;
        Serial.printf("Manuel komut: Şebeke %s\n", mainsAvailable ? "bağlandı" : "kesildi");
        break;
        
      case 'a':
        Serial.println("Manuel komut: Alarm simülasyonu (henüz uygulanmadı)");
        break;
        
      case 'f':
        simData.fuelLevel = (simData.fuelLevel < 500) ? 1000 : 200;
        Serial.printf("Manuel komut: Yakıt seviyesi %.1f%% olarak ayarlandı\n", simData.fuelLevel / 10.0);
        break;
        
      case 'r':
        if (generatorRunning) {
          simData.motorRPM = (simData.motorRPM < 1500) ? 1800 : 1200;
          Serial.printf("Manuel komut: RPM %d olarak ayarlandı\n", simData.motorRPM);
        }
        break;
        
      case '?':
      case 'h':
        Serial.println("\nKomut listesi:");
        Serial.println("  s - Jeneratör başlat");
        Serial.println("  x - Jeneratör durdur");
        Serial.println("  m - Şebekeyi kes/bağla");
        Serial.println("  f - Yakıt seviyesi değiştir");
        Serial.println("  r - RPM değiştir");
        Serial.println("  ? - Bu yardım");
        break;
    }
  }
}

void sendExceptionResponse(uint8_t function, uint8_t exception) {
  uint8_t response[5];
  response[0] = SLAVE_ID;
  response[1] = function | 0x80;  // Exception flag
  response[2] = exception;
  
  uint16_t crc = calculateCRC(response, 3);
  response[3] = crc & 0xFF;
  response[4] = (crc >> 8) & 0xFF;
  modbusSerial.write(response, 5);
}

uint16_t calculateCRC(uint8_t* data, int len) {
  uint16_t crc = 0xFFFF;
  
  for (int i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  
  return crc;
}