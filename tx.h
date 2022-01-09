#ifndef TX_H
#define TX_H

#include "structs.h"
#include <CircularBuffer.h>
#include "logprn.h"

void txchar(char c);

void txprint(char* s);

void txflush();

#endif
