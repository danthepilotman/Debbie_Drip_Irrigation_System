#ifndef SLEEP_TMR_H
#define SLEEP_TMR_H


#include "setup.h"


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