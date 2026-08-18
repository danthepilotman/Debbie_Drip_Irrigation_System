#ifndef LAMP_SERVER_H
#define LAMP_SERVER_H


#include "setup.h"  // Need for Json, debug macros, and LittleFS


extern const char* postErrorLogServerName;

bool getServerSettings();  // download and apply server settings
bool serverSettingsAreNewer( const char *serverUpdated );  // compare server settings timestamp with local
void applyDownloadedSettings( JsonDocument &server_doc );  // apply downloaded settings to local settings
bool initFlashFS();  // initialize LittleFS
bool applyLocalSettings();  // load settings from local FS
bool saveLocalSettings( JsonDocument &server_doc );  // save settings to FS
void sendServerUpdate(); // send update to server
void solenoid_state_Update();  // report solenoid state
void sendServerUpdate();  // send soil readings updates to server
void logError( const char* errortext ); // log error to error log file
bool uploadErrorLog(); // Upload error log file to LAMP server

#endif