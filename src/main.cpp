#include "setup.h"
#include "thingspeak.h"
#include "weather.h"
#include "irrigation.h"
#include "sleep_timer.h"
#include "update_OLED.h"


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

    setup_OLED();  // Setup OLED display    

    connect_WiFi();  // Connect to Wifi network

    setup_NTP();  // Connect to NTP and setup internal RTC

    tsClient.begin(true);  // Initialize ThingSpeak client with insecure SSL (since we're using setInsecure on the client)

    initFlashFS();  // Setup non-volatile storage

    loadSettings();  // Load settings from non-volatile storage

    
    esp_reset_reason_t resetReason = esp_reset_reason();
    
    if ( resetReason  == ESP_RST_POWERON || resetReason == ESP_RST_EXT )  // Check for power applied (cold boot) or reset button press
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
    
    check_button_press();  // Check for button press and update currentPage for OLED navigation if button pressed

    update_Display();  // Update display continuously

    if ( esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0 )  // If woke from button press, skip sleep logic for this cycle 
        return;  // skip rest of loop to avoid sleeping immediately after waking from button press
    
    if ( status.watering_needed == NO )  // Don't sleep if watering needed. Otherwise keep sleeping until next time target window.
        deep_sleep_function();  // Go to sleep until next update cycle

    if ( status.solenoid_state == OFF )  // Only get new soil readings if not currently watering. This avoids constant updates while watering
    {
       get_new_readings();  // Get readings for soil sensor and send to ThingSpeak. Also get weather forecast
       thingSpeak_Update();  // Upload readings to ThingSpeak and fetch TalkBack settings, but only if woke up from timer and have WiFi. This ensures we get fresh data after sleep but don't waste power uploading if we're just waking up to update the OLED after a button press.
    }

    water_soil();  // Water soil if needed for the proper duration.
                   // Sets watering_needed to NO if watering not needed.
                   // This then causes deep sleep during the next loop() cycle

}