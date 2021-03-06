#ifndef STRUCTS_H
#define STRUCTS_H

#define RXBUF_SIZE 1024
#define LOGBUF_SIZE 16384
#define TXBUF_SIZE 1024
#define SND_ENABLE 22

#define VOL_HOOT 0.4
#define VOL_BEEP 0.5
#define VOL_SAM  0.3

#define BEEP_STARTUP 5

#include <Arduino.h>

struct Settings {
  char     hmsg[9]; // Hoot message
  uint8_t  hrcv;    // Hoot on receipt
  char     ssid[9]; // SSID
  uint8_t  wiboot;  // Enable WiFi on boot?
  uint8_t  repeat;  // Repeater (range test) mode?
  float    hzrx;    // Beep tone
  uint8_t  rxbp;    // Beep out received messages?
  float    hztx;    // Beep tone (tx echo)
  uint8_t  txbp;    // Beep out transmitted messages?
  float    mainvol; // Main volume
  float    ttsvol;  // TTS volume adjustment
  float    hootvol; // HOOT volume adjustment
  uint8_t  hootsnd; // Alternate hoot sound
  uint16_t rxunit;  // Length of beep for RX, in milliseconds
  uint16_t dashth;  // Dit-to-dash threshold
  uint16_t wifith;  // Dash-to-wifi threshold
  uint16_t symth;   // Intra-symbol to inter-symbol threshhold
  uint16_t wordth;  // Inter-symbol to inter-word threshhold
  uint16_t endth;   // Inter-word to message end threshold
  uint8_t  tth;     // Touch threshold
  uint8_t  tdb;     // Touch debounce amount
  uint8_t  wrd;     // Sinc word
  uint8_t  pack;    // Packet size
  uint8_t  pwr;     // TX power (2-20)
  uint8_t  sprd;    // Spreading factor (7-12)
  float    battc;   // Battery monitoring calibration factor
};

// Things to modify when adding new settings:
// - This
// - init_settings in main
// - store_params in wifi.cpp
// - parser
// - html

#endif
