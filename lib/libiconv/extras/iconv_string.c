/* Copyright (C) 1999-2001, 2003 Bruno Haible.
   This file is not part of the GNU LIBICONV Library.
   This file is put into the public domain.  */

#include "iconv_string.h"
#include <iconv.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define tmpbufsize 4096

int iconv_string (const char* tocode, const char* fromcode,
                  const char* start, const char* end,
                  char** resultp, size_t* lengthp)
{
  iconv_t cd = iconv_open(tocode,fromcode);
  size_t length;
  char* result;
  if (cd == (iconv_t)(-1)) {
    if (errno != EINVAL)
      return -1;
    /* Unsupported fromcode or tocode. Check whether the caller requested
       autodetection. */
    if (!strcmp(fromcode,"autodetect_utf8")) {
      int ret;
      /* Try UTF-8 first. There are very few ISO-8859-1 inputs that would
         be valid UTF-8, but many UTF-8 inputs are valid ISO-8859-1. */
      ret = iconv_string(tocode,"UTF-8",start,end,resultp,lengthp);
      if (!(ret < 0 && errno == EILSEQ))
        return ret;
      ret = iconv_string(tocode,"ISO-8859-1",start,end,resultp,lengthp);
      return ret;
    }
    if (!strcmp(fromcode,"autodetect_jp")) {
      int ret;
      /* Try 7-bit encoding first. If the input contains bytes >= 0x80,
         it will fail. */
      ret = iconv_string(tocode,"ISO-2022-JP-2",start,end,resultp,lengthp);
      if (!(ret < 0 && errno == EILSEQ))
        return ret;
      /* Try EUC-JP next. Short SHIFT_JIS inputs may come out wrong. This
         is unavoidable. People will condemn SHIFT_JIS.
         If we tried SHIFT_JIS first, then some short EUC-JP inputs would
         come out wrong, and people would condemn EUC-JP and Unix, which
         would not be good. */
      ret = iconv_string(tocode,"EUC-JP",start,end,resultp,lengthp);
      if (!(ret < 0 && errno == EILSEQ))
        return ret;
      /* Finally try SHIFT_JIS. */
      ret = iconv_string(tocode,"SHIFT_JIS",start,end,resultp,lengthp);
      return ret;
    }
    if (!strcmp(fromcode,"autodetect_kr")) {
      int ret;
      /* Try 7-bit encoding first. If the input contains bytes >= 0x80,
         it will fail. */
      ret = iconv_string(tocode,"ISO-2022-KR",start,end,resultp,lengthp);
      if (!(ret < 0 && errno == EILSEQ))
        return ret;
      /* Finally try EUC-KR. */
      ret = iconv_string(tocode,"EUC-KR",start,end,resultp,lengthp);
      return ret;
    }
    errno = EINVAL;
    return -1;
  }
  /* Determine the length we need. */
  {
    size_t count = 0;
    char tmpbuf[tmpbufsize];
    const char* inptr = start;
    size_t insize = end-start;
    while (insize > 0) {
      char* outptr = tmpbuf;
      size_t outsize = tmpbufsize;
      size_t res = iconv(cd,&inptr,&insize,&outptr,&outsize);
      if (res == (size_t)(-1) && errno != E2BIG) {
        if (errno == EINVAL)
          break;
        else {
          int saved_errno = errno;
          iconv_close(cd);
          errno = saved_errno;
          return -1;
        }
      }
      count += outptr-tmpbuf;
    }
    {
      char* outptr = tmpbuf;
      size_t outsize = tmpbufsize;
      size_t res = iconv(cd,NULL,NULL,&outptr,&outsize);
      if (res == (size_t)(-1)) {
        int saved_errno = errno;
        iconv_close(cd);
        errno = saved_errno;
        return -1;
      }
      count += outptr-tmpbuf;
    }
    length = count;
  }
  if (lengthp != NULL)
    *lengthp = length;
  if (resultp == NULL) {
    iconv_close(cd);
    return 0;
  }
  result = (*resultp == NULL ? malloc(length) : realloc(*resultp,length));
  *resultp = result;
  if (length == 0) {
    iconv_close(cd);
    return 0;
  }
  if (result == NULL) {
    iconv_close(cd);
    errno = ENOMEM;
    return -1;
  }
  iconv(cd,NULL,NULL,NULL,NULL); /* return to the initial state */
  /* Do the conversion for real. */
  {
    const char* inptr = start;
    size_t insize = end-start;
    char* outptr = result;
    size_t outsize = length;
    while (insize > 0) {
      size_t res = iconv(cd,&inptr,&insize,&outptr,&outsize);
      if (res == (size_t)(-1)) {
        if (errno == EINVAL)
          break;
        else {
          int saved_errno = errno;
          iconv_close(cd);
          errno = saved_errno;
          return -1;
        }
      }
    }
    {
      size_t res = iconv(cd,NULL,NULL,&outptr,&outsize);
      if (res == (size_t)(-1)) {
        int saved_errno = errno;
        iconv_close(cd);
        errno = saved_errno;
        return -1;
      }
    }
    if (outsize != 0) abort();
  }
  iconv_close(cd);
  return 0;
}
