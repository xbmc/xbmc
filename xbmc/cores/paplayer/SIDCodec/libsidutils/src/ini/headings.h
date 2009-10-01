/***************************************************************************
                          headings.h - Section Heading Manipulation Functions

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

#ifndef _headings_h_
#define _headings_h_

#include <stdio.h>
#include "ini.h"

//*******************************************************************************************************************
// Data structure definitions
//*******************************************************************************************************************
struct keys_tag;
struct ini_t;

// Linked list structure for holding section/heading information.
struct section_tag
{
    char                 *heading;
    struct   key_tag     *first;
    struct   key_tag     *last;
    struct   key_tag     *selected;
    struct   section_tag *pNext;
    struct   section_tag *pPrev;

#ifdef INI_USE_HASH_TABLE
    unsigned long         crc;
    struct   key_tag     *keys[256];
    struct   section_tag *pNext_Acc;
    struct   section_tag *pPrev_Acc;
#endif // INI_USE_HASH_TABLE
};

static struct section_tag *__ini_addHeading    (struct ini_t *ini, char *heading);
static struct section_tag *__ini_faddHeading   (struct ini_t *ini, FILE *file, long pos, size_t length);
static struct section_tag *__ini_createHeading (struct ini_t *ini, char *heading);
static void                __ini_deleteHeading (struct ini_t *ini);
static struct section_tag *__ini_locateHeading (struct ini_t *ini, const char *heading);

#endif // _headings_h_
