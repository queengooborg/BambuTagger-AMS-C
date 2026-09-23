#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

#define FIRMWARE_VERSION "1.3.0-queengooborg"
#define OTA_REPO \
  "queengooborg/BambuTagger-AMS-C"  // "VID-PRO/BambuTagger-AMS-C" -- XXX restore once forked
                                    // changes are merged upstream

#define NUM_SLOTS 4

// --- Pin Assignments ---
// Pin assignments differ because the S3 DevKitC and classic ESP32 expose
// different GPIOs on their headers.
#ifdef BAMBU_TAGGER_ESP32_S3
// RC522 SPI pins
#define SPI_MOSI 11
#define SPI_MISO 13
#define SPI_SCK 12
const uint8_t SS_PINS[NUM_SLOTS] = {4, 5, 6, 7};
const uint8_t RST_PINS[NUM_SLOTS] = {15, 16, 17, 18};

// WS2812 LED daisy chain (single data pin, 4 LEDs in series)
#define LED_DATA_PIN 14

// SPI TFT (240x240 1.3" ST7789VW)
#define TFT_BLK 45
#define TFT_DC 37
#define TFT_RES 0
#define TFT_CS -1
#define TFT_SCL 36
#define TFT_SDA 35

// BME280 I2C pins
#define I2C_SDA 48
#define I2C_SCL 47
#else
// RC522 SPI pins
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18
const uint8_t SS_PINS[NUM_SLOTS] = {13, 12, 14, 27};
const uint8_t RST_PINS[NUM_SLOTS] = {26, 25, 33, 32};

// WS2812 LED daisy chain (single data pin, 4 LEDs in series)
#define LED_DATA_PIN 15

// SPI TFT (240x240 1.3" ST7789VW)
#define TFT_BLK 2
#define TFT_DC 4
#define TFT_RES 5
#define TFT_CS -1
#define TFT_SCL 16
#define TFT_SDA 17

// BME280 I2C pins
#define I2C_SDA 21
#define I2C_SCL 22
#endif

#define LED_COUNT 4
#define LED_BRIGHTNESS 32

// --- RFID Settings ---
#define RFID_POLL_INTERVAL_MS 250
#define RFID_DEBOUNCE_MS 750
#define NTAG_PAGE_SIZE 4
#define NTAG_USER_START_PAGE 4
#define NTAG_MAX_PAGES 231

// --- Spool Info Structure ---
struct SpoolInfo {
  bool present;
  char uid[16];
  char materialType[32];
  char detailedType[256];
  char color[16];
  char colorHex[9];
  uint16_t remainingGrams;
  uint16_t totalGrams;
  uint16_t nozzleTempMin;
  uint16_t nozzleTempMax;
  char batchNumber[32];
  char manufacturer[32];
  uint32_t lastSeen;
  bool tagReadSuccess;
};

// --- System Configuration ---
struct SystemConfig {
  char wifiSSID[32];
  char wifiPassword[64];
  char printerIP[16];
  uint16_t printerPort;
  char printerAccessCode[48];
  char printerSerial[32];
  char deviceName[32];
  uint8_t amsUnit;  // 0=A, 1=B, 2=C, 3=D
  bool mqttEnabled;
  bool mqttUseTLS;
  uint32_t mqttUpdateIntervalMs;
  char mqttTopicPrefix[64];
  bool layoutVertical;
};

// --- Default Configuration ---
inline SystemConfig getDefaultConfig() {
  SystemConfig cfg;
  strcpy(cfg.wifiSSID, "");
  strcpy(cfg.wifiPassword, "");
  strcpy(cfg.printerIP, "192.168.1.100");
  cfg.printerPort = 8883;
  strcpy(cfg.printerAccessCode, "");
  strcpy(cfg.printerSerial, "");
  strcpy(cfg.deviceName, "BambuTagger-AMS");
  cfg.amsUnit = 0;
  cfg.mqttEnabled = false;
  cfg.mqttUseTLS = true;
  cfg.mqttUpdateIntervalMs = 3000;
  strcpy(cfg.mqttTopicPrefix, "device");
  cfg.layoutVertical = false;
  return cfg;
}

// --- Persistent Config Helpers ---
inline void loadConfig(SystemConfig& cfg) {
  Preferences prefs;
  prefs.begin("bambu-ams", true);
  cfg = getDefaultConfig();
  if (prefs.isKey("wifiSSID"))
    prefs.getString("wifiSSID", cfg.wifiSSID, sizeof(cfg.wifiSSID));
  if (prefs.isKey("wifiPwd"))
    prefs.getString("wifiPwd", cfg.wifiPassword, sizeof(cfg.wifiPassword));
  if (prefs.isKey("printerIP"))
    prefs.getString("printerIP", cfg.printerIP, sizeof(cfg.printerIP));
  cfg.printerPort = prefs.getUShort("printerPort", cfg.printerPort);
  if (prefs.isKey("accCode"))
    prefs.getString("accCode", cfg.printerAccessCode, sizeof(cfg.printerAccessCode));
  if (prefs.isKey("printerSN"))
    prefs.getString("printerSN", cfg.printerSerial, sizeof(cfg.printerSerial));
  if (prefs.isKey("devName"))
    prefs.getString("devName", cfg.deviceName, sizeof(cfg.deviceName));
  cfg.mqttEnabled = prefs.getBool("mqttEn", cfg.mqttEnabled);
  cfg.mqttUseTLS = prefs.getBool("mqttTLS", cfg.mqttUseTLS);
  cfg.mqttUpdateIntervalMs = prefs.getULong("mqttInt", cfg.mqttUpdateIntervalMs);
  cfg.amsUnit = prefs.getUChar("amsUnit", cfg.amsUnit);
  cfg.layoutVertical = prefs.getBool("layoutVertical", false);
  prefs.end();
}

inline void saveConfig(const SystemConfig& cfg) {
  Preferences prefs;
  prefs.begin("bambu-ams", false);
  prefs.putString("wifiSSID", cfg.wifiSSID);
  prefs.putString("wifiPwd", cfg.wifiPassword);
  prefs.putString("printerIP", cfg.printerIP);
  prefs.putUShort("printerPort", cfg.printerPort);
  prefs.putString("accCode", cfg.printerAccessCode);
  prefs.putString("printerSN", cfg.printerSerial);
  prefs.putString("devName", cfg.deviceName);
  prefs.putBool("mqttEn", cfg.mqttEnabled);
  prefs.putBool("mqttTLS", cfg.mqttUseTLS);
  prefs.putULong("mqttInt", cfg.mqttUpdateIntervalMs);
  prefs.putUChar("amsUnit", cfg.amsUnit);
  prefs.putBool("layoutVertical", cfg.layoutVertical);
  prefs.end();
}

#endif
