/*
 * D300_WebServer.ino
 * D-300 MK3 Web Monitoring System
 * ESP32 + RS485 + Web Server
 * 
 * Modern, responsive web arayüzü ile jeneratör takibi
 * Gerçek zamanlı veri güncellemesi
 * 
 * Kullanım:
 * 1. WiFi ayarlarını güncelleyin
 * 2. ESP32'ye yükleyin  
 * 3. Serial Monitor'den IP adresini alın
 * 4. Web tarayıcıda IP adresini açın
 */

#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include "D300Controller.h"

// WiFi ayarları - BU BÖLÜMÜ KENDİ AĞINIZA GÖRE GÜNCELLEYIN
const char* ssid = "WiFi_AGINIZ";        // WiFi ağ adınız
const char* password = "WiFi_SIFRENIZ";  // WiFi şifreniz

// Web server ve D-300 controller
WebServer server(80);
D300Controller genset(1, 16, 17);

// Güncelleme süreleri
unsigned long lastDataUpdate = 0;
const unsigned long DATA_UPDATE_INTERVAL = 2000; // 2 saniye

void setup() {
  Serial.begin(115200);
  
  Serial.println("\n========================================");
  Serial.println("    D-300 MK3 Web Monitoring System");
  Serial.println("========================================");
  
  // D-300 MK3 bağlantısı
  Serial.println("🔗 Jeneratöre bağlanılıyor...");
  if (genset.begin(9600, 3000)) {
    Serial.println("✅ D-300 MK3 bağlandı!");
    genset.enableAutoUpdate(true);
  } else {
    Serial.println("⚠️ D-300 MK3 bağlantı hatası! Web server yine de başlatılacak.");
  }
  
  // WiFi bağlantısı
  Serial.println("📶 WiFi'ye bağlanılıyor...");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi bağlandı!");
    Serial.println("🌐 IP Adresi: " + WiFi.localIP().toString());
    Serial.println("🔗 Web Arayüzü: http://" + WiFi.localIP().toString());
  } else {
    Serial.println("\n❌ WiFi bağlantısı başarısız!");
    Serial.println("📱 Access Point modunda başlatılıyor...");
    WiFi.softAP("D300-Monitor", "12345678");
    Serial.println("🔗 AP IP: " + WiFi.softAPIP().toString());
  }
  
  // Web server rotaları
  setupWebRoutes();
  
  // Web server başlat
  server.begin();
  Serial.println("🚀 Web server başlatıldı!");
  Serial.println("========================================\n");
}

void loop() {
  // Web server isteklerini işle
  server.handleClient();
  
  // D-300 verilerini güncelle
  genset.handle();
  
  // Veri güncelleme kontrolü
  if (millis() - lastDataUpdate >= DATA_UPDATE_INTERVAL) {
    if (genset.isConnected()) {
      genset.updateBasicData();
    }
    lastDataUpdate = millis();
  }
  
  delay(10);
}

void setupWebRoutes() {
  // Ana sayfa
  server.on("/", HTTP_GET, handleRoot);
  
  // API endpoints
  server.on("/api/data", HTTP_GET, handleApiData);
  server.on("/api/basic", HTTP_GET, handleApiBasic);
  server.on("/api/status", HTTP_GET, handleApiStatus);
  
  // Kontrol endpoints
  server.on("/api/start", HTTP_POST, handleStart);
  server.on("/api/stop", HTTP_POST, handleStop);
  server.on("/api/auto", HTTP_POST, handleAuto);
  server.on("/api/manual", HTTP_POST, handleManual);
  server.on("/api/test", HTTP_POST, handleTest);
  server.on("/api/emergency", HTTP_POST, handleEmergency);
  
  // 404 handler
  server.onNotFound(handleNotFound);
}

void handleRoot() {
  String html = getMainHTML();
  server.send(200, "text/html", html);
}

void handleApiData() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", genset.getDataAsJSON());
}

void handleApiBasic() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", genset.getBasicDataAsJSON());
}

void handleApiStatus() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  String status = "{";
  status += "\"connected\":" + String(genset.isConnected() ? "true" : "false") + ",";
  status += "\"healthy\":" + String(genset.isSystemHealthy() ? "true" : "false") + ",";
  status += "\"generator_running\":" + String(genset.isJeneratorCalisir() ? "true" : "false") + ",";
  status += "\"mains_available\":" + String(genset.isSebekeMevcut() ? "true" : "false") + ",";
  status += "\"status_text\":\"" + genset.getDurumAciklama() + "\",";
  status += "\"mode_text\":\"" + genset.getModAciklama() + "\",";
  status += "\"uptime\":" + String(millis()) + ",";
  status += "\"wifi_rssi\":" + String(WiFi.RSSI());
  status += "}";
  
  server.send(200, "application/json", status);
}

// Kontrol fonksiyonları
void handleStart() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  bool result = genset.startGenerator();
  server.send(200, "application/json", "{\"success\":" + String(result ? "true" : "false") + ",\"message\":\"" + (result ? "Başlatma komutu gönderildi" : "Komut gönderilemedi") + "\"}");
}

void handleStop() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  bool result = genset.stopGenerator();
  server.send(200, "application/json", "{\"success\":" + String(result ? "true" : "false") + ",\"message\":\"" + (result ? "Durdurma komutu gönderildi" : "Komut gönderilemedi") + "\"}");
}

void handleAuto() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  bool result = genset.setAutoMode();
  server.send(200, "application/json", "{\"success\":" + String(result ? "true" : "false") + ",\"message\":\"" + (result ? "AUTO mod etkinleştirildi" : "Komut gönderilemedi") + "\"}");
}

void handleManual() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  bool result = genset.setManualMode();
  server.send(200, "application/json", "{\"success\":" + String(result ? "true" : "false") + ",\"message\":\"" + (result ? "MANUEL mod etkinleştirildi" : "Komut gönderilemedi") + "\"}");
}

void handleTest() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  bool result = genset.setTestMode();
  server.send(200, "application/json", "{\"success\":" + String(result ? "true" : "false") + ",\"message\":\"" + (result ? "TEST mod etkinleştirildi" : "Komut gönderilemedi") + "\"}");
}

void handleEmergency() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  bool result = genset.emergencyStop();
  server.send(200, "application/json", "{\"success\":" + String(result ? "true" : "false") + ",\"message\":\"" + (result ? "Acil durdurma komutu gönderildi" : "Komut gönderilemedi") + "\"}");
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Sayfa bulunamadı");
}

String getMainHTML() {
  return R"(
<!DOCTYPE html>
<html lang="tr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>D-300 MK3 Jeneratör Monitoring</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            color: #333;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
        }
        
        .header {
            background: rgba(255, 255, 255, 0.95);
            border-radius: 15px;
            padding: 20px;
            margin-bottom: 20px;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
            text-align: center;
        }
        
        .header h1 {
            color: #2c3e50;
            font-size: 2.5em;
            margin-bottom: 10px;
        }
        
        .status-badge {
            display: inline-block;
            padding: 8px 16px;
            border-radius: 25px;
            font-weight: bold;
            margin: 5px;
            transition: all 0.3s ease;
        }
        
        .status-online {
            background: #27ae60;
            color: white;
        }
        
        .status-offline {
            background: #e74c3c;
            color: white;
        }
        
        .status-warning {
            background: #f39c12;
            color: white;
        }
        
        .cards-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 20px;
        }
        
        .card {
            background: rgba(255, 255, 255, 0.95);
            border-radius: 15px;
            padding: 25px;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
            transition: transform 0.3s ease;
        }
        
        .card:hover {
            transform: translateY(-5px);
        }
        
        .card h3 {
            color: #2c3e50;
            margin-bottom: 15px;
            font-size: 1.4em;
            border-bottom: 2px solid #3498db;
            padding-bottom: 8px;
        }
        
        .metric {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin: 12px 0;
            padding: 10px;
            background: #f8f9fa;
            border-radius: 8px;
        }
        
        .metric-label {
            font-weight: 600;
            color: #555;
        }
        
        .metric-value {
            font-size: 1.2em;
            font-weight: bold;
            color: #2c3e50;
        }
        
        .voltage { color: #9b59b6; }
        .current { color: #e67e22; }
        .power { color: #27ae60; }
        .frequency { color: #3498db; }
        .rpm { color: #e74c3c; }
        .temperature { color: #f39c12; }
        .fuel { color: #1abc9c; }
        .battery { color: #34495e; }
        
        .controls {
            background: rgba(255, 255, 255, 0.95);
            border-radius: 15px;
            padding: 25px;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
            margin-bottom: 20px;
        }
        
        .controls h3 {
            color: #2c3e50;
            margin-bottom: 20px;
            text-align: center;
        }
        
        .control-buttons {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
            gap: 15px;
        }
        
        .btn {
            padding: 12px 20px;
            border: none;
            border-radius: 8px;
            font-size: 1em;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s ease;
            color: white;
        }
        
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2);
        }
        
        .btn-start { background: #27ae60; }
        .btn-stop { background: #e74c3c; }
        .btn-auto { background: #3498db; }
        .btn-manual { background: #9b59b6; }
        .btn-test { background: #f39c12; }
        .btn-emergency { background: #c0392b; }
        
        .progress-bar {
            width: 100%;
            height: 10px;
            background: #ecf0f1;
            border-radius: 5px;
            overflow: hidden;
            margin-top: 5px;
        }
        
        .progress-fill {
            height: 100%;
            transition: width 0.5s ease;
            border-radius: 5px;
        }
        
        .fuel-progress { background: #1abc9c; }
        .battery-progress { background: #34495e; }
        
        .alarm-panel {
            background: rgba(255, 255, 255, 0.95);
            border-radius: 15px;
            padding: 25px;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
            margin-bottom: 20px;
        }
        
        .alarm-item {
            display: flex;
            align-items: center;
            margin: 10px 0;
            padding: 10px;
            border-radius: 8px;
            font-weight: bold;
        }
        
        .alarm-ok { background: #d5f4e6; color: #27ae60; }
        .alarm-warning { background: #fef9e7; color: #f39c12; }
        .alarm-critical { background: #fadbd8; color: #e74c3c; }
        
        .loading {
            text-align: center;
            padding: 20px;
            color: #7f8c8d;
        }
        
        .footer {
            text-align: center;
            color: rgba(255, 255, 255, 0.8);
            margin-top: 30px;
        }
        
        @media (max-width: 768px) {
            .container {
                padding: 10px;
            }
            
            .header h1 {
                font-size: 2em;
            }
            
            .cards-grid {
                grid-template-columns: 1fr;
            }
        }
        
        .pulse {
            animation: pulse 2s infinite;
        }
        
        @keyframes pulse {
            0% { opacity: 1; }
            50% { opacity: 0.5; }
            100% { opacity: 1; }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🔌 D-300 MK3 Jeneratör Monitoring</h1>
            <div id="connection-status">
                <span class="status-badge status-offline pulse">Bağlantı Kontrol Ediliyor...</span>
            </div>
            <div id="system-time"></div>
        </div>
        
        <div class="controls">
            <h3>🎛️ Jeneratör Kontrolleri</h3>
            <div class="control-buttons">
                <button class="btn btn-start" onclick="sendCommand('start')">🚀 START</button>
                <button class="btn btn-stop" onclick="sendCommand('stop')">🛑 STOP</button>
                <button class="btn btn-auto" onclick="sendCommand('auto')">🤖 AUTO</button>
                <button class="btn btn-manual" onclick="sendCommand('manual')">👨‍🔧 MANUEL</button>
                <button class="btn btn-test" onclick="sendCommand('test')">🧪 TEST</button>
                <button class="btn btn-emergency" onclick="confirmEmergency()">🚨 ACİL STOP</button>
            </div>
        </div>
        
        <div class="alarm-panel">
            <h3>🚨 Alarm Durumu</h3>
            <div id="alarm-status">
                <div class="loading">Alarm durumu kontrol ediliyor...</div>
            </div>
        </div>
        
        <div class="cards-grid">
            <!-- Sistem Durumu Kartı -->
            <div class="card">
                <h3>🔧 Sistem Durumu</h3>
                <div class="metric">
                    <span class="metric-label">Durum:</span>
                    <span class="metric-value" id="system-status">--</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Mod:</span>
                    <span class="metric-value" id="system-mode">--</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Çalışma Süresi:</span>
                    <span class="metric-value" id="uptime">--</span>
                </div>
            </div>
            
            <!-- Şebeke Kartı -->
            <div class="card">
                <h3>🔌 Şebeke Bilgileri</h3>
                <div class="metric">
                    <span class="metric-label">L1 Voltaj:</span>
                    <span class="metric-value voltage" id="mains-l1-voltage">-- V</span>
                </div>
                <div class="metric">
                    <span class="metric-label">L2 Voltaj:</span>
                    <span class="metric-value voltage" id="mains-l2-voltage">-- V</span>
                </div>
                <div class="metric">
                    <span class="metric-label">L3 Voltaj:</span>
                    <span class="metric-value voltage" id="mains-l3-voltage">-- V</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Frekans:</span>
                    <span class="metric-value frequency" id="mains-frequency">-- Hz</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Güç:</span>
                    <span class="metric-value power" id="mains-power">-- kW</span>
                </div>
            </div>
            
            <!-- Jeneratör Kartı -->
            <div class="card">
                <h3>⚡ Jeneratör Bilgileri</h3>
                <div class="metric">
                    <span class="metric-label">L1 Voltaj:</span>
                    <span class="metric-value voltage" id="gen-l1-voltage">-- V</span>
                </div>
                <div class="metric">
                    <span class="metric-label">L2 Voltaj:</span>
                    <span class="metric-value voltage" id="gen-l2-voltage">-- V</span>
                </div>
                <div class="metric">
                    <span class="metric-label">L3 Voltaj:</span>
                    <span class="metric-value voltage" id="gen-l3-voltage">-- V</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Frekans:</span>
                    <span class="metric-value frequency" id="gen-frequency">-- Hz</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Güç:</span>
                    <span class="metric-value power" id="gen-power">-- kW</span>
                </div>
            </div>
            
            <!-- Motor Kartı -->
            <div class="card">
                <h3>🔩 Motor Bilgileri</h3>
                <div class="metric">
                    <span class="metric-label">RPM:</span>
                    <span class="metric-value rpm" id="engine-rpm">-- rpm</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Sıcaklık:</span>
                    <span class="metric-value temperature" id="engine-temp">-- °C</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Yağ Basıncı:</span>
                    <span class="metric-value" id="oil-pressure">-- bar</span>
                </div>
                <div class="metric">
                    <span class="metric-label">Yakıt Seviyesi:</span>
                    <span class="metric-value fuel" id="fuel-level">--%</span>
                    <div class="progress-bar">
                        <div class="progress-fill fuel-progress" id="fuel-progress"></div>
                    </div>
                </div>
                <div class="metric">
                    <span class="metric-label">Batarya:</span>
                    <span class="metric-value battery" id="battery-voltage">-- V</span>
                    <div class="progress-bar">
                        <div class="progress-fill battery-progress" id="battery-progress"></div>
                    </div>
                </div>
            </div>
        </div>
        
        <div class="footer">
            <p>📡 Son Güncelleme: <span id="last-update">--</span></p>
            <p>🌐 WiFi Sinyal: <span id="wifi-signal">--</span> dBm</p>
        </div>
    </div>

    <script>
        let isConnected = false;
        let updateInterval;
        
        // Sayfa yüklendiğinde başlat
        document.addEventListener('DOMContentLoaded', function() {
            startDataUpdate();
            updateSystemTime();
            setInterval(updateSystemTime, 1000);
        });
        
        function startDataUpdate() {
            updateData();
            updateInterval = setInterval(updateData, 3000); // 3 saniyede bir güncelle
        }
        
        function updateSystemTime() {
            const now = new Date();
            document.getElementById('system-time').textContent = 
                '🕒 ' + now.toLocaleString('tr-TR');
        }
        
        async function updateData() {
            try {
                // Durum bilgisi al
                const statusResponse = await fetch('/api/status');
                const statusData = await statusResponse.json();
                
                // Detaylı veri al
                const dataResponse = await fetch('/api/data');
                const data = await dataResponse.json();
                
                updateConnectionStatus(statusData.connected);
                updateSystemInfo(statusData, data);
                updateElectricalData(data);
                updateEngineData(data);
                updateAlarmStatus(data);
                updateFooter(statusData);
                
                document.getElementById('last-update').textContent = new Date().toLocaleTimeString('tr-TR');
                
            } catch (error) {
                console.error('Veri güncellenirken hata:', error);
                updateConnectionStatus(false);
            }
        }
        
        function updateConnectionStatus(connected) {
            const statusElement = document.getElementById('connection-status');
            
            if (connected) {
                statusElement.innerHTML = '<span class="status-badge status-online">🟢 Bağlı</span>';
                isConnected = true;
            } else {
                statusElement.innerHTML = '<span class="status-badge status-offline pulse">🔴 Bağlantı Yok</span>';
                isConnected = false;
            }
        }
        
        function updateSystemInfo(statusData, data) {
            document.getElementById('system-status').textContent = statusData.status_text || '--';
            document.getElementById('system-mode').textContent = statusData.mode_text || '--';
            
            if (statusData.uptime) {
                const uptimeSeconds = Math.floor(statusData.uptime / 1000);
                const hours = Math.floor(uptimeSeconds / 3600);
                const minutes = Math.floor((uptimeSeconds % 3600) / 60);
                document.getElementById('uptime').textContent = `${hours}s ${minutes}d`;
            }
        }
        
        function updateElectricalData(data) {
            if (data.electrical) {
                // Şebeke
                const mains = data.electrical.mains;
                document.getElementById('mains-l1-voltage').textContent = (mains.voltage?.l1 || 0).toFixed(1) + ' V';
                document.getElementById('mains-l2-voltage').textContent = (mains.voltage?.l2 || 0).toFixed(1) + ' V';
                document.getElementById('mains-l3-voltage').textContent = (mains.voltage?.l3 || 0).toFixed(1) + ' V';
                document.getElementById('mains-frequency').textContent = (mains.frequency || 0).toFixed(1) + ' Hz';
                document.getElementById('mains-power').textContent = (mains.power || 0).toFixed(1) + ' kW';
                
                // Jeneratör
                const gen = data.electrical.generator;
                document.getElementById('gen-l1-voltage').textContent = (gen.voltage?.l1 || 0).toFixed(1) + ' V';
                document.getElementById('gen-l2-voltage').textContent = (gen.voltage?.l2 || 0).toFixed(1) + ' V';
                document.getElementById('gen-l3-voltage').textContent = (gen.voltage?.l3 || 0).toFixed(1) + ' V';
                document.getElementById('gen-frequency').textContent = (gen.frequency || 0).toFixed(1) + ' Hz';
                document.getElementById('gen-power').textContent = (gen.power || 0).toFixed(1) + ' kW';
            }
        }
        
        function updateEngineData(data) {
            if (data.engine) {
                const engine = data.engine;
                document.getElementById('engine-rpm').textContent = (engine.rpm || 0).toFixed(0) + ' rpm';
                document.getElementById('engine-temp').textContent = (engine.temperature || 0).toFixed(1) + ' °C';
                document.getElementById('oil-pressure').textContent = (engine.oil_pressure || 0).toFixed(1) + ' bar';
                
                // Yakıt seviyesi
                const fuelLevel = engine.fuel_level || 0;
                document.getElementById('fuel-level').textContent = fuelLevel.toFixed(1) + '%';
                document.getElementById('fuel-progress').style.width = Math.max(0, Math.min(100, fuelLevel)) + '%';
                
                // Batarya voltajı
                const batteryVoltage = engine.battery_voltage || 0;
                document.getElementById('battery-voltage').textContent = batteryVoltage.toFixed(1) + ' V';
                const batteryPercent = Math.max(0, Math.min(100, ((batteryVoltage - 10) / 4) * 100));
                document.getElementById('battery-progress').style.width = batteryPercent + '%';
            }
        }
        
        function updateAlarmStatus(data) {
            const alarmContainer = document.getElementById('alarm-status');
            
            if (data.system && data.system.alarms) {
                const alarms = data.system.alarms;
                let alarmHtml = '';
                
                if (alarms.shutdown) {
                    alarmHtml += '<div class="alarm-item alarm-critical">🚨 KAPATMA ALARMI AKTİF</div>';
                }
                
                if (alarms.loaddump) {
                    alarmHtml += '<div class="alarm-item alarm-warning">⚠️ YÜK ATMA ALARMI AKTİF</div>';
                }
                
                if (alarms.warning) {
                    alarmHtml += '<div class="alarm-item alarm-warning">🟡 UYARI ALARMI AKTİF</div>';
                }
                
                if (!alarms.shutdown && !alarms.loaddump && !alarms.warning) {
                    alarmHtml = '<div class="alarm-item alarm-ok">✅ TÜM SİSTEMLER NORMAL</div>';
                }
                
                alarmContainer.innerHTML = alarmHtml;
            } else {
                alarmContainer.innerHTML = '<div class="alarm-item alarm-warning">❓ Alarm durumu alınamadı</div>';
            }
        }
        
        function updateFooter(statusData) {
            if (statusData.wifi_rssi) {
                document.getElementById('wifi-signal').textContent = statusData.wifi_rssi;
            }
        }
        
        async function sendCommand(command) {
            if (!isConnected) {
                alert('❌ Jeneratöre bağlantı yok! Komut gönderilemez.');
                return;
            }
            
            try {
                const response = await fetch(`/api/${command}`, {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    }
                });
                
                const result = await response.json();
                
                if (result.success) {
                    showNotification('✅ ' + result.message, 'success');
                    // Komut gönderildikten sonra hemen güncelle
                    setTimeout(updateData, 500);
                } else {
                    showNotification('❌ ' + result.message, 'error');
                }
                
            } catch (error) {
                console.error('Komut gönderme hatası:', error);
                showNotification('❌ Komut gönderilemedi: Bağlantı hatası', 'error');
            }
        }
        
        function confirmEmergency() {
            if (confirm('⚠️ ACİL DURDURMA\n\nBu işlem jeneratörü derhal durduracaktır!\nEmin misiniz?')) {
                sendCommand('emergency');
            }
        }
        
        function showNotification(message, type) {
            // Basit notification sistemi
            const notification = document.createElement('div');
            notification.style.cssText = `
                position: fixed;
                top: 20px;
                right: 20px;
                padding: 15px 20px;
                border-radius: 8px;
                color: white;
                font-weight: bold;
                z-index: 1000;
                max-width: 300px;
                box-shadow: 0 4px 15px rgba(0,0,0,0.3);
                transform: translateX(400px);
                transition: transform 0.3s ease;
            `;
            
            if (type === 'success') {
                notification.style.background = '#27ae60';
            } else {
                notification.style.background = '#e74c3c';
            }
            
            notification.textContent = message;
            document.body.appendChild(notification);
            
            // Animasyon ile göster
            setTimeout(() => {
                notification.style.transform = 'translateX(0)';
            }, 100);
            
            // 3 saniye sonra kaldır
            setTimeout(() => {
                notification.style.transform = 'translateX(400px)';
                setTimeout(() => {
                    document.body.removeChild(notification);
                }, 300);
            }, 3000);
        }
        
        // Sayfa odağını kaybettiğinde güncellemeyi durdur, geri aldığında başlat
        document.addEventListener('visibilitychange', function() {
            if (document.hidden) {
                clearInterval(updateInterval);
            } else {
                startDataUpdate();
            }
        });
        
        // Pencere kapanırken interval'ı temizle
        window.addEventListener('beforeunload', function() {
            clearInterval(updateInterval);
        });
    </script>
</body>
</html>
)";
}