#include "setup.h"  // global setup headers
#include "RS485.h"  // RS485 comms
#include "thingspeak.h"  // ThingSpeak interface
#include "weather.h"  // Weather checks


// ==================================================
// ========= Prototype Functions ===========
// ==================================================
void water_soil();  // perform watering
void compute_watering_parameters();  // compute duration/thresholds
void solenoid_control();  // manage solenoid state