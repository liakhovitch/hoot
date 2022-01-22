#include "logprn.h"

extern CircularBuffer<char, LOGBUF_SIZE> logbuf;
extern AudioOutputI2S *out;
extern ESP8266SAM *sam;
extern struct Settings settings;

void logbytes(char* msg){
  for(char* c = msg; *c != 0; c++){
    logbuf.push(*c);
  }
}

void logbyte(char c){
   if (c == '\"'){
    logbytes("&#34");
   }
   else if (c == '&'){
     logbytes("&amp");
   }
   else if (c == '\''){
     logbytes("&#39");
   }
   else if (c == '\\'){
     logbytes("&#92");
   }
   else if (c == '<'){
     logbytes("&#60");
   }
   else if (c == '>'){
     logbytes("&#62");
   }
   else if (c == '\n'){
     logbytes("<br>");
   }else if (c == 0){
     return;
   }
   else logbuf.push(c);
}

void logprn(char* msg){
  for(char* c = msg; *c != 0; c++){
    logbyte(*c);
  }
}

void logprnln(char* msg){
  logprn(msg);
  logprn("\n");
}

void err(char* msg){
  logprn("Error: ");
  logprnln(msg);
  digitalWrite(SND_ENABLE, HIGH);
  out -> SetGain(VOL_SAM * settings.mainvol * settings.ttsvol);
  sam->Say(out, "Error!");
  sam->Say(out, msg);
  digitalWrite(SND_ENABLE, LOW);
}

void err(char* msg, char* msg_sam){
  logprn("Error: ");
  logprnln(msg);
  digitalWrite(SND_ENABLE, HIGH);
  out -> SetGain(VOL_SAM * settings.mainvol * settings.ttsvol);
  sam->Say(out, "Error!");
  sam->Say(out, msg_sam);
  digitalWrite(SND_ENABLE, LOW);
}

void announce(char* msg){
  logprnln(msg);
  digitalWrite(SND_ENABLE, HIGH);
  out -> SetGain(VOL_SAM * settings.mainvol * settings.ttsvol);
  sam->Say(out, msg);
  digitalWrite(SND_ENABLE, LOW);
}

void announce(char* msg, char* msg_sam){
  logprnln(msg);
  digitalWrite(SND_ENABLE, HIGH);
  out -> SetGain(VOL_SAM * settings.mainvol * settings.ttsvol);
  sam->Say(out, msg_sam);
  digitalWrite(SND_ENABLE, LOW);
}

void nlogprn(char* msg, size_t len){
  for(size_t i = 0; i < len; i++){
    if(*(msg+i) == 0) return;
    logbyte(*(msg+i));
  }
}
