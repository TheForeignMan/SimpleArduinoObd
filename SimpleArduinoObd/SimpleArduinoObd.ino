// Connect to BT OBDII
// Check for connection
// Start getting data
#define DEBUG 1

#include <SD.h>
#include <SPI.h>
#include "FileIO.h"
#include "GpsHelper.h"
#include "ObdHelper.h"

#define SD_SS_PIN 53

#define DELAY 1000 // delay between commands in milliseconds

static bool sendToGraph = false;

unsigned long timer = 0;

// Reset function
void (* ResetFunction) (void) = 0;

void setup() 
{
  // Setup Comms
  Serial.begin(115200);
  // Setup SdCard
  InitialiseSdCard(SD_SS_PIN);
  // Setup Gps
  InitialiseGps();
  // Connect to OBD
  if(!InitBluetoothComms())
  {
#if DEBUG
    Serial.print(F("Failed to connect to OBD"));
#endif
    // Sometimes works if it is reset
    ResetFunction();
    while(1);
  }

#if DEBUG
  Serial.println(F("Bluetooth OK"));
  Serial.println(F("Waiting for GPS signal..."));
#endif
  // Wait for valid GPS signal
  while(!IsGpsDataAvailable(false));
  // Get GPS data
#if DEBUG
  Serial.println(F("Acquiring GPS data..."));
#endif
  GetGpsData(&currentGpsData);
  // Set file name
#if DEBUG
  Serial.println(F("Setting filename..."));
#endif
  SetFileName(
    currentGpsData.year, 
    currentGpsData.month, 
    currentGpsData.day, 
    currentGpsData.hour,
    currentGpsData.minute);

#if DEBUG
  Serial.println(F("Now let's rock and roll!"));
#endif

  timer = millis();
  
  // Bluetooth - ok
  // SD - ok
  // FileName - ok
  // Now start collecting data
}

void loop() 
{
  // Wait for valid GPS data - Should be much less than 1 sec
  while(!IsGpsDataAvailable(false));
  GetGpsData(&currentGpsData);
  
  // Get OBD data
  GetObdReadings(&currentObdReading);
  
  // Save to file
  memset(entryArray, 0, sizeof(entryArray));
  char latChar[10] = {0};
  dtostrf(currentGpsData.lat, 4, 6, latChar);
  char lonChar[10] = {0};
  dtostrf(currentGpsData.lon, 4, 6, lonChar);
  sprintf(entryArray, entryFormat,
    currentGpsData.day, currentGpsData.month, currentGpsData.year, 
    currentGpsData.hour, currentGpsData.minute, currentGpsData.second,
    latChar, lonChar, 
    currentObdReading.RPM, currentObdReading.Speed, currentObdReading.Throttle);

  bool entrySuccess = WriteToFile(entryArray, sizeof(entryArray));
#if DEBUG
  Serial.println(entryArray);
  if(entrySuccess)
    Serial.println("Entry recorded.");
  else
    Serial.println("Entry failed");
#endif
  
  // Wait until next log interval
  while(millis() - timer < DELAY);
  timer = millis();
}



