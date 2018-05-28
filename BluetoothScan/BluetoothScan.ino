// Set module to master AT+ROLE=1
// Set module to connect to any address AT+CMODE=0
// Set module password to 1234 AT+PSWD=1234
// FROM NOW ON, KEY PIN HIGH ALWAYS!!
// Initialise the SPP lib AT+INIT
// Set module to look for devices with access code AT+IAC=9E8B33
// Set to look for the device type AT+CLASS=0
// Set enquiry parameters AT+INQM=1,9,48 (2nd param determines length of list [stop after 9 devices found])
// Get list of nearby devices AT+INQ
// Find name of devices AT+RNAME?1234,56,789ABC
// Set module to connect to one address AT+CMODE=1
// Bind module to slave address AT+BIND=1234,56,789ABC
// Pair module with slave AT+PAIR=1234,56,789ABC,10 (10s timeout should be ok)
// Check slave is in pair list AT+FSAD=1234,56,789ABC
// Connect to slave AT+LINK=1234,56,789ABC
// Disconnect once done AT+DISC

#define BT_PWR 19
#define BT_KEY 3
#define SHOW_OUTPUT true

bool nextCommandReady = false;

char newLine[] = "\r\n";

String * availableDevices;//[9] = {"","","","","","","","",""};

void setup() 
{
  // put your setup code here, to run once:
  pinMode(BT_PWR, OUTPUT);
  digitalWrite(BT_PWR, LOW);
  pinMode(BT_KEY, OUTPUT);
  digitalWrite(BT_KEY, LOW);
  
  Serial.begin(115200);
  Serial2.begin(38400);
  delay(100);
  Bluetooth(HIGH);
  delay(1000);
  nextCommandReady = true;
  AutoConnect();
  delay(1000);
  Serial.println("Enter OBD commands: ");
  nextCommandReady = true;
  delay(1000);
}

void Bluetooth(bool enable)
{
  digitalWrite(BT_KEY, enable);
  delay(100);
  digitalWrite(BT_PWR, enable);
  delay(1000);
  if(enable)
    Serial.println("Bluetooth enabled");
  else
    Serial.println("Bluetooth disabled");
}

bool responseReceived = false;
String command = "";
String response = "";
void loop() 
{
  // put your main code here, to run repeatedly:
  if(Serial.available() > 0)
  {
    while(Serial.available())
    {
      char charByte = Serial.read();
      command += charByte;
      if(command[command.length() - 2] == '\r' && command[command.length() - 1] == '\n')
      {
        SendToBluetooth(command, true);
        command = "";
      }
    }
  }

  if(Serial2.available() > 0)
  {
    while(Serial2.available())
    {
      char charByte = Serial2.read();
      response += charByte;
      if(response[response.length() - 1] == '>')
      {
        //WriteToTerminal(response);
        responseReceived = true;
      }
      
      Serial.write(charByte);
    }
  }

  if(responseReceived)
  {
    nextCommandReady = true;

    // Determine the response from the OBD
    for(int i = 0; i < response.length(); i++)
    {
      //if(i == response.length())
    }
    
    response = "";
  }

}


void SendToBluetooth(String command, bool printCommand)
{
  if(nextCommandReady)
  {
    response = "";
    if(printCommand)
    {
      Serial.print(F("Command sent: ")); Serial.println(command);
    }
    for(int i  = 0; i < command.length(); i++)
    {
      Serial2.write(command[i]);
    }
    Serial2.flush();
    nextCommandReady = false;
  }
  else
  {
    Serial.print("Not ready![");
    Serial.print(command);
    Serial.println("]");
  }
}

void WriteToTerminal(String response)
{
  Serial.println(response);
  response = "";
}

String * StringSplit(String response)
{
  // Count number of new lines/carriage returns
  int i = 0;
  int numberOfNewLines = 0;
  for (i = 0; i < response.length(); i++)
  { 
    response[i] == '\r' ? numberOfNewLines++ : i++;
  }
  Serial.print("Number of new lines = "); Serial.println(i);

  String returnArray[numberOfNewLines];

  bool carriageReturn = false;
  int newLine = 0;
  for(int j = 0; j < response.length(); j++)
  {
    if(response[j] == '\r')
    {
      carriageReturn = true;
    }
    else if(response[j] == '\n' && carriageReturn)
    {
      newLine++;
    }
    else
    {
      returnArray[newLine] += response[j];
    }
  }

  return returnArray;
}

bool StringContains(String str, String subString)
{
  return str.indexOf(subString) >= 0;
}


void AutoConnect()
{
  // Check that the device is in AT mode
  SendToBluetooth(F("AT\r\n"), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); delay(100);
  
  // Reset to original parameters
  SendToBluetooth(F("AT+ORGL\r\n"), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); delay(100);
  
  // Reset the device
  SendToBluetooth(F("AT+RESET\r\n"), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); delay(1000);
  
  // Check that the device is in AT mode
  SendToBluetooth(F("AT\r\n"), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); delay(100);

  // Set the UART properties of the module
  SendToBluetooth(F("AT+UART=115200,0,0\r\n"), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); delay(100);
  
  // Set the role to MASTER
  SendToBluetooth(F("AT+ROLE=1\r\n"), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); delay(100);

  // Set module to connect to any address (open channel)
  SendToBluetooth(F("AT+CMODE=0\r\n"), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); delay(100);

  // Set module password to 1234
  SendToBluetooth(F("AT+PSWD=1234\r\n"), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); delay(100);

  // From now on, the key pin MUST be held high!
  // Might already be high, but just in case...
  digitalWrite(BT_KEY, HIGH); delay(100);

  // Initialise the SPP library
  SendToBluetooth(F("AT+INIT\r\n"), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); delay(100);

  // Set module to look for device with access code 9E8B33
  SendToBluetooth(F("AT+IAC=9E8B33\r\n"), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); delay(100);

  // Set module to look for specific device type
  SendToBluetooth(F("AT+CLASS=0\r\n"), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); delay(100);

  // Set enquiry parameters
  SendToBluetooth(F("AT+INQM=1,3,48\r\n"), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); delay(100);

  // Get a list of nearby devices matching the criteria above (IAC)
  SendToBluetooth(F("AT+INQ\r\n"), true);
  WaitForResponse(false, true); delay(1000);
  
  // For each device nearby, check their name
  // Returned string for each device: +INQ:2:72:D2224,3E0104,FFBC
  // in the form of +INQ:<MAC address>,<DeviceType>,<RSSI>
  // EC88:92:98AF91 <= phone
  
  bool macAddressFlag = false;
  long macAddressLong = 0;
  char macAddress1[3][15] = {0};
  byte addressCountRow = 0;
  byte addressCountCol = 0;
  for(int j = 4; j < response.length(); j++)
  {
    if(response[j] == ':' &&
       response[j-1] == 'Q' &&
       response[j-2] == 'N' &&
       response[j-3] == 'I' &&
       response[j-4] == '+')
    {
      macAddressFlag = true;
    }
    else if(macAddressFlag)
    {
      if(response[j] == ',')
      {
        macAddressFlag = false;
        macAddress1[addressCountRow][addressCountCol] = 0;
        addressCountRow++;
        addressCountCol = 0;
        //Serial.println();
      }
      else if(response[j] == ':')
      {
        macAddress1[addressCountRow][addressCountCol] = ',';
        //Serial.print(macAddress1[addressCountRow][addressCountCol]);
        addressCountCol++;
      }
      else
      {
        macAddress1[addressCountRow][addressCountCol] = response[j];
        //Serial.print(macAddress1[addressCountRow][addressCountCol]);
        addressCountCol++;
      }
    }
  }

  if (addressCountRow == 0)
  {
    Serial.println("No Bluetooth devices found. Check there are devices available, and restart the Arduino.");
    while(1);
  }
  
  for(int i = 0; i < 3; i++)
  {
    // Now that the MAC address has been obtained, a name query can be made.
    Serial.println();
//    Serial.print("MAC Address: "); Serial.println(macAddress1[i]);

    // Now that the MAC address has been obtained, a name query can be made.
    // For some reason, the size of the array row must be displayed, otherwise
    // the name-query will fail. No idea why.
    /*Serial.print("Size of array: "); */Serial.print(sizeof(macAddress1[i]));
    //Serial.print("  Device "); delay(1000); Serial.print(i+1); delay(1000); Serial.println(" :");
    //Serial.flush();
    delay(1000);
    char commandName[] = "AT+RNAME?";
    strcat(commandName, macAddress1[i]);
    
    SendToBluetooth(strcat(commandName, newLine), true);
    WaitForResponse(true,true); delay(100);
  }

  response = "";
  char inputChar = 0;
  while (inputChar != '1' && inputChar != '2' && inputChar != '3')
  {
    Serial.println();
    Serial.print("Select the device to connect to: ");
    while(Serial.available() == 0);
    inputChar = Serial.read();
    while(Serial.available() != 0) Serial.read();
    Serial.println(inputChar);
  } 

  digitalWrite(BT_KEY, HIGH);

  Serial.print("Device selected: "); Serial.println(macAddress1[inputChar-49]);

  Bluetooth(LOW);
  delay(1000);
  Serial2.end();
  delay(500);
  Serial2.begin(38400);
  delay(500);
  Bluetooth(HIGH);
  delay(1000);

  // Check that the device is in AT mode
  SendToBluetooth(F("AT\r\n"), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); 
  Serial.println(F("Received from AT"));delay(1000);

  // Initialise the SPP library
  SendToBluetooth(F("AT+INIT\r\n"), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); delay(1000);

  // Set to 1
  SendToBluetooth(F("AT+CMODE=1\r\n"), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); delay(1000);

  char cmdFull[30] = {0};
  
  // Bind to one address
  char cmd0[] = "AT+BIND=";
  strcat(cmdFull, cmd0);
  strcat(cmdFull, macAddress1[inputChar-49]);
  SendToBluetooth(strcat(cmdFull, newLine), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); delay(1000);
  
  // Pair with it
  memset(cmdFull, 0, sizeof(cmdFull));
  char timeout[] = ",10";
  char cmd1[] = "AT+PAIR=";
  strcat(cmdFull, cmd1);
  strcat(cmdFull, macAddress1[inputChar-49]);
  strcat(cmdFull, timeout);
  SendToBluetooth(strcat(cmdFull, newLine), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); delay(1000);

  // Check it is in the paring list
  memset(cmdFull, 0, sizeof(cmdFull));
  char cmd2[] = "AT+FSAD=";
  strcat(cmdFull, cmd2);
  strcat(cmdFull, macAddress1[inputChar-49]);
  SendToBluetooth(strcat(cmdFull, newLine), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); delay(1000);

  // Connect to it.
  memset(cmdFull, 0, sizeof(cmdFull));
  char cmd3[] = "AT+LINK=";
  strcat(cmdFull, cmd3);
  strcat(cmdFull, macAddress1[inputChar-49]);
  SendToBluetooth(strcat(cmdFull, newLine), SHOW_OUTPUT);
  WaitForResponse(true, SHOW_OUTPUT); delay(1000);
}

void CheckReceiverBuffer(bool showResponse)
{
  if(Serial2.available() > 0)
  {
    while(Serial2.available())
    {
      char charByte = Serial2.read();
      response += charByte;
      if(response[response.length() - 2] == '\r' && response[response.length() - 1] == '\n')
      {
        //WriteToTerminal(response);
        responseReceived = true;
      }
      //Serial.print("Char received: ");
      if(showResponse)
        Serial.write(charByte);
      //Serial.println();
    }
  }
}

void WaitForResponse(bool clearResponse)
{
  WaitForResponse(clearResponse, false);
}

void WaitForResponse(bool clearResponse, bool showResponse)
{
  while(!(responseReceived && 
          (StringContains(response, F("OK")) || 
           StringContains(response, F("ERROR")) || 
           StringContains(response, F("FAIL")))))
  {
    //delay(1000);
    CheckReceiverBuffer(showResponse);
  }
  
  if(clearResponse) response = "";

  nextCommandReady = true;
}


