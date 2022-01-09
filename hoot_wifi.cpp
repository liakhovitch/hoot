#include "hoot_wifi.h"

#define MAX_HEAD_LEN 4096
#define TIMEOUT 5000

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer* dnsServer = NULL;;
WiFiServer* server = NULL;

int wifi_on = 0;

char * strnstr(const char *s, const char *find, size_t slen) {
  char c, sc;
  size_t len;

  if ((c = *find++) != '\0') {
    len = strlen(find);
    do {
      do {
        if ((sc = *s++) == '\0' || slen-- < 1)
          return (NULL);
      } while (sc != c);
      if (len > slen)
        return (NULL);
    } while (strncmp(s, find, len) != 0);
    s--;
  }
  return ((char *)s);
}

int findparam(const char* s, const char* find, size_t slen, char** res, size_t len_max) {
  *res = strnstr(s, find, slen);
  if (*res){
    int len = 0;
    *res = *res + strlen(find);
    while(*res+len < s+slen && len < len_max && *(*res+len)!= '\0' && *(*res+len)!= ' ' && *(*res+len)!= '&' && *(*res+len)!= '\n' && *(*res+len)!= '\r'){
      len++;
    }
    return len;
  }
  else return -1;
}

void store_params(Preferences* prefs, struct Settings* settings){
  prefs->putBytes("hmsg", settings->hmsg, 9);
  prefs->putUChar("hrcv", settings->hrcv);
  prefs->putBytes("ssid", settings->ssid, 9);
  prefs->putFloat("hzrx", settings->hzrx);
  prefs->putUChar("rxbp", settings->rxbp);
  prefs->putFloat("hztx", settings->hztx);
  prefs->putUChar("txbp", settings->txbp);
  prefs->putFloat("vol", settings->vol);
  prefs->putUChar("hootsnd", settings->hootsnd);
  prefs->putULong("rxunit", settings->rxunit);
  prefs->putUChar("wrd", settings->wrd);
  prefs->putUChar("pack", settings->pack);
  prefs->putUChar("pwr", settings->pwr);
  prefs->putUChar("sprd", settings->sprd);
  LoRa.setSyncWord(settings->wrd);
  LoRa.setTxPower(settings->pwr);
  LoRa.setSpreadingFactor(settings->sprd);
}

void wifi_init(char* ssid){
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  wifi_on = 0;
}

void handle_wifi(Preferences* prefs, struct Settings* settings, CircularBuffer<char, LOGBUF_SIZE>* logbuf){
  if(wifi_on){
      dnsServer->processNextRequest();
      WiFiClient client = server->available();   // listen for incoming clients
      if (client) {
        char header[MAX_HEAD_LEN];
        unsigned long currentTime = millis();
        unsigned long previousTime = currentTime;
        char c_prev = '\0';
        int header_done = 0;
        size_t head_index = 0;
        while (client.connected() && currentTime - previousTime <= TIMEOUT) {
          if(client.available()){ 
            char c = client.read();
            if(header_done == 1 && head_index < MAX_HEAD_LEN){
              header[head_index] = c;
              head_index++;
            }
            if (c == '\n' && c_prev == '\n') header_done = 1;
            previousTime = currentTime;
            currentTime = millis();
            if(c != '\r') c_prev = c;
          }
          else {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            // Handle changes
            char* s;
            int len;
            //nlogprn(header, head_index);
            if(strnstr(header, "settings=1", head_index) == header){
              len = findparam(header, "hmsg=", head_index, &s, 8);
              if(len >= 0){
                memset(settings->hmsg, '\0', 9);
                strncpy(settings->hmsg, s, len);
              }
              len = findparam(header, "hrcv=", head_index, &s, 8);
              if(len >= 0) settings->hrcv = 1;
              else settings->hrcv = 0;
              len = findparam(header, "hzrx=", head_index, &s, 8);
              if(len >= 0){
                char stmp[9];
                memset(stmp, '\0', 9);
                strncpy(stmp, s, len);
                char* str_end = NULL;
                long ntmp = strtol(stmp, &str_end, 10);
                if(str_end != stmp && ntmp >= 0 && ntmp <= 2000){
                  settings->hzrx = (float)ntmp;
                }
              }
              len = findparam(header, "rxbp=", head_index, &s, 8);
              if(len >= 0) settings->rxbp = 1;
              else settings->rxbp = 0;
              len = findparam(header, "hztx=", head_index, &s, 8);
              if(len >= 0){
                char stmp[9];
                memset(stmp, '\0', 9);
                strncpy(stmp, s, len);
                char* str_end = NULL;
                long ntmp = strtol(stmp, &str_end, 10);
                if(str_end != stmp && ntmp >= 0 && ntmp <= 2000){
                  settings->hztx = (float)ntmp;
                }
              }
              len = findparam(header, "txbp=", head_index, &s, 8);
              if(len >= 0) settings->txbp = 1;
              else settings->txbp = 0;
              len = findparam(header, "vol=", head_index, &s, 8);
              if(len >= 0){
                char stmp[9];
                memset(stmp, '\0', 9);
                strncpy(stmp, s, len);
                char* str_end = NULL;
                float ntmp = strtof(stmp, &str_end);
                if(str_end != stmp && ntmp >= 0 && ntmp <= 2){
                  settings->vol = ntmp;
                }
              }
              len = findparam(header, "hootsnd=", head_index, &s, 8);
              if(len >= 0){
                char stmp[9];
                memset(stmp, '\0', 9);
                strncpy(stmp, s, len);
                char* str_end = NULL;
                long ntmp = strtol(stmp, &str_end, 10);
                if(str_end != stmp && ntmp >= 0 && ntmp <= 1){
                  settings->hootsnd = (uint8_t)ntmp;
                }
              }
              len = findparam(header, "rxunit=", head_index, &s, 8);
              if(len >= 0){
                char stmp[9];
                memset(stmp, '\0', 9);
                strncpy(stmp, s, len);
                char* str_end = NULL;
                long ntmp = strtol(stmp, &str_end, 10);
                if(str_end != stmp && ntmp >= 0 && ntmp <= 1000){
                  settings->rxunit = (uint16_t)ntmp;
                }
              }
              len = findparam(header, "wrd=", head_index, &s, 8);
              if(len >= 0){
                char stmp[9];
                memset(stmp, '\0', 9);
                strncpy(stmp, s, len);
                char* str_end = NULL;
                long ntmp = strtol(stmp, &str_end, 10);
                if(str_end != stmp && ntmp >= 0 && ntmp <= 255){
                  settings->wrd = (uint8_t)ntmp;
                }
              }
              len = findparam(header, "pack=", head_index, &s, 8);
              if(len >= 0){
                char stmp[9];
                memset(stmp, '\0', 9);
                strncpy(stmp, s, len);
                char* str_end = NULL;
                long ntmp = strtol(stmp, &str_end, 10);
                if(str_end != stmp && ntmp >= 1 && ntmp <= 50){
                  settings->pack = (uint8_t)ntmp;
                }
              }
              len = findparam(header, "pwr=", head_index, &s, 8);
              if(len >= 0){
                char stmp[9];
                memset(stmp, '\0', 9);
                strncpy(stmp, s, len);
                char* str_end = NULL;
                long ntmp = strtol(stmp, &str_end, 10);
                if(str_end != stmp && ntmp >= 2 && ntmp <= 20){
                  settings->pwr = (uint8_t)ntmp;
                }
              }
              len = findparam(header, "sprd=", head_index, &s, 8);
              if(len >= 0){
                char stmp[9];
                memset(stmp, '\0', 9);
                strncpy(stmp, s, len);
                char* str_end = NULL;
                long ntmp = strtol(stmp, &str_end, 10);
                if(str_end != stmp && ntmp >= 7 && ntmp <= 12){
                  settings->sprd = (uint8_t)ntmp;
                }
              }
              len = findparam(header, "ssid=", head_index, &s, 8);
              if(len > 0){ // Null length SSID is not acceptable here
                memset(settings->ssid, '\0', 9);
                strncpy(settings->ssid, s, len);
              }
            } else{
              len = findparam(header, "txmsg=", head_index, &s, 50);
              if(len > 0){
                for(size_t z = 0; z < len; z++){
                  if(*(s+z) == '+') txchar(' ');
                  else txchar(*(s+z));
                }
                txflush();
                logprnln("Web msg submitted.");
              }
            }
            char stmp[9];
            // Display page
            client.println(""
 "<!DOCTYPE html><html><head><title>HOOT Setup</title></head>"
 "<style>"
 "h1 {"
    "font-family: \"Comic Sans MS\", \"Comic Sans\", cursive;"
 "}"
  "h5 {"
    "font-size: 6px;"
 "}"
  "h6 {"
    "font-size: 2px;"
 "}"
 "</style>"
 "<body>"
 "<h1>HOOT Setup</h1>"
 "<h2>Welcome to your HOOT&trade; emergency personal communication device!</h2>"
 "<form action=\"/\" method=\"post\">"
 " <div>"
 " <input type=\"hidden\" value=\"1\" name=\"settings\">"
 " </div>"
 " <div>"
 "   <label for=\"hmsg\">Message that triggers a \"Hoot&trade;\":</label>"
 "   <input name=\"hmsg\" id=\"hmsg\" maxlength=\"8\" value=\"");client.print(settings->hmsg);client.print("\">"
 " </div>"
 " <div>"
 "   <label for=\"hrcv\">Hoot?</label>"
 "   <input type=\"checkbox\" id=\"hrcv\" name=\"hrcv\" value=\"true\" ");if(settings->hrcv)client.print("checked");client.print(">"
 " </div>"
 " <div>"
 "   <label for=\"hzrx\">RX Tone frequency (hz):</label>"
 "   <input type=\"number\" name=\"hzrx\" id=\"hzrx\" min=\"0\" max=\"2000\" value=\"");snprintf(stmp, 9, "%f", settings->hzrx);client.print(stmp);client.print("\">"
 " </div>" 
 " <div>"
 "   <label for=\"rxbp\">Beep out incoming messages?</label>"
 "   <input type=\"checkbox\" id=\"rxbp\" name=\"rxbp\" value=\"true\" ");if(settings->rxbp)client.print("checked");client.print(">"
 " </div>"
 " <div>"
 "   <label for=\"hztx\">TX Tone frequency (hz):</label>"
 "   <input type=\"number\" name=\"hztx\" id=\"hztx\" min=\"0\" max=\"2000\" value=\"");snprintf(stmp, 9, "%f", settings->hztx);client.print(stmp);client.print("\">"
 " </div>" 
 " <div>"
 "   <label for=\"txbp\">Beep out outgoing messages?</label>"
 "   <input type=\"checkbox\" id=\"txbp\" name=\"txbp\" value=\"true\" ");if(settings->txbp)client.print("checked");client.print(">"
 " </div>"
 " <div>"
 "   <label for=\"vol\">Volume:</label>"
 "   <input type=\"range\" name=\"vol\" id=\"vol\" min=\"0\" max=\"2\" step=\"0.1\" value=\"");snprintf(stmp, 9, "%f", settings->vol);client.print(stmp);client.print("\">"
 " </div>" 
 " <div>"
 "   <label for=\"hootsnd\">Alternate hoot sound:</label>"
 "   <input type=\"range\" name=\"hootsnd\" id=\"hootsnd\" min=\"0\" max=\"1\" step=\"1\" value=\"");snprintf(stmp, 9, "%i", settings->hootsnd);client.print(stmp);client.print("\">"
 " </div>" 
 " <div>"
 "   <label for=\"rxunit\">Length of a dit, in ms (RX):</label>"
 "   <input type=\"number\" name=\"rxunit\" id=\"rxunit\" min=\"0\" max=\"1000\" value=\"");snprintf(stmp, 9, "%i", settings->rxunit);client.print(stmp);client.print("\">"
 " </div>" 
 " <div>"
 "   <label for=\"wrd\">Channel (0-255):</label>"
 "   <input type=\"number\" name=\"wrd\" id=\"wrd\" min=\"0\" max=\"255\" value=\"");snprintf(stmp, 9, "%i", settings->wrd);client.print(stmp);client.print("\">"
 " </div>" 
 " <div>"
 "   <label for=\"pack\">Packet size (1-50):</label>"
 "   <input type=\"number\" name=\"pack\" id=\"pack\" min=\"1\" max=\"50\" value=\"");snprintf(stmp, 9, "%i", settings->pack);client.print(stmp);client.print("\">"
 " </div>" 
 " <div>"
 "   <label for=\"pwr\">Transmit power (2-20):</label>"
 "   <input type=\"number\" name=\"pwr\" id=\"pwr\" min=\"2\" max=\"20\" value=\"");snprintf(stmp, 9, "%i", settings->pwr);client.print(stmp);client.print("\">"
 " </div>" 
 " <div>"
 "   <label for=\"sprd\">Spreading factor (7-12:)</label>"
 "   <input type=\"number\" name=\"sprd\" id=\"sprd\" min=\"7\" max=\"12\" value=\"");snprintf(stmp, 9, "%i", settings->sprd);client.print(stmp);client.print("\">"
 " </div>" 
 " <div>"
 "   <label for=\"ssid\">WiFi SSID:</label>"
 "   <input name=\"ssid\" id=\"ssid\" maxlength=\"8\" value=\"");client.print(settings->ssid);client.print("\">"
 " </div>"
 " <div>"
 "   <button>Save</button>"
 " </div>"
 "</form>"
 " <H3>Log:</H3>"
 "<div>");
 // Beyond stupid use of header buffer
 size_t j = 0;
 size_t bufsize = logbuf->size();

 while(j < bufsize){
   size_t i = 0;
   while(i < MAX_HEAD_LEN-1 && j < bufsize){
    header[i] = (*logbuf)[j];
    j++;
    i++;
   }
   header[i] = 0;
   client.print(header);
 }
 
 client.print("</div \"height:240px;width:100%;border:1px solid #ccc;overflow:scroll;\">"
  "<form action=\"/\" method=\"post\">"
 " <div>"
 "   <label for=\"txmsg\">Send Message:</label>"
 "   <input name=\"txmsg\" maxlength=\"50\" id=\"txmsg\">"
 " </div>"
 " <div>"
 "   <button>Send</button>"
 " </div>"
 "</form>"
 " </body>"
 " <script>"
   "if ( window.history.replaceState ) {"
    "window.history.replaceState( null, null, window.location.href );"
   "}"
 " </script>"
 " </html>\n\n");
            store_params(prefs, settings);
            break;
          }
        }
        client.stop();
      }
  }
}

void wifi_enable(char* ssid){
  WiFi.disconnect();   //added to start with the wifi off, avoid crashing
  WiFi.mode(WIFI_OFF); //added to start with the wifi off, avoid crashing
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid);
  delay(100);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  dnsServer = new DNSServer;
  dnsServer->start(DNS_PORT, "*", apIP);
  
  server = new WiFiServer(80);
  server->begin();
  wifi_on = 1;
  announce("WiFi enabled.", "WiFi enaaybled!");
}

void wifi_disable(){
  delete dnsServer;
  dnsServer = NULL;
  delete server;
  server = NULL;
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  wifi_on = 0;
  announce("WiFi disabled.","WiFi Disaaybled.");
}
