/*
  btcontrol ESP32 Arduino sketch

  Instructions:
  - creae config.h with
      #define wifi_ssid "wifiname"
      #define wifi_password (***password must be at least 8 characters ***) "password"
      #define lok_name "esp32-lok"
      #define SOFTAP (softap or client wifi)
      #define CHINA_MONSTER_SHIELD ---or--- KEYES_SHIELD
      
    optionally define SOFTAP
  - Flash the sketch to the ESP32 board
  - Install host software:
    - For Linux, install Avahi (http://avahi.org/).
    - For Windows, install Bonjour (http://www.apple.com/support/bonjour/).
    - For Mac OSX and iOS support is built in through Bonjour already.
  - Open the btclient app on your phone

TODO: bt client für nokia handies
https://techtutorialsx.com/2018/12/09/esp32-arduino-serial-over-bluetooth-client-connection-event/

TODO: BLE gamepad:
https://github.com/nkolban/esp32-snippets/blob/master/Documentation/BLE%20C%2B%2B%20Guide.pdf

TODO: mocute BT gamepad (nicht BLE)
vielleicht geht BTstack lib:
https://github.com/bluekitchen/btstack
https://www.dfrobot.com/blog-945.html

HID host: https://github.com/bluekitchen/btstack/blob/e034024d16933df0720cab3582d625294e26c667/test/pts/hid_host_test.c

TODO: sound mit MAX98357A (semaf)


 */


#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
// WifiClient verwendet pthread_*specific => pthread lib wird immer dazu gelinkt
#include <pthread.h>
// #include <freertos/task.h>
#include "ESP32PWM.h"
#include "clientthread.h"
#include "utils.h"
#include "lokdef.h"

// TODO:
// https://arduino.stackexchange.com/questions/23743/include-git-tag-or-svn-revision-in-arduino-sketch
//#include "gitTagVersion.h"

#include "config.h"

// https://github.com/espressif/arduino-esp32/blob/master/libraries/DNSServer/examples/CaptivePortal/CaptivePortal.ino
#ifdef SOFTAP

// => haut irgendwas zam
 #define DNSSERVER
 #define HTTPSERVER

// DNSServer + HTTPServer + MDNS braucht mehr sockets als default.
// wenn das nicht gesetzt wird funktioniert das netzwerk random mässig irgendwann einfach nicht mehr
#if CONFIG_LWIP_MAX_SOCKETS <= 10 and defined HTTPSERVER
#error increase CONFIG_LWIP_MAX_SOCKETS to 20 in hardware/espressif/esp32/tools/sdk/include/config/sdkconfig.h 
#endif

#ifdef DNSSERVER
#include <DNSServer.h>
DNSServer dnsServer;
#endif

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
// => haut irgendwie ned hin, ESP32 crasht dann hin und wieder

#ifdef HTTPSERVER
WiFiServer HTTPServer(80);
#endif

String responseHTML = ""
  "<!DOCTYPE html><html><head><title>" lok_name "</title></head><body>"
  "<h1>" lok_name "</h1><p>Mit folgender App kannst du die Lok steuern: "
  "<a href='https://github.com/ferbar/btcontrol/raw/master/control-android/bin/btcontrol.apk'>btcontrol.apk</a> "
  "(vor dem Download wieder ins Internet wechseln)"
  "</p>"
  "<p>Bitte auf Sicht fahren und keine Unf&auml;lle bauen!</p>"
  "</body></html>";
#endif

static const char *TAG="main";

Hardware *hardware=NULL;
bool cfg_debug=false;

// TCP server at port 80 will respond to HTTP requests
WiFiServer BTServer(3030);

void setup(void)
{  
    Serial.begin(115200);

    Serial.println("init wifi");
#ifdef SOFTAP
    if(strlen(wifi_password) < 8) {
        Serial.println("Setting Access Point");
        Serial.println("ERROR: password < 8 characters, fallback to unencrypted");      
    } else {
        Serial.printf("Setting Access Point ssid:%s password:%s\n", wifi_ssid, wifi_password);
    }
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(wifi_ssid, wifi_password);
    Serial.print("softAPmacAddress:");
    Serial.println(WiFi.softAPmacAddress());
    IPAddress IP = WiFi.softAPIP();

    // if DNSServer is started with "*" for domain name, it will reply with
    // provided IP to all DNS request
#ifdef DNSSERVER
        Serial.println("starting DNS server");
        dnsServer.start(DNS_PORT, "*", apIP);
#endif

#ifdef HTTPSERVER
        Serial.println("starting HTTP server");
        HTTPServer.begin();
#endif

#else
    // Connect to WiFi network
    WiFi.begin(wifi_ssid, wifi_password);
    Serial.printf("connecting to wifi %s...\n", wifi_ssid);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    IPAddress IP = WiFi.localIP()
    Serial.print("Connected to ");
    Serial.println(ssid);
#endif    
    Serial.print("IP address: ");
    Serial.println(IP);

    // Set up mDNS responder:
    // - first argument is the domain name, in this example
    //   the fully-qualified domain name is "esp8266.local"
    // - second argument is the IP address to advertise
    //   we send our IP address on the WiFi network
    if (!MDNS.begin(lok_name)) {
        Serial.println("Error setting up MDNS responder!");
        while(1) {
            delay(1000);
        }
    }
    Serial.println("btcontrol responder started");

    // Add service to MDNS-SD
    MDNS.addService("_btcontrol", "_tcp", 3030);

    hardware=new ESP32PWM();
    Serial.println("init monstershield done");

    Serial.println("loading message layouts");
    messageLayouts.load();
    Serial.println("loading message layouts - done");

    Serial.println("loading lokdef");
    readLokdef();
    Serial.println("loading lokdef done");

    // Start TCP server
    BTServer.begin();
    Serial.println("TCP server started");

}

class StartupData {
public:
    WiFiClient client;
};

// https://www.freertos.org/a00125.html
int clientID_counter=1;
void startClientThread(void *s) {
    Serial.println("client thread");
    int clientID=clientID_counter++;
    const char *taskname=pcTaskGetTaskName(NULL);
    Serial.printf("task name: %s\n", taskname);
    utils::setThreadClientID(clientID);

    utils::dumpBacktrace();
    
    Serial.printf("local storage: clientID: %d\n", utils::getThreadClientID());
    Serial.println("Free HEAP: " + String(ESP.getFreeHeap()));
    StartupData *startupData=(StartupData *) s;
    /*
    UBaseType_t uxTaskGetSystemState(
                       TaskStatus_t * const pxTaskStatusArray,
                       const UBaseType_t uxArraySize,
                       unsigned long * const pulTotalRunTime );
    */                 
    
    ClientThread *clientThread=NULL;
    try {
        clientThread = new ClientThread(clientID, startupData->client);
        clientThread->run();
    } catch(const char *e) {
        ERRORF("%d: exception %s - client thread killed\n", clientID, e);
    } catch(std::RuntimeExceptionWithBacktrace &e) {
        ERRORF("%d: Runtime Exception [%s] - client thread killed\n", clientID, e.what());
    } catch(std::exception &e) {
        ERRORF("%d: exception %s - client thread killed\n", clientID, e.what());
        /*
    } catch (abi::__forced_unwind&) {
        Serial.printf("%d: exception unwind\n");
        throw; */
    } catch (...) {
        Serial.printf("%d: unknown exception\n", clientID);
    }
    if(clientThread) {
        delete(clientThread);
    }
    delete(startupData);
    Serial.printf("%d: ============= client thread done =============\n", clientID);
    // ohne dem rebootet er mit fehler... [ https://www.freertos.org/FreeRTOS_Support_Forum_Archive/September_2013/freertos_Better_exit_from_task_8682337.html ]
    vTaskDelete( NULL );
}

void handleHTTPRequest(Client &client) {
    Serial.println("handleHTTPRequest");
    String currentLine = "";
    while (client.connected()) {
      Serial.println("handleHTTPRequest connected");
      if (client.available()) {
        Serial.println("handleHTTPRequest available");
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.print(responseHTML);
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    // client.stop();
    Serial.println("handleHTTPRequest done");
}

void loop(void)
{
/*
    Serial.print("loop ");
    Serial.print(ESP.getFreeHeap()); // ./cores/esp32/Esp.h
    Serial.print("/");
    Serial.print(ESP.getMinFreeHeap());
    Serial.print("/");
    Serial.println(ESP.getMaxAllocHeap());
*/
    // Check if a client has connected
    WiFiClient client = BTServer.available();
    if (!client) {
#ifdef HTTPSERVER
        client = HTTPServer.available();
        if(client) {
            Serial.println("New HTTP Client");
            handleHTTPRequest(client);
        } else {
            // Serial.println("loop");
        }
#endif
        return;
    }
    Serial.println("");
    Serial.println("New client");

/*
    // Wait for data from client to become available
    while(client.connected() && !client.available()){
        Serial.println("sleep until available");
        delay(1);
    }
*/

    Serial.println("creating thread");
    StartupData *startupData=new StartupData;
    startupData->client=client;
    //pthread_t thread;
    //int returnValue = pthread_create(&thread, NULL, startClientThread, (void *)&client);
    TaskHandle_t xHandle = NULL;
    BaseType_t xReturned;
    xReturned = xTaskCreate(
                    startClientThread,       /* Function that implements the task. */
                    "client",          /* Text name for the task. */
                    8000,      /* Stack size in words, not bytes. */
                    ( void * ) startupData,    /* Parameter passed into the task. */
                    tskIDLE_PRIORITY,/* Priority at which the task is created. */
                    &xHandle );      /* Used to pass out the created task's handle. */
                    
    if( xReturned != pdPASS ) {
    // if (returnValue) {
        Serial.println("error creating thread: An error has occurred");
    } else {
        Serial.println("thread created");
    }


/*
    // Read the first line of HTTP request
    String req = client.readStringUntil('\r');

    // First line of HTTP request looks like "GET /path HTTP/1.1"
    // Retrieve the "/path" part by finding the spaces
    int addr_start = req.indexOf(' ');
    int addr_end = req.indexOf(' ', addr_start + 1);
    if (addr_start == -1 || addr_end == -1) {
        Serial.print("Invalid request: ");
        Serial.println(req);
        return;
    }
    req = req.substring(addr_start + 1, addr_end);
    Serial.print("Request: ");
    Serial.println(req);
    client.flush();

    String s;
    if (req == "/")
    {
        IPAddress ip = WiFi.localIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP32 at ";
        s += ipStr;
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");
    }
    else
    {
        s = "HTTP/1.1 404 Not Found\r\n\r\n";
        Serial.println("Sending 404");
    }
    client.print(s);

    Serial.println("Done with client");
    */
}

