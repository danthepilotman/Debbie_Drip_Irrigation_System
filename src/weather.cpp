#include "weather.h"  // weather helpers and forecast parsing
#include "update_OLED.h"


/* Leftover for Reference if needed later:

const char* LAT = "29.524";   // Debbie's House latitude
const char* LON = "-81.205";  // Debbie's House longitude

const char* LAT = "28.027";   // My house latitude
const char* LON = "-80.631";  // My house longitude
*/


volatile int precip_prob[6] = {-1, -1, -1, -1, -1, -1};  // Store precit probability values


// ==================================================
// ============= WEATHER Forecast ===================
// ==================================================
bool rainExpectedSoon()
{
    
    bool rain_expected = false;

    #ifdef DEBUG_ENABLED

    DBG( F( "[WEATHER] Checking forecast" ) );  // User debug message

#endif

    display_message( "[WEATHER] Checking forecast\r\n" );

#ifdef DEBBIE_HOUSE

    char url[] = "https://api.weather.gov/gridpoints/JAX/86,33/forecast/hourly";  // buffer for constructed URL   
    
#else

    char url[] = "https://api.weather.gov/gridpoints/MLB/25,69/forecast/hourly";  // buffer for constructed URL

#endif

    WiFiClientSecure client;
    client.setInsecure();  // you’re already doing this elsewhere    
    HTTPClient http;  // HTTP client for OpenWeather requests
    http.setTimeout(10000);  // 10 seconds
    http.useHTTP10(true);
    http.begin( client, url );  // start the HTTP session using URL
    http.addHeader("User-Agent", "ESP32_Irrigation_Controller");
    http.addHeader("Accept", "application/geo+json");

    int code = http.GET();  // Use HTTP GET to fetch the weather data

#ifdef DEBUG_ENABLED

    DBGf( "[WEATHER] HTTP code: %d\r\n", code );  // Print HTTP response code

#endif
    char buff[256];
    sprintf( buff, "[WEATHER] HTTP code: %d\r\n", code);
    display_message(buff);

    if ( code != HTTP_CODE_OK )
    {

        http.end();

#ifdef DEBUG_ENABLED  
  
        DBG( F( "[WEATHER] HTTP request failed" ) );  // User debug message

 #endif 
        
        display_message( "[WEATHER] HTTP request failed", 2000);
        
        return false; // If the HTTP request failed, assume no rain expected

    }
    
    
    // -----------------------------
    // JSON filter
    // -----------------------------
    JsonDocument filter;

    filter["properties"]["periods"][0]["probabilityOfPrecipitation"]["value"] = true;

    // -----------------------------
    // Stream-deserialize
    // -----------------------------
    JsonDocument doc; // automatic allocation

    DeserializationError err = deserializeJson( doc, http.getStream(), DeserializationOption::Filter( filter ) );

    http.end();

    if ( err )  // Check for JSON errors
    {
        
#ifdef DEBUG_ENABLED
        
        DBGf( "JSON parse failed: %s\r\n", err.c_str()  );  // User debug message

#endif        
        
        sprintf(buff, "JSON parse failed: %s\r\n", err.c_str() );
        display_message(buff, 2000);
        return false; // If JSON parsing failed, assume no rain expected

    }
    
   
    // -----------------------------
    // Extract PoP values
    // -----------------------------
    JsonArray filteredPeriods = doc["properties"]["periods"];

    serializeJsonPretty(  filteredPeriods, Serial );
  
    int count = 0;
    
    for ( JsonObject period : filteredPeriods )
    {
        
        if ( count > 5 )
            break; // limit to first 6 periods
        
        precip_prob[count] = period["probabilityOfPrecipitation"]["value"] | -1;
      
#ifdef DEBUG_ENABLED

        DBGf("[WEATHER] Pop: %d\r\n", precip_prob);

#endif

        if ( precip_prob[count] >= RAIN_PROB_MIN )
            rain_expected = true;

        ++count;
           
    }

  
    doc.clear();  // release parsed data
    filter.clear();  // clear temporary filter doc
    
    return rain_expected;  // If you made it past the for loop without finding any precip then no rain is expected

}