// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*  
**********************************************************************
*   Copyright (C) 2002-2016, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   file name:  uconfig.h
*   encoding:   UTF-8
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2002sep19
*   created by: Markus W. Scherer
*/

// MATCH WHAT WAS CONFIGURED DURING BUILD. See build_config.sh

#ifndef __UCONFIG_LOCAL_H__
#define __UCONFIG_LOCAL_H__

#define U_DISABLE_RENAMING 1
#define U_DEBUG 1
#define U_DEFAULT_SHOW_DRAFT 0
#define U_HIDE_DRAFT_API 1
#define UCONFIG_USE_LOCAL 1    // #include uconfig_local.h
#define U_NO_DEFAULT_INCLUDE_UTF_HEADERS 1
#define U_DISABLE_RENAMING 1
#define U_DEFAULT_SHOW_DRAFT 0
#define U_HIDE_DRAFT_API 1
#define U_HIDE_DEPRECATED_API 1
#define U_HIDE_OBSOLETE_UTF_OLD_H 1
#define UNISTR_FROM_STRING_EXPLICIT explicit  // Can not build test suites with this
#define UNISTR_FROM_CHAR_EXPLICIT explicit    // Can not build test suites with this
#define UCONFIG_NO_LEGACY_CONVERSION 1  // Only support UTF-7/8/16/32, CESU-8, SCSU, BOCU-1, US-ASCII & ISO-8859-1
//
// Default size of UnicodeString. This allows for 27 UTF-16 codeunits
// to be stored in the stack-allocated Unicodestring. Since Kodi
// uses UnicodeString for short-term, it may make sense to pump 
// this up to avoid heap usage.
//
//#define DUNISTR_OBJECT_SIZE 64
#define UNISTR_OBJECT_SIZE 256
#endif  // __UCONFIG_H__
