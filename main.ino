// ---------- LIBRARIES ----------
// measuring.
#include <OneWire.h>
#include <DallasTemperature.h>

// network.
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

// miscs.
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
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"

// HTTP Domoticz settings
const char* host = "YOUR_HOST";
const int   port = 8080;
#define INDEX YOUR_INDEX
HTTPClient http;

// ---------- DEEP_SLEEP SETUP ----------
#define DEEP_SLEEP_TIME 10 // in minutes.

// ---------- GLOBAL SETUP ---------- 
void setup() {

  // - Set frequency low for energy economising.
  setCpuFrequencyMhz(81); // Between 10 and 240 Mhz.
  delay(100);
  
  // - Setup debug.
  Serial.begin(115200);

  // - Start ADC.
  adc_power_on();          

  // - Start sensor(s).
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
  while (temperature == -127.00) { // Checking sensor (-127.00 means that the sensor is not connected).
    Serial.println("Error getting temperature");
    temperature = getTemperature();
    delay(1000);
  }

  // - Start WiFi connection.
  setup_wifi();

  // Domoticz JSON API 
  // /json.htm?type=command&param=udevice&idx=IDX&nvalue=0&svalue=TEMPERATURE
  // https://www.domoticz.com/wiki/Domoticz_API/JSON_URL%27s#Temperature

  // - Send temperature to Domoticz host API.
  // Setup URL for temperature.
  String url_t = "/json.htm?type=command&param=udevice&idx=";
    url_t += String(INDEX);
    url_t += "&nvalue=0&svalue=";    
    url_t += String(temperature); 
  
  if (sendToDomoticz(url_t)) {Serial.println("Sent temperature successfully !");}
  else {Serial.println("Cannot send temperature !");}
  delay(500);


  // Domoticz JSON API 
  // json.htm?type=command&param=addlogmessage&message=MESSAGE
  // https://www.domoticz.com/wiki/Domoticz_API/JSON_URL%27s#Add_a_log_message_to_the_Domoticz_log
  
  // - Send log to Domoticz host API.
  // Setup URL for log. (%20 -> space UTF-8)
  String url_m = "/json.htm?type=command&param=addlogmessage&message=";  
    url_m += "ESP32%20INDEX%20";
    url_m += String(INDEX);
    url_m += "%20Temperature%20";
    url_m += String(temperature);
    url_m += "%20Sent%20successfully.";
  
  if (sendToDomoticz(url_m)) {Serial.println("Sent log message successfully !");}
  else {Serial.println("Cannot send log message !");}
  delay(500);

  
  Serial.println(temperature); // Debug

  // - Deep sleep.
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

bool sendToDomoticz(String url)
{
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

      return true;
      }
    }
   else {return false;}
    
  Serial.println("Closing connection."); // DEBUG

  // Closing connection.
  http.end();
}

// --------- GLOBAL LOOP ----------
void loop() 
{
}
