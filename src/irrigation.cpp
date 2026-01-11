#include "irrigation.h"
#include "thingspeak.h"

void compute_watering_parameters()
{

    static bool rain_expected_ESP32;  // Remember last rain expected determination
    
    // -------- Weather Check --------
    if( solenoid_state == OFF);  // Don't recheck rain if we're still watering
        rain_expected_ESP32 = rainExpectedSoon();  

    DBGf( "[LOGIC] Rain expected soon: %s\n", rain_expected_ESP32 ? "YES" : "NO" );  // Report rain expectation

    if ( moisture < threshold && rain_expected_ESP32 == false )  // Determine if watering is needed
        watering_needed_ESP32 = YES;

    if ( ( rain_expected_ESP32 == rain_expected_TS ) && ( watering_needed_ESP32 == watering_needed_TS ) )  // Check if ESP and TS are in agreement
            Serial.println( F( "[LOGIC] Local ESP32 & ThingSpeak rain expected and watering needed agree") );   

}


void solenoid_control()
{

    digitalWrite( RELAY_PIN, solenoid_state );  // Set power to solenoid based on solenoid_state

    solenoid_state_Update();  // Update TS with watering start/stop events

}


void  water_soil()
{

    static time_t watering_start_time;  // Record timestamp when watering started

    static time_t last_Print;  // Remember timestamp of last serial print
 
    compute_watering_parameters();  // Compute watering parameters
    
    if ( watering_needed_ESP32 == YES )
    {
        /*********************** Compute watering time remaining ****************************/
        time_t now = time(nullptr);

        uint32_t elapsed_sec = uint32_t(now - watering_start_time);

        uint32_t watering_time_remaining = duration - elapsed_sec;
  
        if( now - last_Print > 1)
        {
            DBGf( "[IRRIGATION] Watering time remaining: %ld sec\n", watering_time_remaining );

            last_Print = now;
        }

        if( watering_time_remaining == 0 )  // Check if watering cycle has completed
        {
                
            solenoid_state = OFF;  // Update solenoid valve state

            solenoid_control();
             
            DBG( F( "[LOGIC] Watering NOT required" ) );  // Inform user that watering is not needed

        }
        else if( solenoid_state == OFF )
        {
            solenoid_state = ON;  // Open solenoid

            solenoid_control();

            watering_start_time = time(nullptr);  // Record watering start time

        }
      
    }
    
    else if( solenoid_state == ON )
    {
        
        solenoid_state = OFF;
        solenoid_control();

    }

}  // END