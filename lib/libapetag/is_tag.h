/********************************************************************
*    
* Copyright (c) 2002 Artur Polaczynski (Ar't)  All rights reserved.
*            <artii@o2.pl>        LGPL-2.1
*       $ArtId: is_tag.h,v 1.7 2003/04/13 11:24:10 art Exp $
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


#ifndef _IS_TAG_H
#define _IS_TAG_H

/** \file is_tag.h 
    \brief Function for check if tag is avilable 

    All is_* function restore file positon on return
*/

int is_id3v1 (ape_file * fp);

int is_id3v2 (ape_file * fp);

int is_ape (ape_file * fp);

int is_ape_ver (ape_file * fp);

#endif /* _IS_TAG_H */
