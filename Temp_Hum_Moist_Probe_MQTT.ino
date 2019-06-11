//Includes
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <ESP.h>

//Defines
#define temperatureCelsius
#define DHTTYPE DHT11   // DHT 22  (AM2302), AM2321


// Const
// Device Pins
const int dhtpin = 22;
const int soilpin = 32;
const int POWER_PIN = 34;
const int LIGHT_PIN = 33;
const char* ssid = "...............";
const char* password = "..........";
const char* mqtt_server = "10.0.0.211";
// Hostname
const char* host = "esp322";


//Varibles
unsigned long now;
int DEEPSLEEP_SECONDS = 1800;
uint64_t chipid;
long timeout;
static char celsiusTemp[7];
static char humidityTemp[7];
char linebuf[90];
int charcount=0;
char deviceid[21];
char msgMQTT[80];
String tempstrT;
String tempstrH;
String tempstrLDR;
String tempstrWaterLevel;
String tempstrHIC;
String strMQTT;
long interval = 60000;
unsigned long previousMillis, currentMillis = 0;
String hostname;


WiFiClient espClient;
PubSubClient client(espClient);

// Initialize DHT sensor.
DHT dht(dhtpin, DHTTYPE);




void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
     } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}


void setup() {
  dht.begin();
  
  Serial.begin(115200);
  while(!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  pinMode(16, OUTPUT); 
  pinMode(POWER_PIN, INPUT);
  digitalWrite(16, LOW);  

  timeout = 0;

  chipid = ESP.getEfuseMac();
  sprintf(deviceid, "%" PRIu64, chipid);
  Serial.print("DeviceId: ");
  Serial.println(deviceid);

  
  Serial.println("Starting connecting WiFi.");
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  hostname = String(host);
  Serial.println (hostname);
}

void loop() {
  
  char body[1024];
  digitalWrite(16, LOW); //switched on

// Check the MQTT connection
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
 //This section read sensors
 timeout = millis();
  
 int waterlevel = analogRead(soilpin);
 int lightlevel = analogRead(LIGHT_PIN);

 // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
 float humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
 float temperature = dht.readTemperature();
  
 float hic = dht.computeHeatIndex(temperature, humidity, false);       
  
 //Serial.println (temperature);
 //Serial.println (humidity);
 //Serial.println (waterlevel);
 //Serial.println (hic);
 //Serial.println (lightlevel);  
 //Serial.println ();  

 //Convert the data to a String and format it to an MQTT JSON object then back to char
 tempstrT = String(temperature);
 tempstrH = String(humidity);
 tempstrLDR = String(lightlevel);
 tempstrWaterLevel = String(waterlevel);
 tempstrHIC = String(hic);
 
 strMQTT = "{\"HN\":\"" + hostname + "\"" + ",\"T\":" + tempstrT + ",\"H\":" + tempstrH + ",\"LDR\":" + tempstrLDR + ",\"W\":" + tempstrWaterLevel + ",\"HIC\":" + tempstrHIC + "}";
 strMQTT.toCharArray(msgMQTT,90);

 Serial.println (msgMQTT);

 // Publish the MQTT data
 client.publish("data", msgMQTT); 

 delay (interval);
}
