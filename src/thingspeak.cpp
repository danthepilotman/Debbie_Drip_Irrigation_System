#include "thingspeak.h"  // ThingSpeak helpers: upload, TalkBack parsing, and settings


#ifdef DEBBIE_HOUSE

const char* TS_CHANNEL   = "3211645";  // ThingSpeak channel id
const char* TS_WRITE_KEY = "60SYG2RIJ0TW4D32";  // ThingSpeak write key
const char* TS_READ_KEY  = "IN57T91RJ0C8NPFK";  // ThingSpeak read key
const char* TS_TALKBACK_ID = "56070";  // TalkBack id
const char* TS_TALKBACK_KEY = "EJ3TTWSNK2Q6PXSO";  // TalkBack key
const char* TS_WRITE_KEY_2 = "VJQGRESCP5X57UVG";  // ThingSpeak write key for RSSI updates

#else

const char* TS_CHANNEL   = "3325050";  // ThingSpeak channel id
const char* TS_WRITE_KEY = "WZ6S3B4NWT6PKXD2";  // ThingSpeak write key
const char* TS_READ_KEY  = "SQJTM2FB7HICPK6G";  // ThingSpeak read key
const char* TS_TALKBACK_ID = "56669";  // TalkBack id
const char* TS_TALKBACK_KEY = "993A55E3RP9ZI8H0";  // TalkBack key


const char* TS_WATERING_ID = "3325052";  // Watering channel ID used for watering parameters
const char* TS_WATERING_WRITE_KEY = "YLGO60STV9UCS8HL";  // Watering channel write key used for watering parameters status updates
const char* TS_WATERING_READ_KEY = "6LSSZSC5PSPZ0AJQ";  // Watering channel write key used for watering parameters status updates


#endif

// ==================================================
// ================= THINGSPEAK =====================
// ==================================================
void sendThingSpeak()
{

    HTTPClient http;  // HTTP client used for uploads and checks
    
    // Guard against invalid sensor values
    if ( isnan( soil.temp ) || isnan( soil.ec )|| isnan( soil.pH ) )
    {
        Serial.println( F( "[THINGSPEAK][ERROR] NaN value detected, aborting upload" ) );
        return;
    }

    char url[256];  // buffer for update URL (snprintf protects from overflow)
    
    status.status_str = String( "Update sent " ) + Timestamp();  // status message
    
    char status_c[128];  // C-string for status
    
    urlEncode(status.status_str).toCharArray(status_c, sizeof(status_c));  // copy to C-string

    snprintf( url, sizeof(url),  // build ThingSpeak update URL
        "https://api.thingspeak.com/update?api_key=%s&field1=%.1f&field2=%.1f&field3=%d&field4=%.1f&field5=%d&field6=%d&field7=%d&status=%s",
        TS_WRITE_KEY, soil.moisture, ( 1.8 * soil.temp + 32.0 ), int(soil.ec), soil.pH, soil.N, soil.P, soil.K, status_c );

    Serial.printf( "[THINGSPEAK] URL: %s\r\n", url );  // debug: print update URL
    

    // Try several times: upload then confirm server saw it by checking last_data_age
    for( uint8_t tries = 1; tries <= MAX_TRIES; tries++ )  // retry loop
    {

        int update_code = 0, time_check_code = 0;  // Store HTTP response codes, init to avoid stale reads

        String payload;  // HTTP response payload buffer
        
        if( tries == 1 || update_code !=  HTTP_CODE_OK )  // attempt update if first try or previous update failed
        {
            http.begin( url );   // NOTE: http, not https
            
            update_code = http.GET();  // Use GET to send HTTP update to TS and retrieve response code

            http.end();  // close connection

            Serial.printf( "[THINGSPEAK] HTTP code(update): %d\r\n", update_code );  // log update HTTP status

            delay( 5000 );  // Allow some time for ThingSpeak server to process data

        }
    
    // -------- Check latest ThingSpeak Upload Time --------

       
        snprintf( url, sizeof(url), "https://api.thingspeak.com/channels/%s/fields/1/last_data_age.txt?api_key=%s", TS_CHANNEL, TS_READ_KEY );  // build URL to check last upload age
    
        if( tries == 1 || time_check_code !=  HTTP_CODE_OK )  // try time check if first try or previous check failed
        {
            http.begin(url );  // request last_data_age
                
            time_check_code = http.GET();  // GET last_data_age

            payload = http.getString();  // read response body

            http.end();
        }

        Serial.printf( "[THINGSPEAK] HTTP code(timecheck): %d\r\n", time_check_code );  // log timecheck status

        payload.trim();  // remove whitespace/newlines

        int age = payload.toInt();  // parse age in seconds

        Serial.printf( "[THINGSPEAK] Age: %d\r\n", age );  // log age

          
        if ( update_code == HTTP_CODE_OK && time_check_code == HTTP_CODE_OK && age <= ( 5*(tries + 1) ) )  // success: HTTP OK and age within threshold
        {
            Serial.println( "[THINGSPEAK] Upload successful" );  // confirm success
            break;
        }

        delay( TS_PROCESS_DELAY );  // Allow some time for ThingSpeak server to process data before retrying

    }

    send_RSSI();  // No need to wait since RSSI is on a different channel, so send immediately after main update attempts
}


void getSettings()
{
    
    JsonDocument doc;  // Create JSON document for parsing TalkBack response

    uint8_t tries;
    
    HTTPClient http;

    char url[256];  // buffer for TalkBack URL

    JsonArray arr;  // parsed TalkBack commands array
    
    snprintf( url, sizeof(url), "https://api.thingspeak.com/talkbacks/%s/commands.json?api_key=%s", TS_TALKBACK_ID, TS_TALKBACK_KEY );  // build TalkBack commands URL

#ifdef DEBUG_ENABLED

    DBG( F( "[THINGSPEAK] Reading control settings..." ) );  // indicate TalkBack fetch

#endif

    
    for( tries = 1; tries <= MAX_TRIES; ++tries )  // retry loop for TalkBack fetch
    {
           
        /************************ Get watering settings *************************/

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

            DBGf( "[ERROR] Failed to parse Watering JSON: %s\r\n", error.c_str() );

#endif
            return;
        }

        else
            break;  // success, exit retry loop
        
    }

    if ( tries > MAX_TRIES )  // give up after max tries
            return;  // Exit function if no valid TalkBack data


    arr = doc.as<JsonArray>();  // root is array

        
       // Loop through the commands
    for ( JsonObject cmd : arr )  // iterate TalkBack commands
    {
        // We rely on the 'position' field to identify the command
        int position = cmd["position"] | 0;  // fallback to 0 if missing

        String cmdStr = cmd["command_string"] | "";  // fallback to empty string if missing

        switch ( position )
        {
            case 1:  // target soil moisture
                settings.threshold = cmdStr.toFloat();  // Update threshold
                break;
            case 2:  // watering duration
                settings.duration = cmdStr.toInt();  // Update duration
                break;
            case 3:  // rain expected
                settings.rain_min_Prob = cmdStr.toInt();  // Update rain minimum probability
                break;
             case 4:  // update schedule
                update_Schedule ( cmdStr, position );  // Update schedule
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
            default:
                break;
        }
    }

    
    /***************** Print TalkBack data ****************/
#ifdef DEBUG_ENABLED    

    DBGf( "[THINGSPEAK] Moisture threshold: %.1f %%\r\n", settings.threshold );  // show threshold
    DBGf( "[THINGSPEAK] Water duration: %ld sec\r\n", settings.duration );  // show duration
    DBGf( "[THINGSPEAK] Rain minimum probability: %ld %%\r\n", settings.rain_min_Prob );  // show rain minimum probability

#endif

    /***************** Store user parameters if changed from previously store ones ****************/
    
    if ( check_new_settings() == true )   // write ONLY if something changed
        saveSettings();  // save settings to FS

}


void thingSpeak_Update()
{

 if( status.wifi_connectivity && esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0)  // upload and refresh settings only if woke up from timer and have WiFi                                                                                       
    {                                                                                         
        sendThingSpeak();  // -------- ThingSpeak Upload --------

        getSettings();  // -------- Read Control Settings --------, check TalkBack timestamp to ensure freshness of data before applying settings

    }

}


void ping_ThingSpeak()
{

  char url[256];  // Char array to hold URL

  status.status_str =  String( "POWER_ON / RESET at " )  + Timestamp();  // URL encode status string

  char status_c[256];  // Char array to hold URL encoded status string

  HTTPClient http;  // Create HTTP client object

  urlEncode(status.status_str).toCharArray( status_c, sizeof(status_c) );  // Convert to C-string

  // Build URL for ThingSpeak update                                     
  snprintf( url, sizeof(url), "https://api.thingspeak.com/update?api_key=%s&status=%s", TS_WRITE_KEY, status_c ); 
                              
#ifdef DEBUG_ENABLED
 
  DBGf( "[STATUS] Sending first wake status message to ThingSpeak\r\n" );  // Print solenoid state
  Serial.printf( "[THINGSPEAK] URL: %s\r\n", url );  // Print URL being used

#endif

   
  for( uint8_t tries = 1; tries <= MAX_TRIES; tries++ )  // Try updating up to MAX_TRIES times
  {

    http.begin( url );  // Start HTTP session
        
    int response_code = http.GET();  // Use GET to send HTTP update to TS and retrieve response code

    http.end(); // End HTTP session
    
    Serial.printf("[THINGSPEAK] HTTP code: %d\r\n", response_code );  // Print HTTP response code

    if( response_code == HTTP_CODE_OK )  // Successful update
      break;  // Exit retry loop if successful

    delay( TS_PROCESS_DELAY );  // Allow some time for ThingSpeak server to process data before retrying

  }

  send_RSSI();  // Send WiFi RSSI to ThingSpeak Channel

}


void send_RSSI()
{

    HTTPClient http;  // Create HTTP client object
    
    char url[256];  // Char array to hold URL

    status.wifi_rssi = WiFi.RSSI();  // Get WiFi RSSI


    // Build URL for ThingSpeak update                                     
    snprintf( url, sizeof(url), "https://api.thingspeak.com/update?api_key=%s&field1=%d", TS_WATERING_WRITE_KEY, status.wifi_rssi ); 

    for( uint8_t tries = 1; tries <= MAX_TRIES; tries++ )  // Try updating up to MAX_TRIES times
    {
    
        http.begin(url );  // send RSSI
            
        int response_code = http.GET();  // get RSSI upload HTTP code

        http.end();  // close connection

        Serial.printf( "[THINGSPEAK] HTTP code(RSSI upload): %d\r\n", response_code );  // log RSSI upload status

        if( response_code == HTTP_CODE_OK )  // Successful update
            break;  // Exit retry loop if successful
        
        delay( TS_PROCESS_DELAY );  // Allow some time for ThingSpeak server to process data before retrying

    }
    
}