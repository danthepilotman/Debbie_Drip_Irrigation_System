#include "update_OLED.h"  // project-wide definitions and prototypes



void update_Display()
{
    switch (currentPage)
    {
        case PAGE_STATUS:
            status_OLED();
            break;

        case PAGE_SOIL:
            soil_OLED();
            break;

        case PAGE_SETTINGS:
            settings_OLED();
            break;

        case PAGE_WIFI:
            wifi_OLED();
            break;

        default:
            status_OLED();  // default to status page if somehow in an invalid state
            break;
    }
}



void status_OLED()
{

  if ( check_for_changes() == true || esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0 )  // Only update OLED if something has changed since last update to save power
  {
    display.clearDisplay(); // Clear the display buffer

    display.setCursor(0,0); // Start at top-left corner

    
    display.printf("Rain expected: %s \r\n", status.rain_expected ? F("YES") : F("NO") );
    display.printf("Watering needed: %s \r\n", status.watering_needed ? F("YES") : F("NO") );
    display.printf("Solenoid: %s \r\n", status.solenoid_state ? F("OPENED") : F("CLOSED") );
     
    display.printf("Status: %s\r\n", status.status_str.c_str() );  // Display TS status
 

    display.display(); // Update the OLED with new content
  }

}


void soil_OLED()
{

    get_new_readings();  // Get fresh reading from soil sensor
    
    display.clearDisplay(); // Clear the display buffer

    display.setCursor(0,0); // Start at top-left corner

    // Display watering status
    display.printf("WVC: %.1f\r\n", soil.moisture);
   

    // Display solenoid state
    display.printf("Temp: %.0f\r\n", 1.8 * soil.temp + 32.0);

    // Display WiFi status
    display.printf("EC: %.0f\r\n", soil.ec);

     // Display TS status
    display.printf("pH: %.1f\r\n", soil.pH);

    // Display TS status
    display.printf("N: %d\r\n", soil.N);

    // Display TS status
    display.printf("P: %d\r\n", soil.P);

    // Display TS status
    display.printf("K: %d\r\n", soil.K);

    display.display(); // Update the OLED with new content

}


void settings_OLED()
{

    display.clearDisplay(); // Clear the display buffer

    display.setCursor(0,0); // Start at top-left corner

    // Display watering status
    display.printf("Threshold: %.1f\r\n", settings.threshold);

    // Display solenoid state
    display.printf("Duration: %d\r\n", settings.duration);

    // Display WiFi status
    display.printf("Rain Min Prob: %d\r\n", settings.rain_min_Prob);

     // Display TS status
    for ( uint8_t i = 0; i < SCHEDULE_COUNT; i++ )
    {
        display.print(F("Time["));
        display.print(i+1);
        display.print(F("]: "));

        if ( settings.times[i].hour < 10 )
            display.print(F("0"));
        display.print(settings.times[i].hour);
        display.print(F(":"));

        if ( settings.times[i].min < 10 )
            display.print(F("0"));
        display.print(settings.times[i].min);
        display.print(F(":"));

        if ( settings.times[i].sec < 10 )
            display.print(F("0"));
        display.println(settings.times[i].sec);
    }

    display.display(); // Update the OLED with new content

}


void wifi_OLED()
{

    display.clearDisplay(); // Clear the display buffer

    display.setCursor(0,0); // Start at top-left corner

    // Display WiFi status
    display.printf("WiFi: %s \r\n", status.wifi_connectivity ? F("CONNECTED") : F("DISCONNECTED"));
    display.printf("RSSI: %d dBm\r\n", status.wifi_rssi);
    display.printf("IP: %s\r\n", WiFi.localIP().toString().c_str() );

    display.display(); // Update the OLED with new content

}


bool check_for_changes()
{

  static bool last_rain_expected = false; // Initialize to an impossible value to ensure first update
  static bool last_watering_needed = !status.watering_needed; // Force update on first run
  static bool last_solenoid_state = !status.rain_expected; // Force update on first run
  static bool last_wifi_connectivity = !status.wifi_connectivity; // Force update on first run
  static int last_wifi_rssi = status.wifi_rssi; // Initialize to current RSSI to avoid false change on first run
  static String last_status_str = ""; // Store last status string to check for changes

  // Update the OLED display if any of the values have changed
  if (status.rain_expected != last_rain_expected || status.watering_needed != last_watering_needed || status.solenoid_state != last_solenoid_state
     || status.wifi_connectivity != last_wifi_connectivity || status.status_str != last_status_str || status.wifi_rssi != last_wifi_rssi )
  {
    
    last_rain_expected = status.rain_expected; // Update last values to current values for next change detection
    last_watering_needed = status.watering_needed;
    last_solenoid_state = status.solenoid_state;
    last_wifi_connectivity = status.wifi_connectivity;
    last_status_str = status.status_str;
    return true;

  }
  else
    return false;

}