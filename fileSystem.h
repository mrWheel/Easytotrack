/*
 * file syetem routines
 */

 #include "Easytotrack.h"


//------------------------------------------------------
// SPIFFS FSinfo
//------------------------------------------------------
void SPIFFSinfo() {
    FSInfo fs_info;
    SPIFFS.info(fs_info);

    Serial.printf("     Total bytes : %8d kB\n", (fs_info.totalBytes / 1024));
    Serial.printf("      Bytes used : %8d kB\n", ((fs_info.usedBytes/ 1024) + 1));
  //Serial.printf("      Block size : %5d\n", fs_info.blockSize);
  //Serial.printf("       Page size : %5d\n", fs_info.pageSize);
  //Serial.printf(" max. open files : %5d\n", fs_info.maxOpenFiles);
  //Serial.printf("max. path length : %5d\n", fs_info.maxPathLength);
    Serial.printf("      Bytes free : %8d kB\n", ((fs_info.totalBytes - fs_info.usedBytes) / 1024));
    
}   // SPIFFSinfo()

//------------------------------------------------------
// list files in datadir
//------------------------------------------------------
void lsFiles(bool fromRoot) {

    uint8_t fileNr = 0;
    uint8_t lenName;
    String space = "                           ";
    Dir dir;
    if (fromRoot)
            dir = SPIFFS.openDir("");
    else    dir = SPIFFS.openDir(_GPSLOGSROOT);

    //Serial.printf("List files on SPIFFS [%s]\n", _GPSLOGSROOT);
    while (dir.next()) {
        fileNr++;
        lenName = (25 - dir.fileName().length());
        //Serial.print(dir.fileName());
        File f = dir.openFile("r");
        //Serial.print(space.substring(0, (25 - dir.fileName().length())));
        if (fromRoot) {
            Serial.printf("[--] %s %s %d\n" , dir.fileName().c_str()
                                            , space.substring(0, lenName).c_str()
                                            , f.size());
        } else {
            Serial.printf("[%2d] %s %s %d\n", fileNr, dir.fileName().c_str()
                                            , space.substring(0, lenName).c_str()
                                            , f.size());
            
        }
    }
    if (fromRoot)
            Serial.printf("%d files in [/]\n", fileNr);
    else    Serial.printf("%d files in [%s]\n", fileNr, _GPSLOGSROOT);

}   // lsFiles()

//------------------------------------------------------
// cat file 
//------------------------------------------------------
void catFile(uint8_t catFileNr) {

    char        charF;
    char        SDbuffer[200];
    uint8_t     bSize;
    uint8_t     fileNr = 0;
    uint8_t     lenName;
    String      catName = "";
    String      space = "                           ";
    
    Dir dir = SPIFFS.openDir(_GPSLOGSROOT);

    while (dir.next()) {
        //Serial.println(dir.fileName());
        fileNr++;
        if (fileNr == catFileNr) {
            lenName = (25 - dir.fileName().length());
            catName = dir.fileName();
            File f = dir.openFile("r");
            //Serial.print(space.substring(0, (25 - dir.fileName().length())));
            Serial.printf("[%2d] %s %s %d\n\n", fileNr, dir.fileName().c_str()
                                            , space.substring(0, lenName).c_str()
                                            , f.size());
        }
    }

    if (catName.length() == 0) {
        Serial.printf("CAT: File [%d] not found\n", catFileNr);
    }
    File dataFile = SPIFFS.open(catName, "r");

    bSize = 0;
    charF       = '\0';
    memset(SDbuffer,0,sizeof(SDbuffer));

    while (dataFile.available()) {
        charF = (char)dataFile.read();
        if (charF == '\n' || charF == '\r' || charF == '\0' || bSize > 190) {
            if (bSize > 0) {
                Serial.println(SDbuffer);
            }
            bSize = 0;
            memset(SDbuffer,0,sizeof(SDbuffer));
        }
        else {
            if (charF >= 32 && charF <= 126) {
                SDbuffer[bSize++] = (char)charF;
            }
        }
        yield();
    }

    dataFile.close();    

}   // catFile()

//------------------------------------------------------
// rm file 
//------------------------------------------------------
void rmFile(uint8_t rmFileNr) {

    uint8_t     fileNr = 0;
    uint8_t     lenName;
    String      rmName;
    String      space = "                           ";
    
    Dir dir = SPIFFS.openDir(_GPSLOGSROOT);

    while (dir.next()) {
        //Serial.println(dir.fileName());
        fileNr++;
        if (fileNr == rmFileNr) {
            lenName = (25 - dir.fileName().length());
            rmName = dir.fileName();
            File f = dir.openFile("r");
            Serial.printf("\nDelete %s ", rmName.c_str());
            if (SPIFFS.remove(rmName)) {
                Serial.println(" OK");
            } else {
                Serial.println(" ERROR");
            }
            return;
        }
    }
    Serial.printf("DEL: File [%d] not found\n", rmFileNr);

}   // rmFile()


//------------------------------------------------------
// read the GpsDataFile
//------------------------------------------------------
void getGpsDataFromBuffer() {
    char        charF;
    char        SDbuffer[200];
    charF       = '\0';
    uint8_t     bSize;

    //Serial.println("<GPSDATA>");

    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    File dataFile = SPIFFS.open(_GPSDATAFILE, "r");

    bSize = 0;
    memset(SDbuffer,0,sizeof(SDbuffer));

    while (dataFile.available()) {
        charF = (char)dataFile.read();
        if (charF == '\n' || charF == '\r' || charF == '\0' || bSize > 190) {
            if (bSize > 0) {
                Serial.println(SDbuffer);
            }
            bSize = 0;
            memset(SDbuffer,0,sizeof(SDbuffer));
        }
        else {
            if (charF >= 32 && charF <= 126) {
                SDbuffer[bSize++] = (char)charF;
            }
        }
        yield();
    }

    dataFile.close();
    
}   // getGpsDataFromBuffer()


bool putGpsDataInBuffer() {
    if (putGpsDataInBuffer(false)) 
            return true;
    else    return false;
    
}   // putGpsDataInBuffer()


bool putGpsDataInBuffer(bool csvHeaders) {
    String  mqttStrng;
    String  mqttStrngLat    = "";
    String  mqttStrngLng    = "";
    bool    doCsvHeaders;
    char    logFileName[25];
    float   savLat, savLng;
    
    if (SPIFFS.exists(_GPSDATAFILE)) {
        doCsvHeaders = false;
    } else if (csvHeaders) {
        //Serial.println("putGpsDataInBuffer(): new logfile");
        doCsvHeaders = true;
    }

    mqttStrng   = buildMqttStrng();

//    if (   gpsData.lat < confResMin || gpsData.lat > confResMax 
//        || gpsData.lng < confLngMin || gpsData.lng > confLngMax ) {
//            return false;
//    }
    //Serial.println(mqttStrng);
    
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    File dataFile = SPIFFS.open(_GPSDATAFILE, "a");

    // if the file is available, write to it:
    if (dataFile && doCsvHeaders) {
        dataFile.println("\"IMEI\"; \"ID\"; \"Epoch\"; \"Latitude\"; \"Longtitude\"; \"Speed\"; " 
                         "\"Altitude\"; \"StrokePM\"; \"Satellites\"; \"Hdop\"; "
                         "\"Date\"; \"Time\";\"Comment\"");
        return true;
    }

    digitalWrite(YLW_LED, HIGH);
    if (dataFile) {
        dataFile.println(mqttStrng);    // Write to mqttBuffer
        if (locAfterFix) {              // to get a bliep on the track
            savLat          = gpsData.lat;
            savLng          = gpsData.lng;
            gpsData.lat    += 0.0005;
            mqttComment     = "Lat++";
            mqttStrngLat    = buildMqttStrng();
            gpsData.lat     = savLat;
            gpsData.lng    += 0.0005;
            mqttComment     = "Lng++";
            mqttStrngLng    = buildMqttStrng();
            gpsData.lng     = savLng;
            dataFile.println(mqttStrngLat); // Write plus Lat to mqttBuffer
            dataFile.println(mqttStrngLng); // Write plus Lng to mqttBuffer
            dataFile.println(mqttStrng);    // Write original to mqttBuffer
            locAfterFix     = false;
        }
        dataFile.close();
        locationInBuffer = true;
        if ((gpsData.year >= 2017) && (confLogIntern == 1)) {
            sprintf(logFileName, "%04d%02d%02d.dat", gpsData.year, gpsData.month, gpsData.day); 
            if (writeLogFile(logFileName, mqttStrng)) {
                if (mqttStrngLat.length() > 20) {
                    writeLogFile(logFileName, mqttStrngLat);
                    writeLogFile(logFileName, mqttStrngLng);
                    writeLogFile(logFileName, mqttStrng);
                }
                digitalWrite(YLW_LED, LOW);
                return true;
            }
            digitalWrite(YLW_LED, LOW);
            return false;
        }
        digitalWrite(YLW_LED, LOW);
        return true;
    }

    digitalWrite(YLW_LED, LOW);
    return false;
    
}   // putGpsDataInBuffer()


bool emptyGpsDataFile() {

    locationInBuffer = false;
    
    if (SPIFFS.remove(_GPSDATAFILE) ) 
            return true;
    else    return false;
    
}   // emptyGpsDataFile()


bool writeLogFile(String lineIn) {
    
    if (writeLogFile("", lineIn))
            return true;

    return false;
    
}   // writeLogFile(String)


bool writeLogFile(String fileName, String lineIn) {

    String logFileName;
    
    if (lineIn.length() == 0) return true;
    if (fileName.length() == 0) logFileName = String(_GPSLOGSROOT) + "/logFile.txt";
    else                        logFileName = String(_GPSLOGSROOT) + "/" + fileName;
    
    File logFile = SPIFFS.open(logFileName, "a");

    if (logFile) {
        if (fileName.length() == 0) 
                logFile.printf("[%02d:%02d:%02d] %s\n", gpsData.hour, gpsData.minute, gpsData.second, lineIn.c_str());
        else    logFile.printf("%s\n", lineIn.c_str());
        logFile.close();
        return true;
    }

    return false;
    
}   // writeLogFile(String, String)


int16_t getIntegerValue(File dataFile, uint32_t maxReadTime) {
    int16_t tempVal = 0;
    char    charF;
    bool    bNeg    = false;

    while (dataFile.available() && (millis() < maxReadTime)) {
        charF = dataFile.read();
        if (('0' <= charF) && (charF <= '9')) {
            tempVal = (tempVal * 10) + ((int)charF - 48);
        }
        if (charF == '-')   bNeg    = true;
        if ((charF == '\n' || charF == '\r')) break;
        yield();
    }
    if (bNeg)   tempVal = tempVal * -1;

    return tempVal;

}   // getIntegerValue()


String getStringValue(File dataFile, uint32_t maxReadTime) {
    String  tempVal = "";
    char    charF;
    
    while (dataFile.available() && (millis() < maxReadTime)) {
        charF = dataFile.read();
        if ((32 <= charF) && (charF <= 126)) {
            tempVal = tempVal + charF;
        }
        if (charF == '\n' || charF == '\r') break;
        yield();
    }

    return tempVal;

}   // getStringValue()

//------------------------------------------------------
// read the configFile
//------------------------------------------------------
bool readConfigFile(bool orgFile, bool verbose) {

    char        charF;
    bool        rightConfig, bNeg;
    uint32_t    maxReadTime;
    String      fileName;
    uint16_t    confVersion    = 0;

    rightConfig    = true;
    confDevice     = "";
    confHasSIM     = 0;
    confSendSMS    = 0;
    confAPN        = "";
    confAPN_User   = "";
    confAPN_Passwd = "";
    confBroker     = "";
    confDistance   = 0;
    confSample     = 0;
    confSend       = 0;
    confLogIntern  = 0;
    confTimeUTC    = 0;
    confResMin     = 0;
    confResMax     = 0;

    if (verbose) Serial.printf("\nreadConfigFile(): .. \n");
    yield();

    if (orgFile)    fileName = String(_CONFIGFILE);
    else            fileName = String(_CONFIGSAVE);

    maxReadTime = millis() + 60000; // max 60 seconds
    
    if (!SPIFFS.exists(fileName)) {
        Serial.printf("readConfigFile(): error opening [%s]\n", fileName.c_str());
        return false;
    }
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    File dataFile = SPIFFS.open(fileName, "r");

    // 1e: Config Version
    yield();
    confVersion = getIntegerValue(dataFile, maxReadTime);
    if (confVersion != _CONFIGVERSION) {
        rightConfig = false;
    }

    // 2e: read Device name
    yield();
    confDevice  = getStringValue(dataFile, maxReadTime);

    // 3e: Has SIM card
    confHasSIM  = getIntegerValue(dataFile, maxReadTime);

    // 4e: Send SMS on startup
    confSendSMS = getIntegerValue(dataFile, maxReadTime);

    // 5e: Nr to Send SMS to
    confNrToSMS = getStringValue(dataFile, maxReadTime);

    // 6e: read APN
    confAPN     = getStringValue(dataFile, maxReadTime);

    // 7e: read APN_User
    confAPN_User    = getStringValue(dataFile, maxReadTime);

    // 8e: read APN_Passwd
    confAPN_Passwd  = getStringValue(dataFile, maxReadTime);
    
    // 9e: read MQTT broker URL
    confBroker  = getStringValue(dataFile, maxReadTime);

    // 10e: read Distance Interval
    confDistance    = getIntegerValue(dataFile, maxReadTime);
    
    // 11e: read Sample Interval
    confSample  = getIntegerValue(dataFile, maxReadTime);
    
    // 12e: read Send to MQTT Interval
    confSend    = getIntegerValue(dataFile, maxReadTime);
    
    // 13e: log intern
    confLogIntern   = getIntegerValue(dataFile, maxReadTime);
    
    // 14e: read Time offset
    confTimeUTC     = getIntegerValue(dataFile, maxReadTime);
    
    // 15e/16e: read Resolutie Min/Max
    confResMin      = getIntegerValue(dataFile, maxReadTime);
    confResMax      = getIntegerValue(dataFile, maxReadTime);
    
    dataFile.close();

    // ==== sanity check!!! =====
    if (confDevice.length() < 3)    confDevice  = "UnknownTracker";
    if (confHasSIM < 0)             confHasSIM  = 0;
    if (confHasSIM > 0)             confHasSIM  = 1;
    if (confSendSMS < 0)            confSendSMS = 0;
    if (confSendSMS > 0)            confSendSMS = 1;
    if (confNrToSMS.length() < 10)  confNrToSMS = "";
    if (confBroker.length() < 3)    confBroker  = "test.mosquitto.org";
    if (confDistance <= 0)          confDistance = 0;
    if (confDistance > 0 && confDistance < 25)     
                                    confDistance = 25;
    if (confDistance > 1000)        confDistance = 1000;
    if (confSample < 5)             confSample  = 5;
    if (confSample > 60)            confSample  = 60;
    if (confSend < ((confSample * 2) +5))  
                                    confSend = (confSample * 2) + 5;
    if (confSend > 600)             confSend = 600;
    if (confLogIntern < 0)          confLogIntern = 0;
    if (confLogIntern > 0)          confLogIntern = 1;
    if (confTimeUTC < -9)           confTimeUTC = -9;            
    if (confTimeUTC >  9)           confTimeUTC =  9;            
    if (confResMin <  1 || confResMin > 50)     
                                    confResMin =   5;   // default 5 meter
    if (confResMax < 10 || confResMax > 100)     
                                    confResMax =  50;   // default 50 meter/sec. = 180Km/h


    if (verbose) {
        Serial.printf("       Device [%s]\n", confDevice.c_str());
        Serial.printf("      Has SIM [%d] (0=no, 1=yes)\n", confHasSIM);
        Serial.printf("     Send SMS [%d] (0=no, 1=yes)\n", confSendSMS);
        if (confSendSMS) 
            Serial.printf("    Nr.To SMS [%s] \n", confNrToSMS.c_str());
        Serial.printf("          APN [%s]\n", confAPN.c_str());
        Serial.printf("     APN User [%s]\n", confAPN_User.c_str());
        String stars = "********************************************";
        String shortStars;
        shortStars = stars.substring(0,confAPN_Passwd.length());
        if (confAPN_Passwd.length() == 0)
                Serial.printf("   APN Passwd []\n");
        else    Serial.printf("   APN Passwd [%s]\n", shortStars.c_str());
        Serial.printf("       Broker [%s]\n", confBroker.c_str());
        Serial.printf("     Distance [%d] (distance in meters)\n", confDistance);
        Serial.printf("       Sample [%d] (interval in seconds)\n", confSample);
        Serial.printf("         Send [%d] (interval in seconds)\n", confSend);
        Serial.printf("   Log Intern [%d] (0=no, 1=yes do)\n", confLogIntern);
        Serial.printf("          UTC [%d] offset\n", confTimeUTC);
        Serial.printf("Resolutie Min [%d] (meters)\n", confResMin);
        Serial.printf("Resolutie Max [%d] (meters/sec.) - 50m/s = 180 Km/h\n", confResMax);
        Serial.flush();
    }
    
    //SPIFFS.end();
    if (millis() > maxReadTime) {
        return false;
    }
    return rightConfig;
    
}   // readConfigFile()


bool writeConfigFile(bool orgFile) {

    String fileName;
    
    if (orgFile)    fileName = String(_CONFIGFILE);
    else            fileName = String(_CONFIGSAVE);

    if ((!orgFile) && confDevice.equals("UnknownDevice")) return false;
    
    Serial.printf("\nwriteConfigFile(%s): .. \n", fileName.c_str());
    yield();

    //Serial.printf("    Device [%s]\n", confDevice.c_str());
    //Serial.printf("    HasSIM [%d]\n", confHasSIM);
    //Serial.printf("       APN [%s]\n", confAPN.c_str());
    //Serial.printf("  APN User [%s]\n", confAPN_User.c_str());
    //Serial.printf("APN Passwd [%s]\n", confAPN_Passwd.c_str());
    //Serial.printf("    Broker [%s]\n", confBroker.c_str());
    //Serial.printf("    Sample [%04d]\n", confSample);
    //Serial.printf("      Send [%04d]\n", confSend);
    //Serial.printf("       UTC [%04d]\n", confTimeUTC);

    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    File dataFile = SPIFFS.open(fileName, "w");

    // if the file is available, write to it:
    if (dataFile) {
        dataFile.printf("%04d\n", _CONFIGVERSION);
        dataFile.printf("%s\n", confDevice.c_str());
        dataFile.printf("%04d\n", confHasSIM);
        dataFile.printf("%04d\n", confSendSMS);
        dataFile.printf("%s\n", confNrToSMS.c_str());
        dataFile.printf("%s\n", confAPN.c_str());
        dataFile.printf("%s\n", confAPN_User.c_str());
        dataFile.printf("%s\n", confAPN_Passwd.c_str());
        dataFile.printf("%s\n", confBroker.c_str());
        dataFile.printf("%04d\n", confDistance);
        dataFile.printf("%04d\n", confSample);
        dataFile.printf("%04d\n", confSend);
        dataFile.printf("%04d\n", confLogIntern);
        dataFile.printf("%04d\n", confTimeUTC);
        dataFile.printf("%04d\n", confResMin);
        dataFile.printf("%04d\n", confResMax);
        dataFile.printf("\n\n\n\n\n\n");
        dataFile.close();
    }
    // if the file isn't open, pop up an error:
    else {
        Serial.printf("writeConfigFile(): error opening [%s]", fileName.c_str());
        //SPIFFS.end();
        return false;
    }

    return true;

}   // writeConfigFile()


