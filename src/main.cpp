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
   
bool watering_needed_ESP32 = NO;  // Watering needed (yes or no)

bool solenoid_state = OFF;  // Store solenoid open/close (ON/OFF) state

// ==================================================
// ================= LOOP ===========================
// ==================================================
void loop()
{
 
    if ( watering_needed_ESP32 == NO )  // Don't sleep if watering needed
        deep_sleep_function();  // Go to sleep until next update cycle

    if ( solenoid_state == OFF )  // Only get new soil readings if not currently watering
       get_new_readings();  // Get readings for soil sensor and send to ThingSpeak. Also get weather forecast

    water_soil();  // Water soil if needed for the proper duration

}