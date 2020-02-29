
#include "Easytotrack.h"

#include <TinyGPS++.h>

// The TinyGPS++ object
TinyGPSPlus gps;

// This custom version of delay() ensures that the gps object
// is being "fed".
void delayGps(uint32_t ms) {

    char     xIn;
    uint32_t maxTime = millis() + ms;

    do {
        while (Serial.available()) {
            xIn = Serial.read();
            if (debugGetFix) Serial.print(xIn);
            gps.encode(xIn);
            maxTime = millis() + ms;    // so, buy some time to collect ..
        }
        yield();
        //if (motionTaskTimer < millis()) {
        //    motionTaskTimer = millis() + _MOTION_TASK_TIME;     // reset Motion Interval timer
        //    MPU9265process(false);
        //}
    
    } while (   (Serial.available() || (millis() < maxTime)) 
             && !gps.location.isUpdated() && (gps.location.age() > 2000));

    yield();
    updateGpsData();

}   // delayGps()


void unfixGps() {
    char noFix[] = "$GPGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99*30";

    for (int p=0; noFix[p] == 0; p++) {
        gps.encode(noFix[p]);
    }
    updateGpsData();
    
}   // unfixGps()


void updateGpsData() {
    int32_t temp;
    //Serial.print("u");
    if (gps.satellites.isValid()) {
        gpsData.satellites      = gps.satellites.value();
        //Serial.printf("[%d]", gpsData.satellites);
    }
    if (gps.hdop.isValid()) {
        gpsData.hdop            = gps.hdop.value();
        //Serial.print("h");
    }
    if (gps.location.isValid()) {
        gpsData.lat = gps.location.lat();
        gpsData.lng = gps.location.lng();
        gpsData.locValid    = true;
    } else {
        gpsData.locValid    = false;
    }
    if (gps.date.isValid()) {
        gpsData.year            = gps.date.year();
        gpsData.month           = gps.date.month();
        gpsData.day             = gps.date.day();
    }
    if (gps.time.isValid()) {
        gpsData.hour            = gps.time.hour() + confTimeUTC;
        if (gpsData.hour >= 24) gpsData.hour -= 24; 
        gpsData.minute          = gps.time.minute();
        gpsData.second          = gps.time.second();
    }
    if (gps.altitude.isValid()) {
        gpsData.altitude        = gps.altitude.meters();
    }
    if (gps.course.isValid()) {
        gpsData.courseDeg       = gps.course.deg();
    }
    if (gps.speed.isValid()) {
        gpsData.speedKmph       = gps.speed.kmph();
    }
    gpsData.charsRX = gps.charsProcessed();
    
}   // updateGpsData()


void displayGpsData() {
    char sLat[10];
    char sLng[10];
    char sAlt[10];
    char sKmph[10];
    char noFix[] = "$GPGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99*30";

    if (gpsData.lat == 0.0 || gpsData.lng == 0.0) {
        //unfixGps();

        gpsData.lat     = gsmData.lat;
        gpsData.lng     = gsmData.lng;
        gpsData.year    = gsmData.year;
        gpsData.month   = gsmData.month;
        gpsData.day     = gsmData.day;
        gpsData.hour    = gsmData.hour;
        gpsData.minute  = gsmData.minute;
        gpsData.second  = gsmData.second;
    }

    dtostrf(gpsData.lat, 9, 6, sLat);
    dtostrf(gpsData.lng, 9, 6, sLng);

    Serial.printf("Sats: %2d hdop: %4d lat: %s lng: %s (age: %4d) ", gpsData.satellites, gpsData.hdop, sLat, sLng, gpsData.locationAge); 

    Serial.printf("Date: %d-%02d-%02d / %02d:%02d:%02d \n", gpsData.year, gpsData.month, gpsData.day, gpsData.hour, gpsData.minute, gpsData.second); 
    
}   // displayGpsData()


void initGpsData() {
        Serial.print("\nInit gpsData store ...");
        gpsData.satellites = 0;
        gpsData.hdop       = 9999;
        gpsData.year       = 1900;
        gpsData.month      = 1;
        gpsData.day        = 1;
        gpsData.hour       = 1;
        gpsData.minute     = 1;
        gpsData.second     = 1;
        gpsData.locationAge= 9999;
        gpsData.lat        = 0;
        gpsData.lng        = 0;
        gpsData.altitude   = 0;
        gpsData.courseDeg  = 0;
        gpsData.speedKmph  = -1.0;

        Serial.println("done!");

}   // initGpsData()


uint32_t distanceBetween2Points(float lat1, float long1, float lat2, float long2, uint16_t minDistance) {
    uint32_t distInM = 0;
    
    if (lat1 == 0.0 || long1 == 0.0 || lat2 == 0.0 || long2 == 0.0) {
        return true;
    }
    distInM = gps.distanceBetween(lat1, long1, lat2, long2);
    if (distInM >= (long)minDistance*1.0) {
        sprintf(cMsg, "%dm ", distInM);
        mqttComment.concat(cMsg);
        return distInM;
    }

    return 0;
    
}   // distanceBetween2Points()

    
bool getGpsFix(uint8_t minSats, uint32_t maxWait, bool showDetail) {

    uint32_t time2Fix;
    char     cLat[12], cLng[12], cElapse[12];

    time2Fix    = millis() + maxWait;
    debugGetFix    = showDetail;
    locAfterFix = true;
    digitalWrite(RED_LED, HIGH);
    //============= fill GSM buffer =================================
    while ((gpsData.lat == 0 || gpsData.lng == 0 || gpsData.satellites < minSats || !gpsData.locValid) && millis() < time2Fix) {
        startSoftWDT(((maxWait / 1000) + 5), "getGpsFix");
        Serial.print("Collecting GPS data ..");
        if (showDetail) Serial.println();
        gpsTaskTimer = millis() + _GPS_TASK_TIME;
        while (gpsTaskTimer > millis()) {
            digitalWrite(RED_LED, !digitalRead(RED_LED));
            digitalWrite(YLW_LED, !digitalRead(RED_LED));
            //Serial.print(".");
            yield();
            delayGps(500);
            yield();
            //sprintf(cMsg,"RX char %d", gpsData.charsRX);
            sprintf(cMsg,"RX %d/Sat %d", gpsData.charsRX, gpsData.satellites);
        }
        sprintf(cMsg," (RX %d chars/Sat %d) ", gpsData.charsRX, gpsData.satellites);
        Serial.println(cMsg);
        yield();
        debugGetFix = false;
        digitalWrite(RED_LED, LOW);
        digitalWrite(YLW_LED, LOW);

        stopSoftWDT();
    } 

    if (prevLat != 0 && prevLng != 0) {
        epoch   = makeTime( gpsData.hour, gpsData.minute, gpsData.second
                          , gpsData.day,  gpsData.month,  gpsData.year);
        distM   = gps.distanceBetween(prevLat, prevLng, gpsData.lat, gpsData.lng);
        if (distM > (confResMax * ((epoch - prevEpoch) +1))) {
            locAfterFix = false;
            return false;
        }
    }
    
    debugGetFix = false;
    if (time2Fix > millis() && (gps.charsProcessed() > prevCharsRX)) {
        Serial.println(">> got a FIX!");
        mqttComment.concat("Fix! ");
        elapseTime  = (millis() - bootTime) / 1000.0;
        if (firstFix) {
            firstFix = false;
            dtostrf(gpsData.lat, 9, 5, cLat);
            dtostrf(gpsData.lng, 9, 5, cLng);
            dtostrf(elapseTime, 5, 1, cElapse);
            sprintf(cMsg, "[%02d:%02d:%02d] '%s'\nFirstFix\nElapse T+%ssecond\n Lat %s\nLng %s" 
                                                , gsmData.hour, gsmData.minute, gsmData.second
                                                , confDevice.c_str()
                                                , cElapse
                                                , cLat, cLng );
            cMsg[139] = '\0';
            if (confSendSMS == 1) {
                if (modem.sendSMS(confNrToSMS, cMsg)) {
                    int p=0;
                    for (p=0; cMsg[p] > 0; p++) {
                        if (cMsg[p] == '\n') cMsg[p] = ' ';
                    }
                    cMsg[p++] = 0;
                    cMsg[p++] = 0;
                    Serial.printf("SMS send \n%s \nto %s!\n", cMsg, confNrToSMS.c_str());
                    writeLogFile(cMsg);
                } else {
                    Serial.println("Error sending SMS");
                }
            }

        }
        prevLat     = gpsData.lat;
        prevLng     = gpsData.lng;
        prevEpoch   = makeTime(gpsData.hour, gpsData.minute, gpsData.second
                                , gpsData.day,  gpsData.month,  gpsData.year);

        locAfterFix = true;
        return true;
    }

    locAfterFix = false;
    return false;

}   // getGpsFix()


void processGpsData(bool forceWrite) {

    uint8_t loopCount = 0;
    
    RSSI = modem.getSignalQuality();
    if (RSSI < prevRSSI && prevRSSI != 99) {
        sprintf(cMsg, "RSSI dropped from %d to %d", prevRSSI, RSSI);
        Serial.println(cMsg);
        if (RSSI < 10)  writeLogFile(cMsg);
    } else if (RSSI == 99) {
        sprintf(cMsg, "RSSI UnKnown (previous %d)", prevRSSI);
        Serial.println(cMsg);
        writeLogFile(cMsg);
    }
    prevRSSI = RSSI;

    delayGps(100);
    sprintf(cMsg, "Sats %2d hdop %3d", gpsData.satellites, gpsData.hdop);
    Serial.print(cMsg);
    dtostrf(gpsData.lat, 9, 6, r2c);
    sprintf(cMsg, "Lat: %s", r2c);
    Serial.printf(" %s", cMsg);
    dtostrf(gpsData.lng, 9, 6, r2c);
    sprintf(cMsg, "Lng: %s", r2c);
    Serial.printf(" %s", cMsg);
    Serial.printf(" age: %3d RSSI: %2d", gps.location.age(), RSSI);

    // is the distance greater than max distance???
    while (distM > (confResMax * ((epoch - prevEpoch) +1))) {
        getGpsFix(3, 1000, false);          // wait 1 seconds for a fix
        epoch   = makeTime( gpsData.hour, gpsData.minute, gpsData.second
                          , gpsData.day,  gpsData.month,  gpsData.year);
        distM = gps.distanceBetween(prevLat, prevLng, gpsData.lat, gpsData.lng);
        sprintf(cMsg, "D%dm > %dm ", distM, (confResMax * ((epoch - prevEpoch) +1)));
        Serial.printf(" %s", cMsg);
        loopCount++;
        if (loopCount > 5) {
            Serial.println("No valid GPS data! Abort");
            return;
        }
        yield();
    }   // while unlikely location
    
    sprintf(cMsg, "D%dm ", distM);
    Serial.printf(" %s", cMsg);
    mqttComment.concat(cMsg);
    
    sprintf(cMsg, "@%02d:%02d:%02d [%04d]", gpsData.hour, gpsData.minute, gpsData.second, ++sampleCount);

    Serial.printf(" %s ", cMsg);

    // now, if we have no valid fix from GPS and it is not a "forceWrite"
    // skip this location
    if (((prevCharsRX == gps.charsProcessed()) || gpsData.satellites < 2) && !forceWrite) {
        sprintf(cMsg, "No valid GPSdata (Sat's %d) ", gpsData.satellites);
        if (mqttComment.indexOf("No valid GPSdata") == -1) { 
            mqttComment.concat(cMsg);
            writeLogFile(cMsg);
            firstFix = true;
        }
        Serial.println(cMsg);

    } else {    // now we have a normal operation, so save location
        if (forceWrite) Serial.println("Force Saved");
        else            Serial.println("Saved");
        putGpsDataInBuffer();
        locationFromGsm = false;
    }

    prevLat     = gpsData.lat;
    prevLng     = gpsData.lng;
    prevEpoch   = epoch;
    prevCharsRX = gps.charsProcessed();
    
}   // processGpsData()

