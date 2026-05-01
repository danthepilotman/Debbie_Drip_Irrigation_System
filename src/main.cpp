#include "setup.h"
#include "thingspeak.h"
#include "weather.h"
#include "irrigation.h"
#include "sleep_timer.h"
#include "update_OLED.h"
#include "rgb_led.h"
#include "helper.h"


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

 

// ==================================================
// ================= LOOP ===========================
// ==================================================
void loop()
{
    if ( good_cycles >= 2 )
        check_ota_state();

    check_button_press();  // Check if button was pressed to update OLED display

    update_Display();  // Update OLED display based on current page and latest system status

    esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();  // Determine wakeup cause

    if ( wakeup_cause == ESP_SLEEP_WAKEUP_EXT1 )
        return;  // Don't do anything and just return to the top of the loop to update the OLED based on button press

    switch ( system_state )  // Main state machine switch to determine which behavior to execute in this cycle (sleep, sample, or water)
    {
        case STATE_SLEEP:
            handle_sleep_state();
            break;

        case STATE_SAMPLE:
            handle_sample_state();
            break;

        case STATE_WATER:
            handle_watering_state();
            break;
    }
}