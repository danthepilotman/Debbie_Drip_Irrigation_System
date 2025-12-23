#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ModbusMaster.h>

// ==================================================
// ================== BUILD OPTIONS =================
// ==================================================
#define DEBUG_ENABLED
//#define TEST_MODE
//#define DISABLE_RELAY   // comment out when ready for real watering

#ifdef TEST_MODE
const unsigned long UPDATE_INTERVAL = 2UL * 60UL * 1000UL; // 10 minutes
#else
const unsigned long UPDATE_INTERVAL = 6UL * 60UL * 60UL * 1000UL; // 6 hours
#endif

// ==================================================
// ================= WIFI SETTINGS ==================
// ==================================================
const char* WIFI_SSID = "Bobo";
const char* WIFI_PASS = "ryrie9219";

// ==================================================
// ================= THINGSPEAK =====================
// ==================================================
const char* TS_WRITE_KEY = "60SYG2RIJ0TW4D32";
const char* TS_READ_KEY  = "IN57T91RJ0C8NPFK";
const char* TS_CHANNEL   = "3211645";

// ==================================================
// ================= OPENWEATHER ====================
// ==================================================
const char* WEATHER_API_KEY = "1f237060a56d83d3827815039317d2a9";
const char* LAT = "28.027";
const char* LON = "-80.631";

// ==================================================
// ================= HARDWARE =======================
// ==================================================
#define RS485_RX   16
#define RS485_TX   17
#define RELAY_PIN  21

HardwareSerial rs485( 2 );
ModbusMaster node;

unsigned long lastRun = 0;

// ==================================================
// ================= DEBUG MACROS ===================
// ==================================================
#ifdef DEBUG_ENABLED
  #define DBG(x) Serial.println(x)
  #define DBGf(...) Serial.printf(__VA_ARGS__)
#else
  #define DBG(x)
  #define DBGf(...)
#endif

// ==================================================
// ================= WIFI ===========================
// ==================================================
void connectWiFi()
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

// ==================================================
// ================= WEATHER ========================
// ==================================================
bool rainExpectedSoon()
{
    DBG( F( "[WEATHER] Checking forecast" ) );

    HTTPClient http;

    String url = "https://api.openweathermap.org/data/2.5/forecast?lat=" +
                 String(LAT) + "&lon=" + String(LON) +
                 "&appid=" + WEATHER_API_KEY;

    http.begin( url );

    int code = http.GET();

    DBGf( "[WEATHER] HTTP code: %d\n", code );

    if ( code != 200 )
    {
        http.end();
        return false;
    }

    JsonDocument doc;

    DeserializationError err = deserializeJson( doc, http.getString() );

    http.end();

    if ( err )
    {
        DBG( F( "[WEATHER][ERROR] JSON parse failed" ) );
        return false;
    }

    int checks = 0;

    for ( JsonObject item : doc["list"].as<JsonArray>() )
    {
        if ( checks++ > 4 )
            break; // next 12 hours

        String main = item["weather"][0]["main"];

        DBGf( "[WEATHER] Forecast: %s\n", main.c_str() );

        if ( main == "Rain" || main == "Drizzle" || main == "Thunderstorm" )
            return true;
    }
    
    return false;

}

// ==================================================
// ================= THINGSPEAK =====================
// ==================================================
void sendThingSpeak( float m, float t, float ec, float ph, int n, int p, int k )
{

    if ( isnan( m ) || isnan( t ) || isnan( ph ) )
    {
        Serial.println( F( "[THINGSPEAK][ERROR] NaN value detected, aborting upload" ) );
        return;
    }

    String url = "http://api.thingspeak.com/update?api_key=";
    url += TS_WRITE_KEY;
    url += "&field1=" + String( m, 1);
    url += "&field2=" + String( ( 1.8 * t + 32.0 ), 1 );
    url += "&field3=" + String( int( ec ) );
    url += "&field4=" + String( ph, 1 );
    url += "&field5=" + String( n );
    url += "&field6=" + String( p );
    url += "&field7=" + String( k );

    Serial.println( F( "[THINGSPEAK] URL:" ) );
    Serial.println( url );

    HTTPClient http;
    http.begin( url );   // NOTE: http, not https
    int code = http.GET();

    Serial.print( F( "[THINGSPEAK] HTTP code: " ) );
    Serial.println( code );

    http.end();
}


void getSettings( float &threshold, int &duration )
{
    DBG( F( "[THINGSPEAK] Reading control settings" ) );

    HTTPClient http;

    String url = "https://api.thingspeak.com/channels/" +
                 String(TS_CHANNEL) +
                 "/feeds/last.json?api_key=" +
                 TS_READ_KEY;

    http.begin( url );
    int code = http.GET();
    DBGf( "[THINGSPEAK] Read HTTP code: %d\n", code );

    JsonDocument doc;
    deserializeJson( doc, http.getString() );
    http.end();

    threshold = doc["field1"] | 30.0;
    duration  = doc["field2"] | 60;

    DBGf( "[THINGSPEAK] Moisture threshold: %.1f %%\n", threshold );
    DBGf( "[THINGSPEAK] Water duration: %d sec\n", duration );
}

// ==================================================
// ================= SETUP ==========================
// ==================================================
void setup()
{
    Serial.begin( 115200 );
    
    delay( 1000 );

    DBG( F( "\n=== ESP32 SOIL IRRIGATION SYSTEM ===" ) );

    pinMode( RELAY_PIN, OUTPUT );
    digitalWrite( RELAY_PIN, LOW );

    // RS485 / Modbus
    rs485.begin( 4800, SERIAL_8N1, RS485_RX, RS485_TX );
    node.begin( 1, rs485 ); // slave ID = 1
    DBG( F( "[RS485] Modbus initialized" ) );

    connectWiFi();
}

// ==================================================
// ================= LOOP ===========================
// ==================================================
void loop()
{
    if ( millis() - lastRun < UPDATE_INTERVAL )
        return;

    lastRun = millis();

    DBG( F( "\n===== SYSTEM CYCLE START =====" ) );

    // -------- Read Soil Sensor --------
    DBG( F( "[RS485] Reading soil sensor" ) );

    uint8_t result = node.readHoldingRegisters( 0x0000, 7 );

    if ( result != node.ku8MBSuccess )
    {
        DBGf( "[RS485][ERROR] Modbus error code: %u\n", result );
        return;
    }

    uint16_t rawMoisture = node.getResponseBuffer( 0 );
    uint16_t rawTemp     = node.getResponseBuffer( 1 );
    uint16_t rawEC       = node.getResponseBuffer( 2 );
    uint16_t rawPH       = node.getResponseBuffer( 3 );
    uint16_t rawN        = node.getResponseBuffer( 4 );
    uint16_t rawP        = node.getResponseBuffer( 5 );
    uint16_t rawK        = node.getResponseBuffer( 6 );

    float moisture = rawMoisture / 10.0;
    float temp     = int16_t ( rawTemp ) / 10.0;
    float ec       = rawEC;
    float ph       = rawPH / 10.0;

    DBGf( "[DATA] Moisture: %.1f %\n", moisture );
    DBGf( "[DATA] Temp: %.1f F\n", temp );
    DBGf( "[DATA] EC: %.0f uS/cm\n", ec );
    DBGf( "[DATA] pH: %.1f\n", ph );
    DBGf( "[DATA] NPK: %u / %u / %u\n", rawN, rawP, rawK );

    // -------- ThingSpeak Upload --------
    sendThingSpeak( moisture, temp, ec, ph, rawN, rawP, rawK );

    // -------- Read Control Settings --------
    float threshold;

    int duration;

    getSettings( threshold, duration );

    // -------- Weather Check --------
    bool rain = rainExpectedSoon();

    DBGf( "[LOGIC] Rain expected soon: %s\n", rain ? "YES" : "NO" );

    // -------- Irrigation Logic --------
    if ( moisture < threshold && rain == false )
    {
        DBG( F( "[LOGIC] Watering conditions MET" ) );

#ifndef DISABLE_RELAY
        digitalWrite( RELAY_PIN, HIGH );

        delay( duration * 1000UL );

        digitalWrite( RELAY_PIN, LOW );
#else
        DBG( F( "[LOGIC] Relay disabled (TEST MODE)" ) );
#endif
    }
    else
    {
        DBG( F( "[LOGIC] Watering NOT required" ) );
    }

    DBG( F( "===== SYSTEM CYCLE END =====" ) );
}