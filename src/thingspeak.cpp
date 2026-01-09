#include "thingspeak.h"


const char* TS_WRITE_KEY = "60SYG2RIJ0TW4D32";
const char* TS_READ_KEY  = "IN57T91RJ0C8NPFK";
const char* TS_CHANNEL   = "3211645";
const char* TS_TALKBACK_ID = "56070"; 
const char* TS_TALKBACK_KEY = "EJ3TTWSNK2Q6PXSO";


// ==================================================
// ================= THINGSPEAK =====================
// ==================================================
bool sendThingSpeak( float m, float t, float ec, float ph, int n, int p, int k, time_t status_time )
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
     
    // --- Add Status field ---
    url += "&status=" + String( status_time );  // Use timestamp for status updates

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


bool getSettings(uint8_t &threshold, uint32_t &duration, bool &rain_expected, bool &watering_needed, time_t &status_time_TS)
{
    DBG(F("[THINGSPEAK] Reading control settings..."));

    HTTPClient http;

    String url = "https://api.thingspeak.com/talkbacks/" +
                 String(TS_TALKBACK_ID) +
                 "/commands.json?api_key=" +
                 String(TS_TALKBACK_KEY);

    http.begin(url);

    int code = http.GET();

    DBGf("[THINGSPEAK] HTTP code: %d\n", code);

    if (code != HTTP_CODE_OK) {
        http.end();
        return false;
    }

    String payload = http.getString();

    http.end();

    payload.trim();

    // Parse the JSON array from ThingSpeak TalkBack
   
    JsonDocument doc;  // Create JSON document instance

    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        DBGf("[ERROR] Failed to parse TalkBack JSON: %s\n", error.c_str());
        return false;
    }

    JsonArray arr = doc.as<JsonArray>();

    // Loop through the commands
    for (JsonObject cmd : arr)
    {
        // We rely on the 'position' field to identify the command
        int position = cmd["position"] | 0;  // fallback to 0 if missing

        String cmdStr = cmd["command_string"] | "";

        switch (position)
        {
            case 1:  // target soil moisture
                threshold = cmdStr.toInt();
                break;
            case 2:  // watering duration
                duration = cmdStr.toInt();
                break;
            case 3:  // rain expected
                rain_expected = cmdStr.toInt() != 0;
                break;
            case 4:  // watering needed
                watering_needed = cmdStr.toInt() != 0;
                break;
            case 5:  // status (string)
                status_time_TS = cmdStr.toInt();
                break;
            default:
                break;
        }
    }

    DBGf("[THINGSPEAK] Moisture threshold: %ld %%\n", threshold);
    DBGf("[THINGSPEAK] Water duration: %ld sec\n", duration);
    DBGf("[THINGSPEAK] Rain expected: %s\n", rain_expected ? "true" : "false");
    DBGf("[THINGSPEAK] Watering needed: %s\n", watering_needed ? "true" : "false");
    DBGf("[THINGSPEAK] Channel status: %ld\n", status_time_TS);

    return true;
}



// String urlEncode(const String &s)
// {
//     String encoded = "";
//     char c;
//     char buf[4];

//     for (int i = 0; i < s.length(); i++) {
//         c = s.charAt(i);
//         if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
//             encoded += c;
//         } else {
//             sprintf(buf, "%%%02X", (unsigned char)c);
//             encoded += buf;
//         }
//     }
//     return encoded;
// }


void solenoid_state_Update()
{
    time_t now;
    time(&now);

    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    char timeStr[25];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

    String url = "http://api.thingspeak.com/update?api_key=";
    url += TS_WRITE_KEY;
    url += "&field8=" + String(solenoid_state);

    // String statusMsg =
    //     "Watering " +
    //     String(solenoid_state ? "started" : "stopped") +
    //     " at " +
    //     String(timeStr);

    // url += "&status=" + urlEncode(statusMsg);

    DBGf("[IRRIGATION] Solenoid is now %s", solenoid_state ? "ON" : "OFF");
    Serial.println("[THINGSPEAK] URL:");
    Serial.println(url);

    HTTPClient http;
    http.begin(url);
    int code = http.GET();

    Serial.print("[THINGSPEAK] HTTP code: ");
    Serial.println(code);
}
