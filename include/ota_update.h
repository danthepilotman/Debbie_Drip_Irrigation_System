#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <Arduino.h>

extern const char *MANIFEST_URL;
extern const char *FIRMWARE_VERSION;


bool getFirmwareInfo( String &latestVersion, String &firmwareUrl );  // get latest firmware info from ThingSpeak
bool isNewerVersion( const String &latestVersion );  // compare firmware versions
void performOTA( String url );  // perform OTA update from URL
void checkForOTAUpdate();  // check for OTA update and perform if available
void check_ota_state();  // check OTA state and update flags accordingly

#endif