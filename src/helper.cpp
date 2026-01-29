#include "helper.h"


String urlEncode( const String &input )
{
  
    String encoded = "";
    char c;
    char buf[4];

    for (uint8_t i = 0; i < input.length(); ++i )
    {
        c = input[i];

        // Unreserved characters according to RFC 3986
        if ( isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' )
          encoded += c;
        
        else
        {
            // Percent-encode everything else
            snprintf( buf, sizeof( buf ), "%%%02X", (unsigned char)c );
            encoded += buf;
        }
    }

    return encoded;
}


String Timestamp()
{
    struct tm timeinfo;

    char buf[32];

    getLocalTime( &timeinfo );

    strftime( buf, sizeof( buf ), "%a, %b %d, %Y %I:%M:%S %p", &timeinfo );

    return String( buf );

}


void solenoid_state_Update()
{

    String url = "http://api.thingspeak.com/update?api_key=";
    url += TS_WRITE_KEY;
    url += "&field8=" + String(solenoid_state);
    url+= "&status=" + urlEncode( String("Watering ") + String(solenoid_state ? "started " : "stopped ") + Timestamp() );


    DBGf( "[IRRIGATION] Solenoid is now %s", solenoid_state ? "ON\r\n" : "OFF\r\n" );
    Serial.printf( "[THINGSPEAK] URL: %s\r\n", url.c_str() );

    HTTPClient http;

    for( uint8_t tries = 1; tries <= MAX_TRIES; tries++ )
    {

      int code;  // Store HTTP response codes

      if( tries == 1 || code !=  HTTP_CODE_OK )  // Try at least once or if not getting a good HTTP response code
      {
        http.begin( url );
        
        int code = http.GET();

        Serial.printf("[THINGSPEAK] HTTP code: %d\r\n", code );
      }

      delay( TS_DELAY);

    }
   
}


time_t iso8601ToEpochUsingGmtime( const char* ts )
{

    int Y, M, D, h, m, s;

    if (sscanf(ts, "%4d-%2d-%2dT%2d:%2d:%2dZ", &Y, &M, &D, &h, &m, &s) != 6)
      return -1;

    struct tm utc = {};
    utc.tm_year = Y - 1900;
    utc.tm_mon  = M - 1;
    utc.tm_mday = D;
    utc.tm_hour = h;
    utc.tm_min  = m;
    utc.tm_sec  = s;
    utc.tm_isdst = 0;


    // Get current UTC offset
    time_t now = time( nullptr );
    struct tm gmt;
    gmtime_r( &now, &gmt );  // gmt has the now time in epoch
    
    struct tm loc;
    localtime_r( &now, &loc );  // loc has the time now in epoch adjust for my timezone

    time_t gmtEpoch = mktime( &gmt );
    time_t locEpoch = mktime( &loc );
    long offset = locEpoch - gmtEpoch;  // should be 18,000 = 5 * 3600

    // Parse as local, then remove offset to get UTC
    time_t assumedLocal = mktime( &utc );  // timestamp in epoch 

    // DBGf("Assumed local: %ld\r\n",  assumedLocal - offset);
    return assumedLocal + offset;

}


long secondsSincePosition1( JsonArray arr )
{

    const char* ts = nullptr;

    for (JsonObject obj : arr)
    {
        if ( (int)obj["position"] == 1)
        {
            ts = obj["created_at"];
            break;
        }
    }

    if (!ts)
        return -1;

    time_t created = iso8601ToEpochUsingGmtime( ts );

    DBGf( "[THINGSPEAK] TB created time: %ld\r\n",  created );
    
    if ( created < 0 )
        return -2;

    time_t now = time( nullptr );

    long diff = now - created;

    DBGf("[THINGSPEAK] Time diff: %ld\r\n",  diff);

    return long( (now - created) );

}


void update_Schedule ( String cmdStr, uint8_t position )
{

    DBGf( "[THINGSPEAK] Schedule from TalkBack: %s\r\n", cmdStr.c_str() );  // Expected format: "HH:MM:SS,HH:MM:SS,HH:MM:SS,..."

    int hours, minutes, seconds;

    const char* timeStr = cmdStr.c_str();

    if ( sscanf(timeStr, "%d:%d:%d", &hours, &minutes, &seconds) == 3 )
    {
        settings.times[position - 5].hour = hours;
        settings.times[position - 5].min = minutes;
        settings.times[position - 5].sec = seconds;
    }

}


bool check_new_settings()
{
  
  if ( LittleFS.exists("/settings.json") == false )
    return true;
  
  File file = LittleFS.open("/settings.json", "r");

  if (!file)
    return true;
  
  JsonDocument doc;

  DeserializationError err = deserializeJson( doc, file );

  file.close();

  if (err)
    return true;
  

  // ---- Scalars ----
  if ( doc["threshold"].as<float>() != settings.threshold )
    return true;

  if ( doc["duration"].as<int>()  != settings.duration )
    return true;

  // ---- Times ----
  JsonArray times = doc["times"].as<JsonArray>();

  if ( times.isNull() || times.size() != SCHEDULE_COUNT )
    return true;
  

  for ( uint8_t i = 0; i < SCHEDULE_COUNT; ++i )
  {
    JsonObject t = times[i];

    if (t["h"].as<uint8_t>() != settings.times[i].hour) return true;
    if (t["m"].as<uint8_t>() != settings.times[i].min)  return true;
    if (t["s"].as<uint8_t>() != settings.times[i].sec)  return true;
  }

  return false;  // no differences

}


bool initFlashFS()
{

  if ( LittleFS.begin(true) == false )
  {   // true = format if failed
    Serial.println( F( "[FILESYSTEM] LittleFS Mount Failed" ) );
    return false;
  }
  
  Serial.println( F( "[FILESYSTEM] LittleFS Mounted" ) );
  return true;

}


bool loadSettings()
{
  
  if ( LittleFS.exists("/settings.json") == false )
  {
    Serial.println("[FILESYSTEM] Settings file not found");
    return false;
  }

  File file = LittleFS.open("/settings.json", "r");
  
  if ( file == false )
  {
    Serial.println("[FILESYSTEM] Failed to open file for reading");
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error)
  {
    Serial.println("[JSON] JSON parse failed");
    return false;
  }

  settings.threshold = doc["threshold"].as<float>();
  settings.duration  = doc["duration"].as<uint32_t>();

  JsonArray times = doc["times"].as<JsonArray>();
  
  if ( times.isNull() || times.size() != SCHEDULE_COUNT )
  {
    Serial.println("[JSON] Invalid times array");
    return false;
  }

  for ( uint8_t i = 0; i < SCHEDULE_COUNT; ++i )
  {
    JsonObject t = times[i];
    settings.times[i].hour = t["h"].as<uint8_t>();
    settings.times[i].min  = t["m"].as<uint8_t>();
    settings.times[i].sec  = t["s"].as<uint8_t>();
  }

  Serial.println("[IRRIGATION] Settings loaded");
  printSettings();

  return true;

}


bool saveSettings()
{
  File file = LittleFS.open("/settings.json", "w");
  
  if (file == false)
  {
    Serial.println("[FILESYSTEM] Failed to open file for writing");
    return false;
  }

  JsonDocument doc;

  doc["threshold"] = settings.threshold;
  doc["duration"]  = settings.duration;

  JsonArray times = doc["times"].to<JsonArray>();

  for ( uint8_t i = 0; i < SCHEDULE_COUNT; ++i )
  {
    JsonObject t = times.add<JsonObject>();
    t["h"] = settings.times[i].hour;
    t["m"] = settings.times[i].min;
    t["s"] = settings.times[i].sec;
  }

  serializeJsonPretty(doc, file);
  file.close();

  Serial.println("[FILESYSTEM] Settings saved");
  serializeJsonPretty(doc, Serial);
  Serial.println();  // newline for clarity

  return true;

}


void printSettings()
{

    Serial.println(F("---- Settings ----"));
    Serial.print(F("Threshold: "));
    Serial.println(settings.threshold);
    Serial.print(F("Duration : "));
    Serial.println(settings.duration);

    for  (uint8_t i = 0; i < 4; ++i )
    {

        Serial.print(F("Time"));
        Serial.print(i + 1);
        Serial.print(F(": "));
        Serial.print(settings.times[i].hour);
        Serial.print(F(":"));
        if (settings.times[i].min < 10) Serial.print("0");
        Serial.print(settings.times[i].min);
        Serial.print(F(":"));
        if (settings.times[i].sec < 10) Serial.print("0");
        Serial.println(settings.times[i].sec);

    }

    Serial.println(F("------------------"));
}