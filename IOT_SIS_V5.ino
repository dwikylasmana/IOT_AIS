//Library Initialization
#include <ESP8266WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>

//WLAN Config
const char *ssid =  "Tesla";
const char *pass =  "authorized";
WiFiClient asclient;

//Adafruit Config
#define MQTT_SERV "io.adafruit.com"
#define MQTT_PORT 1883
#define MQTT_NAME "alphaixion" //Username
#define MQTT_PASS "aio_PcxL05uba6YMzxNzRk3JaIr1RfWx" //API Key

//Adafruit Feeds Hookup
Adafruit_MQTT_Client mqtt(&asclient, MQTT_SERV, MQTT_PORT, MQTT_NAME, MQTT_PASS);
Adafruit_MQTT_Publish soil_sensor = Adafruit_MQTT_Publish(&mqtt,MQTT_NAME "/feeds/iot-lolin.soil-sensor");
Adafruit_MQTT_Publish water_level = Adafruit_MQTT_Publish(&mqtt,MQTT_NAME "/feeds/iot-lolin.water-level");
Adafruit_MQTT_Subscribe pumpbutton = Adafruit_MQTT_Subscribe(&mqtt, MQTT_NAME "/feeds/iot-lolin.pump-button");
Adafruit_MQTT_Subscribe pumpmanual = Adafruit_MQTT_Subscribe(&mqtt, MQTT_NAME "/feeds/iot-lolin.pump-manual");
Adafruit_MQTT_Publish tempC = Adafruit_MQTT_Publish(&mqtt,MQTT_NAME "/feeds/iot-lolin.temperature");

//Variable initialization
const int soilsensor = D0;
const int buzzer = D1;
const int ledPin = D2;
const int watersensor = D3;
const int motorPin = D4;
const int dhtpin = D6;
const int analog = A0;
unsigned long interval = 10000;
unsigned long previousMillis = 0;
unsigned long interval1 = 1000;
unsigned long previousMillis1 = 0;
float valsoil;
float valwater;
int b = 10;

//DHT Sensor
#define DHTPIN D5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//DML was here
void setup() {
  Serial.begin(115200);
  delay(10);
  mqtt.subscribe(&pumpbutton);
  mqtt.subscribe(&pumpmanual);
  pinMode(motorPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(soilsensor, OUTPUT);
  pinMode(watersensor, OUTPUT);
  pinMode(dhtpin, OUTPUT);
  pinMode(A0, INPUT);
  digitalWrite(motorPin, HIGH); // keep motor off at start
  dht.begin();

//WLan connect
  Serial.println("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print("*");              // print ... till not connected
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void alarm(){
  for(int a = 0; a<b; a++)
  {
    digitalWrite(ledPin, HIGH);
    tone(buzzer, 8000);
    delay(500);
    digitalWrite(ledPin, LOW);
    noTone(buzzer);
    delay(500);
  }
}


//Establish adafruit's cloud connection
void MQTT_connect() 
{
  int8_t ret;
  // Stop if already connected.
  if (mqtt.connected()) 
  {
    return;
    Serial.print("Cloud Connected!");
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

void pump(){
  alarm();
  digitalWrite(motorPin, LOW);
  delay(1500);
  digitalWrite(motorPin, HIGH);
  Serial.print("Pumping Succesful");
}

void loop() {
    MQTT_connect();
    
  //Soil Sensor Read
    digitalWrite(soilsensor, HIGH);
    delay(100);                
    valsoil = 100-(analogRead(analog)-600)/4.23;
    Serial.print("Soil Moisture is  = ");
    Serial.print(valsoil);
    Serial.println("%"); 
    Serial.print("Soil Sensor Raw = ");
    Serial.print(analogRead(analog));
    delay(2000); 
    digitalWrite(soilsensor, LOW); 
    if (valsoil < 50) { //Pumping time
        pump();
    }
    if (valsoil > 51) { //Stable condition
        digitalWrite(motorPin, HIGH);
        digitalWrite(ledPin, LOW); 
        noTone(buzzer);
    }
    Adafruit_MQTT_Subscribe * subscription;
    while ((subscription = mqtt.readSubscription(5000))){
       if (subscription == &pumpbutton){
            Serial.print(F("On-OFF Pump Button: "));
            Serial.println((char *)pumpbutton.lastread);
              if (strcmp((char *) pumpbutton.lastread, "ON") == 0){
                pump();
              }
        if (strcmp((char *) pumpbutton.lastread, "OFF") == 0){
                digitalWrite(motorPin, HIGH);
                delay(500);
        }
        } 
    }
    delay(500);
    Serial.print('\n');
    if (! soil_sensor.publish(valsoil)){ //Publishing monitor                    
      delay(2000);   
    }

//Water Level Sensor Read   
    digitalWrite(watersensor, HIGH);
    delay(100);                 
    Serial.print("Water Level is  = ");
    valwater = (analogRead(analog)/3.5);
    Serial.print(valwater);
    Serial.print("%");
    Serial.print('\n');
    Serial.print("Water Sensor Raw = ");
    Serial.print(analogRead(analog));
    delay(2000);
    digitalWrite(watersensor, LOW);
    if (!water_level.publish(valwater)){ //Publishing monitor                    
       delay(1000);   
    }
    Serial.print('\n');
    delay(1000);


//DHT Sensor Read
    digitalWrite(dhtpin, HIGH);
    delay(1000);
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    float f = dht.readTemperature(true);
    
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
    
    float hi = dht.computeHeatIndex(f, h);
    Serial.print("Humidity: "); 
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print('\n');
    Serial.print("Temperature: "); 
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print('\n');
    Serial.print("Heat index: ");
    Serial.print(hi);
    Serial.println(" *F");
    delay(2000);
    if (! tempC.publish(t)){ //Publishing monitor                    
      delay(2000);   
    }
    digitalWrite(dhtpin, LOW);
    Serial.print('========================================');
}
