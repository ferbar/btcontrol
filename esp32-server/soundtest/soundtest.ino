#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoOTA.h>
#include "SPIFFS.h"
// #include "esp_idf_version.h"
#include "config.h"

#include "ESP32_MAS.h"

class ESP32_MAS_Speed : public ESP32_MAS {

  virtual void openFile(uint8_t channel, File &f) {
    if(channel != 0) {
      return ESP32_MAS::openFile(channel, f);
    }
    printf("openFile [%d] fahrstufe=%d\n", channel, this->curr_fahrstufe);
    String filename;
    if(this->curr_fahrstufe == -1 && this->target_fahrstufe == -1) {
      f.close();
      this->stopChan(channel);
    } else if(this->curr_fahrstufe == this->target_fahrstufe) {
      filename=String("/F") + this->curr_fahrstufe + ".raw";
    } else {
      if(this->curr_fahrstufe > this->target_fahrstufe) {
        filename=String("/F")+this->curr_fahrstufe+"-F"+(this->curr_fahrstufe-1)+".raw";
        this->curr_fahrstufe--;
      } else {
        filename=String("/F")+this->curr_fahrstufe+"-F"+(this->curr_fahrstufe+1)+".raw";        
        this->curr_fahrstufe++;
      }
    }
    if(filename != f.name()) {
      if(filename) {
        printf("f.name=%s\n", f.name() ? f.name() : "null");
        printf("open file [%s]\n", filename.c_str());
        f = SPIFFS.open(filename, "r");
        if(!f) {
          printf("open failed\n");
        }
      } else {
        // stille
      }
    } else {
      printf("seek 0 file=%s, fahrstufe=%d\n", filename.c_str(), this->curr_fahrstufe);
      f.seek(0); // seek0 is nicht viel schneller als neues open !!!!
    }
    switch (this->Channel[channel]) {
      case 2:
        this->Channel[channel] = 5;
        break;
      case 3:
        this->Channel[channel] = 4;
        break;
      case 4:
        this->Channel[channel] = 4;
        break;
    }
  };

public:
// -1 => stille
// 0 => stand
  void setFahrstufe(int fahrstufe) {
    assert(fahrstufe >= -1 && fahrstufe <= 5);
    this->target_fahrstufe=fahrstufe;
  }
  
  int curr_fahrstufe=-1;
  int target_fahrstufe=-1;
};
ESP32_MAS_Speed Audio;

WiFiServer HTTPServer(80);


void setup() {  
    Serial.begin(115200);
    Serial.println("starting esp32 soundtest");

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
//    printf("ESP_IDF Version: %d.%d-%d\n", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
  
    // Connect to WiFi network
    WiFi.begin(wifi_ssid, wifi_password);
    Serial.printf("connecting to wifi %s...\r\n", wifi_ssid);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    IPAddress IP = WiFi.localIP();
    Serial.print("Connected to ");
    Serial.println(wifi_ssid);
    Serial.print("IP address: ");
    Serial.println(IP);

  
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
      Audio.stopDAC();
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();

    Serial.println("starting SPIFFS");
    SPIFFS.begin();
    delay(500);
    if (!SPIFFS.begin()) {
      Serial.println("SPIFFS Mount Failed");
    } else {
      Serial.println("successfully mounted SPIFFS");
    }
    
    Serial.println("starting HTTP server");
    HTTPServer.begin();
    
}

String responseHTML = ""
  "<!DOCTYPE html><html><head><title>" lok_name "</title></head><body>"
  "<h1>" lok_name "</h1><p>test</p> "
  "</body></html>";

void handleHTTPRequest(Client &client) {
    Serial.println("handleHTTPRequest");
    String currentLine = "";
    String path="";
    while (client.connected()) {
      //Serial.println("handleHTTPRequest connected");
      if (client.available()) {
        //Serial.println("handleHTTPRequest available");
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            Serial.print("Path: "); Serial.println(path);
            if(path.startsWith("/volume/")) {
              String s=path.substring(strlen("/volume/"));
              int v=s.toInt();
              client.printf("volume %d\n",v);
              Audio.setVolume(v);
            } else if(path == "/start") {
              if (SPIFFS.begin()) {
                client.println("start");
                Audio.setDAC(true);
                Audio.startDAC();
                Audio.setFahrstufe(1);
                Audio.setGain(0,20);
                Audio.loopFile(0,""); // openFile handles the filenames
              } else {
                client.println("error spiffs");
              }
            } else if(path == "/stop") {
              client.println("stop");
              // Audio.stopDAC();
              // Audio.outChan(0);
              Audio.setFahrstufe(-1);
            } else if(path == "/play") {
              client.println("play");
              if (SPIFFS.begin()) {
                if(!Audio.isRunning()) {
                  client.println("startingDAC");
                  Audio.setDAC(true);
                  Audio.startDAC();
                  Audio.setGain(0,30);
                }
                Audio.playFile(0,"/Motor_Start.raw");
                vTaskDelay(pdMS_TO_TICKS(100));
                Audio.loopFile(0,"/F0.raw");
            
              }
            } else if(path == "/horn") {
              client.println("horn");
              Audio.playFile(1,"/horn.raw");
             
            } else if(path == "/spi/list") {
              client.println("spi/list");
              if(SPIFFS.begin()) {
                File root = SPIFFS.open("/");
                if(root.isDirectory()) {
 
                  File file = root.openNextFile();
 
                  while(file){ 
                    client.print("FILE: ");
                    client.println(file.name());
                    file = root.openNextFile();
                  }
                } else {
                  client.println("/ is not a directory");
                }
              } else {
                client.println("/ doesn't exist");
              }
            } else if(path == "/reset") {
              Audio.stopDAC();
              client.print("reset");
              client.stop();
              delay(100);
              ESP.restart();
            } else {
              client.print(responseHTML);
            }
            break;
          } else {
            Serial.println("header line:");
            Serial.println(currentLine);
            if(currentLine.startsWith("GET ")) {
              int pathEnd=currentLine.indexOf(" ", 4);
              if( pathEnd > 4) {
                path=currentLine.substring(4,pathEnd);
              } else {
                path=currentLine;
              }
            }
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

void loop() {
  // put your main code here, to run repeatedly:
  ArduinoOTA.handle();
  WiFiClient client = HTTPServer.available();
  if(client) {
      Serial.println("New HTTP Client");
      handleHTTPRequest(client);
  }
}

