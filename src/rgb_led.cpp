#include "rgb_led.h"


Adafruit_NeoPixel rgb( LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


void setup_RGB()
{
  rgb.begin(); // Initialize NeoPixel library
  rgb_show_color( RED ); // Set LED to RED (indicating system is starting up)
  rgb.show(); // Update the LED strip to show the new color
}


void rgb_show_color( uint32_t color )
{
  rgb.setPixelColor( 0, color ); // Set LED to specified color
  rgb.show(); // Update the LED strip to show the new color

}