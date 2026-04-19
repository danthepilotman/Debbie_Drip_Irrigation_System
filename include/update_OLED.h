#ifndef UPDATE_OLED_H
#define UPDATE_OLED_H

#include "setup.h"  // project-wide definitions and prototypes
#include "helper.h"  // ThingSpeak helpers: upload, TalkBack parsing, and settings

void update_Display();
void wifi_OLED();  // Update OLED display with current WiFi status
void status_OLED();  // Update OLED display with current status and readings
void soil_OLED();  // Update OLED display with current status and readings
void settings_OLED();  // Update OLED display with current status and readings
bool check_for_changes();  // Check for changes to global variables

#endif