/**
 * Configura una serie de paginas web para conectarse a una red WiFi.
 * 
 * Usa el método de "Captive Portal" que muestra una página HTML para configurar la wifi.
 * 
 * Aprovechamos para poner en esa pagina HTML cosas como ver logs, etc
 * 
 * JJTeam - 2021
 */

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <EEPROM.h>
#include "Tools.h"
#include "WebResources.h"
#include "FSLog.h"

/* hostname for mDNS. Should work at least on windows. Try http://myHostname.local */
inline const String getHostname() { return "pigguard" + getSerialNumber(); }
inline const String getSoftAP_SSID() { return "Pig Guard " + getSerialNumber(); }
constexpr char TEXT_HTML[] = "text/html";
constexpr char TEXT_PLAIN[] = "text/plain";

/* Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM */
char ssid[33] = "";
char password[20] = "";
const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer server(80);

/* Soft AP network parameters */
IPAddress apIP(172, 217, 28, 1);
IPAddress netMsk(255, 255, 255, 0);

/** Should I connect to WLAN asap? */
boolean connect;

/** Last time I tried to connect to WLAN */
unsigned long lastConnectTry = 0;

/** Current WLAN status */
unsigned int status = WL_IDLE_STATUS;

// arma un TAG HTML, y puede tener clase (optativa)
String Tag(String tag, String text, String classs = "")
{
    if (classs.isEmpty())
        return "<" + tag + ">" + text + "</" + tag + ">";
    else
        return "<" + tag + " class=\"" + classs + "\">" + text + "</" + tag + ">";
}

// TAG OPTION (value optativo)
String Option(String text, String value = "")
{
    return "<option value=\"" + value + "\">" + text + "</option>";
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal()
{
    if (!isIp(server.hostHeader()) && server.hostHeader() != (getHostname() + ".local"))
    {
        Serial.println("Request redirected to captive portal");
        server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
        server.send(302, TEXT_PLAIN, ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
        server.client().stop();           // Stop is needed because we sent no content length
        return true;
    }
    return false;
}

/** Load WLAN credentials from EEPROM */
void loadCredentials()
{
    EEPROM.begin(512);
    EEPROM.get(0, ssid);
    EEPROM.get(0 + sizeof(ssid), password);
    char ok[2 + 1];
    EEPROM.get(0 + sizeof(ssid) + sizeof(password), ok);
    EEPROM.end();
    if (String(ok) != String("OK"))
    {
        ssid[0] = 0;
        password[0] = 0;
    }
    Serial.printf("Recovered credentials => [%s/%s]\n", ssid, password);
}

/** Store WLAN credentials to EEPROM */
void saveCredentials()
{
    EEPROM.begin(512);
    EEPROM.put(0, ssid);
    EEPROM.put(0 + sizeof(ssid), password);
    char ok[2 + 1] = "OK";
    EEPROM.put(0 + sizeof(ssid) + sizeof(password), ok);
    EEPROM.commit();
    EEPROM.end();
}

void SendCacheHeader()
{
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
}

bool isLocalIP()
{
    return server.client().localIP() == apIP;
}

String GetConnectThrough()
{
    return Tag("p", "Est&aacute;s conectado a trav&eacute;s de<br>" +
                        (isLocalIP()
                             ? "soft AP: <b>" + getSoftAP_SSID() + "</b>"
                             : "red wifi: <b>" + String(ssid) + "</b>"));
}

/** Handle root or redirect to captive portal */
void handleRoot()
{
    if (captivePortal())
    { // If caprive portal redirect instead of displaying the page.
        return;
    }

    SendCacheHeader();

    String Page;
    Page += HTML_BODY_START;
    Page += GetConnectThrough();
    Page += HTML_MENU_INDEX;
    Page += HTML_BODY_END;
    server.send(200, TEXT_HTML, Page);
}

// cada linea de log la decoro y la evío:
void EnrichAndSend(String line)
{
    String color = (line[1] == 'E' ? "r" : (line[1] == 'I' ? "b" : "n"));
    server.sendContent(Tag("p", line, color));
}

void handleLogs()
{
    SendCacheHeader();

    // envia con Chunk Mode HTTP 1.1

    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, TEXT_HTML, HTML_BODY_START);
    server.sendContent(Tag("h2", "Logs al iniciar el sistema"));

    server.sendContent("<code>");
    FSLOG.forEachStartup(EnrichAndSend);
    server.sendContent("</code>");

    server.sendContent(Tag("h2", "Logs hist&oacute;ricos"));

    server.sendContent("<code>");
    FSLOG.forEachLine(EnrichAndSend);
    server.sendContent("</code>");

    server.sendContent(HTML_BODY_END);
    server.sendContent(""); // END CHUNK!
}

/** Wifi config page handler */
void handleWifi()
{
    SendCacheHeader();

    String Page;
    Page += HTML_BODY_START;
    Page += Tag("h1", "Pig Guard");
    Page += GetConnectThrough();
    Page += Tag("p", Tag("b", "SoftAP config"));
    Page += Tag("p", "SSID: " + getSoftAP_SSID());
    Page += Tag("p", "IP: " + toStringIp(WiFi.softAPIP()));
    Page += Tag("p", Tag("b", "WLAN config"));
    Page += Tag("p", "SSID: " + String(ssid));
    Page += Tag("p", "IP: " + toStringIp(WiFi.localIP()));

    Serial.println("scan start");
    int n = WiFi.scanNetworks();
    Serial.println("scan done");

    if (n > 0)
    {
        Page += Tag("h2", "Redes disponibles:");
        Page += "<form method='POST' action='wifisave'>";
        Page += "<select name=\"n\">";
        Page += Option("Seleccione una red");
        for (int i = 0; i < n; i++)
            Page += Option(WiFi.SSID(i) + "(" + WiFi.RSSI(i) + ")", WiFi.SSID(i));
        Page += "</select>"
                "<br><input type='text' placeholder='Ingrese la clave' size=\"15\" name='p'/>"
                "<br><input type='submit' value='Conectar'/>"
                "<br></form>";
        Page += Tag("p", "(Actualiza la p&aacute;gina para refrescar)");
    }
    else
    {
        Page += Tag("p", "No WLAN found", "r");
    }
    server.send(200, TEXT_HTML, Page);
    server.client().stop(); // Stop is needed because we sent no content length
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void handleWifiSave()
{
    Serial.println("wifi save");

    server.arg("n").toCharArray(ssid, sizeof(ssid) - 1);
    server.arg("p").toCharArray(password, sizeof(password) - 1);
    server.sendHeader("Location", "wifi", true);

    SendCacheHeader();

    server.send(302, TEXT_PLAIN, ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop();           // Stop is needed because we sent no content length
    saveCredentials();
    connect = strlen(ssid) > 0; // Request WLAN connect with new credentials if there is a SSID
}

void handleNotFound()
{
    if (captivePortal())
    { // If captive portal redirect instead of displaying the error page.
        return;
    }
    String message = F("File Not Found\n\n");
    message += F("URI: ");
    message += server.uri();
    message += F("\nMethod: ");
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += F("\nArguments: ");
    message += server.args();
    message += F("\n");

    for (uint8_t i = 0; i < server.args(); i++)
        message += String(F(" ")) + server.argName(i) + F(": ") + server.arg(i) + F("\n");

    SendCacheHeader();
    server.send(404, TEXT_PLAIN, message);
}

void PrintWiFiStatus(uint8_t st)
{
    switch (st)
    {
    case WL_IDLE_STATUS:
        Serial.printf("status : %d, %s\n", st, "WL_IDLE_STATUS");
        break;
    case WL_NO_SSID_AVAIL:
        Serial.printf("status : %d, %s\n", st, "WL_NO_SSID_AVAIL");
        break;
    case WL_SCAN_COMPLETED:
        Serial.printf("status : %d, %s\n", st, "WL_SCAN_COMPLETED");
        break;
    case WL_CONNECTED:
        Serial.printf("status : %d, %s\n", st, "WL_CONNECTED");
        break;
    case WL_CONNECT_FAILED:
        Serial.printf("status : %d, %s\n", st, "WL_CONNECT_FAILED");
        break;
    case WL_CONNECTION_LOST:
        Serial.printf("status : %d, %s\n", st, "WL_CONNECTION_LOST");
        break;
    case WL_DISCONNECTED:
        Serial.printf("status : %d, %s\n", st, "WL_DISCONNECTED");
        break;
    }
}
void connectWifi()
{
    uint8_t st;
    Serial.println("Connecting as wifi client...");
    WiFi.disconnect();
    st = WiFi.begin(ssid, password);
    PrintWiFiStatus(st);
    st = WiFi.waitForConnectResult();
    PrintWiFiStatus(st);

    // Serial.printf("status : %d\n", st);
    /* WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_DISCONNECTED     = 6*/
    //Serial.println(st);
}

void WifiSetup()
{
    Serial.println();
    Serial.println("Configuring access point...");
    bool f;

    f = WiFi.softAPConfig(apIP, apIP, netMsk);
    Serial.println(f ? "OK" : "ERR");
    f = WiFi.softAP(getSoftAP_SSID().c_str(), ""); // sin contraseña (AP OPEN)
    Serial.println(f ? "OK" : "ERR");

    delay(500); // Without delay I've seen the IP address blank
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    /* Setup the DNS server redirecting all the domains to the apIP */
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", apIP);

    /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
    server.on("/", handleRoot);
    server.on("/wifi", handleWifi);
    server.on("/logs", handleLogs);
    server.on("/wifisave", handleWifiSave);
    server.on("/generate_204", handleRoot); //Android captive portal. Maybe not needed. Might be handled by notFound handler.
    server.on("/fwlink", handleRoot);       //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
    server.on("/style.css", []()
              { server.send(200, "text/css", style_css); });
    server.on("/logo.jpg", []()
              { server.send_P(200, "image/jpeg", logo_jpg, logo_jpg_size); });
    server.onNotFound(handleNotFound);
    server.begin(); // Web server start
    Serial.println("HTTP server started");
    loadCredentials();          // Load WLAN credentials from network
    connect = strlen(ssid) > 0; // Request WLAN connect if there is a SSID
}

void WifiLoop()
{
    // loop general...
    dnsServer.processNextRequest();
    server.handleClient();

    // verifico si se desconecta y lo vuelvo a conectar...
    if (connect)
    {
        Serial.println("Connect requested");
        connect = false;
        connectWifi();
        lastConnectTry = millis();
    }

    unsigned int wifi_status = WiFi.status();
    if (wifi_status == 0 && millis() > (lastConnectTry + 60000))
    {
        /* If WLAN disconnected and idle try to connect */
        /* Don't set retry time too low as retry interfere the softAP operation */
        connect = true;
    }

    if (status != wifi_status)
    { // WLAN status change

        PrintWiFiStatus(wifi_status);
        //  Serial.print("Status: ");
        // Serial.println(wifi_status);

        status = wifi_status;
        if (wifi_status == WL_CONNECTED)
        {
            /* Just connected to WLAN */
            Serial.println("");
            Serial.print("Connected to ");
            Serial.println(ssid);
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());

            // Setup MDNS responder
            if (!MDNS.begin(getHostname().c_str()))
            {
                Serial.println("Error setting up MDNS responder!");
            }
            else
            {
                Serial.println("mDNS responder started");
                // Add service to MDNS-SD
                MDNS.addService("http", "tcp", 80);
            }
        }
        else if (wifi_status == WL_NO_SSID_AVAIL)
        {
            WiFi.disconnect();
        }
    }
}