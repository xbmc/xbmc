#pragma once

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

#import <CoreFoundation/CFNumber.h>
#import <CoreFoundation/CFDictionary.h>

Boolean   GetDictionaryBoolean(CFDictionaryRef theDict, const void* key);
long      GetDictionaryLong(CFDictionaryRef theDict, const void* key);
int       GetDictionaryInt(CFDictionaryRef theDict, const void* key);
float     GetDictionaryFloat(CFDictionaryRef theDict, const void* key);
double    GetDictionaryDouble(CFDictionaryRef theDict, const void* key);

void      CFDictionarySetSInt32(CFMutableDictionaryRef dictionary, CFStringRef key, SInt32 numberSInt32);
void      CFDictionarySetDouble(CFMutableDictionaryRef dictionary, CFStringRef key, double numberDouble);
void      CFMutableDictionarySetData(CFMutableDictionaryRef  dict, CFStringRef key, const uint8_t *value, int length);
void      CFMutableDictionarySetObject(CFMutableDictionaryRef dict, CFStringRef key, CFTypeRef *value);
void      CFMutableDictionarySetString(CFMutableDictionaryRef dict, CFStringRef key, const char *value);
