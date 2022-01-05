#include "hoot_wifi.h"

#define MAX_HEAD_LEN 4096
#define TIMEOUT 5000

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
WiFiServer server(80);
int wifi_on;

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
  prefs->putUShort("hz", settings->hz);
  prefs->putUChar("rpt", settings->rpt);
  prefs->putUChar("wrd", settings->wrd);
}

void wifi_init(char* ssid){
  wifi_enable(ssid);
  server.begin();
  wifi_disable();
  wifi_on = 0;
}

void handle_wifi(Preferences* prefs, struct Settings* settings){
  if(wifi_on){
      dnsServer.processNextRequest();
      WiFiClient client = server.available();   // listen for incoming clients
      if (client) {
        char header[MAX_HEAD_LEN];
        unsigned long currentTime = millis();
        unsigned long previousTime = currentTime;
        char c_prev = '\0';
        int header_done = 0;
        size_t head_index = 0;
        while (client.connected() && currentTime - previousTime <= TIMEOUT) { 
          char c = client.read();
          if(header_done == 0 && head_index < MAX_HEAD_LEN){
            header[head_index] = c;
            head_index++;
          }
          if (c == '\n') header_done = 1;
          if (c == '\n' && c_prev == '\n'){
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            // Handle changes
            char* s;
            int len;
            if(strnstr(header, "?", MAX_HEAD_LEN)){
              len = findparam(header, "hmsg=", MAX_HEAD_LEN, &s, 8);
              if(len >= 0){
                memset(settings->hmsg, '\0', 9);
                strncpy(settings->hmsg, s, len);
              }
              len = findparam(header, "hrcv=", MAX_HEAD_LEN, &s, 8);
              if(len >= 0) settings->hrcv = 1;
              else settings->hrcv = 0;
              len = findparam(header, "hz=", MAX_HEAD_LEN, &s, 8);
              if(len >= 0){
                char stmp[9];
                memset(stmp, '\0', 9);
                strncpy(stmp, s, len);
                char* str_end = NULL;
                long ntmp = strtol(stmp, &str_end, 10);
                if(str_end != stmp && ntmp >= 0 && ntmp <= 2000){
                  settings->hz = (uint16_t)ntmp;
                }
              }
              len = findparam(header, "rpt=", MAX_HEAD_LEN, &s, 8);
              if(len >= 0){
                char stmp[9];
                memset(stmp, '\0', 9);
                strncpy(stmp, s, len);
                char* str_end = NULL;
                long ntmp = strtol(stmp, &str_end, 10);
                if(str_end != stmp && ntmp >= 0 && ntmp <= 100){
                  settings->rpt = (uint8_t)ntmp;
                }
              }
              len = findparam(header, "wrd=", MAX_HEAD_LEN, &s, 8);
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
              len = findparam(header, "ssid=", MAX_HEAD_LEN, &s, 8);
              if(len > 0){ // Null length SSID is not acceptable here
                memset(settings->ssid, '\0', 9);
                strncpy(settings->ssid, s, len);
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
 "<h3>This page will get you set up in no time.</h3>"
 "<h4>Just fill out the form below, and press \"save\" to save your settings.</h4>"
 "<h5>If you mess something up, don't worry! You're fucked!</h5>"
 "<h6>Star Trek's communicators didn't look like FREAKING OWLS so Captain Kirk can lick my spicy Vulkan balls.</h6>"
 "<form method=\"GET\">"
 " <div>"
 "   <label for=\"hmsg\">Message that triggers a \"Hoot&trade;\" (max. 8 ch):</label>"
 "   <input name=\"hmsg\" id=\"hmsg\" value=\"");client.print(settings->hmsg);client.print("\">"
 " </div>"
 " <div>"
 "   <label for=\"hrcv\">Hoot on receive</label>"
 "   <input type=\"checkbox\" id=\"hrcv\" name=\"hrcv\" value=\"true\" ");if(settings->hrcv)client.print("checked");client.print(">"
 " </div>"
 " <div>"
 "   <label for=\"hz\">Tone frequency (hz):</label>"
 "   <input type=\"number\" name=\"hz\" id=\"hz\" min=\"0\" max=\"2000\" value=\"");snprintf(stmp, 9, "%i", settings->hz);client.print(stmp);client.print("\">"
 " </div>" 
  " <div>"
 "   <label for=\"rpt\">Maximum send retry count (max. 100):</label>"
 "   <input type=\"number\" name=\"rpt\" id=\"rpt\" min=\"0\" max=\"100\" value=\"");snprintf(stmp, 9, "%i", settings->rpt);client.print(stmp);client.print("\">"
 " </div>" 
  " <div>"
 "   <label for=\"wrd\">Channel (0-255):</label>"
 "   <input type=\"number\" name=\"wrd\" id=\"wrd\" min=\"0\" max=\"255\" value=\"");snprintf(stmp, 9, "%i", settings->wrd);client.print(stmp);client.print("\">"
 " </div>" 
 " <div>"
 "   <label for=\"ssid\">WiFi SSID (max. 8 ch):</label>"
 "   <input name=\"ssid\" id=\"ssid\" value=\"");client.print(settings->ssid);client.print("\">"
 " </div>"
 " <div>"
 "   <button>Save</button>"
 " </div>"
 "</form>"
 "</body></html>\n\n");
            store_params(prefs, settings);
            break;
          }
          previousTime = currentTime;
          currentTime = millis();
          if(c != '\r') c_prev = c;
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
  dnsServer.start(DNS_PORT, "*", apIP);
  
  
  server.begin();
  wifi_on = 1;
}

void wifi_disable(){
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  wifi_on = 0;
}
