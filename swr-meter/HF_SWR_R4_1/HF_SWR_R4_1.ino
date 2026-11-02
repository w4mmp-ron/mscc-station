// ========================================================
// Digital SWR Meter - LVGL code
// 3.5" E32R35T Touch Display
// R4.1: bar graph or cross-needle main page 
// ========================================================

// ---   LF/HF VERSION TOGGLE   ---
#undef VERSION_LF
#define VERSION_HF       // 160m-10m
// --------------------------------

#include "powerTables.h" // Loads appropriate power tables for LF/HF

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_MCP23008.h>   // ← NEW: MCP23008 library
#include <Preferences.h>
#include <driver/ledc.h>
#include <WiFi.h>                // Need this to enable Wi-Fi
#include <WiFiUdp.h>             // Need for UDP Server
#include <WebServer.h>           // Web server for dashboard and OTA updates
#include <time.h>                // Need for clock (only active when wifi is connected)
#include <esp_sntp.h>            // Need for time sync
#include "KS9A.h"                // KS9A Splash Screen
#include <Update.h>              //  Need for Firmware Updates
#include <lvgl.h>                //  LVGL library
#include <montserrat_64_swr.h>   // only characters S W R : . 0-9  included in this font
#include "CrossNeedle.h"         // analog cross-needle face
#include <SD.h>                 //  for SD card access

Preferences preferences;  // Global object

extern const lv_font_t montserrat_64_swr;

float fwdVisualMax = 10;   //default to the 10 watt scale
float refVisualMax = 2;

#ifdef VERSION_HF
  String codeVersion = "HF R4.1 "__DATE__ " " __TIME__;
#else
  String codeVersion = "LF R4.1 "__DATE__ " " __TIME__;
#endif

// LVGL calibration values - we can tweak these if touch is off
#define TS_MIN_X 200
#define TS_MAX_X 3700
#define TS_MIN_Y 200
#define TS_MAX_Y 3800

// SD card
#define SD_CS 5
String sdFirmwareFile = "";

// LVGL Objects 
lv_obj_t * screen_main = NULL;
lv_obj_t * screen_menu_root = NULL;
lv_obj_t * screen_settings = NULL;
lv_obj_t * screen_wifi_scan = NULL;
lv_obj_t * screen_keyboard = NULL;
lv_obj_t * screen_display = NULL;
lv_obj_t * screen_saved_settings = NULL;
lv_obj_t * screen_system_info = NULL;
lv_obj_t * screen_udp_config = NULL;
lv_obj_t * screen_network = NULL;
lv_obj_t * screen_static_ip = NULL;
lv_obj_t * screen_firmware_sd = NULL;
lv_obj_t * screen_update_firmware = NULL;
lv_obj_t * peak_fwd_line = NULL;
lv_obj_t * peak_ref_line = NULL;
lv_obj_t * label_brightness_val = NULL;
lv_obj_t * label_timeout_val = NULL;
lv_obj_t * label_sleep_val = NULL;
lv_obj_t * btn_screensaver = NULL;
lv_obj_t * btn_voltage_mode = NULL;
lv_obj_t * btn_meter_style = NULL;
lv_obj_t * lbl_fwd_val;
lv_obj_t * lbl_ref_val;
lv_obj_t * lbl_swr_val;
lv_obj_t * lbl_swr_title;
lv_obj_t * bar_fwd;
lv_obj_t * bar_ref;
lv_obj_t * btn_peak;
lv_obj_t * btn_range;
//  lv_obj_t * label_range;   CTA removed 02/26/2026
lv_obj_t * label_swr_threshold;
lv_obj_t * wifi_bars[5] = {NULL, NULL, NULL, NULL, NULL};
lv_obj_t * lbl_swr_threshold_val = NULL;
lv_obj_t * lbl_ip = NULL;
lv_obj_t * lbl_net_ssid = NULL;
lv_obj_t * lbl_net_ip = NULL;
lv_obj_t * lbl_net_gw = NULL;
lv_obj_t * lbl_net_sub = NULL;
lv_obj_t * lbl_net_dns = NULL;
lv_obj_t * lbl_net_rssi = NULL;
lv_obj_t * btn_wifi_pwr = NULL;
lv_obj_t * btn_web_server = NULL;
lv_obj_t * btn_ip_mode = NULL;
lv_obj_t * btn_udp = NULL;
lv_obj_t * label_clock = NULL;
lv_obj_t * scale_fwd = NULL;
lv_obj_t * scale_ref = NULL;
lv_obj_t * btn_udp_toggle = NULL;
lv_obj_t * lbl_fwd_aux;
lv_obj_t * lbl_fwd_aux_val;
lv_obj_t * lbl_ref_aux;
lv_obj_t * lbl_ref_aux_val;
lv_obj_t * lbl_peak_btn;
lv_obj_t * ta_ip[4];
lv_obj_t * ta_port;
lv_obj_t * screensaver_shield = NULL;
lv_obj_t * lbl_min_power_val = NULL;
lv_obj_t * lbl_fault_count_val = NULL;
lv_obj_t * lbl_swr_limit = NULL;

static char fwdBuf[16]; 
static char auxBuf[20];

const char* labels_2w[] = {"0", "0.5", "1", "1.5", "2", NULL};
const char* labels_10w[] = {"0", "2", "4", "6", "8", "10", NULL};
const char* labels_50w[] = {"0", "10", "20", "30", "40", "50", NULL};

void drawKeyboardPage(const char* title, String* targetVar, int step = 0);
void drawWiFiScannerPage();
int keyboardStep = 0; // Global to track SSID vs Password steps

//How often to send meter data to COM port
unsigned long lastSerialTime = 0;
const unsigned long serialDelayRX = 1000; // normal delay for com data
const unsigned long serialDelayTX = 30;  // lower delay so updates are faster during TX
unsigned long serialDelay = serialDelayRX; // current delay for com data (uses 2 above)

// Track OTA updates
bool isUpdatingOTA = false;

//static lv_disp_draw_buf_t draw_buf;
//static lv_color_t* buf1;
lv_display_t * disp;
uint8_t * draw_buf_1;

// Network Configuration
bool wifiEnabled = false;       // Track if WiFi is on/off
bool webServerEnabled = false;  // Track if HTTP server is on/off
bool udpEnabled = false;       // Track if UDP unicast is on/off
bool useStaticIP = false;      // Toggle between DHCP and Static
bool wasConnected = false;
String ssid     = "";
String password = "";

// DEBUGGING - add inside code D_P(1), D_P(2) etc for debug points
#define D_P(x) Serial.print("STAMP: "); Serial.println(x); Serial.flush();

// Set your desired Static IP settings here
IPAddress local_IP(192, 168, 1, 100); 
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   // Optional

// Set IP and port for UDP unicast
IPAddress udpAddress(192, 168, 1, 254); // Your PC IP to send data to
int udpPort = 6999; // Your PC udp  port number to send data to
char ipBuf[16] = "DISCONNECTED";
uint8_t editUDPIP[4] = {192, 168, 1, 254}; 
int editUDPPort = 6999;

// wifi and udp variables
WiFiUDP udp;
unsigned long lastUdpTime = 0;
const unsigned long udpDelayRX = 1000; // send data once per second while receiving
const unsigned long udpDelayTX = 30;
unsigned long  udpDelay = udpDelayRX;
unsigned long connectionStartTime = 0;
bool isAttemptingConnection = false;

TFT_eSPI tft = TFT_eSPI();
SPIClass spiTouch = SPIClass(VSPI);
XPT2046_Touchscreen ts(33, 36);

Adafruit_ADS1115 ads;
Adafruit_MCP23008 mcp;                  // ← NEW: MCP23008 object
#define SDA_PIN 32
#define SCL_PIN 25
#define MIN_POWER_THRESHOLD 0.1f
#define ADC_AVERAGE_COUNT   6        // was 15
bool minSWRPowerCheckEnabled = false;
int minSWRPowerThreshold = 10;
int swrFaultCountLimit = 3;

unsigned long lastInteractionTime = 0; 
bool screenIsOff = false;  // Screen on/off
bool screensaverEnabled = true;  // Screensaver on/off
int screensaverMinutes = 3;  // Screensaver time (m)
int menuTimeout = 30; // menu exit timeout (s)

// save static ip info
struct SaveData {
    lv_obj_t * container;
    lv_obj_t * error_label;
};

bool highRange = false;
float forwardPower = 0, reflectedPower = 0, swr = 1.0f;
float peakFWD = 0, peakREF = 0;
float lastForwardPower = -999.0;  
float lastForwardPeakPower = -999.0;    
float lastReflectedPower = -999.0;  
float lastReflectedPeakPower = -999.0;    // ← NEW
float lastSwr = -999.0;                 // ← NEW
bool lastHighRange = !highRange;        // ← NEW
float barSmoothFwd = 0.0f;
float barSmoothRef = 0.0f;
float instantV = 0.0f;
float instantRV = 0.0f;
float instantP = 0.0f;
float instantRP = 0.0f;
float instRho; 
float instSWR;

float zeroOffsetFWD = 0.0f;
float zeroOffsetREV = 0.0f;

// keeping the 5 second calibration window is important for catching noise
const int ZERO_SAMPLES = 1000;           // increased number of samples (oversampling) for better noise reduction
const int ZERO_DELAY_MS = 4;             // sped up rate - ADS is running @860 samples per second in this config

// NEW globals for fault logic
bool faultState = false;
int highSWRCount = 0;

// SW1 button variables (debounced)
bool lastSW1State = HIGH;
bool currentSW1State = HIGH;
bool reading = HIGH;
unsigned long lastSW1Debounce = 0;
const unsigned long sw1DebounceDelay = 50;

const float POWER_CHANGE_THRESHOLD = 0.15;   // Already have — used for FWD & REF
const float SWR_CHANGE_THRESHOLD = 0.03;    // ← NEW — for SWR
unsigned long lastUpdate = 0;

// DEFINE MENU/SCREEN PAGES
enum Page { PAGE_MAIN, PAGE_MENU_ROOT, PAGE_SETTINGS, PAGE_DISPLAY, PAGE_NETWORK, PAGE_IP_CONFIG, PAGE_WIFI_SCAN, PAGE_KEYBOARD, PAGE_SAVED_SETTINGS, PAGE_SYSTEM_INFO, PAGE_UDP_CONFIG, PAGE_FIRMWARE, PAGE_FIRMWARE_SD};
Page currentPage = PAGE_MAIN;

//=======Peak Button===============
bool showPeakMode = false;  // false = Volts mode (default), true = Peak watts mode
bool useCrossNeedle = false; // main page: analog needles vs bar graph
float lastFwdV = -999.0;
float lastRefV = -999.0;
float forwardVoltage = 0.0f;
float reflectedVoltage = 0.0f;
//===============SWR Calculation=============
// we will tune it from real measurements
const float FWD_HIGH_RANGE_SCALE = 2.30f;     // ← start with this value 2.30f

// Peak hold timer 
unsigned long peakHoldStart = 0;          // Timestamp when RF dropped below threshold
const unsigned long PEAK_HOLD_DURATION = 600;  // 5 seconds — easy to adjust here manually  
bool peakHolding = false;                 // Flag if we're in hold period
#define PEAK_DECAY_STEP 0.95             // Multiplier for smooth fall

//=============Voltage Format =============
// Voltage format selection: 0 = Measured, 1 = RMS, 2 = Peak
uint8_t voltageFormatMode = 0;  // start with Measured
const char* voltageFormatLabels[3] = {"Measured", "RMS", "Peak"};

//============Web Server========
WebServer server(80);
static unsigned long lastServerCheck = 0;

// ===========SWR Alarm threshold setting==========
float swrAlarmThreshold = 2.00f;        // Starting value
const float SWR_ALARM_MIN = 1.50f;      // ← Change these if needed later
const float SWR_ALARM_MAX = 2.50f;      // ← Easy to adjust
const float SWR_ALARM_STEP = 0.10f;     // 0.1 step

// ====== touch screen timer to exit menu after no input
const unsigned long screenTimeout = 30000; // 30 seconds in milliseconds

// == FWD & REF readings
float fwdRaw = 0, revRaw = 0;
float fwdPeakRaw = 0;
float fwdPeakPower = 0;
float refPeakRaw = 0;
float refPeakPower = 0;

//============Display Dimmer==================
#define BACKLIGHT_PIN 27
uint8_t backLightLevel = 200;          // Default/start value — comfortable bright
const uint8_t BACKLIGHT_MIN = 10;      // Lowest usable (your tested value)
const uint8_t BACKLIGHT_MAX = 255;     // Full bright
const uint8_t BACKLIGHT_STEP = 25;     // Jump size — gives ~9 steps (25,50,...,225,255)

//========= Dynamic Settings Page

struct SettingItem {
  String label;
  String* stringVal;    
  float* floatVal;      
  int* intVal;
  uint8_t* byteVal;          
  bool* boolVal;        // For WiFi Enable, DHCP mode, etc.
  IPAddress* ipVal;     // For Local IP, Gateway, Mask, DNS
};

// List of variables to track for Saved Settings page
SettingItem mySettings[] = {
  // Label        Str     Flt     Int     Byte    Bool            IP
  {"WiFi Status",  NULL,   NULL,   NULL,   NULL,   &wifiEnabled,   NULL},
  {"Static IP",    NULL,   NULL,   NULL,   NULL,   &useStaticIP,   NULL},
  {"IP",           NULL,   NULL,   NULL,   NULL,   NULL,           &local_IP},
  {"Mask",         NULL,   NULL,   NULL,   NULL,   NULL,           &subnet},
  {"Gateway",      NULL,   NULL,   NULL,   NULL,   NULL,           &gateway},
  {"DNS",          NULL,   NULL,   NULL,   NULL,   NULL,           &primaryDNS},
  {"SSID",        &ssid,  NULL,   NULL,   NULL,   NULL,           NULL},
  {"Password",    &password, NULL, NULL, NULL, NULL,              NULL},
  {"Web Server",  NULL,   NULL,   NULL,   NULL,   &webServerEnabled, NULL},
  {"UDP Enable",  NULL,   NULL,   NULL,   NULL,   &udpEnabled, NULL},
  {"UDP IP",  NULL,   NULL,   NULL,   NULL,   NULL, &udpAddress},  
  {"UDP Port",  NULL,   NULL,   &udpPort,   NULL,   NULL, NULL},
  {"SWR Alarm",   NULL,   &swrAlarmThreshold, NULL, NULL, NULL,   NULL},
  {"Brightness",  NULL,   NULL,   NULL,   &backLightLevel, NULL,  NULL},
  {"Screensaver",  NULL,   NULL,   NULL,   NULL, &screensaverEnabled,  NULL},
  {"Saver Min",  NULL,   NULL,   &screensaverMinutes,   NULL, NULL,  NULL},
  {"Volt Mode", NULL, NULL, NULL, &voltageFormatMode, NULL, NULL},
  {"Menu TO",  NULL,   NULL,   &menuTimeout,   NULL, NULL,  NULL},
  {"Peak Mode", NULL, NULL, NULL, NULL, &showPeakMode, NULL},
  {"SWR Min Pwr", NULL, NULL, NULL, NULL, &minSWRPowerCheckEnabled, NULL},
  {"Min SWR Pwr", NULL, NULL, &minSWRPowerThreshold, NULL, NULL, NULL},
  {"swr Fault Cnt", NULL, NULL, &swrFaultCountLimit, NULL, NULL, NULL}
};

const int SETTINGS_COUNT = sizeof(mySettings) / sizeof(mySettings[0]);

// ==================== YOUR PROVEN LOOKUP TABLES - MOVED TO powerTables.h ====================

void drawMenuPage();  // ← Add this line - are these still needed?
void drawPeakButton();  // Forward declaration
void drawCrossNeedleMainPage();

/* Display flushing for LVGL v9 */
void my_disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    // Cast px_map to uint16_t because TFT_eSPI expects 16-bit colors
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();

    lv_display_flush_ready(disp);
}

// ==================== YOUR PROVEN LOOKUP FUNCTION ====================

float lookupPower(float voltage, bool isHighRange, bool isRev) {
  const float (*table)[2];
  int tableSize;

  if (isRev) {
    table = powerTableRev;
    tableSize = sizeof(powerTableRev) / sizeof(powerTableRev[0]);
  } else if (isHighRange) {
    table = powerTableHigh;
    tableSize = sizeof(powerTableHigh) / sizeof(powerTableHigh[0]);
  } else {
    table = powerTableLow;
    tableSize = sizeof(powerTableLow) / sizeof(powerTableLow[0]);
  }

  if (voltage <= table[0][0]) return table[0][1];
  if (voltage >= table[tableSize - 1][0]) return table[tableSize - 1][1];

  for (int i = 0; i < tableSize - 1; i++) {
    if (voltage >= table[i][0] && voltage <= table[i + 1][0]) {
      float fraction = (voltage - table[i][0]) / (table[i + 1][0] - table[i][0]);
      return table[i][1] + fraction * (table[i + 1][1] - table[i][1]);
    }
  }
  return 0.0;
}

//=============Voltage Format===========
float powerToRMS(float power) {
  if (power <= 0.0f) return 0.0f;
  return sqrt(power * 50.0f);
}

float powerToPeak(float power) {
    // If power is less than 10mW (0.01W), return 0.0 to stop the jitter
    if (power < 0.01f) return 0.0f;

    // High-precision math
    return powerToRMS(power) * sqrtf(2.0f);
}

//  showError() FUNCTION
void showError(const char* message) {
  tft.fillScreen(TFT_BLACK);
  //tft.setFreeFont(&FreeSansBold12pt7b);
  tft.setTextColor(TFT_RED);
  tft.drawString("ERROR", 140, 80);
  //tft.setFreeFont(&FreeSansBold12pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.drawString(message, 40, 140);
  tft.drawString("Check I2C wiring", 40, 180);
  tft.drawString("and power", 40, 210);
  while(1) delay(100);
}


// Load saved settings
void loadSettings() {
  preferences.begin("swr-meter", false);
  
  swrAlarmThreshold = preferences.getFloat("alarmThresh", 2.50f);
  voltageFormatMode = preferences.getUInt("voltMode", 0);
  backLightLevel = preferences.getUChar("backlight", 102);    // Default 40% brightness
  wifiEnabled = preferences.getBool("wifiEnabled", true);
  webServerEnabled = preferences.getBool("webServerEn", false);
  useStaticIP = preferences.getBool("useStaticIP", false);
  local_IP = IPAddress(preferences.getUInt("ipAddr", 0xC0A80164)); // Default 192.168.1.100
  gateway = IPAddress(preferences.getUInt("gateway", 0xC0A80101));  // Default 192.168.1.1
  subnet = IPAddress(preferences.getUInt("subnet", 0xFFFFFF00));   // Default 255.255.255.0
  ssid = preferences.getString("ssid", "YOUR_SSID");
  password = preferences.getString("password", "YOUR_PASSWORD");
  udpEnabled = preferences.getBool("udpEn", false); 
  udpAddress = IPAddress(preferences.getUInt("udpAddr", 0xC0A801FF)); 
  udpPort = preferences.getInt("udpPort", 6999);
  screensaverEnabled = preferences.getBool("ssEnabled", true); // Default to ON
  screensaverMinutes = preferences.getInt("ssMin", 3);
  menuTimeout = preferences.getInt("menuTO", 30);
  showPeakMode = preferences.getBool("peakMode", true);
  useCrossNeedle = preferences.getBool("needleFace", false);
  minSWRPowerCheckEnabled = preferences.getBool("minPwrEn", false);
  minSWRPowerThreshold = preferences.getInt("minPwrVal", 10);
  swrFaultCountLimit = preferences.getInt("faultLimit", 3);
  preferences.end();
}

void saveSettings() {
  preferences.begin("swr-meter", false);
  preferences.putFloat("alarmThresh", swrAlarmThreshold);
  preferences.putUInt("voltMode", voltageFormatMode);
  preferences.putUChar("backlight", backLightLevel);
  preferences.putBool("wifiEnabled", wifiEnabled);
  preferences.putBool("webServerEn", webServerEnabled);
  preferences.putBool("useStaticIP", useStaticIP);
  preferences.putUInt("ipAddr", (uint32_t)local_IP);
  preferences.putUInt("gateway", (uint32_t)gateway);
  preferences.putUInt("subnet", (uint32_t)subnet);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putBool("udpEn", udpEnabled);
  preferences.putUInt("udpAddr", (uint32_t)udpAddress);
  preferences.putInt("udpPort", udpPort);
  preferences.putBool("ssEnabled", screensaverEnabled);
  preferences.putInt("ssMin", screensaverMinutes);
  preferences.putInt("menuTO", menuTimeout);
  preferences.putBool("peakMode", showPeakMode);
  preferences.putBool("needleFace", useCrossNeedle);
  preferences.putBool("minPwrEn", minSWRPowerCheckEnabled);
  preferences.putInt("minPwrVal", minSWRPowerThreshold);
  preferences.putInt("faultLimit", swrFaultCountLimit);
  preferences.end();
}

// ==================== SETUP =========================================

void setup() {
  Serial.begin(115200);
  SPI.begin();
  checkSDCard();  
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  drawSplashScreen(tft);      //Splash screen
  ts.begin();
  ts.setRotation(1);
  lv_init();
  //LVGL buffer
  uint32_t buf_size = 480 * 30 * sizeof(lv_color_t);
  draw_buf_1 = (uint8_t *)malloc(buf_size);

  if (draw_buf_1 == NULL) {
      // Fallback to smaller buffer if RAM is tight
      buf_size = 480 * 10 * sizeof(lv_color_t);
      draw_buf_1 = (uint8_t *)malloc(buf_size);
  }

  disp = lv_display_create(480, 320);
  lv_display_set_default(disp);
  lv_display_set_buffers(disp, draw_buf_1, NULL, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);
  lv_indev_set_long_press_time(indev, 800);

  loadSettings();

  Wire.begin(SDA_PIN, SCL_PIN);
  delay(150); // Give the entire I2C bus 150ms to stabilize

  // ADS1115
  if (!ads.begin(0x48)) {
      delay(100); // Second chance - once in a blue moon it wouldn't be detected
      if (!ads.begin(0x48)) showError("ADS1115 NOT FOUND");
  }

  // MCP23008 
  if (!mcp.begin(0x20, &Wire)) {
      delay(100); // Second chance
      if (!mcp.begin(0x20, &Wire)) showError("MCP23008 NOT FOUND");
  }

  // Increase wire rate to 400 Khz and maximize samples per second
  ads.setDataRate(RATE_ADS1115_860SPS);
  Wire.setClock(400000); // Most modern ADS/MCP chips handle 400kHz (Fast Mode) easily
  ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_0, true); // 'true' means continuous
  Wire.setTimeOut(10);

 // Configure pins per truth table R1
  mcp.pinMode(0, OUTPUT);   // GP0 - Fault LED (active low)
  mcp.pinMode(2, OUTPUT);   // GP2 - SWR Fault Relay (active high)
  mcp.pinMode(4, OUTPUT);   // GP4 - Range Relay (active high)
  mcp.pinMode(6, INPUT);    // GP6 - SW1 button input
  mcp.pullUp(6, HIGH);      // Internal pull-up for SW1

  // Backlight PWM test setup
  pinMode(BACKLIGHT_PIN, OUTPUT);
  analogWrite(BACKLIGHT_PIN, backLightLevel);  // Apply default on boot

  // Force safe power-up states
  mcp.digitalWrite(0, HIGH); // Fault LED OFF
  mcp.digitalWrite(2, LOW);  // SWR Fault Relay OFF
  mcp.digitalWrite(4, LOW);  // Range Relay OFF → low range default

  Serial.println("MCP23008 initialized - safe boot states set");

  drawCalibrationScreen();

  float sumFWD = 0.0f;
  float sumREV = 0.0f;

  for (int i = 0; i < ZERO_SAMPLES; i++) {
    lv_tick_inc(5);
    lv_timer_handler();
    sumFWD += ads.readADC_SingleEnded(0) * 0.0001875f;  // Convert to volts
    sumREV += ads.readADC_SingleEnded(1) * 0.0001875f;
    delay(ZERO_DELAY_MS);
  }

  zeroOffsetFWD = sumFWD / ZERO_SAMPLES;
  zeroOffsetREV = sumREV / ZERO_SAMPLES;

  Serial.print("Zero calibration complete - FWD offset: ");
  Serial.print(zeroOffsetFWD, 6);
  Serial.print(" V, REV offset: ");
  Serial.println(zeroOffsetREV, 6);

  //tft.fillScreen(TFT_BLACK);
  drawMainPage();  
  Serial.println(">>> drawMainPage() finished.");
  lv_obj_t * cal_scr = lv_screen_active();
  lv_screen_load(screen_main);
  lv_refr_now(NULL);
  lv_timer_handler(); 
  lv_obj_delete(cal_scr);

    // start wifi (will only start if enabled)
    setupWiFi();
}


// ================================== MAIN LOOP ===================================================================

void loop() {

    static uint32_t last_lvgl_run = 0;
    static uint32_t last_tick = millis();

    // Always update the tick so LVGL knows time is moving
    uint32_t now = millis();
    lv_tick_inc(now - last_tick);
    last_tick = now;

    // ONLY run the UI handler every 5ms
    if (now - last_lvgl_run > 10) {
        lv_timer_handler(); 
        last_lvgl_run = now;
    }

    static uint32_t frame_count = 0;
    static uint32_t last_check = 0;
    frame_count++;


   // CTA  For troubleshooting, tells me how many loops per second the code is running
    // if (millis() - last_check >= 1000) {
    //     Serial.print("Loops per second: ");
    //     Serial.println(frame_count);
    //     frame_count = 0;
    //     last_check = millis();
    // }

  //  CTA trying to optimize this
  // for (int i = 0; i < ADC_AVERAGE_COUNT; i++) {
  //   fwdRaw += ads.readADC_SingleEnded(0);
  //   revRaw += ads.readADC_SingleEnded(1);
  //   delay(1);
  // }
  //fwdRaw /= ADC_AVERAGE_COUNT; revRaw /= ADC_AVERAGE_COUNT;

  static int  currentSample = 0;
  static long fwdAccumulator = 0;
  static long revAccumulator = 0;
  static int  adcState = 0; // 0: Start FWD, 1: Wait FWD, 2: Start REF, 3: Wait REF
  static float fwdMaxInBatch = 0;
  static float refMaxInBatch = 0;

  // --- NON-BLOCKING ADC ENGINE ---
  bool dataReady = false;

    // --- HIGH SPEED ADC SAMPLING ---
    static uint32_t lastAdcMicros = 0;
    const uint32_t adcInterval = 1200; // 1.2ms (slightly more than 1/860s)

    switch (adcState) {
        case 0: // Trigger FWD Reading
            ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_0, false);
            lastAdcMicros = micros();
            adcState = 1;
            break;

        case 1: // Wait for FWD, then Process
            if (micros() - lastAdcMicros >= adcInterval) {
                int16_t rawF = ads.getLastConversionResults();
                fwdAccumulator += rawF;

                // --- INSTANT PEAK DETECTION ---
                instantV = ads.computeVolts(rawF) - zeroOffsetFWD;
                instantP = lookupPower(max(0.0f, instantV), highRange, false);

                barSmoothFwd = (instantP > MIN_POWER_THRESHOLD) ? instantP : 0.0f;

                if (instantP > MIN_POWER_THRESHOLD) {
                    if (instantP >= peakFWD){
                    peakFWD = instantP;
                        peakHoldStart = millis();
                    }
                    peakHolding = true; 
                }

                // Move to REF
                ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_1, false);
                lastAdcMicros = micros();
                adcState = 2;
            }
        break;

        case 2: // Wait for REF, then Process
            if (micros() - lastAdcMicros >= adcInterval) {
                int16_t rawR = ads.getLastConversionResults();
                revAccumulator += rawR;

                instantRV = ads.computeVolts(rawR) - zeroOffsetREV;
                instantRP = lookupPower(max(0.0f, instantRV), highRange, true);

                // THE REF BAR: Instant tracking (Up AND Down)
                // Follows the REF signal 1:1. Zeroes out if below threshold.
                barSmoothRef = (instantRP > (MIN_POWER_THRESHOLD)) ? instantRP : 0.0f;

                //  THE REF PEAK LINE: Only moves UP here.
                // Falls via your decay block once barSmoothRef drops.
                if (instantRP > peakREF && instantRP > (MIN_POWER_THRESHOLD)) {
                    peakREF = instantRP;
                    // Note: We usually let FWD trigger the peakHoldStart timer for both
                }

                // check for high SWR
                bool skipCheck = (minSWRPowerCheckEnabled && instantP < minSWRPowerThreshold);

                if (skipCheck) {
                    highSWRCount = 0; 
                } 
                else {
                    if (instantP > MIN_POWER_THRESHOLD || instantRP > MIN_POWER_THRESHOLD) {
                        
                        instantP = max(instantP, MIN_POWER_THRESHOLD ); 
                        
                        float instRho = sqrtf(instantRP / instantP);
                        float instSWR = (instRho < 0.98f) ? (1.0f + instRho) / (1.0f - instRho) : 99.9f;

                        if (instSWR > swrAlarmThreshold) {
                            highSWRCount++; 
                            
                            if (highSWRCount >= swrFaultCountLimit && !faultState) {
                                faultState = true;
                                mcp.digitalWrite(2, HIGH); // SWR Fault Relay ON
                                mcp.digitalWrite(0, LOW);  // Fault LED ON
                                Serial.printf("!!! SWR FAULT: %.2f (Count: %d) !!!\n", instSWR, highSWRCount);
                            }
                        } else {
                            highSWRCount = 0; 
                        }
                    }
                }

                currentSample++;
                
                // Trigger the "Averaged" UI updates (SWR/Labels)
                if (currentSample >= ADC_AVERAGE_COUNT) {
                    fwdRaw = (float)fwdAccumulator / ADC_AVERAGE_COUNT;
                    revRaw = (float)revAccumulator / ADC_AVERAGE_COUNT;


                    //added to calculate SWR  the instant all data is ready
                    float avgV_F = ads.computeVolts(fwdRaw) - zeroOffsetFWD;
                    float avgV_R = ads.computeVolts(revRaw) - zeroOffsetREV;
                    
                    forwardPower = lookupPower(max(0.0f, avgV_F), highRange, false);
                    reflectedPower = lookupPower(max(0.0f, avgV_R), highRange, true);
                                        
                    if (forwardPower < MIN_POWER_THRESHOLD && reflectedPower >= MIN_POWER_THRESHOLD) {
                        // Forward is zero, reflected exists → meter reversed or open line
                        swr = 99.9f;
                    } 
                    else if (forwardPower >= MIN_POWER_THRESHOLD) {
                        float rho = sqrtf(reflectedPower / forwardPower);
                        swr = (rho >= 0.98f) ? 99.9f : (1.0f + rho) / (1.0f - rho);
                    } 
                    else {
                        swr = 1.0f;
                    }

                    fwdAccumulator = 0;
                    revAccumulator = 0;
                    currentSample = 0;
                    dataReady = true; 
                }

                // Back to FWD
                ads.startADCReading(ADS1X15_REG_CONFIG_MUX_SINGLE_0, false);
                lastAdcMicros = micros();
                adcState = 1; 
            }
            break;
    }

  // --- ONLY RUN MATH AND UI IF DATA IS READY ---
  if (dataReady) {
        forwardVoltage = ads.computeVolts(fwdRaw) - zeroOffsetFWD;
        reflectedVoltage = ads.computeVolts(revRaw) - zeroOffsetREV;

        if (forwardVoltage < 0.0f) forwardVoltage = 0.0f;
        if (reflectedVoltage < 0.0f) reflectedVoltage = 0.0f;

        //fwdPeakPower = fwdPeakRaw;
        //refPeakPower = refPeakRaw;
        forwardPower = lookupPower(forwardVoltage, highRange, false);
        reflectedPower = lookupPower(reflectedVoltage, highRange, true);
       
        //  do some things when we are transmitting
        if (forwardPower >= MIN_POWER_THRESHOLD || reflectedPower >= MIN_POWER_THRESHOLD) {
            if (udpEnabled) udpDelay = udpDelayTX;
            serialDelay = serialDelayTX;

            static uint32_t lastScreensaverReset = 0;
            if (millis() - lastScreensaverReset > 10000) {  // reset inactivity timer for screensaver
                lv_disp_trig_activity(NULL);
                lastScreensaverReset = millis();
            }

            if (screenIsOff) {
                wake_screen();
                screenIsOff = false;
            }
            lastInteractionTime = millis(); 
        } else {
            serialDelay = serialDelayRX;
            udpDelay = udpDelayRX;
        }
        
        // Send meter data to serial port
        if (millis() - lastSerialTime >= serialDelay) {
            lastSerialTime = millis();
            sendDataToCOM();
        }

        // =============RANGE CHECK============================================
        // --- RANGE / RELAY CHECK ---
        bool rangeChanged = false;

        #ifdef VERSION_HF   //  HF Version (160m-10m)
            if (!highRange && forwardPower > 400.0f) {
                highRange = true;
                rangeChanged = true;
                delay(40);
            } else if (highRange && forwardPower < 390.0f) {
                highRange = false;
                rangeChanged = true;
                delay(40);
            }
        #else      // Longwave version (630m 2200m)
            if (!highRange && forwardPower > 210.0f) {
                highRange = true;
                rangeChanged = true;
                delay(40);
            } else if (highRange && forwardPower < 200.0f) {
                highRange = false;
                rangeChanged = true;
                delay(40);
            }
        #endif

        if (rangeChanged) {
            mcp.digitalWrite(4, highRange ? HIGH : LOW);
        }
       dataReady = false;
   } 

    // --- PEAK HOLDING & DECAY LOGIC ---
    static uint32_t lastDecayTick = 0;

    if (peakHolding && (millis() - peakHoldStart > PEAK_HOLD_DURATION)) {
        if (millis() - lastDecayTick >= 10) { 
            lastDecayTick = millis();

            //  Decay the peak values
            peakFWD *= PEAK_DECAY_STEP;
            peakREF *= PEAK_DECAY_STEP;

            //  SAFETY: Don't let the peak line fall below the current bar
            if (peakFWD < barSmoothFwd) peakFWD = barSmoothFwd;
            if (peakREF < barSmoothRef) peakREF = barSmoothRef;

            //  Reset when values get near zero
            if (peakFWD < 0.15f && barSmoothFwd < 0.15f) {
                peakFWD = 0;
                peakREF = 0;
                peakHolding = false;
                barSmoothFwd = 0;
            }
        }
    }

  // SW1 fault reset - only read 10 times a second
    static uint32_t lastSlowTask = 0;
    if (millis() - lastSlowTask > 100) { 
        reading = mcp.digitalRead(6);
        lastSlowTask = millis();
    }

  if (reading != lastSW1State) {
    lastSW1Debounce = millis();
  }

  if ((millis() - lastSW1Debounce) > sw1DebounceDelay) {
    if (reading != currentSW1State) {
      if (currentSW1State == HIGH && reading == LOW) {  // Falling edge: press
        Serial.println(">>> SW1 PRESS DETECTED - RESETTING FAULT <<<");
        if (faultState) {
          faultState = false;
          highSWRCount = 0;
          mcp.digitalWrite(2, LOW);   // Fault relay OFF
          mcp.digitalWrite(0, HIGH);  // Fault LED OFF
          Serial.println("Fault RESET executed");
        } else {
          Serial.println("No active fault - nothing to reset");
        }
      }

      currentSW1State = reading;
    }
  }
 
  lastSW1State = reading;

  // Only send UDP if enough time has passed
  if (udpEnabled && (millis() - lastUdpTime > udpDelay)) {
      sendUdpPacket();
      lastUdpTime = millis();
  }

  static unsigned long iconTimer = 0;

    if (millis() - iconTimer > 1000) { 
        iconTimer = millis();
        if (currentPage == PAGE_MAIN && screen_main != NULL) {
            updateWiFiIcon();
        }
    }

  static uint32_t lastClockUpdate = 0;
  if (millis() - lastClockUpdate > 1000) { 
      updateClock();
      lastClockUpdate = millis();
  }

  static uint32_t lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 500) {
      handleWiFiBackground();
      lastWiFiCheck = millis();
  }
  
  // Screensaver
  if (screensaverEnabled && !screenIsOff) {
      uint32_t timeoutLimit = (uint32_t)screensaverMinutes * 60000;
      if (lv_disp_get_inactive_time(NULL) > timeoutLimit) {
          Serial.println("Auto-timeout: Screen Off");
          go_to_sleep(); // This handles the backlight and the 'shield'
          screenIsOff = true;
      }
  }

    // Menu timeout using LVGL's internal clock
    if (currentPage != PAGE_MAIN) {
        uint32_t inactiveTime = lv_display_get_inactive_time(lv_display_get_default());
        
        if (inactiveTime > (uint32_t)(menuTimeout * 1000)) {
            Serial.println("Timeout reached! Returning to Main...");

            // CREATE the page if it was deleted
            if(screen_main == NULL) {
                drawMainPage(); 
            }
            
            lv_screen_load_anim(screen_main, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, true);
            currentPage = PAGE_MAIN;

            screen_menu_root = NULL;
            screen_settings = NULL;
            screen_network = NULL;
            screen_wifi_scan = NULL;
            screen_keyboard = NULL;
            screen_display = NULL;
            screen_saved_settings = NULL;
            screen_system_info = NULL;
            screen_udp_config = NULL;
            screen_update_firmware = NULL;
            screen_firmware_sd = NULL;

            lv_display_trigger_activity(NULL); 
            forceDataRedraw();
        }
    }

    if (WiFi.status() == WL_CONNECTED && webServerEnabled) {   // handle web requests if required
        if (millis() - lastServerCheck > 20) {
            server.handleClient();
            lastServerCheck = millis();
        }
    }
 

    updateDynamicValues();

  yield();
}

// ================== END MAIN LOOP ===============================================================================================================

void drawCrossNeedleMainPage() {
    crossNeedle_build(screen_main);

    lv_obj_t * btn_menu = lv_button_create(screen_main);
    lv_obj_set_size(btn_menu, 70, 30);
    lv_obj_align(btn_menu, LV_ALIGN_TOP_LEFT, 6, 6);
    lv_obj_set_style_bg_opa(btn_menu, 0, 0);
    lv_obj_set_style_border_width(btn_menu, 3, 0);
    lv_obj_set_style_border_color(btn_menu, lv_color_hex(0x3A3834), 0);
    lv_obj_set_style_radius(btn_menu, 3, 0);
    lv_obj_set_style_shadow_width(btn_menu, 0, 0);
    lv_obj_set_style_bg_color(btn_menu, lv_color_hex(0xD4CDB8), LV_STATE_PRESSED);

    lv_obj_t * lbl_menu = lv_label_create(btn_menu);
    lv_label_set_text(lbl_menu, "MENU");
    lv_obj_set_style_text_font(lbl_menu, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_menu, lv_color_hex(0x1C1C1A), 0);
    lv_obj_center(lbl_menu);

    lv_obj_add_event_cb(btn_menu, [](lv_event_t * e) {
        LV_UNUSED(e);
        lv_obj_t * old_scr = lv_screen_active();
        if (screen_menu_root == NULL) {
            drawMenuRoot();
        }
        lv_screen_load(screen_menu_root);
        currentPage = PAGE_MENU_ROOT;
        if (old_scr == screen_main) {
            screen_main = NULL;
            lv_obj_del_async(old_scr);
        }
    }, LV_EVENT_CLICKED, NULL);
}

void drawMainPage() {
    
    if (screen_main != NULL) return; 
    screen_main = lv_obj_create(NULL);
    lv_obj_remove_flag(screen_main, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(screen_main, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_snap_x(screen_main, LV_SCROLL_SNAP_NONE);
    lv_obj_set_scroll_snap_y(screen_main, LV_SCROLL_SNAP_NONE);
    lv_obj_set_scroll_dir(screen_main, LV_DIR_NONE); // Force no scroll directions

    lv_obj_set_style_bg_color(screen_main, lv_color_hex(0x000000), 0);
    lv_obj_set_layout(screen_main, 0);

    label_clock = NULL;
    lbl_ip = NULL;
    for (int i = 0; i < 5; i++) wifi_bars[i] = NULL;

    if (useCrossNeedle) {
        drawCrossNeedleMainPage();
        return;
    }

    //  Title
    lv_obj_t * header = lv_label_create(screen_main);
    #ifdef VERSION_HF
      lv_label_set_text_static(header, "160M - 10M SWR METER");
    #else
      lv_label_set_text_static(header, "LONGWAVE SWR METER");
    #endif

    lv_obj_set_style_text_font(header, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(header, lv_color_hex(0x00FFFF), 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_CLICKABLE);
    
    //  BIG SWR 
    lv_obj_t * lbl_swr_title = lv_label_create(screen_main);
    lv_obj_set_style_text_font(lbl_swr_title, &montserrat_64_swr, 0);
    lv_obj_set_style_text_color(lbl_swr_title, lv_color_hex(0xEEEEEE), 0);
    lv_label_set_text_static(lbl_swr_title, "SWR:");
    lv_obj_set_pos(lbl_swr_title, 70, 40);
    //lv_obj_set_style_transform_scale(lbl_swr_title, 333, 0); //~ 1.3X size
    lv_obj_clear_flag(lbl_swr_title, LV_OBJ_FLAG_CLICKABLE);

    // Dynamic SWR Value Label
    lbl_swr_val = lv_label_create(screen_main);
    lv_obj_set_style_text_font(lbl_swr_val, &montserrat_64_swr, 0);
    lv_obj_set_style_text_color(lbl_swr_val, lv_palette_main(LV_PALETTE_AMBER), 0);
    lv_label_set_text(lbl_swr_val, "1.00");
    lv_obj_set_pos(lbl_swr_val, 270, 40);
    //lv_obj_set_style_transform_scale(lbl_swr_val, 333, 0); //~ 1.3X size
    lv_obj_add_flag(lbl_swr_val, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(lbl_swr_val, LV_OBJ_FLAG_CLICKABLE);

    //  FWD Section
    lv_obj_t * lbl_fwd_title = lv_label_create(screen_main);
    lv_obj_set_style_text_font(lbl_fwd_title, &lv_font_montserrat_32, 0);
    lv_label_set_text_static(lbl_fwd_title, "FWD:");
    lv_obj_set_style_text_color(lbl_fwd_title, lv_palette_main(LV_PALETTE_LIGHT_GREEN), 0);
    lv_obj_set_pos(lbl_fwd_title, 5, 110);
    lv_obj_clear_flag(lbl_fwd_title, LV_OBJ_FLAG_CLICKABLE);

    lbl_fwd_val = lv_label_create(screen_main);
    lv_obj_set_style_text_font(lbl_fwd_val, &lv_font_montserrat_32, 0);
    lv_label_set_text(lbl_fwd_val, "0.0");
    lv_obj_set_style_text_color(lbl_fwd_val, lv_palette_main(LV_PALETTE_LIGHT_GREEN), 0);
    lv_obj_set_pos(lbl_fwd_val, 105, 110);
    lv_obj_add_flag(lbl_fwd_val, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(lbl_fwd_val, LV_OBJ_FLAG_CLICKABLE);

    lbl_fwd_aux = lv_label_create(screen_main);
    lv_obj_set_style_text_font(lbl_fwd_aux, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_fwd_aux, lv_palette_main(LV_PALETTE_LIGHT_GREEN), 0);
    lv_label_set_text_static(lbl_fwd_aux, "PEAK:");
    lv_obj_set_pos(lbl_fwd_aux, 260, 115);
    lv_obj_clear_flag(lbl_fwd_aux, LV_OBJ_FLAG_CLICKABLE);

    lbl_fwd_aux_val = lv_label_create(screen_main);
    lv_obj_set_style_text_font(lbl_fwd_aux_val, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_fwd_aux_val, lv_palette_main(LV_PALETTE_LIGHT_GREEN), 0);
    lv_label_set_text(lbl_fwd_aux_val, "0.0");
    lv_obj_set_pos(lbl_fwd_aux_val, 340, 115);
    lv_obj_add_flag(lbl_fwd_aux_val, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(lbl_fwd_aux_val, LV_OBJ_FLAG_CLICKABLE);

    //Create one "Line Style" globally or at the top of setup
    static lv_style_t style_peak_line;
    lv_style_init(&style_peak_line);
    lv_style_set_radius(&style_peak_line, 0);
    lv_style_set_border_width(&style_peak_line, 0);
    lv_style_set_pad_all(&style_peak_line, 0);
    lv_style_set_bg_opa(&style_peak_line, LV_OPA_COVER);


    // --- FWD Bar ---
    bar_fwd = lv_bar_create(screen_main);
    lv_obj_set_size(bar_fwd, 470, 40);
    lv_obj_set_pos(bar_fwd, 5, 150);
    lv_obj_set_style_bg_color(bar_fwd, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
    
    lv_obj_set_style_radius(bar_fwd, 0, 0);                   // Background radius
    lv_obj_set_style_radius(bar_fwd, 0, LV_PART_INDICATOR);   // Indicator (green part) radius

    scale_fwd = lv_scale_create(screen_main);
    lv_obj_set_size(scale_fwd, 470, 40);      
    //lv_obj_set_style_bg_opa(scale_fwd, 0, 0);
    lv_obj_set_pos(scale_fwd, 5, 150);
    lv_obj_add_flag(scale_fwd, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(scale_fwd, LV_OBJ_FLAG_CLICKABLE); 

    // outline/border
    lv_obj_set_style_border_color(bar_fwd, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    lv_obj_set_style_border_width(bar_fwd, 1, LV_PART_MAIN);
    lv_obj_set_style_border_opa(bar_fwd, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_side(bar_fwd, LV_BORDER_SIDE_FULL, LV_PART_MAIN);

    // Mode: Use TOP so ticks point UP from the bottom edge into the bar
    lv_scale_set_mode(scale_fwd, LV_SCALE_MODE_HORIZONTAL_TOP);

    // Align exactly centered ON TOP of the bar
    lv_obj_align_to(scale_fwd, bar_fwd, LV_ALIGN_CENTER, 0, 0);

    // TICK CONFIGURATION
    lv_scale_set_range(scale_fwd, 0, 10);
    lv_scale_set_total_tick_count(scale_fwd, 21); 
    lv_scale_set_major_tick_every(scale_fwd, 2); 

    // MAJOR TICKS
    lv_obj_set_style_length(scale_fwd, 8, LV_PART_INDICATOR); // Longer
    lv_obj_set_style_line_width(scale_fwd, 3, LV_PART_INDICATOR); // Thicker

    // MINOR TICKS
    lv_obj_set_style_length(scale_fwd, 4, LV_PART_ITEMS);    // Shorter
    lv_obj_set_style_line_width(scale_fwd, 1, LV_PART_ITEMS);  // Thinner
    
    // STYLING
    lv_obj_set_style_text_font(scale_fwd, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(scale_fwd, lv_color_white(), LV_PART_MAIN);
    
    // Set tick colors 
    lv_obj_set_style_line_color(scale_fwd, lv_color_white(), LV_PART_ITEMS);     // Major ticks
    lv_obj_set_style_line_color(scale_fwd, lv_color_white(), LV_PART_INDICATOR); // Minor ticks

  // REF Section
    lv_obj_t * lbl_ref_title = lv_label_create(screen_main);
    lv_obj_set_style_text_font(lbl_ref_title, &lv_font_montserrat_32, 0);
    lv_label_set_text_static(lbl_ref_title, "REF:");
    lv_obj_set_style_text_color(lbl_ref_title, lv_color_hex(0xFF0000), 0);
    lv_obj_set_pos(lbl_ref_title, 5, 245);
    lv_obj_clear_flag(lbl_ref_title, LV_OBJ_FLAG_CLICKABLE);

    lbl_ref_val = lv_label_create(screen_main);
    lv_obj_set_style_text_font(lbl_ref_val, &lv_font_montserrat_32, 0);
    lv_label_set_text(lbl_ref_val, "0.0");
    lv_obj_set_style_text_color(lbl_ref_val, lv_color_hex(0xFF0000), 0);
    lv_obj_set_pos(lbl_ref_val, 105, 245);
    lv_obj_add_flag(lbl_ref_val, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(lbl_ref_val, LV_OBJ_FLAG_CLICKABLE);

    lbl_ref_aux = lv_label_create(screen_main);
    lv_obj_set_style_text_font(lbl_ref_aux, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_ref_aux, lv_palette_main(LV_PALETTE_RED), 0);
    lv_label_set_text_static(lbl_ref_aux, "PEAK:");
    lv_obj_set_pos(lbl_ref_aux, 260, 250);
    lv_obj_clear_flag(lbl_ref_aux, LV_OBJ_FLAG_CLICKABLE);

    lbl_ref_aux_val = lv_label_create(screen_main);
    lv_obj_set_style_text_font(lbl_ref_aux_val, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_ref_aux_val, lv_palette_main(LV_PALETTE_RED), 0);
    lv_label_set_text(lbl_ref_aux_val, "0.0");
    lv_obj_set_pos(lbl_ref_aux_val, 340, 250);
    lv_obj_add_flag(lbl_ref_aux_val, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(lbl_ref_aux_val, LV_OBJ_FLAG_CLICKABLE);

    // --- REF Bar (Reflected Power) ---
    bar_ref = lv_bar_create(screen_main);
    lv_obj_set_size(bar_ref, 470, 40);
    // Y Position: 150 (FWD) + 40 (Height) + 10 (Gap) = 200
    lv_obj_set_pos(bar_ref, 5, 200); 
    lv_bar_set_range(bar_ref, 0, refVisualMax * 10);

    // Using Orange or Red for Reflected power
    lv_obj_set_style_bg_color(bar_ref, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
    
    // Square edges
    lv_obj_set_style_radius(bar_ref, 0, 0); 
    lv_obj_set_style_radius(bar_ref, 0, LV_PART_INDICATOR);

    // Outline/Border for REF bar
    lv_obj_set_style_border_color(bar_ref, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    lv_obj_set_style_border_width(bar_ref, 1, LV_PART_MAIN);
    lv_obj_set_style_border_opa(bar_ref, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_side(bar_ref, LV_BORDER_SIDE_FULL, LV_PART_MAIN);

    // --- REF Scale ---
    scale_ref = lv_scale_create(screen_main);
    lv_obj_set_size(scale_ref, 470, 40);
    lv_obj_set_style_bg_opa(scale_ref, 0, 0);
    lv_obj_set_pos(scale_ref, 5, 200);
    lv_obj_add_flag(scale_ref, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(scale_ref, LV_OBJ_FLAG_CLICKABLE); 

    lv_scale_set_mode(scale_ref, LV_SCALE_MODE_HORIZONTAL_TOP);
    lv_obj_align_to(scale_ref, bar_ref, LV_ALIGN_CENTER, 0, 0);

    // TICK CONFIGURATION (Matching your FWD setup)
    lv_scale_set_range(scale_ref, 0, 2); 
    lv_scale_set_total_tick_count(scale_ref, 9); 
    lv_scale_set_major_tick_every(scale_ref, 2); 
    lv_scale_set_text_src(scale_ref, labels_2w);

    // Tick Lengths
    lv_obj_set_style_length(scale_ref, 4, LV_PART_ITEMS);
    lv_obj_set_style_length(scale_ref, 8, LV_PART_INDICATOR);
    
    // STYLING
    lv_obj_set_style_text_font(scale_ref, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(scale_ref, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_line_color(scale_ref, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_line_color(scale_ref, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_line_width(scale_ref, 1, LV_PART_ITEMS); 
    lv_obj_set_style_line_width(scale_ref, 3, LV_PART_INDICATOR);

    //  Menu Button and Action
    lv_obj_t * btn_menu = lv_button_create(screen_main);
    lv_obj_set_size(btn_menu, 70, 30);
    lv_obj_align(btn_menu, LV_ALIGN_BOTTOM_MID, -45, -5);
    lv_obj_set_style_bg_opa(btn_menu, 0, 0); // Transparent background
    lv_obj_set_style_border_width(btn_menu, 2, 0);
    lv_obj_set_style_border_color(btn_menu, lv_color_hex(0x00FFFF), 0); 
    lv_obj_set_style_bg_color(btn_menu, lv_palette_darken(LV_PALETTE_BLUE, 2), LV_STATE_PRESSED);
    lv_obj_t * lbl_menu = lv_label_create(btn_menu);
    lv_obj_set_style_bg_opa(lbl_menu, 0, 0); // Transparent background
      lv_obj_set_style_border_color(lbl_menu, lv_color_hex(0x00FFFF), 0); 
    lv_obj_set_style_bg_color(lbl_menu, lv_palette_darken(LV_PALETTE_BLUE, 2), LV_STATE_PRESSED);
    lv_label_set_text(lbl_menu, "MENU");
    lv_obj_center(lbl_menu);
    lv_obj_add_event_cb(btn_menu, [](lv_event_t * e) {
    lv_obj_t * old_scr = lv_screen_active(); 
    if (screen_menu_root == NULL) {
        drawMenuRoot(); 
    }
    lv_screen_load(screen_menu_root);
    currentPage = PAGE_MENU_ROOT;
    if (old_scr == screen_main) {
        screen_main = NULL; 
        lv_obj_del_async(old_scr); 
    }
        
    }, LV_EVENT_CLICKED, NULL);

  // PEAK BUTTON
  btn_peak = lv_button_create(screen_main);
  lv_obj_set_size(btn_peak, 70, 30);
  lv_obj_align(btn_peak, LV_ALIGN_BOTTOM_MID, 45, -5);

  // Style it (Outline Style)
  lv_obj_set_style_bg_opa(btn_peak, 0, 0); 
  lv_obj_set_style_border_width(btn_peak, 2, 0);
  // lv_obj_set_style_border_color(btn_peak, lv_color_hex(0x00FFFF), 0);
  lv_obj_set_style_bg_color(btn_peak, lv_palette_darken(LV_PALETTE_BLUE, 2), LV_STATE_PRESSED);
  // Add the Label
  lbl_peak_btn = lv_label_create(btn_peak);
  lv_label_set_text(lbl_peak_btn, showPeakMode ? "PEAK" : "VOLTS");
  lv_obj_center(lbl_peak_btn);
  if (showPeakMode) {
     lv_obj_set_style_border_color(btn_peak, lv_palette_main(LV_PALETTE_AMBER), 0);
     lv_obj_set_style_text_color(lbl_peak_btn, lv_palette_main(LV_PALETTE_AMBER), 0);
     saveSettings();
   } else {
     lv_obj_set_style_border_color(btn_peak, lv_color_hex(0x00FFFF), 0); // Cyan
     lv_obj_set_style_text_color(lbl_peak_btn, lv_color_hex(0x00FFFF), 0);
     saveSettings();
   }

  lv_obj_add_event_cb(btn_peak, [](lv_event_t * e) {
      showPeakMode = !showPeakMode;
      lv_label_set_text(lbl_peak_btn, showPeakMode ? "PEAK" : "VOLTS");
      if (showPeakMode) {
          lv_obj_set_style_border_color(btn_peak, lv_palette_main(LV_PALETTE_AMBER), 0);
          lv_obj_set_style_text_color(lbl_peak_btn, lv_palette_main(LV_PALETTE_AMBER), 0);
          saveSettings();
      } else {
          lv_obj_set_style_border_color(btn_peak, lv_color_hex(0x00FFFF), 0); // Cyan
          lv_obj_set_style_text_color(lbl_peak_btn, lv_color_hex(0x00FFFF), 0);
          saveSettings();
      }
      lastFwdV = -999.0f;
      lastRefV = -999.0f;
  }, LV_EVENT_CLICKED, NULL);



    
    // IP address
    lbl_ip = lv_label_create(screen_main);
    lv_label_set_text(lbl_ip, ipBuf); 
    lv_obj_set_style_text_font(lbl_ip, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_ip, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(lbl_ip, LV_ALIGN_BOTTOM_LEFT, 3, -3);
    lv_obj_clear_flag(lbl_ip, LV_OBJ_FLAG_CLICKABLE);
    
    // --- Peak Forward Line ---
    peak_fwd_line = lv_obj_create(screen_main);
    lv_obj_add_style(peak_fwd_line, &style_peak_line, 0); 
    lv_obj_set_size(peak_fwd_line, 3, 40);
    lv_obj_set_pos(peak_fwd_line, 5, 150);
    lv_obj_set_style_bg_color(peak_fwd_line, lv_color_hex(0x33FF33), 0);

    // Performance & Safety Flags
    lv_obj_add_flag(peak_fwd_line, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(peak_fwd_line, LV_OBJ_FLAG_CLICKABLE); // Stops touch-math on this object

    // --- Peak Reflected Line ---
    peak_ref_line = lv_obj_create(screen_main);
    lv_obj_add_style(peak_ref_line, &style_peak_line, 0); 
    lv_obj_set_size(peak_ref_line, 3, 40);
    lv_obj_set_pos(peak_ref_line, 5, 200);
    lv_obj_set_style_bg_color(peak_ref_line, lv_color_hex(0xFF3333), 0);

    // Performance & Safety Flags
    lv_obj_add_flag(peak_ref_line, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(peak_ref_line, LV_OBJ_FLAG_CLICKABLE);

    //  Meter Range - CTA removed 02/26/2026
    // label_range = lv_label_create(screen_main);
    // lv_obj_set_style_text_font(label_range, &lv_font_montserrat_14, 0); 
    // lv_obj_set_pos(label_range, 170, 300);
    // // Set initial state
    // lv_label_set_text(label_range, highRange ? "RANGE: HIGH" : "RANGE: LOW");
    // lv_obj_set_style_text_color(label_range, highRange ? lv_palette_main(LV_PALETTE_AMBER) : lv_color_hex(0x00FFFF), 0);

    // SWR threshold indicator for main screen
    lbl_swr_limit = lv_label_create(screen_main);
    String swr_val = String(swrAlarmThreshold, 1);
    lv_label_set_text(lbl_swr_limit, swr_val.c_str()); 
    lv_obj_set_style_text_font(lbl_swr_limit, &lv_font_montserrat_24, 0);

    lv_obj_set_style_bg_color(lbl_swr_limit, lv_palette_main(LV_PALETTE_AMBER), 0);
    lv_obj_set_style_bg_opa(lbl_swr_limit, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(lbl_swr_limit, lv_color_hex(0x000000), 0); 

    lv_obj_set_style_pad_left(lbl_swr_limit, 8, 0);
    lv_obj_set_style_pad_right(lbl_swr_limit, 8, 0);
    lv_obj_set_style_radius(lbl_swr_limit, 3, 0); 

    lv_obj_align(lbl_swr_limit, LV_ALIGN_BOTTOM_RIGHT, -3, -3);
        
    drawClock();
    setupWiFiIcon(screen_main);
    updateClock();
    //forceDataRedraw();
    lastFwdV = -999.0f;
    lastRefV = -999.0f;
}

void updateDynamicValues() {

  if (isUpdatingOTA) return;
  static uint32_t lastUpdate = 0;
  if (millis() - lastUpdate < 20) return; // Only run 20 times per second
  lastUpdate = millis();
  if (screen_main == NULL) return;
  // CTA added safety checks
  if (forwardPower < 0.01f) forwardPower = 0.0f;
  if (reflectedPower < 0.01f) reflectedPower = 0.0f;
  if (swr < 1.0f || isnan(swr)) swr = 1.0f;

  if (useCrossNeedle) {
      bool alarm = (forwardPower >= MIN_POWER_THRESHOLD && swr > swrAlarmThreshold);
      crossNeedle_update(forwardPower, reflectedPower, alarm, swrAlarmThreshold);
      return;
  }

 if (fabs(swr - lastSwr) >= SWR_CHANGE_THRESHOLD) {
 
     // Giant SWR
      char swrBuf[10];
      dtostrf(swr, 4, 2, swrBuf); // Very reliable float-to-string for Arduino
      lv_label_set_text(lbl_swr_val, swrBuf);

      if (swr > swrAlarmThreshold) {
          lv_obj_set_style_text_color(lbl_swr_val, lv_palette_main(LV_PALETTE_RED), 0);
      } else {
          lv_obj_set_style_text_color(lbl_swr_val, lv_palette_main(LV_PALETTE_AMBER), 0);
      }
      lastSwr = swr; // Update the tracking variable
  }

    // FWD power
    if (fabs(forwardPower - lastForwardPower) >= POWER_CHANGE_THRESHOLD) {

        char fwdBuf[16];
        if (isnan(forwardPower) || isinf(forwardPower)) forwardPower = 0.0;
        snprintf(fwdBuf, sizeof(fwdBuf), "%.1f W", (double)forwardPower);
        lv_label_set_text(lbl_fwd_val, fwdBuf);

        int newMax;  
        int totalTicks;
        int majorEvery;

        // Dynamic Peak Scale Logic
        if (forwardPower <= 10.0f) {
            newMax = 10;   
            totalTicks = 21; // 0, 1, 2...10
            majorEvery = 2;  // Label every 1W
        } else if (forwardPower <= 100.0f) {
            newMax = 100;   
            totalTicks = 21;
            majorEvery = 2; 
        } else if  (forwardPower <= 200.0f) {
            newMax = 200;   
            totalTicks = 21;
            majorEvery = 2; 
        } else if (forwardPower <= 500.0f) {
            newMax = 500;
            totalTicks = 21;
            majorEvery = 2;
        } else {
            newMax = 1000;
            totalTicks = 21;
            majorEvery = 2;
        }
        // ONLY update the scale if the range actually changed
        if (newMax != fwdVisualMax) {
            fwdVisualMax = newMax;
            lv_scale_set_range(scale_fwd, 0, fwdVisualMax);
            lv_scale_set_total_tick_count(scale_fwd, totalTicks);
            lv_scale_set_major_tick_every(scale_fwd, majorEvery);
            lv_bar_set_range(bar_fwd, 0, fwdVisualMax*10);
            lv_obj_set_style_length(scale_fwd, 4, LV_PART_ITEMS);
            lv_obj_set_style_line_width(scale_fwd, 1, LV_PART_ITEMS);
            lv_obj_set_style_length(scale_fwd, 8, LV_PART_INDICATOR);
            lv_obj_set_style_line_width(scale_fwd, 3, LV_PART_INDICATOR);
            Serial.printf("Scale auto-ranged to %d W\n", fwdVisualMax);
        }

     lv_bar_set_value(bar_fwd, (int32_t)((barSmoothFwd * 10.0f) + .5f), LV_ANIM_OFF);
    }
    
    static int32_t last_x_fwd = -1;
    int32_t x_fwd = lv_map((int32_t)((peakFWD * 10.0f) + .5f), 0, fwdVisualMax * 10, 0, 470);
    if (x_fwd != last_x_fwd) {
      lv_obj_set_x(peak_fwd_line, x_fwd + 2); 
      last_x_fwd = x_fwd;
    }

  // REF power
  
  if (fabs(reflectedPower - lastReflectedPower) >= 0.05) {

        char refBuf[16];
        if (isnan(reflectedPower) || isinf(reflectedPower)) reflectedPower = 0.0;
        snprintf(refBuf, sizeof(refBuf), "%.1f W", (double)reflectedPower);
        lv_label_set_text(lbl_ref_val, refBuf);

        // --- REF Dynamic Scale Logic ---
        int newMaxRef;  
        int totalTicksRef;
        int majorEveryRef;

        if (reflectedPower <= 2.0f) {
            newMaxRef = 2;     
            totalTicksRef = 9;  
            majorEveryRef = 2;
        } else if (reflectedPower <= 10.0f) {
            newMaxRef = 10;    
            totalTicksRef = 11; 
            majorEveryRef = 2;  
        } else {
            newMaxRef = 50;
            totalTicksRef = 11;
            majorEveryRef = 2;
        }

        //  APPLY CHANGES (Only if range changed)
        if (newMaxRef != refVisualMax) {
            refVisualMax = newMaxRef;
            
            // Match bar and scale ranges
            lv_bar_set_range(bar_ref, 0, refVisualMax * 10);
            lv_scale_set_range(scale_ref, 0, refVisualMax);
            lv_scale_set_total_tick_count(scale_ref, totalTicksRef);
            lv_scale_set_major_tick_every(scale_ref, majorEveryRef);
            lv_obj_set_style_length(scale_ref, 4, LV_PART_ITEMS);
            lv_obj_set_style_line_width(scale_ref, 1, LV_PART_ITEMS);
            lv_obj_set_style_length(scale_ref, 8, LV_PART_INDICATOR);
            lv_obj_set_style_line_width(scale_ref, 3, LV_PART_INDICATOR);

            // Map the correct text strings to the scale
            if (refVisualMax == 2) {
                lv_scale_set_text_src(scale_ref, labels_2w);
            } else if (refVisualMax == 10) {
                lv_scale_set_text_src(scale_ref, labels_10w);
            } else {
                lv_scale_set_text_src(scale_ref, labels_50w);
            }
        }  

      lv_bar_set_value(bar_ref, (int32_t)((barSmoothRef * 10.0f) + .5f), LV_ANIM_OFF);
   } 
 
    static int32_t last_x_ref = -1;
    int32_t x_ref = lv_map((int32_t)((peakREF * 10.0f) + .5f), 0, refVisualMax * 10, 0, 470);
    if (x_ref != last_x_ref) {
      lv_obj_set_x(peak_ref_line, x_ref + 2);
    last_x_ref = x_ref;
}

  // ==========FWD V or Peak Watts – now with Voltage Format support====
  float displayedFwd;
  const char* fwdUnit;

  if (showPeakMode) {
    displayedFwd = peakFWD;          // Peak hold in watts
    fwdUnit = "W";
  } else {
    switch (voltageFormatMode) {
      case 0:  // Measured (raw ADC voltage)
        displayedFwd = forwardVoltage;
        fwdUnit = "V";
        break;
      case 1:  // RMS V
        displayedFwd = powerToRMS(forwardPower);
        fwdUnit = "V";
        break;
      case 2:  // Peak V
        displayedFwd = powerToPeak(forwardPower);
        fwdUnit = "V";
        break;
      default: // safety fallback
        displayedFwd = forwardVoltage;
        fwdUnit = "V";
    }
  }

  // --- FWD Auxiliary (Voltage or Peak) Logic ---
  if (fabs(displayedFwd - lastFwdV) >= (showPeakMode ? POWER_CHANGE_THRESHOLD : 0.002)) {
    char auxBuf[20];
    
    snprintf(auxBuf, sizeof(auxBuf), "%6.3f %s", (double)displayedFwd, fwdUnit);

    lv_label_set_text(lbl_fwd_aux_val, auxBuf);

    lastFwdV = displayedFwd;
  }
  // REF V or Peak Watts – now with Voltage Format support
  float displayedRef;
  const char* refUnit = "V";  // always "V" unless in peak watts mode

  if (showPeakMode) {
    displayedRef = peakREF;
    refUnit = "W";
  } else {
    switch (voltageFormatMode) {
      case 0:  // Measured
        displayedRef = reflectedVoltage;
        break;
      case 1:  // RMS
        displayedRef = powerToRMS(reflectedPower);
        break;
      case 2:  // Peak
        displayedRef = powerToPeak(reflectedPower);
        break;
      default:
        displayedRef = reflectedVoltage;
    }
  }


  //--- REF Auxiliary (Voltage or Peak) Logic ---

  if (fabs(displayedRef - lastRefV) >= (showPeakMode ? POWER_CHANGE_THRESHOLD : 0.002)) {
      char refAuxBuf[20];
      snprintf(refAuxBuf, sizeof(refAuxBuf), "%6.3f %s", (double)displayedRef, refUnit);
      lv_label_set_text(lbl_ref_aux_val, refAuxBuf);
      lv_obj_set_style_text_color(lbl_ref_aux_val, lv_color_hex(0xFF0000), 0);
      lastRefV = displayedRef;
  }

  // set range level - CTA removed 02/26/2026

  if (highRange != lastHighRange) {
    // lv_label_set_text(label_range, highRange ? "RANGE: HIGH" : "RANGE: LOW");
    // if (highRange) {
    //     lv_obj_set_style_text_color(label_range, lv_palette_main(LV_PALETTE_AMBER), 0);
    // } else {
    //     lv_obj_set_style_text_color(label_range, lv_color_hex(0x00FFFF), 0); // Cyan
    // }
    lastHighRange = highRange;
  }

}

static void root_menu_event_cb(lv_event_t * e) {
    int index = (int)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t * old_screen = screen_menu_root;

    switch(index) {
        case 0: // SETTINGS
            drawSettingsPage();
            lv_screen_load(screen_settings);
            currentPage = PAGE_SETTINGS; 
            break;
            
        case 1: // DISPLAY
            drawDisplayPage();
            lv_screen_load(screen_display);
            currentPage = PAGE_DISPLAY; 
            break;

        case 2: // NETWORK
            drawNetworkPage();
            lv_screen_load(screen_network);
            currentPage = PAGE_NETWORK; 
            break;
            
        case 3: // SAVED SETTINGS
            drawSavedSettingsPage();
            lv_screen_load(screen_saved_settings);
            currentPage = PAGE_SAVED_SETTINGS; 
            break;

        case 4: // SYSTEM INFO
            drawSystemInfo();
            lv_screen_load(screen_system_info);
            currentPage = PAGE_SYSTEM_INFO; 
            break;
    }

    // Clean up the menu we just left
    if (old_screen != NULL) {
        screen_menu_root = NULL; 
        lv_obj_del_async(old_screen); 
    }
}

void drawMenuRoot() {

    if (screen_menu_root != NULL) {
      return; 
    }
    // Root Menu Screen
    screen_menu_root = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_menu_root, lv_color_hex(0x000000), 0);

    // Menu Labels 
    const char * menu_labels[] = {
        "SETTINGS", "DISPLAY", "NETWORK", "SAVED SETTINGS", "SYSTEM INFO"
    };

    // List-style Buttons
    for(int i = 0; i < 5; i++) {
        lv_obj_t * btn = lv_button_create(screen_menu_root);
        lv_obj_set_size(btn, 360, 50);
        lv_obj_set_pos(btn, 80, 15 + (i * 60)); 
        
        // Dark Grey with Rounded Corners
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x333333), 0); // Dark Grey
        lv_obj_set_style_radius(btn, 8, 0);

        // Labels
        lv_obj_t * lbl = lv_label_create(btn);
        lv_label_set_text(lbl, menu_labels[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0); 
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 20, 0); // Padding from left

        lv_obj_add_event_cb(btn, root_menu_event_cb, LV_EVENT_CLICKED, (void*)i);    
    }

    // Back Button (parent, destination)
    setupBackButton(screen_menu_root, PAGE_MAIN);
}


void sendUdpPacket() {
  if (!udpEnabled || WiFi.status() != WL_CONNECTED) return;

  String payload = buildJsonData();
  udp.beginPacket(udpAddress, udpPort);
  udp.print(payload);
  udp.endPacket();
}

void setupWiFi() {
    if (!wifiEnabled) {
        WiFi.mode(WIFI_OFF);
        return;
    }
    #ifdef VERSION_HF
       WiFi.setHostname("HF-SWR-METER");
    #else
       WiFi.setHostname("LF-SWR-METER");
    #endif

    if (useStaticIP) {
        WiFi.config(local_IP, gateway, subnet, primaryDNS);
    }

    // Start the connection process - this returns IMMEDIATELY
    WiFi.begin(ssid.c_str(), password.c_str());
    WiFi.setTxPower(WIFI_POWER_11dBm);
    WiFi.setSleep(false); // Helps with Web Server responsiveness
    connectionStartTime = millis();
    isAttemptingConnection = true;
    Serial.println("WiFi connection started in background...");
    strcpy(ipBuf, "CONNECTING..."); 
}

void setupWiFiIcon(lv_obj_t * parent) {
    int x_base = 445;
    int y_base = 5;

    for(int i = 0; i < 5; i++) {
        wifi_bars[i] = lv_obj_create(parent);
        // Calculate height like your original: 4, 8, 12, 16, 20
        int bar_h = 4 + (i * 4); 
        lv_obj_set_size(wifi_bars[i], 5, bar_h);
        
        // Position each bar (x increases, y decreases to align bottoms)
        lv_obj_set_pos(wifi_bars[i], x_base + (i * 7), y_base + (20 - bar_h));
        
        // Style: No border, dark grey background initially
        lv_obj_set_style_bg_color(wifi_bars[i], lv_palette_main(LV_PALETTE_GREY), 0);
        lv_obj_set_style_border_width(wifi_bars[i], 0, 0);
    }
}

void updateWiFiIcon() {
    
    if (isUpdatingOTA) return;
    if (wifi_bars[0] == NULL) return;
    int32_t rssi = WiFi.RSSI();
    bool connected = (WiFi.status() == WL_CONNECTED);
    
    // Determine the color based on strength
    lv_color_t signal_color;
    if (!connected || rssi < -80) signal_color = lv_palette_main(LV_PALETTE_RED);
    else if (rssi < -70) signal_color = lv_palette_main(LV_PALETTE_YELLOW);
    else signal_color = lv_palette_main(LV_PALETTE_GREEN);

    // Determine how many bars to light up based on your original logic
    int bars_to_show = 0;
    if (connected) {
        if (rssi > -85) bars_to_show = 2;
        if (rssi > -75) bars_to_show = 3;
        if (rssi > -65) bars_to_show = 4;
        if (rssi > -55) bars_to_show = 5;
        if (bars_to_show == 0) bars_to_show = 1; // Minimum 1 bar if connected
    }

    for (int i = 0; i < 5; i++) {
        if (i < bars_to_show) {
            lv_obj_set_style_bg_color(wifi_bars[i], signal_color, 0);
        } else {
            lv_obj_set_style_bg_color(wifi_bars[i], lv_color_hex(0x333333), 0); // Dim/Off
        }
    }
}

void handleWiFiBackground() {
    if (!wifiEnabled) return;
    if (screen_network == NULL && screen_main == NULL) return;
    //static bool wasConnected = false;
    bool isConnected = (WiFi.status() == WL_CONNECTED);

    if (!isConnected && isAttemptingConnection) {
        if (millis() - connectionStartTime > 15000) { // 15 second timeout
            Serial.println("Connection Timeout: Likely Wrong Password. Disabling WiFi.");
            WiFi.disconnect(true); // Shut down the reconnect loop
            isAttemptingConnection = false;
            
            // Update UI to show the failure
            if (lbl_ip != NULL && lv_obj_is_valid(lbl_ip)) lv_label_set_text(lbl_ip, "AUTH FAILED");
            return;
        }
    }

    // STATE CHANGE: Just connected!
    if (isConnected && !wasConnected) {
        isAttemptingConnection = false;
        Serial.println("WiFi Connected!");
        String ipStr = WiFi.localIP().toString();
        strncpy(ipBuf, ipStr.c_str(), sizeof(ipBuf) - 1);
        ipBuf[sizeof(ipBuf) - 1] = '\0'; // Ensure null termination
        
        // Update the IP label on screen
        if (lbl_ip != NULL && lv_obj_is_valid(lbl_ip)) {
            lv_label_set_text(lbl_ip, ipBuf);
        }

        // Start Web Server
        if (webServerEnabled) {
            setupWebServerRoutes();
            const char * headerkeys[] = {"Content-Length"};
            server.collectHeaders(headerkeys, 1);
            server.begin();
            Serial.println("Web Server Started");
        }
        
        // Configure NTP & clock
        sntp_set_sync_interval(24 * 60 * 60 * 1000);
        configTzTime("UTC0", "pool.ntp.org", "time.nist.gov");

        wasConnected = true;
    }

    // STATE CHANGE: Connection Lost
    if (!isConnected && wasConnected) {
        Serial.println("WiFi Lost");
        strcpy(ipBuf, "RECONNECTING...");
        if (lbl_net_ip != NULL) lv_label_set_text(lbl_net_ip, ipBuf);

        // Turn WiFi Bars RED or GREY
        for(int i = 0; i < 5; i++) {
            if(wifi_bars[i]) lv_obj_set_style_bg_color(wifi_bars[i], lv_palette_main(LV_PALETTE_RED), 0);
        }
        wasConnected = false;
    }
}

String buildJsonData() {
    String json = "{";
    json += "\"v\":" + String(forwardVoltage, 3) + ",";
    json += "\"fwd\":" + String(forwardPower, 1) + ",";
    json += "\"ref\":" + String(reflectedPower, 1) + ",";
    json += "\"peak\":" + String(peakFWD, 1) + ",";
    json += "\"swr\":" + String(swr, 2) + ",";
    json += "\"fault\":" + String(faultState ? 1 : 0) + ",";
    json += "\"tx\":" + String(forwardPower > MIN_POWER_THRESHOLD ? 1 : 0) + ",";
    json += "\"swrThr\":" + String(swrAlarmThreshold, 1) + ",";
    json += "\"tripCnt\":" + String(swrFaultCountLimit) + ",";
    json += "\"bypass\":" + String(minSWRPowerCheckEnabled ? 1 : 0) + ",";
    json += "\"lowThr\":" + String((float)minSWRPowerThreshold, 1);
    json += "}";
    return json;
}

void setupWebServerRoutes() {
    // Init Web API Server
    server.on("/api", HTTP_GET, []() {
        // --- HANDLE INCOMING COMMANDS ---
        if (server.hasArg("action")) {
            String act = server.arg("action");
            if (act == "reset" && forwardPower < MIN_POWER_THRESHOLD) {
                resetSWRFault();
            }
        }
        
        // --- HANDLE SETTINGS UPDATES ---
        if (server.hasArg("set")) {
            String target = server.arg("set");
            float val = server.arg("val").toFloat();
            
            if (target == "swrThr") {
                swrAlarmThreshold = val;
                if (lbl_swr_limit != NULL) {
                    swrAlarmThreshold = constrain(val, 1.1, 5.0);
                    lv_label_set_text(lbl_swr_limit, String(swrAlarmThreshold, 1).c_str());
                    lv_obj_invalidate(lbl_swr_limit); 
                }
            }
            if (target == "tripCnt")   swrFaultCountLimit = (int)constrain(val, 3, 24);
            if (target == "bypass")    minSWRPowerCheckEnabled = (val > 0.5);
            if (target == "lowThr")    minSWRPowerThreshold = constrain(val, 5.0, 50.0);
            saveSettings();
        }

        // JSON RESPONSE
        String json = buildJsonData();
        server.send(200, "application/json", json);
    });

    // Route - The Dashboard (HTML)
    server.on("/", HTTP_GET, [](){
      String html = R"rawdelay(
        <!DOCTYPE html>
        <html>
        <head>
            <meta name='viewport' content='width=device-width, initial-scale=1'>
            <title>SWR METER</title>
            <style>
                body { background: #000; color: #eee; font-family: 'Segoe UI', sans-serif; text-align: center; margin: 0; padding: 0; overflow: hidden; }
                .container { max-width: 480px; margin: auto; position: relative; padding: 10px; display: block; overflow: visible; }
                .container::after {
                    content: "";
                    display: table;
                    clear: both;
                }
                h3 { color: #00FFFF; font-size: 1.3em; margin: 5px 0; letter-spacing: 2px; margin-top: 5px; margin-bottom: 5px; }
                
                /* BIG SWR SECTION */
                .swr-box { 
                    margin: 2px auto; 
                    padding: 2px 10px; /* Reduced vertical padding from 5px to 2px */
                    display: flex; 
                    justify-content: center; 
                    align-items: center; 
                    gap: 15px; /* Adjust this to space out "SWR:" from the number */
                    cursor: pointer; 
                    border-radius: 8px; 
                    transition: 0.2s; 
                }

                .swr-box:active {
                    transform: scale(0.98); /* Tiny physical "click" feel when pressed */
                }

                .swr-title { 
                    font-size: 2.5em; 
                    color: #EEEEEE; 
                    font-weight: bold; 
                    margin: 0; 
                    line-height: 1; /* Added to kill extra vertical space */
                }

                .swr-val { 
                    font-size: 3.5em; 
                    color: #FFB300; 
                    font-weight: bold; 
                    line-height: 1; 
                }
                
                /* METER SECTIONS */
                .meter-group { text-align: left; padding: 0 10px; margin-top: 5px; margin-bottom: 5px; position: relative; }
                .label-row { display: flex; justify-content: space-between; align-items: baseline; margin-bottom: 2px; }
                .main-label { font-size: 1.7em; font-weight: bold; }
                .peak-label { font-size: 0.9em; }
                
                /* THE BARS */
                .bar-container { 
                    position: relative; width: 100%; height: 35px; 
                    background: #222; border: 1px solid #666; 
                }
                .bar-fill { height: 100%; transition: width 0.15s ease-out; }

                .peak-line { 
                    position: absolute; top: 0; width: 3px; height: 100%; 
                    transition: left 0.3s ease-in-out; 
                }
                
                /* COLORS */
                .fwd-text { color: #8BC34A; } /* Light Green */
                .fwd-bg { background: #4CAF50; }
                .fwd-peak { background: #33FF33; }
                
                .ref-text { color: #F44336; } /* Red */
                .ref-bg { background: #F44336; }
                .ref-peak { background: #FF3333; }

                .ticks {
                    position: absolute;
                    top: 0; left: 0;
                    width: 100%; height: 100%;
                    /* This creates a vertical line every 10% of the bar width */
                    background-image: linear-gradient(to right, rgba(255,255,255,0.3) 1px, transparent 1px);
                    background-size: 10% 100%; 
                    pointer-events: none; /* Let clicks pass through to the reset button */
                    z-index: 5;
                }

                /* "Mid-Point" thick tick */
                .ticks::after {
                    content: '';
                    position: absolute;
                    left: 50%; top: 0;
                    width: 2px; height: 100%;
                    background: rgba(255,255,255,0.5);
                }
                .settings-grid { 
                    display: none; grid-template-columns: 1fr 1fr; gap: 10px; 
                    margin: 20px 0; padding: 15px; background: #111; border-radius: 8px; 
                    font-size: 0.9em; border: 1px solid #333;
                    width: 400px;
                }
                .set-item { text-align: left; display: flex; flex-direction: column; }
                .set-item label { color: #888; font-size: 0.7em; text-transform: uppercase; margin-bottom: 3px; }
                select, input { background: #222; color: #0f0; border: 1px solid #444; padding: 5px; border-radius: 3px; }

                /* Hide the settings by default */
                .settings-grid { 
                    display: none; 
                    grid-template-columns: 1fr 1fr; gap: 10px; 
                    margin: 15px auto; padding: 15px; background: #111; 
                    border-radius: 8px; border: 1px solid #333;
                    width: 400px;
                }

                /* Style for the Expand Button */
                .toggle-btn {
                    background: #222; color: #888; border: 1px solid #444;
                    padding: 8px; margin: 10px auto; width: 120px; 
                    border-radius: 5px; cursor: pointer; font-size: 0.8em;
                    letter-spacing: 1px; transition: 0.2s;
                }
                .toggle-btn:hover { background: #333; color: #00FFFF; }
                /* Header Layout */
                .header-row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 5px; }
                .mini-btn { background: #333; color: #00FFFF; border: 1px solid #444; padding: 2px 8px; cursor: pointer; border-radius: 3px; font-size: 0.7em; }

                /* COMPACT MODE OVERRIDES */
                body.compact { padding: 5px; overflow: hidden; }
                body.compact .container { max-width: 220px; }
                body.compact h3 { font-size: 0.9em; letter-spacing: 1px; margin: 4; }
                body.compact .swr-box { margin: 5px 0; padding: 5px; }
                body.compact .swr-title { font-size: 1.2em; }
                body.compact .swr-val { font-size: 1.8em; }
                body.compact .meter-group { margin-top: 5px; padding: 0 5px; }
                body.compact .main-label { font-size: 1.1em; }
                body.compact .peak-label { font-size: 0.9em; }
                body.compact .bar-container { height: 18px; }
                body.compact .settings-icon { display: none; }

                .settings-icon {
                    display: block;
                    float: right;   
                    margin-top: 10px;      
                    margin-right: 5px;
                    margin-bottom: 2px;  
                    font-size: 24px;
                    color: #444; 
                    cursor: pointer;
                    transition: 0.3s;
                    line-height: 1;
                }

                .settings-icon:hover {
                    color: #00FFFF;
                    transform: rotate(90deg);
                }

                /* Hide gear in compact mode */
                body.compact .settings-icon {
                    display: none;
                }
              
                .update-btn {
                    width: 100%;
                    background: #1a1a1a;
                    color: #FFB300; /* Amber to match your SWR numbers */
                    border: 1px solid #444;
                    padding: 10px;
                    border-radius: 4px;
                    cursor: pointer;
                    font-size: 0.9em;
                    font-weight: bold;
                    letter-spacing: 1px;
                    transition: 0.2s;
                }

                .update-btn:hover {
                    background: #331a00; /* Subtle dark amber glow */
                    border-color: #FFB300;
                    color: #fff;
                }
            </style>
        </head>
        <body>
            <div class="container" id="main-ui" style="display:none;">
     )rawdelay";
        #ifdef VERSION_HF
                html += "<h3 class='widget-title'>160M - 10M SWR METER</h3>";
        #else
                html += "<h3 class='widget-title'>LONGWAVE SWR METER</h3>";
        #endif
     html += R"rawdelay(<div class="swr-box" id="swr-card" 
                    onclick="resetFault()" 
                    ondblclick="toggleCompact()" 
                    style="user-select: none;">
                    <span class="swr-title">SWR:</span>
                    <span id="swr" class="swr-val">1.00</span>
               </div>

                <div class="meter-group">
                    <div class="label-row fwd-text">
                        <span>FWD: <b id="fwd">0.0</b> W</span>
                        <span class="peak-label"><span id="fwd-max-label">10</span>W</span>
                    </div>
                    <div class="bar-container">
                        <div class="ticks"></div> <div id="fwd-bar" class="bar-fill fwd-bg" style="width: 0%"></div>
                        <div id="fwd-peak-line" class="peak-line fwd-peak" style="left: 0%"></div>
                    </div>
                </div>

                <div class="meter-group">
                    <div class="label-row ref-text">
                        <span>REF: <b id="ref">0.0</b> W</span>
                        <span class="peak-label"><span id="ref-max-label">2</span>W</span>
                    </div>
                    <div class="bar-container">
                        <div class="ticks"></div> <div id="ref-bar" class="bar-fill ref-bg" style="width: 0%"></div>
                        <div id="ref-peak-line" class="peak-line ref-peak" style="left: 0%"></div>
                    </div>
                </div>

                <div class="settings-grid" id="settings-section">
                    <div class="set-item">
                        <label>SWR Threshold</label>
                        <input type="number" step="0.1" min="1.1" max="5.0" id="set-swr" onchange="updateParam('swrThr', this.value)">
                    </div>
                    <div class="set-item">
                        <label>Trip Count</label>
                        <input type="number" step="3" min="3" max="24" id="set-trip" onchange="updateParam('tripCnt', this.value)">
                    </div>
                    <div class="set-item">
                        <label>Low Power Bypass</label>
                        <select id="set-bypass" onchange="updateParam('bypass', this.value)">
                            <option value="1">ON</option>
                            <option value="0">OFF</option>
                        </select>
                    </div>
                    <div class="set-item">
                        <label>Low Thr (W)</label>
                        <input type="number" step="5" min="5" max="50" id="set-lowThr" onchange="updateParam('lowThr', this.value)">
                    </div>
                    <div style="grid-column: span 2; margin-top: 15px; border-top: 1px solid #333; padding-top: 15px;">
                        <button class="update-btn" onclick="openFirmwareUpdate()">FIRMWARE UPDATE</button>
                    </div>
                </div>
                <div class="settings-icon" onclick="toggleSettings()" title="Station Settings">&#9881;</div>
                </div>
            </div>
            <script>
                // INITIAL SETUP & POPUP LOGIC
                const urlParams = new URLSearchParams(window.location.search);
                const isWidget = urlParams.get('window') === 'widget';
                let heartbeatInterval = null;
                let currentSettings = {};

                if (!isWidget) {
                    // THIS IS THE MAIN TAB - FORCE THE POPUP
                    const widgetUrl = window.location.href + (window.location.href.includes('?') ? '&' : '?') + 'window=widget';
                    const widget = window.open(widgetUrl, 'SWR_Widget', 'width=450,height=380,location=no,menubar=no,status=no,resizable=yes');

                    if (widget) {
                        document.body.innerHTML = `
                            <div style="background:#000; color:#0ff; height:100vh; display:flex; flex-direction:column; align-items:center; justify-content:center; font-family:sans-serif; text-align:center; padding:20px;">
                                <h2 style="color:#00FFFF;">METER WIDGET ACTIVE</h2>
                                <p>The dashboard is running in the pop-out window.</p>
                                <button onclick="location.reload()" style="padding:10px 20px; background:#222; color:#fff; border:1px solid #444; cursor:pointer; border-radius:5px;">Restart Launcher</button>
                                <p style="color:#444; margin-top:20px; font-size:0.8em;">You can safely close this tab.</p>
                            </div>`;
                        window.close(); 
                    } else {
                        // If blocked, just run normally in this window
                        startStationMonitor();
                    }
                } else {
                    // THIS IS THE WIDGET - RUN NORMALLY
                    window.onload = startStationMonitor;
                }

                function startStationMonitor() {

                    const ui = document.getElementById('main-ui');
                    if(ui) ui.style.display = 'block';

                    setTimeout(() => syncWindowSize(480), 100);

                    heartbeatInterval = setInterval(() => {
                        fetch('/api')
                            .then(r => r.json())
                            .then(d => processData(d))
                            .catch(e => console.log("API Offline"));
                    }, 500);
                }

                // CORE FUNCTIONS 
                function processData(d) {
                    const fwdVal = parseFloat(d.fwd);
                    const refVal = parseFloat(d.ref);

                    let fwdMax = (fwdVal > 200) ? 500 : (fwdVal > 100) ? 200 : (fwdVal > 10) ? 100 : 10;
                    let refMax = (refVal > 10) ? 50 : (refVal > 2) ? 10 : 2;

                    document.getElementById('fwd-max-label').innerText = fwdMax;
                    document.getElementById('ref-max-label').innerText = refMax;
                    document.getElementById('fwd').innerText = fwdVal.toFixed(1);
                    document.getElementById('ref').innerText = refVal.toFixed(1);
                    document.getElementById('swr').innerText = d.swr;

                    const fwdPct = (fwdVal / fwdMax) * 100;
                    const refPct = (refVal / refMax) * 100;
                    document.getElementById('fwd-bar').style.width = Math.min(fwdPct, 100) + "%";
                    document.getElementById('ref-bar').style.width = Math.min(refPct, 100) + "%";
                    
                    document.getElementById('fwd-peak-line').style.left = Math.min(fwdPct, 99) + "%";
                    document.getElementById('ref-peak-line').style.left = Math.min(refPct, 99) + "%";

                    const isCompactMode = document.body.classList.contains('compact');

                    document.getElementById('swr-card').style.background = (d.fault == 1) ? "#700" : "transparent";

                    syncInput('set-swr', d.swrThr);
                    syncInput('set-trip', d.tripCnt);
                    syncInput('set-bypass', d.bypass);
                    syncInput('set-lowThr', d.lowThr);

                    currentSettings = {...d};
                }

                function syncInput(id, val) {
                    const el = document.getElementById(id);
                    if (!el || document.activeElement === el) return;
                    if (el.value != val) el.value = val;
                }

                function updateParam(key, val) {
                    fetch(`/api?set=${key}&val=${val}`)
                        .then(r => r.json())
                        .then(d => processData(d))
                        .catch(e => console.error("Update failed", e));
                }

                function resetFault() {
                    // Only proceed if a fault actually exists
                    if (currentSettings.fault === 1) {
                        fetch('/api?action=reset')
                            .then(r => {
                                if (r.status === 403) alert("SAFETY LOCK: Stop Transmitting First!");
                                return r.json();
                            })
                            .then(d => processData(d));
                    } else {
                        console.log("No fault detected. Ignoring reset click.");
                    }
                }
                function toggleSettings() {
                    const section = document.getElementById('settings-section');
                    const isOpening = (section.style.display !== "grid");

                    section.style.display = isOpening ? "grid" : "none";

                    // Accessing offsetHeight 'wakes up' the rendering engine
                    const forceReflow = section.offsetHeight; 

                    setTimeout(() => {
                        const isCompact = document.body.classList.contains('compact');
                        const targetW = isCompact ? 280 : 480;
                        syncWindowSize(targetW);
                    }, 50);
                }
                function toggleCompact() {
                    const body = document.body;
                    const section = document.getElementById('settings-section');
                    const btn = document.getElementById('toggle-settings');
                    
                    body.classList.toggle('compact');
                    
                    if (body.classList.contains('compact')) {
                        section.style.display = "none";
                        if (btn) btn.innerText = "+ SETTINGS";
                    }
                    
                    const targetW = body.classList.contains('compact') ? 220 : 480;
                    
                    setTimeout(() => syncWindowSize(targetW), 50);

                    fetch('/api').then(r => r.json()).then(d => processData(d));
                }
                function syncWindowSize(targetWidth) {
                    const chromeH = window.outerHeight - window.innerHeight;
                    const chromeW = window.outerWidth - window.innerWidth;
                    const ui = document.getElementById('main-ui');

                    const isCompact = document.body.classList.contains('compact');

                    const buffer = isCompact ? 5 : 8;
                    const contentHeight = ui.offsetHeight + buffer; 

                    window.resizeTo(targetWidth + chromeW, contentHeight + chromeH);
                }   
                function openFirmwareUpdate() {
                    const baseUrl = window.location.origin;
                    const updateUrl = baseUrl + "/update";
                    window.open(updateUrl, '_blank');
                }          
            </script>
        </body>
        </html>
      )rawdelay";
        server.send(200, "text/html", html);
      });

      // This enables the http://<IP>/update page
      server.on("/update", HTTP_GET, []() {
          String html = R"rawdelay(
          <!DOCTYPE html>
          <html>
          <head>
              <meta name="viewport" content="width=device-width, initial-scale=1">
              <style>
                  body { background: #111; color: #eee; font-family: sans-serif; text-align: center; padding-top: 50px; }
                  .container { max-width: 400px; margin: auto; background: #222; padding: 20px; border-radius: 15px; border: 1px solid #333; }
                  h2 { color: #0f0; letter-spacing: 1px; }
                  input[type=file] { margin: 20px 0; color: #888; display: block; width: 100%; }
                  .btn { background: #0f0; color: #000; border: none; padding: 12px 20px; border-radius: 5px; cursor: pointer; font-weight: bold; width: 100%; }
                  .btn:disabled { background: #444; color: #888; cursor: not-allowed; }
                  #status { color: #666; font-size: 0.8em; margin-top: 15px; }
              </style>
          </head>
          <body>
              <div class="container">
                  <h2>FIRMWARE UPDATE</h2>
                  <form method='POST' action='/update' enctype='multipart/form-data' id='u_f'>
                      <input type='file' name='update' id='file_input' accept='.bin' onchange='checkFile()'>
                      <input type='submit' value='START UPDATE' class='btn' id='submit_btn' disabled>
                  </form>
                  <p id='status'>Please select a .ino.bin file</p>
              </div>
              <script>
                  function checkFile() {
                      var fileInput = document.getElementById('file_input');
                      var submitBtn = document.getElementById('submit_btn');
                      var statusText = document.getElementById('status');
                      
                      // Validate extension
                      if (fileInput.value.toLowerCase().endsWith('.ino.bin')) {
                          submitBtn.disabled = false;
                          statusText.innerText = "File ready. Click Start.";
                          statusText.style.color = "#0f0";
                      } else {
                          submitBtn.disabled = true;
                          statusText.innerText = "Invalid file. Must be .ino.bin";
                          statusText.style.color = "#f00";
                      }
                  }

                  // This is the safe way to disable the button
                  document.getElementById('u_f').onsubmit = function() {
                      document.getElementById('submit_btn').disabled = true;
                      document.getElementById('status').innerText = "Uploading... please wait.";
                      document.getElementById('status').style.color = "#eee";
                  };
              </script>
          </body>
          </html>
          )rawdelay";
          server.send(200, "text/html", html);
      });
      // THIS HANDLES THE ACTUAL DATA UPLOAD
    // THIS HANDLES THE ACTUAL DATA UPLOAD
    static bool updateActualSuccess = false; // Persistent flag for the session

      // THIS HANDLES THE ACTUAL DATA UPLOAD
      server.on("/update", HTTP_POST, []() {
          if (updateActualSuccess) {
              // ACTUAL SUCCESS: Send success page and reboot
              String html = "<html><head><meta http-equiv='refresh' content='10;URL=/update'></head>";
              html += "<body style='background:#111; color:#0f0; font-family:sans-serif; text-align:center; padding-top:50px;'>";
              html += "<h2 style='letter-spacing:2px;'>UPDATE COMPLETE</h2>";
              html += "<div style='background:#222; display:inline-block; padding:20px; border-radius:15px; border:1px solid #333;'>";
              html += "<p style='color:#0f0; font-weight:bold;'>Firmware Written Successfully.</p>";
              html += "<p style='color:#888;'>The meter is rebooting now...</p>";
              html += "</div></body></html>";
              
              server.send(200, "text/html", html);
              delay(1000); 
              ESP.restart();
          } else {
              // ACTUAL FAIL/ABORT: Send error to browser, do NOT reboot
              String html = "<html><body style='background:#111; color:#ff4444; font-family:sans-serif; text-align:center; padding-top:50px;'>";
              html += "<h2>UPDATE ABORTED</h2>";
              html += "<div style='background:#222; display:inline-block; padding:20px; border-radius:15px; border:1px solid #333;'>";
              html += "<p>Hardware authorization was not provided or the write failed.</p>";
              html += "<a href='/update' style='color:#0f0; text-decoration:none;'>[ TRY AGAIN ]</a>";
              html += "</div></body></html>";
              server.send(200, "text/html", html);
          }
       }, []() {
          HTTPUpload& upload = server.upload();
          
          if (upload.status == UPLOAD_FILE_START) {
              updateActualSuccess = false; // Reset flag at start
              isUpdatingOTA = true; 
              screensaverEnabled = false;
              wake_screen();

              tft.fillScreen(TFT_BLUE);
              tft.setTextColor(TFT_WHITE, TFT_BLUE);
              tft.setTextDatum(MC_DATUM);
              tft.setTextSize(3);
              tft.drawString("UPDATE AUTHORIZATION", 240, 100);
              tft.setTextSize(2);
              tft.drawString("PRESS PHYSICAL 'SWR RESET'", 240, 140);
              tft.drawString("BUTTON TO START UPDATE", 240, 160);

              unsigned long startWait = millis();
              bool authorized = false;
              while (millis() - startWait < 30000) { 
                  if (mcp.digitalRead(6) == LOW) { 
                      authorized = true;
                      break;
                  }
                  delay(10);
                  yield(); 
              }

              if (!authorized) {
                  isUpdatingOTA = false; 
                  tft.fillScreen(TFT_BLACK);
                  if (lv_scr_act()) lv_obj_invalidate(lv_scr_act()); 
                  return; 
              }

              tft.fillScreen(TFT_BLUE);
              tft.setTextSize(3);
              tft.drawString("UPDATING...", 240, 140);
              tft.drawRect(38, 198, 404, 24, TFT_WHITE); 

              size_t updateSize = server.header("Content-Length").toInt();
              if (!Update.begin(updateSize > 0 ? updateSize : UPDATE_SIZE_UNKNOWN)) { 
                  Update.printError(Serial);
                  isUpdatingOTA = false;
              }

          } else if (upload.status == UPLOAD_FILE_WRITE) {
              if (Update.isRunning()) {
                  if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                      Update.printError(Serial);
                  }

                  size_t totalExpected = server.header("Content-Length").toInt();
                  if (totalExpected > 0) {
                      int percentage = (upload.totalSize * 100) / totalExpected;
                      static int lastPct = -1;
                      if (percentage != lastPct) {
                          tft.fillRect(40, 200, (percentage * 4), 20, TFT_YELLOW);
                          tft.setTextSize(2);
                          tft.drawString(String(percentage) + "%", 240, 255);
                          lastPct = percentage;
                      }
                  }
              }

          } else if (upload.status == UPLOAD_FILE_END) {
              if (Update.isRunning()) {
                  if (Update.end(true)) {
                      updateActualSuccess = true; // SET SUCCESS FLAG HERE
                      tft.setTextSize(2);
                      tft.drawString("FINISHED! REBOOTING...", 240, 255);
                  } else {
                      Update.printError(Serial);
                      updateActualSuccess = false;
                  }
              }
              isUpdatingOTA = false; 
          }
      });


    // Start the server
    server.begin();
 }

// String getLiveJSON() {
//   String json = "{";
//   json += "\"fwd\":" + String(forwardPower, 1) + ",";
//   json += "\"ref\":" + String(reflectedPower, 1) + ",";
//   json += "\"swr\":" + String(swr, 2) + ",";
//   json += "}";
//   return json;
// }

void drawSettingsPage() {
    if (screen_settings != NULL) return;
    
    screen_settings = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_settings, lv_color_hex(0x000000), 0);
    lv_obj_set_scrollbar_mode(screen_settings, LV_SCROLLBAR_MODE_OFF);

    // --- Header ---
    lv_obj_t * header = lv_label_create(screen_settings);
    lv_label_set_text(header, "SETTINGS");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(header, lv_color_white(), 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t * line = lv_line_create(screen_settings);
    static lv_point_precise_t line_points[] = { {0, 0}, {480, 0} };
    lv_line_set_points(line, line_points, 2);
    lv_obj_set_style_line_width(line, 2, 0);
    lv_obj_set_style_line_color(line, lv_color_hex(0x444444), 0);
    lv_obj_set_pos(line, 0, 42);

    // - SWR Alarm Threshold 
    lv_obj_t * lbl_swr_title = lv_label_create(screen_settings);
    lv_obj_set_style_text_font(lbl_swr_title, &lv_font_montserrat_24, 0); 
    lv_label_set_text(lbl_swr_title, "SWR Threshold:");
    lv_obj_set_style_text_color(lbl_swr_title, lv_color_white(), 0);
    lv_obj_set_pos(lbl_swr_title, 10, 75);

    lbl_swr_threshold_val = lv_label_create(screen_settings);
    lv_obj_set_style_text_font(lbl_swr_threshold_val, &lv_font_montserrat_24, 0); 
    lv_obj_set_style_text_color(lbl_swr_threshold_val, lv_color_hex(0x00FFFF), 0);
    lv_obj_set_pos(lbl_swr_threshold_val, 360, 75); 
    updateSWRAlarmDisplay();

    lv_obj_t * btn_swr_min = lv_button_create(screen_settings);
    lv_obj_set_style_text_font(btn_swr_min, &lv_font_montserrat_24, 0);
    lv_obj_set_size(btn_swr_min, 60, 40);
    lv_obj_set_pos(btn_swr_min, 270, 70);
    lv_obj_t * lbl_swr_min = lv_label_create(btn_swr_min);
    lv_label_set_text(lbl_swr_min, "-");
    lv_obj_center(lbl_swr_min);

    lv_obj_t * btn_swr_plus = lv_button_create(screen_settings);
    lv_obj_set_style_text_font(btn_swr_plus, &lv_font_montserrat_24, 0);
    lv_obj_set_size(btn_swr_plus, 60, 40);
    lv_obj_set_pos(btn_swr_plus, 420, 70);
    lv_obj_t * lbl_swr_plus = lv_label_create(btn_swr_plus);
    lv_label_set_text(lbl_swr_plus, "+");
    lv_obj_center(lbl_swr_plus);


    //  Fault Count (3-15) 
    lv_obj_t * lbl_fault_title = lv_label_create(screen_settings);
    lv_obj_set_style_text_font(lbl_fault_title, &lv_font_montserrat_24, 0); 
    lv_label_set_text(lbl_fault_title, "Fault Trip Count:");
    lv_obj_set_style_text_color(lbl_fault_title, lv_color_white(), 0);
    lv_obj_set_pos(lbl_fault_title, 10, 130);

    lbl_fault_count_val = lv_label_create(screen_settings);
    lv_obj_set_style_text_font(lbl_fault_count_val, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_fault_count_val, lv_color_hex(0x00FFFF), 0);
    lv_obj_set_pos(lbl_fault_count_val, 360, 130);
    updateFaultCountDisplay();

    lv_obj_t * btn_fault_min = lv_button_create(screen_settings);
    lv_obj_set_size(btn_fault_min, 60, 40);
    lv_obj_set_pos(btn_fault_min, 270, 126);
    lv_obj_t * lbl_btn3 = lv_label_create(btn_fault_min);
    lv_label_set_text(lbl_btn3, "-");
    lv_obj_set_style_text_font(lbl_btn3, &lv_font_montserrat_24, 0); 
    lv_obj_center(lbl_btn3);

    lv_obj_t * btn_fault_plus = lv_button_create(screen_settings);
    lv_obj_set_size(btn_fault_plus, 60, 40);
    lv_obj_set_pos(btn_fault_plus, 420, 126);
    lv_obj_t * lbl_btn4 = lv_label_create(btn_fault_plus);
    lv_label_set_text(lbl_btn4, "+");
    lv_obj_set_style_text_font(lbl_btn4, &lv_font_montserrat_24, 0); 
    lv_obj_center(lbl_btn4);

    // Min Power Toggle 
    lv_obj_t * lbl_pwr_chk = lv_label_create(screen_settings);
    lv_obj_set_style_text_font(lbl_pwr_chk, &lv_font_montserrat_24, 0); 
    lv_label_set_text(lbl_pwr_chk, "Low-Pwr Bypass:");
    lv_obj_set_style_text_color(lbl_pwr_chk, lv_color_white(), 0);
    lv_obj_set_pos(lbl_pwr_chk, 10, 210);

    lv_obj_t * sw_min_pwr = lv_switch_create(screen_settings);
    lv_obj_set_pos(sw_min_pwr, 270, 204);
    if(minSWRPowerCheckEnabled) lv_obj_add_state(sw_min_pwr, LV_STATE_CHECKED);

    // Min Power Value (0-50W, 5W steps) 
    lv_obj_t * lbl_min_pwr_val_title = lv_label_create(screen_settings);
    lv_obj_set_style_text_font(lbl_min_pwr_val_title, &lv_font_montserrat_24, 0); 
    lv_label_set_text(lbl_min_pwr_val_title, "Bypass Below (W):");
    lv_obj_set_style_text_color(lbl_min_pwr_val_title, lv_color_white(), 0);
    lv_obj_set_pos(lbl_min_pwr_val_title, 10, 260);

    lbl_min_power_val = lv_label_create(screen_settings);
    lv_obj_set_style_text_font(lbl_min_power_val, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_min_power_val, lv_color_hex(0x00FFFF), 0);
    lv_obj_set_pos(lbl_min_power_val, 360, 260);
    updateMinPowerDisplay(); // Custom helper to update "X Watts"

    lv_obj_t * btn_pwr_min = lv_button_create(screen_settings);
    lv_obj_set_size(btn_pwr_min, 60, 40);
    lv_obj_set_pos(btn_pwr_min, 270, 254);
    lv_obj_t * lbl_btn5 = lv_label_create(btn_pwr_min);
    lv_label_set_text(lbl_btn5, "-");
    lv_obj_set_style_text_font(lbl_btn5, &lv_font_montserrat_24, 0); 
    lv_obj_center(lbl_btn5);

    lv_obj_t * btn_pwr_plus = lv_button_create(screen_settings);
    lv_obj_set_size(btn_pwr_plus, 60, 40);
    lv_obj_set_pos(btn_pwr_plus, 410, 254);
    lv_obj_t * lbl_btn6 = lv_label_create(btn_pwr_plus);
    lv_label_set_text(lbl_btn6, "+");
    lv_obj_set_style_text_font(lbl_btn6, &lv_font_montserrat_24, 0); 
    lv_obj_center(lbl_btn6);

    setupBackButton(screen_settings, PAGE_MENU_ROOT); 

    // --- Event Handlers ---

    lv_obj_add_event_cb(sw_min_pwr, [](lv_event_t * e){
        minSWRPowerCheckEnabled = lv_obj_has_state((lv_obj_t *)lv_event_get_target(e), LV_STATE_CHECKED);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_add_event_cb(btn_pwr_min, [](lv_event_t * e){
        if(minSWRPowerThreshold >= 5) minSWRPowerThreshold -= 5;
        updateMinPowerDisplay();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(btn_pwr_plus, [](lv_event_t * e){
        if(minSWRPowerThreshold <= 95) minSWRPowerThreshold += 5;
        updateMinPowerDisplay();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(btn_fault_min, [](lv_event_t * e){
        if(swrFaultCountLimit >= 6) swrFaultCountLimit -= 3;
        updateFaultCountDisplay();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(btn_fault_plus, [](lv_event_t * e){
        if(swrFaultCountLimit <= 21) swrFaultCountLimit += 3;
        updateFaultCountDisplay();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(btn_swr_min, [](lv_event_t * e){
        swrAlarmThreshold -= 0.1;
        if(swrAlarmThreshold < 1.0) swrAlarmThreshold = 1.0;
        updateSWRAlarmDisplay();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(btn_swr_plus, [](lv_event_t * e){
        swrAlarmThreshold += 0.1;
        if(swrAlarmThreshold > 5.0) swrAlarmThreshold = 5.0;
        updateSWRAlarmDisplay();
    }, LV_EVENT_CLICKED, NULL);
}

void updateMinPowerDisplay() {
    if (lbl_min_power_val != NULL) {
        lv_label_set_text_fmt(lbl_min_power_val, "%d", minSWRPowerThreshold);
    }
}

void updateFaultCountDisplay() {
    if (lbl_fault_count_val != NULL) {
        lv_label_set_text_fmt(lbl_fault_count_val, "%d", swrFaultCountLimit);
    }
}

void drawDisplayPage() {
  if (screen_display != NULL) return;

    lv_obj_t *br_m, *br_p, *tm_m, *tm_p, *sl_m, *sl_p;
    screen_display = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_display, lv_color_hex(0x000000), 0);
    lv_obj_set_scrollbar_mode(screen_display, LV_SCROLLBAR_MODE_OFF);

    // Header: "DISPLAY SETTINGS"
    lv_obj_t * header = lv_label_create(screen_display);
    lv_label_set_text(header, "DISPLAY SETTINGS");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_24, 0); // Bigger header
    lv_obj_set_style_text_color(header, lv_color_white(), 0);      // White
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    // Separator Line
    lv_obj_t * line = lv_line_create(screen_display);
    static lv_point_precise_t line_points[] = { {0, 0}, {480, 0} };
    lv_line_set_points(line, line_points, 2);
    lv_obj_set_style_line_color(line, lv_color_hex(0x444444), 0);
    lv_obj_set_pos(line, 0, 42);

    // --- BRIGHTNESS SECTION ---
    lv_obj_t * lbl_br = lv_label_create(screen_display);
    lv_label_set_text(lbl_br, "Brightness:");
    lv_obj_set_style_text_font(lbl_br, &lv_font_montserrat_18, 0); // Bigger label
    lv_obj_set_style_text_color(lbl_br, lv_color_white(), 0);
    lv_obj_set_pos(lbl_br, 20, 60);

    createPlusMinusGroup(screen_display, 20, 85, &br_m, &br_p);
    label_brightness_val = lv_label_create(screen_display);
    lv_obj_set_style_text_font(label_brightness_val, &lv_font_montserrat_24, 0); // Bigger value
    lv_obj_set_style_text_color(label_brightness_val, lv_color_white(), 0);
    lv_obj_set_pos(label_brightness_val, 100, 95);

    createPlusMinusGroup(screen_display, 20, 90, &br_m, &br_p);
    lv_obj_add_event_cb(br_m, [](lv_event_t * e) {
        backLightLevel = constrain(backLightLevel - 10, 10, 100);
        updateBacklightDisplay(); 
        updateDisplayPageValues();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(br_m, [](lv_event_t * e) {
        backLightLevel = constrain(backLightLevel - 10, 10, 100);
        updateBacklightDisplay(); 
        updateDisplayPageValues();
     }, LV_EVENT_LONG_PRESSED_REPEAT, NULL);


    lv_obj_add_event_cb(br_p, [](lv_event_t * e) {
        backLightLevel = constrain(backLightLevel + 10, 10, 100);
        updateBacklightDisplay();
        updateDisplayPageValues();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(br_p, [](lv_event_t * e) {
        backLightLevel = constrain(backLightLevel + 10, 10, 100);
        updateBacklightDisplay();
        updateDisplayPageValues();
     }, LV_EVENT_LONG_PRESSED_REPEAT, NULL);


    // --- SCREENSAVER TOGGLE ---
    lv_obj_t * lbl_ss = lv_label_create(screen_display);
    lv_label_set_text(lbl_ss, "Screensaver:");
    lv_obj_set_style_text_font(lbl_ss, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_ss, lv_color_white(), 0);
    lv_obj_set_pos(lbl_ss, 280, 60);

    btn_screensaver = lv_button_create(screen_display);
    lv_obj_set_size(btn_screensaver, 180, 45);
    lv_obj_set_pos(btn_screensaver, 280, 85);
    lv_obj_set_style_bg_color(btn_screensaver, lv_color_hex(0x333333), 0); // Darker button
    lv_obj_t * lbl_ss_btn = lv_label_create(btn_screensaver);
    lv_obj_set_style_text_font(lbl_ss_btn, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl_ss_btn);

    lv_obj_add_event_cb(btn_screensaver, [](lv_event_t * e) {
        screensaverEnabled = !screensaverEnabled;
        updateDisplayPageValues();
    }, LV_EVENT_CLICKED, NULL);

    // --- MENU TIMEOUT SECTION ---
    lv_obj_t * lbl_mt = lv_label_create(screen_display);
    lv_label_set_text(lbl_mt, "Menu Exit (s):");
    lv_obj_set_style_text_font(lbl_mt, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_mt, lv_color_white(), 0);
    lv_obj_set_pos(lbl_mt, 20, 150);

    createPlusMinusGroup(screen_display, 20, 175, &tm_m, &tm_p);
    label_timeout_val = lv_label_create(screen_display);
    lv_obj_set_style_text_font(label_timeout_val, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label_timeout_val, lv_color_white(), 0);
    lv_obj_set_pos(label_timeout_val, 110, 185);

    createPlusMinusGroup(screen_display, 20, 180, &tm_m, &tm_p);
    lv_obj_add_event_cb(tm_m, [](lv_event_t * e) {
        if(menuTimeout > 5) menuTimeout -= 5;
        updateDisplayPageValues();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(tm_p, [](lv_event_t * e) {
        menuTimeout += 5;
        updateDisplayPageValues();
    }, LV_EVENT_CLICKED, NULL);

    // --- SLEEP SECTION ---
    lv_obj_t * lbl_sl = lv_label_create(screen_display);
    lv_label_set_text(lbl_sl, "Sleep (min):");
    lv_obj_set_style_text_font(lbl_sl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_sl, lv_color_white(), 0);
    lv_obj_set_pos(lbl_sl, 280, 150);

    createPlusMinusGroup(screen_display, 280, 175, &sl_m, &sl_p);
    label_sleep_val = lv_label_create(screen_display);
    lv_obj_set_style_text_font(label_sleep_val, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label_sleep_val, lv_color_white(), 0);
    lv_obj_set_pos(label_sleep_val, 375, 185);

    // Add Logic for Minus Button
    lv_obj_add_event_cb(sl_m, [](lv_event_t * e) {
        if(screensaverMinutes > 1) { // Don't go below 1 minute
            screensaverMinutes--;
        }
        updateDisplayPageValues(); 
    }, LV_EVENT_CLICKED, NULL);

    // Hold down: -5
    lv_obj_add_event_cb(sl_m, [](lv_event_t * e) {
    if(screensaverMinutes > 5) screensaverMinutes -= 5;
    else screensaverMinutes = 1;
    updateDisplayPageValues();
     }, LV_EVENT_LONG_PRESSED_REPEAT, NULL);

    // Add Logic for Plus Button
    lv_obj_add_event_cb(sl_p, [](lv_event_t * e) {
        if(screensaverMinutes < 60) { // Limit to 1 hour
            screensaverMinutes++;
        }
        updateDisplayPageValues(); 
    }, LV_EVENT_CLICKED, NULL);

    // Hold down: +5
    lv_obj_add_event_cb(sl_p, [](lv_event_t * e) {
        if(screensaverMinutes <= 55) screensaverMinutes += 5;
        else screensaverMinutes = 60;
        updateDisplayPageValues();
    }, LV_EVENT_LONG_PRESSED_REPEAT, NULL);

    // --- VOLTAGE MODE ---
    lv_obj_t * lbl_vm = lv_label_create(screen_display);
    lv_label_set_text(lbl_vm, "Voltage Mode:");
    lv_obj_set_style_text_font(lbl_vm, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_vm, lv_color_white(), 0);
    lv_obj_set_pos(lbl_vm, 20, 240);

    btn_voltage_mode = lv_button_create(screen_display);
    lv_obj_set_size(btn_voltage_mode, 180, 45);
    lv_obj_set_pos(btn_voltage_mode, 20, 265);
    lv_obj_set_style_bg_color(btn_voltage_mode, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_t * lbl_vm_btn = lv_label_create(btn_voltage_mode);
    lv_obj_set_style_text_font(lbl_vm_btn, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl_vm_btn);

    lv_obj_add_event_cb(btn_voltage_mode, [](lv_event_t * e) {
        voltageFormatMode = (voltageFormatMode + 1) % 3; // Cycle through 3 modes
        saveSettings();
        updateDisplayPageValues();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_mf = lv_label_create(screen_display);
    lv_label_set_text(lbl_mf, "Meter Face:");
    lv_obj_set_style_text_font(lbl_mf, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_mf, lv_color_white(), 0);
    lv_obj_set_pos(lbl_mf, 280, 240);

    btn_meter_style = lv_button_create(screen_display);
    lv_obj_set_size(btn_meter_style, 180, 45);
    lv_obj_set_pos(btn_meter_style, 280, 265);
    lv_obj_set_style_bg_color(btn_meter_style, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_t * lbl_mf_btn = lv_label_create(btn_meter_style);
    lv_obj_set_style_text_font(lbl_mf_btn, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl_mf_btn);

    lv_obj_add_event_cb(btn_meter_style, [](lv_event_t * e) {
        useCrossNeedle = !useCrossNeedle;
        saveSettings();
        updateDisplayPageValues();
    }, LV_EVENT_CLICKED, NULL);

    updateDisplayPageValues();

    setupBackButton(screen_display, PAGE_MENU_ROOT);
}

void createPlusMinusGroup(lv_obj_t * parent, int x, int y, lv_obj_t ** out_minus, lv_obj_t ** out_plus) {
    // Minus Button
    *out_minus = lv_button_create(parent);
    lv_obj_set_size(*out_minus, 60, 45);
    lv_obj_set_pos(*out_minus, x, y);
    lv_obj_set_style_bg_color(*out_minus, lv_color_hex(0x333333), 0);
    lv_obj_t * lbl_m = lv_label_create(*out_minus);
    lv_label_set_text(lbl_m, "-");
    lv_obj_center(lbl_m);

    // Plus Button
    *out_plus = lv_button_create(parent);
    lv_obj_set_size(*out_plus, 60, 45);
    lv_obj_set_pos(*out_plus, x + 140, y);
    lv_obj_set_style_bg_color(*out_plus, lv_color_hex(0x333333), 0);
    lv_obj_t * lbl_p = lv_label_create(*out_plus);
    lv_label_set_text(lbl_p, "+");
    lv_obj_center(lbl_p);
}

void updateDisplayPageValues() {
    if (label_brightness_val) lv_label_set_text_fmt(label_brightness_val, "%d%%", backLightLevel);
    if (label_timeout_val) lv_label_set_text_fmt(label_timeout_val, "%d", menuTimeout);
    if (label_sleep_val) lv_label_set_text_fmt(label_sleep_val, "%d", screensaverMinutes);

    // Update Voltage Mode Button Text
    lv_obj_t * lbl_v = lv_obj_get_child(btn_voltage_mode, 0);
    lv_label_set_text(lbl_v, voltageFormatLabels[voltageFormatMode]);

    if (btn_meter_style != NULL) {
        lv_obj_t * lbl_m = lv_obj_get_child(btn_meter_style, 0);
        lv_label_set_text(lbl_m, useCrossNeedle ? "NEEDLES" : "BARS");
    }

    // Update Screensaver Button
    lv_obj_t * lbl_ss = lv_obj_get_child(btn_screensaver, 0);
    if(screensaverEnabled) {
        lv_obj_set_style_bg_color(btn_screensaver, lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_label_set_text(lbl_ss, "ENABLED");
    } else {
        lv_obj_set_style_bg_color(btn_screensaver, lv_palette_main(LV_PALETTE_RED), 0);
        lv_label_set_text(lbl_ss, "DISABLED");
    }
}


void updateBacklightDisplay() {

    //ledcWrite(0, backLightLevel); 
    analogWrite(TFT_BL, backLightLevel);

    if (label_brightness_val != NULL) {
        int percent = map(backLightLevel, 0, 255, 0, 100);
        lv_label_set_text_fmt(label_brightness_val, "%d%%", percent);
    }
}


void drawNetworkPage() {
    if (screen_network != NULL) return;
    
    screen_network = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_network, lv_color_hex(0x000000), 0);
    lv_obj_set_scrollbar_mode(screen_network, LV_SCROLLBAR_MODE_OFF);

    // --- Header ---
    lv_obj_t * header = lv_label_create(screen_network);
    lv_label_set_text(header, "NETWORK SETTINGS");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_24, 0); // Bigger
    lv_obj_set_style_text_color(header, lv_color_white(), 0);      // White
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t * line = lv_line_create(screen_network);
    static lv_point_precise_t line_points[] = { {0, 0}, {480, 0} };
    lv_line_set_points(line, line_points, 2);
    lv_obj_set_style_line_color(line, lv_color_hex(0x444444), 0);
    lv_obj_set_pos(line, 0, 42);

    // ---WiFi Power ---
    lv_obj_t * l1 = lv_label_create(screen_network);
    lv_label_set_text(l1, "WiFi Power:");
    lv_obj_set_style_text_font(l1, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(l1, lv_color_white(), 0);
    lv_obj_set_pos(l1, 20, 55);

    btn_wifi_pwr = lv_button_create(screen_network);
    lv_obj_set_size(btn_wifi_pwr, 120, 35);
    lv_obj_set_pos(btn_wifi_pwr, 200, 48);
    lv_obj_t * lbl_wifi_pwr = lv_label_create(btn_wifi_pwr);
    lv_obj_set_style_text_font(lbl_wifi_pwr, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl_wifi_pwr);

    // --- SSID & Scan ---
    lv_obj_t * l2 = lv_label_create(screen_network);
    lv_label_set_text(l2, "SSID:");
    lv_obj_set_style_text_font(l2, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(l2, lv_color_white(), 0);
    lv_obj_set_pos(l2, 20, 100);

    lbl_net_ssid = lv_label_create(screen_network);
    lv_obj_set_style_text_font(lbl_net_ssid, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_net_ssid, lv_color_white(), 0);
    lv_obj_set_pos(lbl_net_ssid, 100, 100);
    lv_label_set_text(lbl_net_ssid, ssid.c_str());

    lv_obj_t * btn_scan = lv_button_create(screen_network);
    lv_obj_set_size(btn_scan, 120, 35);
    lv_obj_set_pos(btn_scan, 340, 93);
    lv_obj_set_style_bg_color(btn_scan, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_t * lbl_scan = lv_label_create(btn_scan);
    lv_label_set_text(lbl_scan, "SCAN");
    lv_obj_set_style_text_font(lbl_scan, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl_scan);

    // ---  Web Server ---
    lv_obj_t * l3 = lv_label_create(screen_network);
    lv_label_set_text(l3, "Web Server:");
    lv_obj_set_style_text_font(l3, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(l3, lv_color_white(), 0);
    lv_obj_set_pos(l3, 20, 145);

    btn_web_server = lv_button_create(screen_network);
    lv_obj_set_size(btn_web_server, 120, 35);
    lv_obj_set_pos(btn_web_server, 200, 138);
    lv_obj_t * lbl_web_btn = lv_label_create(btn_web_server);
    lv_obj_set_style_text_font(lbl_web_btn, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl_web_btn);
    lv_label_set_text(lbl_web_btn, webServerEnabled ? "ON" : "OFF");

    // -IP Mode & Edit ---
    // Button to choose DHCP or Static
    lv_obj_t * l4 = lv_label_create(screen_network);
    lv_label_set_text(l4, "IP Mode:");
    lv_obj_set_style_text_font(l4, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(l4, lv_color_white(), 0);
    lv_obj_set_pos(l4, 20, 190);

    btn_ip_mode = lv_button_create(screen_network);
    lv_obj_set_size(btn_ip_mode, 120, 35);
    lv_obj_set_pos(btn_ip_mode, 200, 183);
    lv_obj_t * lbl_ip_mode = lv_label_create(btn_ip_mode);
    lv_obj_set_style_text_font(lbl_ip_mode, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl_ip_mode);

    lv_obj_add_event_cb(btn_ip_mode, [](lv_event_t * e) {
        useStaticIP = !useStaticIP; 
        lv_obj_t * btn = (lv_obj_t *)lv_event_get_target(e);
        lv_obj_t * lbl = lv_obj_get_child(btn, 0);
        lv_label_set_text(lbl, useStaticIP ? "Static" : "DHCP");
    }, LV_EVENT_CLICKED, NULL);

    // edit static IP, subnet mask, gateway, dns
    lv_obj_t * btn_edit_ip = lv_button_create(screen_network);
    lv_obj_set_size(btn_edit_ip, 120, 35);
    lv_obj_set_pos(btn_edit_ip, 340, 183);
    lv_obj_set_style_bg_color(btn_edit_ip, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_t * lbl_edit = lv_label_create(btn_edit_ip);
    lv_label_set_text(lbl_edit, "EDIT IP");
    lv_obj_set_style_text_font(lbl_edit, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl_edit);
    lv_obj_add_event_cb(btn_edit_ip, [](lv_event_t * e) {
        drawStaticIPPage();
        currentPage = PAGE_IP_CONFIG; 
    }, LV_EVENT_CLICKED, NULL);

    // ---  UDP Unicast & Settings  ---
    lv_obj_t * l5 = lv_label_create(screen_network);
    lv_label_set_text(l5, "UDP Unicast:");
    lv_obj_set_style_text_font(l5, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(l5, lv_color_white(), 0);
    lv_obj_set_pos(l5, 20, 235);

    btn_udp_toggle = lv_button_create(screen_network); 
    lv_obj_set_size(btn_udp_toggle, 120, 35);
    lv_obj_set_pos(btn_udp_toggle, 200, 228);
    lv_obj_t * lbl_udp_btn = lv_label_create(btn_udp_toggle);
    lv_obj_set_style_text_font(lbl_udp_btn, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl_udp_btn);
    lv_label_set_text(lbl_udp_btn, udpEnabled ? "ON" : "OFF");

    lv_obj_t * btn_udp_settings = lv_button_create(screen_network);
    lv_obj_set_size(btn_udp_settings, 120, 35);
    lv_obj_set_pos(btn_udp_settings, 340, 228);
    lv_obj_set_style_bg_color(btn_udp_settings, lv_color_hex(0x800000),0); // Maroon
    lv_obj_t * lbl_udp_set = lv_label_create(btn_udp_settings);
    lv_label_set_text(lbl_udp_set, "SETTINGS");
    lv_obj_set_style_text_font(lbl_udp_set, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl_udp_set);

    // --- Footer Status Info ---

    lbl_net_ip = lv_label_create(screen_network);
    lv_obj_set_style_text_font(lbl_net_ip, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_net_ip, lv_color_white(), 0);
    lv_obj_set_pos(lbl_net_ip, 20, 275);
    
    lbl_net_sub = lv_label_create(screen_network);
    lv_obj_set_style_text_font(lbl_net_sub, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_net_sub, lv_color_white(), 0);
    lv_obj_set_pos(lbl_net_sub, 220, 275); // Moved right to clear IP
    
    lbl_net_gw = lv_label_create(screen_network); 
    lv_obj_set_style_text_font(lbl_net_gw, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_net_gw, lv_color_white(), 0);
    lv_obj_set_pos(lbl_net_gw, 20, 295);

    lbl_net_dns = lv_label_create(screen_network); 
    lv_obj_set_style_text_font(lbl_net_dns, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_net_dns, lv_color_white(), 0);
    lv_obj_set_pos(lbl_net_dns, 220, 295);

    // RSSI 
    lbl_net_rssi = lv_label_create(screen_network);
    lv_obj_set_style_text_font(lbl_net_rssi, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_net_rssi, lv_color_white(), 0);
    lv_obj_set_pos(lbl_net_rssi, 390, 275);

    // --- Initial Load ---
    updateNetworkPageValues();
    setupBackButton(screen_network, PAGE_MENU_ROOT);

    // --- Event Callbacks ---
    lv_obj_add_event_cb(btn_wifi_pwr, [](lv_event_t * e) {
        wifiEnabled = !wifiEnabled;
        if(wifiEnabled) {
            setupWiFi(); 
        } else { 
            WiFi.disconnect(true);
            wasConnected = false;
            isAttemptingConnection = false;
        }
        updateNetworkPageValues();
    }, LV_EVENT_CLICKED, NULL);

    // Web Server
    lv_obj_add_event_cb(btn_web_server, [](lv_event_t * e) {
        webServerEnabled = !webServerEnabled;
        if (webServerEnabled) {
            if (WiFi.status() == WL_CONNECTED) {
                setupWebServerRoutes();
                const char * headerkeys[] = {"Content-Length"};
                server.collectHeaders(headerkeys, 1);
                server.begin();
                Serial.println("Web Server Started Manually (WiFi was already up)");
            }
        } else {
            server.stop();
            Serial.println("Web Server Stopped");
        }
        updateNetworkPageValues();
    }, LV_EVENT_CLICKED, NULL);

    // UDP
    lv_obj_add_event_cb(btn_udp_toggle, [](lv_event_t * e) {
        udpEnabled = !udpEnabled;
        updateNetworkPageValues();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(btn_scan, [](lv_event_t * e) {
      lv_obj_t * btn = (lv_obj_t *)lv_event_get_target(e);
      lv_obj_t * label = lv_obj_get_child(btn, 0);
      lv_label_set_text(label, "Scanning...");
      lv_refr_now(NULL); 
      lv_obj_t * old_screen = lv_screen_active();
      screen_network = NULL;
      drawWiFiScannerPage(); 
      currentPage = PAGE_WIFI_SCAN;
      if(old_screen) lv_obj_del(old_screen);
    }, LV_EVENT_CLICKED, NULL);


    lv_obj_add_event_cb(btn_udp_settings, [](lv_event_t * e) {
        createUDPConfigScreen(); // Build the new screen
        lv_screen_load(screen_udp_config);
        currentPage = PAGE_UDP_CONFIG;
        lv_obj_del_async(screen_network); 
        screen_network = NULL; // 
    }, LV_EVENT_CLICKED, NULL);
 
        // Add logic for btn_scan, btn_edit_ip, and btn_udp_settings as needed
}

void updateNetworkPageValues() {
    if (screen_network == NULL || !lv_obj_is_valid(screen_network)) return;

    // WiFi Power Toggle Style
    lv_obj_t * l_pwr = lv_obj_get_child(btn_wifi_pwr, 0);
    lv_label_set_text(l_pwr, wifiEnabled ? "ON" : "OFF");
    lv_obj_set_style_bg_color(btn_wifi_pwr, wifiEnabled ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_GREY), 0);

    // Web Server Button Style
    lv_obj_t * l_web = lv_obj_get_child(btn_web_server, 0);
    lv_label_set_text(l_web, webServerEnabled ? "ON" : "OFF");
    lv_obj_set_style_bg_color(btn_web_server, webServerEnabled ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_RED), 0);

    // UDP Button Style
    lv_obj_t * l_udp = lv_obj_get_child(btn_udp_toggle, 0);
    lv_label_set_text(l_udp, udpEnabled ? "ON" : "OFF");
    lv_obj_set_style_bg_color(btn_udp_toggle, udpEnabled ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_RED), 0);

    // SSID Status
    if (WiFi.status() == WL_CONNECTED) {
        lv_label_set_text_fmt(lbl_net_ip, "IP: %s", WiFi.localIP().toString().c_str());
        lv_label_set_text_fmt(lbl_net_sub, "SUB: %s", WiFi.subnetMask().toString().c_str());
        lv_label_set_text_fmt(lbl_net_gw, "GW: %s", WiFi.gatewayIP().toString().c_str());
        lv_label_set_text_fmt(lbl_net_dns, "DNS: %s", WiFi.dnsIP().toString().c_str());
        lv_label_set_text_fmt(lbl_net_rssi, "%d dBm", WiFi.RSSI());
    } else {
        lv_label_set_text(lbl_net_ip, "DISCONNECTED");
        lv_label_set_text(lbl_net_sub, "");
        lv_label_set_text(lbl_net_gw, "");
        lv_label_set_text(lbl_net_dns, "");
        lv_label_set_text(lbl_net_rssi, "");
    }

    // IP Mode Style
    lv_obj_t * l_mode = lv_obj_get_child(btn_ip_mode, 0);
    lv_label_set_text(l_mode, useStaticIP ? "STATIC" : "DHCP");
    lv_obj_set_style_bg_color(btn_ip_mode, useStaticIP ? lv_palette_main(LV_PALETTE_GREY) : lv_palette_main(LV_PALETTE_GREEN), 0);
}


void drawWiFiScannerPage() {
    //  Pre-cleanup: If the scanner screen already exists, kill it now
    if (screen_wifi_scan != NULL) {
        lv_obj_del(screen_wifi_scan);
        screen_wifi_scan = NULL;
    }
    // Shut down background noise
    server.stop();
    webServerEnabled=false;
    udpEnabled=false;
    wifiEnabled = false;
    WiFi.disconnect(true);
    delay(100); 

    // Build the UI
    screen_wifi_scan = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_wifi_scan, lv_color_hex(0x000000), 0);

    lv_obj_t * label = lv_label_create(screen_wifi_scan);
    lv_label_set_text(label, "SELECT NETWORK");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t * list = lv_list_create(screen_wifi_scan);
    lv_obj_set_size(list, 310, 200);
    lv_obj_center(list);
    lv_obj_set_style_text_font(list, &lv_font_montserrat_18, 0);

    //  Force Display Refresh
    lv_screen_load(screen_wifi_scan);
    lv_timer_handler(); 
    yield();

    // Safe Scan
    WiFi.mode(WIFI_STA);
    int n = WiFi.scanNetworks();

    if (n <= 0) {
        lv_list_add_text(list, "No networks found");
    } else {
        // Use a fixed-size array to avoid stack overflow (limited to top 10)
        int count = (n > 10) ? 10 : n;
        int indices[10]; 
        for (int i = 0; i < count; i++) indices[i] = i;

        // Sort by RSSI
        for (int i = 0; i < count - 1; i++) {
            for (int j = i + 1; j < count; j++) {
                if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
                    int temp = indices[i];
                    indices[i] = indices[j];
                    indices[j] = temp;
                }
            }
        }

        for (int i = 0; i < count; i++) {
            int idx = indices[i];
            String currentSSID = WiFi.SSID(idx);
            lv_obj_t * btn = lv_list_add_button(list, LV_SYMBOL_WIFI, currentSSID.c_str());
            
            lv_obj_add_event_cb(btn, [](lv_event_t * e) {
                lv_obj_t * target_btn = (lv_obj_t *)lv_event_get_target(e);
                // We pass NULL for the list to get the text of the button clicked
                ssid = lv_list_get_button_text(NULL, target_btn); 
                drawKeyboardPage("Enter Password", &password);
            }, LV_EVENT_CLICKED, NULL);
        }
    }
    // Manual Entry & Back Buttons
    lv_obj_t * btn_manual = lv_button_create(screen_wifi_scan);
    lv_obj_set_size(btn_manual, 120, 40);
    lv_obj_align(btn_manual, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_t * lbl_manual = lv_label_create(btn_manual);
    lv_label_set_text(lbl_manual, "MANUAL");
    lv_obj_center(lbl_manual);

    lv_obj_add_event_cb(btn_manual, [](lv_event_t * e) {
        drawKeyboardPage("Enter SSID", &ssid, 1);
    }, LV_EVENT_CLICKED, NULL);
    setupBackButton(screen_wifi_scan, PAGE_NETWORK);
    currentPage = PAGE_WIFI_SCAN;
    //  IMMEDIATE Cleanup (Prevents the background task race condition)
    if (screen_network != NULL) {
        lv_obj_del(screen_network); 
        screen_network = NULL; 
    }
}

void drawKeyboardPage(const char* title, String* targetVar, int step) {
    keyboardStep = step;

    screen_keyboard = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_keyboard, lv_color_hex(0x000000), 0);

    // UI Elements (Title and Text Area)
    lv_obj_t * lbl_title = lv_label_create(screen_keyboard);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(lbl_title, title);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t * ta = lv_textarea_create(screen_keyboard);
    lv_obj_set_style_text_font(ta, &lv_font_montserrat_18, 0);
    lv_textarea_set_text(ta, targetVar->c_str());
    lv_obj_set_size(ta, 240, 40); // Made slightly narrower to not overlap back button
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 45);
    lv_textarea_set_one_line(ta, true);
    //if (targetVar == &password) lv_textarea_set_password_mode(ta, true);   // hide password (caused issues so remarked out)

     // Create Keyboard
    lv_obj_t * kb = lv_keyboard_create(screen_keyboard);
    lv_keyboard_set_textarea(kb, ta);

    // Handle Keyboard OK/Cancel Events
    lv_obj_add_event_cb(kb, [](lv_event_t * e) {
        lv_event_code_t code = lv_event_get_code(e);
        if(code == LV_EVENT_READY) {
            lv_obj_t * kb_ptr = (lv_obj_t *)lv_event_get_target(e);
            lv_obj_t * ta_ptr = lv_keyboard_get_textarea(kb_ptr);
            
            // Save data based on step
            if (keyboardStep == 1) ssid = lv_textarea_get_text(ta_ptr);
            else password = lv_textarea_get_text(ta_ptr);

            if (keyboardStep == 1) {
                // Sequence: Move to password
                lv_obj_t * old_kb_screen = screen_keyboard; 
                drawKeyboardPage("ENTER PASSWORD", &password, 2); 
                if(old_kb_screen) lv_obj_del_async(old_kb_screen);
            } else {
                saveSettings();
                drawNetworkPage(); 
                lv_screen_load(screen_network);
            }
        }
        if(code == LV_EVENT_CANCEL) {
            drawNetworkPage();
            lv_screen_load(screen_network);
        }
    }, LV_EVENT_ALL, NULL);

    setupBackButton(screen_keyboard, PAGE_NETWORK);

    lv_obj_t * screen_to_kill = lv_screen_active(); 
    lv_screen_load(screen_keyboard);
    currentPage = PAGE_KEYBOARD;

    if (screen_to_kill != NULL) {
        if (screen_to_kill == screen_network) screen_network = NULL;
        if (screen_to_kill == screen_wifi_scan) screen_wifi_scan = NULL;
        lv_obj_del_async(screen_to_kill);
    }
}

void drawSavedSettingsPage() {
    if (screen_saved_settings != NULL){
      return;
    }
    screen_saved_settings = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_saved_settings, lv_color_hex(0x000000), 0);

    // Header
    lv_obj_t * header = lv_label_create(screen_saved_settings);
    lv_label_set_text(header, "SAVED SETTINGS");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(header, lv_color_white(), 0); 
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t * line = lv_line_create(screen_saved_settings);
    static lv_point_precise_t line_points[] = { {0, 0}, {480, 0} };
    lv_line_set_points(line, line_points, 2);
    lv_obj_set_style_line_color(line, lv_color_hex(0x444444), 0);
    lv_obj_set_pos(line, 0, 42);

    // Grid Settings
    int itemsPerColumn = 11;
    int rowHeight = 25; 
    int yStart = 56;
    int colWidth = 240;

    for (int i = 0; i < SETTINGS_COUNT; i++) {
        int col = i / itemsPerColumn;
        int row = i % itemsPerColumn;
        int xPos = 10 + (col * colWidth);
        int yPos = yStart + (row * rowHeight);

        // Create the Label (Key)
        lv_obj_t * lbl_key = lv_label_create(screen_saved_settings);
        lv_label_set_text_fmt(lbl_key, "%s:", mySettings[i].label.c_str());
        lv_obj_set_style_text_color(lbl_key, lv_palette_main(LV_PALETTE_CYAN), 0);
        lv_obj_set_style_text_font(lbl_key, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(lbl_key, xPos, yPos);

        // Create the Value Label
        lv_obj_t * lbl_val = lv_label_create(screen_saved_settings);
        lv_obj_set_pos(lbl_val, xPos + 110, yPos);
        lv_obj_set_style_text_color(lbl_val, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl_val, &lv_font_montserrat_14, 0);

        // Populate the value based on type
        if (mySettings[i].floatVal != NULL) {
          char buf[16];
          snprintf(buf, sizeof(buf), "%.2f", *(mySettings[i].floatVal)); 
          lv_label_set_text(lbl_val, buf);
        } 
        else if (mySettings[i].intVal != NULL) {
            lv_label_set_text_fmt(lbl_val, "%d", *(mySettings[i].intVal));
        }
        else if (mySettings[i].boolVal != NULL) {
            lv_label_set_text(lbl_val, *(mySettings[i].boolVal) ? "ON" : "OFF");
        }
        else if (mySettings[i].byteVal != NULL) {
            lv_label_set_text_fmt(lbl_val, "%u", *(mySettings[i].byteVal));
        }
        else if (mySettings[i].stringVal != NULL) {
            // Only try to read a String if it's definitely not a number
            lv_label_set_text(lbl_val, mySettings[i].stringVal->c_str());
        } 
        else if (mySettings[i].ipVal != NULL) {
            lv_label_set_text(lbl_val, mySettings[i].ipVal->toString().c_str());
        }
    }

    // Back Button
    setupBackButton(screen_saved_settings, PAGE_MENU_ROOT);
}

void drawSystemInfo() {
  if (screen_system_info != NULL){
      return;
    }
    screen_system_info = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_system_info, lv_color_hex(0x000000), 0);

    // Header: "SYSTEM INFORMATION"
    lv_obj_t * header = lv_label_create(screen_system_info);
    lv_label_set_text(header, "SYSTEM INFORMATION");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(header, lv_color_white(), 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    // Header Line
    lv_obj_t * line1 = lv_line_create(screen_system_info);
    static lv_point_precise_t line_pts[] = { {20, 42}, {460, 42} };
    lv_line_set_points(line1, line_pts, 2);
    lv_obj_set_style_line_color(line1, lv_palette_main(LV_PALETTE_GREY), 0);

    int y = 60;
    int col2 = 160;

    // Helper lambda to create a row (Label: Value)
    auto add_row = [](lv_obj_t * parent, const char * key, const char * val, int y_pos, int val_x, lv_color_t val_color) {
        lv_obj_t * k = lv_label_create(parent);
        lv_label_set_text(k, key);
        lv_obj_set_style_text_color(k, lv_palette_main(LV_PALETTE_CYAN), 0);
        lv_obj_set_pos(k, 20, y_pos);

        lv_obj_t * v = lv_label_create(parent);
        lv_label_set_text(v, val);
        lv_obj_set_style_text_color(v, val_color, 0);
        lv_obj_set_pos(v, val_x, y_pos);
    };

    // Software Info
    add_row(screen_system_info, "VERSION:", codeVersion.c_str(), y, col2, lv_color_hex(0xFFFFFF));
    y += 25;
    add_row(screen_system_info, "COMPILED:", __DATE__, y, col2, lv_color_hex(0xFFFFFF));

    // Hardware Section Divider
    y += 35;
    lv_obj_t * line2 = lv_line_create(screen_system_info);
    static lv_point_precise_t line_pts2[2]; 
    line_pts2[0] = {20, (lv_value_precise_t)(y-10)}; line_pts2[1] = {460, (lv_value_precise_t)(y-10)};
    lv_line_set_points(line2, line_pts2, 2);
    lv_obj_set_style_line_color(line2, lv_palette_main(LV_PALETTE_GREY), 0);

    // MAC Address
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    add_row(screen_system_info, "MAC ADDR:", macStr, y, col2, lv_palette_main(LV_PALETTE_YELLOW));

    y += 25;
    add_row(screen_system_info, "CHIP REV:", String(ESP.getChipRevision()).c_str(), y, col2, lv_color_hex(0xFFFFFF));

    // Health Section Divider
    y += 35;
    lv_obj_t * line3 = lv_line_create(screen_system_info);
    static lv_point_precise_t line_pts3[2];
    line_pts3[0] = {20, (lv_value_precise_t)(y-10)}; line_pts3[1] = {460, (lv_value_precise_t)(y-10)};
    lv_line_set_points(line3, line_pts3, 2);
    lv_obj_set_style_line_color(line3, lv_palette_main(LV_PALETTE_GREY), 0);

    // Heap and Uptime
    char heapBuf[16];
    sprintf(heapBuf, "%d KB", ESP.getFreeHeap() / 1024);
    add_row(screen_system_info, "HEAP FREE:", heapBuf, y, col2, lv_palette_main(LV_PALETTE_GREEN));

    y += 25;
    char uptimeBuf[16];
    sprintf(uptimeBuf, "%luh %lum", millis()/3600000, (millis()%3600000)/60000);
    add_row(screen_system_info, "UPTIME:", uptimeBuf, y, col2, lv_color_hex(0xFFFFFF));

    // Update Firmware Button 
    lv_obj_t * btn_update = lv_btn_create(screen_system_info);
    lv_obj_set_size(btn_update, 180, 50);
    lv_obj_align(btn_update, LV_ALIGN_BOTTOM_RIGHT, -20, -20);

    lv_obj_set_style_bg_color(btn_update, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_radius(btn_update, 8, 0);

    lv_obj_t * btn_label = lv_label_create(btn_update);
    lv_label_set_text(btn_label, "Update Firmware");
    lv_obj_center(btn_label);

    // Button Event
    lv_obj_add_event_cb(btn_update, [](lv_event_t * e) {

        // Delete this screen from memory
        if (screen_system_info) {
            lv_obj_del(screen_system_info);
            screen_system_info = NULL;
        }

        currentPage = PAGE_FIRMWARE;
        drawFirmwareUpdate();   // This will create screen_update_firmware
        lv_scr_load(screen_update_firmware);

    }, LV_EVENT_CLICKED, NULL);

    // Back Button
    setupBackButton(screen_system_info, PAGE_MENU_ROOT);
}


void createUDPConfigScreen() {
    screen_udp_config = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_udp_config, lv_color_hex(0x000000), 0);

    // Header
    lv_obj_t * header = lv_label_create(screen_udp_config);
    lv_obj_set_style_text_font(header, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(header, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(header, "UDP UNICAST SETTINGS");
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    // Header Line
    lv_obj_t * line1 = lv_line_create(screen_udp_config);
    static lv_point_precise_t line_pts[] = { {20, 42}, {460, 42} };
    lv_line_set_points(line1, line_pts, 2);
    lv_obj_set_style_line_color(line1, lv_palette_main(LV_PALETTE_GREY), 0);

    // Create the Number Pad (Keyboard)
    lv_obj_t * kb = lv_keyboard_create(screen_udp_config);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_set_size(kb, 480, 160); // Half the screen
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);

    // IP Octet Text Areas
    for (int i = 0; i < 4; i++) {
        ta_ip[i] = lv_textarea_create(screen_udp_config);
        lv_obj_set_style_text_font(ta_ip[i], &lv_font_montserrat_18, 0);
        lv_textarea_set_one_line(ta_ip[i], true);
        lv_textarea_set_max_length(ta_ip[i], 3);
        lv_obj_set_size(ta_ip[i], 80, 45);
        lv_obj_set_pos(ta_ip[i], 20 + (i * 90), 60);
        
        // Make clicking this octet focus the keyboard
        lv_keyboard_set_textarea(kb, ta_ip[i]); 
        lv_obj_add_event_cb(ta_ip[i], [](lv_event_t * e) {
            lv_keyboard_set_textarea((lv_obj_t*)lv_event_get_user_data(e), (lv_obj_t*)lv_event_get_target(e));
        }, LV_EVENT_FOCUSED, kb);
    }

    lv_textarea_set_text(ta_ip[0], String(udpAddress[0]).c_str());
    lv_textarea_set_text(ta_ip[1], String(udpAddress[1]).c_str());
    lv_textarea_set_text(ta_ip[2], String(udpAddress[2]).c_str());
    lv_textarea_set_text(ta_ip[3], String(udpAddress[3]).c_str());

    // Port Label and Text Area
    lv_obj_t * lbl_port = lv_label_create(screen_udp_config);
    lv_obj_set_style_text_font(lbl_port, &lv_font_montserrat_18, 0);
    lv_label_set_text(lbl_port, "PORT:");
    lv_obj_set_style_text_color(lbl_port, lv_palette_main(LV_PALETTE_CYAN), 0);
    lv_obj_set_pos(lbl_port, 20, 130);

    ta_port = lv_textarea_create(screen_udp_config);
    lv_obj_set_style_text_font(ta_port, &lv_font_montserrat_18, 0);
    lv_textarea_set_one_line(ta_port, true);
    lv_textarea_set_max_length(ta_port, 5);
    lv_obj_set_size(ta_port, 120, 45);
    lv_obj_set_pos(ta_port, 85, 115);
    lv_textarea_set_text(ta_port, String(editUDPPort).c_str());

    lv_textarea_set_text(ta_port, String(udpPort).c_str());

    // Focus keyboard on port if clicked
    lv_obj_add_event_cb(ta_port, [](lv_event_t * e) {
        lv_keyboard_set_textarea((lv_obj_t*)lv_event_get_user_data(e), (lv_obj_t*)lv_event_get_target(e));
    }, LV_EVENT_FOCUSED, kb);

    setupBackButton(screen_udp_config, PAGE_NETWORK);

}

void drawClock() {

  label_clock = lv_label_create(screen_main);
      
  // Kill the white line/box by making the background fully transparent
  lv_obj_set_style_bg_opa(label_clock, 0, 0); 
  lv_obj_set_style_border_width(label_clock, 0, 0);
  lv_obj_set_style_pad_all(label_clock, 0, 0);
  
  // Your existing styles
  lv_obj_set_style_text_font(label_clock, &lv_font_montserrat_24, 0); 
  lv_obj_set_style_text_color(label_clock, lv_palette_main(LV_PALETTE_YELLOW), 0);
  
  // Hide it and move it
  lv_obj_add_flag(label_clock, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_pos(label_clock, 5, 5);
  lv_label_set_text(label_clock, "");
}

void updateClock() {

    // Safety Checks
    if (WiFi.status() != WL_CONNECTED || screenIsOff || screen_main == NULL || label_clock == NULL) {
        return; 
    }

    if (isUpdatingOTA) return;    

    //  IF time is not valid, exit loop
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo, 0)) return; // make sure time is a valid time
    if (timeinfo.tm_year < 125) return; // make sure time is after 2025
    
    if (lv_obj_has_flag(label_clock, LV_OBJ_FLAG_HIDDEN)) {
          lv_obj_clear_flag(label_clock, LV_OBJ_FLAG_HIDDEN);
    }

    char buf[12];
    strftime(buf, sizeof(buf), "%H:%M", &timeinfo); 

    static char lastClockBuf[12] = "";
    if (strcmp(buf, lastClockBuf) != 0) {
        strcpy(lastClockBuf, buf);
        lv_label_set_text(label_clock, buf);
        
    }
}

void forceDataRedraw() {
  lastForwardPower = -1.0;
  lastForwardPeakPower = -1.0;
  lastReflectedPower = -1.0;
  lastSwr = -1.0;
  lastHighRange = "garbage";
  fwdVisualMax = -1;
  refVisualMax = -1;
  crossNeedle_resetTracking();
}

/* Read the touchpad */
void my_touchpad_read(lv_indev_t * indev, lv_indev_data_t * data) {
    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        
        // This turns raw 0-4095 into screen 0-480
        data->point.x = map(p.x, TS_MIN_X, TS_MAX_X, 0, 480);
        data->point.y = map(p.y, TS_MIN_Y, TS_MAX_Y, 0, 320);

        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void drawCalibrationScreen() {
    //  Create a new screen
    lv_obj_t * cal_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(cal_scr, lv_color_hex(0x000000), 0);

    // Main Title: "Calibrating Zero..."
    lv_obj_t * lbl1 = lv_label_create(cal_scr);
    lv_label_set_text(lbl1, "Calibrating Zero...");
    lv_obj_set_style_text_font(lbl1, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(lbl1, lv_palette_main(LV_PALETTE_CYAN), 0);
    lv_obj_align(lbl1, LV_ALIGN_CENTER, 0, -80);

    //  The Spinner (Loading Animation)
    // Parameters: parent, spin time (ms), arc length (degrees)
    lv_obj_t * spinner = lv_spinner_create(cal_scr);
    lv_spinner_set_anim_params(spinner, 5500, 12);
    lv_obj_set_style_anim_duration(spinner, 5500, 0);
    lv_obj_set_size(spinner, 80, 80);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_arc_color(spinner, lv_palette_main(LV_PALETTE_CYAN), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 6, LV_PART_INDICATOR);

    // "Please Wait" Label
    lv_obj_t * lbl2 = lv_label_create(cal_scr);
    lv_label_set_text(lbl2, "Please Wait");
    lv_obj_set_style_text_font(lbl2, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl2, lv_palette_main(LV_PALETTE_CYAN), 0);
    lv_obj_align(lbl2, LV_ALIGN_CENTER, 0, 60);

    // Version Label (at the bottom)
    lv_obj_t * lbl_ver = lv_label_create(cal_scr);
    lv_label_set_text(lbl_ver, codeVersion.c_str());
    lv_obj_set_style_text_font(lbl_ver, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_ver, lv_palette_main(LV_PALETTE_YELLOW), 0);
    lv_obj_align(lbl_ver, LV_ALIGN_BOTTOM_MID, 0, -25);

    //  Load it
    lv_screen_load(cal_scr);
    
    // Force immediate draw
    lv_timer_handler(); 
}

void updateSWRAlarmDisplay() {
    if (lbl_swr_threshold_val != NULL) {
        // Using dtostrf for bulletproof float-to-string conversion
        char buf[10];
        dtostrf(swrAlarmThreshold, 3, 1, buf); 
        lv_label_set_text(lbl_swr_threshold_val, buf);
    }
}

void setupBackButton(lv_obj_t * parent, Page destPage) {
    lv_obj_t * btn_back = lv_button_create(parent);
    lv_obj_set_size(btn_back, 60, 40);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(btn_back, lv_palette_main(LV_PALETTE_RED), 0);

    lv_obj_t * lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT);
    lv_obj_center(lbl_back);
    
    lv_obj_add_event_cb(btn_back, [](lv_event_t * e) {
    Page target = (Page)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t * old_scr = lv_screen_active(); 
    saveSettings();

    //  Special case for UDP config - save settings
    if (currentPage == PAGE_UDP_CONFIG && screen_udp_config != NULL) {
        uint8_t ip0 = (uint8_t)atoi(lv_textarea_get_text(ta_ip[0]));
        uint8_t ip1 = (uint8_t)atoi(lv_textarea_get_text(ta_ip[1]));
        uint8_t ip2 = (uint8_t)atoi(lv_textarea_get_text(ta_ip[2]));
        uint8_t ip3 = (uint8_t)atoi(lv_textarea_get_text(ta_ip[3]));

        udpAddress = IPAddress(ip0, ip1, ip2, ip3);
        
        udpPort = atoi(lv_textarea_get_text(ta_port));
        saveSettings(); 
    }

    // Prepare and Load Destination
        if (target == PAGE_MAIN) {
            if (screen_main == NULL) drawMainPage();
            lv_screen_load(screen_main);
            currentPage = PAGE_MAIN;
        } 
        else if (target == PAGE_MENU_ROOT) {
            if (screen_menu_root == NULL) drawMenuRoot(); 
            lv_screen_load(screen_menu_root);
            currentPage = PAGE_MENU_ROOT;
        }
        else if (target == PAGE_NETWORK) {
            if (screen_wifi_scan != NULL) {
                lv_obj_del_async(screen_wifi_scan);
                screen_wifi_scan = NULL;
            }
            if (screen_network == NULL)  drawNetworkPage();
            lv_screen_load(screen_network);
            currentPage = PAGE_NETWORK;
        }
        else if (target == PAGE_WIFI_SCAN) { // Added path to Scan
            if (screen_wifi_scan == NULL) drawWiFiScannerPage();
            lv_screen_load(screen_wifi_scan);
            currentPage = PAGE_WIFI_SCAN;
        }
        else if (target == PAGE_SYSTEM_INFO) {
           if (screen_system_info == NULL) drawSystemInfo();
           lv_screen_load(screen_system_info);
           currentPage = PAGE_SYSTEM_INFO;
        }
        else if (target == PAGE_FIRMWARE){
            if (screen_update_firmware == NULL) drawFirmwareUpdate();
            lv_screen_load(screen_update_firmware);
            currentPage = PAGE_FIRMWARE;
        }

    // Cleanup 
      if (old_scr != NULL) {
      if (old_scr == screen_menu_root)      screen_menu_root = NULL;
      if (old_scr == screen_settings)       screen_settings = NULL;
      if (old_scr == screen_display)        screen_display = NULL;
      if (old_scr == screen_network)        screen_network = NULL;
      if (old_scr == screen_system_info)    screen_system_info = NULL;
      if (old_scr == screen_saved_settings) screen_saved_settings = NULL;
      if (old_scr == screen_wifi_scan)      screen_wifi_scan = NULL; 
      if (old_scr == screen_keyboard)       screen_keyboard = NULL;  
      if (old_scr == screen_udp_config)     screen_udp_config = NULL;
      if (old_scr == screen_static_ip)       screen_static_ip = NULL;
      if (old_scr == screen_update_firmware) screen_update_firmware = NULL;
      if (old_scr == screen_firmware_sd)    screen_firmware_sd = NULL;

      lv_obj_del_async(old_scr);
      }
      }, LV_EVENT_CLICKED, (void*)(uintptr_t)destPage);
}

void sendDataToCOM(){

  Serial.println(buildJsonData());

}

void go_to_sleep() {
    if (screenIsOff) return;
    
    // Smooth Fade Out from current level to 0
    for (int b = backLightLevel; b >= 0; b--) {
        analogWrite(BACKLIGHT_PIN, b);
        delay(2); 
    }

    screenIsOff = true;

    // Create the invisible shield for the touch screen
    screensaver_shield = lv_btn_create(lv_scr_act());
    lv_obj_set_size(screensaver_shield, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(screensaver_shield, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(screensaver_shield, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_opa(screensaver_shield, LV_OPA_TRANSP, 0);

    lv_obj_add_event_cb(screensaver_shield, wake_up_event_cb, LV_EVENT_PRESSED, NULL);
}

void wake_screen() {   // call directly when TX starts
    if (!screenIsOff) return; // Already awake? Do nothing.

    Serial.println("Waking screen due to activity");

    for (int b = 0; b <= backLightLevel; b++) {
        analogWrite(BACKLIGHT_PIN, b);
        delay(1); 
    }
    
    if (screensaver_shield != NULL) {
        lv_obj_del(screensaver_shield);
        screensaver_shield = NULL;
    }
    
    screenIsOff = false;
    lv_disp_trig_activity(NULL); // Reset LVGL's internal timer
}

static void wake_up_event_cb(lv_event_t * e) {   // used when screen is touched while screensaver is on
    wake_screen();
}


void drawStaticIPPage() {
    if (screen_static_ip != NULL) return;
    screen_static_ip = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_static_ip, lv_color_hex(0x000000), 0);

    // Header 
    lv_obj_t * lbl_title = lv_label_create(screen_static_ip);
    lv_label_set_text(lbl_title, "Static IP Configuration");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 5);

    // Separator Line 
    static lv_point_precise_t line_points[] = { {0, 0}, {300, 0} };
    lv_obj_t * line = lv_line_create(screen_static_ip);
    lv_line_set_points(line, line_points, 2);
    lv_obj_set_style_line_width(line, 2, 0);
    lv_obj_set_style_line_color(line, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 40);

    // Container for Input Fields 
    lv_obj_t * cont = lv_obj_create(screen_static_ip);
    lv_obj_set_size(cont, 400, 140);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_text_font(cont, &lv_font_montserrat_18, 0); 
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(cont, 0, 0);
    lv_obj_set_style_border_side(cont, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_pad_all(cont, 2, 0);
    lv_obj_set_style_pad_row(cont, 5, 0); // Space between rows

    // Keyboard 
    lv_obj_t * kb = lv_keyboard_create(screen_static_ip);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_height(kb, 120); 
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_add_event_cb(kb, [](lv_event_t * e) {
        lv_event_code_t code = lv_event_get_code(e);
        if(code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
            lv_obj_add_flag((lv_obj_t *)lv_event_get_target(e), LV_OBJ_FLAG_HIDDEN);
            lv_indev_reset(NULL, NULL); 
        }
    }, LV_EVENT_ALL, NULL);

    //Error Label 
    lv_obj_t * err_lbl = lv_label_create(screen_static_ip);
    lv_obj_set_style_text_font(err_lbl, &lv_font_montserrat_18, 0); 
    lv_label_set_text(err_lbl, "Check IP Format!");
    lv_obj_set_style_text_color(err_lbl, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_align(err_lbl, LV_ALIGN_TOP_MID, 0, 45); 
    lv_obj_add_flag(err_lbl, LV_OBJ_FLAG_HIDDEN);

    // Inputs 
    const char * placeholders[] = {"IP Address", "Subnet Mask", "Gateway", "DNS"};
    IPAddress * ip_vars[] = {&local_IP, &subnet, &gateway, &primaryDNS}; 

    for(int i = 0; i < 4; i++) {
        lv_obj_t * ta = lv_textarea_create(cont);
        lv_obj_set_width(ta, 180); 
        lv_textarea_set_one_line(ta, true);
        lv_textarea_set_placeholder_text(ta, placeholders[i]);
        lv_textarea_set_text(ta, ip_vars[i]->toString().c_str());

        auto kb_event_cb = [](lv_event_t * e) {
            lv_obj_t * ta_ptr = (lv_obj_t *)lv_event_get_target(e);
            lv_obj_t * kb_ptr = (lv_obj_t *)lv_event_get_user_data(e);
            lv_keyboard_set_textarea(kb_ptr, ta_ptr);
            lv_obj_clear_flag(kb_ptr, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(kb_ptr); 
        };

        lv_obj_add_event_cb(ta, kb_event_cb, LV_EVENT_FOCUSED, kb);
        lv_obj_add_event_cb(ta, kb_event_cb, LV_EVENT_CLICKED, kb); 
    }

    // Save Button 
    lv_obj_t * btn_save = lv_button_create(screen_static_ip);
    lv_obj_set_size(btn_save, 70, 35);
    lv_obj_align(btn_save, LV_ALIGN_BOTTOM_RIGHT, -5, -5);
    lv_obj_t * btn_lbl = lv_label_create(btn_save);
    lv_label_set_text(btn_lbl, "SAVE");
    lv_obj_center(btn_lbl);

    lv_obj_add_event_cb(btn_save, [](lv_event_t * e) {
        lv_obj_t * cont_ptr = (lv_obj_t *)lv_event_get_user_data(e);
        
        IPAddress t1, t2, t3, t4;
        bool v1 = t1.fromString(lv_textarea_get_text(lv_obj_get_child(cont_ptr, 0)));
        bool v2 = t2.fromString(lv_textarea_get_text(lv_obj_get_child(cont_ptr, 1)));
        bool v3 = t3.fromString(lv_textarea_get_text(lv_obj_get_child(cont_ptr, 2)));
        bool v4 = t4.fromString(lv_textarea_get_text(lv_obj_get_child(cont_ptr, 3)));

        if(v1 && v2 && v3 && v4) {
            local_IP = t1; subnet = t2; gateway = t3; primaryDNS = t4;
            saveSettings();
            
            if(screen_network != NULL) { lv_obj_del(screen_network); screen_network = NULL; }
            drawNetworkPage();
            lv_screen_load_anim(screen_network, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, true);
            currentPage = PAGE_NETWORK;
        } else {
            lv_obj_t * scr = lv_screen_active();
            lv_obj_t * err = lv_obj_get_child(scr, 3); // Index 3 based on creation order
            if(err) {
                lv_obj_remove_flag(err, LV_OBJ_FLAG_HIDDEN);
                lv_obj_move_foreground(err);
            }
        }
    }, LV_EVENT_CLICKED, cont);

    setupBackButton(screen_static_ip, PAGE_NETWORK);

    lv_obj_t * old_scr = lv_screen_active();
    lv_screen_load(screen_static_ip);
    if (old_scr != NULL) {
        if (old_scr == screen_network) screen_network = NULL;
        lv_obj_del_async(old_scr);
    }

    lv_obj_add_event_cb(screen_static_ip, [](lv_event_t * e) { screen_static_ip = NULL; }, LV_EVENT_DELETE, NULL);
}

void drawFirmwareUpdate() {

    if (screen_update_firmware != NULL) {
        return;
    }

    screen_update_firmware = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_update_firmware, lv_color_hex(0x000000), 0);

    // Header
    lv_obj_t * header = lv_label_create(screen_update_firmware);
    lv_label_set_text(header, "FIRMWARE UPDATE");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(header, lv_color_white(), 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    // Divider
    lv_obj_t * line = lv_line_create(screen_update_firmware);
    static lv_point_precise_t pts[] = { {20, 42}, {460, 42} };
    lv_line_set_points(line, pts, 2);
    lv_obj_set_style_line_color(line, lv_palette_main(LV_PALETTE_GREY), 0);

    // Instruction text
    lv_obj_t * info = lv_label_create(screen_update_firmware);
    lv_obj_set_style_text_font(info, &lv_font_montserrat_18, 0);
    lv_label_set_text(info, "Select update method:");
    lv_obj_set_style_text_color(info, lv_palette_main(LV_PALETTE_CYAN), 0);
    lv_obj_align(info, LV_ALIGN_TOP_MID, 0, 60);

    // --- SD Card Button ---
    lv_obj_t * btn_sd = lv_btn_create(screen_update_firmware);
    lv_obj_set_size(btn_sd, 250, 60);
    lv_obj_align(btn_sd, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t * lbl_sd = lv_label_create(btn_sd);
    lv_label_set_text(lbl_sd, "SD CARD");
    lv_obj_center(lbl_sd);

    lv_obj_add_event_cb(btn_sd, [](lv_event_t * e) {

        if (screen_update_firmware) {
            lv_obj_del(screen_update_firmware);
            screen_update_firmware = NULL;
        }

        currentPage = PAGE_FIRMWARE_SD;
        drawFirmwareSD();
        lv_scr_load(screen_firmware_sd);
    }, LV_EVENT_CLICKED, NULL);

    // Back Button
    setupBackButton(screen_update_firmware, PAGE_SYSTEM_INFO);
}

void drawFirmwareSD() {
    if (screen_firmware_sd != NULL) return;

    screen_firmware_sd = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_firmware_sd, lv_color_black(), 0);

    // Header
    lv_obj_t * header = lv_label_create(screen_firmware_sd);
    lv_label_set_text(header, "SD Firmware Update");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(header, lv_color_white(), 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    // Divider
    lv_obj_t * line = lv_line_create(screen_firmware_sd);
    static lv_point_precise_t pts[] = { {20, 42}, {460, 42} };
    lv_line_set_points(line, pts, 2);
    lv_obj_set_style_line_color(line, lv_palette_main(LV_PALETTE_GREY), 0);

    // Back button
    setupBackButton(screen_firmware_sd, PAGE_FIRMWARE);

    if (sdFirmwareFile == "") {
        lv_obj_t * info = lv_label_create(screen_firmware_sd);
        lv_obj_set_style_text_font(info, &lv_font_montserrat_18, 0);
        lv_label_set_text(info, "No firmware file found!");
        lv_obj_set_style_text_color(info, lv_color_hex(0xFF0000), 0);
        lv_obj_align(info, LV_ALIGN_CENTER, 0, 0);
        Serial.println("No firmware file found!");
        return;
    }

    Serial.printf("Found firmware file: %s\n", sdFirmwareFile.c_str());

    // Show firmware file on screen
    lv_obj_t * lblFile = lv_label_create(screen_firmware_sd);
    lv_obj_set_style_text_font(lblFile, &lv_font_montserrat_18, 0);
    lv_obj_set_width(lblFile, 360); 
    lv_label_set_long_mode(lblFile, LV_LABEL_LONG_WRAP);
    String fwText = "Firmware File Found:\n" + sdFirmwareFile; 
    lv_label_set_text(lblFile, fwText.c_str());
    lv_obj_set_style_text_color(lblFile, lv_color_hex(0x00FFFF), 0);
    lv_obj_align(lblFile, LV_ALIGN_CENTER, 0, -20);

    // Confirm / Update Button
    lv_obj_t * btnConfirm = lv_btn_create(screen_firmware_sd);
    lv_obj_set_size(btnConfirm, 200, 50);
    lv_obj_align(btnConfirm, LV_ALIGN_CENTER, 0, 40);

    lv_obj_t * lblConfirm = lv_label_create(btnConfirm);
    lv_label_set_text(lblConfirm, "UPDATE NOW");
    lv_obj_center(lblConfirm);

    // click Update Firmware
    lv_obj_add_event_cb(btnConfirm, sdUpdateConfirmEvent, LV_EVENT_CLICKED, NULL);

}


void sdUpdateConfirmEvent(lv_event_t * e) {
    isUpdatingOTA = true;
    screensaverEnabled = false;


    tft.fillScreen(TFT_BLUE);
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
    tft.setTextSize(2);
    tft.drawString("Updating Firmware...", 60, 100);
    tft.drawString("Please wait for a reboot (~30s)", 60, 130);
 
    Serial.println("!!! STARTING UPDATE !!!");

    // Stop LVGL from trying to refresh in the background
    lv_display_set_flush_cb(disp, NULL); 
    
    SPI.end(); 
    delay(100);

    SPI.begin();
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, LOW); // Force select SD

    if (!SD.begin(SD_CS, SPI, 4000000)) { 
        Serial.println("CRITICAL ERROR: SD Card lost during brute force!");
        delay (150);
        ESP.restart(); // Nothing left to do but reboot
        return;
    }

    String path = sdFirmwareFile;
    if (!path.startsWith("/")) {
        path = "/" + path;
    }

    File updateFile = SD.open(path);
    if (!updateFile || updateFile.isDirectory()) {
        Serial.println("Failed to open firmware file!");
        delay (150);
        ESP.restart();
        return;
    }

    size_t totalExpected = updateFile.size();
    Serial.printf("Update size: %u bytes\n", totalExpected);

    if (!Update.begin(totalExpected)) {
        Serial.println("Update.begin failed!");
        updateFile.close();
        delay (150);
        ESP.restart();
        return;
    }

    // Flash loop
    uint8_t buffer[4096];
    size_t totalWritten = 0;
    
    while (updateFile.available()) {
        size_t len = updateFile.read(buffer, sizeof(buffer));
        if (len > 0) {
            if (Update.write(buffer, len) != len) {
                Serial.println("Flash write failed!");
                Update.printError(Serial);
                updateFile.close();
                delay(150);
                ESP.restart();
                return;
            }
            totalWritten += len;
            
            // Progress to Serial only (since TFT bus is occupied by SD)
            if (totalWritten % (4096 * 10) == 0) { 
                Serial.printf("Progress: %d%%\n", (totalWritten * 100) / totalExpected);
            }
        }
        yield();
    }

    // FINISH
    bool ok = Update.end(true);
    updateFile.close();
    SD.end();

    if (ok) {
        Serial.println("Update Success! Rebooting...");
        delay(5000); 
    } else {
        Serial.println("Update failed!");
        delay(5000);
    }

    ESP.restart();
}


void checkSDCard() {
    // if an SD card exists, grabs filename ending in .ino.bin for later use
    if (SD.begin(SD_CS, SPI, 4000000)) {
        File root = SD.open("/");
        File file = root.openNextFile();
        while (file) {
            String fileName = String(file.name());
            if (fileName.endsWith(".ino.bin")) {
                sdFirmwareFile = fileName; // Store it for later
                Serial.println("Update file found: " + sdFirmwareFile);
            }
            file.close();
            file = root.openNextFile();
        }
        SD.end(); 
    }

}

void resetSWRFault() {
    if (forwardPower < MIN_POWER_THRESHOLD && reflectedPower < MIN_POWER_THRESHOLD){
        faultState = false;
        highSWRCount = 0;
        mcp.digitalWrite(2, LOW);   // Fault relay OFF
        mcp.digitalWrite(0, HIGH);  // Fault LED OFF
        Serial.println("Fault RESET executed");
        Serial.println("SWR Fault Reset via Web Interface");
    }
}