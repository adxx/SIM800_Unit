/**************************************************************
 *
 * For this sketch, you need to install    library:
 *   https://github.com/knolleary/pubsubclient
 *   or from http://librarymanager/all#PubSubClient
 *
 * TinyGSM Getting Started guide:
 *   https://tiny.cc/tinygsm-readme
 *
 * For more MQTT examples, see PubSubClient library
 *
 **************************************************************
 * Use Mosquitto client tools to work with MQTT
 *   Ubuntu/Linux: sudo apt-get install mosquitto-clients
 *   Windows:      https://mosquitto.org/download/
 *
 * Subscribe for messages:
 *   mosquitto_sub -h test.mosquitto.org -t GsmClientTest/init -t GsmClientTest/ledStatus -q 1
 * Toggle led:
 *   mosquitto_pub -h test.mosquitto.org -t GsmClientTest/led -q 1 -m "toggle"
 *
 * You can use Node-RED for wiring together MQTT-enabled devices
 *   https://nodered.org/
 * Also, take a look at these additional Node-RED modules:
 *   node-red-contrib-blynk-ws
 *   node-red-dashboard
 *
 **************************************************************/

//***Low-Power section
#include "LowPower.h"
//***Interrupts section
const byte interruptPin = 2;
volatile byte state = LOW;
volatile unsigned long counter = 0;
int flag = 0;
int x = 2;
int timer = 0;
const int sleeping = 2;  //12 hours 8 3600 /8sec WD = 5400
int activity = 0;   //0 - обычная работа. 1 - сработка прерывания по контакту(тревога)


const int pwr_gsm = 5;



// Select your modem:
#define TINY_GSM_MODEM_SIM800

// Set serial for debug console (to the Serial Monitor, default speed 115200)
//#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial

// or Software Serial on Uno, Nano
#include <SoftwareSerial.h>
SoftwareSerial SerialMon(4, 3); // RX, TX

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

// Range to attempt to autobaud
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200

// Add a reception delay - may be needed for a fast processor at a slow baud rate
// #define TINY_GSM_YIELD() { delay(2); }

// Define how you're planning to connect to the internet
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[] = "internet";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Your WiFi connection credentials, if applicable
const char wifiSSID[] = "YourSSID";
const char wifiPass[] = "YourWiFiPass";

// MQTT details
const char* broker = "srv1.clusterfly.ru";
const unsigned int port = 9124;

const char* topicLed = "user_06359334/test";
const char* topicInit = "user_06359334/test";
const char* topicLedStatus = "user_06359334/test";

#include <TinyGsmClient.h>
#include <PubSubClient.h>

// Just in case someone defined the wrong thing..
#if TINY_GSM_USE_GPRS && not defined TINY_GSM_MODEM_HAS_GPRS
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true
#endif
#if TINY_GSM_USE_WIFI && not defined TINY_GSM_MODEM_HAS_WIFI
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#endif

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif
TinyGsmClient client(modem);
PubSubClient mqtt(client);

#define LED_PIN 13
int ledStatus = LOW;

uint32_t lastReconnectAttempt = 0;
/*
void mqttCallback(char* topic, byte* payload, unsigned int len) {
  SerialMon.print("Message arrived [");
  SerialMon.print(topic);
  SerialMon.print("]: ");
  SerialMon.write(payload, len);
  SerialMon.println();

  // Only proceed if incoming message's topic matches
  if (String(topic) == topicLed) {
    ledStatus = !ledStatus;
    digitalWrite(LED_PIN, ledStatus);
    mqtt.publish(topicLedStatus, ledStatus ? "1" : "0");
  }
}
*/
boolean mqttConnect(int msg) {

  // Connect to MQTT Broker
  //boolean status = mqtt.connect("GsmClientTest");
  boolean status = mqtt.connect("GsmClientName", "user_06359334", "pass_4f65d776");

  if (status == false) {
    return false;
  }
  if (msg==0){
    mqtt.publish(topicInit, "regular wake-up");
  }
  if (msg == 1){
    mqtt.publish(topicInit, "alarm report");
  }
  mqtt.publish(topicInit, "GsmClientTest started");
  //mqtt.subscribe(topicLed);
  return mqtt.connected();
}

void wakeUp() {  
  //detachInterrupt(digitalPinToInterrupt(interruptPin));
  activity = 1;
  noInterrupts();
}


void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  pinMode(pwr_gsm, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  delay(300);
  digitalWrite(LED_PIN, LOW);
  // Set console baud rate
  //SerialMon.begin(9600);
  delay(100);


  digitalWrite(pwr_gsm, HIGH);

  digitalWrite(LED_PIN, HIGH);
  delay(300);
  digitalWrite(LED_PIN, LOW);
  // !!!!!!!!!!!
  // Set your reset, enable, power pins here
  // !!!!!!!!!!!

  // Set GSM module baud rate
  // TinyGsmAutoBaud(SerialAT,GSM_AUTOBAUD_MIN,GSM_AUTOBAUD_MAX);
  SerialAT.begin(9600);
  delay(6000);

  // Restart takes quite some time
  // To skip it, call k ninit() instead of restart()
  modem.restart();
  // modem.init();

  digitalWrite(LED_PIN, HIGH);
  delay(300);
  digitalWrite(LED_PIN, LOW);
  
  String modemInfo = modem.getModemInfo();

#if TINY_GSM_USE_GPRS
  // Unlock your SIM card with a PIN if needed
  if ( GSM_PIN && modem.getSimStatus() != 3 ) {
    modem.simUnlock(GSM_PIN);
  }
#endif
/*
#if TINY_GSM_USE_GPRS && defined TINY_GSM_MODEM_XBEE
  // The XBee must run the gprsConnect function BEFORE waiting for network!
  modem.gprsConnect(apn, gprsUser, gprsPass);
#endif
*/
  
  if (!modem.waitForNetwork()) {
    delay(10000);
    return;
  }


#if TINY_GSM_USE_GPRS
  // GPRS connection parameters are usually set after network registration
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      delay(10000);
      return;
    }

  if (modem.isGprsConnected()) {
  }
#endif

  // MQTT Broker setup
  mqtt.setServer(broker, port);
  //mqtt.setCallback(mqttCallback);
}

void loop() {
  if (activity != 0) { //кейс для сработавшей тревоги.
    for (int i = 0 ; i <= x; i ++) { 
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(300);                       // wait for a second
      digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW  
      delay(300);                       // wait for a second  
    }
    digitalWrite(pwr_gsm, HIGH);
    mqttConnect(1);
    digitalWrite(pwr_gsm, LOW);
    
    activity = 0;
    interrupts();
    attachInterrupt(digitalPinToInterrupt(interruptPin), wakeUp, FALLING);
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  
  }
  else { //кейс для обычной работы, когда просыпается по WDT.
    if (timer >= sleeping){
      timer = 0;
      for (int i = 0 ; i <= x; i ++) { 
        digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(300);                       // wait for a second
        digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW  
        delay(300);                       // wait for a second
      }
      digitalWrite(pwr_gsm, HIGH);
      mqttConnect(0);
      digitalWrite(pwr_gsm, LOW);
    }
    else{   //по таймеру еще не прошло нужное кол-во часов, так что ухоидм спать дальше. 
      timer ++;
      // Enter power down state for 8 s with ADC and BOD module disabled
      interrupts();
      attachInterrupt(digitalPinToInterrupt(interruptPin), wakeUp, FALLING);
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  
      detachInterrupt(digitalPinToInterrupt(interruptPin));
    }
  }
  

  /*
  if (!mqtt.connected()) {
    //SerialMon.println("=== MQTT NOT CONNECTED ===");
    // Reconnect every 10 seconds
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
    delay(100);
    return;
  }
  */

  //mqtt.loop();
  
}
