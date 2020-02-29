//Function extracted from the library http://playground.arduino.cc/Code/Time
//if you just need the function without compiling few kbs and you need to save 
//space, this is the way to go

#ifndef UNIXTIME_H
    #define UNIXTIME_H


#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24UL)

// leap year calulator expects year argument as years offset from 1970
#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )
static  const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31}; // API starts months from 1, this array starts from 0

int32_t makeTime(int hr,int min,int sec,int dy, int mnth, int yr){   
    // assemble time elements into time_t 
    // note year argument is offset from 1970 (see macros in time.h to convert to other formats)
    // previous version used full four digit year (or digits since 2000),i.e. 2009 was 2009 or 9
  
    // year can be given as full four digit year or two digts (2010 or 10 for 2010);  
    //it is converted to years since 1970
    if( yr > 99)
          yr = yr - 1970;
    else
          yr += 30;  

    int i;
    uint32_t seconds;

    // seconds from 1970 till 1 jan 00:00:00 of the given year
    seconds= yr*(SECS_PER_DAY * 365);
    for (i = 0; i < yr; i++) {
        if (LEAP_YEAR(i)) {
            seconds +=  SECS_PER_DAY;   // add extra days for leap years
        }
    }
  
    // add days for this year, months start from 1
    for (i = 1; i < mnth; i++) {
        if ( (i == 2) && LEAP_YEAR(yr)) { 
            seconds += SECS_PER_DAY * 29;
        } else {
            seconds += SECS_PER_DAY * monthDays[i-1];  //monthDay array starts from 0
        }
    }
    seconds+= (dy-1) * SECS_PER_DAY;
    seconds+= hr * SECS_PER_HOUR;
    seconds+= min * SECS_PER_MIN;
    seconds+= sec;
    return (int32_t)seconds; 
    
}   // makeTime()

#endif


