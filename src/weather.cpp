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

uint8_t processForecastPeriods( JsonArray filteredPeriods, time_t next_target, int &total, uint8_t &valid_count)
{
    uint8_t count = 0;

    total = 0;
    valid_count = 0;

    for (JsonObject period : filteredPeriods)
    {
        if (count >= MAX_FORECAST_HOURS)
            break;

        const char *startTime =
            period["startTime"].as<const char *>();

        if (startTime == nullptr)
            continue;

        time_t forecast_time;
        time_t current_hour;

        if (getForecastTimes(
                startTime,
                forecast_time,
                current_hour) == false ||
            forecast_time < current_hour)
        {
            continue;
        }

        if (forecast_time >= next_target)
            break;

        int pop =
            period["probabilityOfPrecipitation"]["value"] | -1;

        if (pop >= 0)
        {
            total += pop;
            ++valid_count;
        }

        ++count;
    }

    return count;
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

        if( i == MAX_TRIES)
        {
            avg_precip_prob = -2;
            return false;
        }
        
        if ( getNWSForecast( doc ) == true )
        {
            break;
        }

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

#ifdef DEBUG_ENABLED

        DBG( F( "[WEATHER] No forecast periods found" ) );

#endif

        display_message( "[WEATHER] No forecast periods found", 2000 );
    }

    else
    {
        avg_precip_prob = float(total) / valid_hourly_PoP_count;
    }

    
#ifdef DEBUG_ENABLED

    DBGf( "[WEATHER] Average PoP: %.1f%%\r\n", avg_precip_prob );

#endif


    return avg_precip_prob >= settings.min_precip_prob;  // Check precipitation threshold

}