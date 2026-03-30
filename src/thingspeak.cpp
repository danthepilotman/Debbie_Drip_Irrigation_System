#include "thingspeak.h"  // ThingSpeak helpers: upload, TalkBack parsing, and settings



const char* TS_CHANNEL   = "3211645";  // ThingSpeak channel id
const char* TS_WRITE_KEY = "60SYG2RIJ0TW4D32";  // ThingSpeak write key
const char* TS_READ_KEY  = "IN57T91RJ0C8NPFK";  // ThingSpeak read key
const char* TS_TALKBACK_ID = "56070";  // TalkBack id
const char* TS_TALKBACK_KEY = "EJ3TTWSNK2Q6PXSO";  // TalkBack key


// ==================================================
// ================= THINGSPEAK =====================
// ==================================================
void sendThingSpeak( float t, float ec, float ph, int n, int p, int k )
{

    HTTPClient http;  // HTTP client used for uploads and checks
    
    // Guard against invalid sensor values
    if ( isnan( t ) || isnan( ec )|| isnan( ph ) )
    {
        Serial.println( F( "[THINGSPEAK][ERROR] NaN value detected, aborting upload" ) );
        return;
    }

    char update_url[256];  // buffer for update URL (snprintf protects from overflow)
    String status_str = urlEncode( String( "Update sent " ) + Timestamp() );  // status message
    char status_c[128];  // C-string for status
    status_str.toCharArray(status_c, sizeof(status_c));  // copy to C-string

    snprintf( update_url, sizeof(update_url),  // build ThingSpeak update URL
        "https://api.thingspeak.com/update?api_key=%s&field1=%.1f&field2=%.1f&field3=%d&field4=%.1f&field5=%d&field6=%d&field7=%d&status=%s",
        TS_WRITE_KEY, moisture, ( 1.8 * t + 32.0 ), int(ec), ph, n, p, k, status_c );

    Serial.printf( "[THINGSPEAK] URL: %s\r\n", update_url );  // debug: print update URL
    

    // Try several times: upload then confirm server saw it by checking last_data_age
    for( uint8_t tries = 1; tries <= MAX_TRIES; tries++ )  // retry loop
    {

        int update_code = 0, time_check_code = 0;  // Store HTTP response codes, init to avoid stale reads

        String payload;  // HTTP response payload buffer
        
        if( tries == 1 || update_code !=  HTTP_CODE_OK )  // attempt update if first try or previous update failed
        {
            http.begin( update_url );   // NOTE: http, not https
            
            update_code = http.GET();  // Use GET to send HTTP update to TS and retrieve response code

            http.end();  // close connection

            Serial.printf( "[THINGSPEAK] HTTP code(update): %d\r\n", update_code );  // log update HTTP status

            delay( TS_DELAY );  // Allow some time for ThingSpeak server to process data

        }
    
    // -------- Check latest ThingSpeak Upload Time --------

        char time_check_url[128];
        snprintf( time_check_url, sizeof(time_check_url), "https://api.thingspeak.com/channels/%s/fields/1/last_data_age.txt", TS_CHANNEL );  // build URL to check last upload age
    
        if( tries == 1 || time_check_code !=  HTTP_CODE_OK )  // try time check if first try or previous check failed
        {
            http.begin( time_check_url );  // request last_data_age
                
            time_check_code = http.GET();  // GET last_data_age

            payload = http.getString();  // read response body

            http.end();
        }

        Serial.printf( "[THINGSPEAK] HTTP code(timecheck): %d\r\n", time_check_code );  // log timecheck status

        payload.trim();  // remove whitespace/newlines

        int age = payload.toInt();  // parse age in seconds

        Serial.printf( "[THINGSPEAK] Age: %d\r\n", age );  // log age
    

        if ( update_code == HTTP_CODE_OK && time_check_code == HTTP_CODE_OK && age <= ( TS_DELAY/1000UL * MAX_TRIES ) )  // success: HTTP OK and age within threshold
        {
            Serial.println( "[THINGSPEAK] Upload successful" );  // confirm success
            break;
        }

    }

    send_RSSI();
}


void getSettings()
{
    
#ifdef DEBUG_ENABLED

    DBG( F( "[THINGSPEAK] Reading control settings..." ) );  // indicate TalkBack fetch

#endif

    uint8_t tries;
    
    HTTPClient http;

    char url[128];  // buffer for TalkBack URL

    JsonArray arr;  // parsed TalkBack commands array
    
    snprintf( url, sizeof(url), "https://api.thingspeak.com/talkbacks/%s/commands.json?api_key=%s", TS_TALKBACK_ID, TS_TALKBACK_KEY );  // build TalkBack commands URL

    
    for( tries = 1; tries <= MAX_TRIES; ++tries )  // retry loop for TalkBack fetch
    {
    
        delay( TB_DELAY );  // delay between retries
        
        http.begin( url );  // request TalkBack commands

        int code = http.GET();  // HTTP response code

#ifdef DEBUG_ENABLED

        DBGf( "[THINGSPEAK] HTTP TB code: %d\r\n", code );  // log TB HTTP status

#endif

        if( code != HTTP_CODE_OK )  // skip if not OK
        {
            http.end();
            continue;
        }

        doc.clear(); // Clear previous JSON document
        
        DeserializationError error = deserializeJson( doc, http.getString() );  // parse JSON payload

        http.end();
        
        if (error)  // abort on parse error
        {

#ifdef DEBUG_ENABLED

            DBGf( "[ERROR] Failed to parse TalkBack JSON: %s\r\n", error.c_str() );

#endif
            return;
        }

        arr = doc.as<JsonArray>();  // root is array

        long ageSeconds = secondsSincePosition1( arr );  // compute age since position 1

#ifdef DEBUG_ENABLED

        DBGf( "[THINGSPEAK] Seconds since TB timestamp: %ld\r\n", ageSeconds );  // debug print ageSeconds

#endif
        
        if ( ageSeconds >= 0 && ageSeconds <= ( MAX_TRIES * ( TB_DELAY / 1000UL ) ) )  // validate freshness of TB data
        {

#ifdef DEBUG_ENABLED

            DBG( F( "[THINGSPEAK] Valid TalkBack data received" ) );

#endif
            break;
 
        }

    }

    if ( tries == MAX_TRIES )  // give up after max tries
            return;  // Exit function if no valid TalkBack data

        
       // Loop through the commands
    for ( JsonObject cmd : arr )  // iterate TalkBack commands
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

    
    doc.clear();  // Clear JSON document to free memory
    
    /***************** Print TalkBack data ****************/
#ifdef DEBUG_ENABLED    

    DBGf( "[THINGSPEAK] Moisture threshold: %.1f %%\r\n", settings.threshold );  // show threshold
    DBGf( "[THINGSPEAK] Water duration: %ld sec\r\n", settings.duration );  // show duration
    DBGf( "[THINGSPEAK] Rain expected: %s\r\n", rain_expected_TS ? "true" : "false" );  // show rain_expected flag
    DBGf( "[THINGSPEAK] Watering needed: %s\r\n", watering_needed_TS ? "true" : "false" );  // show watering_needed flag

#endif

    /***************** Store user parameters if changed from previously store ones ****************/
    
    if ( check_new_settings() == true )
        saveSettings();   // write ONLY if something changed

}


/******************************* Get SOIL sensor readings and update ThingSpeak *********************/
void get_new_readings()
{
    
#ifdef DEBUG_ENABLED

    DBG( F( "[STATUS] ===== SYSTEM CYCLE START =====" ) );  // mark cycle start

#endif

    RS485_STATUS status;  // RS485 operation status

    // -------- Read Soil Sensor --------
#ifdef SOIL_SENSOR

    uint16_t values[SOIL_REG_SIZE]; // Store 7 register values

#ifdef DEBUG_ENABLED

    DBG( F( "[RS485] Reading soil sensor" ) );

#endif

    for( uint8_t num_of_attempts = 0; num_of_attempts < MAX_TRIES; ++num_of_attempts )  // try up to 5 times
    {
        status = read_Registers( RS485Serial, 0x01, 0x0000, 5, values );  // read registers via Modbus

        if ( status == RS485_GOOD )
            break;
        
#ifdef DEBUG_ENABLED

        else
            DBG( F( "[RS485][ERROR] Modbus error" ) );
#endif
        
    }
       
#else

    uint16_t values[SOIL_REG_SIZE] = {227,203,100,70,50,40,30}; // Store 7 register values

#endif

    uint16_t rawMoisture = values[ SOIL_MOISTURE ];  // raw moisture register
    uint16_t rawTemp     = values[ SOIL_TEMPERATURE ];  // raw temperature register
    uint16_t rawEC       = values[ SOIL_EC];  // raw EC register
    uint16_t rawPH       = values[ SOIL_PH ];  // raw pH register
    uint16_t rawN        = values[ SOIL_N ];  // raw N register
    uint16_t rawP        = values[ SOIL_P ];  // raw P register
    uint16_t rawK        = values[ SOIL_K ];  // raw K register

    moisture = float(rawMoisture) / 10.0;  // convert to percent
    float temp     = float( int16_t( rawTemp ) ) / 10.0;  // convert to °C (signed)
    float ec       = float(rawEC);  // conductivity µS/cm
    float ph       = float(rawPH) / 10.0;  // pH scaled by 10

#ifdef DEBUG_ENABLED

    DBGf( "[DATA] Moisture: %.1f %%\r\n", moisture );  // log moisture
    DBGf( "[DATA] Temp: %.1f °C\r\n", temp );  // log temperature
    DBGf( "[DATA] EC: %.0f µS/cm\r\n", ec );  // log EC
    DBGf( "[DATA] pH: %.1f\r\n", ph );  // log pH
    DBGf( "[DATA] NPK: %u / %u / %u mg/kg\r\n", rawN, rawP, rawK );  // log NPK registers

#endif

    
    if( wifi_connectivity )  // upload and refresh settings when WiFi available
    {
        sendThingSpeak( temp, ec, ph, rawN, rawP, rawK );  // -------- ThingSpeak Upload --------

        getSettings();  // -------- Read Control Settings --------

    }
    
}


void ping_ThingSpeak()
{

  char url[256];  // Char array to hold URL

  String status_str =  urlEncode( "First waking at " )  + urlEncode( Timestamp() );  // URL encode status string

  char status_c[128];  // Char array to hold URL encoded status string

  HTTPClient http;  // Create HTTP client object


  status_str.toCharArray( status_c, sizeof(status_c) );  // Convert to C-string

  // Build URL for ThingSpeak update                                     
  snprintf( url, sizeof(url), "https://api.thingspeak.com/update?api_key=VJQGRESCP5X57UVG&status=%s", status_c ); 
                              
#ifdef DEBUG_ENABLED
 
  DBGf( "[STATUS] Sending first wake status message to ThingSpeak\r\n" );  // Print solenoid state
  Serial.printf( "[THINGSPEAK] URL: %s\r\n", url );  // Print URL being used

#endif

   
  for( uint8_t tries = 1; tries <= MAX_TRIES; tries++ )  // Try updating up to MAX_TRIES times
  {

    int code = HTTP_CODE_BAD_REQUEST;  // Store HTTP response codes. Initialized to bad code to force retry if needed

    if( code !=  HTTP_CODE_OK )  // Try if not getting a good HTTP response code
    {
      http.begin( url );  // Start HTTP session
        
      code = http.GET();  // Use GET to send HTTP update to TS and retrieve response code

      http.end(); // End HTTP session
    }

    Serial.printf("[THINGSPEAK] HTTP code: %d\r\n", code );  // Print HTTP response code

    if( code == HTTP_CODE_OK )  // Successful update
      break;  // Exit retry loop if successful

  }

  send_RSSI();  // Send WiFi RSSI to ThingSpeak Channel

}


void send_RSSI()
{

    HTTPClient http;  // Create HTTP client object

    char rssi_url[128];  // URL for RSSI upload (connectivity monitoring)
    snprintf( rssi_url, sizeof(rssi_url), "https://api.thingspeak.com/update?api_key=VJQGRESCP5X57UVG&field1=%d", WiFi.RSSI() );  // build RSSI upload URL

    http.begin( rssi_url );  // send RSSI
        
    int update_code_RSSI = http.GET();  // get RSSI upload HTTP code

    http.end();  // close connection

    Serial.printf( "[THINGSPEAK] HTTP code(RSSI upload): %d\r\n", update_code_RSSI );  // log RSSI upload status
    
}