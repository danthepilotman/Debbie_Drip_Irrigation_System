#include "sleep_timer.h"


void printLocalTime(const char *tag)
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    char buf[32];
    strftime(buf, sizeof(buf), "%m-%d-%Y %H:%M:%S", &timeinfo);

    DBGf("[%s] Local time: %s\n", tag, buf);
}


void printTargetTime(time_t target)
{
    struct tm tm_target;
    localtime_r(&target, &tm_target);

    char buf[32];
    strftime(buf, sizeof(buf), "%m-%d-%Y %H:%M:%S", &tm_target);

    DBGf("[TIME] Next target time: %s\n", buf);
}


time_t nextTargetTime()
{
    time_t now;
    struct tm tm_now;

    time(&now);
    localtime_r(&now, &tm_now);

    for (size_t i = 0; i < SCHEDULE_COUNT; i++)
    {
        struct tm tm_target = tm_now;

        tm_target.tm_hour = SCHEDULE[i].hour;
        tm_target.tm_min  = SCHEDULE[i].min;
        tm_target.tm_sec  = SCHEDULE[i].sec;

        time_t candidate = mktime(&tm_target);

        // First schedule time still in the future
        if (difftime(candidate, now) > 0)
        {
            return candidate;
        }
    }

    // All today's schedule times have passed → first one tomorrow
    struct tm tm_target = tm_now;
    tm_target.tm_mday += 1;
    tm_target.tm_hour = SCHEDULE[0].hour;
    tm_target.tm_min  = SCHEDULE[0].min;
    tm_target.tm_sec  = SCHEDULE[0].sec;

    return mktime(&tm_target);
}


void deep_sleep_function()
{
    printLocalTime("WAKE");

    time_t now = time(nullptr);
    time_t target = nextTargetTime();

    printTargetTime(target);

    double seconds_to_target = difftime(target, now);

    DBGf("[TIME] Seconds to target: %.0f\n", seconds_to_target);

    // Case 1: Inside active window → stay awake
    if (seconds_to_target <= ACTIVE_WINDOW_SEC)
    {
        DBGf("[POWER] Inside active window (%ds) — staying awake\n",
             ACTIVE_WINDOW_SEC);

        while (true)
        {
            now = time(nullptr);
            double remaining = difftime(target, now);

            if (remaining <= 0)
                break;

            DBGf("[WAIT] %.0f seconds remaining\n", remaining);
            delay(500);
        }

        printLocalTime("TARGET");
        DBG("[POWER] Target reached — continuing program");
        return;
    }

    // Case 2: Too early → sleep with early wake buffer
    double sleep_seconds = seconds_to_target - WAKE_EARLY_BUFFER_SEC;

    if (sleep_seconds < 1)
        sleep_seconds = 1;

    uint64_t sleep_us = uint64_t(sleep_seconds) * ONE_SECOND_US;

    DBGf("[POWER] Sleeping for %.0f seconds (early buffer = %ds)\n",
         sleep_seconds, WAKE_EARLY_BUFFER_SEC);

    esp_sleep_enable_timer_wakeup(sleep_us);

    DBG("[STATUS] ===== Entering Deep Sleep =====");
    esp_deep_sleep_start();
}
