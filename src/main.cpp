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

    setup_RS485();  // Setup RS-485 communication bus for soil sensor

    connect_WiFi();  // Connect to Wifi network

    setup_NTP();  // Connect to NTP and setup internal RTC
}


uint32_t duration;  // Watering time in seconds
   
bool watering_needed_ESP32 = false;  // Watering needed (yes or no)

bool solenoid_closed = true;  // Store solenoid open/close state

// ==================================================
// ================= LOOP ===========================
// ==================================================
void loop()
{
 
    if ( watering_needed_ESP32 == false )  // Don't sleep if watering needed
        deep_sleep_function();  // Go to sleep until next update cycle

    if ( solenoid_closed == true )  // Only get new soil readings if not currently watering
        while ( get_new_readings() == false);  // Get readings for soil sensor and send to ThingSpeak. Also get weather forecast

    water_soil();  // Water soil if needed for the proper duration

}