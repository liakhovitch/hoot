
#include <Preferences.h>
#define CIRCULAR_BUFFER_INT_SAFE
#include <CircularBuffer.h>
#include <cstring>
#include "hoot_wifi.h"
#include "structs.h"
#include "AudioFileSourcePROGMEM.h"
#include "AudioGeneratorWAV.h"
#include "AudioFileSourceFunction.h"
#include <ESP8266SAM.h>
#include "AudioOutputI2S.h"
#include "hoo.h"
#include "whistle.h"
#include "logprn.h"
#include "morse.h"
#include <SPI.h>
#include <LoRa.h>

#define VERSION 0.1
#define Bit_Clock_BCLK 13
#define Word_Select_WS 17
#define Serial_Data_SD 12

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

Preferences prefs;
struct Settings settings;

AudioGeneratorWAV *wav = NULL;
AudioFileSourcePROGMEM *file_hoot = NULL;
AudioFileSourceFunction *file_beep = NULL;
AudioOutputI2S *out;
ESP8266SAM *sam = NULL;
CircularBuffer<char, RXBUF_SIZE> rxbuf;
CircularBuffer<char, LOGBUF_SIZE> logbuf;
CircularBuffer<char, TXBUF_SIZE> txbuf;
float hz;

// TX related variables
int flush = 0;
int sending = 0;

// RX related variables
char rxchar = 0;
uint8_t rxidx = 0;
unsigned long sch_stop = 0;
unsigned long sch = 0;

int state = 0; // 0 = idle, 1 = rx, 2 = tx

extern int wifi_on;

float sine_wave(const float time) {
  float v = sin(TWO_PI * hz * time);  // C
  v *= 0.5;                           // scale
  return v;
};

void handle_sound(){
  if (wav && wav->isRunning()) {
    if (!wav->loop()){
      stop_audio();
    }
  } else if (wav) {
    stop_audio();
  }
}

// Deletes all potential audio objects
void stop_audio(){
  if(file_hoot) {file_hoot->close(); delete file_hoot; file_hoot = NULL;}
  if(file_beep) {file_beep->close(); delete file_beep; file_beep = NULL;}
  if(wav) {wav->stop(); delete wav; wav = NULL;}
  digitalWrite(SND_ENABLE, LOW);
}

void play_hoot(){
  stop_audio();
  out -> SetGain(VOL_HOOT * settings.vol);
  switch(settings.hootsnd){
    case 0:
      file_hoot = new AudioFileSourcePROGMEM( hoo_wav, sizeof(hoo_wav) );
      break;
    case 1:
      file_hoot = new AudioFileSourcePROGMEM( whistle_wav, sizeof(whistle_wav) );
      break;
    default:
      file_hoot = new AudioFileSourcePROGMEM( hoo_wav, sizeof(hoo_wav) );
      break;
  }
  wav = new AudioGeneratorWAV();
  wav->begin(file_hoot, out);
  digitalWrite(SND_ENABLE, HIGH);
  logprnln("HOOT");
}

void beep(float len, float freq){
  stop_audio();
  hz = freq;
  out -> SetGain(VOL_BEEP  * settings.vol);
  file_beep = new AudioFileSourceFunction(len);
  file_beep->addAudioGenerators(sine_wave);
  wav = new AudioGeneratorWAV();
  wav->begin(file_beep, out);
  digitalWrite(SND_ENABLE, HIGH);
}

void beep_sch(){
  if (rxchar == ' ' || rxchar == '\n'){
    sch = millis() + (settings.rxunit * 6);
    return;
  }
  char symbol = *(morse(rxchar)+rxidx);
  if(symbol == '-'){
    beep(2, settings.hzrx);
    sch_stop = millis() + (settings.rxunit * 3) + BEEP_STARTUP;
    sch = millis() + (settings.rxunit * 4);
  }
  else if (symbol == '.'){
    beep(2, settings.hzrx);
    sch_stop = millis() + (settings.rxunit * 1) + BEEP_STARTUP;
    sch = millis() + (settings.rxunit * 2);
  }
  else{
    sch = millis() + (settings.rxunit * 2);
  }
}

void init_settings(Preferences* prefs, struct Settings* settings){
  if(prefs->getBytesLength("hmsg") == 9){
    prefs->getBytes("hmsg", settings->hmsg, 9);
  } else strcpy(settings->hmsg, "e\0\0\0\0\0\0\0");
  settings->hrcv = prefs->getUChar("hrcv", 0);
  if(prefs->getBytesLength("ssid") == 9){
    prefs->getBytes("ssid", settings->ssid, 9);
  } else strcpy(settings->ssid, "HOOT\0\0\0\0");
  settings->hzrx = prefs->getFloat("hzrx", 700);
  settings->rxbp = prefs->getUChar("rxbp", 1);
  settings->hztx = prefs->getFloat("hztx", 1000);
  settings->txbp = prefs->getUChar("txbp", 1);
  settings->vol = prefs->getFloat("vol", 1.0);
  settings->hootsnd = prefs->getUChar("hootsnd", 0);
  settings->rxunit = prefs->getULong("rxunit", 10);
  settings->wrd = prefs->getUChar("wrd", 69);
  settings->pack = prefs->getUChar("pack", 50);
  settings->pwr = prefs->getUChar("pwr", 20);
  settings->sprd = prefs->getUChar("sprd", 10);
}

void setup() {
  Serial.begin(115200);
  logprnln("Booting HOOT v" STRINGIFY(VERSION) ".");
  Serial.println("Booting HOOT v" STRINGIFY(VERSION) ".");
  // Init hoot preferences
  logprnln("Initializing preferences...");
  Serial.println("Initializing preferences...");
  prefs.begin("hoot", false);
  init_settings(&prefs, &settings);
  Serial.println("Prefs initialized.");
  
  // Init sound
  logprnln("Initializing audio...");
  Serial.println("Initializing audio...");
  pinMode(SND_ENABLE, OUTPUT);
  digitalWrite(SND_ENABLE, LOW);
  out = new AudioOutputI2S();
  out -> SetGain(VOL_HOOT * settings.vol);
  out -> SetPinout(Bit_Clock_BCLK,Word_Select_WS,Serial_Data_SD);
  sam = new ESP8266SAM;
  logprnln("Audio initialized.");
  Serial.println("Audio initialized.");

  // Init wifi
  logprnln("Initializing WiFi...");
  Serial.println("Initializing WiFi...");
  wifi_init(settings.ssid);
  logprnln("WiFi initialized.");
  Serial.println("Wifi initialized.");

  logprnln("Initializing LoRa...");
  Serial.println("Initializing LoRa...");
  LoRa.setPins(18, 14, 26);
  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setSyncWord(settings.wrd);
  LoRa.setTxPower(settings.pwr);
  LoRa.setSpreadingFactor(settings.sprd);
  LoRa.onTxDone(onTxDone);
  LoRa.onReceive(onReceive);
  LoRa.receive();
  logprnln("LoRa initialized.");
  Serial.println("LoRa initialized.");
  
  logprnln("System startup complete.");
  Serial.println("System startup complete.");

  wifi_enable(settings.ssid); // Deleteme

  // Boot sound
  //beep(0.4, settings.hzrx);
  play_hoot();
}

void onReceive(int size) {
  logprn("RX: ");
  while(LoRa.available()){
    char c = LoRa.read();
    logbyte(c);
    if(!rxbuf.push(c)){
      err("Receive buffer overflow.");
    }
  }
  logprn("\n");
}

void onTxDone() {
  sending = 0;
  LoRa.receive();
}

void handle_tx(){
  if(sending == 0){
    if(flush == 1 || txbuf.size() >= settings.pack){
      if (LoRa.beginPacket()){
        logprn("TX: ");
        const size_t avail = txbuf.size();
        size_t j = 0;
        for(int i = 0; i < avail; i++){
          if(j >= settings.pack) break;
          if(txbuf.size() == 0) break;
          char c = txbuf.shift();
          LoRa.write(c);
          logbyte(c);
          j++;
        }
        LoRa.endPacket(true);
        logprn("\n");
        sending = 1;
        if(txbuf.size() == 0) flush = 0;
      } else {
        err("LoRa radio unavailable for transmission.");
      }
    }
  }
}

void handle_rx_beep(){
  // If user is entering text, wait until they are done
  if(state == 2) return;
  size_t bufsize = rxbuf.size();
  if(state == 0 && bufsize > 0){
    if(settings.hrcv && bufsize == strlen(settings.hmsg)){
      char flag = 0;
      for(int i = 0; i < bufsize; i++){
        if(rxbuf[i] != settings.hmsg[i]){
          flag = 1;
          break;
        }
      }
      if(flag == 0){
        for(int i = 0; i < bufsize; i++){
          rxbuf.shift();
        }
        play_hoot();
        return;
      }
    }
    // If we aren't beeping out the messages, clear the RX buffer and move on
    // Note: we aren't calling clear() because that might also clear messages that have come in while this function has been running.
    if(!settings.rxbp){
      for(int i = 0; i < bufsize; i++){
          rxbuf.shift();
      }
      return;
    }
    state = 1;
    rxchar = rxbuf.shift();
    rxidx = 0;
    beep_sch();
  }
  if(state == 1){
    if(wav && millis() > sch_stop) stop_audio();
    if (millis() < sch) return;
    if(*(morse(rxchar)+rxidx) == 0){
      rxidx = 0;
      if(rxbuf.size() == 0){
        state = 0;
        return;
      } else {
        rxchar = rxbuf.shift();
      }
    } else{
      rxidx++;
    }
    beep_sch();
  }
}

void loop() {
    handle_sound();
    if(wifi_on) handle_wifi(&prefs, &settings, &logbuf);
    handle_tx();
    handle_rx_beep();
}
