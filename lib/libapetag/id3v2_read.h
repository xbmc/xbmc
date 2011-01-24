/********************************************************************
*    
* Copyright (c) 2002 Artur Polaczynski (Ar't)  All rights reserved.
*            <artii@o2.pl>        LGPL-2.1
*       $ArtId: id3v2_read.h,v 1.6 2003/04/04 20:06:40 art Exp $
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


#ifndef _ID3V2_READ_H
#define _ID3V2_READ_H

/** \file id3v2_read.h
    \brief reading id3v2 tag to ape_cnt 
*/

#ifdef ID3V2_READ
/**
    reading id3v2 tag to #mem_cnt 

    \param mem_cnt stucture ape_mem_cnt
    \param fileName file name (no file pointer becase libid3 using filename in c mode)
*/
int readtag_id3v2 ( apetag *mem_cnt, char* fileName );
#endif

#endif /* _ID3V2_READ_H */

