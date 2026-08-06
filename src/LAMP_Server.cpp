#include "LAMP_Server.h"  // Associated header file
#include "update_OLED.h"  // For display_mesage() function
#include "weather.h" // For avg_precip_prob variable
#include "ota_update.h" // for FIRMWARE_VERSION const char
#include "setup.h"  // status_str.wattering_needed variable


const char* postServerName = "http://dldesigns.doesntexist.com:30/LAMP-Server/Irrigation%20System/php/post-esp-data.php";

const char* settingsServerName = "http://dldesigns.doesntexist.com:30/LAMP-Server/Irrigation%20System/php/get_settings.php";


bool applyLocalSettings()
{
    JsonDocument local_doc;

    File file = LittleFS.open("/irrigation_settings.json", "r");  // Open settings file for reading

    if (!file)
    {
        return false;  // Return false if local settings file doesn't exist
    }


    if ( deserializeJson( local_doc, file ) )  // Deserialize JSON from file into document
    {
        file.close();  // Close file
        return false; // Return false on deserialization error
    }

    file.close();

    settings.moisture_threshold = local_doc["moisture_threshold"] | 55.0;
    settings.watering_duration_sec  = local_doc["watering_duration"]  | 3600;
    settings.min_precip_prob    = local_doc["min_precip_prob"]    | 50.0;

    JsonArray times = local_doc["times"];

    for ( uint8_t i = 0; i < SCHEDULE_COUNT && i < times.size(); i++ )
    {
        uint8_t h, m, s;

        sscanf( times[i], "%d:%d:%d", &h, &m, &s);

        settings.times[i].hour = h;
        settings.times[i].min  = m;
        settings.times[i].sec  = s;
    }

    strlcpy( settings.updated, local_doc["updated"] | "", sizeof( settings.updated ) );

#ifdef DEBUG_ENABLED

    DBG(F("[SETTINGS] Local settings are applied"));
    
#endif

    return true;
}


bool getServerSettings()
{
    HTTPClient http;

    http.begin( settingsServerName );  // Specify destination for HTTP request

    int httpCode = http.GET();  // Send HTTP GET request

    if ( httpCode != HTTP_CODE_OK )  // Check for successful response
    {

#ifdef DEBUG_ENABLED
        DBGf("[SETTINGS] HTTP GET failed: %d", httpCode);
#endif

        http.end();  // Free resources
        return false; // Return false on failure
    }


    //--------------------------------------------------
    // Deserialize HTTP response directly into JSON doc
    //--------------------------------------------------

    JsonDocument server_doc; // Create a JSON document to hold the response

    DeserializationError error = deserializeJson( server_doc, http.getStream() );  // Deserialize JSON from HTTP response stream to JsonDocument

    if ( error ) // Check for deserialization errors
    {
#ifdef DEBUG_ENABLED
        DBGf("[SETTINGS] JSON parse failed: %s", error.c_str());
#endif

        http.end();  // Free resources
        return false;  // Return false on failure
    }


    //--------------------------------------------------
    // HTTP connection no longer needed
    //--------------------------------------------------

    http.end(); // Free resources


    //--------------------------------------------------
    // Check server timestamp
    //--------------------------------------------------

    const char *serverUpdated = server_doc["updated"] | "";  // Get server timestamp, default to empty string if not present


    //--------------------------------------------------
    // Determine whether server has newer settings
    //--------------------------------------------------

    if ( serverSettingsAreNewer( serverUpdated ) )  // Compare server timestamp with local timestamp
    {

        applyDownloadedSettings( server_doc );  // Apply new settings from server
       
    } 

    return true;  // Return true on success 
}


bool serverSettingsAreNewer( const char *serverUpdated )
{
    
    if ( applyLocalSettings() == false )  // Load local settings to ensure we have the latest timestamp
    {
        return true;  // If local settings can't be loaded, assume server settings are newer
    }

    if ( settings.updated[0] == '\0' )  // Check if local timestamp is empty
    {
        return true;  // If local timestamp is empty, assume server settings are newer
    }

    if ( serverUpdated == nullptr || serverUpdated[0] == '\0')  // Check if server timestamp is null or empty
    {
        return false;  // If server timestamp is null or empty, apply local settings
    }

   
   
    return strcmp( serverUpdated, settings.updated ) > 0;  // Return true if server timestamp is newer than local timestamp

}


void applyDownloadedSettings( JsonDocument &server_doc )
{
    
#ifdef DEBUG_ENABLED

    DBG(F("[SETTINGS] New server settings found"));

#endif    
    
    //--------------------------------------------------
    // Copy scalar settings
    //--------------------------------------------------

    settings.moisture_threshold = server_doc["moisture_threshold"] | settings.moisture_threshold;  // Soil moisture threshold to trigger watering

    settings.watering_duration_sec = server_doc["watering_duration"] | settings.watering_duration_sec;  // Watering time in seconds

    settings.min_precip_prob = server_doc["min_precip_prob"] | settings.min_precip_prob;  // Minimum probability of rain to set rain_expected flag


    //--------------------------------------------------
    // Copy watering schedule
    //--------------------------------------------------

    JsonArray times = server_doc["times"];  // Get watering schedule array from JSON document

    for (uint8_t i = 0; i < SCHEDULE_COUNT && i < times.size(); i++ )  // Loop through each schedule slot, up to SCHEDULE_COUNT or the size of the JSON array
    {
        unsigned int h;
        unsigned int m;
        unsigned int s;

        if ( sscanf(times[i], "%u:%u:%u", &h, &m, &s) == 3 )  // Parse time string in "HH:MM:SS" format
        {
            settings.times[i].hour = h;
            settings.times[i].min  = m;
            settings.times[i].sec  = s;
        }
    }


    //--------------------------------------------------
    // Copy server timestamp
    //--------------------------------------------------

    strlcpy( settings.updated, server_doc["updated"] | "", sizeof(settings.updated ) );  // Copy server timestamp to local settings, default to empty string if not present


    //--------------------------------------------------
    // Save updated settings locally
    //--------------------------------------------------

    if ( saveLocalSettings( server_doc ) == false )  // Check if saving settings to FS was successful
    {

#ifdef DEBUG_ENABLED
        DBG(F("[SETTINGS] Failed to save local settings"));
#endif

    }

    else
    {

#ifdef DEBUG_ENABLED
        DBG(F("[SETTINGS] New settings saved locally"));
#endif

    }

}


bool saveLocalSettings( JsonDocument &server_doc )  // Save settings to FS
{

    File file = LittleFS.open("/irrigation_settings.json", "w");  // Open file for writing

    if (!file)  // Check if file opened successfully
    {
#ifdef DEBUG_ENABLED
        DBG(F("[FILESYSTEM] Failed to open settings file"));
#endif
        return false;  // Return false on failure
    }

    serializeJsonPretty( server_doc, file );  // Save JSON to file

    file.close();  // Close file

#ifdef DEBUG_ENABLED

    serializeJsonPretty( server_doc, Serial );

#endif

    return true;  // Return true on success
}


void solenoid_state_Update()  // Report solenoid state to server
{

#ifdef THINGSPEAK_ENABLE

    const char* url = "https://api.thingspeak.com/update";

    status.status_str = String("Watering ") +
                        String(status.solenoid_state ? "started " : "stopped ") +
                        Timestamp();

    char status_c[128];

    urlEncode(status.status_str).toCharArray(status_c, sizeof(status_c));

#ifdef DEBUG_ENABLED

    DBGf("[IRRIGATION] Solenoid is now %s",
         status.solenoid_state ? "ON\r\n" : "OFF\r\n");

#endif

    // Build ThingSpeak POST body
    String postData = "api_key=" + String(TS_WRITE_KEY);
    postData += "&field8=" + String(status.solenoid_state ? 1 : 0);
    postData += "&status=" + String(status_c);

#ifdef DEBUG_ENABLED

    DBGf("[THINGSPEAK] POST body: %s\r\n", postData.c_str());

#endif

    ThingSpeakResponse resp = tsClient.postWithRetry(
        url,
        postData,
        MAX_TRIES,
        TS_PROCESS_DELAY
    );

#ifdef DEBUG_ENABLED

    DBGf("[THINGSPEAK] HTTP code: %d, payload: %s\r\n",
         resp.httpCode,
         resp.body.c_str());

#endif


#else

    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {

        WiFiClient client;

        HTTPClient http;

        char buff[256];


        // --------------------------------------------------
        // Create status message
        // --------------------------------------------------

        status.status_str = String("Watering ") + String(status.solenoid_state ? "started " : "stopped ");


        // --------------------------------------------------
        // Get WiFi RSSI
        // --------------------------------------------------

        int WiFi_RSSI = WiFi.RSSI();


        // --------------------------------------------------
        // URL encode status message
        // --------------------------------------------------

        String encodedStatus = urlEncode(status.status_str);


        // --------------------------------------------------
        // Prepare HTTP POST data
        // --------------------------------------------------

        String httpRequestData =
              "solenoid_state=" + String(status.solenoid_state ? 1 : 0)
            + "&WiFi_RSSI=" + String(WiFi_RSSI)
            + "&status_message=" + encodedStatus
            + "&time_stamp=" + Timestamp("%Y-%m-%d %H:%M:%S");


        // Specify content type
        http.addHeader(
            F("Content-Type"),
            F("application/x-www-form-urlencoded")
        );


        // Specify destination
        http.begin(client, postServerName);


        // Send HTTP POST request
        int httpResponseCode = http.POST(httpRequestData);


        sprintf( buff, "HTTP code:\r\n%d", httpResponseCode );

        display_message(buff, 2000);

        http.end();

    }

    else
    {
        display_message( "WiFi Disconnected", 2000 );
    }

#endif  // THINGSPEAK_ENABLE

}


void sendServerUpdate()
{

    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {

        WiFiClient client;

        HTTPClient http;

        char buff[256];


        // --------------------------------------------------
        // Get WiFi RSSI
        // --------------------------------------------------

        int WiFi_RSSI = WiFi.RSSI();


        // --------------------------------------------------
        // Prepare HTTP POST data
        // --------------------------------------------------

        status.status_str = String( "DB update sent. " ) 
                          + String( status.watering_needed ? "" : "Watering skipped. " ) + 
                          + "SW: v" + String( FIRMWARE_VERSION );
        
        String encodedStatus = urlEncode(status.status_str);


        String httpRequestData =
              "moisture_wvc=" + String(soil.moisture)
            + "&temperature=" + String(soil.temp)
            + "&ec=" + String(soil.ec)
            + "&ph=" + String(soil.pH)
            + "&nitrogen=" + String(soil.N)
            + "&potassium=" + String(soil.K)
            + "&phosphorus=" + String(soil.P)
            + "&solenoid_state=" + String(status.solenoid_state ? 1 : 0)
            + "&average_PoP=" + String( avg_precip_prob, 1 )
            + "&WiFi_RSSI=" + String(WiFi_RSSI)
            + "&status_message=" + encodedStatus
            + "&time_stamp=" + Timestamp("%Y-%m-%d %H:%M:%S");


        // Specify content type
        http.addHeader(
            F("Content-Type"),
            F("application/x-www-form-urlencoded")
        );


        // Specify destination
        http.begin(client, postServerName);


        // Send HTTP POST request
        int httpResponseCode = http.POST( httpRequestData );

#ifdef DEBUG_ENABLED

        DBGf("DB POST HTTP code: %d\r\n", httpResponseCode );

#endif

        sprintf( buff, "DB POST HTTP code:\r\n%d", httpResponseCode );

        display_message(buff, 2000);

        http.end();

    }

    else
    {
        display_message("WiFi Disconnected", 2000);
    }

}