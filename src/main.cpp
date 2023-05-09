/* 
This proyect is a weather station for a final test in the degree of bachelor Electronics Engineer Universidad Tecnologica Nacional
Creator: Alcoba Ivan - Pennisi Gianfranco
Subject: Medidas Electronicas I / Tecnologia Electronica
*/
#include <Arduino.h>
#include "DHT.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085.h>
#include <SPI.h>
#include <Wire.h>
#include <ThingSpeak.h>
#include <WiFi.h>
#include <BH1750.h>

/*Defines*/
#define windSen 0
#define uS_Factor 1000000ULL
#define TIME_TO_SLEEP 570

WiFiClient client;                                                  //WiFi connection details
char ssid[] = "WIFI NAME";                                          //WiFi Name
char pass[] = "WIFI PASSWORD"; 

unsigned long myChannelNumber; //Your channel number                    //Thingspeak channel number
const char * myWriteAPIKey; //Your channel API KEY     

/*Variables*/
float temp = 0.0;
float hum = 0.0;
float pressure = 0.0;
float lux = 0.0;
int wind = 0;
unsigned long firstMillis = 0;                                       //Timers for the wind speed calculation
unsigned long lastMillis = 0;
unsigned long lastIntTime = 0;
int counter = 0;  
Adafruit_BMP085 bmp;
DHT dht(14,DHT22);
BH1750 lightMeter;

/*Function prototypes*/
void readDHT22();
void readPressure();
void readLight();
void calcWind();
void updateThingSpeak();

void IRAM_ATTR isr ()                                                //Interrupt routine, run with each reed switch interrupt
{
  unsigned long intTime = millis();
  if(intTime - lastIntTime > 150)                                    //Debounce the reed switch input
  {
    if (counter == 0)
      firstMillis = millis();
    counter++;                                                       //Count each revolution
    lastMillis = millis();
    //Serial.println(counter);
  }
  lastIntTime = intTime;                                             //Capture the first and last revolution time
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  dht.begin();
  if(!bmp.begin(0x76))
  {
    Serial.println("BMP180 Sensor not found !");
  }
  WiFi.begin(ssid, pass);                                            //Connect to WiFi network
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  bool status = lightMeter.begin();
  Serial.println(status);
  pinMode(windSen, INPUT_PULLUP);
  attachInterrupt(windSen, isr, RISING);  
  //esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_Factor);
  ThingSpeak.begin(client);
  delay(10000);                                                      //Wait for wind speed readings to be taken
 
//  updateThingSpeak ();                                               //Post the data to Thingspeak
 // Serial.println("Going to sleep now");
 // Serial.flush(); 
 // esp_deep_sleep_start(); 
}

void loop() {

  readDHT22();
  readPressure();
  readLight();
  calcWind ();
  counter = 0;
  Serial.print("Light: ");                                           //Display readings on serial monitor
  Serial.println(lux);
  Serial.print("Temp: ");
  Serial.println(temp);
  Serial.print("Humidity: ");
  Serial.println(hum);
  Serial.print("Pressure: ");
  Serial.println(pressure);
  Serial.print("Wind: ");
  Serial.println(wind); 
  Serial.println("");
  updateThingSpeak ();

  delay(20000);
}

void readDHT22()
{
  temp = dht.readTemperature();
  hum = dht.readHumidity();
}

void readPressure()
{
  pressure = bmp.readPressure()/100;
}

void readLight()
{
  lux = lightMeter.readLightLevel();
}

void calcWind ()                                                     //Function to calculate the wind speed
{
  int ave = 5000;
  if(counter != 0)
    ave = (lastMillis - firstMillis)/counter;
  Serial.print("Average Tick Time: ");
  Serial.println(ave);
  if (ave < 200)
  {
    ave = 200;
    wind = map (ave,200, 4000, 3, 100);
  }
  else if (ave > 4000)
    wind = 0;
  else
  {
    wind = map (ave,200, 4000,3 , 100);
  }
}

void updateThingSpeak ()                                             //Function to post data to Thingspeak
{
  ThingSpeak.setField(1, lux);
  ThingSpeak.setField(2, temp);
  ThingSpeak.setField(3, hum);
  ThingSpeak.setField(4, pressure);
  ThingSpeak.setField(5, wind);
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200)
  {
    Serial.println("Channel update successful.");
  }
  else
  {
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
}