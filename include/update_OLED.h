#ifndef UPDATE_OLED_H
#define UPDATE_OLED_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "helper.h"  // ThingSpeak helpers: upload, TalkBack parsing, and settings


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32



extern Adafruit_SSD1306 display;  // Declare display as external so other .cpp files can see it

void setup_OLED(); // Initialize OLED display
void update_Display();  // Update OLED display based on current page
void wifi_Page();  // Update OLED display with current WiFi status
void status_Page();  // Update OLED display with current status and readings
void soil_Page();  // Update OLED display with current status and readings
void settings_Page();  // Update OLED display with current status and readings

#endif