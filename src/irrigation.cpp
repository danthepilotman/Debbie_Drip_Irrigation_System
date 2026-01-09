#include "irrigation.h"
#include "thingspeak.h"

/******************************* Get SOIL sensor readings and update ThingSpeak *********************/   
bool get_new_readings()
{

    bool success = true;  // Assume true to allow for function to return true in cases where watering_needed == true
    
    if ( watering_needed_ESP32 == NO)  // Don't keep getting new readings if watering is not needed
    {
        DBG( F( "[STATUS] ===== SYSTEM CYCLE START =====" ) );

        // -------- Read Soil Sensor --------
        DBG( F( "[RS485] Reading soil sensor" ) );

        uint16_t values[7]; // Store 7 register values

        RS485_STATUS status = read_Registers( RS485Serial, 0x01, 0x0000, 5, values );

        if ( status != RS485_GOOD )
            DBG(  F( "[RS485][ERROR] Modbus error" ) );
        
    

        uint16_t rawMoisture = values[ SOIL_MOISTURE ];
        uint16_t rawTemp     = values[ SOIL_TEMPERATURE ];
        uint16_t rawEC       = values[ SOIL_EC];
        uint16_t rawPH       = values[ SOIL_PH ];
        uint16_t rawN        = values[ SOIL_N ];
        uint16_t rawP        = values[ SOIL_P ];
        uint16_t rawK        = values[ SOIL_K ];

        float moisture = rawMoisture / 10.0;
        float temp     = int16_t ( rawTemp ) / 10.0;
        float ec       = rawEC;
        float ph       = rawPH / 10.0;

        DBGf( "[DATA] Moisture: %.1f %%\n", moisture );
        DBGf( "[DATA] Temp: %.1f °C\n", temp );
        DBGf( "[DATA] EC: %.0f µS/cm\n", ec );
        DBGf( "[DATA] pH: %.1f\n", ph );
        DBGf( "[DATA] NPK: %u / %u / %u mg/kg\n", rawN, rawP, rawK );

        // -------- ThingSpeak Upload --------
        time_t status_time_ESP32 = time(nullptr);

        success = ( status ==  RS485_GOOD ) && sendThingSpeak( moisture, temp, ec, ph, rawN, rawP, rawK, status_time_ESP32 );

        // -------- Read Control Settings --------

        uint8_t threshold;  // Water content by volume percentage minimum threshold to trigger watering cycle

        bool rain_expected_TS, watering_needed_TS;

        delay( 60000 );
        
        time_t status_time_TS;
        
        success = success && getSettings( threshold, duration, rain_expected_TS, watering_needed_TS, status_time_TS );

        success = success && ( status_time_ESP32 == status_time_TS );

        // -------- Weather Check --------
        bool rain_expected_ESP32 = rainExpectedSoon();

        DBGf( "[LOGIC] Rain expected soon: %s\n", rain_expected_ESP32 ? "YES" : "NO" );

        if ( moisture < threshold && rain_expected_ESP32 == false )
            watering_needed_ESP32 = YES;

        if ( ( rain_expected_ESP32 == rain_expected_TS ) && ( watering_needed_ESP32 == watering_needed_TS ) && ( status_time_ESP32 == status_time_TS ) )
            Serial.println( F( "[LOGIC] Local ESP32 & ThingSpeak rain, watering, status match") );

        return success;
    
    }

    else
        return success;
}


void  water_soil()
{

    static time_t watering_start_time;  // Record timestamp when watering started

    static time_t lastPrint;  // Remember timestamp of last serial print

    /************************ Continue watering if needed ******************************/    
     
    if ( watering_needed_ESP32 == YES )
    {

        if( solenoid_state == OFF)
        {
            DBG( F( "[LOGIC] Watering conditions MET" ) );
  
            digitalWrite( RELAY_PIN, HIGH );  // Open solenoid valve

            solenoid_state = ON;

            solenoid_state_Update();

            time( &watering_start_time );  // Record timestamp when watering started

            lastPrint = watering_start_time;

            struct tm* localTime = localtime( &watering_start_time );

            char buffer[30];

            strftime( buffer, sizeof(buffer), "%m-%d-%Y %l:%M:%S %p", localTime );
           
            DBGf( "[IRRIGATION] Watering start time: %s\n", buffer );

            
        }

        time_t now = time(nullptr);

        if( now - lastPrint >= 1 )
        {
            lastPrint = now;

            uint32_t elapsed_sec = uint32_t(now - watering_start_time);

            uint32_t watering_time_remaining = duration - elapsed_sec;
  
            DBGf( "[IRRIGATION] Watering time remaining: %ld sec\n", watering_time_remaining );

            if( watering_time_remaining == 0 )  // Check if watering cyle has completed
            {
                
                digitalWrite( RELAY_PIN, LOW );  // Remove power from solenoid to close

                solenoid_state = OFF;  // Update solenoid valve state

                watering_needed_ESP32 = NO;

                solenoid_state_Update();

                DBG( F( "[LOGIC] Watering NOT required" ) );  // Inform user that watering is not needed

            }
    
        }    

    }

}