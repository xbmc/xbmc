/***************************************************************************
                          types.i - Libini supported data types
                                    Use readString for others

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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ini.h"


/********************************************************************************************************************
 * Function          : __ini_read
 * Parameters        : ini - pointer to ini file database, size - key length returned here
 * Returns           : -1 for Error and 0 on success.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Common read functionality
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
static int __ini_read (ini_t *ini, size_t *size)
{
    struct key_tag *_key;

    if (!ini->selected)
        return -1;
    // Locate and read keys value
    _key = ini->selected->selected;
    if (!_key)
        return -1;

    if (_key->length)
    {   // Locate and read back keys data
        fseek (ini->ftmp, _key->pos, SEEK_SET);
    }
    else if (_key == &ini->tmpKey)
        return -1; // Can't read tmpKey
    *size = _key->length;
    return 0;
}


#ifdef INI_ADD_LIST_SUPPORT
/********************************************************************************************************************
 * Function          : __ini_readList
 * Parameters        : ini - pointer to ini file database.
 * Returns           : Pointer to buffer holding data.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Common read functionality from a list
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
static char *__ini_readList (ini_t *ini)
{
    if (!ini->selected)
        return NULL;
    // Locate and read keys value
    if (!ini->selected->selected)
        return NULL;
    return __ini_listRead (ini);
}
#endif // INI_ADD_LIST_SUPPORT


/********************************************************************************************************************
 * Function          : __ini_write
 * Parameters        : ini - pointer to ini file database.  heading - heading name.  key - key name.
 * Returns           : Pointer to new key.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Make calls to add a new key and appends a description of changes in the backup file
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
static struct key_tag *__ini_write (ini_t *ini)
{
    struct section_tag *section;
    struct key_tag     *key;

    // Is file read only?
    if (ini->mode == INI_READ)
        return NULL;

    // Check to make sure a section/key has
    // been asked for by the user
    section = ini->selected;
    if (!section)
        return NULL;
    key = section->selected;
    if (!key)
        return NULL;

    // Add or replace key
    if (!__ini_addHeading (ini, section->heading))
        return NULL;
    return __ini_addKey (ini, key->key);
}


/********************************************************************************************************************
 * Function          : ini_readString
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for error or the number of chars read.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Reads data part from a key and returns it as a string
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_readString (ini_fd_t fd, char *str, size_t size)
{
    ini_t *ini = (ini_t *) fd;

    // Check size and reserve space for NULL
    if (size-- <= 0)
        return -1;

#ifdef INI_ADD_LIST_SUPPORT
    if (ini->listDelims)
    {
        char  *data = __ini_readList (ini);
        if (!data)
            return -1;
		strncpy (str, data, size);
    }
    else
#endif // INI_ADD_LIST_SUPPORT
    {   // Locate and read back keys data (Index ignored)
        // Check to make sure size is correct
        size_t length;
        if (__ini_read (ini, &length) < 0)
            return -1;
        if (size > length)
            size = length;
        size = fread (str, sizeof(char), size, ini->ftmp);
    }

    str[size] = '\0';
    __ini_strtrim (str);
    return (int) size;
}


/********************************************************************************************************************
 * Function          : ini_writeString
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for Error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Writes data part to a key.
 *                   : Conforms to Microsoft API call.  E.g. use NULLS to remove headings/keys
 *                   : Headings and keys will be created as necessary
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_writeString (ini_fd_t fd, const char *str)
{
    ini_t *ini = (ini_t *) fd;
    struct key_tag *_key;

    _key = __ini_write (ini);
    if (!_key)    
        return -1;

    // Write data to bottom of backup file
    _key->length = strlen (str);
    fprintf (ini->ftmp, "%s\n", str);
    return 0;
}


/********************************************************************************************************************
 * Function          : ini_readInt
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for error or the number of values read.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Reads data part from a key and returns it as a int
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_readInt (ini_fd_t fd, int *value)
{
    ini_t *ini = (ini_t *) fd;

#ifdef INI_ADD_LIST_SUPPORT
    if (ini->listDelims)
    {
        char *data = __ini_readList (ini);
        if (!data)
            return -1;
        sscanf (data, "%d", value);
    }
    else
#endif // INI_ADD_LIST_SUPPORT
    {
        size_t length;
        if (__ini_read (ini, &length) < 0)
            return -1;
        if (length > 0)
            fscanf (ini->ftmp, "%d", value);
    }
    return 0;
}


#ifdef INI_ADD_EXTRAS

/********************************************************************************************************************
 * Function          : ini_readLong
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for error or the number of values read.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Reads data part from a key and returns it as a long
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_readLong (ini_fd_t fd, long *value)
{
    ini_t *ini = (ini_t *) fd;

#ifdef INI_ADD_LIST_SUPPORT
    if (ini->listDelims)
    {
        char *data = __ini_readList (ini);
        if (!data)
            return -1;
        sscanf (data, "%ld", value);
    }
    else
#endif // INI_ADD_LIST_SUPPORT
    {
        size_t length;
        if (__ini_read (ini, &length) < 0)
            return -1;
        if (length > 0)
            fscanf (ini->ftmp, "%ld", value);
    }
    return 0;
}


/********************************************************************************************************************
 * Function          : ini_readDouble
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for error or the number of values read.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Reads data part from a key and returns it as a double (real)
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_readDouble (ini_fd_t fd, double *value)
{
    ini_t *ini = (ini_t *) fd;

#ifdef INI_ADD_LIST_SUPPORT
    if (ini->listDelims)
    {
        char *data = __ini_readList (ini);
        if (!data)
            return -1;
        sscanf (data, "%lf", value);
    }
    else
#endif // INI_ADD_LIST_SUPPORT
    {
        size_t length;
        if (__ini_read (ini, &length) < 0)
            return -1;
        if (length > 0)
            fscanf (ini->ftmp, "%lf", value);
    }
    return 0;
}


/********************************************************************************************************************
 * Function          : ini_readBool
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for error or the number of values read.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Reads data part from a key and returns it as a int.  Supported bool strings are 0, 1, true
 *                   : false.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_readBool (ini_fd_t fd, int *value)
{
    ini_t *ini = (ini_t *) fd;
    char   buffer[6] = "";

#ifdef INI_ADD_LIST_SUPPORT
    if (ini->listDelims)
    {
        char *data = __ini_readList (ini);
        if (!data)
            return -1;
        sscanf (data, "%5s", buffer);
    }
    else
#endif // INI_ADD_LIST_SUPPORT
    {
        size_t length;
        if (__ini_read (ini, &length) < 0)
            return -1;
        if (length > 0)
            fscanf (ini->ftmp, "%5s", buffer);
    }

    {   // Convert string to lower case
        char *p = buffer;
        while (*p != '\0')
        {
            *p = (char) tolower (*p);
            p++;
        }
    }

    // Decode supported bool types
    switch (*buffer)
    {
    case '0':
    case '1':
        if (buffer[1] != '\0')
            return -1;
        *value = *buffer - '0';
        break;
    case 't':
        if (strcmp (buffer, "true"))
            return -1;
        *value = 1;
        break;
    case 'f':
        if (strcmp (buffer, "false"))
            return -1;
        *value = 0;
        break;
    default:
        // No match
        return -1;
    }
    return 0;
}


/********************************************************************************************************************
 * Function          : ini_writeInt
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for Error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Writes data part to a key.
 *                   : Headings and keys will be created as necessary
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_writeInt (ini_fd_t fd, int value)
{
    ini_t *ini = (ini_t *) fd;
    struct key_tag *_key;
    long   pos;

    _key = __ini_write (ini);
    if (!_key)
        return -1;

    // Write data to bottom of backup file
    fprintf (ini->ftmp, "%d", value);
    pos = ftell (ini->ftmp);
    _key->length = (size_t) (pos - _key->pos);
    fprintf (ini->ftmp, "\n");
    return 0;
}


/********************************************************************************************************************
 * Function          : ini_writeLong
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for Error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Writes data part to a key.
 *                   : Headings and keys will be created as necessary
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_writeLong (ini_fd_t fd, long value)
{
    ini_t *ini = (ini_t *) fd;
    struct key_tag *_key;
    long   pos;

    _key = __ini_write (ini);
    if (!_key)
        return -1;

    // Write data to bottom of backup file
    fprintf (ini->ftmp, "%ld", value);
    pos = ftell (ini->ftmp);
    _key->length = (size_t) (pos - _key->pos);
    fprintf (ini->ftmp, "\n");
    return 0;
}


/********************************************************************************************************************
 * Function          : ini_writeDouble
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for Error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Writes data part to a key.
 *                   : Headings and keys will be created as necessary
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_writeDouble (ini_fd_t fd, double value)
{
    ini_t *ini = (ini_t *) fd;
    struct key_tag *_key;
    long   pos;

    _key = __ini_write (ini);
    if (!_key)
        return -1;

    // Write data to bottom of backup file
    fprintf (ini->ftmp, "%f", value);
    pos = ftell (ini->ftmp);
    _key->length = (size_t) (pos - _key->pos);
    fprintf (ini->ftmp, "\n");
    return 0;
}


/********************************************************************************************************************
 * Function          : ini_writeBool
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for Error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Writes data part to a key.
 *                   : Headings and keys will be created as necessary
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_writeBool (ini_fd_t fd, int value)
{
    ini_t *ini = (ini_t *) fd;
    struct key_tag *_key;
    long   pos;

    // Check if value is legal
    if ((value < 0) || (value > 1))
        return -1;

    _key = __ini_write (ini);
    if (!_key)
        return -1;

    // Write data to bottom of backup file
    if (value)
        fprintf (ini->ftmp, "true");
    else
        fprintf (ini->ftmp, "false");        
    pos = ftell (ini->ftmp);
    _key->length = (size_t) (pos - _key->pos);
    fprintf (ini->ftmp, "\n");
    return 0;
}

#endif // INI_ADD_EXTRAS
