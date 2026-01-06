#include "irrigation.h"


/******************************* Get SOIL sensor readings and update ThingSpeak *********************/   
bool get_new_readings( uint32_t &duration, bool &watering_needed )
{

    bool success = true;  // Assume true to allow for function to return true in cases where watering_needed == true
    
    if ( watering_needed == false )  // Don't keep getting new readings if watering is not needed
    {
        DBG( F( "[STATUS] ===== SYSTEM CYCLE START =====" ) );

        // -------- Read Soil Sensor --------
        DBG( F( "[RS485] Reading soil sensor" ) );

        uint8_t result = node.readHoldingRegisters( 0x0000, 7 );

        if ( result != node.ku8MBSuccess )
            DBGf( "[RS485][ERROR] Modbus error code: %u\n", result );
        
    

        uint16_t rawMoisture = node.getResponseBuffer( 0 );
        uint16_t rawTemp     = node.getResponseBuffer( 1 );
        uint16_t rawEC       = node.getResponseBuffer( 2 );
        uint16_t rawPH       = node.getResponseBuffer( 3 );
        uint16_t rawN        = node.getResponseBuffer( 4 );
        uint16_t rawP        = node.getResponseBuffer( 5 );
        uint16_t rawK        = node.getResponseBuffer( 6 );

        float moisture = rawMoisture / 10.0;
        float temp     = int16_t ( rawTemp ) / 10.0;
        float ec       = rawEC;
        float ph       = rawPH / 10.0;

        DBGf( "[DATA] Moisture: %.1f %%\n", moisture );
        DBGf( "[DATA] Temp: %.1f °F\n", temp );
        DBGf( "[DATA] EC: %.0f µS/cm\n", ec );
        DBGf( "[DATA] pH: %.1f\n", ph );
        DBGf( "[DATA] NPK: %u / %u / %u mg/kg\n", rawN, rawP, rawK );

        // -------- ThingSpeak Upload --------
        success = ( result == node.ku8MBSuccess ) && sendThingSpeak( moisture, temp, ec, ph, rawN, rawP, rawK );

        // -------- Read Control Settings --------

        uint8_t threshold;  // Water content by volume percentage minimum threshold to trigger watering cycle
        
        success = success && getSettings( threshold, duration );

        // -------- Weather Check --------
        bool rain = rainExpectedSoon();

        DBGf( "[LOGIC] Rain expected soon: %s\n", rain ? "YES" : "NO" );

        if ( moisture < threshold && rain == false )
            watering_needed = true;

        return success;
    
    }

    else
        return success;
}


void  water_soil( bool &watering_needed,  bool &solenoid_closed, uint32_t duration )
{

    static time_t watering_start_time;  // Record timestamp when watering started

    static time_t lastPrint;  // Remember timestamp of last serial print

    /************************ Continue watering if needed ******************************/    

#ifndef DISABLE_RELAY 
       
    // -------- Irrigation Logic --------
    if ( watering_needed )
    {

        if( solenoid_closed )
        {
            DBG( F( "[LOGIC] Watering conditions MET" ) );
  
            digitalWrite( RELAY_PIN, HIGH );  // Open solenoid valve

            time( &watering_start_time );  // Record timestamp when watering started

            lastPrint = watering_start_time;

            struct tm* localTime = localtime( &watering_start_time );

            char buffer[30];

            strftime( buffer, sizeof(buffer), "%m-%d-%Y %l:%M:%S %p", localTime );
           
            DBGf( "[IRRIGATION] Watering start time: %s\n", buffer );

            solenoid_closed = false;
        }

   
        time_t now = time(nullptr);

        if( now - lastPrint >= 1 )
        {
            lastPrint = now;

            uint32_t elapsed_sec = uint32_t(now - watering_start_time);

            uint32_t watering_time_remaining = duration - elapsed_sec;
  
            DBGf( "[IRRIGATION] Watering time remaining: %ld sec\n", watering_time_remaining );

            if( watering_time_remaining == 0 )
            {
                watering_needed = false;
            }
    
        }    

    }

#endif

}


void deep_sleep_function( bool watering_needed,  bool &solenoid_closed )
{

/****************************** Stop watering and go to sleep ***********************************/    

    if ( watering_needed == false )  // Only go to sleep if watering is no longer needed
    {
        
        if ( solenoid_closed == false )  // Check if solenoid is open
        {
            digitalWrite( RELAY_PIN, LOW );  // Remove power from solenoid to close
            solenoid_closed = true;  // Update solenoid valve state
            DBG( F( "[LOGIC] Watering NOT required" ) );  // Inform user that watering is not needed
        }

        esp_sleep_enable_timer_wakeup( UPDATE_INTERVAL );  // Initiate CPU sleep cycle
        DBG( F( "[STATUS] ===== Entering Deep Sleep =====" ) );  // Inform user that system is about to go into deep sleep mode
        esp_deep_sleep_start();  // Go into deep sleep
    }

}