#include "irrigation.h"


void compute_watering_parameters()  // evaluate if watering is needed
{

#ifdef ESP32_WX

    rainExpectedSoon();  // Get NWS weather forecast

#endif
    
#ifdef DEBUG_ENABLED

    DBGf( "[LOGIC] Rain expected soon: %s", rain_expected_TS ? "YES\r\n" : "NO\r\n" );  // Report rain expectation

#endif

    if ( moisture < settings.threshold && rain_expected_TS == false )  // Determine if watering is needed
        watering_needed_ESP32 = YES;

    if ( watering_needed_ESP32 == watering_needed_TS )  // Check if ESP and TS are in agreement
            Serial.println( F( "[LOGIC] Local ESP32 & ThingSpeak watering needed agree") );

}


void solenoid_control()  // apply solenoid state and report
{

    digitalWrite( RELAY_PIN, solenoid_state );  // drive solenoid MOSFET

    
    if ( wifi_connectivity == true )
        solenoid_state_Update();  // Update TS with watering start/stop events

}


void  water_soil()  // perform watering flow control
{

    static time_t watering_start_time;  // timestamp when watering started

    static time_t last_Print;  // last printed time timestamp

    if( solenoid_state == OFF )  // Don't recheck watering parameters if we're still watering
        compute_watering_parameters();  // Compute watering parameters
    
    if ( watering_needed_ESP32 == YES )
    {
        /*********************** Compute watering time remaining ****************************/
        
        time_t now = time(nullptr);  // Get the time right now

        time_t elapsed_sec;  // elapsed watering seconds
        
        if( solenoid_state == OFF)
            elapsed_sec = 0;  // not running
        else
            elapsed_sec = now - watering_start_time;  // compute elapsed time

        time_t watering_time_remaining = settings.duration - elapsed_sec;

        if(  now - last_Print  >= 1)
        {

#ifdef DEBUG_ENABLED

            DBGf( "[IRRIGATION] Watering time remaining: %ld sec\r\n", watering_time_remaining );

#endif
            last_Print = now;
        }

        /*********************** Duration exceeded ****************************/
        
        if( watering_time_remaining == 0 )  // Check if watering cycle has completed
        {
                
            watering_needed_ESP32 = NO;  // Update watering needed 
            
            solenoid_state = OFF;  // Update solenoid valve state

            solenoid_control();  // Control solenoid valve and report status to ThingSpeak
        
        }
        
        /*********************** Start watering cycle ****************************/
        
        else if( solenoid_state == OFF )
        {
            
            solenoid_state = ON;  // Set variable value

            solenoid_control();  // Control solenoid valve and report status to ThingSpeak

            watering_start_time = time(nullptr);  // Record watering start time

        }
    
    }
    
    /*********************** Stop watering cycle ****************************/
    
    else if( solenoid_state == ON )
    {
        
        solenoid_state = OFF;  // Set variable value
        solenoid_control();  // Control solenoid valve and report status to ThingSpeak

    }

}  // END