/***************************************************************************
                          headings.i - Section Heading Manipulation Functions

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headings.h"


/********************************************************************************************************************
 * Function          : __ini_addHeading
 * Parameters        : ini - pointer to ini file database.  heading - heading name
 * Returns           : Pointer to new heading.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Adds a new heading to the ini file database and updates the temporary workfile
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
struct section_tag *__ini_addHeading (ini_t *ini, char *heading)
{
    struct section_tag *section;
    size_t length;
    long   pos;

    // Format heading for storing
    __ini_strtrim (heading);

    /* Create a backup of the file we are about to edit */
    if (ini->heading == heading)
        return __ini_locateHeading (ini, heading);

    // Add new heading to work file and read it in
    // using file add
    fseek (ini->ftmp, 0, SEEK_END);
    fputs ("\n[", ini->ftmp);
    pos = ftell (ini->ftmp);
    fputs (heading, ini->ftmp);
    length  = (size_t) (ftell (ini->ftmp) - pos);
    section = __ini_faddHeading (ini, ini->ftmp, pos, length);
    fseek (ini->ftmp, 0, SEEK_END);
    fputs ("]\n", ini->ftmp);
    ini->heading = section->heading;
    return section;
}


/********************************************************************************************************************
 * Function          : __ini_faddheading
 * Parameters        : ini - pointer to ini file database,  file - ini file to read heading from
 *                   : pos - heading position in file, length - heading length
 * Returns           : Pointer to new heading.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Adds a new heading to the ini file database from the input file.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
struct section_tag *__ini_faddHeading (ini_t *ini, FILE *file, long pos, size_t length)
{
    struct section_tag *section;
    char  *str;

    if (length)
    {
        length++;
        str = (char *) malloc (sizeof(char) * length);
        assert (str);
        fseek  (file, pos, SEEK_SET);
        fgets  (str, (int) length, file);
        __ini_strtrim (str);
    }
    else
        str = "";

    section = __ini_createHeading (ini, str);
    // Make sure heading was created
    if (!section && length)
    {
        free (str);
        return NULL;
    }

    return section;
}


/********************************************************************************************************************
 * Function          : __ini_createHeading
 * Parameters        : ini - pointer to ini file database.  heading - heading name
 * Returns           : Pointer to new heading.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Adds an entry into the heading linked list ready for formating by the addHeading commands
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
struct section_tag *__ini_createHeading (ini_t *ini, char *heading)
{
    struct section_tag *pNew;

    pNew  = __ini_locateHeading (ini, heading);
    // Check to see if heading already exists
    if (pNew)
        free (heading);
    else
    {   // Create a new heading as dosen't exist
        pNew = (struct section_tag *) malloc (sizeof (struct section_tag));
        if (!pNew)
            return NULL;
        memset (pNew, 0, sizeof (struct section_tag));
        pNew->heading = heading;

        if (*heading)
        {   // Found a named heading
            pNew->pPrev = ini->last;
            ini->last   = pNew;
            if (pNew->pPrev)
                pNew->pPrev->pNext = pNew;
            else
                ini->first = pNew;
        }
        else
        {   // Anonymous heading (special case),
            // always comes first
            pNew->pNext = ini->first;
            ini->first  = pNew;
            if (pNew->pNext)
                pNew->pNext->pPrev = pNew;
            else
                ini->last = pNew;
        }

#ifdef INI_USE_HASH_TABLE
        {   // Rev 1.3 - Added
            struct   section_tag *pOld;
            unsigned long crc32;
            unsigned char accel;

            crc32     = __ini_createCrc32 (heading, strlen (heading));
            pNew->crc = crc32;
            // Rev 1.3 - Add accelerator list
            accel = (unsigned char) crc32 & 0x0FF;
            pNew->pPrev_Acc = NULL;
            pOld = ini->sections[accel];
            ini->sections[accel]      = pNew;
            if (pOld) pOld->pPrev_Acc = pNew;
            pNew->pNext_Acc           = pOld;
        }
#endif // INI_USE_HASH_TABLE
    }

    ini->selected = pNew;
    ini->changed  = true;
    return pNew;
}


/********************************************************************************************************************
 * Function          : __ini_deleteHeading
 * Parameters        : ini - pointer to ini file database.  heading - heading name
 * Returns           :
 * Globals Used      :
 * Globals Modified  :
 * Description       : Removes a heading from the database only.
 *                   : This change does not occur in the file until ini_close is called.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
void __ini_deleteHeading (ini_t *ini)
{
    struct section_tag *current_h;

    // Delete Heading
    current_h = ini->selected;
    if (current_h)
    {   // Delete Keys
        while (current_h->first)
        {
            current_h->selected = current_h->first;
            __ini_deleteKey (ini);
        }
    
        // Tidy up all users of this heading
        ini->selected =  NULL;
        if (ini->last == current_h)
            ini->last =  current_h->pPrev;

        // Break heading out of list
        if (!current_h->pPrev)
            ini->first = current_h->pNext;
        else
            current_h->pPrev->pNext = current_h->pNext;
        if (current_h->pNext)
            current_h->pNext->pPrev = current_h->pPrev;

#ifdef INI_USE_HASH_TABLE
        // Rev 1.3 - Take member out of accelerator list
        if (!current_h->pPrev_Acc)
            ini->sections[(unsigned char) current_h->crc & 0x0FF] = current_h->pNext_Acc;
        else
            current_h->pPrev_Acc->pNext_Acc = current_h->pNext_Acc;
        if (current_h->pNext_Acc)
            current_h->pNext_Acc->pPrev_Acc = current_h->pPrev_Acc;
#endif // INI_USE_HASH_TABLE

        // Delete Heading
        if (*current_h->heading)
            free (current_h->heading);
        free (current_h);
        ini->changed = true;
    }
}


/********************************************************************************************************************
 * Function          : __ini_locateHeading
 * Parameters        : ini - pointer to ini file database.  heading - heading name
 * Returns           : Pointer to heading.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Locates a heading entry in the database.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
struct section_tag *__ini_locateHeading (ini_t *ini, const char *heading)
{
    struct   section_tag *current_h;

#ifdef INI_USE_HASH_TABLE
    // Rev 1.3 - Revised to use new accelerator
    unsigned long crc32;
    crc32 = __ini_createCrc32 (heading, strlen (heading));

    // Search for heading
    for (current_h = ini->sections[(unsigned char) crc32 & 0x0FF]; current_h; current_h = current_h->pNext_Acc)
    {
        if (current_h->crc == crc32)
        {
            if (!strcmp (current_h->heading, heading))
                break;
        }
    }
#else
    // Search for heading
    for (current_h = ini->first; current_h; current_h = current_h->pNext)
    {
        if (!strcmp (current_h->heading, heading))
            break;
    }
#endif // INI_USE_HASH_TABLE

    ini->selected = current_h;
    return current_h;
}


/********************************************************************************************************************
 * Function          : (public) ini_deleteHeading
 * Parameters        : ini - pointer to ini file descriptor
 * Returns           : -1 for Error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Equivalent Microsoft write string API call where both data & key are set to NULL.
 ********************************************************************************************************************/
int INI_LINKAGE ini_deleteHeading (ini_fd_t fd)
{
    ini_t *ini = (ini_t *) fd;
    if (!ini->selected)
        return -1;
    // Can't delete a temporary section
    if (ini->selected == &(ini->tmpSection))
        return -1;
    __ini_deleteHeading (ini);
    return 0;
}


/********************************************************************************************************************
 * Function          : (public) ini_locateHeading
 * Parameters        : fd - pointer to ini file descriptor
 * Returns           : -1 for Error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Equivalent Microsoft write string API call where both data & key are set to NULL.
 ********************************************************************************************************************/
int INI_LINKAGE ini_locateHeading (ini_fd_t fd, const char *heading)
{
    ini_t *ini = (ini_t *) fd;
    struct section_tag *section;

    if (!heading)
        return -1;

    section = __ini_locateHeading (ini, heading);

#ifdef INI_ADD_LIST_SUPPORT
    // Rev 1.1 - Remove buffered list
    if (ini->list)
    {
        free (ini->list);
        ini->list = NULL;
    }
#endif // INI_ADD_LIST_SUPPORT

    if (section)
    {
        section->selected = NULL;
        return 0;
    }

    // Ok no section was found, but maybe the user is wanting to create a
    // new one so create it temporarily and see what actually happens later
    {   // Remove old heading
        section = &(ini->tmpSection);
        if (section->heading)
            free (section->heading);

        // Add new heading
        section->heading  = strdup (heading);
        if (!section->heading)
            return -1;
        section->selected = NULL;
        ini->selected     = section;
    }
    return -1;
}
