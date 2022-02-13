/*
  esp32-server btcontrol ESP32 Arduino sketch

  Instructions:
  - setup Arduino + ESP32 support (and test it with a simple led bink + wifi sample)
    Modify hardware/espressif/esp32/tools/sdk/include/config/sdkconfig.h:
       set CONFIG_LWIP_MAX_SOCKETS to 20
  - creae config.h with (or copy config.h.sample)
      #define wifi_ap_ssid "wifiname"
      #define wifi_ap_password (***password must be at least 8 characters ***) "password"
      #define lok_name "esp32-lok"
      #define CHINA_MONSTER_SHIELD ---or--- VNH5019_DUAL_SHIELD ---or--- DRV8871
      
    optionally define SOUND FILES
       -> upload SPIFFS with arduino-esp32fs-plugin
          F-F-1.raw F0.raw F0-F-1.raw F0-F1.raw F1.raw .... horn.raw
          at the moment only internal DAC + pam is supporded - GPIO25
          TODO: sound with i2s / MAX98357A
       
  - Flash the sketch to the ESP32 board
  - Install host software:
    - For Linux, install Avahi (http://avahi.org/).
    - For Windows, install Bonjour (http://www.apple.com/support/bonjour/).
    - For Mac OSX and iOS support is built in through Bonjour already.
  - Open the btclient app on your phone




 */


#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
// WifiClient verwendet pthread_*specific => pthread lib wird immer dazu gelinkt
#include <pthread.h>
// fürs esp_wifi_set_ps
#include <esp_wifi.h>
// #include <freertos/task.h>
#include "ESP32PWM.h"
#include "ESP32_MAS_Speed.h"
#include "clientthread.h"
#include "utils.h"
#include "lokdef.h"

// TODO:
// https://arduino.stackexchange.com/questions/23743/include-git-tag-or-svn-revision-in-arduino-sketch
//#include "gitTagVersion.h"

#include "config.h"

#ifdef OTA_UPDATE
// https://lastminuteengineers.com/esp32-ota-updates-arduino-ide/
#include <ArduinoOTA.h>
#endif

// https://github.com/espressif/arduino-esp32/blob/master/libraries/DNSServer/examples/CaptivePortal/CaptivePortal.ino
#ifdef wifi_ap_ssid

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
// => haut irgendwie ned hin, ESP32 crasht dann hin und wieder => fix mit delay(...)

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

#define TAG "main"

Hardware *hardware=NULL;
bool cfg_debug=false;

// TCP server at port 80 will respond to HTTP requests
WiFiServer BTServer(3030);

void init_wifi_start() {
#ifdef RESET_INFO_PIN
    pinMode(RESET_INFO_PIN, OUTPUT);
#endif

    WiFi.persistent(false);

#ifdef DNSSERVER
    DEBUGF("cleanup dnsserver");
    dnsServer.stop();
#endif
#ifdef HTTPSERVER
    DEBUGF("cleanup HTTPServer");
    HTTPServer.end();
#endif

#ifdef wifi_ap_ssid
    WiFi.enableAP(false);
    WiFi.softAPdisconnect();
#endif


#ifdef OTA_UPDATE
    DEBUGF("cleanup update OTA");
    ArduinoOTA.end();
#endif

    DEBUGF("cleanup mdns");
    MDNS.end();

    DEBUGF("disconnect wifi");
    WiFi.disconnect();

    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(lok_name);
}

void init_wifi_done(bool softAP) {
#ifdef RESET_INFO_PIN
    pinMode(RESET_INFO_PIN, INPUT);
#endif

    utils::log.init(softAP);
    // Set up mDNS responder:
    // - first argument is the domain name, in this example
    //   the fully-qualified domain name is "esp8266.local"
    // - second argument is the IP address to advertise
    //   we send our IP address on the WiFi network
    NOTICEF("Setting up MDNS responder. Hostname: %s", lok_name);
    if (!MDNS.begin(lok_name)) {
        ERRORF("Error setting up MDNS responder!");
        while(1) {
            delay(1000);
        }
    }

    // Add service to MDNS-SD
    MDNS.addService("_btcontrol", "_tcp", 3030);
    DEBUGF("btcontrol MDNS started [%s]", lok_name);

  
#ifdef OTA_UPDATE
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  NOTICEF("Setting up ArduinoOTA");
  ArduinoOTA
    .onStart([]() {
      const char *type="unknown";
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      NOTICEF("Start updating %s", type);
#ifdef HAVE_SOUND
      Audio.stop();
#endif
    })
    .onEnd([]() {
      NOTICEF("Update done");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      DEBUGF("Progress: %u%%", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      const char *e="unknown";
      if (error == OTA_AUTH_ERROR) e="Auth Failed";
      else if (error == OTA_BEGIN_ERROR) e="Begin Failed";
      else if (error == OTA_CONNECT_ERROR) e="Connect Failed";
      else if (error == OTA_RECEIVE_ERROR) e="Receive Failed";
      else if (error == OTA_END_ERROR) e="End Failed";
      ERRORF("Error[%u]: %s", error, e);
    });

  // ArduinoOTA.begin startet sonst mdns mit default hostname
  // ArduinoOTA.setHostname(lok_name);
  ArduinoOTA.setMdnsEnabled(false);
  ArduinoOTA.begin();
  MDNS.enableArduino(3232, "");
#endif

}

#ifdef wifi_ap_ssid
void init_wifi_softap() {
    init_wifi_start();
    
    NOTICEF("Setting Access Point=====================================================");
    if(strlen(wifi_ap_password) < 8) {
        ERRORF("ERROR: password < 8 characters, fallback to unencrypted");      
    } else {
        NOTICEF("Setting Access Point ssid:%s password:%s", wifi_ap_ssid, wifi_ap_password);
    }
    // WiFi.mode(WIFI_AP); // 20210627: softAP setzt das, nicht notwendig
    
//    Serial.println("esp_wifi_set_protocol()"); Serial.flush(); delay(200);
// mit LR wirds von normalen Geräten nicht mehr gefunden
// 20210627: ich finde keine Möglichkeit 802.11n beacon + LR damit ein handy auch verwendet werden kann
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html
/*
//     if(esp_wifi_set_protocol( WIFI_IF_AP, WIFI_PROTOCOL_11B| WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR ) != ESP_OK ) {
    if(esp_wifi_set_protocol( WIFI_IF_AP, WIFI_PROTOCOL_11B| WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N ) != ESP_OK ) {
      Serial.println("************ esp_wifi_set_protocol failed ************");
    }
    Serial.println("esp_wifi_set_protocol() done");
  */  
    //delay(2000); // VERY IMPORTANT  https://github.com/espressif/arduino-esp32/issues/2025 --- 20210627: nicht notwendig
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    // bool softAP(const char* ssid, const char* passphrase = NULL, int channel = 1, int ssid_hidden = 0, int max_connection = 4);
    WiFi.softAP(wifi_ap_ssid, wifi_ap_password);
    Serial.print("softAPmacAddress: ");
    Serial.println(WiFi.softAPmacAddress());
    IPAddress IP = WiFi.softAPIP();

    for(int i=0; i < 20; i++) {
      delay(200);
#ifdef RESET_INFO_PIN
      digitalWrite(RESET_INFO_PIN, 1);
      delay(10);
      digitalWrite(RESET_INFO_PIN, 0);
#endif
    }

    // if DNSServer is started with "*" for domain name, it will reply with
    // provided IP to all DNS request
#ifdef DNSSERVER
        NOTICEF("starting DNS server");
        dnsServer.start(DNS_PORT, "*", apIP);
#endif

#ifdef HTTPSERVER
        NOTICEF("starting HTTP server");
        HTTPServer.begin();
#endif
    NOTICEF("IP address: %s",IP.toString().c_str());

    init_wifi_done(true);
}
#endif


#ifdef wifi_client_ssid
void init_wifi_client() {
    init_wifi_start();

    NOTICEF("connecting to wifi %s with mac:%s", wifi_client_ssid, WiFi.macAddress().c_str());
    RETRY_WIFI_CONNECT:
    // Connect to WiFi network
    int APConnectedWait=0;
    WiFi.persistent(false);
    WiFi.begin(wifi_client_ssid, wifi_client_password);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
    
        delay(500);
        Serial.print(".");
#ifdef RESET_INFO_PIN
        digitalWrite(RESET_INFO_PIN, 1);
        delay(10);
        digitalWrite(RESET_INFO_PIN, 0);
#endif
        APConnectedWait++;
        if(APConnectedWait > 40) {
          ERRORF("unable to connect to wifi, disconnecting");
          WiFi.disconnect();
#ifdef RESET_INFO_PIN
          digitalWrite(RESET_INFO_PIN, 1);
          delay(1000);
          digitalWrite(RESET_INFO_PIN, 0);
#endif
          goto RETRY_WIFI_CONNECT;
        }
    }
#ifdef RESET_INFO_PIN
    for(int i=0; i < 10; i++) {
      delay(10);
      digitalWrite(RESET_INFO_PIN, 1);
      delay(10);
      digitalWrite(RESET_INFO_PIN, 0);
    }
#endif
    IPAddress IP = WiFi.localIP();
    long rssi = WiFi.RSSI();
    NOTICEF("Connected to %s IP: %s, rssi: %ld", wifi_client_ssid, IP.toString().c_str(), rssi);
    
    init_wifi_done(false);
}
#endif

void init_wifi() {
#if defined wifi_ap_ssid && defined wifi_client_ssid
    uint8_t ap_client = readEEPROM(EEPROM_WIFI_AP_CLIENT);
    NOTICEF("############# switch Wifi AP / client mode: %d (CV:510) ###########################", ap_client);

    if(ap_client == 0) {
      init_wifi_softap();
    } else {
      init_wifi_client();
    }
#elif defined wifi_ap_ssid
    init_wifi_softap();

#elif defined wifi_client_ssid
    init_wifi_client();

#else
#error neither wifi_ap_ssid nor wifi_client_ssid defined!
#endif

}


void setup(void)
{
    Serial.begin(115200);
    NOTICEF("starting esp32 btcontrol version " __DATE__ " " __TIME__ "====================================");

    // uint32_t realSize = ESP.getFlashChipRealSize();
    uint32_t ideSize = ESP.getFlashChipSize();
    FlashMode_t ideMode = ESP.getFlashChipMode();

    Serial.printf("ChipRevision:   %02X\r\n", ESP.getChipRevision());
    Serial.printf("SketchSize:     %0d\r\n", ESP.getSketchSize());
    Serial.printf("FreeSketchSpace:%d\r\n", ESP.getFreeSketchSpace());

    Serial.printf("Flash ide size:  %u bytes\r\n", ideSize);
    Serial.printf("Flash ide speed: %u Hz\r\n", ESP.getFlashChipSpeed());
    Serial.printf("Flash ide mode:  %s\r\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));
    if(ideMode != FM_QIO) {
      ERRORF("WARNING: Flash not in QIO mode!");
    }
        /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    printf("This is %s chip with %d CPU cores (%s) , WiFi%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            chip_info.model==CHIP_ESP32 ? "ESP32" :
#if ESP_IDF_VERSION_MAJOR >= 4        
            chip_info.model==CHIP_ESP32S2 ? "ESP32-S2" :
#endif
            "unknown",
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    printf("ESP_IDF Version: %s\n", esp_get_idf_version());

    ESP32PWM::dumpConfig();

    init_wifi();

    NOTICEF("loading message layouts");
    messageLayouts.load();
    DEBUGF("loading message layouts - done");

    NOTICEF("loading lokdef");
    readLokdef();
    DEBUGF("loading lokdef done");

    DEBUGF("init hardware");
    hardware=new ESP32PWM(); //muss nach wifi connect sein wegen RESET_INFO_PIN, muss nach readLokdef sein
    DEBUGF("init hardware done");

    // Start TCP server
    BTServer.begin();
    NOTICEF("TCP server started");

    DEBUGF("spi filesystem files:");
    printDirectory();
}

class StartupData {
public:
    WiFiClient client;
};

// https://www.freertos.org/a00125.html
int clientID_counter=1;
void startClientThread(void *s) {
    DEBUGF("==============client thread======================");
    int clientID=clientID_counter++;
    const char *taskname=pcTaskGetTaskName(NULL);
    DEBUGF("task name: %s", taskname);
    utils::setThreadClientID(clientID);

    // utils::dumpBacktrace();

    DEBUGF("local storage: clientID: %d", utils::getThreadClientID());
    DEBUGF("Free HEAP: %d",ESP.getFreeHeap());
    StartupData *startupData=(StartupData *) s;
    /*
    UBaseType_t uxTaskGetSystemState(
                       TaskStatus_t * const pxTaskStatusArray,
                       const UBaseType_t uxArraySize,
                       unsigned long * const pulTotalRunTime );
    */                 

#if not defined SOFTAP
    DEBUGF("disable wifi power saving");
    esp_wifi_set_ps(WIFI_PS_NONE);
#endif
#ifdef HAVE_SOUND
    Audio.begin();
#endif
    ClientThread *clientThread=NULL;
    try {
        clientThread = new ClientThread(clientID, startupData->client);
        clientThread->run();
    } catch(const char *e) {
        ERRORF("%d: exception %s - client thread killed", clientID, e);
    } catch(std::RuntimeExceptionWithBacktrace &e) {
        ERRORF("%d: Runtime Exception [%s] - client thread killed", clientID, e.what());
    } catch(std::exception &e) {
        ERRORF("%d: exception %s - client thread killed", clientID, e.what());
        /*
    } catch (abi::__forced_unwind&) {
        Serial.printf("%d: exception unwind\n");
        throw; */
    } catch (...) {
        ERRORF("%d: unknown exception", clientID);
    }
    if(clientThread) {
        delete(clientThread);
    }
    delete(startupData);
    NOTICEF("%d: ============= client thread done =============", clientID);
#if not defined SOFTAP
    if(TCPClient::numClients == 0) {
        DEBUGF("enable wifi power saving");
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    }
#endif
#ifdef HAVE_SOUND
    if(TCPClient::numClients == 0) {
        Audio.stop();
    }
#endif
    // ohne dem rebootet er mit fehler... [ https://www.freertos.org/FreeRTOS_Support_Forum_Archive/September_2013/freertos_Better_exit_from_task_8682337.html ]
    vTaskDelete( NULL );
}

#ifdef HTTPSERVER
void handleHTTPRequest(Client &client) {
    NOTICEF("handleHTTPRequest");
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
    DEBUGF("handleHTTPRequest done");
}
#endif

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
    if(hardware) {
      ESP32PWM *esp32pwm=(ESP32PWM *) hardware;
      esp32pwm->loop();
    }
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
#ifdef OTA_UPDATE
        ArduinoOTA.handle();
#endif
        return;
    }
    DEBUGF("");
    DEBUGF("New client");

/*
    // Wait for data from client to become available
    while(client.connected() && !client.available()){
        Serial.println("sleep until available");
        delay(1);
    }
*/

    DEBUGF("creating thread");
#warning das auf thread.run umstellen
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
        ERRORF("error creating thread: An error has occurred");
    } else {
        DEBUGF("thread created");
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

