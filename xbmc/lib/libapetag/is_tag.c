/********************************************************************
*    
* Copyright (c) 2002 Artur Polaczynski (Ar't)  All rights reserved.
*            <artii@o2.pl>        LGPL-2.1
*       $ArtId: is_tag.c,v 1.10 2003/04/16 21:06:27 art Exp $
********************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include "file_io.h"
#include "is_tag.h"

unsigned long
is_tag_ape2long (unsigned char *p);

/*
    PL: czy dany plik ma taga odpowiednio id3v1, id3v2 i ape ???
    PL: nie zmienia pozycji w pliku !!!
*/

/** 
    return size of all id3v1 tags (some bugy tagers add this again and 
    again tag) 0 no tag at all 

    \param fp File pointer
    \return Return size of id3v1 tag (in bytes) 0 no tag at all 
*/
int
is_id3v1 (ape_file * fp)
{
    int n=0;
    char buf[16];
    size_t savedFilePosition;
    
    savedFilePosition = ape_ftell(fp);
    ape_fseek (fp, 0, SEEK_END);
    do {
        n++;
        memset (buf, 0, sizeof (buf));
        ape_fseek (fp, ((-128)*n) - 3 , SEEK_END);
        ape_fread (&buf, 1, sizeof (buf), fp);
        if (memcmp (buf, "APETAGEX",8) == 0) /*APE.TAG.EX*/
        break;
    } while (memcmp (buf+3, "TAG", 3) == 0);
    
    ape_fseek (fp, savedFilePosition, SEEK_SET);
    return (n-1)*128;
}

/** 
    return size of tag id3v2 on begining of file. 
    check for buggy tagers (2 or more tags)
    
    \param fp File pointer
    \return Return size of id3v2 tag (in bytes)
    (some bugy tagers add this again and again ) 0 no tag at all 
*/
int
is_id3v2 (ape_file * fp)
{
    char buf[16];
    size_t savedFilePosition;
    long id3v2size=0;
        
    savedFilePosition = ape_ftell (fp);
    ape_fseek (fp, 0, SEEK_SET);
    do {    
        memset (buf, 0, sizeof (buf));
        ape_fseek (fp, id3v2size, SEEK_SET);
        ape_fread (&buf, 1, sizeof (buf), fp);
        if (memcmp (buf, "ID3", 3) != 0) {
        break;
        }
        /* ID3v2 tag skipeer $49 44 33 yy yy xx zz zz zz zz [zz size + this 10 bytes] */
        id3v2size += 10 + (((long) (buf[9])) | ((long) (buf[8]) << 7) |
        ((long) (buf[7]) << 14) | ((long) (buf[6]) << 21));
    } while(memcmp (buf, "ID3", 3) == 0);
    
    ape_fseek (fp, savedFilePosition, SEEK_SET);
    return (int) id3v2size;
}


/** 
    return 0 or 1000 or 2000 this is version of ape tag 0 no tag 

    \param fp File pointer
    \return Version of ape tag if any, else 0 
*/
int
is_ape_ver (ape_file * fp)
{
    char unsigned buf[32];
    size_t savedFilePosition;
        
    savedFilePosition = ape_ftell (fp);
    memset (buf, 0, sizeof (buf));
        
    ape_fseek (fp, (is_id3v1 (fp) ? -32 - 128 : -32), SEEK_END);
    ape_fread (&buf, 1, sizeof (buf), fp);
    if (memcmp (buf, "APETAGEX", 8) != 0) {
        ape_fseek (fp, savedFilePosition, SEEK_SET);
        return 0;
    }
        
    ape_fseek (fp, savedFilePosition, SEEK_SET);
    return (int) is_tag_ape2long (buf + 8);
}

#define IS_TAG_FOOTER_NOT       0x40000000

/** 
    return size of ape tag id3v1 is not counting 

    \param fp File pointer
    \return Size of ape tag if any, else 0 
*/
int
is_ape (ape_file * fp)
{
    char unsigned buf[32];
    size_t savedFilePosition;
        
    savedFilePosition = ape_ftell(fp);
    memset (buf, 0, sizeof (buf));
        
    ape_fseek (fp, (is_id3v1 (fp) ? -32 - 128 : -32), SEEK_END);
    ape_fread (&buf, 1, sizeof (buf), fp);
    if (memcmp (buf, "APETAGEX", 8) != 0) {
        ape_fseek (fp, savedFilePosition, SEEK_SET);
        return 0;
    }
        
    ape_fseek (fp, savedFilePosition, SEEK_SET);
    /* WARNING! macabra code */
    return (int) (is_tag_ape2long (buf + 8 + 4) +
        ( 
            ( (is_tag_ape2long (buf + 8) == 2000) &&
            !(is_tag_ape2long (buf + 8 + 4 + 8) & IS_TAG_FOOTER_NOT)
            ) ? 32 : 0 
        ) /* footer size = 32 */
        );
}


unsigned long
is_tag_ape2long (unsigned char *p)
{

    return (((unsigned long) p[0] << 0)  |
            ((unsigned long) p[1] << 8)  |
            ((unsigned long) p[2] << 16) |
            ((unsigned long) p[3] << 24) );

}
