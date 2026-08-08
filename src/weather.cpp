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
#include <time.h>          // for time library functions


int precip_prob[24];   // NWS hourly forecast precipitation probability [%]

float avg_precip_prob = 0;   // Store average PoP

uint8_t valid_hourly_PoP_count = 0;


// ==================================================
// Get NWS hourly forecast
// ==================================================

bool getNWSForecast( JsonDocument &doc )
{

#ifdef DEBUG_ENABLED

    DBG( F( "[WEATHER] Getting NWS forecast" ) );

#endif

    display_message( "[WEATHER] Getting NWS forecast\r\n" );


#ifdef DEBBIE_HOUSE

    char url[] =
        "https://api.weather.gov/gridpoints/JAX/86,33/forecast/hourly";

#else

    char url[] =
        "https://api.weather.gov/gridpoints/MLB/25,69/forecast/hourly";

#endif


    // --------------------------------------------------
    // HTTP SETUP
    // --------------------------------------------------

    WiFiClientSecure client;

    client.setInsecure();

    HTTPClient http;

    http.setTimeout( 10000 );

    http.useHTTP10( true );

    http.begin( client, url );

    http.addHeader( F( "User-Agent" ), F( "ESP32_Irrigation_Controller" ) );

    http.addHeader( F( "Accept" ), F( "application/geo+json" ) );


    // --------------------------------------------------
    // HTTP GET
    // --------------------------------------------------

    int code = http.GET();


#ifdef DEBUG_ENABLED

    DBGf( "[WEATHER] HTTP code: %d\r\n", code );

#endif

    char buff[128];

    snprintf( buff, sizeof(buff), "[WEATHER] HTTP code: %d\r\n", code );

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


    // --------------------------------------------------
    // JSON FILTER
    // --------------------------------------------------

    JsonDocument filter;

    filter["properties"]["periods"][0]["startTime"] = true;

    filter["properties"]["periods"][0]["probabilityOfPrecipitation"]["value"] = true;


    // --------------------------------------------------
    // STREAM DESERIALIZE
    // --------------------------------------------------

    DeserializationError err = deserializeJson( doc, http.getStream(), DeserializationOption::Filter( filter ) );


    http.end();


    if ( err )
    {
#ifdef DEBUG_ENABLED

        DBGf( "[WEATHER] JSON parse failed: %s\r\n", err.c_str() );

#endif

        snprintf( buff, sizeof(buff), "JSON parse failed: %s\r\n", err.c_str() );

        display_message( buff, 2000 );

        return false;
    }


    return true;
}


// ==================================================
// Process forecast periods
// ==================================================

uint8_t processForecastPeriods( JsonArray filteredPeriods, time_t next_target )
{
    uint8_t count = 0;


    for ( JsonObject period : filteredPeriods )
    {
        if ( count >= MAX_FORECAST_HOURS )
        {
            break;
        }


        // --------------------------------------------------
        // Get NWS startTime
        // --------------------------------------------------

        const char *startTime = period["startTime"].as<const char *>();



        if ( startTime == nullptr )
        {
            continue;
        }


        // --------------------------------------------------
        // Determine forecast time and current hour
        // --------------------------------------------------

        time_t forecast_time;
        time_t current_hour;


        // --------------------------------------------------
        // Skip forecast periods that have already ended
        // --------------------------------------------------

        if ( getForecastTimes( startTime, forecast_time, current_hour ) == false || forecast_time < current_hour  )
        {
            continue;
        }


        // --------------------------------------------------
        // Stop at next scheduled watering target
        // --------------------------------------------------

        if ( forecast_time >= next_target )
        {
            break;
        }


        // --------------------------------------------------
        // Store PoP
        // --------------------------------------------------

        precip_prob[count] = period["probabilityOfPrecipitation"]["value"] | -1;


#ifdef DEBUG_ENABLED

        DBGf( "[WEATHER] %s  PoP=%d%%\r\n", startTime, precip_prob[count] );

#endif


        ++count;
    }


    return count;
}


// ==================================================
// Calculate average PoP
// ==================================================

float calculateAveragePoP( uint8_t count )
{
    if ( count == 0 )
    {
        return -1;
    }


    int total = 0;

    uint8_t valid_count = 0;


    for ( uint8_t i = 0; i < count; ++i )
    {
        if ( precip_prob[i] >= 0 )
        {
            total += precip_prob[i];

            ++valid_count;
        }
    }


    if ( valid_count == 0 )
    {
        return -1;
    }


    return float( total ) / valid_count;
}


// ==================================================
// ============= WEATHER FORECAST ==================
// ==================================================

bool rainExpectedSoon()
{
    // --------------------------------------------------
    // Reset results from previous forecast
    // --------------------------------------------------

    valid_hourly_PoP_count = 0;

    avg_precip_prob = -1;


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

    for( uint8_t i = 0; i <= MAX_TRIES; i++ )
    {

        if ( getNWSForecast( doc ) == true )
        {
            break;
        }

        if( i == MAX_TRIES)
        {
            return false;
        }

    }
    
    
    JsonArray filteredPeriods = doc["properties"]["periods"];


#ifndef DEBUG_ENABLED

    serializeJsonPretty( filteredPeriods, Serial );

    Serial.println();

    Serial.flush();

#endif


    // --------------------------------------------------
    // Process forecast periods
    // --------------------------------------------------

    valid_hourly_PoP_count = processForecastPeriods( filteredPeriods, next_target );


    // --------------------------------------------------
    // Check for forecast data
    // --------------------------------------------------

    if ( valid_hourly_PoP_count == 0 )
    {

#ifdef DEBUG_ENABLED

        DBG( F( "[WEATHER] No forecast periods found" ) );

#endif

        display_message( "[WEATHER] No forecast periods found", 2000 );

        return false;
    }


    // --------------------------------------------------
    // Calculate average PoP
    // --------------------------------------------------

    avg_precip_prob = calculateAveragePoP( valid_hourly_PoP_count );


#ifdef DEBUG_ENABLED

    DBGf( "[WEATHER] Average PoP: %.1f%%\r\n", avg_precip_prob );

#endif


    // --------------------------------------------------
    // Check precipitation threshold
    // --------------------------------------------------

     return avg_precip_prob >= settings.min_precip_prob;

}