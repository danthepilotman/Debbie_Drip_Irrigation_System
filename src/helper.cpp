#include "helper.h"  // Associated header file
#include "irrigation.h"  // for compute_watering_parameters() function
#include "LAMP_Server.h"  // for getServerSettings() function
#include "soil_sensor.h"  // for get_new_readings() function
#include "update_OLED.h"  // for currentPage, Page, NUM_OF_PAGES variables

#ifdef THINGSPEAK_ENABLE
    #include "thingspeak.h"  // thingspeak interface
    #include "ThingSpeak.h"  // ThingSpeak client
#endif



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
    

String Timestamp( const char* format )  // Get formatted timestamp string)
{
    struct tm timeinfo;  // Time structure
    
    char buf[64];  // Buffer to hold formatted time

    getLocalTime( &timeinfo );  // Get local time

    strftime( buf, sizeof(buf), format, &timeinfo );  // Format time according to provided format string

    return String( buf );  // Return formatted time as String
}


bool initFlashFS()  // Initialize LittleFS
{

  if ( LittleFS.begin(true) == false )
  {   // true = format if failed


#ifdef DEBUG_ENABLED

    DBG( F( "[FILESYSTEM] LittleFS Mount Failed" ) );

#endif

    return false;
  }
  
#ifdef DEBUG_ENABLED

  DBG( F( "[FILESYSTEM] LittleFS Mounted" ) );  // mounted

#endif

  return true;  // success

}


#ifdef DEBUG_ENABLED

void printSettings()  // Print current settings to serial
{

    DBG(F("---- Settings ----"));  // header
    Serial.print(F("Threshold: "));  // label
    DBG(settings.moisture_threshold);  // threshold value
    Serial.print(F("Duration : "));  // label
    DBG(settings.watering_duration_sec);  // duration value
    Serial.print(F("Rain minimum probability: "));  // label
    DBG(settings.min_precip_prob);  // rain minimum probability value


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
        DBG(settings.times[i].sec);  // second

    }

    DBG(F("------------------"));  // footer
}

#endif  // DEBUG_ENABLED


#ifdef THINGSPEAK_ENABLE

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
  if ( doc["threshold"].as<float>() != settings.moisture_threshold )  // threshold differs
    return true;

  if ( doc["duration"].as<u32_t>()  != settings.watering_duration_sec )  // duration differs
    return true;

  if ( doc["rain_min_Prob"].as<uint32_t>()  != settings.min_precip_prob )  // rain_min_Prob differs
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


bool saveSettings()
{
  
  JsonDocument doc;  // Create JSON document for saving settings

  File file = LittleFS.open("/settings.json", "w");  // open file for writing
  
  if ( file == false )  // open failed
  {

#ifdef DEBUG_ENABLED

    DBG( F( "[FILESYSTEM] Failed to open file for writing" ) );

#endif

    return false;
  }


  doc["threshold"] = settings.moisture_threshold;  // store threshold
  doc["duration"]  = settings.watering_duration_sec;  // store duration
  doc["rain_min_Prob"] = settings.min_precip_prob;  // store rain minimum probability

  JsonArray times = doc["times"].to<JsonArray>();  // create times array


  for ( uint8_t i = 0; i < SCHEDULE_COUNT; ++i )  // add each scheduled time
  {
    times[i]["h"] = settings.times[i].hour;  // save hour
    times[i]["m"] = settings.times[i].min;  // save minute
    times[i]["s"] = settings.times[i].sec;  // save second
  }

  serializeJsonPretty( doc, file );  // write JSON to file
 
  file.close();  // close file

#ifdef DEBUG_ENABLED

  DBG( F( "[FILESYSTEM] Settings saved" ) );  // log saved
  serializeJsonPretty( doc, Serial );  // echo JSON
  DBG();  // newline for clarity

#endif

  doc.clear();  // clear JSON doc

  display_message( "[FILESYSTEM] Settings saved", 2000);
  
  return true;  // saved OK

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

#endif  // THINGSPEAK_ENABLE


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

      currentPage = (Page)((currentPage + 1) % NUM_OF_PAGES);  // Cycle pages

    }

  }

}


void handle_sample_state()
{
    get_new_readings();

    compute_watering_parameters();

#ifdef THINGSPEAK_ENABLE

    thingSpeak_Update();

#else

    sendServerUpdate();  // Update server with latest readings

#endif

    status.watering_needed ? system_state = STATE_WATER : system_state = STATE_SLEEP;
    
}