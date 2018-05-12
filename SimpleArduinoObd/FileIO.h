
File myFile;
File root;
char fileName[30] = {0};
const char entryFormat[] = "%02d/%02d/%02d,%02d:%02d:%02d,%s,%s,%05d,%04d,%04d"; // 55+ bytes
char entryArray[80] = {0};
char rootLocation[] = "/";
static bool isRootOpened = false;
Sd2Card card;
SdVolume volume;
SdFile root2;


////////////////////////
// FUNCTION DECLARATIONS
////////////////////////
void InitialiseSdCard(int chipSelect);
void SetFileName(byte year, byte month, byte day, byte hour);
bool WriteToFile(char* text, int textLength);
void ShowCardInfo();
uint32_t GetVolumeSize(Sd2Card sdCard, SdVolume vol);
void PrintCardContents();
uint32_t GetSizeOfAllFiles();
uint32_t GetSizeOfAllFiles(File dir);
void printDirectory(File dir, int numTabs);


///////////////////////
// FUNCTION DEFINITIONS
///////////////////////

// Initialises the SD card to prepare for writing to files,
// and reading card information.
void InitialiseSdCard(int chipSelect)
{
  if(!SD.begin(chipSelect))
  {
#if DEBUG
    Serial.print(F("SD initialisation failed."));
#endif
    while(1);
  }
  
  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, chipSelect)) 
  {
#if DEBUG
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    Serial.println("* is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
#endif
    while (1);
  } 
  else 
  {
#if DEBUG
    Serial.println("Wiring is correct and a card is present.");
#endif
  }
}


// Sets the file name for the text file.
// - year - byte with the year number
// - month - byte with the month number
// - day - byte with the day number
// - hour - byte with the hour number
void SetFileName(byte year, byte month, byte day, byte hour, byte minute)
{
  sprintf(fileName,
      "%02d%02d%02d%02d",
//      "%02d%02d%02d%02d%02d", // Setting the minutes in the filename seems to fail creating a file for some reason...
      year, month, day, hour, minute);

#if DEBUG
      Serial.print(F("File name: "));
      Serial.println(fileName);
#endif
}


// Writes text to file. 
// - text - char array containing the text to write
// - textLength - the length of array
bool WriteToFile(char* text, int textLength)
{
  myFile = SD.open(fileName, O_RDWR | O_CREAT | O_APPEND);
  if(myFile)
  {
    for(int i = 0; i < textLength; i++)
    {
      char c = *(text+i);
      if(c == 0 || c == '\0')
        break;
      myFile.write(*(text+i));
    }
    myFile.println();
    myFile.close();
    return true;
  }
  return false;
}


// Shows card info, such as card type and volume size, on the terminal.
// Only prints if the program is compiled in DEBUG mode.
void ShowCardInfo()
{
#if DEBUG
  // print the type of card
  Serial.println();
  Serial.print("Card type:         ");
  switch (card.type()) 
  {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) 
  {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    while (1);
  }

  Serial.print("Clusters:          ");
  Serial.println(volume.clusterCount());
  Serial.print("Blocks x Cluster:  ");
  Serial.println(volume.blocksPerCluster());

  Serial.print("Total Blocks:      ");
  Serial.println(volume.blocksPerCluster() * volume.clusterCount());
  Serial.println();

  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("Volume type is:    FAT");
  Serial.println(volume.fatType(), DEC);

  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1KB)
  Serial.print("Volume size (Kb):  ");
  Serial.println(volumesize);
  Serial.print("Volume size (Mb):  ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Gb):  ");
  Serial.println((float)volumesize / 1024.0);
#endif
}


// Gets the volume size of the SD card in KB, as this is more detailed than in
// MB or GB.
// - sdCard - the location of the SD card
// - vol - the location of the volume of said SD card
uint32_t GetVolumeSizeInKb(Sd2Card sdCard, SdVolume vol)
{
  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!vol.init(sdCard)) 
  {
#if DEBUG
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
#endif
    while (1);
  }
  uint32_t volumeSize = vol.blocksPerCluster();    // clusters are collections of blocks
  volumeSize *= vol.clusterCount();       // we'll have a lot of clusters
  volumeSize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1KB)

  return volumeSize;
}


// Print the contents of the SD card, including files/folders, date modified, 
// and file size (KB). To be used in DEBUG mode only.
void PrintCardContents()
{
#if DEBUG
  if (!volume.init(card)) 
  {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    while (1);
  }
  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  root2.openRoot(volume);

  // list all files in the card with date and size
  root2.ls(LS_R | LS_DATE | LS_SIZE);

  root2.close();
  Serial.println();
#endif
}


// Gets the total size of all files on the card. This will show how much
// space is used up by the files from the root directory.
uint32_t GetSizeOfAllFiles()
{
  if(isRootOpened)
  {
    root.rewindDirectory();
  }
  else
  {
    root = SD.open(rootLocation, FILE_READ);
    isRootOpened = true;
  }
  return GetSizeOfAllFiles(root);
}


// Gets the total size of all files in the folder 'dir'. This will show how much
// space is used up by the files from this directory.
// - dir - the folder location to get file details from.
uint32_t GetSizeOfAllFiles(File dir)
{
  uint32_t sizeOfAllFiles = 0;
  while (true) 
  {
    File entry = dir.openNextFile();
    if (! entry) 
    {
      // no more files
      entry.close();
      dir.rewindDirectory();
      return sizeOfAllFiles;
    }

    if (entry.isDirectory()) 
    {
      sizeOfAllFiles += GetSizeOfAllFiles(entry);
    }
    else
    {
      // files have sizes, directories do not
      sizeOfAllFiles += entry.size();
    }
    entry.close();
  }
}


// Another way of printing the contents of the SD card, but does not
// print dates. To be used in DEBUG mode only.
// - dir - directory from which to print files
// - numTabs - number of tab characters ('\t') to start off from
void printDirectory(File dir, int numTabs) 
{
  while (true) 
  {
    File entry =  dir.openNextFile();
    if (! entry) 
    {
      // no more files
      dir.rewindDirectory();
      break;
    }
    
    for (uint8_t i = 0; i < numTabs; i++) 
      Serial.print('\t');
    
    Serial.print(entry.name());
    if (entry.isDirectory()) 
    {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } 
    else 
    {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}
