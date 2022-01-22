
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
#include "batt.h"
#include <SPI.h>
#include <LoRa.h>

#define VERSION 1.1
#define Bit_Clock_BCLK 23
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

// Touch related variables
char touchbuf[8];
size_t touchbuf_idx = 0;
unsigned long last_touch;
byte touch_state;

int state = 0; // 0 = idle, 1 = rx, 2 = tx

int sleep_counter = 0;

byte safemode = 0;
byte safemode_handled = 0;

extern int wifi_on;

float sine_wave(const float time) {
  float v = sin(TWO_PI * hz * time);  // C
  v *= 0.5;                           // scale
  return v;
};

// 0: no touch, 1: touch, 2: touch (debounce)
byte touch_measure(){
  int touchval = 0;
  // Take highest of four measurements
  for(byte i = 0; i < 8; i++){
    int newval = touchRead(T2);
    if (newval > touchval) touchval = newval;
  }
  if (touchval < (settings.tth - settings.tdb)) return 2;
  else if (touchval < settings.tth) return 1;
  else return 0;
}

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
  out -> SetGain(VOL_HOOT * settings.mainvol * settings.hootvol);
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
  out -> SetGain(VOL_BEEP  * settings.mainvol);
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

void handle_safemode(){
  if(!safemode_handled && millis() > 2000){
    prefs.putUChar("safecnt", 0);
    safemode_handled = 1;
  }
  if(safemode){
    if(touch_measure() >= 2){
      if(touch_state == 0){
        touch_state = 1;
        beep(20, settings.hztx);
      }
    }else{
      if(touch_state == 1){
        touch_state = 0;
        stop_audio();
      }
    }
  }
}


void onReceive(int size) {
  logprn("RX: ");
  while(LoRa.available()){
    char c = LoRa.read();
    logbyte(c);
    if(settings.repeat) txchar(c);
    if(!rxbuf.push(c)){
      err("Receive buffer overflow.");
    }
  }
  logprn("\n");
  if(settings.repeat){ flush = 1;}
}

void onTxDone() {
  sending = 0;
  LoRa.receive();
}

void handle_tx(){
  if(sending == 0){
    if(flush == 1 || txbuf.size() >= settings.pack){
      if (LoRa.beginPacket()){
        if(settings.repeat){ delay(1000);}
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

byte commit_symbol(){
  touchbuf[touchbuf_idx] = 0;
  char res = 0;
  for(size_t i = 0; i < NUM_VALID_CHARS; i++){
    if(strcmp(touchbuf, morse(valid_chars[i])) == 0){
      res = valid_chars[i];
      break;
    }
  }
  touchbuf_idx = 0;
  if (res == 0){
    txbuf.clear();
    state = 0;
    err("Invalid character.");
    return 1;
  } else{
    txchar(res);
    return 0;
  }
}

void handle_touch(){
  int measurement = touch_measure();
  if(measurement >= 2 && state < 2){
    // State transition - clear everything
    rxbuf.clear();
    rxidx = 0;
    sch = 0;
    sch_stop = 0;
    state = 2;
    last_touch = millis();
    txbuf.clear();
    touch_state = 1;
    touchbuf_idx = 0;
    if(settings.txbp) beep(2, settings.hztx);
    return;
  } else if(measurement == 0 && state < 2){
    touch_state = 0;
  }
  if(state == 2){
    if(touch_state == 1){
      if(measurement == 0){
        // Going from touch to no-touch
        if(settings.txbp) stop_audio();
        touch_state = 0;
        unsigned long last_touch_prev = last_touch;
        last_touch = millis();
        if(touchbuf_idx >= 7){
              txbuf.clear();
              state = 0;
              err("Symbol too long.");
              touchbuf_idx = 0;
              return;
        }
        if(last_touch - last_touch_prev < settings.dashth){
          // Dit
          touchbuf[touchbuf_idx] = '.';
        } else{
          // Dash
          touchbuf[touchbuf_idx] = '-';
        }
        touchbuf_idx++;
        return;
      }else{
        // Still holding down the button
        if(millis()-last_touch >= settings.wifith){
          txbuf.clear();
          touchbuf_idx = 0;
          state = 0;
          if(wifi_on){
            wifi_disable();
          } else {
            wifi_enable(settings.ssid);
          }
        }
        else return;
      }
    }else{
      if(touch_measure() >= 2){
        // Going from no-touch to touch
        if(settings.txbp) beep(2, settings.hztx);
        touch_state = 1;
        unsigned long last_touch_prev = last_touch;
        last_touch = millis();
        if(last_touch - last_touch_prev < settings.symth){
          // Intra-symbol
          return;
        } else if(last_touch - last_touch_prev < settings.wordth){
          // Inter-symbol
          commit_symbol();
          return;
        } else{
          // Inter-word
          if(commit_symbol()){
            return;
          }
          txchar(' ');
          return;
        }
      }
      else{
        // Still not pressing the button
        if(millis()-last_touch >= settings.endth){
          // End message
          if(commit_symbol()){
            return;
          } else{
            // Transition back to state 1
            state = 0;
            // Flush tx buffer
            flush = 1;
            return;
          }
        }
        else return;
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

void handle_sleep(){
  if(state == 0 && wifi_on == 0 && sending == 0 && flush == 0 && safemode_handled == 1 && wav == NULL && txbuf.size() == 0 && rxbuf.size() == 0){
    if(sleep_counter >= 4){
      sleep_counter = 0;
      esp_light_sleep_start();
    }
    else sleep_counter++;
  } else sleep_counter = 0;
}


void init_settings(Preferences* prefs, struct Settings* settings){
  if(prefs->getBytesLength("hmsg") == 9){
    prefs->getBytes("hmsg", settings->hmsg, 9);
  } else strcpy(settings->hmsg, "e\0\0\0\0\0\0\0");
  settings->hrcv = prefs->getUChar("hrcv", 0);
  if(prefs->getBytesLength("ssid") == 9){
    prefs->getBytes("ssid", settings->ssid, 9);
  } else strcpy(settings->ssid, "HOOT\0\0\0\0");
  settings->wiboot = prefs->getUChar("wiboot", 0);
  settings->repeat = 0;
  settings->hzrx = prefs->getFloat("hzrx", 700);
  settings->rxbp = prefs->getUChar("rxbp", 1);
  settings->hztx = prefs->getFloat("hztx", 1000);
  settings->txbp = prefs->getUChar("txbp", 1);
  settings->mainvol = prefs->getFloat("mainvol", 1.0);
  settings->ttsvol = prefs->getFloat("ttsvol", 1.0);
  settings->hootvol = prefs->getFloat("hootvol", 1.0);
  settings->hootsnd = prefs->getUChar("hootsnd", 0);
  settings->rxunit = prefs->getULong("rxunit", 60);
  settings->dashth = prefs->getULong("dashth", 120);
  settings->wifith = prefs->getULong("wifith", 5000);
  settings->symth = prefs->getULong("symth", 120);
  settings->wordth = prefs->getULong("wordth", 300);
  settings->endth = prefs->getULong("endth", 1200);
  settings->tth = prefs->getUChar("tth", 45);
  settings->tdb = prefs->getUChar("tdb", 10);
  settings->wrd = prefs->getUChar("wrd", 69);
  settings->pack = prefs->getUChar("pack", 50);
  settings->pwr = prefs->getUChar("pwr", 20);
  settings->sprd = prefs->getUChar("sprd", 10);
  settings->battc = prefs->getFloat("battc", 1.0);
}

void setup() {
  Serial.begin(115200);
  logprnln("Booting HOOT v" STRINGIFY(VERSION) ".");
  Serial.println("Booting HOOT v" STRINGIFY(VERSION) ".");

  // Note- this is sort-f split into two parts because the counter has to be incremented ASAFP,
  //       but the announcement can only be made after audio is initialized.
  logprnln("Handling safemode...");
  Serial.println("Handling safemode...");
  prefs.begin("hoot", false);
  byte safecnt = prefs.getUChar("safecnt", 0);
  if(safecnt < 3){
    prefs.putUChar("safecnt", safecnt+1);
  }else{
    safemode = 1;
  }
  logprnln("Safemode handled.");
  Serial.println("Safemode handled.");
  
  // Init hoot preferences
  logprnln("Initializing preferences...");
  Serial.println("Initializing preferences...");
  init_settings(&prefs, &settings);
  logprnln("Preferences initialized.");
  Serial.println("Preferences initialized.");
  
  // Init sound
  logprnln("Initializing audio...");
  Serial.println("Initializing audio...");
  pinMode(SND_ENABLE, OUTPUT);
  digitalWrite(SND_ENABLE, LOW);
  out = new AudioOutputI2S();
  out -> SetGain(VOL_HOOT * settings.mainvol * settings.hootvol);
  out -> SetPinout(Bit_Clock_BCLK,Word_Select_WS,Serial_Data_SD);
  sam = new ESP8266SAM;
  logprnln("Audio initialized.");
  Serial.println("Audio initialized.");

  // Init battery monitoring
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);
  char stmp[9];
  snprintf(stmp, 9, "%.2f", get_batt(settings.battc));
  logprn("Battery voltage: ");
  logprn(stmp);
  logprn("V.\n");

  // Init wifi
  logprnln("Initializing WiFi...");
  Serial.println("Initializing WiFi...");
  wifi_init(settings.ssid);
  logprnln("WiFi initialized.");
  Serial.println("Wifi initialized.");

  if(safemode == 1){
    settings.hztx = 1000;
    settings.mainvol = 1.0;
    announce("Safemode enabled.", "Safe mode enaybled.");
    wifi_enable("HOOT");
  } else if(safemode == 0 && settings.wiboot){
    logprnln("Enabling WiFi.");
    wifi_enable(settings.ssid);
  }

  logprnln("Configuring sleep...");
  Serial.println("Configuring sleep...");
  esp_sleep_enable_touchpad_wakeup();
  touchAttachInterrupt(T2, onTouch, settings.tth);
  esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_ON);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  esp_sleep_enable_ext1_wakeup(0x4000000,ESP_EXT1_WAKEUP_ANY_HIGH);
  logprnln("Sleep configured.");
  Serial.println("Sleep configured.");

  if(!safemode){
    logprnln("Initializing LoRa...");
    Serial.println("Initializing LoRa...");
    LoRa.setPins(18, 14, 26);
    if (!LoRa.begin(915E6)) {
      err("Unable to initialize LoRa module.", "Uneyble to initialize LoRa module.");
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
  } else{
    logprnln("Skipping LoRa init due to safemode.");
  }
  
  logprnln("System startup complete.");
  Serial.println("System startup complete.");

  // Boot sound
  //beep(0.4, settings.hzrx);
  if(!safemode) play_hoot();
}

void loop() {
    handle_sound();
    if(wifi_on) handle_wifi(&prefs, &settings, &logbuf);
    if(!safemode){
      handle_tx();
      handle_rx_beep();
      handle_touch();
      handle_sleep();
    }
    handle_safemode();
}
