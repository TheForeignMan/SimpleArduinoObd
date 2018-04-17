// Connect to BT OBDII
// Check for connection
// Start getting data

#include <SD.h>
#include <SoftwareSerial.h>
#include <SPI.h>

#define GRAPH_MODE 0
#define ENABLE_SD_LOG 1

#define BT_PWR 5
#define BT_KEY 2
#define BT_RX 4
#define BT_TX 3
#define SHOW_OUTPUT false
#define DELAY 1000 // delay between commands in milliseconds

#define MODE_01 0x0100
#define PIDS_AVAIL 0x00
#define ENGINE_RPM 0x0C
#define VEHICLE_SPEED 0x0D
#define ACCEL_POS 0x11

File myFile;
SoftwareSerial mySerial(BT_RX, BT_TX); // RX , TX

struct Reading
{
  uint32_t PIDs;    // 4 bytes
  uint16_t RPM;     // 2 bytes
  uint8_t Speed;    // 1 byte
  uint8_t Throttle; // 1 byte
} currentReading;   // Total size of struct = 8 bytes (same size as double)

uint32_t* pPid;
uint16_t* pRpm;
uint8_t* pSpeed;
uint8_t* pThrottle;

bool nextCommandReady = false;
bool responseReceived = false;
static bool sendToGraph = false;

// CHANGE THIS MAC ADDRESS TO OBDII BT MAC ADDRESS
const char obdMacAddress[] = "1D,A5,68988B";
const char cmdCheck[] = "AT";
const char cmdInit[] = "+INIT";
const char cmdPair[] = "+PAIR=";
const char cmdPairTimeout[] = ",10";
const char cmdPairCheck[] = "+FSAD=";
const char cmdConnect[] = "+LINK=";
const char cmdPids[] = "0100";
const char cmdRpm[] = "010C";
const char cmdSpeed[] = "010D";
const char cmdThrottle[] = "0111";
const char newLine[] = "\r\n";
const char btResponseOk[] = "OK";
const char btResponseError[] = "ERROR";
const char btResponseFail[] = "FAIL";
const char obdNoData[] = "NO DATA";

const byte arraySize = 30;
char cmdFull[arraySize] = {0};
char command[arraySize] = {0}; byte cmdIndex = 0;
char response[arraySize] = {0}; byte responseIndex = 0;
char mainResponse[arraySize] = {0}; byte mainResponseIndex = 0;

byte numberOfCmdsReceived = 0;
byte receivedCommands[8] = {0};

byte value = 0;
byte addition = 0;
byte* pValue = &value;
byte* pAddition = &addition;

bool startByteReceived = false;
bool pidByteReceived = false;
byte pidReceived = 0;
uint32_t responseData = 0;

int commandCount = 0;

unsigned long timer = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(BT_PWR, OUTPUT);
  digitalWrite(BT_PWR, LOW);
  pinMode(BT_KEY, OUTPUT);
  digitalWrite(BT_KEY, LOW);

  currentReading.PIDs = 0;
  currentReading.RPM = 0;
  currentReading.Speed = 0;
  currentReading.Throttle = 0;

  pPid = &currentReading.PIDs;
  pRpm = &currentReading.RPM;
  pSpeed = &currentReading.Speed;
  pThrottle = &currentReading.Throttle;
  
  Serial.begin(115200);
  mySerial.begin(38400);
  delay(100);
  Bluetooth(HIGH);
  nextCommandReady = true;

  while(mySerial.available() > 0)
  {
    mySerial.read();
  }

  if(!SD.begin(10))
  {
    SerialPrint(F("Initialisation failed."));
    while(1);
  }

  if(!ConnectToObdDevice())
  {
    SerialPrint(F("Failed to connect to OBDII module\r\n"));
    while(1);
  }

  // Done talking to BT module, no need to be in AT mode anymore
  digitalWrite(BT_KEY, LOW);
  
  Serial.print("O");

  delay(100);
  memset(cmdFull, 0, sizeof(cmdFull));
  strcat(cmdFull, cmdPids);
  strcat(cmdFull, newLine);
  SendToBluetooth(cmdFull, sizeof(cmdPids) + sizeof(newLine), SHOW_OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:

  timer = millis();

  // Check the main serial port for any commands.
  if(Serial.available() > 0)
  {
    while(Serial.available() > 0)
    {
      mainResponse[mainResponseIndex] = Serial.read();

      // Check for command to send data to PC graph program
      if((mainResponse[mainResponseIndex] == 'g') || (mainResponse[mainResponseIndex] == 'G'))
      {
        sendToGraph = !sendToGraph;
        memset(mainResponse, 0, sizeof(mainResponse));
        mainResponseIndex = 0;
        Serial.println(sendToGraph ? 1 : 0);
      }
    }
  }

  // We are only sending 3 commands - speed, RPM, throttle
  while(commandCount < 3)
  {
    // We're not interested in what's coming from the main UART bus, so no need to check that
    // We will check for any responses before sending anything over.
    if(mySerial.available() > 0)
    {
      while(mySerial.available())
      {
        response[responseIndex] = mySerial.read();
        //Serial.print(response[responseIndex]); // sends received response to uart
        // '>' marks end of response of OBD
        if(response[responseIndex] == '>') 
        {
          responseReceived = true;
        }
        responseIndex++;
        if(responseIndex >= arraySize)
        {
          responseIndex = arraySize - 1;
        }
      }
    }
  
    // Determine the response from the OBDII device when the full message has been received
    if(responseReceived)
    {
      nextCommandReady = true;

      // Check if the data return is valid. A response of "NO DATA" means that there is no data
      // and the car cannot provide the value requested. Ergo, don't check for values, and move on.
      if(StringContains(response, obdNoData))
      {
        responseReceived = false;
        memset(response, 0, arraySize);
        responseIndex = 0;
        continue;
      }
  
      // Each response we get is comprised of a series of 3 bytes:
      // 1. upper nibble
      // 2. lower nibble
      // 3. space character OR command terminator ('>')
      // Ergo, the number of command bytes received is the response index
      // divided by 3.
      numberOfCmdsReceived = (responseIndex - 4) / 3; 
      memset(receivedCommands, 0, sizeof(receivedCommands));
      int commandIndex = 0;
      *pValue = 0; 
      *pAddition = 0;
      // Convert the response from the OBD to actual numbers (instead of a char array)
      for(int i = 4; i < responseIndex; i++)
      {
        // Only get the numbers if they are part of a hex digit. If it is not a hex digit,
        // the command has been completed, so move on to the next character.
        if(IsCharHexDigit(response[i]))
        {
          *pValue = receivedCommands[commandIndex]; // first time round, this is 0. Second is upper nibble.
          *pAddition = response[i];
          if(response[i] >= '0' && response[i] <= '9')
            *pAddition -= 48;
          else if(response[i] >= 'A' && response[i] <= 'F')
            *pAddition -= 55;
          
          receivedCommands[commandIndex] = (*pValue << 4) + *pAddition;
        }
        else
        {
          commandIndex++;
          *pValue = 0; 
          *pAddition = 0;
        }
      }
  
      responseReceived = false;
  
      // The response is no longer necessary, so can be emptied.
      memset(response, 0, sizeof(response));
      responseIndex = 0;
  
      // Get the value from the response
      
      // Example command: 01 00
      // Example response: 41 00 BE 1F B8 10
      // 41 - Reponse for mode 01 (0x40 + 0x01)
      // 00 - Repeat of the PID number requested
      // Rest - data bytes
  
      // Example command: 01 0C  [request RPM]
      // Example response: 41 0C 1A F8
      // 41 - Response for mode 01
      // 0C - Response for RPM
      // 1A F8 - value 6904. Actual RPM: 6904 / 4 = 1726
      
      startByteReceived = false;
      pidByteReceived = false;
      pidReceived = 0;
      responseData = 0;
      for(int i = 1; i < numberOfCmdsReceived; i++)
      { 
        if(receivedCommands[i] == 0x41 && !startByteReceived)
        {
          startByteReceived = true;
          pidByteReceived = false;
        }
        else if(startByteReceived && !pidByteReceived)
        {
          pidReceived = receivedCommands[i];
          switch(pidReceived)
          {
            case PIDS_AVAIL: // 4 bytes return
              responseData = (responseData << 8) + receivedCommands[++i];
              responseData = (responseData << 8) + receivedCommands[++i];
            case ENGINE_RPM: // 2 bytes return
              responseData = (responseData << 8) + receivedCommands[++i];
            case VEHICLE_SPEED: // 1 byte return
            case ACCEL_POS: // 1 byte return
              responseData = (responseData << 8) + receivedCommands[++i];
              break;
            default:
              responseData = -1;
              break;
          }
          
          switch(pidReceived)
          {
            case PIDS_AVAIL:
              *pPid = responseData;
              break;
            case ENGINE_RPM:
              *pRpm = ((uint16_t)responseData) / 4;
              break;
            case VEHICLE_SPEED:
              *pSpeed = (uint8_t)responseData;
              break;
            case ACCEL_POS:
              *pThrottle = (uint8_t)responseData;
              break;
            default:
              break;
          }
  
          startByteReceived = false;
          pidByteReceived = false;
        }
      }

#if GRAPH_MODE
      if(commandCount > 1)
      {
        Serial.print("S");
        Serial.print(*pRpm);
        Serial.print(",");
        Serial.print(*pSpeed);
        Serial.print(",");
        Serial.print(*pThrottle);
        Serial.print("E");
      }
#elif ENABLE_SD_LOG
      if(commandCount > 1)
      {
        
      }
#else
      if(sendToGraph)
      {
        if(commandCount > 1)
        {
          Serial.print("S");
          Serial.print(*pRpm);
          Serial.print(",");
          Serial.print(*pSpeed);
          Serial.print(",");
          Serial.print(*pThrottle);
          Serial.print("E");
        }
      }
      else
      {
        switch(commandCount)
        {
          case 0:
            Serial.print(F("\tRPM: ")); Serial.print(*pRpm);
  //          WriteToFile(
            break;
          case 1:
            Serial.print(F("\tSpeed (kph): ")); Serial.print(*pSpeed);
            break;
          case 2:
            Serial.print(F("\tThrottle (%): ")); Serial.print(*pThrottle);
          default:
            Serial.print(" ");
            // Check time between OBD requests
  //          Serial.print(millis()); Serial.print(" ");
  //          Serial.print(timer); Serial.print(" ");
  //          Serial.print(millis() - timer);
  
            // Check available SRAM in the Arduino
  //          extern int __heap_start, *__brkval;
  //          int v;
  //          int ret = (int)&v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
  //          Serial.print(ret);
            
            Serial.println();
            break;
        }
      }
#endif
    }
  
    // Send the next command if the device is ready
    if(nextCommandReady)
    {
      switch(commandCount)
      {
        case 0:
          memset(cmdFull, 0, sizeof(cmdFull));
          strcat(cmdFull, cmdRpm);
          strcat(cmdFull, newLine);
          SendToBluetooth(cmdFull, sizeof(cmdRpm) + sizeof(newLine) - 2, SHOW_OUTPUT);
          break;
        case 1:
          memset(cmdFull, 0, sizeof(cmdFull));
          strcat(cmdFull, cmdSpeed);
          strcat(cmdFull, newLine);
          SendToBluetooth(cmdFull, sizeof(cmdSpeed) + sizeof(newLine) - 2, SHOW_OUTPUT);
          break;
        case 2:
          memset(cmdFull, 0, sizeof(cmdFull));
          strcat(cmdFull, cmdThrottle);
          strcat(cmdFull, newLine);
          SendToBluetooth(cmdFull, sizeof(cmdThrottle) + sizeof(newLine) - 2, SHOW_OUTPUT);
          break;
      }
      commandCount++;
    }
  }
  commandCount = 0;

  // Wait until the next turn
  while(millis() - timer < DELAY);
}

bool ConnectToObdDevice()
{
  bool deviceResponseOk = false;
  
  // Check that the device is in AT mode
  memset(cmdFull, 0, sizeof(cmdFull));
  strcat(cmdFull, cmdCheck);
  strcat(cmdFull, newLine);
  SendToBluetooth(cmdFull, sizeof(cmdCheck) + sizeof(newLine) - 2, SHOW_OUTPUT);
  deviceResponseOk = WaitForResponse(true, SHOW_OUTPUT); 

  if(!deviceResponseOk) 
  {
//    Serial.println(F("Failed to talk to BT module"));
    Serial.println(-1);
    return false;
  }
  
  delay(100);

  // Initialise the SPP library
  memset(cmdFull, 0, sizeof(cmdFull));
  strcat(cmdFull, cmdCheck);
  strcat(cmdFull, cmdInit);
  strcat(cmdFull, newLine);
  SendToBluetooth(cmdFull, sizeof(cmdCheck) + sizeof(cmdInit) + sizeof(newLine) - 3, SHOW_OUTPUT);
  deviceResponseOk = WaitForResponse(true, SHOW_OUTPUT);
  
  if(!deviceResponseOk) 
  {
//    Serial.println(F("Failed to initialise BT module"));
    Serial.println(-2);
    return false;
  }

  delay(100);

  // Pair with it
  memset(cmdFull, 0, sizeof(cmdFull));
  strcat(cmdFull, cmdCheck);
  strcat(cmdFull, cmdPair);
  strcat(cmdFull, obdMacAddress);
  strcat(cmdFull, cmdPairTimeout);
  strcat(cmdFull, newLine);
  SendToBluetooth(cmdFull, sizeof(cmdCheck) + sizeof(cmdPair) + sizeof(obdMacAddress) + sizeof(cmdPairTimeout) + sizeof(newLine) - 5, SHOW_OUTPUT);
  //WaitForResponse(true, SHOW_OUTPUT); delay(1000)
  deviceResponseOk = WaitForResponse(true, SHOW_OUTPUT);
  
  if(!deviceResponseOk) 
  {
//    Serial.println(F("Failed to pair with OBD device"));
    Serial.println(-3);
    return false;
  }
  
  delay(100);

  // Check it is in the paring list
  memset(cmdFull, 0, sizeof(cmdFull));
  strcat(cmdFull, cmdCheck);
  strcat(cmdFull, cmdPairCheck);
  strcat(cmdFull, obdMacAddress);
  strcat(cmdFull, newLine);
  SendToBluetooth(cmdFull, sizeof(cmdCheck) + sizeof(cmdPairCheck) + sizeof(obdMacAddress) + sizeof(newLine) - 4, SHOW_OUTPUT);
  deviceResponseOk = WaitForResponse(true, SHOW_OUTPUT);
  
  if(!deviceResponseOk) 
  {
//    Serial.println(F("MAC address not in pairing list"));
    Serial.println(-4);
    return false;
  }
  
  delay(100);

  // Connect to it.
  memset(cmdFull, 0, sizeof(cmdFull));
  strcat(cmdFull, cmdCheck);
  strcat(cmdFull, cmdConnect);
  strcat(cmdFull, obdMacAddress);
  strcat(cmdFull, newLine);
  SendToBluetooth(cmdFull, sizeof(cmdCheck) + sizeof(cmdConnect) + sizeof(obdMacAddress) + sizeof(newLine) - 4, SHOW_OUTPUT);
  deviceResponseOk = WaitForResponse(true, SHOW_OUTPUT);
  
  if(!deviceResponseOk) 
  {
//    Serial.println(F("Failed to connect to device"));
    Serial.println(-5);
    return false;
  }

  return deviceResponseOk;
}


void Bluetooth(bool enable)
{
  digitalWrite(BT_KEY, enable);
  delay(100);
  digitalWrite(BT_PWR, enable);
#if SHOW_OUTPUT
  Serial.print(F("Bluetooth "));
  if(enable)
    Serial.println(F("enabled"));
  else
    Serial.println(F("disabled"));
#endif
  delay(1000);
}

void SendToBluetooth(const char *pCommand, const int commandLength, bool printCommand)
{
  if(nextCommandReady || true)
  {
    memset(response, 0, sizeof(response));
    responseIndex = 0;
    if(printCommand)
    {
      Serial.print(F("=>"));
      for(int i = 0; i < commandLength; i++)
      {
        if((*(pCommand + i)) != '\r' || (*(pCommand + i)) != '\n')
          Serial.write(*(pCommand + i));
//        Serial.print(*(pCommand + i), HEX);
//        Serial.print(" ");
      }
      Serial.println();
      Serial.flush();
    }

    for(int i = 0; i < commandLength; i++)
    {
      mySerial.write(*(pCommand + i));
    }
    //mySerial.flush();
    nextCommandReady = false;
  }
  else
  {
    Serial.print(F("Not ready!["));
    for(int i = 0; i < commandLength; i++)
    {
      Serial.print(*(pCommand + i));
    }
    Serial.println(F("]"));
    Serial.flush();
  }
}

void CheckReceiverBuffer(bool showResponse)
{
  if(mySerial.available() > 0)
  {
    while(mySerial.available())
    {
      char charByte = mySerial.read();
      
      if(showResponse)
//        Serial.print(charByte, HEX);
        Serial.write(charByte);
        
      response[responseIndex] = charByte;
      if(response[responseIndex - 1] == '\r' || response[responseIndex - 2] == '\n')
      {
        //WriteToTerminal(response);
        responseReceived = true;
      }
      responseIndex++;
      if(responseIndex >= arraySize)
      {
        responseIndex = arraySize - 1;
      }
    }
  }
}

bool WaitForResponse(bool clearResponse)
{
  return WaitForResponse(clearResponse, false);
}

bool WaitForResponse(bool clearResponse, bool showResponse)
{
  while(!(responseReceived && 
        (StringContains(response, btResponseOk) || 
         StringContains(response, btResponseError) || 
         StringContains(response, btResponseFail)/* ||
         StringContains(response, F(">"))*/)))
  {
    CheckReceiverBuffer(showResponse);
  }

  bool responseValid = StringContains(response, btResponseOk)/* || 
     StringContains(response, F("ERROR")) || 
     StringContains(response, F("FAIL")) ||
     StringContains(response, F(">"))*/;
  
  if(clearResponse) 
  {
    memset(response, 0, sizeof(response));
    responseIndex = 0;
  }

  nextCommandReady = true;
  responseReceived = false; 

  if(responseValid) return true;
  else return false;
}


void SerialPrint(const __FlashStringHelper *ifsh)
{
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  size_t n = 0;
  while (1) {
    unsigned char c = pgm_read_byte(p++);
    if (c == 0) break;
    if (Serial.write(c)) n++;
    else break;
  }
}

void SerialPrint(char* text, int textLength)
{
#if SHOW_OUTPUT
  for(int i = 0; i < textLength; i++)
  {
    if(*(text+i) == '\0')
      break;
      
    Serial.print(*(text+i));
  }
#endif
}

static int lineCounter = 0;
static byte fileCounter = 0;
static char fileText[] = "OBD_File_XX.txt";
void WriteToFile(char* text, int textLength)
{
  // No need to constantly rename to the char array every 
  // time text is written to the file. Only do it when the
  // line counter is reset.
  if(lineCounter == 0)
  {
    fileText[9] = (fileCounter / 10) + 48;
    fileText[10] = (fileCounter % 10) + 48;
  }
  
  myFile = SD.open(fileText, FILE_WRITE);
  if(myFile)
  {
    for(int i = 0; i < textLength; i++)
      myFile.print(*(text+i));

    myFile.close();
    lineCounter++;
  }

  // 1200 lines at 1s per line equates to 20mins-worth of data per file
  if(lineCounter > 1200)
  {
    lineCounter = 0;
    fileCounter++;
  }
}


