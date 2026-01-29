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

    HTTPClient http;
    
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
    update_url += "&status=" + urlEncode( String( "Update sent " ) + Timestamp() );  // Use timestamp for status updates

    Serial.printf( "[THINGSPEAK] URL: %s", update_url.c_str() );
    

    for( uint8_t tries = 1; tries <= MAX_TRIES; tries++ )
    {

        int update_code, time_check_code ;  // Store HTTP response codes

        String payload;
        
        if( tries == 1 || update_code !=  HTTP_CODE_OK )  // Try updating at least once or if not getting a good HTTP response code
        {
            http.begin( update_url );   // NOTE: http, not https
            
            update_code = http.GET();  // Use GET to send HTTP update to TS and retrieve response code

            http.end();

            Serial.printf( "[THINGSPEAK] HTTP code(update): %d\r\n", update_code );

            delay( TS_DELAY );  // Allow some time for ThingSpeak server to process data

        }
    
    // -------- Check latest ThingSpeak Upload Time --------

        String time_check_url = "https://api.thingspeak.com/channels/";
            time_check_url += TS_CHANNEL;
            time_check_url += "/fields/1/last_data_age.txt";  // Latest soil data upload should have field 1 populated
    
        if( tries == 1 || time_check_code !=  HTTP_CODE_OK )  // Try updating at least once or if not getting a good HTTP response code
        {
            http.begin( time_check_url );
                
            time_check_code = http.GET();

            payload = http.getString();

            http.end();
        }

        Serial.printf( "[THINGSPEAK] HTTP code(timecheck): %d\r\n", time_check_code );

        payload.trim();  // remove whitespace/newlines

        int age = payload.toInt();

        Serial.printf( "[THINGSPEAK] Age: %d\r\n", age );
    

        if ( update_code == HTTP_CODE_OK && time_check_code == HTTP_CODE_OK && age <= ( TS_DELAY/1000UL * MAX_TRIES ) )
        {
            Serial.println( "[THINGSPEAK] Upload successful" );
            break;
        }

    }

    String rssi_url = "http://api.thingspeak.com/update?api_key=VJQGRESCP5X57UVG&field1=" + String( WiFi.RSSI() );

    http.begin( rssi_url );
        
    int update_code_RSSI = http.GET();

    http.end();

    Serial.printf( "[THINGSPEAK] HTTP code(RSSI upload): %d\r\n", update_code_RSSI );
    
    return false;

}


void getSettings()
{
    DBG( F( "[THINGSPEAK] Reading control settings..." ) );

    HTTPClient http;

    String payload;

    String url = "https://api.thingspeak.com/talkbacks/" + String(TS_TALKBACK_ID) + "/commands.json?api_key=" + TS_TALKBACK_KEY;

    
    for( uint8_t tries = 1; tries <= MAX_TRIES; ++tries )
    {
    
        delay( TB_DELAY );
        
        http.begin( url );

        int code = http.GET();

        DBGf( "[THINGSPEAK] HTTP TB code: %d\r\n", code );

        payload = http.getString();

        http.end();

        if( code == HTTP_CODE_OK )
            break;

        if ( tries == MAX_TRIES )
            return;
 
    }

    payload.trim();

    // Parse the JSON array from ThingSpeak TalkBack
    
    JsonDocument doc;  // Create JSON document instance

    DeserializationError error = deserializeJson( doc, payload );
    if (error)
    {
        DBGf( "[ERROR] Failed to parse TalkBack JSON: %s\r\n", error.c_str() );
        return;
    }

    JsonArray arr = doc.as<JsonArray>();

    long ageSeconds = secondsSincePosition1( arr );

    DBGf( "[DEBUG] Seconds since TB timestamp: %ld\r\n", ageSeconds );


       // Loop through the commands
    for ( JsonObject cmd : arr )
    {
        // We rely on the 'position' field to identify the command
        int position = cmd["position"] | 0;  // fallback to 0 if missing

        String cmdStr = cmd["command_string"] | "";  // fallback to empty string if missing

        switch ( position )
        {
            case 1:  // target soil moisture
                settings.threshold = cmdStr.toFloat();
                break;
            case 2:  // watering duration
                settings.duration = cmdStr.toInt();
                break;
            case 3:  // rain expected
                rain_expected_TS = cmdStr.toInt() != 0; // Any non-zero value → true
                break;
            case 4:  // watering needed
                watering_needed_TS = cmdStr.toInt() != 0;  // Any non-zero value → true
                break;
             case 5:  // update schedule
                update_Schedule ( cmdStr, position );
                break;
            case 6:  // update schedule
                update_Schedule ( cmdStr, position );
                break;
            case 7:  // update schedule
                update_Schedule ( cmdStr, position );
                break;
            case 8:  // update schedule
                update_Schedule ( cmdStr, position );
                break;
            default:
                break;
        }
    }

     /***************** Print TalkBack data ****************/
    
    DBGf( "[THINGSPEAK] Moisture threshold: %.1f %%\r\n", settings.threshold );
    DBGf( "[THINGSPEAK] Water duration: %ld sec\r\n", settings.duration );
    DBGf( "[THINGSPEAK] Rain expected: %s\r\n", rain_expected_TS ? "true" : "false" );
    DBGf( "[THINGSPEAK] Watering needed: %s\r\n", watering_needed_TS ? "true" : "false" );


    /***************** Store user parameters if changed from previously store ones ****************/
    
    if ( check_new_settings() == true )
        saveSettings();   // write ONLY if something changed

}



/******************************* Get SOIL sensor readings and update ThingSpeak *********************/
void get_new_readings()
{

    uint8_t num_of_attemps = 0;
    

    DBG( F( "[STATUS] ===== SYSTEM CYCLE START =====" ) );

    uint16_t values[7] = {227,203,100,70,50,40,30}; // Store 7 register values

    RS485_STATUS status;

    // -------- Read Soil Sensor --------
#ifdef SOIL_SENSOR

    DBG( F( "[RS485] Reading soil sensor" ) );

    for( num_of_attemps = 0; num_of_attemps < 5; num_of_attemps++ )
    {
        status = read_Registers( RS485Serial, 0x01, 0x0000, 5, values );

        if (status == RS485_GOOD)
            break;
    }

    if ( status != RS485_GOOD )
    {
        DBG( F( "[RS485][ERROR] Modbus error" ) );
    }
        
#endif
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

    DBGf( "[DATA] Moisture: %.1f %%\r\n", moisture );
    DBGf( "[DATA] Temp: %.1f °C\r\n", temp );
    DBGf( "[DATA] EC: %.0f µS/cm\r\n", ec );
    DBGf( "[DATA] pH: %.1f\r\n", ph );
    DBGf( "[DATA] NPK: %u / %u / %u mg/kg\r\n", rawN, rawP, rawK );

    
    if( wifi_connectivity )
    {
        sendThingSpeak( temp, ec, ph, rawN, rawP, rawK );  // -------- ThingSpeak Upload --------

        getSettings();  // -------- Read Control Settings --------

    }
    
}