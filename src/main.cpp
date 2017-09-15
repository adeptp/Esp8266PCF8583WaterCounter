#include <dummy.h>
#include "Wire.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <OneWire.h> //http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h> //https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <SoftwareSerial.h>
#include <PCF8583.h> //GitHub url lost
#include "RemoteDebug.h"        //https://github.com/JoaoLopesF/RemoteDebug
#include "privatefile.h" // Remove this line by commenting on it

#define LOCATION_OFFSET_COUNTER_OVERFLOW 0x51 //2 BYTE
#define LOCATION_OFFSET_INITIAL_INDICATION 0x53 //4 BYTE
#define LOCATION_OFFSET_DIVISION_RATIO 0x57 //4 BYTE
#define LOCATION_OFFSET_PREVIOUS_VALUE 0X61 //4 BYTE
#define LED_PIN 1


uint32_t divisionRatio1 = 0;
uint32_t divisionRatio2 = 0;

// Expose Espressif SDK functionality - wrapped in ifdef so that it still
// compiles on other platforms
#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif

#define MYHOSTNAME "HozblokWC1"

const char* ssid = SSID;//Put  yuor ssid here
const char* password = SSIDPASSWORD;//Put  yuor ssid password here
const char* host = MYHOSTNAME;

OneWire  ds(4);  // on pin 4 (a 4.7K resistor is necessary)

DallasTemperature sensors(&ds);// Pass our oneWire reference to Dallas Temperature. 
SoftwareSerial swSer(14, 12, false, 100);

PCF8583 counter1(0xA0);
PCF8583 counter2(0xA2);

DeviceAddress t0, t1, t2;
//String tS0, tS1, tS2;

float tempT0, tempT1, tempT2;
float cVal1, cVal2;
uint32_t co1, co2, ii1, ii2, cv1, cv2;


uint32_t count_dr1 = 100, count_dr2 = 100;
uint8_t showSwSerial = 0;


volatile uint16_t humidity = -1, temperature1 = -1, temperature2 = -1, temperature3 = -1;
volatile uint8_t Dimmer1 = -1, Dimmer2 = -1;
volatile uint8_t modeFan1 = -1, modeFan2 = -1;
volatile uint8_t Door1 = -1, Door2 = -1, Lamp1 = -1, Lamp2 = -1;

long upTimeDay = 0;
int upTimeHour = 0;
int upTimeMinute = 0;
int upTimeSecond = 0;
int upTimeMilli = 0;
int upTimeHighMillis = 0;
int upTimeRollover = 0;


void callback(char* topic, byte* payload, unsigned int length);

const char* mqtt_server = "192.168.1.39";
WiFiClient espClient;
PubSubClient client(mqtt_server, 1883, callback, espClient);

const byte msg_size = 100;
char msg[msg_size];
char topic[msg_size];
String inputString = "";


unsigned long previousMillis = 0;
unsigned long previousMillisSend = 0;
unsigned long previousMillisLamp = 0;

RemoteDebug Debug;

///mqtt callback
void callback(char* topic, byte* payload, unsigned int length) {
	uint32_t val;

	//Serial.println("");
	//Serial.println("-------");
	//Serial.println("New callback of MQTT-broker");
	//Serial.setDebugOutput(true);

	//преобразуем тему(topic) и значение (payload) в строку
	payload[length] = '\0';
	String strTopic = String(topic);

	if (!strTopic.startsWith(MYHOSTNAME)) return;

	String strPayload = String((char*)payload);
	if (Debug.isActive(Debug.INFO)) {
		Debug.printf("strTopic %s ~ strPayload %s \n", topic, payload);
	}

}

void upTimeCalc(){
    
        long secsUp = millis() / 1000;
    
        upTimeMilli = millis() % 1000;
        upTimeSecond = secsUp % 60;
        upTimeMinute = (secsUp / 60) % 60;
        upTimeHour = (secsUp / (60 * 60)) % 24;
        upTimeDay = (upTimeRollover * 50) + (secsUp / (60 * 60 * 24));  //First portion takes care of a rollover [around 50 days]
    }
    

void SendParam()
{
	//Serial.println("SendParam");

	snprintf(topic, msg_size, "%s/freemem", host);
	snprintf(msg, msg_size, "%ld", ESP.getFreeHeap());
	client.publish(topic, msg);
	//Serial.printf("client send: topic:%s msg:%s\n", topic, msg);

	snprintf(topic, msg_size, "%s/rssi", host);
	snprintf(msg, msg_size, "%ld", WiFi.RSSI());
	client.publish(topic, msg);
	//Serial.printf("client send: topic:%s msg:%s\n", topic, msg);

	snprintf(topic, msg_size, "%s/uptime", host);
	snprintf(msg, msg_size, "%ld", millis());
	client.publish(topic, msg);
	//Serial.printf("client send: topic:%s msg:%s\n", topic, msg);

	upTimeCalc();
	snprintf(topic, msg_size, "%s/uptimeString", host);
	snprintf(msg, msg_size, "%d days %02d:%02d:%02d.%02d", upTimeDay, upTimeHour, upTimeMinute, upTimeSecond, upTimeMilli);
	client.publish(topic, msg);

	snprintf(topic, msg_size, "%s/t0", host);
	snprintf(msg, msg_size, "%d.%02d", int(tempT0), int((tempT0 - int(tempT0)) * 100));
	client.publish(topic, msg);
	//Serial.printf("client send: topic:%s msg:%s\n", topic, msg);

	snprintf(topic, msg_size, "%s/t1", host);
	snprintf(msg, msg_size, "%d.%02d", int(tempT1), int((tempT1 - int(tempT1)) * 100));
	client.publish(topic, msg);
	//Serial.printf("client send: topic:%s msg:%s\n", topic, msg);

	snprintf(topic, msg_size, "%s/t2", host);
	snprintf(msg, msg_size, "%d.%02d", int(tempT2), int((tempT2 - int(tempT2)) * 100));
	client.publish(topic, msg);
	//Serial.printf("client send: topic:%s msg:%s\n", topic, msg);


	snprintf(topic, msg_size, "%s/c1_tablo", host);
	dtostrf(cVal1, 5, 4, msg);
	client.publish(topic, msg);
	//Serial.printf("client send: topic:%s msg:%s\n", topic, msg);

	snprintf(topic, msg_size, "%s/c2_tablo", host);
	dtostrf(cVal2, 5, 4, msg);
	client.publish(topic, msg);
	//Serial.printf("client send: topic:%s msg:%s\n", topic, msg);


	snprintf(topic, msg_size, "%s/c1_Overflow", host);
	snprintf(msg, msg_size, "%ld", co1);
	client.publish(topic, msg);
	//Serial.printf("client send: topic:%s msg:%s\n", topic, msg);

	snprintf(topic, msg_size, "%s/c2_Overflow", host);
	snprintf(msg, msg_size, "%ld", co2);
	client.publish(topic, msg);
	//Serial.printf("client send: topic:%s msg:%s\n", topic, msg);

	snprintf(topic, msg_size, "%s/c1_Initial_indicator", host);
	snprintf(msg, msg_size, "%ld", ii1);
	client.publish(topic, msg);
	//Serial.printf("client send: topic:%s msg:%s\n", topic, msg);

	snprintf(topic, msg_size, "%s/c2_Initial_indicator", host);
	snprintf(msg, msg_size, "%ld", ii2);
	client.publish(topic, msg);
	//Serial.printf("client send: topic:%s msg:%s\n", topic, msg);

	snprintf(topic, msg_size, "%s/c1_Current_val", host);
	snprintf(msg, msg_size, "%ld", cv1);
	client.publish(topic, msg);
	//Serial.printf("client send: topic:%s msg:%s\n", topic, msg);

	snprintf(topic, msg_size, "%s/c2_Current_val", host);
	snprintf(msg, msg_size, "%ld", cv2);
	client.publish(topic, msg);
	//Serial.printf("client send: topic:%s msg:%s\n", topic, msg);

	snprintf(topic, msg_size, "%s/c1_Division_Ratio", host);
	snprintf(msg, msg_size, "%ld", count_dr1);
	client.publish(topic, msg);
	//Serial.printf("client send: topic:%s msg:%s\n", topic, msg);

	snprintf(topic, msg_size, "%s/c2_Division_Ratio", host);
	snprintf(msg, msg_size, "%ld", count_dr2);
	client.publish(topic, msg);
	//Serial.printf("client send: topic:%s msg:%s\n", topic, msg);

}


void doWorkCounters()
{
	cv1 = counter1.getCount();
	uint32_t pv1 = counter1.getMemoryLong(LOCATION_OFFSET_PREVIOUS_VALUE);

	co1 = counter1.getMemoryInt(LOCATION_OFFSET_COUNTER_OVERFLOW);
	if (co1 == 0) {
		co1 = 1;
		counter1.setMemoryInt(LOCATION_OFFSET_COUNTER_OVERFLOW, co1);
	}
	ii1 = counter1.getMemoryLong(LOCATION_OFFSET_INITIAL_INDICATION);

	if (cv1 < pv1){
		co1++;
		counter1.setMemoryInt(LOCATION_OFFSET_COUNTER_OVERFLOW, co1);
	}

	counter1.setMemoryLong(LOCATION_OFFSET_PREVIOUS_VALUE, cv1);

	cVal1 = (cv1*co1);
	cVal1 = cVal1 / count_dr1 + ii1;

	//	dtostrf(cVal1, 5, 4, msg);
	//	Serial.printf("counter1: cv1 %d, pv1: %d, co1: %d, ii1: %d, count_dr1: %d, cVal1:%s\n", cv1, pv1, co1, ii1, count_dr1, msg);

	cv2 = counter2.getCount();
	uint32_t pv2 = counter2.getMemoryLong(LOCATION_OFFSET_PREVIOUS_VALUE);

	co2 = counter2.getMemoryInt(LOCATION_OFFSET_COUNTER_OVERFLOW);
	if (co2 == 0) {
		co2 = 1;
		counter2.setMemoryInt(LOCATION_OFFSET_COUNTER_OVERFLOW, co2);
	}
	ii2 = counter2.getMemoryLong(LOCATION_OFFSET_INITIAL_INDICATION);

	if (cv2 < pv2){
		co2++;
		counter2.setMemoryInt(LOCATION_OFFSET_COUNTER_OVERFLOW, co2);
	}
	counter2.setMemoryLong(LOCATION_OFFSET_PREVIOUS_VALUE, cv2);

	cVal2 = (cv2*co2);
	cVal2 = cVal2 / count_dr2 + ii2;

	//dtostrf(cVal2, 5, 4, msg);
	//Serial.printf("counter2: cv2 %d, pv2: %d, co2: %d, ii2: %d, count_dr2: %d cVal2:%s\n", cv2, pv2, co2, ii2, count_dr2, msg);
}


void SwSerialHandler(){
	while (swSer.available()) {
		// get the new byte:
		char inChar = (char)swSer.read();
		uint16_t index = 0;
		uint16_t numVal = 0;
		uint16_t parVal = 0;
		// if the incoming character is a newline, set a flag
		// so the main loop can do something about it:
		if ((inChar == (char)10)) {
			//stringComplete = true;
			if (Debug.isActive(Debug.DEBUG)){
				if (showSwSerial == 1){
					Debug.print("SwSerial: ");
					Debug.println(inputString);
				}
			}
			if ((inputString.length() > 0))	{
				switch (inputString[0]){
				case '$':
					inputString.remove(0, 1);
					index = inputString.indexOf(";");
					while (index > 0){
						parVal = (inputString.substring(0, index)).toInt();

						inputString.remove(0, index + 1);
						switch (numVal){
						case 0://humidity
							if (humidity != parVal){
								humidity = parVal;

								snprintf(topic, msg_size, "%s/humidity", host);
								snprintf(msg, msg_size, "%d.%d", parVal / 10, parVal % 10);
								client.publish(topic, msg);
							}
							break;
						case 1://temperature1
							if (temperature1 != parVal){
								temperature1 = parVal;

								snprintf(topic, msg_size, "%s/temperature1", host);
								snprintf(msg, msg_size, "%d.%d", parVal / 10, parVal % 10);
								client.publish(topic, msg);
							}

							break;
						case 2://temperature2
							if (temperature2 != parVal){
								temperature2 = parVal;

								snprintf(topic, msg_size, "%s/temperature2", host);
								snprintf(msg, msg_size, "%d.%d", parVal / 10, parVal % 10);
								client.publish(topic, msg);
							}
							break;
						case 3://дверь 1
							if (Door1 != parVal){
								Door1 = parVal;

								snprintf(topic, msg_size, "%s/Door1", host);
								snprintf(msg, msg_size, "%s", parVal == 0 ? "false" : "true");
								client.publish(topic, msg);
							}

							break;
						case 4://дверь 2
							if (Door2 != parVal){
								Door2 = parVal;

								snprintf(topic, msg_size, "%s/Door2", host);
								snprintf(msg, msg_size, "%s", parVal == 0 ? "false" : "true");
								client.publish(topic, msg);
							}
							break;
						case 5://лампа 1
							if (Lamp1 != parVal){
								Lamp1 = parVal;

								snprintf(topic, msg_size, "%s/Lamp1", host);
								snprintf(msg, msg_size, "%s", parVal == 0 ? "false" : "true");
								client.publish(topic, msg);
							}

							break;
						case 6://лампа 2
							if (Lamp2 != parVal){
								Lamp2 = parVal;

								snprintf(topic, msg_size, "%s/Lamp2", host);
								snprintf(msg, msg_size, "%s", parVal == 0 ? "false" : "true");
								client.publish(topic, msg);
							}
							break;
						case 7://Dimmer1
							if (Dimmer1 != parVal){
								Dimmer1 = parVal;

								snprintf(topic, msg_size, "%s/Dimmer1", host);
								snprintf(msg, msg_size, "%d", parVal);
								client.publish(topic, msg);
							}

							break;
						case 8://Dimmer2
							if (Dimmer2 != parVal){
								Dimmer2 = parVal;

								snprintf(topic, msg_size, "%s/Dimmer2", host);
								snprintf(msg, msg_size, "%d", parVal);
								client.publish(topic, msg);
							}
							break;
						case 9://modeFan1
							if (modeFan1 != parVal){
								modeFan1 = parVal;

								snprintf(topic, msg_size, "%s/modeFan1", host);
								snprintf(msg, msg_size, "%d", parVal);
								client.publish(topic, msg);
							}

							break;
						case 10://modeFan2
							if (modeFan2 != parVal){
								modeFan2 = parVal;

								snprintf(topic, msg_size, "%s/modeFan2", host);
								snprintf(msg, msg_size, "%d", parVal);
								client.publish(topic, msg);
							}break;
						case 11://resrved
							break;
						}

						numVal++;
						if (inputString.length() < 2) break;
						index = inputString.indexOf(";");
					}
					break;

				case '#':
					inputString.remove(0, 1);
					snprintf(topic, msg_size, "%s/DebugHB", host);
					snprintf(msg, msg_size, "%s", inputString.c_str());
					client.publish(topic, msg);
					break;

				case '%':
					inputString.remove(0, 1);
					break;
				}
			}
			inputString = "";
			//swSer.flush();
			//return;
		}
		else {
			// add it to the inputString:
			inputString += inChar;
		}
	}

}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
	for (uint8_t i = 0; i < 8; i++)
	{
		// zero pad the address if necessary
		if (deviceAddress[i] < 16) Debug.print("0");
		Debug.print(deviceAddress[i], HEX);
	}
}


String getStrAddress(DeviceAddress deviceAddress)
{
	char buffer[17];
	buffer[16] = 0;
	for (uint8_t j = 0; j < 8; j++)
		sprintf(&buffer[2 * j], "%02X", deviceAddress[j]);

	return String(buffer);
}


void showInfo(){
	// locate devices on the bus
	Debug.isActive(Debug.INFO);
	Debug.print("Found ");
	Debug.print(sensors.getDeviceCount(), DEC);
	Debug.println(" devices.");

	if (!sensors.getAddress(t0, 0)) Debug.println("Unable to find address for Device 0");
	if (!sensors.getAddress(t1, 1)) Debug.println("Unable to find address for Device 1");
	if (!sensors.getAddress(t2, 2)) Debug.println("Unable to find address for Device 2");

	Debug.print("Device 0 Address: ");
	printAddress(t0);
	Debug.println();
	Debug.print("Device 1 Address: ");
	printAddress(t1);
	Debug.println();
	Debug.print("Device 2 Address: ");
	printAddress(t2);
	Debug.println();

	Debug.print("Device 0 Resolution: ");
	Debug.print(sensors.getResolution(t0), DEC);
	Debug.println();


	Debug.print("Device 1 Resolution: ");
	Debug.print(sensors.getResolution(t1), DEC);
	Debug.println();

	Debug.print("Device 2 Resolution: ");
	Debug.print(sensors.getResolution(t2), DEC);
	Debug.println();

	// report parasite power requirements
	Debug.print("Parasite power is: ");
	if (sensors.isParasitePowerMode()) Debug.println("ON");
	else Debug.println("OFF");

	//DeviceAddress deviceAddress = {7, 8, 9, 10, 11, 12, 13, 14};
	//Debug.printf("test  getStrAddress %s ", getStrAddress(deviceAddress).c_str());

	Debug.print("IP address: ");
	Debug.println(WiFi.localIP());


	uint32_t realSize = ESP.getFlashChipRealSize();
	uint32_t ideSize = ESP.getFlashChipSize();
	FlashMode_t ideMode = ESP.getFlashChipMode();

	Debug.printf("Flash real id:   %08X\n", ESP.getFlashChipId());
	Debug.printf("Flash real size: %u\n\n", realSize);

	Debug.printf("Flash ide  size: %u\n", ideSize);
	Debug.printf("Flash ide speed: %u\n", ESP.getFlashChipSpeed());
	Debug.printf("Flash ide mode:  %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));



	if (ideSize != realSize) {
		Debug.println("Flash Chip configuration wrong!\n");
	}
	else {
		Debug.println("Flash Chip configuration ok.\n");
	}

	upTimeCalc();
	Debug.printf("Uptime %d days %02d:%02d:%02d.%02d\n", upTimeDay, upTimeHour, upTimeMinute, upTimeSecond, upTimeMilli);
}


void showValue(){
	if (Debug.isActive(Debug.INFO)){

		upTimeCalc();
		Debug.printf("Uptime %d days %02d:%02d:%02d.%02d\n", upTimeDay, upTimeHour, upTimeMinute, upTimeSecond, upTimeMilli);


		Debug.println("Counter 1 value:");
		Debug.printf("Count %d \n\r", counter1.getCount());
		Debug.println("Counter 1 param:");

		cv1 = counter1.getCount();
		uint32_t pv1 = counter1.getMemoryLong(LOCATION_OFFSET_PREVIOUS_VALUE);

		co1 = counter1.getMemoryInt(LOCATION_OFFSET_COUNTER_OVERFLOW);
		ii1 = counter1.getMemoryLong(LOCATION_OFFSET_INITIAL_INDICATION);

		cVal1 = (cv1*co1);
		cVal1 = cVal1 / count_dr1 + ii1;

		Debug.printf("counter1: Current value %d, Previos value: %d, Counter overlov: %d, Initial indicator: %d, count_dr1: %d, cVal1:%d.%02d\n", cv1, pv1, co1, ii1, count_dr1, int(cVal1), int((cVal1 - int(cVal1)) * 100));

		Debug.println("Counter 2 value:");
		Debug.printf("Count %ld \r\n", counter2.getCount());

		Debug.println("Counter 2 param:");

		cv2 = counter2.getCount();
		uint32_t pv2 = counter2.getMemoryLong(LOCATION_OFFSET_PREVIOUS_VALUE);

		co2 = counter2.getMemoryInt(LOCATION_OFFSET_COUNTER_OVERFLOW);
		ii2 = counter2.getMemoryLong(LOCATION_OFFSET_INITIAL_INDICATION);


		cVal2 = (cv2*co2);
		cVal2 = cVal2 / count_dr2 + ii2;

		Debug.printf("counter2: Current value %d, Previos value: %d, Counter overlov: %d, Initial indicator: %d, count_dr2: %d, cVal2:%d.%02d\n", cv2, pv2, co2, ii2, count_dr2, int(cVal2), int((cVal2 - int(cVal2)) * 100));

		Debug.println("Temperature:");
		Debug.printf("T1: %d.%02d T2: %d.%02d T3: %d.%02d\n", int(tempT0), int((tempT0 - int(tempT0)) * 100), int(tempT1), int((tempT1 - int(tempT1)) * 100), int(tempT2), int((tempT2 - int(tempT2)) * 100));


		Debug.println("Hozblock:");
		Debug.printf("humidity:%d.%2d, temperature1:%d.%d, temperature2:%d.%d\n", humidity / 10, humidity % 10, temperature1 / 10, temperature1 % 10, temperature2 / 10, temperature2 % 10);
		Debug.printf("Door1:%s,Door2:%s,Lamp1:%s,Lamp2:%s,Fan1:%s,Fan2:%s\n", Door1 == 0 ? "Closed" : "Open", Door2 == 0 ? "Closed" : "Open", Lamp1 == 0 ? "Off" : "On", Lamp2 == 0 ? "Off" : "On", Dimmer1 == 255 ? "Off" : String(Dimmer1).c_str(), Dimmer2 == 255 ? "Off" : String(Dimmer2).c_str());

	}
	Debug.isActive(Debug.VERBOSE);
}

void processCmdRemoteDebug() {
	String lastCmd = Debug.getLastCommand();
	lastCmd.toLowerCase();

	uint8_t selCounter = 0;
	uint8_t selParam = 0;
	uint32_t selValue = 0;

	if (lastCmd == "gi") {
		Debug.isActive(Debug.VERBOSE);
		Debug.println("Get info");
		showInfo();
	}
	else if (lastCmd == "gv") {
		Debug.isActive(Debug.VERBOSE);
		Debug.println("Get value");
		showValue();
	}
	else if (lastCmd == "swserial on") {
		showSwSerial = 1;
		Debug.println("swserial set  to on");
	} if (lastCmd == "swserial off") {
		showSwSerial = 0;
		Debug.println("swserial set  to off");
	}
	else

		if (lastCmd.startsWith("set")) {
			lastCmd.remove(0, 3);
			lastCmd.trim();

			if (lastCmd.startsWith("c1")){
				selCounter = 1;
			}
			else if (lastCmd.startsWith("c2")){
				selCounter = 2;
			}
			else
			{
				Debug.printf("Error in string %s (%s)\r\n", Debug.getLastCommand().c_str(), lastCmd.c_str());
				return;
			}

			lastCmd.remove(0, 2);
			lastCmd.trim();

			if (lastCmd.startsWith("co")){
				selParam = 1;
			}
			else if (lastCmd.startsWith("ii")){
				selParam = 2;
			}
			else if (lastCmd.startsWith("cv")){
				selParam = 3;
			}
			else if (lastCmd.startsWith("dr")){
				selParam = 4;
			}
			else {
				Debug.printf("Error in string %s (%s)\r\n", Debug.getLastCommand().c_str(), lastCmd.c_str());
				return;
			}

			lastCmd.remove(0, 2);
			lastCmd.trim();

			selValue = lastCmd.toInt();
			if (selValue == 0) {
				Debug.printf("Error in string %s (%s) Value can not be 0!!!\r\n", Debug.getLastCommand().c_str(), lastCmd.c_str());
				return;
			}

			if (selCounter == 1){
				switch (selParam)
				{
				case 1:
					counter1.setMemoryInt(LOCATION_OFFSET_COUNTER_OVERFLOW, selValue);
					break;
				case 2:
					counter1.setMemoryLong(LOCATION_OFFSET_INITIAL_INDICATION, selValue);
					break;
				case 3:
					counter1.setCount(selValue);
					break;
				case 4:
					count_dr1 = selValue;
					counter1.setMemoryLong(LOCATION_OFFSET_DIVISION_RATIO, count_dr1);
					break;
				}
			}
			else
				if (selCounter == 2){
					switch (selParam)
					{
					case 1:
						counter2.setMemoryInt(LOCATION_OFFSET_COUNTER_OVERFLOW, selValue);
						break;
					case 2:
						counter2.setMemoryLong(LOCATION_OFFSET_INITIAL_INDICATION, selValue);
						break;
					case 3:
						counter2.setCount(selValue);
						break;
					case 4:
						count_dr2 = selValue;
						counter2.setMemoryLong(LOCATION_OFFSET_DIVISION_RATIO, count_dr2);
						break;
					}
				}


		}

	/*else if (lastCmd == "se") {
		Debug.setSerialEnabled(true);
		Debug.println("Set seral enabled");
		}*/
}


void led_Pin_invert(){
	if (digitalRead(LED_PIN))
		digitalWrite(LED_PIN, LOW);
	else
		digitalWrite(LED_PIN, HIGH);
}


void setup() {
	uart_set_debug(UART_NO);
	Serial.begin(115200);
	Serial.println("Booting v.1.6");
	swSer.begin(9600);

	WiFi.hostname(host);
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);


	uint8_t i = 0;

	while (WiFi.waitForConnectResult() != WL_CONNECTED) {
		Serial.println("Connection Failed!");
		digitalWrite(LED_PIN, LOW);
		led_Pin_invert(); delay(500); led_Pin_invert(); delay(500); led_Pin_invert(); delay(1000); led_Pin_invert(); delay(500); led_Pin_invert(); delay(1000);

		if (i++ > 20){
			Serial.println("Rebooting...");
			ESP.restart();
		}
	}


	Debug.begin(MYHOSTNAME);
	Debug.setResetCmdEnabled(true);
	Debug.showDebugLevel(false);
	//Debug.showProfiler(true); // Profiler
	//Debug.showColors(true); // Colors
	//Debug.setSerialEnabled(true);

    Serial.print("Ip module:");
    Serial.println(WiFi.localIP());


	String helpCmd = "gi get info\r\n";
	helpCmd.concat("gv get value\r\n");
	helpCmd.concat("\r\n");
	helpCmd.concat("swSerial On/off\r\n");
	helpCmd.concat("\r\n");
	helpCmd.concat("set counter value:\r\n");
	helpCmd.concat("set [cN] [param] [value]\r\n");
	helpCmd.concat("with cN c1 or c2\r\n");
	helpCmd.concat("[param]:\r\n");
	helpCmd.concat("co - counter overlov\r\n");
	helpCmd.concat("ii - initial indicator\r\n");
	helpCmd.concat("cv - current value\r\n");
	helpCmd.concat("dr - division ratio\r\n");


	Debug.setHelpProjectsCmds(helpCmd);
	Debug.setCallBackProjectCmds(&processCmdRemoteDebug);



	// Start up the library
	sensors.begin();


	if (counter1.getMode() != MODE_EVENT_COUNTER)
	{
		Debug.println("chip pcf8583 1 need init");
		counter1.setMode(MODE_EVENT_COUNTER);
		for (int i = 0x10; i < 0xFF; i++)
			counter1.setRegister(i, 0);
	}


	if (counter2.getMode() != MODE_EVENT_COUNTER)
	{
		Debug.println("chip pcf8583 2 need init");
		counter2.setMode(MODE_EVENT_COUNTER);
		for (int i = 0x10; i < 0xFF; i++)
			counter2.setRegister(i, 0);
	}


	count_dr1 = counter1.getMemoryLong(LOCATION_OFFSET_DIVISION_RATIO);
	if (count_dr1 == 0) {
		count_dr1 = 100;
		counter1.setMemoryLong(LOCATION_OFFSET_DIVISION_RATIO, count_dr1);
	}


	count_dr2 = counter2.getMemoryLong(LOCATION_OFFSET_DIVISION_RATIO);
	if (count_dr2 == 0) {
		count_dr2 = 100;
		counter2.setMemoryLong(LOCATION_OFFSET_DIVISION_RATIO, count_dr2);
	}


	// Port defaults to 8266
	// ArduinoOTA.setPort(8266);

	// Hostname defaults to esp8266-[ChipID]
	// ArduinoOTA.setHostname("myesp8266");

	// No authentication by default
	//ArduinoOTA.setPassword((const char *)"123");


	//================================================================================================
	// OTA auto update


	ArduinoOTA.onStart([]() {
		ESP.wdtDisable();
		Debug.println("Start");

	});


	ArduinoOTA.onEnd([]() {
		Debug.println("\nEnd");
	});


	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Debug.printf("Progress: %u%%\r\n", (progress / (total / 100)));
	});


	ArduinoOTA.onError([](ota_error_t error) {
		Debug.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) Debug.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Debug.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Debug.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Debug.println("Receive Failed");
		else if (error == OTA_END_ERROR) Debug.println("End Failed");
	});


	//================================================================================================

	ArduinoOTA.setHostname(host);
	ArduinoOTA.begin();

	showInfo();

	pinMode(LED_PIN, OUTPUT);

	ESP.wdtEnable(WDTO_4S);
}


void loop() {

	uint8_t i = 0;
	float t = 0;

	while (WiFi.waitForConnectResult() != WL_CONNECTED) {
		Serial.println("Connection Failed!");
		delay(1000);
		if (i++ > 10){
			Serial.println("Rebooting...");
			ESP.restart();
		}
	}



	ESP.wdtFeed();

	if (millis() - previousMillisLamp >= 10000) {
		previousMillisLamp = millis();

		//чтение сенсоров
		Debug.isActive(Debug.DEBUG);
		Debug.println("Read sensor");

		sensors.requestTemperatures();
		i = 0;
		t = sensors.getTempC(t0);
		/*
		while (t == 85)
		{
		if ((i++) > 5) break;
		delayMicroseconds(100);
		t = sensors.getTempC(t0);
		}

		*/
		tempT0 = t;

		//Serial.print("T0 Temp C: ");
		//Serial.print(tempT0);
		//Serial.println();

		tempT1 = sensors.getTempC(t1);
		//Serial.print("T1 Temp C: ");
		//Serial.print(tempT1);
		//Serial.println();

		tempT2 = sensors.getTempC(t2);
		//Serial.print("T2 Temp C: ");
		//Serial.print(tempT2);
		//Serial.println();

		doWorkCounters();

		//char buf[50];
		//sprintf(buf, "test send string:%d", millis());

		//Serial.print("____Send: "); Serial.println(buf);
		//swSer.println(buf);

		//** Making Note of an expected rollover *****//  
		if (millis() >= 3000000000){
			upTimeHighMillis = 1;

		}
		//** Making note of actual rollover **//
		if (millis() <= 100000 && upTimeHighMillis == 1){
			upTimeRollover++;
			upTimeHighMillis = 0;
		}

	}

	client.loop();

	if (!client.connected()) {
		Debug.isActive(Debug.DEBUG);
		Debug.println("Connect to MQTT-boker...  ");
		snprintf(msg, msg_size, "%s", host);

		if (client.connect(msg, "q1", "q1")) {
			Debug.println("publish an announcement");
			// Once connected, publish an announcement...
			snprintf(msg, msg_size, "%ld", ESP.getFreeHeap());
			snprintf(topic, msg_size, "%s/freemem", host);
			client.publish(topic, msg);

			snprintf(topic, msg_size, "%s/wanip", host);
			WiFi.localIP().toString().toCharArray(msg, msg_size);
			client.publish(topic, msg);
			client.subscribe(topic);

			snprintf(topic, msg_size, "%s/mac", host);
			WiFi.macAddress().toCharArray(msg, msg_size);
			client.publish(topic, msg);

			snprintf(topic, msg_size, "%s/rssi", host);
			snprintf(msg, msg_size, "%ld", WiFi.RSSI());
			client.publish(topic, msg);

			snprintf(topic, msg_size, "%s/uptime", host);
			snprintf(msg, msg_size, "%ld", millis());
			client.publish(topic, msg);

			SendParam();
		}
		else
		{
			Debug.println("Delay");
			delay(1000);
		}
	}
	else
	{
		if (millis() - previousMillisSend >= 60000) {
			previousMillisSend = millis();
			Debug.println("Sendparam start");
			SendParam();
			Debug.println("Sendparam end");
		}
	}

	if (millis() - previousMillis >= 1000) {
		previousMillis = millis();
		led_Pin_invert();
	}



	SwSerialHandler();

	ArduinoOTA.handle();
	client.loop();
	Debug.handle();
}




