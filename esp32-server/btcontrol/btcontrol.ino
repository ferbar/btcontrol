/*
  ESP32 mDNS responder sample

  This is an example of an HTTP server that is accessible
  via http://esp32.local URL thanks to mDNS responder.

  Instructions:
  - Update WiFi SSID and password as necessary.
  - Flash the sketch to the ESP32 board
  - Install host software:
    - For Linux, install Avahi (http://avahi.org/).
    - For Windows, install Bonjour (http://www.apple.com/support/bonjour/).
    - For Mac OSX and iOS support is built in through Bonjour already.
  - Point your browser to http://esp32.local, you should see a response.

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

#include "wifi_sid_password.h"

static const char *TAG="main";

Hardware *hardware=NULL;
bool cfg_debug=false;

// TCP server at port 80 will respond to HTTP requests
WiFiServer server(3030);

void setup(void)
{  
    Serial.begin(115200);

    // Connect to WiFi network
    WiFi.begin(ssid, password);
    Serial.println("connecting to wifi ...");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Set up mDNS responder:
    // - first argument is the domain name, in this example
    //   the fully-qualified domain name is "esp8266.local"
    // - second argument is the IP address to advertise
    //   we send our IP address on the WiFi network
    if (!MDNS.begin("esp32")) {
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
    server.begin();
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

void loop(void)
{
    // Check if a client has connected
    WiFiClient client = server.available();
    if (!client) {
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

