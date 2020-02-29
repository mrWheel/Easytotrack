

#include "quaternionFilters.h"
#include "MPU9250.h"

MPU9250 myIMU;

float       maxAv, actAv, minAv, centerAv;
bool        bAbCenter;

extern uint32_t     countStep;
extern float        stepsPM;
extern uint32_t     timeLaps;
extern uint32_t     timeStart;

float accelValue() {

    if (myIMU.readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01) {  
        myIMU.readAccelData(myIMU.accelCount);  // Read the x/y/z adc values
        myIMU.getAres();

        // Now we'll calculate the accleration value into actual g's
        // This depends on scale being set
        myIMU.ax = (float)myIMU.accelCount[0]*myIMU.aRes; // - accelBias[0];
        myIMU.ay = (float)myIMU.accelCount[1]*myIMU.aRes; // - accelBias[1];
        myIMU.az = (float)myIMU.accelCount[2]*myIMU.aRes; // - accelBias[2];
    } // if (readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01)

    return ((1000*(myIMU.ax * myIMU.ax)) + (1000*(myIMU.ay * myIMU.ay)) + (1000*(myIMU.az * myIMU.az)));

}   // accelValue()


void MPU9265setup() {
    byte        c = 0;

    delay(500); // even settelen
    // Read the WHO_AM_I register, this is a good test of communication
    c = myIMU.readByte(MPU9250_ADDRESS, WHO_AM_I_MPU9250);
    Serial.print("MPU9250 "); Serial.print("I AM "); Serial.print(c, HEX);
    Serial.print(" I should be "); Serial.println(0x71, HEX);

    if (c == 0x71) {  // WHO_AM_I should always be 0x71
        Serial.println("MPU9250 is online...");

        // Start by performing self test and reporting values
        myIMU.MPU9250SelfTest(myIMU.SelfTest);
        Serial.print("x-axis self test: acceleration trim within : ");
        Serial.print(myIMU.SelfTest[0],1); Serial.println("% of factory value");
        Serial.print("y-axis self test: acceleration trim within : ");
        Serial.print(myIMU.SelfTest[1],1); Serial.println("% of factory value");
        Serial.print("z-axis self test: acceleration trim within : ");
        Serial.print(myIMU.SelfTest[2],1); Serial.println("% of factory value");
        Serial.print("x-axis self test: gyration trim within : ");
        Serial.print(myIMU.SelfTest[3],1); Serial.println("% of factory value");
        Serial.print("y-axis self test: gyration trim within : ");
        Serial.print(myIMU.SelfTest[4],1); Serial.println("% of factory value");
        Serial.print("z-axis self test: gyration trim within : ");
        Serial.print(myIMU.SelfTest[5],1); Serial.println("% of factory value");

        // Calibrate gyro and accelerometers, load biases in bias registers
        myIMU.calibrateMPU9250(myIMU.gyroBias, myIMU.accelBias);

        myIMU.initMPU9250();
        // Initialize device for active mode read of acclerometer, gyroscope, and
        // temperature
        Serial.println("MPU9250 initialized for active data mode....");

        // Read the WHO_AM_I register of the magnetometer, this is a good test of
        // communication
        byte d = myIMU.readByte(AK8963_ADDRESS, WHO_AM_I_AK8963);
        Serial.print("AK8963 "); Serial.print("I AM "); Serial.print(d, HEX);
        Serial.print(" I should be "); Serial.println(0x48, HEX);

        // Get magnetometer calibration from AK8963 ROM
        myIMU.initAK8963(myIMU.magCalibration);
        // Initialize device for active mode read of magnetometer
        Serial.println("AK8963 initialized for active data mode....");
        //  Serial.println("Calibration values: ");
        Serial.print("X-Axis sensitivity adjustment value ");
        Serial.println(myIMU.magCalibration[0], 2);
        Serial.print("Y-Axis sensitivity adjustment value ");
        Serial.println(myIMU.magCalibration[1], 2);
        Serial.print("Z-Axis sensitivity adjustment value ");
        Serial.println(myIMU.magCalibration[2], 2);

    }   // if (c == 0x71)
    else {
        Serial.print("Could not connect to MPU9250: 0x");
        Serial.println(c, HEX);
        Serial.println("Power Off and then On to reboot MPU");
        while(1) { yield(); }; // Loop forever if communication doesn't happen
    }

    maxAv    = -10000.0;
    minAv    = 10000.0;
    centerAv = 0.0;
    countStep= 0;
    timeStart = millis();
    timeLaps  = 0;
    Serial.println("\nStart counting steps ...\n");

}   // MPU9265setup()


void MPU9265process(bool doVerbose) {
    // Print acceleration values in milligs!
    if (doVerbose) {
        Serial.print("X-acceleration: "); Serial.print(1000*myIMU.ax);
        Serial.print(" mg ");
        Serial.print("Y-acceleration: "); Serial.print(1000*myIMU.ay);
        Serial.print(" mg ");
        Serial.print("Z-acceleration: "); Serial.print(1000*myIMU.az);
        Serial.print(" mg ");
        Serial.println();
    }
    
    actAv = accelValue();

    timeLaps = millis() - timeStart;
    if (countStep >= 10) {
        stepsPM = (countStep * (60000.0 / timeLaps));
        if (doVerbose) {
           Serial.printf("\nSteps %2d", countStep);
            Serial.printf(" timeLap %5d", timeLaps);
            Serial.print(", S/min ");      Serial.print(stepsPM, 0);
            Serial.print(", actAv ");    Serial.print(actAv, 0);
            Serial.print(", minAv ");    Serial.print(minAv, 0);
            Serial.print(", maxAv ");    Serial.print(maxAv, 0);
            Serial.print(", centerAv "); Serial.print(centerAv, 0);
            Serial.print(", diffAv ");   Serial.print(abs(centerAv - actAv), 0);
            Serial.println();
            Serial.println();
        }
        countStep = 0;
        timeStart = millis();
    }
    if ((actAv > centerAv) && abs(actAv - centerAv) > (centerAv * 0.25) && !bAbCenter) {
        countStep++;
        bAbCenter = true;
        if (doVerbose) Serial.print(".");
        delay(10);
    }
    else if ((actAv < centerAv) &&  abs(centerAv - actAv) > (centerAv * 0.25) ) {
        bAbCenter = false;
        delay(10);
    }
    
    if (actAv > maxAv)  maxAv = actAv;
    if (actAv < minAv)  minAv = actAv;
    centerAv = (maxAv + minAv) / 2;

    if (minAv < 0)  minAv *= 0.9;
    else            minAv *= 1.1;
    if (maxAv < 0)  maxAv *= 1.1;
    else            maxAv *= 0.9;

    myIMU.count = millis();

}   // MPU9265process()

