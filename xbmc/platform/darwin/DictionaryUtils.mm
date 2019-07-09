/*
 *  Copyright (C) 2015 Team MrMC
 *      https://github.com/MrMC
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "platform/darwin/DictionaryUtils.h"

#import <CoreFoundation/CFString.h>


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
