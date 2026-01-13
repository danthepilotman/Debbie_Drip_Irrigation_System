#include "thingspeak.h"


const char* TS_CHANNEL   = "3211645";
const char* TS_WRITE_KEY = "60SYG2RIJ0TW4D32";
const char* TS_READ_KEY  = "IN57T91RJ0C8NPFK";
const char* TS_TALKBACK_ID = "56070"; 
const char* TS_TALKBACK_KEY = "EJ3TTWSNK2Q6PXSO";


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

    String update_url = "http://api.thingspeak.com/update?api_key=";
    update_url += TS_WRITE_KEY;
    update_url += "&field1=" + String( moisture, 1);
    update_url += "&field2=" + String( ( 1.8 * t + 32.0 ), 1 );
    update_url += "&field3=" + String( int( ec ) );
    update_url += "&field4=" + String( ph, 1 );
    update_url += "&field5=" + String( n );
    update_url += "&field6=" + String( p );
    update_url += "&field7=" + String( k );
     
    // --- Add Status field ---
    update_url += "&status=" + urlEncode( String("Update sent at ") + Timestamp() );  // Use timestamp for status updates

    Serial.print( F( "[THINGSPEAK] URL: " ) );
    Serial.println( update_url );

    HTTPClient http;

    for( uint8_t tries = 0; tries < MAX_TRIES; tries++ )
    {
        http.begin( update_url );   // NOTE: http, not https
        
        int update_code = http.GET();

        http.end();

        Serial.printf( "[THINGSPEAK] HTTP code(update): %d\n", update_code );
 
        delay(2000);  // Allow some time for ThingSpeak server to process data 
    
    // -------- Check latest ThingSpeak Upload Time --------

        String time_check_url = "https://api.thingspeak.com/channels/";
            time_check_url += TS_CHANNEL;
            time_check_url += "/fields/1/last_data_age.txt";  // Latest soil data upload should have field 1 populated
     
        http.begin( time_check_url );
            
        int time_check_code = http.GET();

        String payload = http.getString();

        http.end();

        Serial.printf( "[THINGSPEAK] HTTP code(timecheck): %d\n", time_check_code );

        payload.trim();  // remove whitespace/newlines

        int age = payload.toInt();

        Serial.printf( "[THINGSPEAK] Age: %d\n", age );
      

        if ( update_code == HTTP_CODE_OK && time_check_code == HTTP_CODE_OK && age < 10 ) 
            return true;
    }

    return false;
  
}


bool getSettings()
{
    DBG(F("[THINGSPEAK] Reading control settings..."));

    HTTPClient http;

    String payload;

    String url = "https://api.thingspeak.com/talkbacks/" + String(TS_TALKBACK_ID) + "/commands.json?api_key=" + TS_TALKBACK_KEY;

    
    for(uint8_t tries = 0; tries < MAX_TRIES; tries++)
    {
    
        http.begin(url);

        int code = http.GET();

        DBGf("[THINGSPEAK] HTTP TB code: %d\n", code);

        if (code != HTTP_CODE_OK) {
            http.end();
            return false;
        }

        payload = http.getString();

        http.end();

         if(code == HTTP_CODE_OK)
             break;
    }

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

 
    long ageSeconds = secondsSincePosition1(arr);

    DBGf("[DEBUG] Seconds since TB timestamp: %ld\n", ageSeconds);

    if ( ageSeconds - TB_DELAY > TB_MAX_DELAY )
        return false;
  
       // Loop through the commands
    for (JsonObject cmd : arr)
    {
        // We rely on the 'position' field to identify the command
        int position = cmd["position"] | 0;  // fallback to 0 if missing

        String cmdStr = cmd["command_string"] | "";  // fallback to empty string if missing

        switch (position)
        {
            case 1:  // target soil moisture
                threshold = cmdStr.toFloat();
                //DBGf("[DEBUG]Threshold: %ld\n", threshold);
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

     /***************** Print TalkBack data ****************/
    
    DBGf("[THINGSPEAK] Moisture threshold: %.1f %%\n", threshold);
    DBGf("[THINGSPEAK] Water duration: %ld sec\n", duration);
    DBGf("[THINGSPEAK] Rain expected: %s\n", rain_expected_TS ? "true" : "false");
    DBGf("[THINGSPEAK] Watering needed: %s\n", watering_needed_TS ? "true" : "false");


    /***************** Store user parameters if changed from previously store ones ****************/
    
    if ( threshold != prefs.getFloat( "threshold" ,0 ) )
        prefs.putFloat( "threshold", threshold );
    
    if ( duration != prefs.getInt( "duration" ,0 ) )
        prefs.putInt( "duration", duration );
 
    return true;
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

        delay( TB_DELAY * 1000UL );  // Wait for ThingSpeak REACT to trigger and run TalkBack updates
        success = success && getSettings();

        return success;
    
    }

    else
        return success;
}


String urlEncode(const String &input)
{
    String encoded = "";
    char c;
    char buf[4];

    for (int i = 0; i < input.length(); i++)
    {
        c = input[i];

        // Unreserved characters according to RFC 3986
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            encoded += c;
        }
        else
        {
            // Percent-encode everything else
            snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
            encoded += buf;
        }
    }

    return encoded;
}


String Timestamp()
{
    
    struct tm timeinfo;

    getLocalTime(&timeinfo);      

    char buf[25];

    strftime(buf, sizeof(buf), "%m-%d-%Y %H:%M:%S", &timeinfo);

    return String(buf);
}


void solenoid_state_Update()
{
 
    String url = "http://api.thingspeak.com/update?api_key=";
    url += TS_WRITE_KEY;
    url += "&field8=" + String(solenoid_state);
    url+= "&status=" + urlEncode( String("Watering ") + String(solenoid_state ? "started" : "stopped") + " at " + Timestamp() );

 
    DBGf("[IRRIGATION] Solenoid is now %s", solenoid_state ? "ON\n" : "OFF\n");
    Serial.printf("[THINGSPEAK] URL: %s\n", url.c_str() );

    HTTPClient http;
    http.begin(url);
    int code = http.GET();

    Serial.print("[THINGSPEAK] HTTP code: ");
    Serial.println(code);
}


time_t iso8601ToEpochUsingGmtime(const char* ts)
{
    int Y, M, D, h, m, s;
    if (sscanf(ts, "%4d-%2d-%2dT%2d:%2d:%2dZ",
               &Y, &M, &D, &h, &m, &s) != 6)
        return -1;

    struct tm utc = {};
    utc.tm_year = Y - 1900;
    utc.tm_mon  = M - 1;
    utc.tm_mday = D;
    utc.tm_hour = h;
    utc.tm_min  = m;
    utc.tm_sec  = s;
    utc.tm_isdst = 0;

 
    // Get current UTC offset
    time_t now = time(nullptr);
    struct tm gmt;
    gmtime_r(&now, &gmt);  // gmt has the now time in epoch
    
    struct tm loc;
    localtime_r(&now, &loc);  // loc has the time now in epoch adjust for my timezone

    time_t gmtEpoch = mktime(&gmt);
    time_t locEpoch = mktime(&loc);
    long offset = locEpoch - gmtEpoch;  // should be 18,000 = 5 * 3600

    DBGf("Offset: %ld\n",  offset);

    // Parse as local, then remove offset to get UTC
    time_t assumedLocal = mktime(&utc);  // timestamp in epoch 

    DBGf("Assumed local: %ld\n",  assumedLocal - offset);
    return assumedLocal + offset;
}



long secondsSincePosition1(JsonArray arr)
{
    const char* ts = nullptr;

    for (JsonObject obj : arr)
    {
        if ( (int)obj["position"] == 1)
        {
            ts = obj["created_at"];
            break;
        }
    }

    if (!ts) return -1;

    time_t created = iso8601ToEpochUsingGmtime(ts);

    DBGf("Created time: %ld\n",  created);
    
    if (created < 0) return -2;

    time_t now = time(nullptr);

    long diff = now - created;

    DBGf("Return val: %ld\n",  diff);

    return (long)(now - created);
}
