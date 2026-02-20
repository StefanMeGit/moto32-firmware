#include "ble_interface.h"
#include "settings_store.h"
#include <NimBLEDevice.h>
#include <Preferences.h>

// ============================================================================
// GATT SERVER (diagnostics / settings – original)
// ============================================================================

static NimBLEServer*         pServer       = nullptr;
static NimBLECharacteristic* pCharState    = nullptr;
static NimBLECharacteristic* pCharVoltage  = nullptr;
static NimBLECharacteristic* pCharSettings = nullptr;
static NimBLECharacteristic* pCharErrors   = nullptr;
static NimBLECharacteristic* pCharCommand  = nullptr;

static bool     newSettingsAvailable = false;
static Settings pendingSettings;
static unsigned long lastBleUpdate = 0;

// ============================================================================
// KEYLESS STATE
// ============================================================================

#define MAX_PAIRED_DEVICES 3
#define KEYLESS_SCAN_INTERVAL_MS  2000
#define KEYLESS_DETECT_HOLD_MS    3000   // Must see phone for 3s before unlock
#define KEYLESS_LOST_TIMEOUT_MS   5000   // Phone must be gone 5s before lock

struct PairedDevice {
  bool    valid = false;
  uint8_t mac[6] = {};
  char    name[20] = {};
  int     lastRssi = -127;
  bool    detected = false;
};

static struct {
  bool    enabled         = false;
  int     rssiThreshold   = -65;
  int     graceSeconds    = 10;

  PairedDevice devices[MAX_PAIRED_DEVICES];
  int  pairedCount        = 0;

  // State machine
  bool phoneDetected      = false;
  bool ignitionGranted    = false;
  bool engineWasRunning   = false;
  bool graceActive        = false;
  unsigned long graceStart = 0;
  unsigned long firstDetectTime = 0;
  unsigned long lastDetectTime  = 0;

  // Scanning
  bool         scanActive  = false;
  NimBLEScan*  pScan       = nullptr;
  unsigned long lastScanTime = 0;
} keyless;

static Preferences keylessPref;

// ============================================================================
// KEYLESS PERSISTENCE
// ============================================================================

static void saveKeylessConfig() {
  keylessPref.begin("ble_kl", false);
  keylessPref.putBool("enabled", keyless.enabled);
  keylessPref.putInt("rssi", keyless.rssiThreshold);
  keylessPref.putInt("grace", keyless.graceSeconds);
  keylessPref.putInt("count", keyless.pairedCount);
  for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
    char k[12];
    snprintf(k, sizeof(k), "mac%d", i);
    if (keyless.devices[i].valid) {
      keylessPref.putBytes(k, keyless.devices[i].mac, 6);
      snprintf(k, sizeof(k), "name%d", i);
      keylessPref.putString(k, keyless.devices[i].name);
    } else {
      keylessPref.remove(k);
    }
  }
  keylessPref.end();
}

static void loadKeylessConfig() {
  keylessPref.begin("ble_kl", true);
  keyless.enabled       = keylessPref.getBool("enabled", false);
  keyless.rssiThreshold = keylessPref.getInt("rssi", -65);
  keyless.graceSeconds  = keylessPref.getInt("grace", 10);
  keyless.pairedCount   = keylessPref.getInt("count", 0);
  for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
    char k[12];
    snprintf(k, sizeof(k), "mac%d", i);
    if (keylessPref.isKey(k)) {
      keylessPref.getBytes(k, keyless.devices[i].mac, 6);
      snprintf(k, sizeof(k), "name%d", i);
      String n = keylessPref.getString(k, "");
      strncpy(keyless.devices[i].name, n.c_str(), sizeof(keyless.devices[i].name)-1);
      keyless.devices[i].valid = true;
    }
  }
  keylessPref.end();
  LOG_I("Keyless: loaded %d paired devices, enabled=%d", keyless.pairedCount, keyless.enabled);
}

// ============================================================================
// MAC HELPERS
// ============================================================================

static String macToString(const uint8_t* mac) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

static bool macFromString(const char* str, uint8_t* mac) {
  return sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) == 6;
}

static bool macEquals(const uint8_t* a, const uint8_t* b) {
  return memcmp(a, b, 6) == 0;
}

// ============================================================================
// GATT CALLBACKS
// ============================================================================

class ServerCB : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*) override {
    bike.bleConnected = true;
    LOG_I("BLE GATT client connected");
  }
  void onDisconnect(NimBLEServer*) override {
    bike.bleConnected = false;
    LOG_I("BLE GATT client disconnected");
    NimBLEDevice::startAdvertising();
  }
};

class SettingsWriteCB : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar) override {
    std::string val = pChar->getValue();
    if (val.size() >= 14) {
      const uint8_t* d = (const uint8_t*)val.data();
      pendingSettings.handlebarConfig = static_cast<HandlebarConfig>(constrain(d[0], CONFIG_A, CONFIG_E));
      pendingSettings.rearLightMode   = d[1];
      pendingSettings.turnSignalMode  = static_cast<TurnSignalMode>(constrain(d[2], TURN_OFF, TURN_30S));
      pendingSettings.brakeLightMode  = static_cast<BrakeLightMode>(constrain(d[3], BRAKE_CONTINUOUS, BRAKE_EMERGENCY));
      pendingSettings.alarmMode       = d[4];
      pendingSettings.positionLight   = constrain(d[5], 0, 9);
      pendingSettings.moWaveEnabled   = d[6] != 0;
      pendingSettings.lowBeamMode     = d[7];
      pendingSettings.aux1Mode        = d[8];
      pendingSettings.aux2Mode        = d[9];
      pendingSettings.standKillMode   = d[10];
      pendingSettings.parkingLightMode = d[11];
      pendingSettings.turnDistancePulsesTarget =
          constrain(d[12] | (d[13] << 8), TURN_DISTANCE_MIN_PULSES, TURN_DISTANCE_MAX_PULSES);
      newSettingsAvailable = true;
      LOG_I("BLE: settings received");
    }
  }
};

class CommandCB : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar) override {
    std::string val = pChar->getValue();
    if (val.empty()) return;
    if (val[0] == 0x01) esp_restart();
    if (val[0] == 0x02) { bike.errorFlags = ERR_NONE; LOG_I("BLE: errors cleared"); }
  }
};

// ============================================================================
// SCAN CALLBACK (for keyless device discovery & pairing)
// ============================================================================

// Temporary scan result storage for the web UI
struct ScanResult {
  uint8_t mac[6];
  char    name[24];
  int     rssi;
};
static ScanResult scanResults[16];
static int scanResultCount = 0;
static bool scanResultsReady = false;

// Helper: extract 6-byte MAC in normal order from NimBLEAddress
// (getNative() returns bytes in reversed/NimBLE-internal order in v1.4)
static void addressToMac(const NimBLEAddress& addr, uint8_t* mac) {
  const uint8_t* native = addr.getNative();
  for (int i = 0; i < 6; i++) mac[i] = native[5 - i];
}

class ScanCB : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* dev) override {
    // Extract MAC in normal byte order
    uint8_t devMac[6];
    addressToMac(dev->getAddress(), devMac);

    // Check if this is a paired device → update RSSI
    for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
      if (!keyless.devices[i].valid) continue;
      if (macEquals(devMac, keyless.devices[i].mac)) {
        keyless.devices[i].lastRssi = dev->getRSSI();
        keyless.devices[i].detected =
            (dev->getRSSI() >= keyless.rssiThreshold);
      }
    }

    // Store scan results for web UI pairing
    if (keyless.scanActive && scanResultCount < 16) {
      // Only add devices with names
      std::string name = dev->getName();
      if (name.empty()) return;

      // Deduplicate
      for (int i = 0; i < scanResultCount; i++) {
        if (macEquals(scanResults[i].mac, devMac)) {
          scanResults[i].rssi = dev->getRSSI();
          return;
        }
      }

      memcpy(scanResults[scanResultCount].mac, devMac, 6);
      strncpy(scanResults[scanResultCount].name, name.c_str(), 23);
      scanResults[scanResultCount].rssi = dev->getRSSI();
      scanResultCount++;
      scanResultsReady = true;
    }
  }
};

static ScanCB scanCallback;

// Scan-complete callback (free function for v1.4 API)
static void onScanComplete(NimBLEScanResults results) {
  scanResultsReady = true;
}

// ============================================================================
// PUBLIC: INIT
// ============================================================================

void bleInit() {
  NimBLEDevice::init(BLE_DEVICE_NAME);
  NimBLEDevice::setPower(ESP_PWR_LVL_P3);

  // GATT server
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCB());

  NimBLEService* svc = pServer->createService(BLE_SERVICE_UUID);
  pCharState    = svc->createCharacteristic(BLE_CHAR_STATE_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  pCharVoltage  = svc->createCharacteristic(BLE_CHAR_VOLTAGE_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  pCharSettings = svc->createCharacteristic(BLE_CHAR_SETTINGS_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
  pCharSettings->setCallbacks(new SettingsWriteCB());
  pCharErrors   = svc->createCharacteristic(BLE_CHAR_ERRORS_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  pCharCommand  = svc->createCharacteristic(BLE_CHAR_COMMAND_UUID,
      NIMBLE_PROPERTY::WRITE);
  pCharCommand->setCallbacks(new CommandCB());
  svc->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(BLE_SERVICE_UUID);
  adv->setScanResponse(true);
  adv->start();

  // Scanner for keyless
  keyless.pScan = NimBLEDevice::getScan();
  keyless.pScan->setAdvertisedDeviceCallbacks(&scanCallback, true);
  keyless.pScan->setActiveScan(true);
  keyless.pScan->setInterval(100);
  keyless.pScan->setWindow(80);

  // Load keyless config
  loadKeylessConfig();

  LOG_I("BLE initialized: %s", BLE_DEVICE_NAME);
}

// ============================================================================
// PUBLIC: GATT UPDATE
// ============================================================================

void bleUpdate() {
  unsigned long now = millis();
  if (now - lastBleUpdate < 200) return;
  lastBleUpdate = now;

  if (!bike.bleConnected) return;

  // Pack state
  uint8_t buf[8] = {};
  buf[0] = (bike.ignitionOn ? 0x01 : 0)
          | (bike.engineRunning ? 0x02 : 0)
          | (bike.starterEngaged ? 0x04 : 0)
          | (bike.killActive ? 0x08 : 0)
          | (bike.standDown ? 0x10 : 0)
          | (bike.lowVoltageWarning ? 0x20 : 0);
  buf[1] = (bike.lowBeamOn ? 0x01 : 0)
          | (bike.highBeamOn ? 0x02 : 0)
          | (bike.leftTurnOn ? 0x04 : 0)
          | (bike.rightTurnOn ? 0x08 : 0)
          | (bike.hazardLightsOn ? 0x10 : 0)
          | (bike.brakePressed ? 0x20 : 0)
          | (bike.hornPressed ? 0x40 : 0);
  uint16_t vBat = (uint16_t)(bike.batteryVoltage * 100.0f);
  buf[2] = vBat & 0xFF;
  buf[3] = (vBat >> 8) & 0xFF;
  buf[4] = bike.errorFlags;

  pCharState->setValue(buf, sizeof(buf));
  pCharState->notify();

  char vStr[8];
  snprintf(vStr, sizeof(vStr), "%.1f", bike.batteryVoltage);
  pCharVoltage->setValue(vStr);

  if (bike.errorFlags != ERR_NONE) {
    pCharErrors->setValue(&bike.errorFlags, 1);
    pCharErrors->notify();
  }
}

bool bleHasNewSettings()  { return newSettingsAvailable; }
Settings bleGetNewSettings() { newSettingsAvailable = false; return pendingSettings; }

// ============================================================================
// KEYLESS: STATE MACHINE
// ============================================================================
//
// State flow:
//   LOCKED → (phone detected for 3s) → UNLOCKED (ignition granted)
//   UNLOCKED → (engine starts) → engineWasRunning = true
//   UNLOCKED → (engine stops + engineWasRunning) → GRACE (10s countdown)
//   GRACE → (restart within 10s) → stays UNLOCKED
//   GRACE → (timeout) → LOCKED (need phone again)
//   UNLOCKED → (phone lost for 5s without engine running) → LOCKED
//
// ============================================================================

void bleKeylessUpdate() {
  if (!keyless.enabled || keyless.pairedCount == 0) {
    keyless.ignitionGranted = false;
    keyless.phoneDetected = false;
    return;
  }

  unsigned long now = millis();

  // Periodic background scan for paired devices
  if (now - keyless.lastScanTime >= KEYLESS_SCAN_INTERVAL_MS) {
    keyless.lastScanTime = now;
    // Short scan burst (non-blocking)
    if (!keyless.pScan->isScanning()) {
      keyless.pScan->start(1, onScanComplete, false);  // 1 second, non-blocking
    }
  }

  // Check if any paired device is in range
  bool anyDetected = false;
  for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
    if (keyless.devices[i].valid && keyless.devices[i].detected) {
      anyDetected = true;
      break;
    }
  }

  // ── Phone detection with hysteresis ──

  if (anyDetected && !keyless.phoneDetected) {
    // First detection → start hold timer
    if (keyless.firstDetectTime == 0) {
      keyless.firstDetectTime = now;
    }
    // Must be detected for KEYLESS_DETECT_HOLD_MS continuously
    if (now - keyless.firstDetectTime >= KEYLESS_DETECT_HOLD_MS) {
      keyless.phoneDetected = true;
      keyless.lastDetectTime = now;
      keyless.ignitionGranted = true;
      keyless.graceActive = false;
      LOG_I("Keyless: phone detected – ignition granted");
    }
  } else if (anyDetected) {
    keyless.lastDetectTime = now;
    keyless.firstDetectTime = now;  // Keep resetting
  } else {
    keyless.firstDetectTime = 0;

    // Phone lost
    if (keyless.phoneDetected) {
      if (now - keyless.lastDetectTime >= KEYLESS_LOST_TIMEOUT_MS) {
        keyless.phoneDetected = false;
        // If engine was never running, lock immediately
        if (!keyless.engineWasRunning) {
          keyless.ignitionGranted = false;
          LOG_I("Keyless: phone lost – locked (engine never ran)");
        }
        // If engine was running, grace period is handled by engineOff
      }
    }
  }

  // Track if engine was running during this unlock cycle
  if (keyless.ignitionGranted && bike.engineRunning) {
    keyless.engineWasRunning = true;
  }

  // Grace period countdown
  if (keyless.graceActive) {
    unsigned long graceMs = (unsigned long)keyless.graceSeconds * 1000UL;
    if (now - keyless.graceStart >= graceMs) {
      keyless.graceActive = false;
      keyless.ignitionGranted = false;
      keyless.engineWasRunning = false;
      LOG_I("Keyless: grace period expired – locked");
    }
    // If engine restarts during grace, cancel grace
    if (bike.engineRunning) {
      keyless.graceActive = false;
      keyless.engineWasRunning = true;
      LOG_I("Keyless: engine restarted during grace");
    }
  }

  // Clear device detection flags for next scan cycle
  for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
    if (keyless.devices[i].valid) {
      // Fade detection if not refreshed by scan
      // (will be set again by scanCallback if still in range)
    }
  }
}

bool bleKeylessIgnitionAllowed() {
  return keyless.ignitionGranted;
}

void bleKeylessEngineOff() {
  if (!keyless.enabled || !keyless.ignitionGranted) return;
  if (!keyless.engineWasRunning) return;

  // Engine was running and now stopped → start grace period
  if (!keyless.graceActive && !keyless.phoneDetected) {
    keyless.graceActive = true;
    keyless.graceStart = millis();
    LOG_I("Keyless: engine off – %ds grace period", keyless.graceSeconds);
  }
}

// ============================================================================
// KEYLESS: CONFIGURATION
// ============================================================================

void bleKeylessConfigure(bool enabled, int rssiThreshold, int graceSeconds) {
  keyless.enabled = enabled;
  keyless.rssiThreshold = constrain(rssiThreshold, -90, -30);
  keyless.graceSeconds = constrain(graceSeconds, 5, 60);
  saveKeylessConfig();
  LOG_I("Keyless config: enabled=%d rssi=%d grace=%ds",
        enabled, rssiThreshold, graceSeconds);
}

// ============================================================================
// PAIRING
// ============================================================================

void bleStartScan() {
  scanResultCount = 0;
  scanResultsReady = false;
  keyless.scanActive = true;
  keyless.pScan->start(10, onScanComplete, false);  // 10s active scan
  LOG_I("BLE scan started");
}

void bleStopScan() {
  keyless.scanActive = false;
  if (keyless.pScan->isScanning()) {
    keyless.pScan->stop();
  }
  LOG_I("BLE scan stopped");
}

void blePairDevice(const char* macStr) {
  uint8_t mac[6];
  if (!macFromString(macStr, mac)) {
    LOG_W("Invalid MAC: %s", macStr);
    return;
  }

  // Find empty slot
  int slot = -1;
  for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
    if (!keyless.devices[i].valid) { slot = i; break; }
  }
  if (slot < 0) {
    LOG_W("Max paired devices reached");
    return;
  }

  memcpy(keyless.devices[slot].mac, mac, 6);
  // Try to find name from scan results
  for (int i = 0; i < scanResultCount; i++) {
    if (macEquals(scanResults[i].mac, mac)) {
      strncpy(keyless.devices[slot].name, scanResults[i].name,
              sizeof(keyless.devices[slot].name) - 1);
      break;
    }
  }
  keyless.devices[slot].valid = true;
  keyless.pairedCount++;
  saveKeylessConfig();
  LOG_I("Paired device: %s (%s)", macStr, keyless.devices[slot].name);
}

void bleRemovePaired(const char* macStr) {
  uint8_t mac[6];
  if (!macFromString(macStr, mac)) return;

  for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
    if (keyless.devices[i].valid && macEquals(keyless.devices[i].mac, mac)) {
      keyless.devices[i].valid = false;
      memset(keyless.devices[i].mac, 0, 6);
      keyless.devices[i].name[0] = 0;
      keyless.pairedCount--;
      saveKeylessConfig();
      LOG_I("Removed paired device: %s", macStr);
      return;
    }
  }
}

// ============================================================================
// JSON BUILDER
// ============================================================================

void bleKeylessBuildJson(JsonDocument& doc) {
  doc["type"] = "keyless";
  doc["enabled"] = keyless.enabled;
  doc["rssiThreshold"] = keyless.rssiThreshold;
  doc["graceSeconds"] = keyless.graceSeconds;
  doc["phoneDetected"] = keyless.phoneDetected;
  doc["graceActive"] = keyless.graceActive;

  if (keyless.graceActive) {
    unsigned long elapsed = millis() - keyless.graceStart;
    unsigned long totalMs = (unsigned long)keyless.graceSeconds * 1000UL;
    int remaining = (int)((totalMs - min(elapsed, totalMs)) / 1000UL);
    doc["graceRemaining"] = remaining;
  }

  // Current RSSI of best paired device
  int bestRssi = -127;
  for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
    if (keyless.devices[i].valid && keyless.devices[i].lastRssi > bestRssi) {
      bestRssi = keyless.devices[i].lastRssi;
    }
  }
  if (bestRssi > -127) doc["currentRssi"] = bestRssi;

  // Paired devices
  JsonArray arr = doc["paired"].to<JsonArray>();
  for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
    if (!keyless.devices[i].valid) continue;
    JsonObject d = arr.add<JsonObject>();
    d["mac"] = macToString(keyless.devices[i].mac);
    d["name"] = keyless.devices[i].name;
    d["rssi"] = keyless.devices[i].lastRssi > -127 ? keyless.devices[i].lastRssi : 0;
    d["connected"] = keyless.devices[i].detected;
  }

  // Scan results – embed directly in keyless document
  doc["scanning"] = keyless.scanActive;
  if (scanResultsReady && scanResultCount > 0) {
    JsonArray devs = doc["scanResults"].to<JsonArray>();
    for (int i = 0; i < scanResultCount; i++) {
      JsonObject d = devs.add<JsonObject>();
      d["mac"] = macToString(scanResults[i].mac);
      d["name"] = scanResults[i].name;
      d["rssi"] = scanResults[i].rssi;
    }
  }
}
