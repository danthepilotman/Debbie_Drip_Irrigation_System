//#include "setup.h"  // global setup headers
#include "soil_sensor.h"  // RS485 comms
#include "weather.h"  // Weather checks
#include <math.h.>  // math functions for time calculations

#ifdef THINGSPEAK_ENABLE
    #include "thingspeak.h"  // ThingSpeak interface
#endif

// ==================================================
// ========= Prototype Functions ===========
// ==================================================
void water_soil();  // perform watering
void compute_watering_parameters();  // compute duration/thresholds
void solenoid_control();  // manage solenoid state
void handle_watering_state(); // main function to call in loop to manage watering behavior