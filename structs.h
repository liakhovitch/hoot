#ifndef STRUCTS_H
#define STRUCTS_H

#define RXBUF_SIZE 1024
#define LOGBUF_SIZE 16384
#define TXBUF_SIZE 1024
#define SND_ENABLE 36

#define VOL_HOOT 0.4
#define VOL_BEEP 0.5
#define VOL_SAM  0.3

#define BEEP_STARTUP 5

#include <Arduino.h>

struct Settings {
  char     hmsg[9]; // Hoot message
  uint8_t  hrcv;    // Hoot on receipt
  char     ssid[9];
  float    hzrx;    // Beep tone
  uint8_t  rxbp;    // Beep out received messages?
  float    hztx;    // Beep tone (tx echo)
  uint8_t  txbp;    // Beep out transmitted messages?
  float    vol;
  uint8_t  hootsnd; // Alternate hoot sound
  uint16_t rxunit;  // Length of beep for RX, in milliseconds
  uint8_t  wrd;     // Sinc word
  uint8_t  pack;    // Packet size
  uint8_t  pwr;     // TX power (2-20)
  uint8_t  sprd;    // Spreading factor (7-12)
};

// Things to modify when adding new settings:
// - This
// - init_settings in main
// - store_params in wifi.cpp
// - parser
// - html

#endif
