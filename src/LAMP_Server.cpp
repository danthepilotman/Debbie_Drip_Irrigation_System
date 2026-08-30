#include "LAMP_Server.h"  // Associated header file
#include "update_OLED.h"  // For display_mesage() function
#include "weather.h" // For avg_precip_prob variable
#include "ota_update.h" // for FIRMWARE_VERSION const char
#include "setup.h"  // status_str.wattering_needed variable


const char*phpServerName = "http://dldesigns.doesntexist.com:30/LAMP-Server/Irrigation%20System/php/";

const char* errorlogFileName = "/error_log.txt";

const char* debuglogFileName = "/debug_log.txt";

const uint8_t IRRIGATION_ZONE = 1;


bool getServerSettings()
{
    HTTPClient http;

    JsonDocument server_doc; // Create a JSON document to hold the response


    for ( uint8_t i = 0; i <= MAX_TRIES; ++i )
    {
    
        if ( i == MAX_TRIES )
        {
            return false; // Return false on failure
        }
        
        const String url = String( phpServerName ) + String ( "get_settings.php" );
        
        http.begin( url );  // Specify destination for HTTP request

        int httpCode = http.GET();  // Send HTTP GET request

        if ( httpCode != HTTP_CODE_OK )  // Check for successful response
        {

            http.end();  // Close HTTP connection

            char buff[512];  // Message buffer
                    
            snprintf ( buff, sizeof ( buff ), "[SETTINGS] HTTP GET failed: %d", httpCode );  // Build message
                    
            logToFile ( buff, errorlogFileName );  // Log to error file

            display_message ( buff, 2000 );  // Show error message on OLED

    #ifdef DEBUG_ENABLED
            DBGf("[SETTINGS] HTTP GET failed: %d", httpCode);
    #endif
            
            continue;  // Skip the remainder of the for loop
                    
        }
    

        //--------------------------------------------------
        // Deserialize HTTP response directly into JSON doc
        //--------------------------------------------------

        DeserializationError error = deserializeJson( server_doc, http.getStream() );  // Deserialize JSON from HTTP response stream to JsonDocument

        http.end();  // Close HTTP connection after JSON processing is complete

         if ( error == DeserializationError::Ok )
        {
            
            char buff[512];

            snprintf(
                buff,
                sizeof(buff),
                "[SETTINGS] Server doc: threshold=%.1f duration=%lu minPoP=%.1f updated=%s",
                server_doc["moisture_threshold"] | 0.0,
                server_doc["watering_duration"] | 0UL,
                server_doc["min_precip_prob"] | 0.0,
                server_doc["updated"] | ""
            );

            logToFile( buff, debuglogFileName );

            break;  // break out of loop if everything worked
        }

        else
        {

            char buff[512];  // Message buffer
                
            snprintf ( buff, sizeof ( buff ), "[SETTINGS] Server JSON parse failed: %s", error.c_str() );  // Build message
                
            logToFile ( buff, errorlogFileName );  // Log to error file

            display_message ( buff, 2000 );

#ifdef DEBUG_ENABLED

            BGf("[SETTINGS] JSON parse failed: %s", error.c_str());  // Print debug statement
#endif

            }

    }  // end for loop

    
    //--------------------------------------------------
    // Determine whether server has newer settings
    //--------------------------------------------------

    if ( serverSettingsAreNewer( server_doc["updated"] | "" ) )  // Compare server timestamp with local timestamp
    {

        applyDownloadedSettings( server_doc );  // Apply new settings from server
       
    } 

    return true;  // Return true on success 

}


bool serverSettingsAreNewer( const char *serverUpdated )
{
    // Log timestamps to erro file

    char buff[256];

    snprintf(
        buff,
        sizeof(buff),
        "[SETTINGS] Compare: server=%s local=%s",
        serverUpdated,
        settings.updated
    );

    logToFile( buff, debuglogFileName );
    
    
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

    // Log timestamp comparison
    int comparison = strcmp( serverUpdated, settings.updated );

    snprintf(
        buff,
        sizeof(buff),
        "[SETTINGS] Timestamp comparison=%d",
        comparison
    );

    logToFile( buff, debuglogFileName );

    return comparison > 0;
    
    
    //return strcmp( serverUpdated, settings.updated ) > 0;  // Return true if server timestamp is newer than local timestamp

}


bool applyLocalSettings()
{
    JsonDocument local_doc;

    File file = LittleFS.open( "/irrigation_settings.json", FILE_READ );  // Open settings file for reading

    if ( !file )
    {
        logToFile ( "[FS] Couldn't open irrigation_settings.json", errorlogFileName );

        return false;  // Return false if local settings file can't be opened
    }

    DeserializationError error =  deserializeJson( local_doc, file );

    if ( error != DeserializationError::Ok )  // Deserialize JSON from file into document
    {
        char buff[512];

        snprintf ( buff, sizeof ( buff ), "[SETTINGS] irrigation_settings.json parse failed: %s", error.c_str() );  // Build message

        logToFile ( buff, errorlogFileName );

        file.close();  // Close file

        return false; // Return false on deserialization error

    }

    file.close();

    settings.moisture_threshold = local_doc["moisture_threshold"] | settings.moisture_threshold;
    settings.watering_duration_sec  = local_doc["watering_duration"]  | settings.watering_duration_sec ;
    settings.min_precip_prob = local_doc["min_precip_prob"]    | settings.min_precip_prob ;

    JsonArray times = local_doc["times"];

    for ( uint8_t i = 0; i < SCHEDULE_COUNT && i < times.size(); ++i )
    {
        unsigned int h, m, s;

        sscanf( times[i], "%u:%u:%u", &h, &m, &s);

        settings.times[i].hour = h;
        settings.times[i].min  = m;
        settings.times[i].sec  = s;
    }

    strlcpy( settings.updated, local_doc["updated"] |  settings.updated, sizeof( settings.updated ) );

#ifdef DEBUG_ENABLED

    DBG(F("[SETTINGS] Local settings are applied"));
    
#endif

    logSettings( "Local" ); // Log settings after applying local json values

    return true;
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

    for ( uint8_t i = 0; i < SCHEDULE_COUNT && i < times.size(); i++ )  // Loop through each schedule slot, up to SCHEDULE_COUNT or the size of the JSON array
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

    strlcpy( settings.updated, server_doc["updated"] | settings.updated, sizeof( settings.updated ) );  // Copy server timestamp to local settings, default to empty string if not present

    logSettings( "Server" );  // Log settings after updating from server file


    //--------------------------------------------------
    // Save updated settings locally
    //--------------------------------------------------

    if ( saveLocalSettings( server_doc ) == false )  // Check if saving settings to FS was successful
    {

       logToFile ( "[FS] Failed to save updated server settings to local file", errorlogFileName );

#ifdef DEBUG_ENABLED
        DBG(F("[SETTINGS] Failed to save local settings"));
#endif

    }

    else
    {

       logToFile( "[FS] Successfully saved updated server settings to local file", debuglogFileName );

#ifdef DEBUG_ENABLED
        DBG(F("[SETTINGS] New settings saved locally"));
#endif

    }

}


bool saveLocalSettings( JsonDocument &server_doc )  // Save settings to FS
{

    File file = LittleFS.open("/irrigation_settings.json", FILE_WRITE );  // Open file for writing

    if (!file)  // Check if file opened successfully
    {

#ifdef DEBUG_ENABLED

        DBG(F("[FILESYSTEM] Failed to open settings file"));

#endif
        return false;  // Return false on failure
    }

    if ( serializeJsonPretty( server_doc, file ) > 0 )  // Save JSON to file
    {

        file.close();  // Close file

        return true;  // Return true on success

    }

    else
    {

        file.close();  // Close file

#ifdef DEBUG_ENABLED

        serializeJsonPretty( server_doc, Serial );

#endif

        return false;  // Return false if no bytes written to file

    }

}


void logSettings( const char *source )
{

    char buff[512];

    snprintf(
        buff,
        sizeof(buff),
        "[SETTINGS] %s: threshold=%.1f duration=%lu minPoP=%.1f updated=%s",
        source,
        settings.moisture_threshold,
        settings.watering_duration_sec,
        settings.min_precip_prob,
        settings.updated
        );

   logToFile( buff, debuglogFileName );

}


void solenoid_state_Update()  // Report solenoid state to server
{

    // Check WiFi connection status
    if ( WiFi.status() == WL_CONNECTED )
    {

        WiFiClient client;

        HTTPClient http;

        char buff[256];

        // --------------------------------------------------
        // Create status message
        // --------------------------------------------------

        status.status_str = String("Watering ") + String( status.solenoid_state ? "started." : "stopped." );


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
            "zone=" + String( IRRIGATION_ZONE )
            + "&solenoid_state=" + String( status.solenoid_state ? 1 : 0 )
            + "&WiFi_RSSI=" + String( WiFi_RSSI )
            + "&status_message=" + encodedStatus
            + "&time_stamp=" + Timestamp( "%Y-%m-%d %H:%M:%S" );


        // Specify content type
        http.addHeader( F("Content-Type"), F("application/x-www-form-urlencoded" ) );

        const String url = String( phpServerName ) + String ( "post-esp-data.php" );

        for ( uint8_t i = 0; i < MAX_TRIES; ++i )
        {

            http.begin( client, url );  // Specify destination

            int httpResponseCode = http.POST( httpRequestData );  // Send HTTP POST request and get response code

            sprintf( buff, "[IRRIGATION] DB solenoid state POST failed. HTTP code: %d", httpResponseCode );  // Build OLED message

            display_message( buff, 2000 );  // Show OLED message

            http.end();  // Close HTTP connection

            if ( httpResponseCode == HTTP_CODE_OK )  // Check HTTP post response code
            {
                break;  // End loop if we successfully posted an update
            }

            else
            {
               logToFile( buff, errorlogFileName );
            }
       
        }

    }

    else
    {
       logToFile( "[WIFI] Disconnected", errorlogFileName );

        display_message( "[WIFI] Disconnected", 2000 );
    }

}


void sendServerUpdate()
{

    if ( WiFi.status() == WL_CONNECTED )   // Check WiFi connection status
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
        
        String encodedStatus = urlEncode( status.status_str );


        String httpRequestData =
            "zone=" + String( IRRIGATION_ZONE )
            + "&moisture_wvc=" + String( soil.moisture, 1 )
            + "&temperature=" + String( soil.temp, 1 )
            + "&ec=" + String( soil.ec )
            + "&ph=" + String( soil.pH, 1 )
            + "&nitrogen=" + String( soil.N )
            + "&potassium=" + String( soil.K )
            + "&phosphorus=" + String( soil.P )
            + "&solenoid_state=" + String( status.solenoid_state ? 1 : 0 )
            + "&average_PoP=" + String( avg_precip_prob, 1 )
            + "&WiFi_RSSI=" + String( WiFi_RSSI )
            + "&status_message=" + encodedStatus
            + "&time_stamp=" + Timestamp("%Y-%m-%d %H:%M:%S");


        // Specify content type
        http.addHeader( F("Content-Type"), F("application/x-www-form-urlencoded") );

        const String url = String( phpServerName ) + String ( "post-esp-data.php" );

        for( uint8_t i = 0; i < MAX_TRIES; ++i )
        {
            http.begin( client, url );  // Specify destination

            int httpResponseCode = http.POST( httpRequestData );  // Send POST data and receive response

            http.end();  // Close HTTP connection

#ifdef DEBUG_ENABLED

            DBGf("DB POST HTTP code: %d\r\n", httpResponseCode );


#endif

            if ( httpResponseCode == HTTP_CODE_OK )  // Check HTTP post response code
            {
                break;  // End loop if we successfully posted an update
            }

            else
            {
                sprintf( buff, "[IRRIGATION] DB soil readings POST failed. HTTP code: %d", httpResponseCode );  // Build OLED message

                logToFile( buff, errorlogFileName );
                
                display_message( buff, 2000 );  // Show OLED message
            }

        }

    }

    else
    {
        logToFile( "[WIFI] Disconnected", errorlogFileName );

        display_message( "[WIFI] Disconnected", 2000 );
    }

}


void logToFile( const char* text, const char* fileName )
{

  File file = LittleFS.open( fileName, FILE_APPEND );  // Open file for appending

  if ( !file )
  {
      Serial.println( F( "[FS] Failed to open error log for appending" ) ); // Report error if file can't open
      return;
  }

  file.print( Timestamp( "%m-%d-%Y %H:%M:%S" ).c_str() );  // Add timestamp to error file entry

  file.print( " - " );  // Add separator
  
  file.println( text );  // Error message text written to file

  file.close();  // Close error log file

}


bool uploadFile( const char* fileName )
{

    // Make sure the log file exists and contains something
    if ( LittleFS.exists( fileName ) == false )
    {
        Serial.println( "No file to upload" );
        return true;
    }

    File file = LittleFS.open( fileName, FILE_READ );

    if ( !file )
    {
        Serial.printf( "Failed to open file:  %s\r\n", fileName );
        return false;
    }

    WiFiClient client;
    HTTPClient http;

    const String url = String( phpServerName ) + String ( "post-log.php" );

    http.begin( client, url );
    http.addHeader( "X-Log-File", fileName );
    http.addHeader( "Content-Type", "text/plain" );

    // Send the LittleFS file directly
    int httpCode = http.sendRequest( "POST", &file, file.size() );

    file.close();

    bool uploadSuccessful = false;

    if ( httpCode == HTTP_CODE_OK )
    {
        String response = http.getString();

        Serial.printf( "File upload response: %s\n", response.c_str() );

        // Server must explicitly confirm receipt
        if ( response == "OK" )
        {
            uploadSuccessful = true;
        }

        else
        {
            Serial.println( "Server did not confirm file receipt" );
        }
    }

    else
    {
        char buff[256];

        snprintf ( buff, 
                   sizeof ( buff ), 
                   "[FS] %s upload failed. HTTP code: %d => %s",
                   fileName, httpCode, http.errorToString( httpCode ).c_str() );  // Build message

        logToFile ( buff, errorlogFileName );
        
        Serial.printf( buff );

    }

    http.end();

    // Only delete the local log after confirmed receipt
    if ( uploadSuccessful )
    {
        
        char buff[256];

        snprintf ( buff, sizeof ( buff ), "[FS] %s upload failed.", fileName );  // Build message

        if ( LittleFS.remove( fileName ) )
        {
            snprintf ( buff, sizeof ( buff ), "[FS] %s successfully uploaded and deleted\r\n", fileName );  // Build message
            Serial.printf( buff );
            display_message( buff , 2000 );
        }
        else
        {
            snprintf ( buff, sizeof ( buff ), "[FS] %s uploaded, but failed to delete local file\r\n", fileName );  // Build message
            Serial.printf(  buff, fileName );
            display_message( buff , 2000 );
        }

        return true;
    }

    else
    {

    Serial.printf( "%s file retained because upload was not confirmed\r\n", fileName );

    return false;

    }

}