#ifndef hoot_wifi_h
#define hoot_wifi_h

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>
#include "structs.h"

void wifi_init(char* ssid);

void handle_wifi(Preferences* prefs, struct Settings* settings);

void wifi_enable(char* ssid);

void wifi_disable();

#endif
