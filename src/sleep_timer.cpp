#include "sleep_timer.h"  // sleep timer helpers, schedule constants, and settings


void printLocalTime( const char *tag )  // Print current local time with tag
{
    time_t now;  // current epoch time (seconds since 1970)
    struct tm timeinfo;  // broken-out local time fields

    time(&now);  // populate now
    localtime_r( &now, &timeinfo );  // convert epoch to local time struct safely

    char buf[32];  // formatted timestamp buffer
    strftime( buf, sizeof( buf ), "%m-%d-%Y %H:%M:%S", &timeinfo );  // write human-readable time

#ifdef DEBUG_ENABLED

    DBGf( "[%s] Local time: %s\r\n", tag, buf );  // log the timestamp with the provided tag

#endif

}


void printTargetTime( time_t target )  // Print next target time
{
    struct tm tm_target;  // target time fields
    localtime_r( &target, &tm_target );  // convert target epoch to local time

    char buf[32];  // buffer for formatted target time
    strftime( buf, sizeof( buf ), "%m-%d-%Y %H:%M:%S", &tm_target );  // format for logging

#ifdef DEBUG_ENABLED

    DBGf( "[TIME] Next target time: %s\r\n", buf );  // log the formatted target time

#endif

}


time_t nextTargetTime()  // Compute next scheduled target time
{
    time_t now;  // current epoch time
    struct tm tm_now;  // current local time struct

    time( &now );  // get epoch
    localtime_r( &now, &tm_now );  // convert to local struct

    for ( size_t i = 0; i < SCHEDULE_COUNT; i++ )  // iterate through today's schedule slots
    {
        struct tm tm_target = tm_now;  // start from now's date

        tm_target.tm_hour = settings.times[i].hour;  // apply scheduled hour
        tm_target.tm_min  = settings.times[i].min;  // apply scheduled minute
        tm_target.tm_sec  = settings.times[i].sec;  // apply scheduled second

        time_t candidate = mktime(&tm_target);  // convert broken-out time to epoch candidate

        // If this candidate is still in the future, it's our next target
        if ( difftime(candidate, now) > 0 )
            return candidate;  // return first future schedule this day
        
    }

    // If all of today's schedule slots passed, schedule the first slot for tomorrow
    struct tm tm_target = tm_now;  // base on now's date
    tm_target.tm_mday += 1;  // move to next day
    tm_target.tm_hour = settings.times[0].hour;  // set to first slot hour
    tm_target.tm_min  = settings.times[0].min;  // set minute
    tm_target.tm_sec  = settings.times[0].sec;  // set second

    return mktime( &tm_target );  // return tomorrow's first target as epoch
}


void deep_sleep_function()  // decide whether to sleep, wait, or continue running until target
{
    printLocalTime("WAKE");  // log wake time

    time_t now = time(nullptr);  // current epoch
    time_t target = nextTargetTime();  // compute next schedule target

    printTargetTime( target );  // log target time

    double seconds_to_target = difftime( target, now );  // seconds until the target

#ifdef DEBUG_ENABLED

    DBGf( "[TIME] Seconds to target: %.0f\r\n", seconds_to_target );  // log remaining seconds

#endif

    // Case 1: Inside active window → stay awake and loop until target
    if ( seconds_to_target <= ACTIVE_WINDOW_SEC )
    {

#ifdef DEBUG_ENABLED

        DBGf( "[POWER] Inside active window (%ds) — staying awake\r\n", ACTIVE_WINDOW_SEC );  // debug note

#endif

        while ( true )
        {
            now = time( nullptr );  // refresh current time
            
            double remaining = difftime( target, now ); // compute remaining seconds

            if ( remaining <= 0 )  // target reached?
                break;  // exit loop when time arrives

#ifdef DEBUG_ENABLED

            DBGf( "[WAIT] %.0f seconds remaining\r\n", remaining );  // log remaining time periodically

#endif
            delay( 950 );  // small delay to avoid tight-looping and allow background tasks
        }

        printLocalTime( "TARGET" );  // log the moment target is reached

#ifdef DEBUG_ENABLED

        DBG( "[POWER] Target reached — continuing program" );  // note continuation

#endif
        return;  // return to normal operation
    }

    // Case 2: Too early → compute sleep duration and enter deep sleep
    double sleep_seconds = seconds_to_target - WAKE_EARLY_BUFFER_SEC;  // subtract early wake buffer

    if ( sleep_seconds < 1 )  // ensure we sleep at least 1 second
        sleep_seconds = 1;

    uint64_t sleep_us = uint64_t( sleep_seconds ) * ONE_SECOND_US;  // convert seconds to microseconds for ESP32

#ifdef DEBUG_ENABLED

    DBGf( "[POWER] Sleeping for %.0f seconds (early buffer = %ds)\r\n", sleep_seconds, WAKE_EARLY_BUFFER_SEC );  // log planned sleep

#endif 

    esp_sleep_enable_timer_wakeup( sleep_us );  // program the wakeup timer

#ifdef DEBUG_ENABLED

    DBG( "[STATUS] ===== Entering Deep Sleep =====" );  // final status log

#endif

    display.clearDisplay();
    display.setCursor(0,0);    
    display.print(F("[STATUS]\r\nEntering Deep Sleep")); 
    display.display();

    esp_deep_sleep_start();  // hand off to hardware deep sleep

}