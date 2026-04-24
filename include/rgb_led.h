#ifndef RGB_LED_H
#define RGB_LED_H

#include <Arduino.h>  // Arduino core
#include <Adafruit_NeoPixel.h>

#define LED_PIN 48
#define LED_COUNT 1


#define RED rgb.Color(255, 0, 0)  // Red color for RGB LED
#define GREEN rgb.Color(0, 255, 0)  // Green color for RGB LED
#define BLUE rgb.Color(0, 0, 255)  // Blue color for RGB LED
#define BLACK_LED rgb.Color(0, 0, 0)  // Off state for RGB LED    
#define YELLOW rgb.Color(255, 255, 0)  // Yellow color for RGB LED
#define CYAN rgb.Color(0, 255, 255)  // Cyan color for RGB LED
#define MAGENTA rgb.Color(255, 0, 255)  // Magenta color
#define WHITE_LED rgb.Color(255, 255, 255)  // White color for RGB LED
#define ORANGE rgb.Color(255, 165, 0)  // Orange color for RGB LED
#define PURPLE rgb.Color(128, 0, 128)  // Purple color for RGB LED
#define PINK rgb.Color(255, 192, 203)  // Pink color for RGB LED


extern Adafruit_NeoPixel rgb;
      
void setup_RGB(); // Initialize RGB LED strip
void rgb_show_color( uint32_t color ); // Helper function to set RGB LED color and update strip

#endif