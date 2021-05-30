//Library Initialization
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Adafruit_ADS1015.h>
#include <Wire.h>

//WLAN Config
const char *ssid =  "Eos";
const char *pass =  "aphrodite";

WiFiClient asclient; //Login as Client

//Adafruit Config
#define MQTT_SERV "io.adafruit.com"
#define MQTT_PORT 1883
#define MQTT_NAME "alphaixion" //Username
#define MQTT_PASS "aio_PcxL05uba6YMzxNzRk3JaIr1RfWx" //API Key

//Adafruit Feeds Hookup (monitor, led, pump)
Adafruit_MQTT_Client mqtt(&asclient, MQTT_SERV, MQTT_PORT, MQTT_NAME, MQTT_PASS);
Adafruit_MQTT_Publish soil_sensor = Adafruit_MQTT_Publish(&mqtt,MQTT_NAME "/f/monitor");
Adafruit_MQTT_Publish water_level = Adafruit_MQTT_Publish(&mqtt,MQTT_NAME "/f/water");
Adafruit_MQTT_Subscribe pumpbutton = Adafruit_MQTT_Subscribe(&mqtt, MQTT_NAME "/feeds/pump");
Adafruit_MQTT_Subscribe pumpmanual = Adafruit_MQTT_Subscribe(&mqtt, MQTT_NAME "/feeds/pumpmanual");

//Variable initialization
const int ledPin = D6;
const int motorPin = D8;
const int buzzer = D7;
const int soilsensor = D3;
const int analog = A0;
unsigned long interval = 10000;
unsigned long previousMillis = 0;
unsigned long interval1 = 1000;
unsigned long previousMillis1 = 0;
int valsoil;
int valwater;
int tilt;
int b = 10;

//Initialize Module ADS-1115
Adafruit_ADS1015 ads1015;
const float multiplier = 0.125F;
float adc01;
float adc02;

//DML was here
void setup() {
  Serial.begin(115200); //Baud rate
  delay(10);
  mqtt.subscribe(&pumpbutton);
  mqtt.subscribe(&pumpmanual);
  pinMode(motorPin, OUTPUT); //Pin out mode
  pinMode(ledPin, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(soilsensor, OUTPUT);
  pinMode(A0, INPUT);
  digitalWrite(motorPin, LOW); // keep motor off at start
  ads1015.begin();

//WLan connect
  Serial.println("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");              // print ... till not connected
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

//Establish adafruit's cloud connection
void MQTT_connect() 
{
  int8_t ret;
  // Stop if already connected.
  if (mqtt.connected()) 
  {
    return;
  }

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) // connect will return 0 for connected
  { 
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) 
       {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
}

uint32_t x=0;

void loop() {
  MQTT_connect();
  int16_t adc0, adc1, adc2, adc3;
  
//Soil Sensor Read
    digitalWrite(soilsensor, HIGH);
    delay(10);                
    adc0 = ads1015.readADC_SingleEnded(0);
    valsoil = (100-(analog-500))/5.23;
    Serial.print("Soil Moisture is  = ");
    Serial.print(valsoil);
    Serial.println("%"); 
    Serial.print("Soil Sensor Raw = ");
    Serial.print(analog);
    delay(2000); 
    digitalWrite(soilsensor, LOW);
    Adafruit_MQTT_Subscribe * subscription;
    while ((subscription = mqtt.readSubscription(5000))){
        if (subscription == &pumpbutton){
          Serial.print(F("On-OFF Pump Button: "));
          Serial.println((char *)pumpbutton.lastread);
          if (strcmp((char *) pumpbutton.lastread, "OFF") == 0){
      
          }
          if (strcmp((char *) pumpbutton.lastread, "ON") == 0){
            if (valsoil < 50 && tilt == LOW) { //Pumping time
              digitalWrite(motorPin, LOW);
              for(int a = 0; a<b; a++)
              {
                 digitalWrite(ledPin, HIGH);
                 delay(500);
                 digitalWrite(ledPin, LOW);
                 delay(500);
              }

            }
            if (valsoil <50 && tilt == HIGH) { //Stop Pumping
              digitalWrite(motorPin, HIGH);
              for(int a = 0; a<b; a++)
              {
                 digitalWrite(ledPin, HIGH);
                 tone(buzzer, 1000);
                 delay(500);
                 digitalWrite(ledPin, LOW);
                 noTone(buzzer);
                 delay(500);
              }
            }
            if (valsoil > 51) { //Stable condition
              digitalWrite(motorPin, HIGH);
            }
          }
        }  
        if (subscription == &pumpmanual){
          Serial.print(F("On-OFF Pump Button: "));
          Serial.println((char *)pumpbutton.lastread);
          if (strcmp((char *) pumpbutton.lastread, "OFF") == 0){
            digitalWrite(motorPin, HIGH);
          }
          if (strcmp((char *) pumpbutton.lastread, "ON") == 0){
            digitalWrite(motorPin, LOW);
            delay(3000);
            digitalWrite(motorPin, HIGH);
          }
        } 
    }
    delay(500);
    if (!soil_sensor.publish(valsoil)){ //Publishing monitor                    
      delay(1000);   
    }
    Serial.print('\n');
    
//Water Level Sensor Read                    
    Serial.print("Water Level is  = ");
    adc1 = ads1015.readADC_SingleEnded(1);
    valwater = (adc1-150)/7.87;
    Serial.print(valwater);
    Serial.print("%");
    Serial.print("Water Sensor Raw = ");
    Serial.print(adc1);
    delay(2000);
    if (!water_level.publish(valwater)){ //Publishing monitor                    
       delay(1000);   
    }
    Serial.print('\n');
    if(! mqtt.ping()) {
      mqtt.disconnect();
    }
    delay(1000);
}
