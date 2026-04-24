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
    // Guard against invalid sensor values
    if (isnan(soil.temp) || isnan(soil.ec) || isnan(soil.pH))
    {
        Serial.println(F("[THINGSPEAK][ERROR] NaN value detected, aborting upload"));
        return;
    }

    const char* url_post = "https://api.thingspeak.com/update";

    status.status_str = String("Update sent ") + Timestamp();

    char status_c[128];
    
    urlEncode(status.status_str).toCharArray(status_c, sizeof(status_c));

    Serial.println("[THINGSPEAK] Preparing POST upload");

    // Build POST body only (no HTTP code here anymore)
    String postData = "api_key=" + String(TS_WRITE_KEY);

    postData += "&field1=" + String( soil.moisture, 1 );
    postData += "&field2=" + String( ( 1.8 * soil.temp + 32.0 ), 1) ;
    postData += "&field3=" + String( int( soil.ec ) );
    postData += "&field4=" + String( soil.pH, 1 );
    postData += "&field5=" + String( soil.N );
    postData += "&field6=" + String( soil.P );
    postData += "&field7=" + String( soil.K );
    postData += "&field8=" + String( status.solenoid_state ? 1 : 0 );
    postData += "&status=" + String(status_c);

#ifdef DEBUG_ENABLED
        Serial.printf("[THINGSPEAK] POST body: %s\r\n", postData.c_str());
#endif

    ThingSpeakResponse resp = tsClient.postWithRetry( url_post, postData, MAX_TRIES, TS_PROCESS_DELAY );  // SINGLE abstraction call replaces all HTTP logic

    Serial.printf( "[THINGSPEAK] HTTP code(update): %d payload: %s\r\n", resp.httpCode, resp.body.c_str() );  // log HTTP status and payload for debugging

    send_RSSI();  // No need to wait since RSSI is on a different channel, so send immediately after main update attempts
}


void getSettings()
{
    
    JsonDocument doc;  // Create JSON document for parsing TalkBack response
    JsonArray arr;  // parsed TalkBack commands array

    char url[256];  // buffer for TalkBack URL

    snprintf( url, sizeof(url), "https://api.thingspeak.com/talkbacks/%s/commands.json?api_key=%s", TS_TALKBACK_ID, TS_TALKBACK_KEY );  // build TalkBack commands URL

#ifdef DEBUG_ENABLED

    DBG( F( "[THINGSPEAK] Reading control settings..." ) );  // indicate TalkBack fetch

#endif

    display.clearDisplay();
    display.setCursor(0,0);

    display.print(F("[THINGSPEAK] Reading control settings...")); 
    display.display();
        
   
     bool success = false;
    
    for( uint8_t tries = 1; tries <= MAX_TRIES; ++tries )  // retry loop for TalkBack fetch
    {
           
        /************************ Get watering settings *************************/

        ThingSpeakResponse resp = tsClient.getWithRetry(url, MAX_TRIES, TS_PROCESS_DELAY);

#ifdef DEBUG_ENABLED

        DBGf( "[THINGSPEAK] HTTP TB code: %d\r\nbody: %s", resp.httpCode, resp.body.c_str() );  // log TB HTTP status

#endif

        if( resp.httpCode != HTTP_CODE_OK )  // skip if not OK
        {
            delay(1000);
            continue;
        }

        doc.clear(); // Clear previous JSON document
        
        DeserializationError error = deserializeJson( doc, resp.body );  // parse JSON payload

       
        if (error)  // abort on parse error
        {

#ifdef DEBUG_ENABLED

            DBGf( "[ERROR] Failed to parse Watering JSON: %s\r\n", error.c_str() );

#endif
            return;
        }

        success = true;
        break;
        
    }

    if ( success == false )  // give up after max tries
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

    display.clearDisplay();
    display.setCursor(0,0);
    
    display.print(F("[THINGSPEAK]\r\nSuccessfully read control settings.")); 
    display.display();
    delay(2000);  // Leave time for user to read message on OLED before updating with settings details

    /***************** Store user parameters if changed from previously store ones ****************/
    
    if ( check_new_settings() == true )   // write ONLY if something changed
        saveSettings();  // save settings to FS
    
}


void thingSpeak_Update()
{

 if( status.wifi_connectivity )  // upload and refresh settings only if woke up from timer and have WiFi                                                                                       
    {                                                                                         
        sendThingSpeak();  // -------- ThingSpeak Upload --------

        getSettings();  // -------- Read Control Settings --------, check TalkBack timestamp to ensure freshness of data before applying settings

    }

}


void ping_ThingSpeak()
{
    
#ifdef DEBUG_ENABLED
    DBGf( "[STATUS] Sending first wake status message to ThingSpeak\r\n" );
#endif

    display.clearDisplay();
    display.setCursor( 0, 0 );
    display.print( F( "[STATUS]\r\nSending first wake status message to ThingSpeak" ) );
    display.display();

    status.status_str = String( "POWER_ON / RESET on " ) + Timestamp();
    String body = "api_key=" + String( TS_WRITE_KEY );
    body += "&status=" + urlEncode( status.status_str );

#ifdef DEBUG_ENABLED
    Serial.printf( "[THINGSPEAK] POST body: %s\r\n", body.c_str() );
#endif

    ThingSpeakResponse resp =
        tsClient.postWithRetry(
            "https://api.thingspeak.com/update",
            body,
            MAX_TRIES,
            TS_PROCESS_DELAY
        );

    Serial.printf( "[THINGSPEAK] HTTP code: %d, payload: %s\r\n", resp.httpCode, resp.body.c_str() );

    // keep original behavior
    send_RSSI();
}


void send_RSSI()
{
    status.wifi_rssi = WiFi.RSSI();

    String body = "api_key=" + String( TS_WATERING_WRITE_KEY );

    body += "&field1=" + String( status.wifi_rssi );

    ThingSpeakResponse resp = tsClient.postWithRetry(
            "https://api.thingspeak.com/update",
            body,
            MAX_TRIES,
            TS_PROCESS_DELAY
        );

    Serial.printf( "RSSI upload code: %d body: %s\n", resp.httpCode, resp.body.c_str() );
}