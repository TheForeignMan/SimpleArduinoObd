

bool StringContains(const char* charArray, const char* subString)
{
  return strstr(charArray, subString) > 0;
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

