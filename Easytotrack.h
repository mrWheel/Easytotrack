/*
**  R-Tracker defines and includes 
*/

#ifndef EASYTOTRACK_H
    #define EASYTOTRACK_H

    #include "unixTime.h"

/*** let op! in "TinyGSM-master/TinyGsmClientSIM800.h" 
 *  de volgende functie aangepast:
 *
 * String getgsmLocation() {
 *     sendAT(GF("+CIPGSMLOC=1,1"));
 * //  if (waitResponse(GF(GSM_NL "+CIPGSMLOC:")) != 1) {
 *     if (waitResponse(10000, GF(GSM_NL "+CIPGSMLOC:")) != 1) { // By AaW
 *         return "";
 *     }
 *     String res = stream.readStringUntil('\n');
 *     waitResponse();
 *     res.trim();
 *     return res;
 * }
***/
    #include <Ticker.h>
    // Select your modem:
    #define TINY_GSM_MODEM_SIM800
    //#define TINY_GSM_MODEM_SIM900
    //#define TINY_GSM_MODEM_A6
    //#define TINY_GSM_MODEM_M590
    #include <TinyGsmClient.h>
    
    //  LET OP! PubSubClient.h aangepast door AaW
    //          - MQTT_MAX_PACKET_SIZE 250  // was 100
    //          - MQTT_KEEPALIVE        30  // was  15
    //          - MQTT_SOCKET_TIMEOUT   30  // was  15
    #include <PubSubClient.h>
    
    // or Software Serial on ESP8266
    #include <EspSoftwareSerial.h>

    #define _SDA                4       // GPIO04, D2
    #define _SCL                5       // GPIO05, D1

    #define GSM_TX              13      // (v1: 14) (v3: 13)
    #define GSM_RX              12      // (v1: 13) (v3: 12)
    // Don't use GPIO02 for anything !!
    #define CONFIG_SWITCH_PIN   0
    #define RED_LED            16     // (LED1)
    #define YLW_LED            15     // (LED3)
    #define GRN_LED            14     // (LED2)

    #define _PROGVERSION    3.2
    #define _CONFIGVERSION  31
    #define _CONFIGFILE     "/Config/config.txt"
    #define _CONFIGSAVE     "/Config/config.sav"
    #define _GPSDATAFILE    "/Buffer/GPSdata.csv"
    #define _GPSLOGSROOT    "/Data"

    #define _GPS_TASK_TIME      2000
    #define _DIST_TASK_TIME     1000
    #define _MOTION_TASK_TIME   100
    #define _BLINK_TIME         0.5

    SoftwareSerial  SerialAT(GSM_RX, GSM_TX,  false, 1024);   // RX(D6), TX(D5)
    TinyGsm         modem(SerialAT);
    TinyGsmClient   client(modem);
    PubSubClient    mqttClient(client);
    
    Ticker tickObject;

    enum { MODEM_UNKNOWN, MODEM_RESET, MODEM_RESTART, MODEM_INIT, MODEM_TO_NET, MODEM_TO_APN, MODEM_CONNECT_TO_APN, MODEM_SEND_TO_MQTT };

    #define AHRS true       // Set to false for basic data read
    //MPU9250 myIMU;          // defined in main module

    uint16_t    sampleCount, sendCount;
    uint32_t    watchDogTimer, gpsTaskTimer, sampleTaskTimer, distTaskTimer, sendTaskTimer, blinkTaskTimer, motionTaskTimer;
    int32_t     heapSpace;
    String      modemIMEI;

    struct structGPS {
        uint8_t     satellites;
        uint16_t    hdop;
        uint32_t    epoch;
        uint16_t    year;
        uint8_t     month;
        uint8_t     day;
        uint8_t     hour;
        uint8_t     minute;
        uint8_t     second;
        uint32_t    locationAge;
        float       lat;
        float       lng;
        bool        locValid;
        float       altitude;
        float       courseDeg;
        float       speedKmph;
        uint32_t    charsRX;
    };   // structGPS

    struct structGSM {
        uint32_t    epoch;
        uint16_t    year;
        uint8_t     month;
        uint8_t     day;
        uint8_t     hour;
        uint8_t     minute;
        uint8_t     second;
        float       lat;
        float       lng;
    };   // structGSM
    
    volatile    structGPS   gpsData;
    volatile    structGSM   gsmData;
    bool        rebootFlag, softWDTtimeout;
    bool        hasGPSmodule, hasGPRSmodule, hasMPU9265, hasAK8963;
    bool        locationFromGsm, locationInBuffer, bufferWasSend;
    uint8_t     modemState;
    String      confDevice      = "UnknownTracker";
    byte        confHasSIM      = 0;
    String      confAPN         = "";
    String      confAPN_User    = "";
    String      confAPN_Passwd  = "";
    String      confBroker      = "test.mosquitto.org";
    uint16_t    confDistance    = 0;   // in meters
    uint16_t    confSample      = 10;
    uint16_t    confSend        = 30;
    uint16_t    confLogIntern   = 1;
    int8_t      confTimeUTC     = 0;
    int8_t      confSendSMS     = 0;
    String      confNrToSMS     = "";
    uint16_t    confResMin      = 0;
    uint16_t    confResMax      = 0;
    String      lastReset       = "";
    String      posSoftWDT      = "";
    String      gsmLocation     = "";
    String      mqttComment     = "";
    uint8_t     strokePM        = 0;
    bool        debugGetFix        = false;
    bool        firstFix        = true;
    bool        locAfterFix     = true;

    char        cMsg[250];
    char        r2c[50];
    
    uint32_t    prevCharsRX;        // characters received from GPS
    float       prevLat, prevLng;
    uint32_t    prevEpoch, epoch;
    int8_t      prevRSSI, RSSI;
    int32_t     distM;              // distance in Meters

    uint32_t    countStep;
    float       stepsPM;
    uint32_t    timeLaps;
    uint32_t    timeStart;
    uint32_t    bootTime;
    float       elapseTime;

    extern void signal(int);
    extern void delayGps(uint32_t);
    extern void initGpsData();
    extern void updateGpsData();
    extern void configMode();
    extern bool readConfigFile(bool, bool);
    extern bool writeLogFile(String, String);
    extern bool writeLogFile(String);
    extern bool putGpsDataInBuffer(bool);
    extern bool putGpsDataInBuffer();
    extern void getGpsDataFromBuffer();
    extern bool emptyGpsDataFile();
    extern String buildMqttStrng();
    extern void useGsmLocation();
    extern void stopSoftWDT();
    extern void startSoftWDT(uint32_t, String);
    //extern void MPU9265process(bool);
        
#endif

