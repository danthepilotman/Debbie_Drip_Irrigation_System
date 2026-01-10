#include "thingspeak.h"


const char* TS_WRITE_KEY = "60SYG2RIJ0TW4D32";
const char* TS_READ_KEY  = "IN57T91RJ0C8NPFK";
const char* TS_CHANNEL   = "3211645";
const char* TS_TALKBACK_ID = "56070"; 
const char* TS_TALKBACK_KEY = "EJ3TTWSNK2Q6PXSO";


String iso8601_Timestamp()
{
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo)) 
        return "1970-01-01T00:00:00Z";  // fallback
    

    char buf[25];

    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);

    return String(buf);
}


// ==================================================
// ================= THINGSPEAK =====================
// ==================================================
bool sendThingSpeak( float t, float ec, float ph, int n, int p, int k )
{

    if ( isnan( t ) || isnan( ec )|| isnan( ph ) )
    {
        Serial.println( F( "[THINGSPEAK][ERROR] NaN value detected, aborting upload" ) );
        return false;
    }

    String url = "http://api.thingspeak.com/update?api_key=";
    url += TS_WRITE_KEY;
    url += "&field1=" + String( moisture, 1);
    url += "&field2=" + String( ( 1.8 * t + 32.0 ), 1 );
    url += "&field3=" + String( int( ec ) );
    url += "&field4=" + String( ph, 1 );
    url += "&field5=" + String( n );
    url += "&field6=" + String( p );
    url += "&field7=" + String( k );
     
    // --- Add Status field ---
    url += "&status=" + String("Update%20sent%20at%20") + iso8601_Timestamp();  // Use timestamp for status updates

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


bool getSettings()
{
    DBG(F("[THINGSPEAK] Reading control settings..."));

    HTTPClient http;

    String url = "https://api.thingspeak.com/talkbacks/" + String(TS_TALKBACK_ID) + "/commands.json?api_key=" + TS_TALKBACK_KEY;

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

        String cmdStr = cmd["command_string"] | "";  // fallback to empty string if missing

        switch (position)
        {
            case 1:  // target soil moisture
                threshold = cmdStr.toInt();
                break;
            case 2:  // watering duration
                duration = cmdStr.toInt();
                break;
            case 3:  // rain expected
                rain_expected_TS = cmdStr.toInt() != 0; // Any non-zero value → true
                break;
            case 4:  // watering needed
                watering_needed_TS = cmdStr.toInt() != 0;  // Any non-zero value → true
                break;
            default:
                break;
        }
    }

    DBGf("[THINGSPEAK] Moisture threshold: %ld %%\n", threshold);
    DBGf("[THINGSPEAK] Water duration: %ld sec\n", duration);
    DBGf("[THINGSPEAK] Rain expected: %s\n", rain_expected_TS ? "true" : "false");
    DBGf("[THINGSPEAK] Watering needed: %s\n", watering_needed_TS ? "true" : "false");
 
    return true;
}


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


/******************************* Get SOIL sensor readings and update ThingSpeak *********************/   
bool get_new_readings()
{

    bool success = false;  // Assume false

    uint8_t num_of_attemps = 0;
    
    if ( watering_needed_ESP32 == NO )  // Don't keep getting new readings if watering is not needed
    {
        DBG( F( "[STATUS] ===== SYSTEM CYCLE START =====" ) );

        // -------- Read Soil Sensor --------
        DBG( F( "[RS485] Reading soil sensor" ) );

        uint16_t values[7]; // Store 7 register values

        RS485_STATUS status;

        for(num_of_attemps = 0; num_of_attemps < 5; num_of_attemps++)
        {
            status = read_Registers( RS485Serial, 0x01, 0x0000, 5, values );

            if (status == RS485_GOOD)
                break;
        }

        if ( status != RS485_GOOD )
        {
            success = false;
            DBG(  F( "[RS485][ERROR] Modbus error" ) );
        }
        
    

        uint16_t rawMoisture = values[ SOIL_MOISTURE ];
        uint16_t rawTemp     = values[ SOIL_TEMPERATURE ];
        uint16_t rawEC       = values[ SOIL_EC];
        uint16_t rawPH       = values[ SOIL_PH ];
        uint16_t rawN        = values[ SOIL_N ];
        uint16_t rawP        = values[ SOIL_P ];
        uint16_t rawK        = values[ SOIL_K ];

        moisture = float(rawMoisture) / 10.0;
        float temp     = float( int16_t( rawTemp ) ) / 10.0;
        float ec       = float(rawEC);
        float ph       = float(rawPH) / 10.0;

        DBGf( "[DATA] Moisture: %.1f %%\n", moisture );
        DBGf( "[DATA] Temp: %.1f °C\n", temp );
        DBGf( "[DATA] EC: %.0f µS/cm\n", ec );
        DBGf( "[DATA] pH: %.1f\n", ph );
        DBGf( "[DATA] NPK: %u / %u / %u mg/kg\n", rawN, rawP, rawK );

        // -------- ThingSpeak Upload --------

        success = ( status ==  RS485_GOOD ) && sendThingSpeak( temp, ec, ph, rawN, rawP, rawK );

        // -------- Read Control Settings --------

        float threshold;  // Water content by volume percentage minimum threshold to trigger watering cycle

        delay( 60000 );  // Replace with last update timestamp check
        
        success = success && getSettings();

        return success;
    
    }

    else
        return success;
}
