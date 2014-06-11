#include <Wire.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(5, 4);
Adafruit_GPS GPS(&mySerial);

uint8_t day_of_week(uint16_t y, uint8_t m, uint8_t d)
{
  static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
  y -= m < 3;
  return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

uint8_t dec_to_bcd(int val)
{
    return (uint8_t) ((val / 10 * 16) + (val % 10));
}

void setup()
{
  Wire.begin();
  Serial.begin(9600);
  Serial.println("DS3231 time setting from GPS");
  
  GPS.begin(9600);
  GPS.sendCommand("$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0*29");
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);
    
  delay(1000);
}

SIGNAL(TIMER0_COMPA_vect)
{
  char c = GPS.read();
}


void loop()
{
  char *nmea;
  uint8_t hour, minutes, seconds, month, day, dow;
  uint16_t year;
  
  if (!GPS.newNMEAreceived())
    return;
    
  nmea = GPS.lastNMEA();
    
  if (nmea[strlen(nmea)-4] == '*') {
    uint16_t sum = GPS.parseHex(nmea[strlen(nmea)-3]) * 16;
    sum += GPS.parseHex(nmea[strlen(nmea)-2]);
      
    for (uint8_t i = 1; i < (strlen(nmea)-4); i++)
      sum ^= nmea[i];
      
    if (sum != 0)
      return;
  } else
    return;
    
  if (strstr(nmea, "$GPZDA")) {
    //Serial.println(nmea);
    char *p = nmea;

    /* Read in time, we don't care about the fractional milliseconds */
    p = strchr(p, ',') + 1;
    uint32_t time = atol(p);
    hour = time / 10000;
    minutes = (time % 10000) / 100;
    seconds = time % 100;

    /* Read in day */
    p = strchr(p, ',') + 1;
    day = atoi(p);

    /* Read in month */
    p = strchr(p, ',') + 1;
    month = atoi(p);
 
    /* Read in year */
    p = strchr(p, ',') + 1;
    year = atoi(p);

    /* Calculate the day of the week */
    dow = day_of_week(year, month, day) + 1;  
  } else
    return;

  /* Program the DS3231 */
  Wire.beginTransmission(0x68);
  Wire.write(0x00);
  Wire.write(dec_to_bcd(seconds));
  Wire.write(dec_to_bcd(minutes));
  Wire.write(dec_to_bcd(hour));
  Wire.write(dec_to_bcd(dow));
  Wire.write(dec_to_bcd(day));
  Wire.write(dec_to_bcd(month) | ((year > 2000) ? 0x80 : 0x00));
  Wire.write(dec_to_bcd(year % 100));
  Wire.endTransmission();
    
  Serial.println("DS3231 programmed with current time");
}
