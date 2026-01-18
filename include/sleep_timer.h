#ifndef SLEEP_TMR_H
#define SLEEP_TMR_H


#include "setup.h"


typedef struct
{
    int hour;
    int min;
    int sec;
} ScheduleTime;

static ScheduleTime SCHEDULE[] = {
    { 0,  0, 0 },  // HH:MM:SS
    { 6,  0, 0 },
    { 12, 0, 0 },
    { 18, 0, 0 }
};

static const size_t SCHEDULE_COUNT =
    sizeof(SCHEDULE) / sizeof(SCHEDULE[0]);

#define ACTIVE_WINDOW_SEC      60   // Stay awake inside this window
#define WAKE_EARLY_BUFFER_SEC  30   // Wake at least this early
#define ONE_SECOND_US          1000000ULL


// ==================================================
// ========= Prototype Functions ===========
// ==================================================
void printLocalTime(const char *tag);
void printTargetTime(time_t target);
time_t nextTargetTime();
void deep_sleep_function();

#endif