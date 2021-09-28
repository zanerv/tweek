#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

void callback(char* topic, byte* payload, unsigned int length);

#define MQTT_SERVER "192.168.0.17"  //you MQTT IP Address
const char* ssid = "wifi";
const char* password = "wifi_pass";

const int sw_c1 = D7; // espresso
const int sw_c2 = D6; // coffee
const int sw_on = D8; // switch on/off
int led_coffee = D4; // coffee LED hight=led on, low=led off
int led_on = D3;  // status LED high=led on, low=led off
int lastHighMillis;
int intervalMillis = 1500;
int stare = '0';
int type;
char const* state = "home/tweek/status";
char const* cmd = "home/tweek/cmd";

WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wifiClient);

void setup() {
  //initialize the switch as an output and set to LOW (off)
  pinMode(sw_c1, OUTPUT); // espresso
  digitalWrite(sw_c1, LOW);

  pinMode(sw_c2, OUTPUT); // coffee
  digitalWrite(sw_c2, LOW);

  pinMode(sw_on, OUTPUT); // switch on/off
  digitalWrite(sw_on, LOW);

  pinMode(led_on, INPUT); // status LED

  pinMode(led_coffee, INPUT); // coffee status LED

  ArduinoOTA.setHostname("tweek"); // A name given to your ESP8266 module when discovering it as a port in ARDUINO IDE
  ArduinoOTA.begin(); // OTA initialization

  //start the serial line for debugging
  Serial.begin(115200);
  delay(100);

  //start wifi subsystem
  WiFi.begin(ssid, password);
  //attempt to connect to the WIFI network and then connect to the MQTT server
  reconnect();

  //wait a bit before starting the main loop
  delay(2000);
}


void loop() {

  //reconnect if connection is lost
  if (!client.connected() && WiFi.status() == 3) {
    reconnect();
  }

  //maintain MQTT connection
  client.loop();

  //MUST delay to allow ESP8266 WIFI functions to run
  delay(10);
  ArduinoOTA.handle();
  coffee();
}

void callback(char* topic, byte* payload, unsigned int length) {

  //convert topic to string to make it easier to work with
  String topicStr = topic;
  //EJ: Note:  the "topic" value gets overwritten everytime it receives confirmation (callback) message from MQTT

  //Print out some debugging info
  Serial.println("Callback update.");
  Serial.print("Topic: ");
  Serial.println(topicStr);

  if (topicStr == "home/tweek/cmd")
  {

    //turn the switch on if the payload is '1' and publish to the MQTT server a confirmation message
    if (payload[0] == '0') {
      if (stare == '0') {
      push(sw_on);
      }
      client.publish("home/tweek/status", "turn on");
    }
    else if (payload[0] == '1') {
      if (stare == '1') {
         push(sw_c1);
      } else if (stare == '0') {
         push(sw_on);
         type='1';
      }
      client.publish("home/tweek/status", "espresso");
      }
    else if (payload[0] == '2') {
      if (stare == '1') {
         push(sw_c2);
      } else if (stare == '0') {
         push(sw_on);
         type='2';
      }
      client.publish("home/tweek/status", "coffee");
    }
    else if (payload[0] == '9') {
      if (stare == '1') {
      push(sw_on);
      }
      client.publish("home/tweek/status", "turn off");
    }
  }
}

void reconnect() {

  //attempt to connect to the wifi if connection is lost
  if (WiFi.status() != WL_CONNECTED) {
    //debug printing
    Serial.print("Connecting to ");
    Serial.println(ssid);

    //loop while we wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    //print out some more debug once connected
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  //make sure we are connected to WIFI before attemping to reconnect to MQTT
  if (WiFi.status() == WL_CONNECTED) {
    // Loop until we're reconnected to the MQTT server
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");

      // Generate client name based on MAC address and last 8 bits of microsecond counter
      String clientName;
      clientName += "esp8266-";
      uint8_t mac[6];
      WiFi.macAddress(mac);
      clientName += macToStr(mac);

      //if connected, subscribe to the topic(s) we want to be notified about
      //EJ: Delete "mqtt_username", and "mqtt_password" here if you are not using any
      if (client.connect((char*) clientName.c_str(), "mqtt_username", "mqtt_password")) { //EJ: Update accordingly with your MQTT account
        Serial.print("\tMQTT Connected");
        client.subscribe(cmd);
        //EJ: Do not forget to replicate the above line if you will have more than the above number of relay switches
      }

      //otherwise print failed for debugging
      else {
        Serial.println("\tFailed.");
        abort();
      }
    }
  }
}

void push(int sw) {       // push
  digitalWrite(sw, HIGH);
  delay(500);
  digitalWrite(sw, LOW);
}

void coffee() {
  if ((digitalRead(led_on) == HIGH) && (stare == '0')) { // machine is on, POWER LED OFF

   if (digitalRead(led_coffee) == HIGH) { // machine is on, COFFEE LED OFF
     lastHighMillis = millis();
    }

    if (millis() - lastHighMillis > intervalMillis) {
    client.publish("home/tweek/status", "ready");
    stare = '1';
    delay(1000);
       if (type == '1') {
         push(sw_c1);
       } else if (type == '2') {
         push(sw_c2);
       }
    }
  
  }
  if ((digitalRead(led_on) == LOW) && (stare == '1')) { // machine was turn off
    client.publish("home/tweek/status", "standby");
    stare = '0';
    type = '0';
  }

 
  delay(500);
}

//generate unique name from MAC addr
String macToStr(const uint8_t* mac) {

  String result;
}
