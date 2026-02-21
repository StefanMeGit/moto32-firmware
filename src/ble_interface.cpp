#include "ble_interface.h"
#include "inputs.h"
#include "settings_store.h"
#include <NimBLEDevice.h>
#include <NimBLESecurity.h>
#include <Preferences.h>
#include <nvs_flash.h>
#include <ctype.h>

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
#define KEYLESS_IDLE_SCAN_INTERVAL_MS    2000
#define KEYLESS_REFRESH_SCAN_INTERVAL_MS 30000
#define KEYLESS_IDLE_SCAN_SECONDS        1
#define KEYLESS_REFRESH_SCAN_SECONDS     2
#define KEYLESS_FAILED_SCAN_EXTENSION_SECONDS 10
#define KEYLESS_DETECT_HOLD_MS    3000   // Must see phone for 3s before unlock
#define KEYLESS_SEEN_TIMEOUT_MS   4500   // Keep detection valid for recent scan data
#define KEYLESS_PASSKEY            691
#define KEYLESS_RSSI_LEVEL_MIN      1
#define KEYLESS_RSSI_LEVEL_MAX      6
#define KEYLESS_RSSI_LEVEL_DEFAULT  4
#define KEYLESS_RSSI_DBM_MIN      -100
#define KEYLESS_RSSI_DBM_MAX       -30
#define KEYLESS_ACTIVE_DEFAULT_MINUTES 5
static const int KEYLESS_ACTIVE_MINUTES_OPTIONS[] = {1, 5, 10};
static const size_t KEYLESS_ACTIVE_MINUTES_OPTION_COUNT =
    sizeof(KEYLESS_ACTIVE_MINUTES_OPTIONS) / sizeof(KEYLESS_ACTIVE_MINUTES_OPTIONS[0]);
#define KEYLESS_ACTIVATION_MODE_ANY      0
#define KEYLESS_ACTIVATION_MODE_SELECTED 1
#define KEYLESS_ACTIVATION_MODE_DEFAULT  KEYLESS_ACTIVATION_MODE_ANY
#define KEYLESS_ACTIVATION_BUTTON_DEFAULT 0

enum KeylessActivationButton : uint8_t {
  KEYLESS_BUTTON_START = 0,
  KEYLESS_BUTTON_LIGHT = 1,
  KEYLESS_BUTTON_HORN  = 2,
  KEYLESS_BUTTON_LEFT  = 3,
  KEYLESS_BUTTON_RIGHT = 4,
  KEYLESS_BUTTON_COUNT
};

struct PairedDevice {
  bool    valid = false;
  uint8_t mac[6] = {};
  char    name[20] = {};
  int     lastRssi = -127;
  bool    detected = false;
  unsigned long lastSeenMs = 0;
};

static struct {
  bool    enabled         = false;
  int     rssiLevel       = KEYLESS_RSSI_LEVEL_DEFAULT;
  int     rssiThreshold   = -76;   // Derived from level (default level 4)
  int     activeMinutes   = KEYLESS_ACTIVE_DEFAULT_MINUTES;
  uint8_t activationMode  = KEYLESS_ACTIVATION_MODE_DEFAULT;
  uint8_t activationButton = KEYLESS_ACTIVATION_BUTTON_DEFAULT;

  PairedDevice devices[MAX_PAIRED_DEVICES];
  int  pairedCount        = 0;

  // Session state
  bool phoneDetected      = false;
  bool statusDetected     = false;
  bool ignitionGranted    = false;
  bool ignitionOwned      = false;
  bool requestIgnitionOn  = false;
  bool requestIgnitionOff = false;
  unsigned long firstDetectTime = 0;
  unsigned long sessionStartMs  = 0;
  unsigned long sessionExpiryMs = 0;

  // Scanning
  bool         scanActive           = false;   // Manual web scan for pairing
  bool         autoScanActive       = false;   // Automatic background scan
  bool         sessionRefreshScan   = false;   // Auto scan while session active
  bool         autoScanExtended     = false;   // One-time 10s extension after failed auto scan
  NimBLEScan*  pScan                = nullptr;
  unsigned long lastIdleScanTime    = 0;
  unsigned long lastRefreshScanTime = 0;
} keyless;

static Preferences keylessPref;
static const char* KEYLESS_NAMESPACE = "ble_kl";

static int clampRssiLevel(int level) {
  return constrain(level, KEYLESS_RSSI_LEVEL_MIN, KEYLESS_RSSI_LEVEL_MAX);
}

static int rssiThresholdForLevel(int level) {
  switch (clampRssiLevel(level)) {
    case 1:  return -100;  // Very weak signal still accepted
    case 2:  return -92;
    case 3:  return -84;
    case 4:  return -76;   // Default
    case 5:  return -68;
    case 6:  return -60;   // Strong signal required
    default: return -76;
  }
}

static int rssiLevelForThreshold(int thresholdDbm) {
  const int dbm = constrain(thresholdDbm, KEYLESS_RSSI_DBM_MIN, KEYLESS_RSSI_DBM_MAX);
  int bestLevel = KEYLESS_RSSI_LEVEL_DEFAULT;
  int bestDiff = abs(dbm - rssiThresholdForLevel(bestLevel));
  for (int level = KEYLESS_RSSI_LEVEL_MIN; level <= KEYLESS_RSSI_LEVEL_MAX; level++) {
    const int diff = abs(dbm - rssiThresholdForLevel(level));
    if (diff < bestDiff) {
      bestDiff = diff;
      bestLevel = level;
    }
  }
  return bestLevel;
}

static void applyRssiLevel(int level) {
  keyless.rssiLevel = clampRssiLevel(level);
  keyless.rssiThreshold = rssiThresholdForLevel(keyless.rssiLevel);
}

static int normalizeActiveMinutes(int minutes) {
  int best = KEYLESS_ACTIVE_MINUTES_OPTIONS[0];
  int bestDiff = abs(minutes - best);
  for (size_t i = 1; i < KEYLESS_ACTIVE_MINUTES_OPTION_COUNT; i++) {
    const int candidate = KEYLESS_ACTIVE_MINUTES_OPTIONS[i];
    const int diff = abs(minutes - candidate);
    if (diff < bestDiff) {
      best = candidate;
      bestDiff = diff;
    }
  }
  return best;
}

static uint8_t normalizeActivationMode(int mode) {
  return (mode == KEYLESS_ACTIVATION_MODE_SELECTED)
      ? KEYLESS_ACTIVATION_MODE_SELECTED
      : KEYLESS_ACTIVATION_MODE_ANY;
}

static uint8_t normalizeActivationButton(int button) {
  if (button < 0 || button >= (int)KEYLESS_BUTTON_COUNT) {
    return KEYLESS_ACTIVATION_BUTTON_DEFAULT;
  }
  return (uint8_t)button;
}

static int activeMinutesFromLegacySeconds(int seconds) {
  if (seconds <= 0) {
    return KEYLESS_ACTIVE_DEFAULT_MINUTES;
  }
  const int approxMinutes = max(1, (seconds + 30) / 60);
  return normalizeActiveMinutes(approxMinutes);
}

static bool timeReached(unsigned long now, unsigned long deadline) {
  return (long)(now - deadline) >= 0;
}

static int sessionRemainingSeconds(unsigned long now) {
  if (!keyless.ignitionGranted || keyless.sessionExpiryMs == 0) return 0;
  if (timeReached(now, keyless.sessionExpiryMs)) return 0;
  const unsigned long remainingMs = keyless.sessionExpiryMs - now;
  return (int)((remainingMs + 999UL) / 1000UL);
}

static int nextAutoScanInSeconds(unsigned long now) {
  if (!keyless.enabled || keyless.pairedCount == 0) return -1;
  if (keyless.autoScanActive) return 0;

  const unsigned long intervalMs = keyless.ignitionGranted
      ? KEYLESS_REFRESH_SCAN_INTERVAL_MS
      : KEYLESS_IDLE_SCAN_INTERVAL_MS;
  const unsigned long lastScanMs = keyless.ignitionGranted
      ? keyless.lastRefreshScanTime
      : keyless.lastIdleScanTime;

  if (now - lastScanMs >= intervalMs) return 0;
  const unsigned long remainMs = intervalMs - (now - lastScanMs);
  return (int)((remainMs + 999UL) / 1000UL);
}

static bool isSelectedActivationButtonPressed() {
  switch (keyless.activationButton) {
    case KEYLESS_BUTTON_START: return startEvent.pressed;
    case KEYLESS_BUTTON_LIGHT: return lightEvent.pressed;
    case KEYLESS_BUTTON_HORN:  return hornEvent.pressed;
    case KEYLESS_BUTTON_LEFT:  return turnLeftEvent.pressed;
    case KEYLESS_BUTTON_RIGHT: return turnRightEvent.pressed;
    default: return false;
  }
}

static bool isAnyActivationButtonPressed() {
  return startEvent.pressed
      || lightEvent.pressed
      || hornEvent.pressed
      || turnLeftEvent.pressed
      || turnRightEvent.pressed;
}

static bool isActivationButtonPressed() {
  if (keyless.activationMode == KEYLESS_ACTIVATION_MODE_SELECTED) {
    return isSelectedActivationButtonPressed();
  }
  return isAnyActivationButtonPressed();
}

// ============================================================================
// KEYLESS PERSISTENCE
// ============================================================================

static bool beginKeylessPrefsWithRecovery(bool readOnly) {
  if (keylessPref.begin(KEYLESS_NAMESPACE, readOnly)) return true;

  LOG_E("Keyless NVS open failed (ro=%d)", readOnly ? 1 : 0);

  esp_err_t nvsErr = nvs_flash_init();
  if (nvsErr == ESP_ERR_NVS_NO_FREE_PAGES
      || nvsErr == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    LOG_W("Keyless NVS needs erase (%s)", esp_err_to_name(nvsErr));
    esp_err_t eraseErr = nvs_flash_erase();
    if (eraseErr != ESP_OK) {
      LOG_E("Keyless NVS erase failed: %s", esp_err_to_name(eraseErr));
      return false;
    }
    nvsErr = nvs_flash_init();
  }

  if (nvsErr != ESP_OK) {
    LOG_E("Keyless NVS init failed: %s", esp_err_to_name(nvsErr));
    return false;
  }

  if (!keylessPref.begin(KEYLESS_NAMESPACE, readOnly)) {
    LOG_E("Keyless NVS reopen failed after recovery");
    return false;
  }

  LOG_W("Keyless NVS recovered");
  return true;
}

static void saveKeylessConfig() {
  if (!beginKeylessPrefsWithRecovery(false)) {
    LOG_E("Keyless config save skipped: NVS unavailable");
    return;
  }
  keylessPref.putBool("enabled", keyless.enabled);
  keylessPref.putUChar("rssiLvl", (uint8_t)keyless.rssiLevel);
  keylessPref.putInt("rssi", keyless.rssiThreshold);
  keylessPref.putInt("activeMin", keyless.activeMinutes);
  keylessPref.putUChar("actMode", keyless.activationMode);
  keylessPref.putUChar("actBtn", keyless.activationButton);
  keylessPref.remove("grace");  // remove legacy key if still present
  keylessPref.putInt("count", keyless.pairedCount);
  for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
    char macKey[12];
    char nameKey[12];
    snprintf(macKey, sizeof(macKey), "mac%d", i);
    snprintf(nameKey, sizeof(nameKey), "name%d", i);
    if (keyless.devices[i].valid) {
      keylessPref.putBytes(macKey, keyless.devices[i].mac, 6);
      keylessPref.putString(nameKey, keyless.devices[i].name);
    } else {
      keylessPref.remove(macKey);
      keylessPref.remove(nameKey);
    }
  }
  keylessPref.end();
}

static void loadKeylessConfig() {
  for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
    keyless.devices[i] = PairedDevice{};
  }
  keyless.pairedCount = 0;

  if (!beginKeylessPrefsWithRecovery(false)) {
    keyless.enabled = false;
    applyRssiLevel(KEYLESS_RSSI_LEVEL_DEFAULT);
    keyless.activeMinutes = KEYLESS_ACTIVE_DEFAULT_MINUTES;
    keyless.activationMode = KEYLESS_ACTIVATION_MODE_DEFAULT;
    keyless.activationButton = KEYLESS_ACTIVATION_BUTTON_DEFAULT;
    LOG_W("Keyless config load fallback: defaults");
    return;
  }

  keyless.enabled       = keylessPref.getBool("enabled", false);
  if (keylessPref.isKey("rssiLvl")) {
    applyRssiLevel(keylessPref.getUChar("rssiLvl", KEYLESS_RSSI_LEVEL_DEFAULT));
  } else {
    // Legacy compatibility: convert old dBm threshold to nearest level.
    int legacyDbm = constrain(keylessPref.getInt("rssi", -76),
                              KEYLESS_RSSI_DBM_MIN, KEYLESS_RSSI_DBM_MAX);
    applyRssiLevel(rssiLevelForThreshold(legacyDbm));
  }
  if (keylessPref.isKey("activeMin")) {
    keyless.activeMinutes = normalizeActiveMinutes(
        keylessPref.getInt("activeMin", KEYLESS_ACTIVE_DEFAULT_MINUTES));
  } else {
    // Legacy compatibility: convert old grace seconds to session minutes.
    keyless.activeMinutes = activeMinutesFromLegacySeconds(
        keylessPref.getInt("grace", KEYLESS_ACTIVE_DEFAULT_MINUTES * 60));
  }
  keyless.activationMode = normalizeActivationMode(
      keylessPref.getUChar("actMode", KEYLESS_ACTIVATION_MODE_DEFAULT));
  keyless.activationButton = normalizeActivationButton(
      keylessPref.getUChar("actBtn", KEYLESS_ACTIVATION_BUTTON_DEFAULT));
  for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
    char macKey[12];
    char nameKey[12];
    snprintf(macKey, sizeof(macKey), "mac%d", i);
    snprintf(nameKey, sizeof(nameKey), "name%d", i);

    if (keylessPref.getBytesLength(macKey) == 6) {
      if (keylessPref.getBytes(macKey, keyless.devices[i].mac, 6) == 6) {
        String n = keylessPref.getString(nameKey, "");
        strncpy(keyless.devices[i].name, n.c_str(),
                sizeof(keyless.devices[i].name) - 1);
        keyless.devices[i].name[sizeof(keyless.devices[i].name) - 1] = '\0';
        keyless.devices[i].valid = true;
        keyless.devices[i].lastRssi = -127;
        keyless.devices[i].detected = false;
        keyless.devices[i].lastSeenMs = 0;
        keyless.pairedCount++;
      }
    }
  }
  keylessPref.end();
  LOG_I("Keyless: loaded %d paired devices, enabled=%d, level=%d (%d dBm), active=%d min, trigger=%s/%u",
        keyless.pairedCount, keyless.enabled,
        keyless.rssiLevel, keyless.rssiThreshold, keyless.activeMinutes,
        keyless.activationMode == KEYLESS_ACTIVATION_MODE_SELECTED ? "selected" : "any",
        keyless.activationButton);
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

static bool isGenericDeviceLabel(const char* name) {
  if (!name || !name[0]) return true;
  return strcmp(name, "Unknown Device") == 0
      || strcmp(name, "Apple Device") == 0;
}

static bool equalsIgnoreCase(const char* a, const std::string& b) {
  if (!a || !a[0] || b.empty()) return false;
  size_t i = 0;
  for (; a[i] != '\0' && i < b.size(); i++) {
    if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) {
      return false;
    }
  }
  return a[i] == '\0' && i == b.size();
}

static uint16_t manufacturerCompanyId(const std::string& mfg) {
  if (mfg.size() < 2) return 0;
  return ((uint16_t)(uint8_t)mfg[1] << 8) | (uint8_t)mfg[0];
}

// ============================================================================
// GATT CALLBACKS
// ============================================================================

class SecurityCB : public NimBLESecurityCallbacks {
  uint32_t onPassKeyRequest() override {
    LOG_I("BLE security: passkey requested (use %04u)", KEYLESS_PASSKEY);
    return KEYLESS_PASSKEY;
  }

  void onPassKeyNotify(uint32_t pass_key) override {
    LOG_I("BLE security: passkey notify %06lu", (unsigned long)pass_key);
  }

  bool onSecurityRequest() override {
    LOG_I("BLE security: incoming security request accepted");
    return true;
  }

  void onAuthenticationComplete(ble_gap_conn_desc* desc) override {
    if (!desc) {
      LOG_W("BLE security: authentication complete with null descriptor");
      return;
    }
    LOG_I("BLE security: auth complete encrypted=%d bonded=%d mitm=%d",
          desc->sec_state.encrypted ? 1 : 0,
          desc->sec_state.bonded ? 1 : 0,
          desc->sec_state.authenticated ? 1 : 0);
  }

  bool onConfirmPIN(uint32_t pin) override {
    LOG_I("BLE security: confirm pin %06lu", (unsigned long)pin);
    return true;
  }
};

class ServerCB : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*) override {
    bike.bleConnected = true;
    bike.bleConnectBlinkActive = true;
    bike.bleConnectBlinkStart = millis();
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
static void onScanComplete(NimBLEScanResults results);

static void clearDeviceDetectionFlags() {
  for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
    keyless.devices[i].detected = false;
  }
}

static bool anyPairedDeviceInRange(unsigned long now) {
  bool anyDetected = false;
  for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
    if (!keyless.devices[i].valid) {
      keyless.devices[i].detected = false;
      continue;
    }

    const bool seenRecently = keyless.devices[i].lastSeenMs != 0
        && (now - keyless.devices[i].lastSeenMs) <= KEYLESS_SEEN_TIMEOUT_MS;
    const bool inRange = seenRecently
        && (keyless.devices[i].lastRssi >= keyless.rssiThreshold);
    keyless.devices[i].detected = inRange;
    if (inRange) anyDetected = true;
  }
  return anyDetected;
}

static void startIgnitionSession(unsigned long now) {
  const unsigned long durationMs =
      (unsigned long)keyless.activeMinutes * 60UL * 1000UL;
  keyless.ignitionGranted = true;
  keyless.phoneDetected = true;
  keyless.statusDetected = true;
  keyless.sessionStartMs = now;
  keyless.sessionExpiryMs = now + durationMs;
  keyless.lastRefreshScanTime = now;
  keyless.ignitionOwned = !bike.ignitionOn;
  keyless.requestIgnitionOn = false;
  keyless.requestIgnitionOff = false;
}

static void extendIgnitionSession(unsigned long now) {
  const unsigned long durationMs =
      (unsigned long)keyless.activeMinutes * 60UL * 1000UL;
  keyless.sessionStartMs = now;
  keyless.sessionExpiryMs = now + durationMs;
}

static bool tryStartAutoScan(uint32_t seconds, bool sessionRefresh, bool extended) {
  if (!keyless.pScan) return false;
  if (keyless.pScan->isScanning()) return false;
  keyless.autoScanActive = true;
  keyless.sessionRefreshScan = sessionRefresh;
  keyless.autoScanExtended = extended;
  if (!keyless.pScan->start(seconds, onScanComplete, false)) {
    keyless.autoScanActive = false;
    keyless.sessionRefreshScan = false;
    keyless.autoScanExtended = false;
    return false;
  }
  return true;
}

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
    const int rssi = dev->getRSSI();
    const uint8_t addrType = dev->getAddressType();
    const std::string advName = dev->getName();
    const std::string mfg = dev->getManufacturerData();
    const uint16_t companyId = manufacturerCompanyId(mfg);
    const bool isApple = (companyId == 0x004C);

    LOG_D("BLE adv: %s type=%u rssi=%d name='%s' mfg=0x%04X",
          macToString(devMac).c_str(),
          addrType,
          rssi,
          advName.empty() ? "-" : advName.c_str(),
          companyId);

    // Check if this is a paired device -> update RSSI.
    // iPhone fallback: when address randomizes, allow Apple name match.
    for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
      if (!keyless.devices[i].valid) continue;

      bool matched = macEquals(devMac, keyless.devices[i].mac);
      const bool canUseNameFallback =
          isApple
          && !advName.empty()
          && !isGenericDeviceLabel(keyless.devices[i].name)
          && equalsIgnoreCase(keyless.devices[i].name, advName);

      if (!matched && canUseNameFallback) {
        matched = true;
        LOG_D("Keyless fallback match by Apple name: paired='%s' seen='%s' (%s)",
              keyless.devices[i].name,
              advName.c_str(),
              macToString(devMac).c_str());
      }

      if (matched) {
        keyless.devices[i].lastRssi = rssi;
        keyless.devices[i].lastSeenMs = millis();
      }
    }

    // Store scan results for web UI pairing
    if (keyless.scanActive && scanResultCount < 16) {
      std::string label = advName;
      if (label.empty()) {
        // iPhones often advertise without local-name in BLE scan packets.
        label = isApple ? "Apple Device" : "Unknown Device";
      }

      // Deduplicate
      for (int i = 0; i < scanResultCount; i++) {
        if (macEquals(scanResults[i].mac, devMac)) {
          scanResults[i].rssi = rssi;
          return;
        }
      }

      memcpy(scanResults[scanResultCount].mac, devMac, 6);
      memset(scanResults[scanResultCount].name, 0,
             sizeof(scanResults[scanResultCount].name));
      strncpy(scanResults[scanResultCount].name, label.c_str(),
              sizeof(scanResults[scanResultCount].name) - 1);
      scanResults[scanResultCount].rssi = rssi;
      LOG_D("BLE scan result: %s (%s), RSSI=%d",
            scanResults[scanResultCount].name,
            macToString(devMac).c_str(),
            scanResults[scanResultCount].rssi);
      scanResultCount++;
      scanResultsReady = true;
    }
  }
};

static ScanCB scanCallback;
static SecurityCB securityCallback;

// Scan-complete callback (free function for v1.4 API)
static void onScanComplete(NimBLEScanResults results) {
  (void)results;

  if (keyless.scanActive) {
    keyless.scanActive = false;
    scanResultsReady = true;
    LOG_I("BLE scan complete: results=%d scannerRunning=%d",
          scanResultCount,
          keyless.pScan && keyless.pScan->isScanning() ? 1 : 0);
  } else {
    const bool wasAutoScan = keyless.autoScanActive;
    const bool wasSessionRefresh = keyless.sessionRefreshScan;
    const bool wasExtended = keyless.autoScanExtended;
    keyless.autoScanActive = false;
    keyless.sessionRefreshScan = false;
    keyless.autoScanExtended = false;

    if (!wasAutoScan) {
      return;
    }

    const unsigned long now = millis();
    const bool detected = anyPairedDeviceInRange(now);
    keyless.statusDetected = detected;

    if (!detected && !wasExtended) {
      LOG_I("Keyless: auto scan no match, extending by %us",
            KEYLESS_FAILED_SCAN_EXTENSION_SECONDS);
      if (tryStartAutoScan(KEYLESS_FAILED_SCAN_EXTENSION_SECONDS,
                           wasSessionRefresh,
                           true)) {
        return;
      }
      LOG_W("Keyless: extension scan start failed");
    }

    if (wasSessionRefresh && keyless.ignitionGranted) {
      keyless.phoneDetected = detected;
      if (detected) {
        extendIgnitionSession(now);
        LOG_I("Keyless: session extended (%d min, RSSI >= %d dBm)",
              keyless.activeMinutes, keyless.rssiThreshold);
      } else {
        LOG_I("Keyless: refresh scan no match, timeout in %ds",
              sessionRemainingSeconds(now));
      }
    } else {
      LOG_D("BLE background scan complete (detected=%d, extended=%d)",
            detected ? 1 : 0,
            wasExtended ? 1 : 0);
    }
  }
}

// ============================================================================
// PUBLIC: INIT
// ============================================================================

void bleInit() {
  NimBLEDevice::init(BLE_DEVICE_NAME);
  NimBLEDevice::setPower(ESP_PWR_LVL_P3);
  NimBLEDevice::setSecurityAuth(true, true, true);
  NimBLEDevice::setSecurityIOCap(ESP_IO_CAP_OUT);
  NimBLEDevice::setSecurityInitKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  NimBLEDevice::setSecurityRespKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  NimBLEDevice::setSecurityPasskey(KEYLESS_PASSKEY);
  NimBLEDevice::setSecurityCallbacks(&securityCallback);
  LOG_I("BLE security enabled (bonding + passkey %04u)", KEYLESS_PASSKEY);

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
  keyless.pScan->setInterval(120);
  keyless.pScan->setWindow(110);

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
//   LOCKED -> (phone detected for 3s) -> SESSION ACTIVE (ignition granted)
//   SESSION ACTIVE -> every 30s run refresh scan (2s burst)
//   SESSION ACTIVE + refresh detected -> extend session to full duration
//   SESSION ACTIVE + refresh missed -> countdown continues
//   SESSION ACTIVE + timeout -> lock and request ignition OFF (if keyless-owned)
//
// ============================================================================

void bleKeylessUpdate() {
  keyless.requestIgnitionOn = false;
  keyless.requestIgnitionOff = false;

  if (!keyless.enabled || keyless.pairedCount == 0) {
    if (keyless.pScan && keyless.pScan->isScanning() && !keyless.scanActive) {
      keyless.pScan->stop();
    }
    keyless.autoScanActive = false;
    keyless.sessionRefreshScan = false;
    keyless.autoScanExtended = false;
    keyless.ignitionGranted = false;
    keyless.ignitionOwned = false;
    keyless.phoneDetected = false;
    keyless.statusDetected = false;
    keyless.firstDetectTime = 0;
    keyless.sessionStartMs = 0;
    keyless.sessionExpiryMs = 0;
    clearDeviceDetectionFlags();
    return;
  }

  const unsigned long now = millis();
  const bool anyDetected = anyPairedDeviceInRange(now);

  if (!keyless.ignitionGranted) {
    // Idle background scan while locked.
    if (now - keyless.lastIdleScanTime >= KEYLESS_IDLE_SCAN_INTERVAL_MS) {
      keyless.lastIdleScanTime = now;
      tryStartAutoScan(KEYLESS_IDLE_SCAN_SECONDS, false, false);
    }

    if (anyDetected) {
      if (keyless.firstDetectTime == 0) {
        keyless.firstDetectTime = now;
        LOG_D("Keyless: detection hold started");
      } else if (now - keyless.firstDetectTime >= KEYLESS_DETECT_HOLD_MS) {
        startIgnitionSession(now);
        keyless.firstDetectTime = 0;
        LOG_I("Keyless: session started (%d min, threshold L%d/%d dBm, owned=%d)",
              keyless.activeMinutes,
              keyless.rssiLevel,
              keyless.rssiThreshold,
              keyless.ignitionOwned ? 1 : 0);
      }
    } else {
      keyless.phoneDetected = false;
      keyless.firstDetectTime = 0;
    }
    return;
  }

  // Active session.
  keyless.phoneDetected = anyDetected;

  // BLE ignition activation requires a button trigger.
  if (!bike.ignitionOn && isActivationButtonPressed()) {
    keyless.requestIgnitionOn = true;
    LOG_I("Keyless: ignition ON requested by button trigger (mode=%u btn=%u)",
          keyless.activationMode, keyless.activationButton);
  }

  // Keep keyless session alive while engine is running. Engine state is
  // externally maintained and only cleared by kill switch / ignition lock logic.
  if (bike.engineRunning && keyless.sessionExpiryMs != 0) {
    extendIgnitionSession(now);
    return;
  }

  if (keyless.sessionExpiryMs != 0 && timeReached(now, keyless.sessionExpiryMs)) {
    const bool lockActive = inputActive(PIN_LOCK);
    const bool shouldTurnOff = keyless.ignitionOwned && bike.ignitionOn && !lockActive;
    keyless.ignitionGranted = false;
    keyless.ignitionOwned = false;
    keyless.phoneDetected = false;
    keyless.firstDetectTime = 0;
    keyless.sessionStartMs = 0;
    keyless.sessionExpiryMs = 0;
    keyless.autoScanActive = false;
    keyless.sessionRefreshScan = false;
    keyless.autoScanExtended = false;
    if (shouldTurnOff) {
      keyless.requestIgnitionOff = true;
      LOG_I("Keyless: session expired -> ignition OFF requested");
    } else {
      LOG_I("Keyless: session expired (manual lock active=%d)", lockActive ? 1 : 0);
    }
    return;
  }

  if (now - keyless.lastRefreshScanTime >= KEYLESS_REFRESH_SCAN_INTERVAL_MS) {
    keyless.lastRefreshScanTime = now;
    if (tryStartAutoScan(KEYLESS_REFRESH_SCAN_SECONDS, true, false)) {
      LOG_D("Keyless: refresh scan started");
    }
  }
}

bool bleKeylessIgnitionAllowed() {
  return keyless.ignitionGranted;
}

bool bleKeylessTakeIgnitionOnRequest() {
  const bool requested = keyless.requestIgnitionOn;
  keyless.requestIgnitionOn = false;
  return requested;
}

bool bleKeylessTakeIgnitionOffRequest() {
  const bool requested = keyless.requestIgnitionOff;
  keyless.requestIgnitionOff = false;
  return requested;
}

// ============================================================================
// KEYLESS: CONFIGURATION
// ============================================================================

void bleKeylessConfigure(bool enabled,
                         int rssiLevelOrLegacyDbm,
                         int activeMinutesOrLegacySeconds,
                         int activationMode,
                         int activationButton) {
  keyless.enabled = enabled;
  if (rssiLevelOrLegacyDbm >= KEYLESS_RSSI_LEVEL_MIN
      && rssiLevelOrLegacyDbm <= KEYLESS_RSSI_LEVEL_MAX) {
    applyRssiLevel(rssiLevelOrLegacyDbm);
  } else {
    // Backward compatibility for older clients that still send dBm.
    applyRssiLevel(rssiLevelForThreshold(rssiLevelOrLegacyDbm));
  }
  if (activeMinutesOrLegacySeconds > KEYLESS_ACTIVE_MINUTES_OPTIONS[KEYLESS_ACTIVE_MINUTES_OPTION_COUNT - 1]) {
    keyless.activeMinutes = activeMinutesFromLegacySeconds(activeMinutesOrLegacySeconds);
  } else {
    keyless.activeMinutes = normalizeActiveMinutes(activeMinutesOrLegacySeconds);
  }
  keyless.activationMode = normalizeActivationMode(activationMode);
  keyless.activationButton = normalizeActivationButton(activationButton);
  saveKeylessConfig();
  LOG_I("Keyless config: enabled=%d level=%d threshold=%d dBm active=%d min trigger=%s/%u",
        enabled, keyless.rssiLevel, keyless.rssiThreshold, keyless.activeMinutes,
        keyless.activationMode == KEYLESS_ACTIVATION_MODE_SELECTED ? "selected" : "any",
        keyless.activationButton);
}

// ============================================================================
// PAIRING
// ============================================================================

void bleStartScan() {
  if (!keyless.pScan) {
    LOG_E("BLE scan start failed: scanner not initialized");
    return;
  }
  if (keyless.pScan->isScanning()) {
    keyless.pScan->stop();
  }
  keyless.autoScanActive = false;
  keyless.sessionRefreshScan = false;
  keyless.autoScanExtended = false;
  scanResultCount = 0;
  scanResultsReady = false;
  keyless.scanActive = true;
  keyless.pScan->start(12, onScanComplete, false);  // 12s active scan
  LOG_I("BLE scan started (12s, lvl=%d, threshold=%d dBm, paired=%d, active=%d)",
        keyless.rssiLevel,
        keyless.rssiThreshold,
        keyless.pairedCount,
        keyless.pScan->isScanning() ? 1 : 0);
}

void bleStopScan() {
  keyless.scanActive = false;
  keyless.autoScanActive = false;
  keyless.sessionRefreshScan = false;
  keyless.autoScanExtended = false;
  if (keyless.pScan && keyless.pScan->isScanning()) {
    keyless.pScan->stop();
  }
  LOG_I("BLE scan stopped (results=%d)", scanResultCount);
}

void blePairDevice(const char* macStr) {
  uint8_t mac[6];
  if (!macFromString(macStr, mac)) {
    LOG_W("Invalid MAC: %s", macStr);
    return;
  }

  // Already paired: keep existing slot and refresh name if available.
  for (int i = 0; i < MAX_PAIRED_DEVICES; i++) {
    if (keyless.devices[i].valid && macEquals(keyless.devices[i].mac, mac)) {
      for (int j = 0; j < scanResultCount; j++) {
        if (macEquals(scanResults[j].mac, mac)) {
          strncpy(keyless.devices[i].name, scanResults[j].name,
                  sizeof(keyless.devices[i].name) - 1);
          keyless.devices[i].name[sizeof(keyless.devices[i].name) - 1] = '\0';
          break;
        }
      }
      saveKeylessConfig();
      LOG_I("Device already paired: %s", macStr);
      return;
    }
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
  keyless.devices[slot].name[0] = '\0';
  // Try to find name from scan results
  for (int i = 0; i < scanResultCount; i++) {
    if (macEquals(scanResults[i].mac, mac)) {
      strncpy(keyless.devices[slot].name, scanResults[i].name,
              sizeof(keyless.devices[slot].name) - 1);
      keyless.devices[slot].name[sizeof(keyless.devices[slot].name) - 1] = '\0';
      break;
    }
  }
  keyless.devices[slot].valid = true;
  keyless.devices[slot].detected = false;
  keyless.devices[slot].lastSeenMs = 0;
  keyless.devices[slot].lastRssi = -127;
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
      keyless.devices[i].detected = false;
      keyless.devices[i].lastSeenMs = 0;
      keyless.devices[i].lastRssi = -127;
      keyless.pairedCount = max(0, keyless.pairedCount - 1);
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
  const unsigned long now = millis();
  doc["type"] = "keyless";
  doc["enabled"] = keyless.enabled;
  doc["rssiLevel"] = keyless.rssiLevel;
  doc["rssiThresholdDbm"] = keyless.rssiThreshold;
  doc["rssiThreshold"] = keyless.rssiThreshold;
  doc["activeMinutes"] = keyless.activeMinutes;
  doc["activationMode"] = keyless.activationMode;
  doc["activationButton"] = keyless.activationButton;
  doc["sessionActive"] = keyless.ignitionGranted;
  doc["sessionRemaining"] = sessionRemainingSeconds(now);
  doc["nextScanIn"] = nextAutoScanInSeconds(now);
  doc["autoSearching"] = keyless.autoScanActive;
  doc["sessionRefreshSearching"] = keyless.autoScanActive && keyless.sessionRefreshScan;
  doc["phoneDetected"] = keyless.phoneDetected;
  doc["statusDetected"] = keyless.statusDetected;
  doc["waitingForButton"] = keyless.ignitionGranted && !bike.ignitionOn;
  // Backward compatibility fields for older web clients.
  doc["graceSeconds"] = keyless.activeMinutes * 60;
  doc["graceActive"] = false;
  doc["graceRemaining"] = 0;

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
  doc["scanReady"] = scanResultsReady;
  doc["scanResultCount"] = scanResultCount;
  doc["scannerRunning"] =
      (keyless.pScan != nullptr) ? keyless.pScan->isScanning() : false;
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
