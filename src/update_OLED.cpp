#include "update_OLED.h"  // project-wide definitions and prototypes
#include "helper.h"
#include "weather.h"


volatile Page currentPage = PAGE_STATUS;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


void setup_OLED()
{

  Wire.begin( 5, 6 );  // SDA, SCL
  
  if( display.begin( SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS ) == false )
  { 
#ifdef DEBUG
    DBG(F("SSD1306 allocation failed\r\n"));
#endif
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();  // Clear display buffer
  
  //display.setRotation(2); // Rotate display if needed (adjust as per your mounting)
  display.setTextSize(1);   // Draw 1X-scale text
  display.setTextColor(SSD1306_WHITE);  // Draw white text
  display.setTextWrap(true); // Enable text wrapping
  

  char buff[256];
  sprintf(buff, "Soil Monitoring &\r\nIrrigation System\r\nv%s", FIRMWARE_VERSION );  // Initial splash screen
  display_message( buff, 2000);

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

        case PAGE_WX:
            weather_Page();
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


void weather_Page()
{

    display.clearDisplay(); // Clear the display buffer

    display.setCursor(0,0); // Start at top-left corner

    for( uint8_t i = 0; i <= NUM_OF_PAGES; ++i )
    {
        display.printf( "POP[%d]: %d %%\r\n", i, precip_prob[i] );
    }

    display.display();

}


void display_message( const char *msg, uint32_t show_time )  // Display message on OLED
{

    display.clearDisplay(); // Clear the display buffer

    display.setCursor( 0, 0 ); // Start at top-left corner

    display.printf( msg );  // Display TS status
 
    display.display(); // Update the OLED with new content

    delay ( show_time );

    display.clearDisplay(); // Clear the display buffer

    display.display(); // Update the OLED with new content


}


void display_message( const char *msg )  // Display message on OLED
{

    display.clearDisplay(); // Clear the display buffer

    display.setCursor( 0, 0 ); // Start at top-left corner

    display.printf( msg );  // Display TS status
 
    display.display(); // Update the OLED with new content

}