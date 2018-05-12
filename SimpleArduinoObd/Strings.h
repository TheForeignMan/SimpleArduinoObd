

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

//#define ARDBUFFER 16
//int ardprintf(char *str, ...)
//{
//  int i, count=0, j=0, flag=0;
//  char temp[ARDBUFFER+1];
//  for(i=0; str[i]!='\0';i++)  if(str[i]=='%')  count++;
//
//  va_list argv;
//  va_start(argv, count);
//  for(i=0,j=0; str[i]!='\0';i++)
//  {
//    if(str[i]=='%')
//    {
//      temp[j] = '\0';
//      Serial.print(temp);
//      j=0;
//      temp[0] = '\0';
//
//      switch(str[++i])
//      {
//        case 'd': Serial.print(va_arg(argv, int));
//                  break;
//        case 'l': Serial.print(va_arg(argv, long));
//                  break;
//        case 'f': Serial.print(va_arg(argv, double));
//                  break;
//        case 'c': Serial.print((char)va_arg(argv, int));
//                  break;
//        case 's': Serial.print(va_arg(argv, char *));
//                  break;
//        default:  ;
//      };
//    }
//    else 
//    {
//      temp[j] = str[i];
//      j = (j+1)%ARDBUFFER;
//      if(j==0) 
//      {
//        temp[ARDBUFFER] = '\0';
//        Serial.print(temp);
//        temp[0]='\0';
//      }
//    }
//  };
//  Serial.println();
//  return count + 1;
//}


