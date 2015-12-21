/*
 *      Copyright (C) 2015 Team MrMC
 *      https://github.com/MrMC
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with MrMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#import <CoreFoundation/CFString.h>
#import "platform/darwin/DictionaryUtils.h"


//------------------------------------------------------------------------------------------
Boolean GetDictionaryBoolean(CFDictionaryRef theDict, const void* key)
{
  // get a boolean from the dictionary
  Boolean value = false;
  CFBooleanRef boolRef;
  boolRef = (CFBooleanRef)CFDictionaryGetValue(theDict, key);
  if (boolRef != NULL)
    value = CFBooleanGetValue(boolRef);
  return value;
}
//------------------------------------------------------------------------------------------
long GetDictionaryLong(CFDictionaryRef theDict, const void* key)
{
  // get a long from the dictionary
  long value = 0;
  CFNumberRef numRef;
  numRef = (CFNumberRef)CFDictionaryGetValue(theDict, key);
  if (numRef != NULL)
    CFNumberGetValue(numRef, kCFNumberLongType, &value);
  return value;
}
//------------------------------------------------------------------------------------------
int GetDictionaryInt(CFDictionaryRef theDict, const void* key)
{
  // get a long from the dictionary
  int value = 0;
  CFNumberRef numRef;
  numRef = (CFNumberRef)CFDictionaryGetValue(theDict, key);
  if (numRef != NULL)
    CFNumberGetValue(numRef, kCFNumberIntType, &value);
  return value;
}
//------------------------------------------------------------------------------------------
float GetDictionaryFloat(CFDictionaryRef theDict, const void* key)
{
  // get a long from the dictionary
  int value = 0;
  CFNumberRef numRef;
  numRef = (CFNumberRef)CFDictionaryGetValue(theDict, key);
  if (numRef != NULL)
    CFNumberGetValue(numRef, kCFNumberFloatType, &value);
  return value;
}
//------------------------------------------------------------------------------------------
double GetDictionaryDouble(CFDictionaryRef theDict, const void* key)
{
  // get a long from the dictionary
  double value = 0.0;
  CFNumberRef numRef;
  numRef = (CFNumberRef)CFDictionaryGetValue(theDict, key);
  if (numRef != NULL)
    CFNumberGetValue(numRef, kCFNumberDoubleType, &value);
  return value;
}
//------------------------------------------------------------------------------------------
void CFDictionarySetSInt32(CFMutableDictionaryRef dictionary, CFStringRef key, SInt32 numberSInt32)
{
  CFNumberRef number;

  number = CFNumberCreate(NULL, kCFNumberSInt32Type, &numberSInt32);
  CFDictionarySetValue(dictionary, key, number);
  CFRelease(number);
}
//------------------------------------------------------------------------------------------
// helper function that inserts an double into a dictionary
void CFDictionarySetDouble(CFMutableDictionaryRef dictionary, CFStringRef key, double numberDouble)
{
  CFNumberRef number;

  number = CFNumberCreate(NULL, kCFNumberDoubleType, &numberDouble);
  CFDictionaryAddValue(dictionary, key, number);
  CFRelease(number);
}

void CFMutableDictionarySetData(CFMutableDictionaryRef dict, CFStringRef key, const uint8_t *value, int length)
{
  CFDataRef data = CFDataCreate(NULL, value, length);
  CFDictionarySetValue(dict, key, data);
  CFRelease(data);
}

void CFMutableDictionarySetObject(CFMutableDictionaryRef dict, CFStringRef key, CFTypeRef *value)
{
  CFDictionarySetValue(dict, key, value);
  CFRelease(value);
}

void CFMutableDictionarySetString(CFMutableDictionaryRef dict, CFStringRef key, const char *value)
{
  CFStringRef string = CFStringCreateWithCString(NULL, value, kCFStringEncodingASCII);
  CFDictionarySetValue(dict, key, string);
  CFRelease(string);
}
