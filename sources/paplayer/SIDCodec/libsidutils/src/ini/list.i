/***************************************************************************
                          list.i - Adds list support to ini files

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

#include <stdlib.h>
#include <string.h>
#include "list.h"

#ifdef INI_ADD_LIST_SUPPORT

/********************************************************************************************************************
 * Function          : ini_listEval
 * Parameters        : ini - pointer to ini file database.
 * Returns           : -1 for Error or the numbers of list items found
 * Globals Used      :
 * Globals Modified  :
 * Description       : Uses the currently specified delimiters to calculate the number of eliments in a list
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int __ini_listEval (ini_t *ini)
{
    int  length, count, i, ret;
    int  ldelim;
    char ch;

    // Remove old list
    if (ini->list)
    {
        free (ini->list);
        ini->list = NULL;
    }

    // Re-evaluate with new settings
    length = ini->selected->selected->length;
    if (length < 0)
        return -1;
    if (!length)
    {
        ini->listIndex  = 0;
        ini->listLength = 0;
        if (ini->selected->selected == &ini->tmpKey)
            return -1; // Can't read tmpKey
        return 0;
    }

    // See if there are any chars which can be used to split the string into sub ones
    if (!ini->listDelims)
        return -1;
    ldelim = (int) strlen (ini->listDelims);
        
    // Buffer string for faster access
    ini->list = (char *) malloc (length + 1);
    if (!ini->list)
        return -1;

    {   // Backup up delims to avoid causing problems with readString
        char *delims    = ini->listDelims;
        ini->listDelims = NULL;
        ret = ini_readString ((ini_fd_t) ini, ini->list, length + 1);
        ini->listDelims = delims;
        if (ret < 0)
            return -1;
    }
    
    // Process buffer string to find number of sub strings
    {
        char lastch = '\0';
        count = 1;
        while (length)
        {
            length--;
            ch = ini->list[length];
            for (i = 0; i < ldelim; i++)
            {
                if ((char) ch == ini->listDelims[i])
                {   // Prevent lots of NULL strings on multiple
                    // whitespace
                    if (lastch == '\0')
                    {
                        if (isspace (ch))
                        {
                            ch = '\0';
                            break;
                        }
                    }
 
                    // Seperate strings
                    ini->list[length] = (ch = '\0');
                    count++;
                    break;
                }
            }
            lastch = ch;
        }
    }

    ini->listLength   = count;
    ini->listIndexPtr = ini->list;
    ini->listIndex    = 0;
    return count;
}


/********************************************************************************************************************
 * Function          : __ini_listRead
 * Parameters        : ini - pointer to ini file database.
 * Returns           : NULL for error, string point otherwise
 * Globals Used      :
 * Globals Modified  :
 * Description       : Reads the indexed parameter from the list and auto
 *                   : increments
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
char *__ini_listRead (ini_t *ini)
{
    char *p;

    // we must read an element from the list
    // Rev 1.2 Changed order of these two ifs as test was performed
    // sometimes before anything was read
    if (!ini->list)
    {
        if (__ini_listEval (ini) < 0)
            return NULL;
        // Handle an empty list
        if (!ini->listLength)
            return "";
    }

    // Check to see if we are trying to get a value beyond the end of the list
    if (ini->listIndex >= ini->listLength)
        return NULL;
    p = ini->listIndexPtr;
    // Auto increment pointers to next index
    ini->listIndexPtr += (strlen (ini->listIndexPtr) + 1);
    ini->listIndex++;
    return p;
}


/********************************************************************************************************************
 * Function          : ini_listLength
 * Parameters        : ini - pointer to ini file database.  heading - heading name.  key - key name.
 * Returns           : -1 for Error or the numbers of list items found
 * Globals Used      :
 * Globals Modified  :
 * Description       : Uses the currently specified delimiters to calculate the number of eliments in a list
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_listLength (ini_fd_t fd)
{
    ini_t *ini = (ini_t *) fd;

    // Check to make sure a section/key has
    // been asked for by the user
    if (!ini->selected)
        return -1;
    if (!ini->selected->selected)
        return -1;

    // Check to see if we have moved to a new key
    if (!ini->list)
        return __ini_listEval (ini);

    return ini->listLength;
}


/********************************************************************************************************************
 * Function          : ini_listDelims
 * Parameters        : ini - pointer to ini file database.  delims - string of delimitor chars
 * Returns           : -1 for Error or 0 for success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Sets the delimiters used for list accessing, (default delim is NULL)
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_listDelims (ini_fd_t fd, const char *delims)
{
    ini_t *ini = (ini_t *) fd;
    if (ini->listDelims)
        free (ini->listDelims);
    ini->listDelims = NULL;

    // Make sure we have something to copy
    if (delims)
    {
        if (*delims)
        {   // Store delims for later use
            ini->listDelims = strdup (delims);
            if (!ini->listDelims)
                return -1;
        }
    }

    // List will need recalculating at some point
    if (ini->list)
    {
        free (ini->list);
        ini->list = NULL;
    }
    return 0;
}


/********************************************************************************************************************
 * Function          : ini_listIndex
 * Parameters        : ini - pointer to ini file database.  delims - string of delimitor chars
 * Returns           : 
 * Globals Used      :
 * Globals Modified  :
 * Description       : Sets the index that the next call to any of the read function will obtain (default 0)
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_listIndex (ini_fd_t fd, unsigned long index)
{
    ini_t *ini = (ini_t *) fd;
    unsigned int count;
    char    *p;

    // Check to make sure a section/key has
    // been asked for by the user
    if (!ini->selected)
        return -1;
    if (!ini->selected->selected)
        return -1;

    // Pull in new list
    if (!ini->list)
    {
        if (__ini_listEval (ini) < 0)
            return -1;
    }

    // Now scroll through to required index
    if (!ini->listLength)
        return -1;
    if (index == ini->listIndex)
        return 0;

    if (index > ini->listIndex)
    {   // Continue search from the from current position
        count = ini->listIndex;
        p     = ini->listIndexPtr;
    }
    else
    {   // Reset list and search from beginning
        count = 0;
        p     = ini->list;
    }
    
    while (count != index)
    {
        count++;
        if (count >= ini->listLength)
            return -1;
        // Jump to next sub string
        p += (strlen (p) + 1);
    }

    ini->listIndex    = count;
    ini->listIndexPtr = p;
    return 0;
}


/********************************************************************************************************************
 * Function          : __ini_listIndexLength
 * Parameters        : ini - pointer to ini file database
 * Returns           : 
 * Globals Used      :
 * Globals Modified  :
 * Description       : Returns the length the indexed sub string
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int __ini_listIndexLength (ini_t *ini)
{
    if (!ini->list)
    {   // No list yet.  So try to get one
        if (__ini_listEval (ini) < 0)
            return -1;
        if (!ini->listLength)
            return 0;
    }

    // Now return length
    return strlen (ini->listIndexPtr);
}

#endif // INI_ADD_LIST_SUPPORT
