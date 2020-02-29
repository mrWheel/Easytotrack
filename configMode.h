/*
 * configMode.h
 */

#include "Easytotrack.h"

void configHelp() {
        
    Serial.println("Commando's:");
    Serial.println("     help                 (this screen)");
    Serial.println("     set device <name>");
    Serial.println("     set sim {0|1}         (0=no SIMcard, 1=has SIMcard)");
    Serial.println("     set SMS {0|1}         (0=don't send, 1=Send SMS after startup)");
    Serial.println("     set SMSnr <phone number> ");
    Serial.println("     set apn <provider>");
    Serial.println("     set user <apn user>");
    Serial.println("     set passwd <apn password>");
    Serial.println("     set broker <broker URL>");
    Serial.println("     set dist {10 .. 1000} (distance between samples in meters)");
    Serial.println("     set sample {5 .. 60}  (time between samples in seconds)");
    Serial.println("     set send {15 .. 600}  (time between sending to MQTT in seconds)");
    Serial.println("     set utc <{+|-}0..9>   (time difference from UTC)");
    Serial.println("     set log <0|1>         (0=don't log, 1=log intern)");
    Serial.println("     set ResMin <1..50>     (meter)");
    Serial.println("     set ResMax <10..1000>  (meters/sec.)");
    Serial.println("     info           (filesystem info)");
    Serial.println("     format SPIFFS  (format SPIFFS filesystem)");
    Serial.println("     ls             (list all datafiles)");
    Serial.println("     lsroot         (list all files from \"/\")");
    Serial.println("     cat <1..99>    (cat file)");
    Serial.println("     rm <1..99>     (remove file)");
    Serial.println("     Exit [exit's config mode]");

}   // configHelp()


void configMode() {
    String exitString, inputString = "";
    char cIn; 
    uint32_t maxTime = millis() + 600000;   // tien minuten

    if (!readConfigFile(true, true)) {
        Serial.println("Error reading config file..");
        confDevice      = "TrackerUnknown";
        confLogIntern   = 1;
        confHasSIM      = 0;
    }
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GRN_LED, LOW);
    
    Serial.println("\nConfig Mode ..");
    Serial.println("Don't forget to togle GPS<->DEBUG switch!");
    Serial.println("type 'help' for list of commando's");
    while (millis() < maxTime) {
        if (blinkTaskTimer < millis()) {
            blinkTaskTimer = millis() + 1000;
            digitalWrite(GRN_LED, !digitalRead(GRN_LED));
            digitalWrite(RED_LED, !digitalRead(GRN_LED));
        }

        yield();
        while (Serial.available() && (cIn != '\r' && cIn != '\n')) {
            yield();
            delay(10);
            char cIn = Serial.read();  //gets one byte from serial buffer
            if ( 32 <= cIn <= 126) {
                inputString += cIn; //makes the string inputString
                maxTime = millis() + 600000;    // reset tien minuten
            }
            if (millis() > maxTime) break;
        }
        yield();

        inputString.replace('\n', '\0');
        inputString.replace('\r', '\0');
        inputString.replace("  ", " ");

        yield();
        if (inputString.length() > 0) {
            yield();
            yield();
            //Serial.printf("input [%s]\n", inputString.c_str());     // see what was received
            int8_t idx1 = inputString.indexOf(' ');                 // finds location of first ,
            String command = inputString.substring(0, idx1);        // captures first data String
            command.toLowerCase();
            int8_t idx2 = inputString.indexOf(' ', idx1+1 );        // finds location of second ,
            String object = inputString.substring(idx1+1, idx2);    // captures second data String
            object.toLowerCase();
            String parm = inputString.substring(idx2+1);
            parm.replace(' ', '_');
            
            if (command.equals("exit")) {
                Serial.println("Exit configMode");
                Serial.println("Switch GPS<->DEBUG switch to GPS!!!");
                writeConfigFile(false);
                return;
            }
            //Serial.printf("\nCommand [%s], object [%s], parm [%s]\n", command.c_str(), object.c_str(), parm.c_str());

            if (command.equals("set")) {
                if (object.equals("device")) {
                    if (parm.length() > 20) confDevice = parm.substring(0, 20);
                    else                    confDevice = parm;
                    confDevice.replace('#', '_');
                    confDevice.replace('&', '_');
                    confDevice.replace('?', '_');
                    confDevice.replace('>', '-');
                    confDevice.replace('<', '-');
                    confDevice.replace('=', '-');
                    writeConfigFile(true);
                }
                if (object.equals("sim")) {
                    confHasSIM = parm.toInt();
                    if (confHasSIM > 0)   confHasSIM = 1;
                    writeConfigFile(true);
                }
                if (object.equals("sms")) {
                    confSendSMS = parm.toInt();
                    if (confSendSMS > 0)    confSendSMS = 1;
                    writeConfigFile(true);
                }
                if (object.equals("smsnr")) {
                    if (parm.length() > 20) confNrToSMS = parm.substring(0, 20);
                    else                    confNrToSMS = parm;
                    writeConfigFile(true);
                }
                if (object.equals("apn")) {
                    if (parm.length() > 20) confAPN = parm.substring(0, 20);
                    else                    confAPN = parm;
                    writeConfigFile(true);
                }
                if (object.equals("user")) {
                    if (parm.length() > 10) confAPN_User = parm.substring(0, 10);
                    else                    confAPN_User = parm;
                    writeConfigFile(true);
                }
                if (object.equals("passwd")) {
                    if (parm.length() > 10) confAPN_Passwd = parm.substring(0, 10);
                    else                    confAPN_Passwd = parm;
                    writeConfigFile(true);
                }
                if (object.equals("broker")) {
                    if (parm.length() > 200) confBroker = parm.substring(0, 200);
                    else                     confBroker = parm;
                    writeConfigFile(true);
                }
                if (object.equals("dist")) {
                    confDistance = parm.toInt();
                    if (confDistance <= 0)    confDistance = 0;
                    if (confDistance > 0 && confDistance < 25)     
                                              confDistance = 25;
                    if (confDistance > 1000)  confDistance = 1000;
                    writeConfigFile(true);
                }
                if (object.equals("sample")) {
                    confSample = parm.toInt();
                    if (confSample < 5)    confSample = 5;
                    if (confSample > 60)   confSample = 60;
                    writeConfigFile(true);
                }
                if (object.equals("send")) {
                    confSend = parm.toInt();
                    if (confSend < confSample) confSend = (confSample * 2) + 5;
                    if (confSend > 600)        confSend = 600;
                    writeConfigFile(true);
                }
                if (object.equals("log")) {
                    confLogIntern = parm.toInt();
                    if (confLogIntern > 0)   confLogIntern = 1;
                    writeConfigFile(true);
                }
                if (object.equals("utc")) {
                    confTimeUTC = parm.toInt();
                    if (confTimeUTC < -9)     confTimeUTC = -9;
                    if (confTimeUTC >  9)     confTimeUTC =  9;
                    writeConfigFile(true);
                }
                if (object.equals("resmin")) {
                    confResMin = parm.toInt();
                    if (confResMin <  1)     confResMin =   1;
                    if (confResMin > 50)     confResMin =  50;
                    writeConfigFile(true);
                }
                if (object.equals("resmax")) {
                    confResMax = parm.toInt();
                    if (confResMax <   10)   confResMax =  10;
                    if (confResMax >  100)   confResMax = 100;
                    writeConfigFile(true);
                }
                readConfigFile(true, true);
            } else if (command.equals("format")) {
                if (object.equals("spiffs")) {
                    Serial.println("This operation may take a while!");
                    Serial.print("Type \"OK\" to proceed .. ");
                    inputString = "";
                    while (millis() < maxTime) {
                        yield();
                        while (Serial.available() && (cIn != '\r' && cIn != '\n')) {
                            yield();
                            delay(10);
                            char cIn = Serial.read();  //gets one byte from serial buffer
                            if ( 32 <= cIn <= 126) {
                                inputString += cIn; //makes the string inputString
                                maxTime = millis() + 600000;    // reset tien minuten
                            }
                            if (millis() > maxTime) break;
                            if (cIn == '\r' || cIn == '\n') maxTime = millis();
                        }
                        yield();
                        if (cIn == '\r' || cIn == '\n') maxTime = millis();
                    }
                    inputString.replace('\n', '\0');
                    inputString.replace('\r', '\0');
                    Serial.println(inputString);
                    if (inputString.equals("OK")) {
                        Serial.print("Start formatting ..");
                        SPIFFS.format();
                        Serial.println(" Done!");
                    } else {
                        Serial.print("Cancelled formatting SPIFFS\n");
                    }
                    maxTime = millis() + 600000;    // reset tien minuten
                    writeConfigFile(true);
                }
            } else {
                if (command.equals("help")) {
                    configHelp();
                }
                if (command.equals("info")) {
                    readConfigFile(true, true);
                    Serial.println("\nSPIFFS:");
                    SPIFFSinfo();
                }
                if (command.equals("ls")) {
                    lsFiles(false);
                }
                if (command.equals("lsroot")) {
                    lsFiles(true);
                }
                if (command.equals("cat")) {
                    catFile(object.toInt());
                }
                if (command.equals("rm")) {
                    rmFile(object.toInt());
                    lsFiles(false);
                }
                if (command.length() == 0) {
                    readConfigFile(true, true);
                }

            }
            inputString  = "";
            // process string()
            Serial.println("\nType 'help' for list of commando's\n");
        }
    }
  
 }  // configMode()
 

