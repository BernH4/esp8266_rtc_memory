//Credits to Andreas Spies @YouTube, code is partly from there
/* file:///tmp/mozilla_bs0/2c-esp8266_non_os_sdk_api_reference_en.pdf */

//Data can be written to RTC in buckets of 4 Bytes each,
//userdata can be stored in Bucket 64 to 127

ADC_MODE(ADC_VCC); //vcc read-mode
#include <ESP8266WiFi.h>
#include <credentials.h>
extern "C" {
#include "user_interface.h" // this is for the RTC memory read/write functions
}
#define RTCMEMORYSTART 66 // starting at bucket 66 because rtcManagement needs 2
//Set this to the ammount of cycles before sending data !127 / data_bucketsize / 4 is maximum!
//If your data is more than 1 Bucket you have to multiply it by data_bucketsize
#define RTCMEMORYLEN 6 
#define TRUE 1 //Integer instead of Bool because datatype size of 4 byte
#define VCC_ADJ  1.0   // measure with your voltmeter and calculate that the number mesured from ESP is correct

#define SLEEPTIME 1000000 //one sec

typedef struct {
  int initialized; //Integer instead of Bool because datatype size of 4 byte
  int valueCounter;
} rtcManagementStruc;

rtcManagementStruc rtcManagement;

typedef struct {
  float battery; 
  int other;
} rtcStore;

rtcStore rtcMem;

int i;
int data_bucketsize;
unsigned long startTime;

WiFiClient client;


void setup() {
  Serial.begin(115200);
  Serial.print("Booted, ");

  data_bucketsize = (sizeof(rtcMem)+3)/4; //using integer cutoff

  Serial.print("Buckets needed for data: ");
  Serial.println(data_bucketsize);
  Serial.print("Max possibles iterations with data_bucketsize_are:");
  Serial.print(127 / data_bucketsize);
  system_rtc_mem_read(64, &rtcManagement, 8);

  if (rtcManagement.initialized != TRUE) {
    rtcManagement.valueCounter = 0;
    rtcManagement.initialized = TRUE;
    system_rtc_mem_write(64, &rtcManagement, 8);
    Serial.println("Initial values set");
    /* ESP.deepSleep(10, WAKE_RF_DISABLED); */ //Why should we sent to sleep afterwards? Go continue with measurement
  }
  if (rtcManagement.valueCounter <= RTCMEMORYLEN / data_bucketsize) {
    Serial.println("Sensor reads");

    rtcMem.battery = ESP.getVcc() * VCC_ADJ ;
    rtcMem.other = rtcManagement.valueCounter;

    int rtcPos = RTCMEMORYSTART + rtcManagement.valueCounter * data_bucketsize;
    system_rtc_mem_write(rtcPos, &rtcMem, data_bucketsize * 4);
    /* system_rtc_mem_write(rtcPos, &rtcMem, 4); */
    system_rtc_mem_write(64, &rtcManagement, 4);

    Serial.print("Counter : ");
    Serial.print(rtcManagement.valueCounter);
    Serial.print(" Position: ");
    Serial.print(rtcPos);
    Serial.print(", battery: ");
    Serial.print(rtcMem.battery);
    rtcManagement.valueCounter++;
    system_rtc_mem_write(64, &rtcManagement, 8);
    Serial.println("before sleep W/O RF");
    ESP.deepSleep(SLEEPTIME, WAKE_NO_RFCAL);
  }
  else {  // Send to Cloud
    startTime = millis();
    Serial.println();
    Serial.println();
    WiFi.mode(WIFI_STA);
    Serial.print("Start Sending values. Connecting to ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWD);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Sending ");

    for (i = 0; i <= RTCMEMORYLEN / data_bucketsize; i++) {
      int rtcPos = RTCMEMORYSTART + i * data_bucketsize;
      system_rtc_mem_read(rtcPos, &rtcMem, sizeof(rtcMem));
      //SEND YOUR DATA HERE 
      //Or accumulate data end send once
      Serial.print("i: ");
      Serial.print(i);
      Serial.print(" Position ");
      Serial.print(rtcPos);
      Serial.print(", battery: ");
      Serial.print(rtcMem.battery);
      Serial.print(", other: ");
      Serial.println(rtcMem.other);
      yield();
    }
    rtcManagement.valueCounter = 0;
    system_rtc_mem_write(64, &rtcManagement, 8);
    Serial.print("Writing to Cloud done. It took ");
    Serial.print((millis() - startTime) / 1000.0);
    Serial.println(" Seconds ");
    ESP.deepSleep(SLEEPTIME, WAKE_NO_RFCAL);
  }
}
void loop() {}
