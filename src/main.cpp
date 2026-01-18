#include "setup.h"
#include "thingspeak.h"
#include "weather.h"
#include "irrigation.h"
#include "sleep_timer.h"


// ==================================================
// ================= SETUP ==========================
// ==================================================
void setup()
{
    
    setup_Serial();  // Setup serial port for debug statements

    setup_Digital();  // Setup GPIO to drive solenoid valve

#ifndef NO_SOIL_SENSOR

    setup_RS485();  // Setup RS-485 communication bus for soil sensor
    
#endif

    connect_WiFi();  // Connect to Wifi network

    setup_NTP();  // Connect to NTP and setup internal RTC

    setup_Storage();  // Setup non-volatile storage
    
}


    float moisture = 0.0;  // Set in get_new_readings(), used in sendThingSpeak() and compute_watering_parameters()

    float threshold;  // Set in getSettings() used in compute_watering_parameters()

    uint32_t duration;  // Set in getSettings() used in water_soil()

    bool watering_needed_ESP32 = NO;  // Set in compute_watering_parameters() used in water_soil() and loop()

    bool solenoid_state = OFF;  // Set in water_soil() used in loop()

    bool rain_expected_TS;  // Set in getSettings() used in compute_watering_parameters()

    bool watering_needed_TS;  // Set in getSettings()  used in compute_watering_parameters()

    bool wifi_connectivity = false;

// ==================================================
// ================= LOOP ===========================
// ==================================================
void loop()
{


    if ( watering_needed_ESP32 == NO )  // Don't sleep if watering needed. Otherwise keep sleeping until next time target window.
        deep_sleep_function();  // Go to sleep until next update cycle


    if ( solenoid_state == OFF )  // Only get new soil readings if not currently watering. This avoids constant updates while watering
       get_new_readings();  // Get readings for soil sensor and send to ThingSpeak. Also get weather forecast

    water_soil();  // Water soil if needed for the proper duration.
                   // Sets watering_needed to OFF if watering not needed.
                   // This then causes deep sleep during the next loop() cycle

}