/*
 Name:		LO_Example_Tiny_GSM.ino
 Created:	13/03/2018 14:39:03
 Author:	JWSC8367
*/

//
//    FILE: TinyGSMMQTTClient.ino
//  AUTHOR: Allard saint albin Thierry
// VERSION: 0.1.00
// PURPOSE: This sketch purpose is to use a MQTT Client for Live object with a GSM modem
//  It used tiny GSM Library, PubSub client library and ArduinoJson library
//     URL:
//		ArduinoJson : https://github.com/bblanchon/ArduinoJson
//		PubSubClient : https://github.com/knolleary/pubsubclient
//		TinyGSM : https://github.com/vshymanskyy/TinyGSM
//
// 
//

/**************************************************************

For this example, you need to install PubSubClient library:
https://github.com/knolleary/pubsubclient/releases/latest
or from http://librarymanager/all#PubSubClient

TinyGSM Getting Started guide:
http://tiny.cc/tiny-gsm-readme

**************************************************************
You can use Node-RED for wiring together MQTT-enabled devices
https://nodered.org/
Also, take a look at these additional Node-RED modules:
node-red-contrib-blynk-websockets
node-red-dashboard

**************************************************************/

// Select your modem:
#define TINY_GSM_MODEM_SIM800
//#define TINY_GSM_MODEM_SIM900
//#define TINY_GSM_MODEM_A6
//#define TINY_GSM_MODEM_M590
#define TINY_GSM_DEBUG Serial
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Topic.h"

#define _rstpin 4

// Increase RX buffer
#define TINY_GSM_RX_BUFFER 128

#include "LO_config.h"
#include "CMD.h"

//IF YOU  USE HARDWARE SERIAL ON MEGA, LEONARDO, MICRO
#define SerialAT Serial1

// IF YOU USE SOFTWARE SERIAL ON UNO, NANO
//#include <SoftwareSerial.h>
//SoftwareSerial SerialAT(2, 3); // RX, TX


TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);


//Timestamp of last reconnection
long lastReconnectAttempt = 0;
//Timestamp for last execution
long lastExecution = 0;

//Configuration ID
String CID = "";

//Command ID
String  C_CID = "";
//Ressources ID
String  R_CID = "";
//BOOLEAN to detect First connection
int FIRST_CONNECTION = 1;

//Static jsonBuffer to send data in JSON Format.
StaticJsonBuffer<256> jsonBuffer;
bool LED_BUILTIN_STATUS = LOW;

void Error()
{
	LED_BUILTIN_STATUS = !LED_BUILTIN_STATUS;
	digitalWrite(13, LED_BUILTIN_STATUS);
}

int lightValue;
int temperatureValue;
int humidityValue;


//Function to calculate Temperature and Humidity
void CalculateTemperatureAndHumidity()
{
	temperatureValue = random(100);
	humidityValue = random(100);
}

//Function to calculateLuminosity
void CalculateLuminosity()
{
	lightValue = random(100);
}

//Function to initSensor
void initSensor()
{
	randomSeed(analogRead(0));
}
/**
*  Function to setup the board
*/
void setup() {

  pinMode(_rstpin, OUTPUT);
  digitalWrite(_rstpin, HIGH);
  delay(10);
  digitalWrite(_rstpin, LOW);
  delay(100);
  digitalWrite(_rstpin, HIGH);
  
	Serial.begin(115200);
	Serial.println("***SETUP***");
	// Set console baud rate
	delay(10);
	// Set GSM module baud rate
	SerialAT.begin(115200);
	delay(3000);
	// Restart takes quite some time
	// To skip it, call init() instead of restart()
	Serial.println("Initializing modem...");
	modem.restart();
	
	// Unlock your SIM card with a PIN
	//modem.simUnlock("1234");

	Serial.print("Waiting for network...");
	if (!modem.waitForNetwork()) {
		Serial.println(" fail");
		while (true)
		{
			Error();
		}
	}
	Serial.println(" OK");
	Serial.print("Connecting to ");
	Serial.print(apn);
	if (!modem.gprsConnect(apn, user, pass)) {
		Serial.println(" fail");
		while (true)
		{
			Error();
		}
	}
	Serial.println(" OK");

	// MQTT Broker setup
	mqtt.setServer(broker, PORT);
	mqtt.setCallback(mqttCallback);
	Serial.println("***END SETUP***");
	initSensor();
}

/***
* Function to connect to the LO plateform as device
*/
boolean mqttConnect() {
	Serial.print("Connecting to ");
	Serial.print(broker);

	if (!mqtt.connect(deviceid, LO_USER, LO_MDP)) {
		Serial.println(" fail");
		return false;
	}
	Serial.println(" OK");
	int connected = mqtt.connected();
	if (connected)
	{
		Serial.println("Connected");
		subscribeToTopics();
	}
	else
	{
		Serial.println("Not connected");
	}
	return connected;
}
/***
*  Function to subscribeToTopics
*/
void subscribeToTopics()
{
	// dev/cfg/upd
	mqtt.subscribe(receiveCfgTopic);

	// dev/cmd
	mqtt.subscribe(cmdTopic);

	// dev/rsc/upd
	mqtt.subscribe(updtRscTopic);
}



/**
* Main function of the prototype
* Every 20 000 milli seconds, it sends data to the plateform
* The function connects the device to the plateform when the connection is lost every 50 seconds
*
*/
void loop() {

	unsigned long t = 0;
	unsigned long e = 0;
	if (mqtt.connected()) {
		mqtt.loop();
		e = millis();
		if (e - lastExecution > 20000L || lastExecution == 0)
		{
			sendConfig();
			sendStatus();
			CalculateTemperatureAndHumidity();
			CalculateLuminosity();
			sendData();
			lastExecution = e;
		}
	}
	else {
		// Reconnect every 10 seconds

		t = millis();
		if (FIRST_CONNECTION == 1 || t - lastReconnectAttempt > 50000L) {
			lastReconnectAttempt = t;
			if (mqttConnect()) {
				lastReconnectAttempt = 0;
				lastExecution == 0;
				FIRST_CONNECTION = 0;
			}
		}
	}
}


/**
* Function use to publish a Json object root to the topicArg
*/
bool publish(const char * topicArg, JsonObject& root, bool alertUser, void(*callback)(void))
{
	boolean result = true;
	if (root.success())
	{
		char buffer[250];
		root.printTo(buffer);
		result = mqtt.publish(topicArg, buffer);
		if (result)
		{
			Serial.println();
			Serial.print("publish : ");
			Serial.print(topicArg);
			Serial.println(buffer);
			if (alertUser)
				callback();

		}
		else
		{
			Serial.println();
			Serial.print("publish : Error during publishing");
		}
	}
	else
	{
		Serial.println("Wrong jsonObject");
		result = false;
	}
	jsonBuffer.clear();
	return result;
}

/**
* Function use to send the device configuration to the plateform to the topic dev/cfg
*/
void sendConfig()
{
	JsonObject& root = jsonBuffer.createObject();
	JsonObject & cfg = root.createNestedObject("cfg");
	JsonObject & log_level = cfg.createNestedObject("log_level");
	log_level["t"] = "str";
	log_level["v"] = "DEBUG";
	JsonObject & secret_key = cfg.createNestedObject("secret_key");
	secret_key["t"] = "bin";
	secret_key["v"] = "Nzg3ODY4Ng==";
	JsonObject & conn_freq = cfg.createNestedObject("conn_freq");
	conn_freq["t"] = "i32";
	conn_freq["v"] = 8000;
	if (CID != "")
		root["cid"] = CID;
	publish(postCurrentCfgTopic, root, false, NULL);
}


/***
* Function use to send ressources of the device to the topic dev/rsc
*
*/
// dev/rsc
void sendResources()
{
	JsonObject& root = jsonBuffer.createObject();
	JsonObject & rsc = root.createNestedObject("rsc");
	JsonObject & firmware = rsc.createNestedObject("firmware");
	firmware["v"] = "1_2";
	JsonObject & m = firmware.createNestedObject("m");
	m["username"] = "heracles";
	if (R_CID != "")
	{
		root["cid"] = R_CID;
		publish(updtResponseRscTopic, root, false, NULL);
	}
	else
	{
		publish(sendRscTopic, root, false, NULL);
	}
}

/***
* Function use to send a response of a command of the device to the topic dev/cmd/res
*/
void sendReponseCommand()
{
	JsonObject& root = jsonBuffer.createObject();
	JsonObject & res = root.createNestedObject("res");
	res["done"] = "true";
	if (C_CID != "")
	{
		root["cid"] = C_CID;
	}
	publish(cmdResTopic, root, false, NULL);
}



/***
* Function use to send the status of the device to the topic dev/info
*/
void sendStatus()
{
	JsonObject& root = jsonBuffer.createObject();
	JsonObject & info = root.createNestedObject("info");
	info["ledStatus"] = LED_BUILTIN_STATUS ? "On" : "OFF";
	info["Status"] = mqtt.connected() ? "Connected" : "Not connected";
	publish(statusTopic, root, false, NULL);
}



/**
* Function use to send data of sensors to the LO plateform by using the topic dev/data
*/
void sendData()
{
	JsonObject& root = jsonBuffer.createObject();
	String s = String(deviceid);
	s = String("!measuresTest");
	String m = "measuresTest_v1_3";
	root["s"] = s;
	root["m"] = m;
	JsonObject& v = root.createNestedObject("v");
	v["LedStatus"] = LED_BUILTIN_STATUS ? "1" : "0";
	v["Luminosity"] = lightValue;
	v["Temperature"] = temperatureValue;
	v["Humidity"] = humidityValue;
	v["Battery_Level"] = 100;
	v["Battery_Charging"] = 1;
	JsonArray& t = root.createNestedArray("t");
	//root["ts"] = "2017-09-19T10:00:00Z";
	t.add("Heracles");
	t.add("prototype");
	publish(posDataTopic, root, true, AlertSending);
}

void AlertSending(void)
{
	Serial.println("Sending data");
}

void AlertReceive(void)
{
	Serial.println("CMD receive");
}



/**
* Function call everytime the user receive a mqtt message
*/
void mqttCallback(char* topic, uint8_t * payload, unsigned int len) {


	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("]: ");
	Serial.write(payload, len);
	Serial.println();
	char * buffer = new char[len];
	buffer = (char *)payload;
	StaticJsonBuffer<200> jsonBuffer;
	// Only proceed if incoming message's topic matches


	if (String(topic) == receiveCfgTopic) {            // When receiving message from topic "dev/cfg/upd"     
		Serial.println("Config");
		JsonObject& root = jsonBuffer.parseObject(buffer);
		CID = String(root["cid"].as<String>());
		sendConfig();
	}

	if (String(topic) == cmdTopic)                   // When receiving message from topic "dev/cmd"
	{
		Serial.println("cmd");
		JsonObject& root = jsonBuffer.parseObject(buffer);
		String tmp_C_CID = root["cid"].as<String>();
		if (String(C_CID) != tmp_C_CID)
		{
			AlertReceive();
			C_CID = tmp_C_CID;
			String req = root["req"].as<String>();
			if (req == SWITCH_OFF)
			{
				JsonObject& arg = root["arg"].asObject();;
				int light = arg.get<int>("Light");
				if (light == 0)
					LED_BUILTIN_STATUS = false;
				else
					LED_BUILTIN_STATUS = true;
				digitalWrite(13, LED_BUILTIN_STATUS);
			}
			sendReponseCommand();
		}
		else
		{
			Serial.println("CCID don't change");
		}
	}
	if (String(topic) == updtRscTopic)        // When receiving message from topic dev/rsc/upd
	{
		Serial.println("update resource");
		JsonObject& root = jsonBuffer.parseObject(buffer);

		String tmp_R_CID = root["cid"].as<String>();
		R_CID = tmp_R_CID;
		sendResources();
	}
}
