﻿/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>

/*End of auto generated code by Atmel studio */

//v4 added compiler switch to turn on logging to sdcard
//v5 added close when stopLogging is set; begin and open when logging is resumed
//added mod to SD.cpp
//v6 added varCountTMP and varCountChipTemp to packet
//initialise syncTime in setup()
//track packet transmission% by node
//change filename to SD_03_xx
//packet tracking for all nodes
//v7 created 19/11/18 to create logger on serial port; might not use this fork!
//change #if defined to #if
//added compiler switch for RT_CLOCK
//fork to report data from SoilSense; separate monitor for TNode
//modify packet structure

/*
if (root.isOpen()) root.close();  // <<<<<<<<<<<<<<<<<<  ADD THIS LINE
*/

#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile
//Beginning of Auto generated function prototypes by Atmel Studio
void logfilecomma();
void serialcomma();
void error(byte error_no);
void flushbuffer();
//End of Auto generated function prototypes by Atmel Studio




// for the data logging shield, we use digital pin 10 for the SD cs line
#define ECHO_TO_SERIAL 1 // echo data to serial port
#define LOG_TO_SDCARD 1 // log data received
#define RT_CLOCK 1 // use of RTC
#define MAX_NODE 16
const uint32_t syncInterval = 180000; // mills between calls to flush() - to write data tao the card
const int chipSelect = 10;
const byte buttonPin = 5;
const byte ledPin = 13;
const byte rxPin = 7;
const byte txPin = 8;
const uint16_t baudRate = 500;

// parameters to calculate %transmission
const unsigned int packetCountMax = 16;
unsigned int packetCount[MAX_NODE];
uint32_t syncTime; // time of last sync()
uint8_t startPacket[MAX_NODE];
boolean stopLogging = false; // used for runtime remove of SD card
boolean buttonState = true; // high unless button pressed; used for runtime remove of SD card
boolean firstPacket[MAX_NODE]; // flag to initialise variables in main loop



#if RT_CLOCK
RTC_DS1307 RTC; // define the Real Time Clock object
#endif
#if LOG_TO_SDCARD
File logfile;  // the logging file
char filename[] = "sd_03_00.CSV";
#endif

RH_ASK driver(baudRate, rxPin, txPin); //speed, Rx pin, Tx pin
/// Constructor.
/// At present only one instance of RH_ASK per sketch is supported.
/// \param[in] speed The desired bit rate in bits per second
/// \param[in] rxPin The pin that is used to get data from the receiver
/// \param[in] txPin The pin that is used to send data to the transmitter
/// \param[in] pttPin The pin that is connected to the transmitter controller. It will be set HIGH to enable the transmitter (unless pttInverted is true).
/// \param[in] pttInverted true if you desire the pttin to be inverted so that LOW wil enable the transmitter.

////////////////////////////////////struct PayloadItem/////////////////////////////////////
typedef struct PayloadItem
{
	byte nodeId; //store this nodeId
	byte count;
	byte bin2usCoarse;
	byte bin2usFine;
	byte varCoarse;
	byte varFine;
	unsigned long coarseTime;
	unsigned long fineTime;
};
// count keeps track of the # packets sent out

PayloadItem payloadTime;


////////////////////////////////////struct PayloadItem2/////////////////////////////////////
typedef struct PayloadItem2
{
	byte nodeId; //store this nodeId
	byte count;
	int temp;
	int Vcc;
	unsigned long millisec;
	unsigned long empty;
};
// time is approximate since  using only watchdog timer

PayloadItem2 payloadStatus;



////////////////////////////////////logfilecomma/////////////////////////////////////
void logfilecomma()
{
	#if LOG_TO_SDCARD
	logfile.print(",");
	#endif
}
/////////////////////////////////serialcomma////////////////////////////////////////
void serialcomma()
{
	Serial.print(",");
}
/////////////////////////////////error////////////////////////////////////////


void error(byte error_no)
{
	#if ECHO_TO_SERIAL
	Serial.print(F("error: "));
	switch (error_no) {
		case 1:
		Serial.println(F("Card"));
		break;
		case 2:
		Serial.println(F("File"));
		break;
		case 3:
		Serial.println(F("RTC"));
		break;
		case 4:
		Serial.println(F("RF"));
		break;
	}
	#endif
	// LED indicates error
	digitalWrite(ledPin, HIGH);
	while (1);
}

///////////////////////////////////flushbuffer//////////////////////////////////////
void flushbuffer()
// only called once
{
	#if LOG_TO_SDCARD

	// LED to show we are syncing data to the card & updating FAT!
	digitalWrite(ledPin, HIGH);
	cli(); // block all interrupts; unsure whether RF Rx can interrupt SPI comms
	logfile.flush();
	sei();
	digitalWrite(ledPin, LOW);
	#if ECHO_TO_SERIAL
	Serial.println(F("Flushed buffer"));
	#endif

	#endif

}
///////////////////////////////////setup//////////////////////////////////////

void setup() {
	#if ECHO_TO_SERIAL
	Serial.begin(115200);
	#endif

	// use debugging LED
	pinMode(ledPin, OUTPUT);

	#if ECHO_TO_SERIAL
	Serial.println(F("Type character"));
	while (!Serial.available());
	#endif // wait to start


	#if LOG_TO_SDCARD

	// initialize the SD card
	#if ECHO_TO_SERIAL
	Serial.print(F("Initializing SD card..."));
	#endif
	// make sure that the default chip select pin is set to
	// output, even if you don't use it:
	pinMode(chipSelect, OUTPUT);

	// see if the card is present and can be initialized:
	if (!SD.begin(chipSelect)) {
		error(1);
	}
	#if ECHO_TO_SERIAL
	Serial.println(F("card initialized."));
	#endif


	// create a new file
	for (uint8_t i = 0; i < 100; i++) {
		filename[6] = i / 10 + '0';
		filename[7] = i % 10 + '0';
		if (! SD.exists(filename)) {
			// only open a new file if it doesn't exist
			logfile = SD.open(filename, FILE_WRITE);
			break;  // leave the loop!
		}
	}

	if (! logfile) {
		error(2);
	}
	#if ECHO_TO_SERIAL
	Serial.print(F("Log-> "));
	Serial.println(filename);
	#endif

	logfile.println(F("millis,stamp,node,count,RCtime,tmp,chipTemp,Vbatt,var1,var2"));
	#endif // LOG_TO_SDCARD
	
	#if RT_CLOCK
	// connect to RTC
	Wire.begin();
	if (!RTC.begin())
	{
		error(3);
	}
	#endif
	
	
	if (!driver.init())
	{
		error(4);
	}

	#if LOG_TO_SDCARD
	logfile.print(F("millis,"));
	#if RT_CLOCK
	logfile.print(F("stamp,"));
	#endif
	logfile.println(F("node,count,varCoarse,varFine,bin2usCoarse,bin2usFine,coarseTime,fineTime"));
	#endif



	#if ECHO_TO_SERIAL
	Serial.print(F("millis,"));
	#if RT_CLOCK
	Serial.print(F("stamp,"));
	#endif
	Serial.println(F("node,count,varCoarse,varFine,bin2usCoarse,bin2usFine,coarseTime,fineTime"));
	#endif

	syncTime = millis(); // time of last sync()

	for (uint8_t i = 0; i < MAX_NODE; i++) firstPacket[i] = true;

	// signal completed initialisation
	for (uint8_t i = 0; i < 10; i++) {
		digitalWrite(ledPin, HIGH);
		delay(500);
		digitalWrite(ledPin, LOW);
		delay(500);
	}
}
/////////////////////////////////loop////////////////////////////////////////

void loop() {
	#if LOG_TO_SDCARD

	if (buttonState && !digitalRead(buttonPin)) // check for button push NB debouncing hardwired
	{
		buttonState = false;
		stopLogging = !stopLogging;
		if (stopLogging)
		{
			logfile.close();
			#if ECHO_TO_SERIAL
			Serial.println(F("OK to remove SD card"));
			#endif
		}
		else // start logging so initialise SD & open file
		{
			// see if the card is present and can be initialized:
			if (!SD.begin(chipSelect)) {
				error(1);
			}
			logfile = SD.open(filename, FILE_WRITE);
			#if ECHO_TO_SERIAL
			Serial.println(F("Logging resumed"));
			#endif
		}
	}
	if (!buttonState && digitalRead(buttonPin)) buttonState = true; // check for button release

	#endif

	if (stopLogging == true) // when logging stopped only action is to flash LED; packets are ignored
	{
		digitalWrite(ledPin, HIGH);
		delay(50);
		digitalWrite(ledPin, LOW);
		delay(250);
	}
	else
	{
		uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
		uint8_t buflen = sizeof(buf);
		if (driver.recv(buf, &buflen)) // Non-blocking
		{
			digitalWrite(ledPin, HIGH);
			delay(250);
			payloadTime = *(PayloadItem*)buf;
			payloadStatus = *(PayloadItem2*)buf;  // time and status packets have different structures
			byte node = payloadTime.nodeId & 0x0f; // the ID is the first nibble
			// log milliseconds since starting
			uint32_t m = millis();
			// fetch the time
			#if RT_CLOCK
			DateTime now = RTC.now();
			#endif
			long packetsSent = 0; // the first value logged is indeterminate only set on later logs

			if (firstPacket[node]) // initialise %packet variables
			{
				firstPacket[node] = false;
				packetCount[node] = 0;
				startPacket[node] = payloadTime.count;
			}
			else
			{
				++packetCount[node];
				// calculate the percentx10 transmission rate
				if (packetCount[node] == packetCountMax)
				{
					packetsSent = payloadTime.count - startPacket[node];
					if (packetsSent < 0) packetsSent = packetsSent + 256; // check for wrap-round
					packetsSent = 1000L * packetCountMax / packetsSent; // convert to percentx10
					packetCount[node] = 0;
					startPacket[node] = payloadTime.count;
				}
			}


			if ((payloadTime.nodeId & 0x0f) == 12)
			{
				#if LOG_TO_SDCARD
				logfile.print(m);         // milliseconds since start
				logfilecomma();
				#if RT_CLOCK
				logfile.print(now.unixtime()); // seconds since 1/1/1970
				logfilecomma();
				#endif
				logfile.print(payloadTime.nodeId);
				logfilecomma();
				logfile.print(payloadTime.count);
				logfilecomma();
				if (payloadTime.nodeId & 0x20)
				{
					logfile.print(payloadStatus.temp);
					logfilecomma();
					logfile.print(payloadStatus.Vcc);
					logfilecomma();
					logfile.print(payloadStatus.millisec);
					logfilecomma();
				}
				else
				{
					logfile.print(payloadTime.bin2usCoarse);
					logfilecomma();
					logfile.print(payloadTime.bin2usFine);
					logfilecomma();
					logfile.print(payloadTime.varCoarse);
					logfilecomma();
					logfile.print(payloadTime.varFine);
					logfilecomma();
					
					/*
					Print coarse measurement
					*/
					uint8_t convert2Microsec = payloadTime.bin2usCoarse;
					logfile.print(payloadTime.coarseTime >> convert2Microsec);
					logfile.print(F("."));
					logfile.print(((payloadTime.coarseTime & ((1L << convert2Microsec) - 1L)) * 100) >> convert2Microsec);
					logfilecomma();
					
					/*
					Now for the fine measurement
					*/
					convert2Microsec = payloadTime.bin2usFine;
					
					logfile.print(payloadTime.fineTime >> convert2Microsec);
					logfile.print(F("."));
					logfile.print(((payloadTime.fineTime & ((1L << convert2Microsec) - 1L)) * 10000) >> convert2Microsec);
				}
				if (!packetCount[node])
				{
					logfilecomma();
					logfile.println(packetsSent);
				}
				else
				{
					logfile.println();
				}
				#endif

				#if ECHO_TO_SERIAL
				Serial.print(m);         // milliseconds since start
				serialcomma();
				#if RT_CLOCK
				Serial.print(now.unixtime()); // seconds since 1/1/1970
				serialcomma();
				#endif
				Serial.print(payloadTime.nodeId);
				serialcomma();
				Serial.print(payloadTime.count);
				serialcomma();
				if (payloadTime.nodeId & 0x20)
				{
					Serial.print(payloadStatus.temp);
					serialcomma();
					Serial.print(payloadStatus.Vcc);
					serialcomma();
					Serial.print(payloadStatus.millisec);
					serialcomma();
				}
				else
				{
					Serial.print(payloadTime.bin2usCoarse);
					serialcomma();
					Serial.print(payloadTime.bin2usFine);
					serialcomma();
					Serial.print(payloadTime.varCoarse);
					serialcomma();
					Serial.print(payloadTime.varFine);
					serialcomma();
					
					/*
					Print coarse measurement
					*/
					uint8_t convert2Microsec = payloadTime.bin2usCoarse;
					Serial.print(payloadTime.coarseTime >> convert2Microsec);
					Serial.print(F("."));
					Serial.print(((payloadTime.coarseTime & ((1L << convert2Microsec) - 1L)) * 100) >> convert2Microsec);
					serialcomma();
					
					/*
					Now for the fine measurement
					*/
					convert2Microsec = payloadTime.bin2usFine;
					
					Serial.print(payloadTime.fineTime >> convert2Microsec);
					Serial.print(F("."));
					Serial.print(((payloadTime.fineTime & ((1L << convert2Microsec) - 1L)) * 10000) >> convert2Microsec);
				}
				if (!packetCount[node])
				{
					serialcomma();
					Serial.println(packetsSent);
				}
				else
				{
					Serial.println();
				}
				#endif
			}
			digitalWrite(ledPin, LOW);
		}

		// Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
		// which uses a bunch of power and takes time
		#if LOG_TO_SDCARD
		uint32_t m = millis();
		if ((m - syncTime) > syncInterval)
		{
			flushbuffer();
			syncTime = m;
		}
		#endif
	}
}