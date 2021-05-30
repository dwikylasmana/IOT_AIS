//Library Initialization
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

//WLAN Config
const char *ssid =  "Eos";
const char *pass =  "aphrodite";

WiFiClient client; //Login as Client

//Adafruit Config
#define MQTT_SERV "io.adafruit.com"
#define MQTT_PORT 1883
#define MQTT_NAME "alphaixion" //Username
#define MQTT_PASS "aio_PcxL05uba6YMzxNzRk3JaIr1RfWx" //API Key

//Adafruit Feeds Hookup (monitor, led, pump)
Adafruit_MQTT_Client mqtt(&client, MQTT_SERV, MQTT_PORT, MQTT_NAME, MQTT_PASS);
Adafruit_MQTT_Publish monitor = Adafruit_MQTT_Publish(&mqtt,MQTT_NAME "/f/monitor");
Adafruit_MQTT_Publish water = Adafruit_MQTT_Publish(&mqtt,MQTT_NAME "/f/water");
Adafruit_MQTT_Subscribe pumpbutton = Adafruit_MQTT_Subscribe(&mqtt, MQTT_NAME "/feeds/pump");
//Variable initialization
const int ledPin = D6;
const int motorPin = D2;
const int buzzer = D1;
const int soilsensor = D3;
const int analog = A0;
unsigned long interval = 10000;
unsigned long previousMillis = 0;
unsigned long interval1 = 1000;
unsigned long previousMillis1 = 0;
float valsoil;

//DML was here
void setup() {
  Serial.begin(115200);
  delay(10);
  mqtt.subscribe(&pumpbutton);
  pinMode(motorPin, OUTPUT); //Pin out mode
  pinMode(ledPin, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(soilsensor, OUTPUT);
  pinMode(A0, INPUT);
  digitalWrite(motorPin, LOW); // keep motor off at start

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

uint32_t x=0;

void loop() {
  MQTT_connect();
  //Soil Sensor Read
    digitalWrite(soilsensor, HIGH);
    delay(10);                
    valsoil = 100-(analogRead(analog)-500)/5.23;
    Serial.print("Soil Moisture is  = ");
    Serial.print(valsoil);
    Serial.println("%"); 
    delay(2000); 
    digitalWrite(soilsensor, LOW); 

    if (valsoil < 50) { //Pumping time
      digitalWrite(motorPin, LOW);
      digitalWrite(ledPin, HIGH);
      tone(buzzer, 1000);
    }
    if (valsoil > 51) { //Stable condition
      digitalWrite(motorPin, HIGH);
      digitalWrite(ledPin, LOW); 
    }
    
    delay(500);
    Serial.print('\n');
    
    if (! monitor.publish(valsoil)){ //Publishing monitor                    
      delay(2000);   
    }

    //Reading subcription (Useless)
    Adafruit_MQTT_Subscribe * subscription;
    while ((subscription = mqtt.readSubscription(5000))){
        if (subscription == &pumpbutton){
          Serial.print(F("On-OFF Pump Button: "));
          Serial.println((char *)pumpbutton.lastread);
          if (strcmp((char *) pumpbutton.lastread, "OFF") == 0){
            digitalWrite(motorPin, LOW);
            delay(3000);
          }
          if (strcmp((char *) pumpbutton.lastread, "ON") == 0){
            digitalWrite(motorPin, HIGH);
          }
        }
    }

    if(! mqtt.ping()) {
      mqtt.disconnect();
    }
    Serial.print('\n');
    delay(1000);
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
