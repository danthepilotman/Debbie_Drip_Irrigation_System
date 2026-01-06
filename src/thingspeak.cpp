#include "thingspeak.h"


const char* TS_WRITE_KEY = "60SYG2RIJ0TW4D32";
const char* TS_READ_KEY  = "IN57T91RJ0C8NPFK";
const char* TS_CHANNEL   = "3211645";
const char* TS_TALKBACK_ID = "56070"; 
const char* TS_TALKBACK_KEY = "EJ3TTWSNK2Q6PXSO";


// ==================================================
// ================= THINGSPEAK =====================
// ==================================================
bool sendThingSpeak( float m, float t, float ec, float ph, int n, int p, int k )
{

    if ( isnan( m ) || isnan( t ) || isnan( ph ) )
    {
        Serial.println( F( "[THINGSPEAK][ERROR] NaN value detected, aborting upload" ) );
        return false;
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

    Serial.print( F( "[THINGSPEAK] URL: " ) );
    Serial.println( url );

    HTTPClient http;
    http.begin( url );   // NOTE: http, not https
    int code = http.GET();

    Serial.print( F( "[THINGSPEAK] HTTP code: " ) );
    Serial.println( code );

    http.end();

    return code == HTTP_CODE_OK ? true : false;
  
}


bool getSettings( uint8_t &threshold, uint32_t &duration )
{
    DBG( F( "[THINGSPEAK] Reading control settings" ) );

    HTTPClient http;

     String url = "https://api.thingspeak.com/talkbacks/" +
                 String(TS_TALKBACK_ID) +
                 "/commands.json?api_key=" +
                 String(TS_TALKBACK_KEY);

    http.begin( url );
    int code = http.GET();
    DBGf( "[THINGSPEAK] Read HTTP code: %d\n", code );

    if ( code != HTTP_CODE_OK )
        http.end();
    

    String payload = http.getString();
    http.end();

    payload.trim();

   // Expected format: TH=45;DUR=120
    int thPos  = payload.indexOf("TH=");
    int durPos = payload.indexOf("DUR=");

    if (thPos >= 0)
        threshold = payload.substring(thPos + 3).toInt();

    if (durPos >= 0)
        duration = payload.substring(durPos + 4).toInt();


    DBGf( "[THINGSPEAK] Moisture threshold: %u %%\n", threshold );
    DBGf( "[THINGSPEAK] Water duration: %u sec\n", duration );

    return code == HTTP_CODE_OK ? true : false;
}