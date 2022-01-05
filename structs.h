#ifndef STRUCTS_H
#define STRUCTS_H

struct Settings {
  char     hmsg[9]; // Hoot message
  uint8_t  hrcv;    // Hoot on receipt
  char     ssid[9];
  uint16_t hz;      // Beep tone
  uint8_t  rpt;     // Max send retry count
  uint8_t  wrd;     // Sinc word
};

#endif
