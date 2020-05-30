// ---------- LIBRARIES ----------
// measuring libraries.
#include <OneWire.h>
#include <DallasTemperature.h>

// network libraries.
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

// miscs libraries.
#include "driver/adc.h"
#include <esp_wifi.h>
#include <esp_bt.h>
#include "esp32-hal-cpu.h"

// ---------- DS18B20 SETUP ----------
#define ONE_WIRE_BUS 2 // Data wire plugged into port 2.
//#define TEMPERATURE_PRECISION 12 // 12 bits precision (0.625°C).

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);


// ---------- NETWORK SETUP ----------
// WiFi settings
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "PASSWORD"

// HTTP Domoticz settings
const char* host = "HOST";
const int   port = 8080;
#define INDEX 32 // index API.
HTTPClient http;

// ---------- DEEP_SLEEP SETUP ----------
#define DEEP_SLEEP_TIME 10 // in minutes.

// ---------- GLOBAL SETUP ---------- 
void setup() {

  // Set frequency low for getting temperature.
  setCpuFrequencyMhz(81);
  delay(1000);

  // Setup debug.
  Serial.begin(115200);

  // Start ADC.
  adc_power_on();          

  // Start sensor(s).
  sensors.begin();

  // - Locate devices on the bus - DEBUG
  //Serial.print("Locating devices...");
  //Serial.print("Found ");
  //Serial.print(sensors.getDeviceCount(), DEC);
  //Serial.println(" devices.");

  // - Report parasite power requirements - DEBUG
  //Serial.print("Parasite power is: "); 
  //if (sensors.isParasitePowerMode()) Serial.println("ON");
  //else Serial.println("OFF");

  // - Set sensor(s) resolution.
  //sensors.setResolution(TEMPERATURE_PRECISION);
  //Serial.println(sensors.getResolution()); // DEBUG

  // Get the device information.
  float temperature = getTemperature();

  // Checking sensor (-127.00 means that the sensor is not connected).
  while (temperature == -127.00) {
    Serial.println("Error getting temperature");
    temperature = getTemperature();
    delay(1000);
  }

  // Set frequency low for enabling wi-fi.
  setCpuFrequencyMhz(80);
  delay(1000);

  // Start WiFi connection.
  setup_wifi();
  
  // Send information to Domoticz host API.
  //sendToDomoticz(temperature);
  delay(500);

  Serial.println(temperature);

  // Deep sleep.
  setCpuFrequencyMhz(10); // For minimal purposes.
  goToDeepSleep();

}

// Wi-Fi connection setup.
void setup_wifi()
{
  // DEBUG
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  // Connection.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Check. (debug?)
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Prints IP. DEBUG
  Serial.println("");
  Serial.println("WiFi connection OK ");
  Serial.print("=> IP ADRESS: ");
  Serial.print(WiFi.localIP());
  Serial.println("");
}

// DeepSleep.
void goToDeepSleep()
{
  Serial.println("Going to sleep...");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();

  adc_power_off();
  esp_wifi_stop();
  esp_bt_controller_disable();

  // Configure the timer to wake it up.
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIME * 60L * 1000000L);

  // Go to sleep.
  esp_deep_sleep_start();
}

// Get sensor temperature.
float getTemperature()
{
  // Request and get sensor temperature.
  sensors.requestTemperatures(); 
  float tempC = sensors.getTempCByIndex(0);

  // Prints temp - DEBUG
  //Serial.print(tempC);
  
  return tempC;
}

void sendToDomoticz(float temperature)
{
  // Domoticz JSON API 
  // /json.htm?type=command&param=udevice&idx=IDX&nvalue=0&svalue=TEMP
  // https://www.domoticz.com/wiki/Domoticz_API/JSON_URL%27s#Temperature

  // Setup URL.
  String url = "/json.htm?type=command&param=udevice&idx=";
    url += String(INDEX);
    url += "&nvalue=0&svalue=";    
    url += String(temperature); 

  // DEBUG
  Serial.print("Connecting to ");
  Serial.println(host);
  Serial.print("Requesting URL: ");
  Serial.println(url);

  // Connecting and sending information to host.
  http.begin(host, port, url);
  int httpCode = http.GET();

  // Checking response.
  if (httpCode) { // exist
    if (httpCode == 200) { // is valid
      String payload = http.getString();

      // DEBUG
      Serial.println("Domoticz response "); 
      Serial.println(payload);
      }
    }
    
  Serial.println("Closing connection."); // DEBUG

  // Closing connection.
  http.end();
}

// --------- GLOBAL LOOP ----------
void loop() 
{
}
