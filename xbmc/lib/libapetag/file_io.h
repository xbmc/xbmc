/********************************************************************
*    
* Copyright (c) 2010 Team XBMC. All rights reserved.
*
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


#ifndef _FILEIO_H
#define _FILEIO_H

/** \file
    \brief All function related to apetag file i/o
*/


/**
 object ape_file used to store file i/o functions
 contains functions read_func,seek_func,tell_func,close_func and a data pointer.
 \brief object ape_file used to store file i/o functions
 */
typedef struct _ape_file_io ape_file;

ape_file *ape_fopen(const char *file, const char *rw);
size_t ape_fread(void *ptr, size_t size, size_t nmemb, ape_file *fp);
int ape_fseek(ape_file *fp, long int offset, int whence);
int ape_fclose(ape_file *fp);
long ape_ftell(ape_file *fp);
int ape_fflush(ape_file *fp);
size_t ape_fwrite (const void * ptr, size_t size, size_t count, ape_file *fp);

#endif /* _FILEIO_H */
