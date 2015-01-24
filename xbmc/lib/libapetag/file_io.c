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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <math.h>
#if !defined(__BORLANDC__) && !defined(_MSC_VER)
#    include <unistd.h>
#endif
#include "file_io.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#define strcasecmp stricmp
#define snprintf _snprintf
#define USE_CHSIZE
#define S_IRUSR 0
#define S_IWUSR 0
#define S_IRGRP 0
#define S_IWGRP 0
#if defined(_WIN64)
 typedef __int64 ssize_t; 
#else
 typedef long ssize_t;
#endif
#endif

/* LOCAL STRUCTURES */

/** 
 \struct _ape_ape_file
 \brief structure for file I/O
 */
struct _ape_file_io
{
  size_t (*read_func)  (void *ptr, size_t size, size_t nmemb, void *datasource);
  int    (*seek_func)  (void *datasource, long int offset, int whence);
  long   (*tell_func)  (void *datasource);
  void *data;
};

/* LOCAL FUNCTIONS */

static size_t fread_internal(void *ptr, size_t size, size_t nmemb, void *fp)
{
  return fread(ptr, size, nmemb, (FILE *)fp);
}

static int fseek_internal(void *fp, long int offset, int whence)
{
  return fseek((FILE *)fp, offset, whence);
}

static long ftell_internal(void *fp)
{
  return ftell((FILE *)fp);
}

ape_file *ape_fopen(const char *file, const char *rw)
{
  FILE *data = NULL;
  ape_file *fp = NULL;
  
  data = fopen(file, rw);
  if (data)
  {
    fp = malloc(sizeof(ape_file));
    if (fp)
    {
      fp->read_func = fread_internal;
      fp->seek_func = fseek_internal;
      fp->tell_func = ftell_internal;
      fp->data = data;
    }
    else
    {
      fclose(data);
    }
  }
  return fp;
}

int ape_fclose(ape_file *fp)
{
  int ret = fclose((FILE *)fp->data);
  free(fp);
  return ret;
}

int ape_fflush(ape_file *fp)
{
  return fflush((FILE *)fp->data);
}

size_t ape_fwrite(const void * ptr, size_t size, size_t count, ape_file *fp)
{
  return fwrite(ptr, size, count, (FILE *)fp->data);
}

size_t ape_fread(void *ptr, size_t size, size_t nmemb, ape_file *fp)
{
  return fp->read_func(ptr, size, nmemb, fp->data);
}

int ape_fseek(ape_file *fp, long int offset, int whence)
{
  return fp->seek_func(fp->data, offset, whence);
}

long ape_ftell(ape_file *fp)
{
  return fp->tell_func(fp->data);
}
