#include "weather.h"  // weather helpers and forecast parsing


const char* WEATHER_API_KEY = "1f237060a56d83d3827815039317d2a9";  // OpenWeatherMap API key

#ifdef DEBBIE_HOUSE

const char* LAT = "29.524";  // Debbie's House latitude
const char* LON = "-81.205";  // 

#else

const char* LAT = "28.027";   // My house latitude
const char* LON = "-80.631";  // My house longitude

#endif


// ==================================================
// ================= WEATHER ========================
// ==================================================
bool rainExpectedSoon()
{
    DBG( F( "[WEATHER] Checking forecast" ) );  // User debug message

    HTTPClient http;  // HTTP client for OpenWeather requests

    char url[256];  // buffer for constructed URL

    snprintf(url, sizeof(url),
             "https://api.openweathermap.org/data/3.0/onecall?lat=%s&lon=%s&exclude=current,daily,minutely,alerts&appid=%s",
             LAT, LON, WEATHER_API_KEY);  // Build URL string

    http.begin( url );  // start the HTTP session using URL

    int code = http.GET();  // Use HTTP GET to fetch the weather data

    DBGf( "[WEATHER] HTTP code: %d\r\n", code );  // Print HTTP response code

    if ( code != HTTP_CODE_OK )  // Check for errors
    {
        http.end();  // End HTTP session
        return false;  // Return no rain expected
    }
    

    JsonDocument filter;  // Create JSON filter document

    // Apply the same filter to ALL hourly elements
    JsonObject hourly = filter["hourly"][0].to<JsonObject>();  // Create Json object for hourly filter

    hourly["pop"] = true;  // Probability of precipitation
    hourly["weather"][0]["main"] = true;  // Main weather description

    // Stream-deserialize using the filter to reduce memory usage
    DeserializationError err = deserializeJson( doc, http.getStream(), DeserializationOption::Filter(filter) );

    http.end();  // End HTTP session after consuming the stream


    if ( err )  // Check for JSON errors
    {
        DBG( F( "[WEATHER][ERROR] JSON parse failed" ) );
        return false;
    }

    uint8_t checks = 0;  // limit checks to next 6 hours

    for ( JsonObject item : doc["hourly"].as<JsonArray>() )  // Loop through the forecast JSON doc
    {
        if ( checks++ > 5 )  // next 6 hours since forecast are hourly
            break; 

        String main = item["weather"][0]["main"];  // main description (Rain/Drizzle/etc)

        String pop = item["pop"];  // probability (as string)

        float precip_prob = pop.toFloat();  // numeric probability value
       
        DBGf( "[WEATHER] Forecast: %s\tPop: %.2f\r\n", main.c_str(),  precip_prob );  // Print main forecast

        if ( ( main == "Rain" || main == "Drizzle" || main == "Thunderstorm" ) && precip_prob > rain_prob_min )  // Check for "rain" events
            return true;
        
    }

    doc.clear();  // release parsed data
    filter.clear();  // clear temporary filter doc
    
    return false;  // If you made it past the for loop without finding any precip then no rain is expected

}
