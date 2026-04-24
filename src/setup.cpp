#include "setup.h"  // project-wide definitions and prototypes
#include "update_OLED.h"  // prototypes for OLED update functions
#include "helper.h"  // helper functions


bool firmware_pending_verify = true;


// User interface serial port setup parameters
const unsigned long SERIAL_BAUD_RATE = 115200;  // Serial baud rate for debug output


// Create a hardware serial instance for RS485 communication
const int8_t RS485_TX_PIN = 17;  // RS485 TX pin (GPIO)
const int8_t RS485_RX_PIN = 16;  // RS485 RX pin (GPIO)

const unsigned long RS485_BAUD = 4800;  // RS485 bus baud rate (sensor)

HardwareSerial RS485Serial(2); // Use UART2 instance for RS485 communication

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_NeoPixel rgb(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


volatile bool buttonPressed = false;
volatile Page currentPage = PAGE_STATUS;

#ifdef DEBBIE_HOUSE

const char* WIFI_SSID = "SpectrumSetup-5B"; // WiFi SSID for Debbie's house
const char* WIFI_PASS = "cosmicmajor724"; // WiFi password for Debbie's house

#else
const char* WIFI_SSID = "Bobo"; // default WiFi SSID
const char* WIFI_PASS = "ryrie9219"; // default WiFi password

// const char* WIFI_SSID = "Linksys00164-guest"; // default WiFi SSID
// const char* WIFI_PASS = "nitsuJ12"; // default WiFi password

#endif


// NTP parameters
const char *timeZone = "EST5EDT,M3.2.0,M11.1.0";  // POSIX timezone string for EST/EDT
const char *ntpServer_1 = "pool.ntp.org"; // primary NTP server
const char *ntpServer_2 = "time.nist.gov"; // secondary NTP server
const char *ntpServer_3 = "north-america.pool.ntp.org"; // tertiary NTP server


// Default initialization (optional)
Settings settings = {
    32.1,  // threshold (soil moisture threshold percent)
    1800,  // duration (watering seconds)
    50,  // rain_min_Prob (minimum probability of rain)
    {{0,0,0},  // times - schedule slot 0
    {6,0,0},   // schedule slot 1
    {12,0,0},  // schedule slot 2
    {16,0,0}}  // schedule slot 3
};

Soil soil;
Status status;


void IRAM_ATTR handleButtonInterrupt()
{
  buttonPressed = true;   // keep ISR SHORT
}


void setup_RGB()
{
  rgb.begin(); // Initialize NeoPixel library
  rgb.setPixelColor(0, rgb.Color(255, 0, 0)); // Set LED to RED (indicating system is starting up)
  rgb.show(); // Update the LED strip to show the new color
}

void setup_OLED()
{

  Wire.begin( 5, 6 );  // SDA, SCL
  
  if( display.begin( SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS ) == false )
  { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();  // Clear display buffer
  
  display.setRotation(2); // Rotate display if needed (adjust as per your mounting)
  display.setTextSize(1);   // Draw 1X-scale text
  display.setTextColor(SSD1306_WHITE);  // Draw white text
  display.setTextWrap(true); // Enable text wrapping
  display.setCursor(0,0);    // Start at top-left corner
  display.print(F("Soil Monitoring &\r\nIrrigation System\r\nV1.1"));  // Initial splash screen

  display.display();  // Show initial message
  
  delay(2000);  // Display splash screen for 2 seconds

  display.clearDisplay();  // Clear display for next updates

  display.display();  // Update display to show cleared screen

}


void setup_Serial()
{

  Serial.begin( SERIAL_BAUD_RATE ); // Start Serial at configured baud
    
  delay( 1000 ); // allow Serial to initialize

#ifdef DEBUG_ENABLED  

  DBG( F( "\n[STATUS] === ESP32 SOIL IRRIGATION SYSTEM ===" ) ); // print startup banner

#endif

}


void setup_Discretes()
{

  pinMode( 15, INPUT_PULLUP );  // Setup button pin with pull-up resistor

  attachInterrupt( digitalPinToInterrupt(15 ), handleButtonInterrupt, FALLING );  // Se=tup interrupt on button pin for falling edge (button press)

  esp_sleep_enable_ext1_wakeup(
    1ULL << GPIO_NUM_15,
    ESP_EXT1_WAKEUP_ANY_LOW
); // Set GPIO17 (button pin) as wakeup source with LOW level trigger
  
  pinMode( RELAY_PIN, OUTPUT ); // Configure relay pin as output

  digitalWrite( RELAY_PIN, LOW ); // Ensure relay is off by default

}


void setup_RS485()
{

    RS485Serial.begin( RS485_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN ); // Initialize RS485 UART

#ifdef DEBUG_ENABLED

    DBG( F( "[RS485] Modbus initialized" ) ); // Log RS485 initialization

#endif    

}


// ==================================================
// ================= WIFI ===========================
// ==================================================
void connect_WiFi()
{
    
    uint8_t timeout = 0; // timeout counter for connection attempts

    Serial.print( F( "[WIFI] Connecting" ) ); // indicate start of connection

    display.clearDisplay();
    display.setCursor(0,0);
    display.print(F("[WIFI] Connecting to WiFi...\r\n")); 
    display.display();

 
    WiFi.begin( WIFI_SSID, WIFI_PASS, 6 ); // attempt to connect to WiFi

    while ( WiFi.status() != WL_CONNECTED && timeout < 60 ) // wait up to ~60 seconds
    {
      rgb.setPixelColor(0, rgb.Color(0, 255, 0)); // Set LED to off
      rgb.show(); // Update the LED strip to show the new color

      delay( 500 ); // wait half a second between dots

      rgb.setPixelColor(0, rgb.Color(255, 0, 0)); // Set LED to green
      rgb.show(); // Update the LED strip to show the new color

      delay( 500 ); // wait half a second between dots

      Serial.print( "." ); // progress indicator
      timeout++; // increment timeout
    }

    if ( WiFi.status() == WL_CONNECTED )
    {
      status.wifi_connectivity = true;  // set connectivity flag when connected
      status.wifi_rssi = WiFi.RSSI();  // store RSSI for status display and ThingSpeak upload
    }
    
    else
    {
      status.wifi_connectivity = false;  // clear connectivity flag on failure

#ifdef DEBUG_ENABLED

      DBG( F( "[WIFI] Unable to connect to WiFi" ) ); // log failure

#endif    

    display.print(F( "[WIFI] Unable to connect to WiFi" )); 
    display.display();

       // bail out if not connected
    }

    Serial.println(); // finish progress line

#ifdef DEBUG_ENABLED

    DBG( F( "[WIFI] Connected" ) ); // log successful connection
    DBGf( "[WIFI] IP: %s\r\n", WiFi.localIP().toString().c_str() ) ; // print assigned IP
    DBGf( "[WIFI] RSSI: %d dBm\r\n", status.wifi_rssi ) ; // print signal strength

#endif

    wifi_Page(); // Update OLED with WiFi status

}


// ==================================================
// ================= NTP ===========================
// ==================================================
void setup_NTP()
{

  struct tm timeinfo; // allocate time info struct

  uint8_t tries = 0; // retry counter for NTP

  configTzTime( timeZone, ntpServer_1, ntpServer_2, ntpServer_3 ); // configure timezone and NTP servers

  while( getLocalTime( &timeinfo ) == false && tries < 5 ) // attempt to get time with retries
  {

#ifdef DEBUG_ENABLED

    DBG( F ( "[NTP] Failed to obtain time from NTP server" ) ); // log retry attempt

#endif

    display.clearDisplay();
    display.setCursor(0,0);

    display.print(F ( "[NTP] Failed to obtain time from NTP server" )); 
    display.display();   

    tries++;  // retry a few times
  }

  
  if ( getLocalTime( &timeinfo ) == false ) // final check after retries
    return; // give up if still no time
    
  else
  {

#ifdef DEBUG_ENABLED

    DBG( F ( "[NTP] Got good time update from NTP server" ) ); // log success

#endif

    display.clearDisplay();
    display.setCursor(0,0);
   
    display.print(F("[NTP] Got good time update from NTP server")); 
    display.display();
    delay(1000); // allow user to read message on OLED before updating with status info   

  }

}


void checkForOTAUpdate()
{
    String latestVersion;
    String firmwareUrl;

    if (!getFirmwareInfo(latestVersion, firmwareUrl))
        return;

    Serial.println("Current: " + String(FIRMWARE_VERSION));
    Serial.println("Latest: " + latestVersion);

    if (isNewer(latestVersion))
    {
        Serial.println("Update available!");
        performOTA(firmwareUrl);
    }
    else
    {
        Serial.println("Firmware up to date.");
    }
}