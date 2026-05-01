#include "setup.h"  // global setup headers
#include "RS485.h"  // RS485 comms
#include "thingspeak.h"  // ThingSpeak interface
#include "weather.h"  // Weather checks
#include <math.h.>  // math functions for time calculations

// ==================================================
// ========= Prototype Functions ===========
// ==================================================
void water_soil();  // perform watering
void compute_watering_parameters();  // compute duration/thresholds
void solenoid_control();  // manage solenoid state
void handle_watering_state(); // main function to call in loop to manage watering behavior