/*
**  Project: Easytotrack 
**  Revision: v3.3
**  With HWSerial for GPS module
**    
*/

#include "Easytotrack.h"

// see: https://github.com/nrwiersma/ESP8266Scheduler
#include <ESP.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <FS.h>

#include "GPRSmodem.h"
#include "GPS.h"
//#include "MPU9265module.h"
#include "fileSystem.h" // moet vóór configMode.h
#include "configMode.h"


void rebootTracker() {
    char cElapse[12];
    
    elapseTime = (millis() - bootTime) / 1000.0;
    dtostrf(elapseTime, 5, 1, cElapse);

    Serial.println("*** REBOOT *** (Watchdog Timer)!");
    Serial.printf("Time elapsed [T+%s])!\n", cElapse);

    sprintf(cMsg, "Software WDT (processing [%s])!", posSoftWDT.c_str());
    writeLogFile(cMsg);
    sprintf(cMsg, "Time elapsed [T+%s])!", cElapse);
    writeLogFile(cMsg);
    heapSpace = ESP.getFreeHeap();
    sprintf(cMsg, "Free Heap [%d] bytes", heapSpace);
    writeLogFile(cMsg);

    delay(100);
    ESP.restart();
    delayGps(5000);   

}   // rebootTracker()

void tockleGrnLed() {
    digitalWrite(GRN_LED, !digitalRead(GRN_LED));

}   // tockleGrnLed()


void stopSoftWDT() {
    posSoftWDT = "";
    //Serial.println("Stop SoftWDT");
    tickObject.attach(_BLINK_TIME, tockleGrnLed);
    digitalWrite(RED_LED, LOW);

}   // stopSoftWDT()


void startSoftWDT(uint32_t maxTime, String programPosition) {
    posSoftWDT = programPosition;
    tickObject.attach(maxTime, rebootTracker);
    sprintf(cMsg, "Set SoftWDS for [%d]sec. @[%s]", maxTime, posSoftWDT.c_str());
    Serial.println(cMsg);
    if (!programPosition.equals("getGpsFix")) {
        digitalWrite(RED_LED, HIGH);
    }
    digitalWrite(GRN_LED, !digitalRead(GRN_LED));
    
}   // startSoftWDT()

                                                                                                                                                     
void signal(int Led, int repeat) {
    digitalWrite(Led, LOW);
    for (int L=0; L<(repeat * 2); L++) {
        digitalWrite(Led, !digitalRead(Led));
        delayGps(100);  
    }
    digitalWrite(Led, LOW);
    
}   // signal()


void freeHeap() {
    int32_t oldHeapSpace;
    oldHeapSpace = heapSpace;
    heapSpace = ESP.getFreeHeap();
    Serial.print("\nFree Heap: ");
    Serial.print(heapSpace);
    Serial.print(" bytes ");

    if (oldHeapSpace == 0) {
        Serial.println();
        return;
    }
    
    if (heapSpace < oldHeapSpace) {
        sprintf(cMsg, "Lost %d bytes", (oldHeapSpace - heapSpace));
        Serial.print(cMsg);
        sprintf(cMsg, "now [%d] bytes", heapSpace);
    } else if (heapSpace > oldHeapSpace) {
        sprintf(cMsg, "Gained %d bytes", (oldHeapSpace - heapSpace));
        Serial.print(cMsg);
        sprintf(cMsg, "now [%d] bytes", heapSpace);
    }
    
    Serial.println("\n");
    
} // freeHeap();


String popFromString(String &sIn, char seperator) {
    int sStart, sEnd;
    String  sUit = "";

    if (sIn.length() == 0) {
        return sUit;
    }
    //Serial.printf("[%s] popFromString()\n", sIn.c_str());
    sEnd    = sIn.indexOf(seperator);
    if (sEnd == 0) sEnd = sIn.length();
    sUit    = sIn.substring(0, sEnd);
    sUit.trim();
    sIn     = sIn.substring(sEnd + 1);
    if (sIn == sUit)   sIn = "";

    //Serial.printf("sUit[%s] => sIn[%s]\n", sUit.c_str(), sIn.c_str());

    return sUit;
    
}   // popFromString()


void sendSMSonline() {
    char cElapse[12];

    Serial.print("Send SMS ");
    useGsmLocation();
    elapseTime  = (millis() - bootTime) / 1000.0;
    dtostrf(elapseTime, 5, 1, cElapse);
    
    sprintf(cMsg, "Online T+%s ", cElapse);
    mqttComment.concat(cMsg);
    
    useGsmLocation();
    
    sprintf(cMsg, "[%02d:%02d:%02d] '%s' is Online\nElapse T+%s\n[%s]\n%s" 
                                                , gsmData.hour, gsmData.minute, gsmData.second
                                                , confDevice.c_str()
                                                , cElapse
                                                , modemIMEI.c_str() );

    cMsg[139] = '\0';
    if (modem.sendSMS(confNrToSMS, cMsg)) {
        yield();
        int p=0;
        for (p=0; cMsg[p] > 0; p++) {
            if (cMsg[p] == '\n') {
                cMsg[p] = ' ';
            }
        }
        cMsg[p++] = 0;
        cMsg[p++] = 0;
        Serial.printf("%s \nto %s\n", cMsg, confNrToSMS.c_str());
    } else {
        Serial.println("Error sending SMS");
    }

}   // sendSMSonline()


void useGsmLocation() {
    String  temp, Date, Time;
    int     sStart, sEnd;

    if (confHasSIM) {
        gsmLocation = modem.getGsmLocation();
    }
    if (gsmLocation.length() < 15) return;
    
    // gsmLocation is like "0,5.215241,52.346447,2017/07/25,12:29:57"
    //------------------    1 <--2---> <---3---> <----4---> <--4--->     
         
    temp = popFromString(gsmLocation, ',');                         // skip -1-
    //  Longtitude
    gsmData.lng     = popFromString(gsmLocation, ',').toFloat();    // -2-
    // Latitude
    gsmData.lat     = popFromString(gsmLocation, ',').toFloat();    // -3-
    gps.location.isValid();
    // date
    Date            = popFromString(gsmLocation, ',');              // -4-
    // Time
    Time = popFromString(gsmLocation, ',');                         // -5-
    // now split date
    gsmData.year    = popFromString(Date, '/').toInt();
    gsmData.month   = popFromString(Date, '/').toInt();
    gsmData.day     = popFromString(Date, '/').toInt();
    gps.date.isValid();
    // now split time
    gsmData.hour    = popFromString(Time, ':').toInt();
    gsmData.hour   += confTimeUTC;
    if (gsmData.hour >= 24) gsmData.hour -= 24;
    gsmData.minute  = popFromString(Time, ':').toInt();
    gsmData.second  = popFromString(Time, ':').toInt();
    gps.time.isValid();
    
    locationFromGsm = true;

    //displayGpsData();
    
}   // useGsmLocation()


String buildMqttStrng() {
    char        mqttStrng[200];
    char        cTmp[20], cLat[12], cLng[12], cSpeed[12], cAlt[12];
    uint32_t    epoch;
    int         p, p2;

    if (confHasSIM) {
        useGsmLocation();

        if (gpsData.satellites < 1) {
            gpsData.lat     = gsmData.lat;
            gpsData.lng     = gsmData.lng;
            gpsData.year    = gsmData.year;
            gpsData.month   = gsmData.month;
            gpsData.day     = gsmData.day;
            gpsData.hour    = gsmData.hour;
            gpsData.minute  = gsmData.minute;
            gpsData.second  = gsmData.second;
        }
    }

    epoch = makeTime(gpsData.hour, gpsData.minute, gpsData.second
                   , gpsData.day,  gpsData.month,  gpsData.year);

    dtostrf(gpsData.lat, 10, 6, cTmp);
    p2 = 0;
    for (p=0; cTmp[p] != 0; p++) if (cTmp[p] != ' ') cLat[p2++] = cTmp[p];
    cLat[p2] = 0;
    dtostrf(gpsData.lng, 10, 6, cTmp);
    p2 = 0;
    for (p=0; cTmp[p] != 0; p++) if (cTmp[p] != ' ') cLng[p2++] = cTmp[p];
    cLng[p2] = 0;
    dtostrf(gpsData.speedKmph, 10, 2, cTmp);
    p2 = 0;
    for (p=0; cTmp[p] != 0; p++) if (cTmp[p] != ' ') cSpeed[p2++] = cTmp[p];
    cSpeed[p2] = 0;
    dtostrf(gpsData.altitude, 10, 2, cTmp);
    p2 = 0;
    for (p=0; cTmp[p] != 0; p++) if (cTmp[p] != ' ') cAlt[p2++] = cTmp[p];
    cAlt[p2] = 0;
    mqttComment.substring(0, 25).trim();

    sprintf(mqttStrng, "%s; %s; %d; %s; %s; %s; %s; %3d; %2d; %4d; %04d-%02d-%02d; %02d:%02d:%02d; %s; "
                        , modemIMEI.c_str(), confDevice.c_str(), epoch
                        , cLat, cLng, cSpeed, cAlt, strokePM
                        , gpsData.satellites, gpsData.hdop
                        , gpsData.year, gpsData.month, gpsData.day
                        , gpsData.hour, gpsData.minute, gpsData.second
                        , mqttComment.c_str() );

    mqttComment = "";
    return String(mqttStrng);
    
}   // buildMqttStrng()


bool sendDataToMQTT() {
    sendCount++;
    digitalWrite(RED_LED, HIGH);
    Serial.printf("[%d] Send data to MQTT server [%s] @%02d:%02d:%02d\n" 
                        , sendCount, confBroker.c_str()
                        , gpsData.hour, gpsData.minute, gpsData.second);

    if (confHasSIM) {
        // als GPRSconnect goed gaat, wordt de Buffer naar MQTT gestuurd
        GPRSconnect(3, MODEM_SEND_TO_MQTT); 
        //if (modemState <= MODEM_TO_APN) {
        //    Serial.println("\nNo connection to APN, stop trying!!!!!\n");
        //    confHasSIM = 0;
        //}
    } else {
        Serial.println("Records in Buffer:");
        getGpsDataFromBuffer();
    }
    
    digitalWrite(RED_LED, LOW);

}   // sendDataToMQTT()


void setup() {
    char cElapse[12];

    WiFi.mode(WIFI_OFF);
    bootTime    = millis();
    firstFix    = true;
    
    Serial.begin(9600);
    Serial.flush();

    SerialAT.begin(19200);
    SerialAT.flush();
    pinMode(RED_LED, OUTPUT);  // RED LED
    digitalWrite(RED_LED, LOW);
    pinMode(GRN_LED, OUTPUT);  // GREEN LED (WDT)
    digitalWrite(GRN_LED, LOW);
    pinMode(YLW_LED, OUTPUT);  // GREEN LED (WDT)
    digitalWrite(YLW_LED, LOW);

    pinMode(CONFIG_SWITCH_PIN, INPUT);

    startSoftWDT(25, "Booting");

    Serial.println("\nBooting....");

    Serial.println("\n---------------------------------------------------");
    String progName = String(__FILE__);
    progName = progName.substring(progName.lastIndexOf('/')+1);
    progName = progName.substring(0, progName.lastIndexOf(".ino"));
    Serial.printf("Program : %s\n", progName.c_str());
    Serial.printf("Version : %s / Config Version: %s\n",String(_PROGVERSION).c_str(), String(_CONFIGVERSION).c_str() );
    Serial.printf("Compiled: %s / %s\n", String(__DATE__).c_str(), String(__TIME__).c_str());
    Serial.println("Library's ----");
    Serial.println(" * TinyGsmClient    : Nov 2016");
    Serial.println(" * Ticker           : Copyright (c) 2014 Ivan Grokhotkov");
    Serial.println(" * PubSubClient     : June 2017 (?)");
    Serial.println(" * TinyGPS          : Copyright (C) 2008-2013 Mikal Hart");
    Serial.println(" * EspSoftwareSerial: Copyright (c) 2015-2016 Peter Lerup");
    Serial.println("---------------------------------------------------\n");
    rebootFlag  = true;
    Serial.println("\nPress [Config] to config ..\n");

    yield();
    signal(RED_LED, 10);
    signal(GRN_LED, 10);
    signal(YLW_LED, 5);

    lastReset   = ESP.getResetReason();
    sprintf(cMsg, "Last reset reason: [%s]\n", ESP.getResetReason().c_str());
    if (lastReset.length() > 2) {
        Serial.println(cMsg);
        mqttComment.concat(cMsg);
        mqttComment.concat(" ");
    }

    Serial.print("mount SPIFFS file system .. ");
    int t=0;
    while (!SPIFFS.begin() && t<10) {
        t++;
        Serial.print(".");
        yield();
    }
    if (t < 10) Serial.println("OK");
    else        Serial.println("ERROR");    
    if (lastReset.length() > 2) {
        writeLogFile(cMsg);
    }

    yield();

    stopSoftWDT();   // stop WDT and start blinking GRN led
    
    while ((digitalRead(CONFIG_SWITCH_PIN) == LOW) || !readConfigFile(true, false)) {
        digitalWrite(RED_LED, LOW);
        digitalWrite(RED_LED, HIGH);
        configMode();
        yield();
        if (emptyGpsDataFile()) Serial.println("Buffer empty");
      //else                    Serial.println("ERROR emptying Buffer");
        ESP.reset();
        delayGps(5000);   
    }
    
    startSoftWDT(30, "Read Config file");
    yield();
    if (!readConfigFile(true, true)) {
        if (!readConfigFile(false, false)) {   // read brom backup
            confDevice      = "Easytotrack";
            confBroker      = "test.mosquitto.org";
            confLogIntern   = 1;
            confHasSIM      = 0;
        }
    }
    // --- (in)sanity --------------------
    if (confDevice.length() < 3) {
        confDevice      = "Easytotrack";
        confBroker      = "test.mosquitto.org";
        confLogIntern   = 1;
        confHasSIM      = 0;
    }

    stopSoftWDT();

    yield();

    digitalWrite(RED_LED, LOW);

    initGpsData();
    //delayGps(1000); << this "hangs" the system .. why????

    sendCount = 0;
    int tempHasSIM = confHasSIM;
    sendDataToMQTT();
    Serial.print("empty GPS buffer .. ");
    if (emptyGpsDataFile()) Serial.println("OK");
    else                    Serial.println("Alreay empty!");
    confHasSIM = tempHasSIM;

    modemIMEI = modem.getIMEI();
    Serial.printf("\nmodem IMEI: %s\n", modemIMEI.c_str());

    if (confHasSIM) {
        yield();
        delayGps(10);
        useGsmLocation(); // so we now have the date & time

        if (confSendSMS) {
            yield();
            delayGps(10);
            sendSMSonline();
        }
    } 

    startSoftWDT(10, "ElapseTime2Log");
    elapseTime  = (millis() - bootTime) / 1000.0;
    dtostrf(elapseTime, 5, 1, cElapse);
    sprintf(cMsg, "[%04d-%02d-%02d] Online after T+%sseconds", gsmData.year, gsmData.month, gsmData.day
                                                             , cElapse );
    gpsData.hour    = gsmData.hour;
    gpsData.minute  = gsmData.minute;
    gpsData.second  = gsmData.second;
    writeLogFile(cMsg);
    stopSoftWDT();

    delayGps(10);

    //startSoftWDT(10, "MPU9265setup()");
    //MPU9265setup();
    //stopSoftWDT();
    motionTaskTimer = millis() + 90000;    // pas na setup motion detection starten

    putGpsDataInBuffer(true); // only write the CSV-Headers

    if (getGpsFix(3, 120000, true)) {    // give it 120 seconds to fix
        writeLogFile("Got a GPS fix!");
    } else {
        if (confHasSIM) {
            useGsmLocation();
            Serial.printf("\nGSM location data: %s\n\n", gsmLocation.substring(2).c_str());
        } else {
            initGsmData();
        }
    }
    prevLat = gpsData.lat;
    prevLng = gpsData.lng;

    sampleCount = 0;
    processGpsData(true);   // always write first location
    
    sendTaskTimer   = millis() + (confSend * 1000);     // Send every Interval seconds
    sampleTaskTimer = millis() + (confSample * 1000);   // sample every confSample seconds
    distTaskTimer   = millis() + _DIST_TASK_TIME;                  // iterate every second
    gpsTaskTimer    = millis() + _GPS_TASK_TIME;
    blinkTaskTimer  = millis() + _BLINK_TIME;
    
}   // setup()


void loop() {
    delayGps(500);  // spend some time collecting GPS data
    
    epoch   = makeTime( gpsData.hour, gpsData.minute, gpsData.second
                      , gpsData.day,  gpsData.month,  gpsData.year);
    distM   = gps.distanceBetween(prevLat, prevLng, gpsData.lat, gpsData.lng);
    
    yield();
    // see if we have to store a sample based on distance
    if ((confDistance > 0) && (distM >= confResMin)) {       // location @distance?
        //Serial.printf("Save sample based on distance (%dm) @%02d:%02d:%02d [age %dms]\n", distM
        //              , gpsData.hour, gpsData.minute, gpsData.second, gps.location.age());
        distTaskTimer   = millis() + _DIST_TASK_TIME;       // sample every one _DIST_TASK_TIME
        sampleTaskTimer = millis() + (confSample * 1000);   // reset timer
        processGpsData(false);
    } 
    
    yield();
    // see if we have to store a sample based on sample-time
    if ((millis() > sampleTaskTimer) && (distM >= confResMin)) {       // location @distance?
        //Serial.printf("Save sample based on time @%02d:%02d:%02d (%dm) [age %dms]\n", gpsData.hour, gpsData.minute, gpsData.second
        //                                                                 , distM, gps.location.age());
        sampleTaskTimer = millis() + (confSample * 1000);   // reset timer
        processGpsData(false);
    } 

    yield();
    if (millis() > sendTaskTimer) {
        sendTaskTimer = millis() + (confSend * 1000);     // reset Send Interval timer
        RegStatus mRS = modem.getRegistrationStatus();
        RSSI          = modem.getSignalQuality();
        if (confHasSIM && (mRS == 3 || mRS == 4 || RSSI == 99)) {
            sprintf(cMsg, "No use sending data to MQTT! modemState is %d, RSSI is %d", mRS, RSSI);
            Serial.println(cMsg);
            writeLogFile(cMsg);
            //modenState  = MODEM_TO_APN; // Dit lijkt niet nodig!
        } else {

            if (!locationInBuffer) {    // no sample stored yet!
                Serial.println("Time to send, but buffer is empty, so force Save");
                processGpsData(true);   // so force sample into buffer
            }
            sendDataToMQTT();
            if (bufferWasSend) {
                Serial.print("empty GPS buffer .. ");
                if (emptyGpsDataFile()) Serial.println("OK");
                else                    Serial.println("already empty(?)");
            } else {
                Serial.print("keep GPS buffer .. ");
            }
        }
    }
    gpsData.satellites--;

    //if (millis() > motionTaskTimer) {
    //    motionTaskTimer = millis() + _MOTION_TASK_TIME;     // reset Motion Interval timer
    //    MPU9265process(false);
    //}

}   // loop()
