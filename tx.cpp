#include "tx.h"

extern CircularBuffer<char, TXBUF_SIZE> txbuf;
extern int flush;

void txchar(char c){
  if(!txbuf.push(c)){
    err("Transmit buffer overflow.");
  }
}

void txprint(char* s){
  for(char* c = s; *c != 0; c++){
    txchar(*c);
  }
}

void txflush(){
  flush = 1;
}
