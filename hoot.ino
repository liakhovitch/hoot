//#include "heltec.h"
#include <Preferences.h>
#include <CircularBuffer.h>
#include <cstring>
#include "hoot_wifi.h"
#include "structs.h"

Preferences prefs;
CircularBuffer<byte,1024> rxbuf;

#define BAND    915E6

void init_settings(Preferences* prefs, struct Settings* settings){
  if(prefs->getBytesLength("hmsg") == 9){
    prefs->getBytes("hmsg", settings->hmsg, 9);
  } else strcpy(settings->hmsg, "e\0\0\0\0\0\0\0");
  settings->hrcv = prefs->getUChar("hrcv", 0);
  if(prefs->getBytesLength("ssid") == 9){
    prefs->getBytes("ssid", settings->ssid, 9);
  } else strcpy(settings->ssid, "HOOT\0\0\0\0");
  settings->hz = prefs->getUShort("hz", 700);
  settings->rpt = prefs->getUChar("rpt", 3);
  settings->wrd = prefs->getUChar("wrd", 69);
}

void setup() {
  // Init hoot preferences
  struct Settings settings;
  prefs.begin("hoot", false);
  init_settings(&prefs, &settings);
  // Init LoRa
//  Heltec.begin(false /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
//  LoRa.setSyncWord(settings.wrd);
  // Init wifi
  wifi_init(settings.ssid);
  wifi_enable(settings.ssid); // Deleteme

  while(1){
    handle_wifi(&prefs, &settings);
  }
}

void loop() {
}
