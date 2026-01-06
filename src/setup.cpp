#include "setup.h"


HardwareSerial rs485( 2 );
ModbusMaster node;


const unsigned long SERIAL_BAUD_RATE = 115200;

const char* WIFI_SSID = "Bobo";
const char* WIFI_PASS = "ryrie9219";

const char *timeZone = "EST5EDT,M3.2.0,M11.1.0";  // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
const char *ntpServer_1 = "pool.ntp.org";
const char *ntpServer_2 = "time.nist.gov";
const char *ntpServer_3 = "north-america.pool.ntp.org";

void setup_Serial()
{

  Serial.begin( SERIAL_BAUD_RATE );
    
  delay( 1000 );

  DBG( F( "\n=== ESP32 SOIL IRRIGATION SYSTEM ===" ) ); 

}


void setup_Digital()
{

 pinMode( RELAY_PIN, OUTPUT );
 
 digitalWrite( RELAY_PIN, LOW );

}


void setup_RS485()
{

    rs485.begin( 4800, SERIAL_8N1, RS485_RX, RS485_TX );

    node.begin( 1, rs485 ); // slave ID = 1

    DBG( F( "[RS485] Modbus initialized" ) );

}


// ==================================================
// ================= NTP ===========================
// ==================================================
void setup_NTP()
{

  struct tm timeinfo;

  configTzTime( timeZone, ntpServer_1, ntpServer_2, ntpServer_3 );

  while( getLocalTime( &timeinfo ) == false )
    DBG( F ( "[NTP] Failed to obtain time from NTP server." ) );

  DBG( F ( "[NTP] Got good time update from NTP server." ) );

}


// ==================================================
// ================= WIFI ===========================
// ==================================================
void connect_WiFi()
{
    DBG( F( "[WIFI] Connecting..." ) );

    WiFi.begin( WIFI_SSID, WIFI_PASS );

    while ( WiFi.status() != WL_CONNECTED )
    {
        delay( 500 );
        Serial.print( "." );
    }

    Serial.println();
    DBG( F( "[WIFI] Connected" ) );
    DBGf( "[WIFI] IP: %s\n", WiFi.localIP().toString().c_str() ) ;
}