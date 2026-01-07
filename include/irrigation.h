#include "setup.h"
#include "RS485.h"
#include "thingspeak.h"
#include "weather.h"

bool get_new_readings( uint32_t &duration, bool &watering_needed );

void  water_soil( bool &watering_needed, bool &solenoid_closed, uint32_t duration );

void deep_sleep_function( bool watering_needed,  bool &solenoid_closed );