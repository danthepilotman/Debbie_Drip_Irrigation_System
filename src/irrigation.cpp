#include "irrigation.h"
#include "rgb_led.h"


void compute_watering_parameters()  // evaluate if watering is needed
{

    bool rain_expected_ESP32 = rainExpectedSoon();  // Get NWS weather forecast

    if ( soil.moisture < settings.threshold && rain_expected_ESP32 == false )  // Determine if watering is needed
        status.watering_needed = YES;
    else
        status.watering_needed = NO;

#ifdef DEBUG_ENABLED

    DBGf( "[LOGIC] Rain expected soon: %s", status.rain_expected ? "YES\r\n" : "NO\r\n" );  // Report rain expectation
    DBGf( "[LOGIC] Watering needed: %s", status.watering_needed ? "YES\r\n" : "NO\r\n" );  // Report watering need

#endif

}


void solenoid_control()  // apply solenoid state and report
{
    
    digitalWrite( RELAY_PIN, status.solenoid_state ? HIGH : LOW );  // Apply solenoid state to relay
    
    if ( status.solenoid_state == ON) 
        rgb_show_color( BLUE ); // Set LED to BLUE (indicating system is watering)

    else
        rgb_show_color( BLACK ); // Set LED to off (indicating system is not watering)
    
    

    if ( status.wifi_connectivity == true )
        solenoid_state_Update();  // Update TS with watering start/stop events

}


void  water_soil()  // perform watering flow control
{

    static time_t watering_start_time;  // timestamp when watering started

    static time_t last_Print;  // last printed time timestamp

    if( status.solenoid_state == OFF )  // Don't recheck watering parameters if we're still watering
        compute_watering_parameters();  // Compute watering parameters
    
    if ( status.watering_needed == YES )
    {
        /*********************** Compute watering time remaining ****************************/
        
        time_t now = time(nullptr);  // Get the time right now

        time_t elapsed_sec;  // elapsed watering seconds
        
        if( status.solenoid_state == OFF)
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
        
        if( watering_time_remaining <= 0 )  // Check if watering cycle has completed
        {
                
            status.watering_needed = NO;  // Update watering needed 
            
            status.solenoid_state = OFF;  // Update solenoid valve state

            solenoid_control();  // Control solenoid valve and report status to ThingSpeak
        
        }
        
        /*********************** Start watering cycle ****************************/
        
        else if( status.solenoid_state == OFF )
        {
            
            status.solenoid_state = ON;  // Set variable value

            solenoid_control();  // Control solenoid valve and report status to ThingSpeak

            watering_start_time = time(nullptr);  // Record watering start time

        }
    
    }
    
    /*********************** Stop watering cycle ****************************/
    
    else if( status.solenoid_state == ON )
    {
        
        status.solenoid_state = OFF;  // Set variable value
        solenoid_control();  // Control solenoid valve and report status to ThingSpeak

    }

}  // END