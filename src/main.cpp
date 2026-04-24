#include "setup.h"
#include "thingspeak.h"
#include "weather.h"
#include "irrigation.h"
#include "sleep_timer.h"
#include "update_OLED.h"
#include "rgb_led.h"


// ==================================================
// ================= SETUP ==========================
// ==================================================
void setup()
{
    
     setup_Serial();  // Setup serial port for debug statements

     setup_Discretes();  // Setup GPIO to drive solenoid valve

 #ifdef SOIL_SENSOR

    setup_RS485();  // Setup RS-485 communication bus for soil sensor
    
 #endif

    setup_RGB();  // Setup RGB LED strip for status indication. Initial color is RED (indicating system is booting up)

    setup_OLED();  // Setup OLED display    

    connect_WiFi();  // Connect to Wifi network

    rgb_show_color( GREEN ); // Set LED to GREEN (indicating system is online)

    checkForOTAUpdate();  // Check for OTA updates and perform update if available

    setup_NTP();  // Connect to NTP and setup internal RTC

    tsClient.begin(true);  // Initialize ThingSpeak client with insecure SSL (since we're using setInsecure on the client)

    initFlashFS();  // Setup non-volatile storage

    loadSettings();  // Load settings from non-volatile storage

    // Check for power applied (cold boot) AND not waking from button press (i.e. woke from power-on event, not just reset or waking up to update OLED after button press)
    if ( esp_reset_reason()  == ESP_RST_POWERON && status.wifi_connectivity == true )  
    {
        ping_ThingSpeak();  // Transmit status message to ThingSpeak Channel
        getSettings();  // Fetch latest control settings from ThingSpeak TalkBack, don't check TalkBack timestamp since this is a manual reset or power-on event
    }
   
}

 
static uint8_t good_cycles = 0;

// ==================================================
// ================= LOOP ===========================
// ==================================================
void loop()
{
    // Check if firmware should be validated
    if ( good_cycles >= 2 )
        check_ota_state();
    

    check_button_press();  // Check for button press and update OLED if needed

    update_Display();  // Update OLED display with current status
    

    esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();

    if ( wakeup_cause == ESP_SLEEP_WAKEUP_EXT1 )
        return;
    

    else if ( wakeup_cause == ESP_SLEEP_WAKEUP_TIMER )
    {

        if (status.solenoid_state == OFF)
        {
            get_new_readings();
            thingSpeak_Update();
        }

        water_soil();

        if ( status.watering_needed == NO )
        {
            good_cycles++;
            deep_sleep_function();
        }

    }

    else
        deep_sleep_function();  // Go to sleep 
 
}