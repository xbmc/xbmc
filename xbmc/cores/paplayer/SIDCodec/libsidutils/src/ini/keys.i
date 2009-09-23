/***************************************************************************
                          keys.i - Key Manipulation Functions

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
#include "keys.h"

#define INI_EQUALS_ALIGN 10


/********************************************************************************************************************
 * Function          : __ini_addKey
 * Parameters        : ini - pointer to ini file database.  key - key name
 * Returns           : Pointer to key.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Adds a new key to the ini file database and updates the temporary workfile.
 *                   : A heading operation must be called before this function.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
struct key_tag *__ini_addKey (ini_t *ini, char *key)
{
    struct key_tag *_key;
    size_t length;
    long   pos;

    // Format heading for storing
    __ini_strtrim (key);
    if (!*key)
        return NULL;

    // Add new key to work file and read it in
    // using file add
    fseek (ini->ftmp, 0, SEEK_END);
    pos = ftell (ini->ftmp);
    fputs (key, ini->ftmp);
    length = (size_t) (ftell (ini->ftmp) - pos);
    _key   = __ini_faddKey (ini, ini->ftmp, pos, length);
    fseek (ini->ftmp, 0, SEEK_END);
    fputc ('=', ini->ftmp);
    return _key;
}


/********************************************************************************************************************
 * Function          : __ini_averageLengthKey
 * Parameters        : current_h - pointer to current header.
 * Returns           : Returns the average key length
 * Globals Used      :
 * Globals Modified  :
 * Description       : Finds average key length for aligning equals.
 ********************************************************************************************************************/
size_t __ini_averageLengthKey (struct section_tag *current_h)
{
#ifdef INI_EQUALS_ALIGN
    size_t equal_pos, equal_max, keylength;
    size_t average = 0, count = 0;
    struct key_tag *current_k;
   
    // Rev 1.1 Added - Line up equals characters for keys
    // Calculate Average
    current_k = current_h->first;
    while (current_k)
    {
        count++;
        average  += strlen (current_k->key);
        current_k = current_k->pNext;
    }

    if (!count)
        return 0;

    average  /= count;
    equal_pos = (equal_max = average);

#if INI_EQUALS_ALIGN > 0
    // Work out the longest key in that range
    current_k = current_h->first;
    while (current_k)
    {
        keylength = strlen (current_k->key);
        equal_max = average + INI_EQUALS_ALIGN;

        if ((equal_max > keylength) && (keylength > equal_pos))
            equal_pos = keylength;
        current_k = current_k->pNext;
    }
#endif // INI_EQUALS_ALIGN > 0
    return equal_pos;
#else
    return 0;
#endif // INI_EQUALS_ALIGN
}


/********************************************************************************************************************
 * Function          : __ini_faddKey
 * Parameters        : ini - pointer to ini file database,  file - ini file to read key from
 *                   : pos - key position in file, length - key length
 * Returns           : Pointer to key.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Adds a new key to the ini file database from the input file.
 *                   : A heading operation must be called before this function.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
struct key_tag *__ini_faddKey (ini_t *ini, FILE *file, long pos, size_t length)
{
    struct key_tag *_key;
    char  *str;

    length++;
    str = (char *) malloc (sizeof(char) * length);
    assert (str);
    fseek  (file, pos, SEEK_SET);
    fgets  (str, (int) length, file);
    __ini_strtrim (str);

    _key = __ini_createKey (ini, str);
    if (!_key)
    {   free (str);
        return NULL;
    }

    _key->pos = pos + (long) length;
    return _key;
}


/********************************************************************************************************************
 * Function          : __ini_createKey
 * Parameters        : ini - pointer to ini file database.  key - key name
 * Returns           : Pointer to key.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Adds an entry into the key linked list ready for formating by the addKey commands
 *                   : A heading operation must be called before this function.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
struct key_tag *__ini_createKey (ini_t *ini, char *key)
{
    struct section_tag *section;
    struct key_tag     *pNew;
    long   pos;

    if (!*key)
        return NULL;

    section = ini->selected;
    pNew    = __ini_locateKey (ini, key);
    if (pNew)
    {   // Reset details of existing key
        free (pNew->key);
        pNew->key = key;
        pos       = 0;
    }
    else
    {   // Create a new key and add at end;
        pNew = (struct key_tag *) malloc (sizeof (struct key_tag));
        if (!pNew)
            return NULL;
        memset (pNew, 0, sizeof (struct key_tag));
        pNew->key = key;

        if (!section->first)
            section->first = pNew;
        else
            section->last->pNext = pNew;

        pNew->pPrev       = section->last;
        section->last     = pNew;
        section->selected = pNew;

#ifdef INI_USE_HASH_TABLE
        {   // Rev 1.3 - Added
            struct   key_tag *pOld;
            unsigned long crc32;
            unsigned char accel;

            crc32     = __ini_createCrc32 (key, strlen (key));
            pNew->crc = crc32;
            // Rev 1.3 - Add accelerator list
            accel = (unsigned char) crc32 & 0x0FF;
            pNew->pPrev_Acc = NULL;
            pOld = section->keys[accel];
            section->keys[accel]      = pNew;
            if (pOld) pOld->pPrev_Acc = pNew;
            pNew->pNext_Acc           = pOld;
        }
#endif
    }

    section->selected = pNew;
    ini->changed      = true;
    return pNew;
}


/********************************************************************************************************************
 * Function          : __ini_deleteKey
 * Parameters        : ini - pointer to ini file database.  key - key name
 * Returns           :
 * Globals Used      :
 * Globals Modified  :
 * Description       : Removes a key from the database only.
 *                   : This change does not occur in the file until ini_close is called.
 *                   : A heading operation must be called before this function.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
void __ini_deleteKey (ini_t *ini)
{
    struct key_tag     *current_k;
    struct section_tag *section = ini->selected;

    current_k = section->selected;
    if (current_k)
    {
        // Tidy up all users of this key
        section->selected =  NULL;
        if (section->last == current_k)
            section->last =  current_k->pPrev;

        // Check to see if all keys were removed
        if (!current_k->pPrev)
            section->first = current_k->pNext;
        else
            current_k->pPrev->pNext = current_k->pNext;
        if (current_k->pNext)
            current_k->pNext->pPrev = current_k->pPrev;

#ifdef INI_USE_HASH_TABLE
        // Rev 1.3 - Take member out of accelerator list
        if (!current_k->pPrev_Acc)
            section->keys[(unsigned char) current_k->crc & 0x0FF] = current_k->pNext_Acc;
        else
            current_k->pPrev_Acc->pNext_Acc = current_k->pNext_Acc;
        if (current_k->pNext_Acc)
            current_k->pNext_Acc->pPrev_Acc = current_k->pPrev_Acc;
#endif // INI_USE_HASH_TABLE

        // Delete Key
        free (current_k->key);
        free (current_k);
        ini->changed = true;
    }
}


/********************************************************************************************************************
 * Function          : __ini_locateKey
 * Parameters        : ini - pointer to ini file database.  key - key name
 * Returns           : Pointer to key.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Locates a key entry in the database.
 *                   : A heading operation must be called before this function.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
struct key_tag *__ini_locateKey (ini_t *ini, const char *key)
{
    struct key_tag     *current_k;
    struct section_tag *section;
    section = ini->selected;

#ifdef INI_USE_HASH_TABLE
    {   // Rev 1.3 - Added
        unsigned long crc32;
        crc32 = __ini_createCrc32 (key, strlen (key));

        // Search for key
        for (current_k = section->keys[(unsigned char) crc32 & 0x0FF]; current_k; current_k = current_k->pNext_Acc)
        {
            if (current_k->crc == crc32)
            {
                if (!strcmp (current_k->key, key))
                    break;
            }
        }
    }
#else
    {   // Search for key
        for (current_k = section->first; current_k; current_k = current_k->pNext)
        {
            if (!strcmp (current_k->key, key))
                break;
        }
    }
#endif // INI_USE_HASH_TABLE

    section->selected = current_k;
    return current_k;
}


/********************************************************************************************************************
 * Function          : (public) ini_deleteKey
 * Parameters        : ini - pointer to ini file database.  heading - heading name.  key - key name.
 * Returns           : -1 for Error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Equivalent Microsoft write string API call where data set to NULL
 ********************************************************************************************************************/
int INI_LINKAGE ini_deleteKey (ini_fd_t fd)
{
    ini_t *ini = (ini_t *) fd;
    if (!ini->selected)
        return -1;
    // Can't delete a temporary key
    if (ini->selected->selected == &(ini->tmpKey))
        return -1;
    
    __ini_deleteKey (ini);
    return 0;
}


/********************************************************************************************************************
 * Function          : (public) ini_locateKey
 * Parameters        : fd - pointer to ini file descriptor
 * Returns           : -1 for Error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : 
 ********************************************************************************************************************/
int INI_LINKAGE ini_locateKey (ini_fd_t fd, const char *key)
{
    ini_t *ini = (ini_t *) fd;
    struct key_tag *_key = NULL;

    if (!key)
        return -1;
    if (!ini->selected)
        return -1;

    // Can't search for a key in a temporary heading
    if (ini->selected != &(ini->tmpSection))
        _key = __ini_locateKey (ini, key);

#ifdef INI_ADD_LIST_SUPPORT
    // Rev 1.1 - Remove buffered list
    if (ini->list)
    {
        free (ini->list);
        ini->list = NULL;
    }
#endif // INI_ADD_LIST_SUPPORT

    if (_key)
        return 0;

    // Ok no key was found, but maybe the user is wanting to create a
    // new one so create it temporarily and see what actually happens later
    {   // Remove all key
        _key = &(ini->tmpKey);
        if (_key->key)
            free (_key->key);

        // Add new key
        _key->key = strdup (key);
        if (!_key)
            return -1;
        ini->selected->selected = _key;
    }
    return -1;
}
