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

    strftime( buf, sizeof( buf ), "%a %b %d, %Y %I:%M:%S %p", &timeinfo );  // Format time as "Dow Mon DD, YYYY HH:MM:SS AM/PM"

    return String( buf );  // Return formatted timestamp

}


void solenoid_state_Update()  // Report solenoid state to ThingSpeak
{
    const char* url = "https://api.thingspeak.com/update";

    status.status_str = String("Watering ") +
                        String(status.solenoid_state ? "started " : "stopped ") +
                        Timestamp();

    char status_c[128];

    urlEncode(status.status_str).toCharArray(status_c, sizeof(status_c));

#ifdef DEBUG_ENABLED
    DBGf("[IRRIGATION] Solenoid is now %s",
         status.solenoid_state ? "ON\r\n" : "OFF\r\n");
#endif

    // Build POST body only
    String postData = "api_key=" + String(TS_WRITE_KEY);
    postData += "&field8=" + String(status.solenoid_state ? 1 : 0);
    postData += "&status=" + String(status_c);

#ifdef DEBUG_ENABLED
    Serial.printf("[THINGSPEAK] POST body: %s\r\n", postData.c_str());
#endif

    // Single call replaces entire retry + HTTP logic
    ThingSpeakResponse resp = tsClient.postWithRetry(
            url,
            postData,
            MAX_TRIES,
            TS_PROCESS_DELAY
        );

    Serial.printf("[THINGSPEAK] HTTP code: %d, payload: %s\r\n",
                  resp.httpCode,
                  resp.body.c_str());
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
  
  Serial.println(F("[IRRIGATION] Settings loaded"));  // log loaded
  printSettings();  // output settings

  display.clearDisplay();
  display.setCursor(0,0);

  display.print(F("[IRRIGATION]\r\nSettings loaded")); 
  display.display();
    
  delay(2000);

  display.clearDisplay();
  display.display();

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


/******************************* Get SOIL sensor readings and update ThingSpeak *********************/
void get_new_readings()
{
    
#ifdef DEBUG_ENABLED

    DBG( F( "[STATUS] ===== SYSTEM CYCLE START =====" ) );  // mark cycle start

#endif

    RS485_STATUS rs485_status;  // RS485 operation status

    // -------- Read Soil Sensor --------
#ifdef SOIL_SENSOR

    uint16_t values[SOIL_REG_SIZE]; // Store 7 register values

#ifdef DEBUG_ENABLED

    DBG( F( "[RS485] Reading soil sensor" ) );

#endif

    for( uint8_t num_of_attempts = 0; num_of_attempts < MAX_TRIES; ++num_of_attempts )  // try up to 5 times
    {
        status = read_Registers( RS485Serial, 0x01, 0x0000, 5, values );  // read registers via Modbus

        if ( status == RS485_GOOD )
            break;
        
#ifdef DEBUG_ENABLED

        else
            DBG( F( "[RS485][ERROR] Modbus error" ) );
#endif
        
    }
       
#else

    const uint16_t values[SOIL_REG_SIZE] = {227,203,100,70,50,40,30}; // Store 7 register values

#endif

    uint16_t rawMoisture = values[ SOIL_MOISTURE ];  // raw moisture register
    uint16_t rawTemp     = values[ SOIL_TEMPERATURE ];  // raw temperature register
    uint16_t rawEC       = values[ SOIL_EC];  // raw EC register
    uint16_t rawPH       = values[ SOIL_PH ];  // raw pH register
    uint16_t rawN        = values[ SOIL_N ];  // raw N register
    uint16_t rawP        = values[ SOIL_P ];  // raw P register
    uint16_t rawK        = values[ SOIL_K ];  // raw K register

    soil.moisture = float(rawMoisture) / 10.0;  // convert to percent
    soil.temp     = float( int16_t( rawTemp ) ) / 10.0;  // convert to °C (signed)
    soil.ec       = float(rawEC);  // conductivity µS/cm
    soil.pH       = float(rawPH) / 10.0;  // pH scaled by 10
    soil.N        = rawN;  // N in mg/kg
    soil.P        = rawP;  // P in mg/kg    
    soil.K        = rawK;  // K in mg/kg

#ifdef DEBUG_ENABLED

    DBGf( "[DATA] Moisture: %.1f %%\r\n", soil.moisture );  // log moisture
    DBGf( "[DATA] Temp: %.1f °C\r\n", soil.temp );  // log temperature
    DBGf( "[DATA] EC: %.0f µS/cm\r\n", soil.ec );  // log EC
    DBGf( "[DATA] pH: %.1f\r\n", soil.pH );  // log pH
    DBGf( "[DATA] NPK: %u / %u / %u mg/kg\r\n", rawN, rawP, rawK );  // log NPK registers

#endif
  
}


void check_button_press()
{

  static unsigned long lastButtonTime = 0;
    
  const unsigned long debounceDelay = 700;

  if ( buttonPressed )
  {
    buttonPressed = false;

    unsigned long now = millis();

    if (now - lastButtonTime > debounceDelay)
    {
      lastButtonTime = now;

      currentPage = (Page)((currentPage + 1));  // Cycle pages

      if (currentPage >= NUM_OF_PAGES)
        currentPage = PAGE_STATUS ; // Wrap around to first page if we go past the last one
    }

  }

}