/***************************************************************************
                          keys.h - Key Manipulation Functions

                             -------------------
    begin                : Fri Apr 21 2000
    copyright            : (C) 2000 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _keys_h_
#define _keys_h_

#include <stdio.h>
#include "ini.h"

//*******************************************************************************************************************
// Data structure definitions
//*******************************************************************************************************************
struct ini_t;
struct section_tag;

// Linked list structure for holding key information.
struct key_tag
{
    char           *key;
    long            pos;
    size_t          length;
    struct key_tag *pNext;
    struct key_tag *pPrev;

#ifdef INI_USE_HASH_TABLE
    unsigned long   crc;
    struct key_tag *pNext_Acc;
    struct key_tag *pPrev_Acc;
#endif // INI_USE_HASH_TABLE
};

static struct key_tag *__ini_addKey    (struct ini_t *ini, char *key);
static struct key_tag *__ini_faddKey   (struct ini_t *ini, FILE *file, long pos, size_t length);
static struct key_tag *__ini_createKey (struct ini_t *ini, char *key);
static void            __ini_deleteKey (struct ini_t *ini);
static struct key_tag *__ini_locateKey (struct ini_t *ini, const char *key);
static size_t __ini_averageLengthKey   (struct section_tag *current_h);

#endif // _keys_h_
