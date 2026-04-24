#include "update_OLED.h"  // project-wide definitions and prototypes


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


void setup_OLED()
{

  Wire.begin( 5, 6 );  // SDA, SCL
  
  if( display.begin( SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS ) == false )
  { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();  // Clear display buffer
  
  display.setRotation(2); // Rotate display if needed (adjust as per your mounting)
  display.setTextSize(1);   // Draw 1X-scale text
  display.setTextColor(SSD1306_WHITE);  // Draw white text
  display.setTextWrap(true); // Enable text wrapping
  display.setCursor(0,0);    // Start at top-left corner
  display.print(F("Soil Monitoring &\r\nIrrigation System\r\nV1.0.1"));  // Initial splash screen

  display.display();  // Show initial message
  
  delay(2000);  // Display splash screen for 2 seconds

  display.clearDisplay();  // Clear display for next updates

  display.display();  // Update display to show cleared screen

}


void update_Display()
{
    switch (currentPage)
    {
        case PAGE_STATUS:
            status_Page();
            break;

        case PAGE_SOIL:
            soil_Page();
            break;

        case PAGE_SETTINGS:
            settings_Page();
            break;

        case PAGE_WIFI:
            wifi_Page();
            break;

        default:
            status_Page();  // default to status page if somehow in an invalid state
            break;
    }
}


void status_Page()
{

    display.clearDisplay(); // Clear the display buffer

    display.setCursor( 0, 0 ); // Start at top-left corner

    
    display.printf( "Rain expected: %s \r\n", status.rain_expected ? F("YES") : F("NO") );
    display.printf( "Watering needed: %s \r\n", status.watering_needed ? F("YES") : F("NO") );
    display.printf( "Solenoid: %s \r\n", status.solenoid_state ? F("OPENED") : F("CLOSED") );
     
    display.printf( "Status: %s\r\n", status.status_str.c_str() );  // Display TS status
 

    display.display(); // Update the OLED with new content
  
}


void soil_Page()
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


void settings_Page()
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


void wifi_Page()
{

    display.clearDisplay(); // Clear the display buffer

    display.setCursor(0,0); // Start at top-left corner

    // Display WiFi status
    display.printf("WiFi: %s \r\n", status.wifi_connectivity ? F("CONNECTED") : F("DISCONNECTED"));
    display.printf("RSSI: %d dBm\r\n", status.wifi_rssi);
    display.printf("IP: %s\r\n", WiFi.localIP().toString().c_str() );

    display.display(); // Update the OLED with new content

}