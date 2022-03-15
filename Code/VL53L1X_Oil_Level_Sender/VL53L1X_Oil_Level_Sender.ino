#include <ComponentObject.h>
#include <RangeSensor.h>
#include <SparkFun_VL53L1X.h>
#include <vl53l1x_class.h>
#include <vl53l1_error_codes.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "SparkFun_VL53L1X.h"
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "arduino_secrets.h"

/*
 * You should define your wifi info and OTA Password in a secrets file. Add a tab to this sketch
 * and name it "arduino_secrets.h". Use the format:
 * #ifndef STASSID
 * #define SECRET_STASSID ""               //set SSID here
 * #define SECRET_STAPSK  ""   //set network password here
 * #endif
 * 
 * #define SECRET_OTAPASSWORD ""  //set OTA Password here
 */
const char* ssid = SECRET_STASSID;
const char* password = SECRET_STAPSK;

String OTA = SECRET_OTAPASSWORD;

//Optional interrupt and shutdown pins.
#define SHUTDOWN_PIN 0
#define INTERRUPT_PIN 2

/*
 * Set your MQTT topics here. If you leave these as I have defined them
 * everything will work with Node Red as written. If you change anything 
 * you will need to update Node Red as well. 
 */
#define dist_topic "sensors/oil/distance"   //raw distance reading
#define avg_topic "sensors/oil/avgdistance" //average distance reading
#define alive_topic "sensors/alive/oil"     //Heartbeat topic for detecting when sensors go offline

//Variables that I use throughout the sketch
int period = 300000;                //the length of time between sampling in milliseconds
int samplerate = 250;               //Time between each distance read used to generate the average
int alive = 60000;                  //Time between alive signals
int boot = 0;                       //Used to indicate boot to the main loop
unsigned long time_now = 0;         //Used for calculating if it's time for another reading
unsigned long sample_now = 0;       //Used for calculating if it's time for another sample
unsigned long alive_now = 0;        //Used for calculating if it's time to send the heartbead signal


String dist_str;     //string variable for storing distance readings
char dist[50];       //another variable for putting the readings into the right format for MQTT
int dist_array[6];  //array for storing each sample during a reading. 
int sample = 0;     //variable used for counting which sample is being taken


// this sets up the connection to the MQTT broker. So you need that setup first. 
const char* mqtt_server = "192.168.x.x";    //Enter the IP address of your broker here. You should make sure this is a static IP
const char* mqttclient = "OIL";           //Name the ESP device here, each ESP needs a distinct name.

// Initializes the espClient. You should change the espClient name if you have multiple ESPs running in your network
WiFiClient OIL;           //I make this the same as the mqttclient above
PubSubClient client(OIL); //I make this the same as the mqttclient above
String clientId = "OIL";  //I make this the same as the mqttclient above

SFEVL53L1X distanceSensor;
//Uncomment the following line to use the optional shutdown and interrupt pins.
//SFEVL53L1X distanceSensor(Wire, SHUTDOWN_PIN, INTERRUPT_PIN);

void setup() {
  Wire.begin();
  
  Serial.begin(115200);
  setup_wifi();         //runs the start wifi block, see further down

//start of OTA setup
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("OIL");  //recomend making this the same as above client names

  // No authentication by default
  ArduinoOTA.setPassword(OTA.c_str());

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
//end OTA setup    

  client.setServer(mqtt_server, 1883);  //starts the mqtt setup process
  client.setCallback(callback);
  Serial.println("setup");  //debugging line.

  if (distanceSensor.begin() != 0) //Begin returns 0 on a good init
  {
    Serial.println("Sensor failed to begin. Please check wiring. Freezing...");
    while (1)
      ;
  }
  Serial.println("Sensor online!");
  

  time_now = millis();    //sets the current time variable to the current milliseconds reading
  //sample_now = millis();  //sets the sample time variable to the current milliseconds reading
}

void loop() {

  if (!client.connected()) {    //reconnects the ESP if it becomes dissconnected
    reconnect();
  }

  if (boot == 0){   //If this is the first time the loop runs, send out an initial hello heartbeat
    client.publish(alive_topic, "hello");
    boot = 1;
  }

  if (sample > 4){     
    Serial.print("5 readings reached. Average = ");
    sample = 0;
    //disregard first reading, average the rest
    long dist_avg = (dist_array[1] + dist_array[2] + dist_array[3] + dist_array[4])/4;
    Serial.println(dist_avg);
    dist_str = String(dist_avg);
    dist_str.toCharArray(dist, dist_str.length() + 1);
    client.publish(avg_topic, dist);
    for (int i = 0; i < 7; i++){
      dist_array[i] = 0;
    }
    time_now = millis();    //reset the time now variable so the program will wait for another period of time
  }

  if (millis() > time_now + period){        //compares the current time to the period defined above. once that period is reached it runs the reading again
    if ( millis() > sample_now + samplerate){
      sample = sample + 1;
      sample_now = millis();
      measure();
    }
    
  }
  client.loop();
  ArduinoOTA.handle();

  if (millis() > alive_now + alive){    //if it's time to send a heartbeat message, send "1"
    client.publish(alive_topic, "1");
    alive_now = millis();
  }
}

//Distance measure function
void measure(){
  distanceSensor.startRanging(); //Write configuration bytes to initiate measurement
  while (!distanceSensor.checkForDataReady())
  {
    delay(1);
  }
  int distance = distanceSensor.getDistance(); //Get the result of the measurement from the sensor
  distanceSensor.clearInterrupt();
  distanceSensor.stopRanging();
  dist_array[sample] = distance;
  Serial.print("sample = ");
  Serial.println(sample);
  Serial.print("stored distance: ");
  Serial.println(dist_array[sample]);
}


//This is what connects you to the wifi network. you shouldn't have to touch this. There are serial debugging lines included
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  WiFi.mode(WIFI_STA);
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}


// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    /*
     YOU MIGHT NEED TO CHANGE THIS LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
     To change the ESP device ID, you will have to give a new name to the ESP8266.
     Here's how it looks:
       if (client.connect("ESP8266Client")) {
     You can do it like this:
       if (client.connect("ESP1_Office")) {
     Then, for the other ESP:
       if (client.connect("ESP2_Garage")) {
      That should solve your MQTT multiple connections problem
    */
    if (client.connect(clientId.c_str())) { 
      Serial.println("connected");  
      
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics if you want. 
      // For my setup the ESP doesn't really need to listen at all, but I have this setup so I can "ping" the senors periodically if needed
      client.subscribe("sensors/status");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266 is subscribed you can actually do something
// In this case it's not important to do anything since the ESP is only broadcasting data, nothing else
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // If a message is received on the topic sensor/status, respond with connected
  if(topic=="sensors/status"){
      Serial.print("call for status");
      if(messageTemp == "check"){
        client.publish(alive_topic, "hello");    
      }
  }
  Serial.println();
}
