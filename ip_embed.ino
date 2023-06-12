#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <ESP8266WiFi.h>
#include "ArduinoMqttClientCustom.h"
//#include <PubSubClient.h>

#include "DHT.h"

#define DHTPIN D4
#define DHTTYPE DHT11
#define MSG_BUFFER_SIZE (50)

DHT dht(DHTPIN, DHTTYPE);


float h = 0, t = 0;
//WIFI INFO
const char* ssid = "TP-Link_3BB0";
const char* password = "Aceg141507#";

//MQTT BROKER INFO
const char* mqtt_server = "test.mosquitto.org";
const int port = 1883;
const char* topic = "careband_iot_topic";


// EKG + BPM
int valECG = 0;
long int curent = 0;
long int curentBPM = 0;
long int curent5Sec = 0;
long int curent60Sec = 0;
bool flag = false;
unsigned int sumaBPM = 0;
unsigned int iBPM = 0;
float sumaTemp = 0;
float sumaHumid = 0;
long int contor = 0;
int bpmValue;


DynamicJsonDocument doc(2048);
char serialJson[2000];

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);


unsigned long lastMsg = 0;

char msg[MSG_BUFFER_SIZE];
char msg1[MSG_BUFFER_SIZE];

int value = 0;


void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);
  } else {
    digitalWrite(BUILTIN_LED, HIGH);
  }
}

void senzorDHT() {
  float h = dht.readHumidity();     // Umiditate
  float t = dht.readTemperature();  // Temperatura
}

void senzorECG() {
  if ((digitalRead(D1) == 1) || (digitalRead(D2) == 1)) {
    //Serial.println('!');
  } else {
    // ECG
    valECG = analogRead(A0);

    // BPM
    if (analogRead(A0) > 800)  // Threshold = 800
    {
      if (!flag) {
        curent = millis();
        flag = true;
      }
      if ((millis() - curent > 200) && flag == true) {
        bpmValue = 60000 / (millis() - curent);
        //Serial.println(bpmValue);
        if (millis() - curentBPM >= 60000) {
          curentBPM = millis();
        } else {
          sumaBPM += bpmValue;
          iBPM++;
        }
        flag = false;
      }
    }
    delay(1);
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);  // Initialize the BUILTIN_LED pin as an output
  Serial.begin(9600);
  setup_wifi();

  if (!mqttClient.connect(mqtt_server, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    while (1)
      ;
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
}

void loop() {

  mqttClient.poll(); //poll broker to avoid being disconnected

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    if ((digitalRead(D1) == 1) || (digitalRead(D2) == 1)) {
      Serial.println('!');
    }
    doc.clear();  //clear doc to send again
    lastMsg = now;
    ++value;
    senzorDHT();
    //snprintf(msg, MSG_BUFFER_SIZE, "temperatura #%.2f", );
    doc["Temp"] = dht.readTemperature();
    //snprintf(msg1, MSG_BUFFER_SIZE, "umiditate #%.2f",);
    doc["Umiditate"] =  dht.readHumidity();

    if(iBPM > 0)
      doc["Bpm"] = sumaBPM / iBPM;
    else
      doc["Bpm"] = 0;

    for (int i = 0; i < 1024; i++) {
      doc["Ekg"][i] = analogRead(A0);
      senzorECG();
      delay(2);
    }

    

    serializeJson(doc, serialJson);
    Serial.println(serialJson);
    

    //sending message to mqtt
    mqttClient.beginMessage(topic);
    mqttClient.println(serialJson);
    mqttClient.endMessage();
  }
}
