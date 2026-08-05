#include "weather.h"  // weather helpers and forecast parsing
#include "update_OLED.h"  // for display_mesage() function
#include "helper.h"  // for getForecastHour() function
#include "sleep_timer.h"  // for next_watering_time() function
#include <time.h>  // for time library functions


/* Leftover for Reference if needed later:

const char* LAT = "29.524";   // Debbie's House latitude
const char* LON = "-81.205";  // Debbie's House longitude

const char* LAT = "28.027";   // My house latitude
const char* LON = "-80.631";  // My house longitude
*/


volatile int precip_prob[24];  // NWS hourly forecast precipitation probability values for the next 24 hours (0-23). Values are in percentage [%]

constexpr uint8_t NUM_FORECAST_HOURS = sizeof( precip_prob ) / sizeof( precip_prob[0] );  // Number of forecast hours to store in precip_prob array

float avg_precip_prob = 0;  // Store average PoP

uint8_t valid_hourly_PoP_count = 0;  // Count how many valid hourly forecast periods are found


// ==================================================
// ============= WEATHER Forecast ===================
// ==================================================
bool rainExpectedSoon()
{
    bool rain_expected = false;


#ifdef DEBUG_ENABLED

    DBG( F( "[WEATHER] Checking forecast" ) );

#endif

    display_message( "[WEATHER] Checking forecast\r\n" );


#ifdef DEBBIE_HOUSE

    char url[] = "https://api.weather.gov/gridpoints/JAX/86,33/forecast/hourly";

#else

    char url[] = "https://api.weather.gov/gridpoints/MLB/25,69/forecast/hourly";

#endif


    // ==================================================
    // HTTP SETUP
    // ==================================================

    WiFiClientSecure client;

    client.setInsecure();

    HTTPClient http;

    http.setTimeout( 10000 );

    http.useHTTP10( true );

    http.begin( client, url );

    http.addHeader( "User-Agent", "ESP32_Irrigation_Controller" );

    http.addHeader( "Accept", "application/geo+json" );


    int code = http.GET();  // HTTP GET


#ifdef DEBUG_ENABLED

    DBGf( "[WEATHER] HTTP code: %d\r\n", code );

#endif


    char buff[256];

    sprintf( buff, "[WEATHER] HTTP code: %d\r\n", code );

    display_message( buff );


    if ( code != HTTP_CODE_OK )
    {
        http.end();

#ifdef DEBUG_ENABLED

        DBG( F( "[WEATHER] HTTP request failed" ) );

#endif

        display_message( "[WEATHER] HTTP request failed", 2000 );

        return false;
    }


    time_t next_target = nextTargetTime();     // DETERMINE NEXT TARGET

#ifdef DEBUG_ENABLED

        struct tm next_target_localTime;

        localtime_r( &next_target, &next_target_localTime );

        char next_target_buffer[32];

        strftime(next_target_buffer, sizeof(next_target_buffer), "%Y-%m-%d %H:%M:%S", &next_target_localTime);

        DBGf("[TIME} Next target time: %s\r\n", next_target_buffer );

#endif


    // ==================================================
    // JSON FILTERS
    // ==================================================

    JsonDocument filter;  // Create JSON filer document 

    filter["properties"]["periods"][0]["startTime"] = true;  // Create timestamp filer

    filter["properties"]["periods"][0]["probabilityOfPrecipitation"]["value"] = true;  // Create POP filter


    // ==================================================
    // STREAM-DESERIALIZE
    // ==================================================

    JsonDocument doc;  // Create JSON document for storing filtered forecast data

    // Deserialize JSON from HTTP response stream to JsonDocument with filtering
    DeserializationError err = deserializeJson( doc, http.getStream(), DeserializationOption::Filter( filter ) );  

    http.end();  // Close HTTP connection and free resources


    if ( err )
    {

#ifdef DEBUG_ENABLED

        DBGf( "JSON parse failed: %s\r\n", err.c_str() );

#endif

        sprintf( buff, "JSON parse failed: %s\r\n", err.c_str() );

        display_message( buff, 2000 );

        return false;
    }


    JsonArray filteredPeriods = doc["properties"]["periods"];  // FILTER FORECAST PERIODS


#ifndef DEBUG_ENABLED

    serializeJsonPretty( filteredPeriods, Serial );
    Serial.println();
    Serial.flush();

#endif


/********************************* PROCESS FORECAST PERIODS *************************************/

    for ( JsonObject period : filteredPeriods )
    {

        if ( valid_hourly_PoP_count >= NUM_FORECAST_HOURS )
            break;

        // ----------------------------------------------
        // Get NWS startTime
        // ----------------------------------------------

        const char *startTime = period["startTime"].as<const char *>();

        if ( startTime == nullptr )
        {
            continue;
        }
     
        
        /*************************** Determine forecast time and current hour *****************************/

        time_t forecast_time;
        time_t current_hour;

        if ( getForecastTimes( startTime, forecast_time, current_hour ) == false )
        {
            continue;
        }

        if ( forecast_time < current_hour )  // Skip forecast periods that have already ended.
        {
            continue;
        }

        if ( forecast_time >= next_target )  // Stop at the next scheduled target.
        {
            break;
        }

        precip_prob[valid_hourly_PoP_count] = period["probabilityOfPrecipitation"]["value"] | -1; // Store POP values


#ifdef DEBUG_ENABLED

        DBGf( "[WEATHER] %s  PoP=%d%%\r\n", startTime, precip_prob[valid_hourly_PoP_count] );

#endif

        ++valid_hourly_PoP_count;  // Increment valid hourly forecast count

    }


    /************************* CLEAN UP JSON ***************************/
  
    doc.clear();

    filter.clear();

    /********************* CHECK FOR FORECAST DATA *********************/


    if ( valid_hourly_PoP_count == 0 )
    {
#ifdef DEBUG_ENABLED

        DBG( F( "[WEATHER] No forecast periods found" ) );

#endif

        display_message( "[WEATHER] No forecast periods found", 2000 );

        return false;
    }


    /****************************** CALCULATE AVERAGE PoP ******************************/
  
    for ( uint8_t i = 0; i < valid_hourly_PoP_count; ++i )  // Add up all Pop values
    {
        if ( precip_prob[i] >= 0 )
        {
            avg_precip_prob += precip_prob[i];
        }
    }

    avg_precip_prob /= valid_hourly_PoP_count;  // Divide by number of valid forecast periods to get average


#ifdef DEBUG_ENABLED

    DBGf( "[WEATHER] Average PoP: %.1f%%\r\n", avg_precip_prob );

#endif


    // ==================================================
    // CHECK RAIN THRESHOLD
    // ==================================================

    if ( avg_precip_prob >= settings.min_precip_prob )
    {
        rain_expected = true;
    }


    return rain_expected;
}