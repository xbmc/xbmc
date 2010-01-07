/*
    $Id: _cdio_stream.h,v 1.2 2005/01/20 01:00:52 rocky Exp $

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef __CDIO_STREAM_H__
#define __CDIO_STREAM_H__

#include <cdio/types.h>
#include "cdio_private.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  /* typedef'ed IO functions prototypes */
  
  typedef int(*cdio_data_open_t)(void *user_data);
  
  typedef long(*cdio_data_read_t)(void *user_data, void *buf, long count);
  
  typedef long(*cdio_data_seek_t)(void *user_data, long offset, int whence);
  
  typedef long(*cdio_data_stat_t)(void *user_data);
  
  typedef int(*cdio_data_close_t)(void *user_data);
  
  typedef void(*cdio_data_free_t)(void *user_data);
  
  
  /* abstract data source */
  
  typedef struct {
    cdio_data_open_t open;
    cdio_data_seek_t seek; 
    cdio_data_stat_t stat; 
    cdio_data_read_t read;
    cdio_data_close_t close;
    cdio_data_free_t free;
  } cdio_stream_io_functions;
  
  CdioDataSource_t *
  cdio_stream_new(void *user_data, const cdio_stream_io_functions *funcs);

  /*!
     Like fread(3) and in fact may be the same.

     DESCRIPTION:
     The function fread reads nmemb elements of data, each size bytes long,
     from the stream pointed to by stream, storing them at the location
     given by ptr.

     RETURN VALUE:
     return the number of items successfully read or written (i.e.,
     not the number of characters).  If an error occurs, or the
     end-of-file is reached, the return value is a short item count
     (or zero).

     We do not distinguish between end-of-file and error, and callers
     must use feof(3) and ferror(3) to determine which occurred.
  */
  long cdio_stream_read(CdioDataSource_t* p_obj, void *ptr, long size, 
                        long nmemb);
  
  /*! 
    Like fseek(3) and in fact may be the same.

    This  function sets the file position indicator for the stream
    pointed to by stream.  The new position, measured in bytes, is obtained
    by  adding offset bytes to the position specified by whence.  If whence
    is set to SEEK_SET, SEEK_CUR, or SEEK_END, the offset  is  relative  to
    the  start of the file, the current position indicator, or end-of-file,
    respectively.  A successful call to the fseek function clears the end-
    of-file indicator for the stream and undoes any effects of the
    ungetc(3) function on the same stream.
    
    RETURN VALUE
    Upon successful completion, return 0,
    Otherwise, -1 is returned and the global variable errno is set to indi-
    cate the error.
   */
  long int cdio_stream_seek(CdioDataSource_t *p_obj, long offset, int whence);
  
  /*!
    Return whatever size of stream reports, I guess unit size is bytes. 
    On error return -1;
  */
  long int cdio_stream_stat(CdioDataSource_t *p_obj);
  
  /*!
    Deallocate resources assocaited with p_obj. After this p_obj is unusable.
  */
  void cdio_stream_destroy(CdioDataSource_t *p_obj);
  
  void cdio_stream_close(CdioDataSource_t *p_obj);
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_STREAM_H__ */


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
