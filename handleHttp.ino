/** Handle root or redirect to captive portal */
void handleRoot() {
  if (captivePortal()) { // If captive portal, redirect instead of displaying the page.
    return;
  }
  httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  httpServer.sendHeader("Pragma", "no-cache");
  httpServer.sendHeader("Expires", "-1");
  httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  httpServer.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  httpServer.sendContent(
    "<html><head>WaveNet</head><body>"
    "<h1>WaveNet</h1>"
  );
  httpServer.sendContent(
    "<p>Chip ID: " + String(ESP.getChipId()) + "</p>"
  );
  httpServer.sendContent(
    "<p>Update: " + String(update_cmd) + "</p>"
  );
  if (httpServer.client().localIP() == ap_ip) {
    httpServer.sendContent(String("<p>You are connected through the soft AP: ") + ap_ssid + "</p>");
  } else {
    httpServer.sendContent(String("<p>You are connected through the wifi network: ") + ssid + "</p>");
  }
  httpServer.sendContent(
    "<p>You may want to <a href='/wifi'>config the wifi connection</a>.</p>"
    "</body></html>"
  );
  httpServer.client().stop(); // Stop is needed because we sent no content length
}

/* Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal() {
  if (!isIp(httpServer.hostHeader()) && httpServer.hostHeader() != (String(mdns_hostname) + ".local")) {
    Serial.print("Request redirected to captive portal");
    httpServer.sendHeader("Location", String("http://") + toStringIp(httpServer.client().localIP()), true);
    httpServer.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    httpServer.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

/* Wifi config page handler */
void handleWifi() {
  httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  httpServer.sendHeader("Pragma", "no-cache");
  httpServer.sendHeader("Expires", "-1");
  httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  httpServer.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  httpServer.sendContent(
    "<html><head></head><body>"
    "<h1>Wifi config</h1>"
  );
  if (httpServer.client().localIP() == ap_ip) {
    httpServer.sendContent(String("<p>You are connected through the soft AP: ") + ap_ssid + "</p>");
  } else {
    httpServer.sendContent(String("<p>You are connected through the wifi network: ") + ssid + "</p>");
  }
  httpServer.sendContent(
    "\r\n<br />"
    "<table><tr><th align='left'>SoftAP config</th></tr>"
  );
  httpServer.sendContent(String() + "<tr><td>SSID " + String(ap_ssid) + "</td></tr>");
  httpServer.sendContent(String() + "<tr><td>IP " + toStringIp(WiFi.softAPIP()) + "</td></tr>");
  httpServer.sendContent(
    "</table>"
    "\r\n<br />"
    "<table><tr><th align='left'>WLAN config</th></tr>"
  );
  httpServer.sendContent(String() + "<tr><td>SSID " + String(ssid) + "</td></tr>");
  httpServer.sendContent(String() + "<tr><td>IP " + toStringIp(WiFi.localIP()) + "</td></tr>");
  httpServer.sendContent(
    "</table>"
    "\r\n<br />"
    "<table><tr><th align='left'>WLAN list (refresh if any missing)</th></tr>"
  );
  Serial.println("scan start");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      httpServer.sendContent(String() + "\r\n<tr><td>SSID " + WiFi.SSID(i) + String((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":" *") + " (" + WiFi.RSSI(i) + ")</td></tr>");
    }
  } else {
    httpServer.sendContent(String() + "<tr><td>No WLAN found</td></tr>");
  }
  httpServer.sendContent(
    "</table>"
    "\r\n<br /><form method='POST' action='wifisave'><h4>Connect to network:</h4>"
    "<input type='text' placeholder='network' name='n'/>"
    "<br /><input type='password' placeholder='password' name='p'/>"
    "<br /><input type='submit' value='Connect/Disconnect'/></form>"
    "<p>You may want to <a href='/'>return to the home page</a>.</p>"
    "</body></html>"
  );
  httpServer.client().stop(); // Stop is needed because we sent no content length
}

/* Handle the WLAN save form and redirect to WLAN config page again */
void handleWifiSave() {
  Serial.println("wifi save");
  httpServer.arg("n").toCharArray(ssid, sizeof(ssid) - 1);
  httpServer.arg("p").toCharArray(password, sizeof(password) - 1);
  httpServer.sendHeader("Location", "wifi", true);
  httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  httpServer.sendHeader("Pragma", "no-cache");
  httpServer.sendHeader("Expires", "-1");
  httpServer.send(302, "text/plain", "");  // Empty content inhibits Content-length header so we have to close the socket ourselves.
  httpServer.client().stop(); // Stop is needed because we sent no content length
  saveCredentials();
  connect = strlen(ssid) > 0; // Request WLAN connect with new credentials if there is a SSID
}

void handleNotFound() {
  if (captivePortal()) { // If captive portal, redirect instead of displaying the error page.
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += httpServer.uri();
  message += "\nMethod: ";
  message += ( httpServer.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += httpServer.args();
  message += "\n";

  for (uint8_t i = 0; i < httpServer.args(); i++) {
    message += " " + httpServer.argName(i) + ": " + httpServer.arg(i) + "\n";
  }
  httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  httpServer.sendHeader("Pragma", "no-cache");
  httpServer.sendHeader("Expires", "-1");
  httpServer.send(404, "text/plain", message);
}

