#include "ota_update.h"  // associated header file
#include "update_OLED.h"  // for display_message() function
#include "setup.h"  // for debug macros, Json, WifiClientSecure, HTTPClient, Arduino OTA update library
#include <HTTPUpdate.h>  // For OTA update functionality
#include <esp_ota_ops.h>  // For OTA update state checking


const char *FIRMWARE_VERSION = "1.2.10";  // current firmware version


bool getFirmwareInfo( String &latestVersion, String &firmwareUrl )
{

    const char *MANIFEST_URL = "https://raw.githubusercontent.com/danthepilotman/Releases/main/Irrigation_System/manifest.json";
    
    HTTPClient http;

    http.begin( MANIFEST_URL);

    int code = http.GET();

    if ( code != HTTP_CODE_OK )
    {

#ifdef DEBUG_ENABLED

        DBG("Failed to fetch manifest");

#endif

        return false;
    }

    
    JsonDocument doc;  // Create JSON document for parsing TalkBack response
    
    DeserializationError err = deserializeJson( doc, http.getStream() );  // Deserialize JSON from HTTP response stream to JsonDocument

    http.end();

    if ( err )
    {
        DBG("JSON parse failed");
        return false;
    }

    latestVersion = doc["version"].as<String>();
    firmwareUrl   = doc["url"].as<String>();

    return true;
}


bool isNewerVersion( const String &latestVersion )
{
    int currentMajor;
    int currentMinor;
    int currentPatch;

    int latestMajor;
    int latestMinor;
    int latestPatch;


    //--------------------------------------------------
    // Parse current firmware version
    //--------------------------------------------------

    if (sscanf( FIRMWARE_VERSION, "%d.%d.%d", &currentMajor, &currentMinor, &currentPatch ) != 3 )
    {
        return false;
    }


    //--------------------------------------------------
    // Parse latest available version
    //--------------------------------------------------

    if ( sscanf( latestVersion.c_str(), "%d.%d.%d", &latestMajor, &latestMinor, &latestPatch ) != 3 )
    {
        return false;
    }


    // Major version
    if ( latestMajor > currentMajor )
        return true;

    if ( latestMajor < currentMajor )
        return false;


    // Minor version
    if ( latestMinor > currentMinor )
        return true;

    if ( latestMinor < currentMinor )
        return false;


    // Patch version
    return latestPatch > currentPatch;

}


void performOTA( String url )
{
    
    char buff[256];

    sprintf( buff, "Starting OTA from:\r\n%s", url.c_str() );

    display_message( buff );

    WiFiClientSecure client;
    client.setInsecure();  // skip certificate validation (OK for your use case)

    t_httpUpdate_return ret = httpUpdate.update(client, url);  // No cert, no client key

    httpUpdate.onEnd([]() {

#ifdef DEBUG_ENABLED

                DBG("OTA update successful, rebooting...");

#endif
            });

    switch ( ret )
    {
        case HTTP_UPDATE_FAILED:
            sprintf( buff, "OTA Failed: %s\n", httpUpdate.getLastErrorString().c_str() );
            display_message( buff, 2000 );
            break;

        case HTTP_UPDATE_NO_UPDATES:
            sprintf( buff, "No update available" );
            display_message( buff, 2000 );
            break;

        case HTTP_UPDATE_OK:
            sprintf( buff, "OTA Success !" );
            display_message( buff, 2000 );
            break;
    }

    httpUpdate.onEnd([]() {

#ifdef DEBUG_ENABLED

                DBG("OTA update successful, rebooting...");

#endif
    });
}


void checkForOTAUpdate()
{
    String latestVersion;
    String firmwareUrl;

    if ( getFirmwareInfo( latestVersion, firmwareUrl ) == false )
        return;

#ifdef DEBUG_ENABLED

    DBGf( "[FIRMWARE] Current: %s\r\n[FIRMWARE] Latest: %s\r\n", FIRMWARE_VERSION, latestVersion.c_str());

#endif


    char buff[256];

    sprintf( buff, "[FIRMWARE]\r\nCurrent:%s\r\nLatest: %s\r\n", FIRMWARE_VERSION, latestVersion.c_str() );

    display_message( buff, 2000 );
    

    if ( isNewerVersion( latestVersion ) )
    {

#ifdef DEBUG_ENABLED

        DBG( "[FIRMWARE] Update available!");

#endif

        display_message( "[FIRMWARE] Update available!\r\n" );
    
        performOTA( firmwareUrl );
    }
    
    else
    {

#ifdef DEBUG_ENABLED

        DBG( "[FIRMWARE] Firmware up to date." );

#endif

        display_message( "[FIRMWARE] Firmware up to date.\r\n", 1000 );
    }

}


void check_ota_state()
{
    const esp_partition_t* running = esp_ota_get_running_partition();

    esp_ota_img_states_t ota_state;

    esp_err_t err = esp_ota_get_state_partition( running, &ota_state );

    if ( err == ESP_OK )
    {
        if ( ota_state == ESP_OTA_IMG_PENDING_VERIFY )
        {

#ifdef DEBUG_ENABLED

            DBG("[OTA] Pending OTA firmware detected → confirming");

#endif

            display_message( "[OTA] Pending firmware detected\r\nConfirming..." );

            esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();

            if(err == ESP_OK)
            {

#ifdef DEBUG_ENABLED

                DBG("Firmware confirmed successfully");

#endif
          
                display_message("[OTA] Firmware confirmed successfully");
            }
            else
            {

#ifdef DEBUG_ENABLED

                DBGf( "[OTA] Failed to confirm firmware: %s\r\n", esp_err_to_name(err));

#endif
               
                char buff[256];
                sprintf(buff, "[OTA] Failed to confirm firmware:\r\n%s", esp_err_to_name(err));
                display_message(buff);
            }

        }
    }
}