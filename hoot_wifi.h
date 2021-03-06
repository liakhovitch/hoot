#ifndef hoot_wifi_h
#define hoot_wifi_h

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>
#include "structs.h"
#include <CircularBuffer.h>
#include "logprn.h"
#include "tx.h"
#include "batt.h"
#include <LoRa.h>

void onTouch();

void wifi_init(char* ssid);

void handle_wifi(Preferences* prefs, struct Settings* settings, CircularBuffer<char, LOGBUF_SIZE>* logbuf);

void wifi_enable(char* ssid);

void wifi_disable();

#endif
