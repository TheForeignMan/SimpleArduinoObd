#include <TinyGPS.h>

// GPS variables
TinyGPS gps;
bool newData = false;
struct GpsData
{
  float lat;
  float lon;
  uint32_t timeRaw;
  byte second;
  byte minute;
  byte hour;
  uint32_t dateRaw;
  byte day;
  byte month;
  byte year;
};

GpsData currentGpsData;


////////////////////////
// FUNCTION DECLARATIONS
////////////////////////
void InitialiseGps();
bool IsGpsDataAvailable(bool showOutput);
void GetGpsData(GpsData* dest);


///////////////////////
// FUNCTION DEFINITIONS
///////////////////////

// Initialises the GPS module.
void InitialiseGps()
{
  Serial3.begin(9600);
  memset(&currentGpsData, 0, sizeof(GpsData));
}


// Waits until there is valid GPS data before returning true for yes.
bool IsGpsDataAvailable(bool showOutput)
{
  // For one second we parse GPS data and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (Serial3.available())
    {
      char c = Serial3.read();
      if (showOutput) 
      {
        Serial.write(c); // uncomment this line if you want to see the GPS data flowing
      }
      
      if(gps.encode(c))
      {
        return true;
      }
    }
  }
  return false;
}


// Gets the GPS data, only if valid.
void GetGpsData(GpsData* dest)
{
  float flat, flon;
  unsigned long age1;
  gps.f_get_position(&flat, &flon, &age1);
  unsigned long date, time, age2;
  gps.get_datetime(&date, &time, &age2);

  // Set new coordinates - Latitude
  if(flat != TinyGPS::GPS_INVALID_F_ANGLE)
    dest->lat = flat;
  
  // Set new coordinates - Longitude
  if(flon != TinyGPS::GPS_INVALID_F_ANGLE)
    dest->lon = flon;

  // Set the date
  if(date != TinyGPS::GPS_INVALID_DATE)
  {
    dest->dateRaw = date;
    // returned date is in format DDMMYY
    dest->year = date %100;
    dest->month = (date / 100) % 100;
    dest->day = date / 10000;
  }

  // Set the time
  if(time != TinyGPS::GPS_INVALID_TIME)
  {
    dest->timeRaw = time;
    // returned time is in format HHMMSScc (what's cc?)
    dest->hour = time / 1000000;
    dest->minute = (time / 10000) % 100;
    dest->second = (time / 100) % 100;
  }
}

