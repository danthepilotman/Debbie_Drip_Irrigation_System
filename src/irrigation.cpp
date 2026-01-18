#include "irrigation.h"
#include "thingspeak.h"

void compute_watering_parameters()
{

    bool rain_expected_ESP32 = false;  // Remember last rain expected determination
    
    // -------- Weather Check --------
    if ( wifi_connectivity == true)
        rain_expected_ESP32 = rainExpectedSoon();


    DBGf( "[LOGIC] Rain expected soon: %s", rain_expected_ESP32 ? "YES\r\n" : "NO\r\n" );  // Report rain expectation

    if ( moisture < threshold && rain_expected_ESP32 == false )  // Determine if watering is needed
        watering_needed_ESP32 = YES;

    if ( ( rain_expected_ESP32 == rain_expected_TS ) && ( watering_needed_ESP32 == watering_needed_TS ) )  // Check if ESP and TS are in agreement
            Serial.println( F( "[LOGIC] Local ESP32 & ThingSpeak rain expected and watering needed agree") );

}


void solenoid_control()
{

    digitalWrite( RELAY_PIN, solenoid_state );  // Set power to solenoid based on solenoid_state

    
    if ( wifi_connectivity == true )
        solenoid_state_Update();  // Update TS with watering start/stop events

}


void  water_soil()
{

    static time_t watering_start_time;  // Record timestamp when watering started

    static time_t last_Print;  // Remember timestamp of last serial print

    if( solenoid_state == OFF )  // Don't recheck watering parameters if we're still watering
        compute_watering_parameters();  // Compute watering parameters
    
    if ( watering_needed_ESP32 == YES )
    {
        /*********************** Compute watering time remaining ****************************/
        
        time_t now = time(nullptr);  // Get the time right now

        time_t elapsed_sec;
        
        if( solenoid_state == OFF)
            elapsed_sec = 0;
        else
            elapsed_sec = now - watering_start_time;

        time_t watering_time_remaining = duration - elapsed_sec;

        if(  now - last_Print  >= 1)
        {
            DBGf( "[IRRIGATION] Watering time remaining: %ld sec\r\n", watering_time_remaining );

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