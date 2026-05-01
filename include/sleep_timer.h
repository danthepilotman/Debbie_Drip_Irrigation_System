#ifndef SLEEP_TMR_H  // header guard start
#define SLEEP_TMR_H  // header guard definition


#include "setup.h"  // i    nclude common project definitions and types


constexpr uint16_t ACTIVE_WINDOW_SEC = 61;   // seconds to stay awake inside active window
constexpr uint16_t WAKE_EARLY_BUFFER_SEC = 60;   // wake at least this many seconds early
#define ONE_SECOND_US          1000000ULL  // microseconds per second constant


// ==================================================
// ========= Prototype Functions ===========
// ==================================================
void printLocalTime(const char *tag); // Print local time with an identifying tag
void printTargetTime(time_t target); // Print a formatted target/wake time
time_t nextTargetTime(); // Compute next scheduled wake time (returns epoch)
void deep_sleep_function(); // Enter deep sleep according to schedule/logic
void handle_sleep_state(); // Main function to call in loop to manage sleep/wake behavior


#endif // SLEEP_TMR_H