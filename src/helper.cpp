#include "helper.h"  // helper functions and globals


String urlEncode( const String &input )  // URL-encode input
{
  
    String encoded = "";  // Encoded output string
    char c;  // Character being processed
    char buf[4];  // Buffer for percent-encoding

    for ( uint8_t i = 0; i < input.length(); ++i )  // Loop through each character in input string
    {
        c = input[i];  // Get current character

        // Unreserved characters according to RFC 3986
        if ( isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' )  // Check if character is unreserved
          encoded += c;  // Append unreserved character as-is
        
        else  // Character needs percent-encoding
        {
            // Percent-encode everything else
            snprintf( buf, sizeof( buf ), "%%%02X", (unsigned char)c );  // Format as %HH
            encoded += buf;  // Append percent-encoded string
        }
    }

    return encoded;  // Return URL-encoded string
}


String Timestamp()  // Return formatted current time
{
    struct tm timeinfo;  // Time structure

    char buf[32];  // Buffer to hold formatted timestamp

    getLocalTime( &timeinfo );  // Get local time

    strftime( buf, sizeof( buf ), "%a, %b %d, %Y %I:%M:%S %p", &timeinfo );  // Format time as "Day, Mon DD, YYYY HH:MM:SS AM/PM"

    return String( buf );  // Return formatted timestamp

}


void solenoid_state_Update()  // Report solenoid state to ThingSpeak
{

  char url[256];  // Char array to hold URL

  String status_str = urlEncode( String("Watering ") + String(solenoid_state ? "started " : "stopped ") + Timestamp() );  // URL encode status string

  char status_c[128];  // Char array to hold URL encoded status string

  HTTPClient http;  // Create HTTP client object


  status_str.toCharArray(status_c, sizeof(status_c));  // Convert to C-string

  // Build URL for ThingSpeak update
  snprintf( url, sizeof(url), "https://api.thingspeak.com/update?api_key=%s&field8=%d&status=%s", TS_WRITE_KEY, solenoid_state ? 1 : 0, status_c ); 

#ifdef DEBUG_ENABLED

  DBGf( "[IRRIGATION] Solenoid is now %s", solenoid_state ? "ON\r\n" : "OFF\r\n" );  // Print solenoid state
  Serial.printf( "[THINGSPEAK] URL: %s\r\n", url );  // Print URL being used

#endif
   
  for( uint8_t tries = 1; tries <= MAX_TRIES; tries++ )  // Try updating up to MAX_TRIES times
  {

    int code = HTTP_CODE_BAD_REQUEST;  // Store HTTP response codes. Initialized to bad code to force retry if needed

    if( code !=  HTTP_CODE_OK )  // Try if not getting a good HTTP response code
    {
      http.begin( url );  // Start HTTP session
        
      code = http.GET();  // Use GET to send HTTP update to TS and retrieve response code

      http.end(); // End HTTP session
    }

    Serial.printf("[THINGSPEAK] HTTP code: %d\r\n", code );  // Print HTTP response code

    if( code == HTTP_CODE_OK )  // Successful update
      break;  // Exit retry loop if successful

  }

}


time_t iso8601ToEpochUsingGmtime( const char* ts )  // Parse ISO8601 to epoch
{

    int Y, M, D, h, m, s;  // Time components

    if (sscanf(ts, "%4d-%2d-%2dT%2d:%2d:%2dZ", &Y, &M, &D, &h, &m, &s) != 6)  // Parse ISO 8601 timestamp
      return -1;   // Return error if parsing fails

    struct tm utc = {};  // UTC time structure
    utc.tm_year = Y - 1900;  // tm_year is years since 1900
    utc.tm_mon  = M - 1;  // tm_mon is 0-11
    utc.tm_mday = D;  // tm_mday is day of month
    utc.tm_hour = h;  // tm_hour is hours since midnight
    utc.tm_min  = m;  // tm_min is minutes after the hour
    utc.tm_sec  = s;  // tm_sec is seconds after the minute
    utc.tm_isdst = 0;  // Not considering daylight saving time


    // Get current UTC offset
    time_t now = time( nullptr );  // Current time in epoch
    struct tm gmt;  // GMT time structure
    gmtime_r( &now, &gmt );  // gmt has the now time in epoch
    
    struct tm loc;  // Local time structure
    localtime_r( &now, &loc );  // loc has the time now in epoch adjust for my timezone

    time_t gmtEpoch = mktime( &gmt );  // timestamp in epoch
    time_t locEpoch = mktime( &loc );  // timestamp in epoch
    long offset = locEpoch - gmtEpoch;  // should be 18,000 = 5 * 3600

    // Parse as local, then remove offset to get UTC
    time_t assumedLocal = mktime( &utc );  // timestamp in epoch 

    return assumedLocal + offset;  // Return epoch time adjusted to UTC

}


long secondsSincePosition1( JsonArray arr )  // Seconds since TalkBack position 1
{

    const char* ts = nullptr;  // Timestamp string

    for (JsonObject obj : arr)  // Loop through each object in the array
    {
        if ( (int)obj["position"] == 1)  // Check for position 1
        {
            ts = obj["created_at"];  // Get timestamp
            break;  // Exit loop once found
        }
    }

    if (!ts)  // No timestamp found
        return -1;  // Return error

    time_t created = iso8601ToEpochUsingGmtime( ts );  // Convert ISO 8601 to epoch time
#ifdef DEBUG_ENABLED

    DBGf( "[THINGSPEAK] TB created time: %ld\r\n",  created );  // Debug print created time

#endif
    
    if ( created < 0 )  // Error in conversion
        return -2;  // Return error

    time_t now = time( nullptr );  // Get current epoch time

    long diff = now - created;  // Calculate time difference

#ifdef DEBUG_ENABLED

    DBGf( "[THINGSPEAK] Time diff: %ld\r\n",  diff );  // Debug print time difference
  
#endif

    return long( now - created );  // Return time difference in seconds

}


void update_Schedule ( String cmdStr, uint8_t position )  // Update schedule from TalkBack
{

    int hours, minutes, seconds;  // Time components

    const char* timeStr = cmdStr.c_str();  // Convert to C-string

    if ( sscanf(timeStr, "%d:%d:%d", &hours, &minutes, &seconds) == 3 )  // Parse time components
    {
        
      if( hours >= 0 && hours <24 )
        {
            settings.times[position - 4].hour = hours;  // Update hour (position 5 maps to index 0)
        }
        
        if( minutes >= 0 && minutes < 60 )
        {
             settings.times[position - 4].min = minutes;  // Update minute
        }
        
        if( seconds >= 0 && seconds < 60 )
        {
             settings.times[position - 4].sec = seconds;  // Update second
        }

    }

#ifdef DEBUG_ENABLED

    DBGf( "[THINGSPEAK] Schedule from TalkBack: %02d:%02d:%02d\r\n", 
      settings.times[position - 4].hour, settings.times[position - 4].min, settings.times[position - 4].sec );  // Expected format: "HH:MM:SS,HH:MM:SS,HH:MM:SS,..."

#endif

}


bool check_new_settings()  // Compare stored settings with current ones
{
  
  JsonDocument doc;  // Create JSON document for parsing settings file
  
  if ( LittleFS.exists( "/settings.json") == false )  // Settings file does not exist
    return true;  // Need to save settings
  
  File file = LittleFS.open( "/settings.json", "r" );  // open settings file for reading

  if (!file)  // open failed
    return true;
  
  DeserializationError err = deserializeJson( doc, file );  // parse JSON file

  file.close();  // close file

  if ( err )  // parse failed
    return true;
  

  // ---- Scalars ----
  if ( doc["threshold"].as<float>() != settings.threshold )  // threshold differs
    return true;

  if ( doc["duration"].as<u32_t>()  != settings.duration )  // duration differs
    return true;

  if ( doc["rain_min_Prob"].as<uint32_t>()  != settings.rain_min_Prob )  // rain_min_Prob differs
    return true;

  // ---- Times ----
  JsonArray times = doc["times"].as<JsonArray>();  // load times array

  if ( times.isNull() || times.size() != SCHEDULE_COUNT )  // invalid times array
    return true;
  

  for ( uint8_t i = 0; i < SCHEDULE_COUNT; ++i )  // iterate times
  {
    JsonObject t = times[i];  // individual time object

    if (t["h"].as<uint8_t>() != settings.times[i].hour) return true;  // hour differs
    if (t["m"].as<uint8_t>() != settings.times[i].min)  return true;  // minute differs
    if (t["s"].as<uint8_t>() != settings.times[i].sec)  return true;  // second differs
  }

  return false;  // no differences

}


bool initFlashFS()  // Initialize LittleFS
{

  if ( LittleFS.begin(true) == false )
  {   // true = format if failed
    Serial.println( F( "[FILESYSTEM] LittleFS Mount Failed" ) );
    return false;
  }
  
  Serial.println( F( "[FILESYSTEM] LittleFS Mounted" ) );  // mounted
  return true;  // success

}


bool loadSettings()  // Load settings from LittleFS
{
  
  JsonDocument doc;  // Create JSON document for parsing settings file
  
  if ( LittleFS.exists("/settings.json") == false )  // settings file missing
  {
    Serial.println( F( "[FILESYSTEM] Settings file not found" ) );
    return false;
  }

  File file = LittleFS.open("/settings.json", "r");  // open file for reading
  
  if ( file == false )  // open failed
  {
    Serial.println( F( "[FILESYSTEM] Failed to open file for reading" ) );
    return false;
  }


  DeserializationError error = deserializeJson( doc, file );  // parse JSON file
  file.close();  // close file

  if ( error )  // parse failed
  {
    Serial.println( F( "[JSON] JSON parse failed" ) );
    return false;
  }

  settings.threshold = doc["threshold"].as<float>();  // load threshold
  settings.duration  = doc["duration"].as<uint32_t>();  // load duration
  settings.rain_min_Prob = doc["rain_min_Prob"].as<uint32_t>();  // load rain minimum probability  

  JsonArray times = doc["times"].as<JsonArray>();  // load times array

    
  if ( times.isNull() || times.size() != SCHEDULE_COUNT )  // invalid times
  {
    Serial.println( F( "[JSON] Invalid times array" ) );
    return false;
  }

  for ( uint8_t i = 0; i < SCHEDULE_COUNT; ++i )  // populate settings times
  {
    JsonObject t = times[i];  // get time object
    settings.times[i].hour = t["h"].as<uint8_t>();  // set hour
    settings.times[i].min  = t["m"].as<uint8_t>();  // set minute
    settings.times[i].sec  = t["s"].as<uint8_t>();  // set second
  }

  doc.clear();  // clear JSON doc
  
  Serial.println("[IRRIGATION] Settings loaded");  // log loaded
  printSettings();  // output settings

  return true;

}


bool saveSettings()
{
  
  JsonDocument doc;  // Create JSON document for saving settings

  File file = LittleFS.open("/settings.json", "w");  // open file for writing
  
  if ( file == false )  // open failed
  {
    Serial.println( F( "[FILESYSTEM] Failed to open file for writing" ) );
    return false;
  }


  doc["threshold"] = settings.threshold;  // store threshold
  doc["duration"]  = settings.duration;  // store duration
  doc["rain_min_Prob"] = settings.rain_min_Prob;  // store rain minimum probability

  JsonArray times = doc["times"].to<JsonArray>();  // create times array


  for ( uint8_t i = 0; i < SCHEDULE_COUNT; ++i )  // add each scheduled time
  {
    times[i]["h"] = settings.times[i].hour;  // save hour
    times[i]["m"] = settings.times[i].min;  // save minute
    times[i]["s"] = settings.times[i].sec;  // save second
  }

  serializeJsonPretty( doc, file );  // write JSON to file
 
  file.close();  // close file

  Serial.println( F( "[FILESYSTEM] Settings saved" ) );  // log saved
  serializeJsonPretty( doc, Serial );  // echo JSON
  Serial.println();  // newline for clarity

  doc.clear();  // clear JSON doc
  
  return true;  // saved OK

}


void printSettings()  // Print current settings to serial
{

    Serial.println(F("---- Settings ----"));  // header
    Serial.print(F("Threshold: "));  // label
    Serial.println(settings.threshold);  // threshold value
    Serial.print(F("Duration : "));  // label
    Serial.println(settings.duration);  // duration value
    Serial.print(F("Rain minimum probability: "));  // label
    Serial.println(settings.rain_min_Prob);  // rain minimum probability value


    for  (uint8_t i = 0; i < 4; ++i )  // print each scheduled time
    {

        Serial.print(F("Time"));  // label
        Serial.print(i + 1);  // index
        Serial.print(F(": "));  // separator
        Serial.print(settings.times[i].hour);  // hour
        Serial.print(F(":"));  // separator
        if (settings.times[i].min < 10) Serial.print("0");  // leading zero for minutes
        Serial.print(settings.times[i].min);  // minute
        Serial.print(F(":"));  // separator
        if (settings.times[i].sec < 10) Serial.print("0");  // leading zero for seconds
        Serial.println(settings.times[i].sec);  // second

    }

    Serial.println(F("------------------"));  // footer
}