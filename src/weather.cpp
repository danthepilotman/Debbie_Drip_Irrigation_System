/* Leftover for Reference if needed later:

const char* LAT = "29.524";   // Debbie's House latitude
const char* LON = "-81.205";  // Debbie's House longitude

const char* LAT = "28.027";   // My house latitude
const char* LON = "-80.631";  // My house longitude
*/

#include "weather.h"       // weather helpers and forecast parsing
#include "update_OLED.h"   // for display_message() function
#include "helper.h"        // for getForecastTimes() function
#include "sleep_timer.h"   // for nextTargetTime() function
#include "LAMP_Server.h"  // For error logging
#include <time.h>          // for time library functions


int precip_prob[24];   // NWS hourly forecast precipitation probability [%]

float avg_precip_prob = -3;   // Store average PoP

uint8_t valid_hourly_PoP_count = 0;


// ==================================================
// Get NWS hourly forecast
// ==================================================

bool getNWSForecast( JsonDocument &doc )
{

#ifdef DEBUG_ENABLED

    DBG( F( "[WX] Getting NWS forecast" ) );

#endif

    display_message( "[WX] Getting NWS forecast\r\n" );

#ifdef DEBBIE_HOUSE

    const char url[] = "https://api.weather.gov/gridpoints/JAX/86,31/forecast/hourly";

#else

    const char url[] = "https://api.weather.gov/gridpoints/MLB/57,49/forecast/hourly";

#endif

    // --------------------------------------------------
    // JSON FILTER
    // --------------------------------------------------

    JsonDocument filter;  // Create filter object

    filter["properties"]["periods"][0]["startTime"] = true;  // Define startTime filter

    filter["properties"]["periods"][0]["probabilityOfPrecipitation"]["value"] = true;  // Define PoP filter

    // --------------------------------------------------
    // HTTP SETUP
    // --------------------------------------------------

    WiFiClientSecure client;

    client.setInsecure();

    HTTPClient http;

    http.setTimeout( 10000 );

    http.useHTTP10( true );

    http.setUserAgent( F("ESP32 Irrigation Controller") );

    http.addHeader( F( "Accept" ), F( "application/geo+json" ) );

    if ( http.begin( client, url ) == false )
    {
        logError ( "[WX] HTTP begin failed" );
    }

    // --------------------------------------------------
    // HTTP GET
    // --------------------------------------------------

    int code = http.GET();

    // --------------------------------------------------
    // STREAM DESERIALIZE
    // --------------------------------------------------

    DeserializationError err = deserializeJson( doc, http.getStream(), DeserializationOption::Filter( filter ) );


    http.end();  // Kill http connection once Json is deserialized

    
#ifdef DEBUG_ENABLED

    DBGf( "[WX] HTTP code: %d\r\n", code );

#endif

    char buff[256];

    snprintf( buff, sizeof( buff ), "[WX] HTTP code: %d\r\n", code );

    display_message( buff );

    if ( code != HTTP_CODE_OK )
    {
    
        if ( code > 0 )
        {

            logError( buff );

        }

        else
        {

           snprintf( buff, sizeof( buff ), "[WX] HTTP GET failed: %s", http.errorToString( code ).c_str() ); 

           logError( buff );
           
        }

#ifdef DEBUG_ENABLED

        DBG( F( "[WX] HTTP request failed" ) );

#endif

        display_message( buff, 2000 );  // Show error message on OLED

        return false;  // Return false since http request was NOT successful
    }

   
    if ( err != DeserializationError::Ok )
    {

        snprintf( buff, sizeof( buff ), "[WX] JSON parse failed: %s", err.c_str() );

#ifdef DEBUG_ENABLED

        DBG( "[WX] JSON parse failed: %s\r\n", err.c_str() );

#endif

        logError ( buff );  // Log error message

        display_message( buff, 2000 );  // Display OLED error message

        return false;  // Return false since Json deserialization failed
    }


    return true;  // If you made it this far the NWS forecast PoP values were successfully obtained
}


// ==================================================
// Process forecast periods
// ==================================================

void processForecastPeriods( JsonArray filteredPeriods, time_t next_target, int &total, uint8_t &valid_count)
{
    uint8_t count = 0;

    total = 0;
    valid_count = 0;

    for ( JsonObject period : filteredPeriods )
    {
        if ( count >= MAX_FORECAST_HOURS )
            break;

        const char *startTime = period["startTime"].as<const char *>();

        if ( startTime == nullptr )
            continue;

        time_t forecast_time;
        time_t current_hour;

        if ( getForecastTimes( startTime, forecast_time, current_hour ) == false || forecast_time < current_hour )
        {
            continue;  // Skip missing or outdated forecasts
        }

        if ( forecast_time >= next_target )
            break;  // Break out of the loop once you encounter a forecast that is at or after the next target time

        int pop = period["probabilityOfPrecipitation"]["value"] | -4;

        if ( pop >= 0 )
        {
            total += pop;  // Only add current PoP if it is a positive value
            ++valid_count;  // Only increase the valid count if the current Pop is a positive value
        }

        ++count;  // Increment loop counter
    }

}



// ==================================================
// ============= WEATHER FORECAST ==================
// ==================================================

bool rainExpectedSoon()
{
    // --------------------------------------------------
    // Reset results from previous forecast
    // --------------------------------------------------

    valid_hourly_PoP_count = 0;  // Initialize valid_hourly_PoP_count

    avg_precip_prob = -3;  // Initialize  avg_precip_prob


    // --------------------------------------------------
    // Determine next scheduled watering target
    // --------------------------------------------------

    time_t next_target = nextTargetTime();


#ifdef DEBUG_ENABLED

    struct tm next_target_localTime;

    localtime_r( &next_target, &next_target_localTime );

    char next_target_buffer[32];

    strftime( next_target_buffer, sizeof(next_target_buffer), "%Y-%m-%d %H:%M:%S", &next_target_localTime );

    DBGf( "[TIME] Next target time: %s\r\n", next_target_buffer );

#endif


    // --------------------------------------------------
    // Get NWS forecast
    // --------------------------------------------------

    JsonDocument doc;

    for( uint8_t i = 0; i <= MAX_TRIES; ++i )
    {

        if( i == MAX_TRIES)  // Check if attempts have been exhausted
        {
            avg_precip_prob = -2;  // Set error code value

            logError ( "[WX] Could not obtain NWS forecast");

            return false;  // Retrun false since forecast was unavailable
        }
        
        if ( getNWSForecast( doc ) == true )
        {
            break;  // Break out of loop once forecast is successfully obtained
        }

        delay( 5000 );  // Wait between forecast fetch attempts

    }
    
    
    JsonArray filteredPeriods = doc["properties"]["periods"];


#ifdef DEBUG_ENABLED

    serializeJsonPretty( filteredPeriods, Serial );

    Serial.println();

    Serial.flush();

#endif

    // --------------------------------------------------
    // Process forecast periods
    // --------------------------------------------------

    int total = 0;
  
    processForecastPeriods( filteredPeriods, next_target, total, valid_hourly_PoP_count );

    if ( valid_hourly_PoP_count == 0 )
    {
        avg_precip_prob = -1;

        logError( "[WX] No forecast periods found" );

#ifdef DEBUG_ENABLED

        DBG( F( "[WX] No forecast periods found" ) );

#endif

        display_message( "[WX] No forecast periods found", 2000 );

        return false;  // Return false if we couldn't retrieve any valid hourly PoP values
    }

    else
    {
        avg_precip_prob = float( total ) / valid_hourly_PoP_count;
    }

    
#ifdef DEBUG_ENABLED

    DBGf( "[WX] Average PoP: %.1f%%\r\n", avg_precip_prob );

#endif


    return ( avg_precip_prob >= settings.min_precip_prob );  // Check precipitation threshold

}