
#include "Easytotrack.h"

uint8_t tries;

void initGsmData() {
        Serial.print("\nInit gsmData store ...");
        gsmData.year       = 1900;
        gsmData.month      = 1;
        gsmData.day        = 1;
        gsmData.hour       = 1;
        gsmData.minute     = 1;
        gsmData.second     = 1;
        gsmData.lat        = 0;
        gsmData.lng        = 0;

        Serial.println("done!");

}   // initGsmData()


void mqttCallback(char* topic, byte* payload, unsigned int len) {
    // dummy, ik verwacht geen callback..
        
}   // mqttCallback()


bool modemReset() {

    Serial.print("Easytotrack 4a, reset MODEM        ");
    startSoftWDT(30, "modemReset()");
    
    yield();
    if (modem.factoryDefault()) {
        Serial.println(">>OK");
        tries = 0;
        return true;
    }
    Serial.println(">>FAIL");

    return false;
        
}   // modemReset()


bool modemRestart() {

    Serial.print("Easytotrack 4b, restart MODEM      ");
    startSoftWDT(30, "modemRestart()");

    yield();
    if (modem.restart()) {
        Serial.println(">>PASS");
        tries--;
        return true;
    }
    Serial.println(">>FAIL");
    return false;
        
}   // modemRestart()


bool modemInit() {

    Serial.print("Easytotrack 4c, init MODEM         ");
    startSoftWDT(30, "modemInit()");
    yield();
    if (modem.restart()) {
        Serial.println(">>PASS");
        tries--;
        return true;
    } 
    Serial.println(">>FAIL");
    return false;;
        
}   // modemInit()

bool modemJoinNET() {

    Serial.print("Easytotrack 4d, Connect to Network ");
    startSoftWDT(30, "modemJoinNET()");

    yield();

    if (modem.waitForNetwork()) {
         Serial.println(">>PASS");
         tries--;
         return true;
    } 
    Serial.println(">>FAIL");
    
    return false;
       
}   // modemJoinNET()


bool modemJoinAPN() {
    
    sprintf(cMsg, "Easytotrack 4e, Connect to APN [gprsConnect(\"%s\", \"%s\", \"%s\")]", confAPN.c_str(), confAPN_User.c_str(), confAPN_Passwd.c_str());
    Serial.println(cMsg);
    Serial.print("Easytotrack 4e, Connect to APN     ");
    startSoftWDT(30, "modemJoinAPN()");

    yield();
    if (modem.gprsConnect(confAPN.c_str(), confAPN_User.c_str(), confAPN_Passwd.c_str())) {
        Serial.println(">>PASS");
        return true;
    }
    Serial.println(">>FAIL");
    return false;

}   // modemJoinAPN()


bool modemSendToMQTT() {
    uint16_t ncCount = 0;
    int      fromPos = 0;
    int      toPos   = 16;
    int      dspLine = 1;
    char     charF   = 0;
    char     mqttStrng[200];
    uint8_t  bSize;
    bool     noErrorState = true;

    mqttClient.setServer(confBroker.c_str(), 1883);

    Serial.print("Easytotrack 4f, Send to MQTT ");
    startSoftWDT(30, "modemSendToMQTT()");
    yield();
    
    if (!mqttClient.connected()) {
        Serial.println(" ---> (re)Connect to MQTT ");
        //startSoftWDT(50, "modemSendToMQTT()-->(re)Connect2MQTT");
        stopSoftWDT();
        yield();
        if (mqttClient.connect(modemIMEI.c_str())) { 
            Serial.println(" ---> (re)Connect to MQTT >>PASS");
        } else { 
            Serial.println(" ---> (re)Connect to MQTT >>FAIL");
            return false;
        }
    } else {
        Serial.println("Easytotrack 4f, modemSendToMQTT(): is connected!");
    }
    
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    File dataFile = SPIFFS.open(_GPSDATAFILE, "r");

    bSize = 0;
    memset(mqttStrng,0,sizeof(mqttStrng));
    while (dataFile.available()) {
        charF = (char)dataFile.read();
        if (charF == '\n' || charF == '\r' || charF == '\0' || bSize > 190) {
            if (bSize > 0) {
                digitalWrite(YLW_LED, HIGH);
                String(mqttStrng).replace("  ", " ");
                Serial.printf("[1] => [%s]\n", mqttStrng);
                if (!mqttClient.publish("/RTracker/Data/", mqttStrng, false)) {
                    Serial.println("Easytotrack 4f, Seem to have lost connection with MQTT the server ...");
                    Serial.print("Easytotrack 4f, Reconnect");
                    mqttClient.disconnect();
                    if (mqttClient.connect(modemIMEI.c_str())) {
                        Serial.println("ed to the MQTT server!");
                        Serial.printf("[try 2] => [%s]\n", mqttStrng);
                        if (!mqttClient.publish("/RTracker/Data/", mqttStrng, false)) {
                            Serial.println("Easytotrack 4f, MQTT publish error!");
                            Serial.printf("[notSend] => [%s]\n", mqttStrng);
                            noErrorState = false;
                        } else {
                            Serial.println("Easytotrack 4f, data published");
                            bufferWasSend = true;
                        }
                    } else {
                        Serial.println(" error re-connecting to MQTT server!");
                        noErrorState = false;
                    }
                } else {
                    bufferWasSend = true;
                }
            }
            digitalWrite(YLW_LED, LOW);
            bSize = 0;
            memset(mqttStrng,0,sizeof(mqttStrng));
        }
        else {
            //-- skip all spaces ---
            if (charF >= 32 && charF <= 126) {
                mqttStrng[bSize++] = (char)charF;
            }
        }
        yield();
    }

    dataFile.close();
    
    return noErrorState;

}   // modemSendToMQTT()


void showModemState() {
    Serial.print("modemState is: ");
    switch(modemState) {
        case MODEM_UNKNOWN:     Serial.println("UNKNOWN");          break;
        case MODEM_RESTART:     Serial.println("RESTART");          break;
        case MODEM_INIT:        Serial.println("INIT");             break;
        case MODEM_TO_NET:      Serial.println("TO_NET");           break;
        case MODEM_TO_APN:      Serial.println("TO_APN");           break;
        case MODEM_CONNECT_TO_APN:   
                                Serial.println("CONNECT_TO_APN");   break;
      //case MODEM_CONNECT_TO_MQTT:   
      //                        Serial.println("CONNECT_TO_MQTT");  break;
        case MODEM_SEND_TO_MQTT:   
                                Serial.println("SEND_TO_MQTT");     break;
        default:                Serial.printf("uhh?? [%d]\n", modemState);
    }

}   // showModemState()


void GPRSconnect(uint8_t numTries, uint8_t doMode) {
    RegStatus mRS;
    tries = 0;
    bufferWasSend  = false;
    while ((modemState <= doMode) && (tries < numTries)) {
        // If, one way or an other, the Registration Status has decreased to 3 or 4
        // just return from this call and goback to the main loop
        // at the next write to MQTT we will check again
        mRS     = modem.getRegistrationStatus();
        RSSI    = modem.getSignalQuality();

        if ((modemState == MODEM_SEND_TO_MQTT) && (mRS == 3 || mRS == 4 || RSSI == 99)) {
            sprintf(cMsg, "modemRegistrationStatus %d", mRS);
            writeLogFile(cMsg);
            modemState = MODEM_TO_NET;  // probable best to re-connect to the APN
            return;
        }
        //Serial.printf("RegStatus is : %d\n", x);
        
        tries++;
        delayGps(10);

        switch (modemState) {
            case MODEM_UNKNOWN:     modemState = MODEM_RESET;
                                    tries--;
                                    break;
            
            case MODEM_RESET:       if (modemReset())       modemState = MODEM_RESTART;
                                    else                    modemState = MODEM_UNKNOWN;
                                    break;
            
            case MODEM_INIT:        if (modemInit())        modemState = MODEM_TO_NET;
                                    else                    modemState = MODEM_RESET;
                                    break;
            
            case MODEM_RESTART:     if (modemRestart())     modemState = MODEM_TO_NET;
                                    else                    modemState = MODEM_RESET;
                                    break;

            case MODEM_TO_NET:      if (modemJoinNET())     modemState = MODEM_TO_APN;
                                    else                    modemState = MODEM_INIT;
                                    break;
                                
            case MODEM_TO_APN:      if (modemJoinAPN())     modemState = MODEM_SEND_TO_MQTT;
                                    else                    modemState = MODEM_TO_NET;
                                    break;

            //case MODEM_CONNECT_TO_MQTT:   
            case MODEM_SEND_TO_MQTT:   
                                    if (modemSendToMQTT()) {modemState = MODEM_SEND_TO_MQTT; tries = 99; }
                                    else                    modemState = MODEM_TO_APN;
                                    break;

            default:                modemState = MODEM_UNKNOWN;
                                    tries = 0;
            
        }   // switch(modemState)
        stopSoftWDT();

    }   // while tries ...

}   // GPRSconnect()


