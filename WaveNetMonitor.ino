#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266SSDP.h>
#include <ESP8266HTTPUpdateServer.h>

#include <DNSServer.h>
#include <EEPROM.h>

/* Access point ssid and password */
const char* ap_ssid = "wavenet";
const char* ap_pass = "wavenet";
/* Access point network parameters */
IPAddress ap_ip(192,168,4,1);
IPAddress ap_netmask(255,255,255,0);
/* mDNS hostname */
const char* mdns_hostname = "wavenet";

/* Don't set this wifi credentials. They are configured at runtime and stored on EEPROM */
char ssid[32] = "";
char password[32] = "";

/* Access points to connect to */
const int try_ap_n = 2;
const char* try_ap_ssid[try_ap_n] = {"Sedition", "xinabox"};
const char* try_ap_pass[try_ap_n] = {"Butt20!7", "xinabox1234"};

/* Firmware update. */
const char* update_path = "/firmware";
const char* update_user = "admin";
const char* update_pass = "admin";
String update_cmd;

/* Attempt to connect to multiple APs */
ESP8266WiFiMulti wifiMulti;

/* Http server and updater */
#define HTTP_PORT 80
ESP8266WebServer httpServer(HTTP_PORT);
ESP8266HTTPUpdateServer httpUpdater;

/* Captive portal */
#define DNS_PORT 53
DNSServer dnsServer;

/* Should I connect to WLAN asap? */
boolean connect;
/* Last time I tried to connect to WLAN */
long lastConnectTry = 0;
/* Current WLAN status */
int status = WL_IDLE_STATUS;

void setup() {

  /* Wait until settled */
	delay(1000);

  /* Start serial */
	Serial.begin(115200);

  /* Attempt to connect to access points */
  for (int i = 0; i < try_ap_n; i++) {
    Serial.printf("Attempting to connect to access point '%s' with password '%s'...",try_ap_ssid[i],try_ap_pass[i]);
    wifiMulti.addAP(try_ap_ssid[i],try_ap_pass[i]);
  }
  IPAddress myIP;
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println(" connected.");
    Serial.print("Client IP address: ");
    myIP = WiFi.localIP();
    Serial.println(myIP);
  } else {
    Serial.println(" failed.");

    /* Attempt to setup access point */
    Serial.print("Configuring access point...");
    WiFi.softAPConfig(ap_ip, ap_ip, ap_netmask);
    WiFi.softAP(ap_ssid);

    Serial.print("AP IP address: ");
    myIP = WiFi.softAPIP();
    Serial.println(myIP);
  }

  /* Starting http server */
  httpServer.on("/", handleRoot);
  httpServer.on("/index.html", handleRoot);
  httpServer.on("/description.xml", HTTP_GET, [](){
    SSDP.schema(httpServer.client());
  });
  httpServer.on("/wifi", handleWifi);
  httpServer.on("/wifisave", handleWifiSave);
  httpServer.on("/generate_204", handleRoot); // Android captive portal. Maybe not needed. Might be handled by notFound handler.
  httpServer.on("/fwlink", handleRoot); // Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  httpServer.onNotFound(handleNotFound);
  /* Starting http update server */
  httpUpdater.setup(&httpServer, update_path, update_user, update_pass);
  httpServer.begin();

  /* Starting mDNS */
  MDNS.begin(mdns_hostname);
  MDNS.addService("http", "tcp", 80);

  /* Starting SSDP */
  SSDP.setSchemaURL("description.xml");
  SSDP.setHTTPPort(80);
  SSDP.setName("WaveNet node");
  SSDP.setSerialNumber("001788102201");
  SSDP.setURL("index.html");
  SSDP.setModelName("WaveNet");
  SSDP.setModelNumber("1");
  SSDP.setModelURL("http://wavenet.local");
  SSDP.setManufacturer("Wouter Deconinck");
  SSDP.setManufacturerURL("http://github.com/wdconinc");
  SSDP.begin();

  /* Starting DNS */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", myIP);
  
  /* Update command */
  update_cmd = "curl -u " + String(update_user) + ":" + String(update_pass) + " -F image=@firmware.bin " + String(mdns_hostname) + ".local" + String(update_path);
}

void loop() {
  /* Handle requests */
	httpServer.handleClient();
  dnsServer.processNextRequest();

  /* Serial output */
  static int t0 = millis();
  const int dt = 2000;
  if (millis() - t0 > dt) {
    
    IPAddress myIP;
    if(wifiMulti.run() == WL_CONNECTED) {
      Serial.println("Connected to AP.");
      myIP = WiFi.localIP();
    } else {
      Serial.printf("Access point: %s.\n",ap_ssid);
      myIP = WiFi.softAPIP();
    }
    Serial.print("IP address: "); Serial.println(myIP);
    Serial.print("Update cmd: "); Serial.println(update_cmd);

    t0 = millis();
  }

  /* Deep sleep for 2 seconds */
  ESP.deepSleep(2e6);
}
