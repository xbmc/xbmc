/*
 *  Copyright (C) 2015 Team MrMC
 *      https://github.com/MrMC
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
