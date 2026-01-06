#include "setup.h"
#include "thingspeak.h"
#include "weather.h"
#include "irrigation.h"


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


// ==================================================
// ================= LOOP ===========================
// ==================================================
void loop()
{
 
    static uint32_t duration;  // Watering time in seconds
   
    static bool watering_needed = false;  // Watering needed (yes or no)

    static bool solenoid_closed = true;  // Store solenoid open/close state

    
    get_new_readings( duration, watering_needed );  // Get readings for soil sensor and send to ThingSpeak. Also get weather forecast

    water_soil( watering_needed,  solenoid_closed, duration );  // Water soil if needed for the proper duration

    deep_sleep_function( watering_needed, solenoid_closed );  // Close solenoid valve and go to sleep until next update cycle
    
 
}