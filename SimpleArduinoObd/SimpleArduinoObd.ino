// Connect to BT OBDII
// Check for connection
// Start getting data

#include <SoftwareSerial.h>

#define BT_PWR 5
#define BT_KEY 2
#define BT_RX 4
#define BT_TX 3
#define SHOW_OUTPUT false
#define DELAY 5000 // delay between commands in milliseconds

#define MODE_01 0x0100
#define PIDS_AVAIL 0x00
#define ENGINE_RPM 0x0C
#define VEHICLE_SPEED 0x0D
#define ACCEL_POS 0x11


SoftwareSerial mySerial(BT_RX, BT_TX); // RX , TX

struct Reading
{
  uint32_t PIDs;
  uint16_t RPM;
  uint8_t Speed;
  uint8_t Throttle;
} currentReading;

bool nextCommandReady = false;
bool responseReceived = false;

// CHANGE THIS MAC ADDRESS TO OBDII BT MAC ADDRESS
const char obdMacAddress[] = "1D,A5,68988B";
const char cmdCheck[] = "AT";
const char cmdInit[] = "+INIT";
const char cmdPair[] = "AT+PAIR=";
const char cmdPairTimeout[] = ",10";
const char cmdPairCheck[] = "AT+FSAD=";
const char cmdConnect[] = "AT+LINK=";
const char cmdPids[] = "0100";
const char cmdRpm[] = "010C";
const char cmdSpeed[] = "010D";
const char cmdThrottle[] = "0111";
const char newLine[] = "\r\n";
const char btResponseOk[] = "OK";
const char btResponseError[] = "ERROR";
const char btResponseFail[] = "FAIL";

char cmdFull[30] = {0};
char command[30] = {0}; byte cmdIndex = 0;
char response[30] = {0}; byte responseIndex = 0;

int timer = 0;

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
  
  Serial.begin(115200);
  mySerial.begin(38400);
  delay(100);
  Bluetooth(HIGH);
  delay(1000);
  nextCommandReady = true;

  while(mySerial.available() > 0)
  {
    mySerial.read();
  }

  if(!ConnectToObdDevice())
  {
    Serial.println(F("Failed to connect to OBDII module"));
    while(1);
  }

  // Done talking to BT module, no need to be in AT mode anymore
  digitalWrite(BT_KEY, LOW);

  delay(100);
  memset(cmdFull, 0, sizeof(cmdFull));
  strcat(cmdFull, cmdPids);
  strcat(cmdFull, newLine);
  SendToBluetooth(cmdFull, sizeof(cmdPids) + sizeof(newLine), SHOW_OUTPUT);
  
  timer = millis();
}

void loop() {
  // put your main code here, to run repeatedly:

  // We're not interested in what's coming from the main UART bus, so no need to check that
  // We will check for any responses before sending anything over.
  if(mySerial.available() > 0)
  {
    while(mySerial.available())
    {
      char charByte = mySerial.read();
      response[responseIndex] = charByte;
      if(response[responseIndex] == '>') // '>' marks end of response of OBD
      {
        //WriteToTerminal(response);
        responseReceived = true;
      }
      responseIndex++;
      
      Serial.write(charByte);
    }
  }

  // Determine the response from the OBDII device when the full message has been received
  if(responseReceived)
  {
    nextCommandReady = true;
    
    byte numberOfCmdsReceived = responseIndex / 3; 
    byte receivedCommands[numberOfCmdsReceived] = {0};
    int commandIndex = 0;

    Serial.print(F("Cmds: ")); Serial.print(numberOfCmdsReceived); Serial.print(" ");
    // Convert the response from the OBD to actual numbers (instead of a string)
    for(int i = 0; i < responseIndex; i++)
    {
//      if(/*response[i] == '>' || */response[i] == '\0')
//        break;
      
      if(IsCharHexDigit(response[i]))
      {
        byte value = receivedCommands[commandIndex];
        byte addition = response[i];
        if(response[i] >= '0' && response[i] <= '9')
          addition -= 48;
        else if(response[i] >= 'A' && response[i] <= 'F')
          addition -= 55;
          
        receivedCommands[commandIndex] = (value << 4) + addition;
      }
      else
      {
        //Serial.print(F("Char: ")); Serial.println(receivedCommands[commandIndex], HEX);
        commandIndex++;
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
    
    bool startByteReceived = false;
    bool pidByteReceived = false;
    byte pidReceived = 0;
    uint32_t responseData = 0;
    Serial.print(F("Char:")); 
    for(int i = 0; i < numberOfCmdsReceived; i++)
    {
      Serial.print(F(" ")); Serial.print(receivedCommands[i], HEX);
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
            Serial.print(F(" ")); Serial.print(receivedCommands[i], HEX);
            responseData = (responseData << 8) + receivedCommands[++i];
            Serial.print(F(" ")); Serial.print(receivedCommands[i], HEX);
          case ENGINE_RPM: // 2 bytes return
            responseData = (responseData << 8) + receivedCommands[++i];
            Serial.print(F(" ")); Serial.print(receivedCommands[i], HEX);
          case VEHICLE_SPEED: // 1 byte return
          case ACCEL_POS: // 1 byte return
            responseData = (responseData << 8) + receivedCommands[++i];
            Serial.print(F(" ")); Serial.print(receivedCommands[i], HEX);
            break;
          default:
            responseData = -1;
            break;
        }

        //Serial.print(F("Resp: ")); Serial.println(responseData);
        
        switch(pidReceived)
        {
          case PIDS_AVAIL:
            currentReading.PIDs = responseData;
            break;
          case ENGINE_RPM:
            currentReading.RPM = ((uint16_t)responseData) / 4;
            break;
          case VEHICLE_SPEED:
            currentReading.Speed = (uint8_t)responseData;
            break;
          case ACCEL_POS:
            currentReading.Throttle = (uint8_t)responseData;
            break;
          default:
            break;
        }

        startByteReceived = false;
        pidByteReceived = false;
      }
    }

    Serial.print(F("\tRPM: ")); Serial.print(currentReading.RPM);
    Serial.print(F("\tSpeed (kph): ")); Serial.print(currentReading.Speed);
    Serial.print(F("\tThrottle (%): ")); Serial.print(currentReading.Throttle);
    Serial.println();
  }

  // Send the next command if the device is ready
  if(nextCommandReady)
  {
    memset(cmdFull, 0, sizeof(cmdFull));
    strcat(cmdFull, cmdRpm);
    strcat(cmdFull, newLine);
    SendToBluetooth(cmdFull, sizeof(cmdRpm) + sizeof(newLine) - 2, SHOW_OUTPUT);
    
    memset(cmdFull, 0, sizeof(cmdFull));
    strcat(cmdFull, cmdSpeed);
    strcat(cmdFull, newLine);
    SendToBluetooth(cmdFull, sizeof(cmdSpeed) + sizeof(newLine) - 2, SHOW_OUTPUT);
    
    memset(cmdFull, 0, sizeof(cmdFull));
    strcat(cmdFull, cmdThrottle);
    strcat(cmdFull, newLine);
    SendToBluetooth(cmdFull, sizeof(cmdThrottle) + sizeof(newLine) - 2, SHOW_OUTPUT);
  }

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
  strcat(cmdFull, cmdPair);
  strcat(cmdFull, obdMacAddress);
  strcat(cmdFull, cmdPairTimeout);
  strcat(cmdFull, newLine);
  SendToBluetooth(cmdFull, sizeof(cmdPair) + sizeof(obdMacAddress) + sizeof(cmdPairTimeout) + sizeof(newLine) - 4, SHOW_OUTPUT);
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
  strcat(cmdFull, cmdPairCheck);
  strcat(cmdFull, obdMacAddress);
  strcat(cmdFull, newLine);
  SendToBluetooth(cmdFull, sizeof(cmdPairCheck) + sizeof(obdMacAddress) + sizeof(newLine) - 3, SHOW_OUTPUT);
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
  strcat(cmdFull, cmdConnect);
  strcat(cmdFull, obdMacAddress);
  strcat(cmdFull, newLine);
  SendToBluetooth(cmdFull, sizeof(cmdConnect) + sizeof(obdMacAddress) + sizeof(newLine) - 3, SHOW_OUTPUT);
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
  delay(1000);
  Serial.print(F("Bluetooth "));
  if(enable)
    Serial.println(F("enabled"));
  else
    Serial.println(F("disabled"));
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

bool IsCharHexDigit(char character)
{
  if((character >= '0' && character <= '9') ||
     (character >= 'A' && character <= 'F') ||
     (character >= 'a' && character <= 'f'))
    return true;
  else
    return false;
}

byte HexCharToDigit(char character)
{
  if(character >= '0' && character <= '9')
    return character - '0';
  else if(character >= 'A' && character <= 'F')
    return character - 'A' + 10;
  else if(character >= 'a' && character <= 'f')
    return character = 'a' + 10;
  else
    return 0;
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
      //Serial.print("Char received: ");
      //Serial.println();
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

bool StringContains(char* charArray, char* subString)
{
  return strstr(charArray, subString) > 0;
}
