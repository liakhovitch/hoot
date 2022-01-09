#ifndef logprn_h
#define logprn_h

#include "structs.h"
#include <CircularBuffer.h>
#include <ESP8266SAM.h>
#include "AudioOutputI2S.h"

void logbyte(char c);

void logprn(char* msg);

void logprnln(char* msg);

void err(char* msg);

void err(char* msg, char* msg_sam);

void announce(char* msg);

void announce(char* msg, char* msg_sam);

void nlogprn(char* msg, size_t len);

#endif
