
#define TINY_GSM_MODEM_A6

#include <TinyGsmClient.h>
#include <PubSubClient.h>

//#define DUMP_AT_COMMANDS

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial
//
#define printLine(a) (SerialMon.println(a));
#define printString(a) (SerialMon.print(a));

//#define printLine(a) // UNCOMMENT TO DISABLE SERIAL
//#define printString(a) // UNCOMMENT TO DISABLE SERIAL

// MQTT_SOCKET_TIMEOUT: socket timeout interval in Seconds
#define MQTT_SOCKET_TIMEOUT 60

//#define SMS_TARGET  "+447885471792"
//#define SMS_TARGET2  "+447506021935"
#define SMS_TARGET  "+447803853836"
#define SMS_TARGET2 "+447967920350"

// or Software Serial on Uno, Nano
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(2, 3); // RX, TX

const char apn[]  = "TM";
const char user[] = "";
const char pass[] = "";

// MQTT details
//const char* broker = "iot.eclipse.org";
const char* broker = "test.mosquitto.org";
const char* topicDuration = "churchheater/duration";
const char* topicStatus = "churchheater/status";
const char* topicSignal = "churchheater/signal";


int csq; // signal quality
float heaterduration = 0;
float heaterdurationforSMS = 0;

boolean messageReceived = false;
int numberOfConnectAttempts = 0;

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);

PubSubClient mqtt(client);


long lastReconnectAttempt = 0;
long lastPublishAttempt = 0;

const long hour = 3600000ul; // 3600000ul milliseconds in an hour

#define INTERVAL_1 (3 * hour)      // 3.0 hours
#define INTERVAL_2 (3.5 * hour)     // 3.5 hours
#define INTERVAL_3 hour // then every hour

int messageStatus = 0;
unsigned long lastMessageSent;

  
void setup() {

  // Set console baud rate
  SerialMon.begin(115200);
  delay(10);

  // Set GSM module baud rate
  SerialAT.begin(28800);
  delay(3000);

  printString(F("RESET"));
  
  isModemConnected();

  printString(F("MQTT Configuration: "));
  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);
  printLine(F("OK"));

  printString(F("CONFIG COMPLETE"));

}


void loop() {
  
    if ( messageStatus == 0 && millis() >= INTERVAL_1 ){
      heaterduration = 3;
      publishMessage(heaterduration); 
      lastMessageSent = millis(); 
      messageStatus = 1;
    }
    if ( messageStatus == 1 && millis() >= INTERVAL_2 ){
      heaterduration = 3.5;
      publishMessage(heaterduration); 
      lastMessageSent = millis();
      messageStatus = 2;
    }
    if ( messageStatus == 2 && millis() >= INTERVAL_3 + lastMessageSent){
      heaterduration = heaterduration + 1;
      publishMessage(heaterduration);
      lastMessageSent = millis();
      if ( heaterduration > 6.5) {
        sendText(heaterduration);
      }
    }
}

void initializeModem() {
  printLine(F("Initializing modem..."));
  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  modem.restart();
  isModemConnected();
}

boolean isModemConnected() {
  printString(F("Restarting Modem: "));
  if (!modem.restart()) {
    printLine("FAIL");
    initializeModem();
    delay(10000);
    return;
  }
  printLine(F("OK"));

  printString(F("Beginning Modem: "));
  if (!modem.begin()) {
    printLine("FAIL");
    initializeModem();
    delay(10000);
    return;
  }
  printLine(F("OK"));
  
  printString(F("Test AT: "));
  if (!modem.testAT()) {
    printLine("FAIL");
    initializeModem();
    delay(10000);
    return;
  }
  printLine(F("OK"));
  
//  String modemInfo = modem.getModemInfo();
//  printString("Modem Info: "));
//  printLine(modemInfo);
//
//  String operatorInfo = modem.getOperator();
//  printString("Operator Info: "));
//  printLine(operatorInfo);

  printString(F("Checking if modem is connected:"));
  
  if (!modem.waitForNetwork()) { // if network can't be found
    while(true);
    printLine(F("Can't connect to network. Trying again"));
    initializeModem();
  }

  if (!modem.gprsConnect(apn, user, pass)) { // if network can't connect
    while(true);
    printLine(F("Can't connect to GPRS. Trying again"));
    initializeModem();
  }
  
  printLine(F(" OK"));

  updateSignalQuality();

  return true;
}

void updateSignalQuality() {
  delay(1000);
  csq = modem.getSignalQuality();
  printString(F("Signal: "));
  printString(csq);
  printLine(F("/30"));
  delay(1000);
}


bool attemptMessageSend = false;
unsigned long startTimer = millis();
unsigned long messageTimer = millis();

boolean publishMessage(float duration) {
  printLine(F("\n__________________________"));
  printString(F("___STARTING SEND "));
  printString(duration);
  printLine(F("_____"));
  printLine(F("__________________________"));
  if ( isModemConnected() ) {
    messageReceived = false;
    if (!connectToMQTT()) {
        printLine(F("Connect to MQTT failed, send a text instead"));
        sendText(duration);
        return true;
    } else {
        int publishAttempts = 0;
        while ( !messageReceived ) {
            publishAttempts++;
            printString( "Publish Attempt " + String(publishAttempts) + ": " );
            if ( sendMQTT(duration) ) { // send MQTT command
              printLine(F("OK"));
              messageReceived = true;
              closeConnection();   
              return true;
            } else {
              printLine(F("Fail"));
            }
            if ( publishAttempts >= 3 && publishAttempts < 5 ) { // after 3 or 4 failed mqtt publishes, reinitialize the modem
              printLine( "Reinitialize" );
              isModemConnected();
            } else if ( publishAttempts >= 5 ) { // after 5 failed mqtt publishes, send a test
              sendText(duration);
              messageReceived = true;
              closeConnection();   
              return true;
            }
        }
    }  
  }
}   

boolean sendMQTT(float duration) {
  char durationPayload[15];
  dtostrf(duration, 1, 1, durationPayload);
  while ( !messageReceived ) { // required know when to move on to next duration timer
    if ( publishMQTT(durationPayload) ) {
      return true;
    } else {
      return false;
    } 
  }
}

boolean publishMQTT(char durationPayload[15]) {
  char churchHeaterSignalPayload[2]; // Buffer big enough for 7-character float
  dtostrf(csq, 2, 0, churchHeaterSignalPayload); // Leave room for large numbers!
  mqtt.publish(topicSignal, churchHeaterSignalPayload);
  mqtt.publish(topicDuration, durationPayload);
  
  mqtt.loop();
  if ( subscribeMQTT() ) {
    return true; // exit true because message has been received
  } else {
    return false; // exit false because no message was received
  }
}

boolean subscribeMQTT() {
  startTimer = millis();
  while ( !messageReceived ) {
    messageTimer = millis(); // timeout
    if ( (messageTimer - startTimer) < 10000 ) { // stop looking for response after 10 seconds
      mqtt.subscribe(topicStatus);
      mqtt.loop();
    } else {
      return false; // fail because no response after 10 seconds
    }
  }
  return true;  // exit true because message has been received
}

boolean connectToMQTT() {

  int connectAttempt = 1;
  mqtt.disconnect();
  
  while (!mqtt.connected()) {
    printString(F("Connection attempt "));
    printString(connectAttempt);
    printString(F("/3 "));
    printString(F("to mqtt://"));
    printString( broker );
    printString(F(": "));

    boolean status = mqtt.connect("GsmClientTestChurchHeater");
    
    // Attempt to connect
    if ( connectAttempt >= 3) {
      return false;
    } else if ( status == false ) {
      printString(F("Failed. Response="));
      printLine(mqtt.state());
      connectAttempt++;
      delay(500);
    } else {
      printLine(F("Connected"));
      return true;
    }
  }
}





void mqttCallback(char* topic, byte* payload, unsigned int len) {
  payload[len] = '\0';
  String payloadString = String((char*)payload);

//  printString("Payload: "));
//  printLine(payloadString);
    
//   Only proceed if incoming message's topic matches
  if (payloadString == "1") {

//      SerialMon.print("Payload matches, unsub and disconnect"));

          messageReceived = true;
        
  }


  
  
}


bool sendText(float duration) {
  #if defined(SMS_TARGET)
    smsSend( SMS_TARGET ,duration);
  #endif 
  delay(5000);
  #if defined(SMS_TARGET2)
    smsSend( SMS_TARGET2 ,duration);
  #endif 

  
}

void smsSend( String phoneNumber , float duration ) {
    bool textPublished = false;
    int textAttempt = 1;
    String message = "The Church Heater has been on for " + String(duration) +" hours. The device is having trouble connecting to MQTT.";

    while ( !textPublished ) {

      printString(F("Text attempt "));
      printString( textAttempt );
      printString(F(": "));

          if ( modem.sendSMS(phoneNumber, message) ) {
            printLine(F("OK"));
            textPublished = true;
            return true;
          } else {
            printLine(F("Fail"));
            textAttempt++;
          }
          if ( textAttempt > 3 ) {
            return false;
          }
    }
}


void closeConnection() {

  printString("Unsubscribe from Topic (" + String(topicStatus) + "): ");
  if ( mqtt.unsubscribe(topicStatus) ) {
    printLine(F("OK"));
  } else {
    printLine(F("Fail"));
  }
  
  printString(F("Disconnect from MQTT: "));
  mqtt.disconnect();
  printLine(F("OK"));

  printString(F("Disconnect from GPRS: "));
  modem.gprsDisconnect();
  if (!modem.isGprsConnected()) {
    printLine(F("OK"));
  } else {
    printLine(F("Fail"));
  }

  // Try to power-off (modem may decide to restart automatically)
  // To turn off modem completely, please use Reset/Enable pins
  modem.poweroff();
  printLine(F("Poweroff."));


    messageReceived = false;
}
