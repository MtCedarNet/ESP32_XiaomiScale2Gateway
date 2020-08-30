#include <stdio.h>

#include <WiFi.h>
#include <WiFiClient.h>

#include <BLEDevice.h>
#include <BLEAdvertisedDevice.h>

//#define USE_OLED

#ifdef USE_OLED
#include <OLEDDisplay.h>
#include <SSD1306Wire.h>

SSD1306Wire display(0x3c, 5, 4, GEOMETRY_128_64);
OLEDDISPLAY_COLOR c;
#endif

BLEScan* pBLEScan;
BLEAddress targetaddr = BLEAddress("c4:6b:40:******TARGET_DEVICE_ADDRESS_HERE*******");

char* baseURL = "http://maker.ifttt.com/trigger/Scale2/with/key/*********KEY_HERE**********";
char* hostname = "maker.ifttt.com";

const char* ssid     = "****SSID_HERE****";
const char* password = "****WIFI_PASSWORD_HERE*****";
WiFiClient client;

int laststate=0;

#define BUFSIZE 5
#define SENDSPAN 8

int bufpos=0;
char sendbuf[BUFSIZE][100];
int tosendtick=0;

void connectWiFi()
{
  int count=0;
  char tmp[30];
  
  if(WiFi.status() == WL_CONNECTED) return;
  retry:
  WiFi.disconnect();
  WiFi.begin(ssid, password);

#ifdef USE_OLED
  display.init();
  display.setFont(ArialMT_Plain_24);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setColor(BLACK);
  display.fillRect(0,30, 100,30);
  sprintf(tmp, "WiFi: %d", count);
  display.setColor(WHITE);
  display.drawString(0,30, tmp);
  display.display();
  display.end();
#endif

  Serial.print("\nWiFi connecting...");
  for(int i=0;i<10;i++){
    if(WiFi.status() == WL_CONNECTED) {
      Serial.print(" connected. \n");
      break;
    }
    Serial.print(WiFi.status());
//    display.drawString(i*10,32, ".");
//    display.display();
    delay(500);
  }
  if(WiFi.status() != WL_CONNECTED && count++<10){
    goto retry;
  }
  Serial.println(WiFi.localIP());
}

int postdata(char *params){
  int count=0;

  retry:
  connectWiFi();

  Serial.println("\nStarting connection to server...");
  if (!client.connect(hostname, 80)) {
    Serial.println("Server connection failed!");
    if((++count)<10){
      goto retry;
    }
    return -1;
  } else {
    Serial.println("Connected to server!");

    // Make a HTTP request:
    client.print("GET ");
    client.print(baseURL);
    client.print(params);    
    client.print(" HTTP/1.0\n\n");
//    Serial.println("Reqest sent!");
    delay(1000);
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }
//    Serial.println(":request done.");
  }
  client.stop();
  WiFi.disconnect();
  return count;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    char tmp[100],date[30];
    // アドバタイジングデータを受け取ったとき
    BLEAddress addr = advertisedDevice.getAddress();
    if(addr.equals(targetaddr)){
      //Serial.print("Advertised Device found: ");
      //Serial.println(advertisedDevice.toString().c_str());
      std::string svdata = advertisedDevice.getServiceData();
      int status = svdata[1];
      // status bits: [Memory] 0 [Fixed] 0  0 1 [Impedance] 0

      if(((status& 0xA0) == 0x20) && laststate != status){
        float weight = ((float)((int)svdata[12]<<8)+svdata[11])/200;
        int impe = ((int)svdata[10] << 8) + svdata[9];
        snprintf(date, 30, "%04d-%02d-%02d%%20%02d:%02d:%02d", ((int)svdata[3] << 8) + svdata[2], svdata[4],svdata[5],svdata[6],svdata[7],svdata[8]);
        Serial.println(date);

#ifdef USE_OLED
        snprintf(tmp, sizeof(tmp), "%2.1fkg", weight);
        display.init();
        display.clear();
        display.drawString(5,2, tmp);
        display.display();
        display.end();
#endif

        if(bufpos<BUFSIZE-1){
          tosendtick = SENDSPAN;
          snprintf(sendbuf[bufpos], sizeof(tmp), "?value1=%s&value2=%2.1f&value3=%d\n",date, weight, impe);
          bufpos++;
        }
        if(bufpos==BUFSIZE){
          tosendtick = 0;
        }
      }
      laststate = status;
    }
  }
};

void setup() {
  BLEDevice::init("Scale2Gateway");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setActiveScan(false);  // パッシブスキャンに設定
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  Serial.begin(115200);

#ifdef USE_OLED
  display.init();
  display.resetDisplay();
  display.setFont(ArialMT_Plain_24);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setColor(WHITE);
  display.drawString(0,20, "Initialized");
  display.display();

  delay(2000);
  display.clear();
  display.display();
  display.displayOff();
  display.end();
#endif
}

void loop() {
  int ret;
  char tmp[50];
  
  BLEScanResults foundDevices = pBLEScan->start(1);
  tosendtick--;
  snprintf(tmp,50,"tosend:%d  bufpos:%d", tosendtick,bufpos);
  Serial.println(tmp);
  if(tosendtick<0 && bufpos>0){
    for(int i=0;i<bufpos;i++){
      ret = postdata(sendbuf[i]);
    }
    bufpos=0;

#ifdef USE_OLED
    display.init();
    display.setColor(BLACK);
    display.fillRect(0,30, 100,40);
    display.setColor(WHITE);
    display.drawString(0,20, ret>=0 ? "Post done!" : "Post FAIL!");
    display.display();

    delay(2000);
    display.end();
    delay(200);
    display.init();
    display.clear();
    display.display();
    display.displayOff();
    display.end();
#endif
  }
}
