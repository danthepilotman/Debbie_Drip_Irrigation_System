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
    
#ifdef DEBUG_ENABLED

    DBG( F( "[WEATHER] Checking forecast" ) );  // User debug message

#endif

    HTTPClient http;  // HTTP client for OpenWeather requests

    char url[] = "https://api.weather.gov/gridpoints/JAX/86,33/forecast/hourly";  // buffer for constructed URL

    JsonDocument filter;  // Create JSON filter document

    // Apply the same filter to ALL period elements
    for ( uint8_t i = 0; i < 6; ++i )
    {
        filter["properties"]["periods"][i]["probabilityOfPrecipitation"]["value"] = true;
    }

    filter["properties"]["periods"] = true;  // <-- important

    http.begin( url );  // start the HTTP session using URL
    http.addHeader("User-Agent", "ESP32_Irrigation_Controller");
    http.addHeader("Accept", "application/geo+json");

    int code = http.GET();  // Use HTTP GET to fetch the weather data

#ifdef DEBUG_ENABLED

    DBGf( "[WEATHER] HTTP code: %d\r\n", code );  // Print HTTP response code

#endif

    // if ( code != HTTP_CODE_OK )  // Check for errors
    //     return false;  // Return no rain expected
    
   String payload = http.getString();

    Serial.println("**** RAW JSON ****");
    Serial.println(payload);

    // Stream-deserialize using the filter to reduce memory usage
    DeserializationError err = deserializeJson( doc, payload, DeserializationOption::Filter(filter) );

    http.end();  // End HTTP session after consuming the stream


    if ( err )  // Check for JSON errors
    {

#ifdef DEBUG_ENABLED

        DBG( F( "[WEATHER][ERROR] JSON parse failed" ) );

#endif
        return false;
    }

    
    Serial.print("Doc size: ");
    Serial.println(doc.size());

    JsonArray periods = doc["properties"]["periods"];

    Serial.print("Periods size: ");
    Serial.println(periods.size());
    

    Serial.println("**** FILTERED JSON ****");
    serializeJsonPretty(doc, Serial);
    Serial.println();


    for (JsonObject period : periods)
    {
        int precip_prob = period["probabilityOfPrecipitation"]["value"] | 0;
        Serial.println(precip_prob);
    }

    uint8_t count = 0;

    for (JsonObject period : periods)
    {
        if (count++ >= 6)
            break;

        int precip_prob = period["probabilityOfPrecipitation"]["value"] | 0;

#ifdef DEBUG_ENABLED

        DBGf("[WEATHER] Pop: %d\r\n", precip_prob);

#endif

        if (precip_prob > rain_prob_min)
            return true;
    }

    doc.clear();  // release parsed data
    filter.clear();  // clear temporary filter doc
    
    return false;  // If you made it past the for loop without finding any precip then no rain is expected

}