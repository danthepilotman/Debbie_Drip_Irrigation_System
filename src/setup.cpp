#include "setup.h"


// User interface serial port setup parameters
const unsigned long SERIAL_BAUD_RATE = 115200;


// Create a hardware serial instance for RS485 communication
const int8_t RS485_TX_PIN = 17;
const int8_t RS485_RX_PIN = 16;

const unsigned long RS485_BAUD = 4800;

HardwareSerial RS485Serial(2); // Use UART2


// Wifi network parameters
const char* WIFI_SSID = "Bobo";
const char* WIFI_PASS = "ryrie9219";


// NTP parameters
const char *timeZone = "EST5EDT,M3.2.0,M11.1.0";  // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
const char *ntpServer_1 = "pool.ntp.org";
const char *ntpServer_2 = "time.nist.gov";
const char *ntpServer_3 = "north-america.pool.ntp.org";


Preferences prefs;


void setup_Serial()
{

  Serial.begin( SERIAL_BAUD_RATE );
    
  delay( 1000 );

  DBG( F( "\n[STATUS] === ESP32 SOIL IRRIGATION SYSTEM ===" ) ); 

}


void setup_Digital()
{

 pinMode( RELAY_PIN, OUTPUT );
 
 digitalWrite( RELAY_PIN, LOW );

}


void setup_RS485()
{

    RS485Serial.begin( RS485_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN );

    DBG( F( "[RS485] Modbus initialized" ) );

}


// ==================================================
// ================= WIFI ===========================
// ==================================================
void connect_WiFi()
{
    Serial.print( F( "[WIFI] Connecting" ) );

    WiFi.begin( WIFI_SSID, WIFI_PASS );

    while ( WiFi.status() != WL_CONNECTED )
    {
        delay( 500 );
        Serial.print( "." );
    }

    Serial.println();
    DBG( F( "[WIFI] Connected" ) );
    DBGf( "[WIFI] IP: %s\n", WiFi.localIP().toString().c_str() ) ;
    DBGf( "[WIFI] RSSI: %d dBm\n", WiFi.RSSI() ) ;
}


// ==================================================
// ================= NTP ===========================
// ==================================================
void setup_NTP()
{

  struct tm timeinfo;

  uint8_t tries = 0;

  configTzTime( timeZone, ntpServer_1, ntpServer_2, ntpServer_3 );

  while( getLocalTime( &timeinfo ) == false && tries < 5 )
  {
    DBG( F ( "[NTP] Failed to obtain time from NTP server" ) );
    tries++;
  }

  DBG( F ( "[NTP] Got good time update from NTP server" ) );

}


void setup_Storage()
{


  prefs.begin( "TalkBack", false ); // false = read/write privileges

  threshold = prefs.getFloat( "threshold", 0.0 );

  duration = prefs.getInt( "duration", 0 );

}