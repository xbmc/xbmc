// -*- C++ -*-
// $Id$

// id3lib: a C++ library for creating and manipulating id3v1/v2 tags
// Copyright 1999, 2000  Scott Thomas Haug
// Copyright 2002 Thijmen Klok (thijmen@id3lib.org)

// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2 of the License, or (at your
// option) any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
// License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// The id3lib authors encourage improvements and optimisations to be sent to
// the id3lib coordinator.  Please see the README file for details on where to
// send such submissions.  See the AUTHORS file for a list of people who have
// contributed to id3lib.  See the ChangeLog file for a list of changes to
// id3lib.  These files are distributed with id3lib at
// http://download.sourceforge.net/id3lib/

#ifndef _ID3LIB_UTILS_H_
#define _ID3LIB_UTILS_H_

#if defined(__BORLANDC__)
// due to a bug in borland it sometimes still wants mfc compatibility even when you disable it
#  if defined(_MSC_VER)
#    undef _MSC_VER
#  endif
#  if defined(__MFC_COMPAT__)
#    undef __MFC_COMPAT__
#  endif
#endif

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

#include "id3lib_streams.h"
#include "globals.h" //has <stdlib.h> "id3/sized_types.h"
#include "id3lib_strings.h"
#include <stdlib.h>

namespace dami
{
#ifdef  MAXPATHLEN
#  define ID3_PATH_LENGTH   (MAXPATHLEN + 1)
#elif   defined (PATH_MAX)
#  define ID3_PATH_LENGTH   (PATH_MAX + 1)
#else   /* !MAXPATHLEN */
#  define ID3_PATH_LENGTH   (2048 + 1)
#endif  /* !MAXPATHLEN && !PATH_MAX */

#ifndef min
  template<typename T>
  const T& min(const T& a, const T& b)
  {
    return (a < b) ? a : b;
  }
#endif

#ifndef max
  template<typename T>
  const T& max(const T& a, const T& b)
  {
    return (b < a) ? a : b;
  }
#endif

#ifndef mid
  template<typename T>
  const T& mid(const T& lo, const T& mid, const T& hi)
  {
    return max(lo, min(mid, hi));
  }
#endif

#ifndef abs
  template<typename T>
  T abs(const T& a)
  {
    return (a < T(0)) ? -a : a;
  }
#endif

  size_t ID3_C_EXPORT renderNumber(uchar *buffer, uint32 val, size_t size = sizeof(uint32));
  String ID3_C_EXPORT renderNumber(uint32 val, size_t size = sizeof(uint32));

  String ID3_C_EXPORT toString(uint32 val);
  WString ID3_C_EXPORT toWString(const unicode_t[], size_t);

  size_t ID3_C_EXPORT ucslen(const unicode_t *unicode);
  String ID3_C_EXPORT convert(String data, ID3_TextEnc, ID3_TextEnc);

  // file utils
  size_t ID3_C_EXPORT getFileSize(fstream&);
  size_t ID3_C_EXPORT getFileSize(ifstream&);
  size_t ID3_C_EXPORT getFileSize(ofstream&);
  ID3_Err ID3_C_EXPORT createFile(String, fstream&);
  ID3_Err ID3_C_EXPORT openWritableFile(String, fstream&);
  ID3_Err ID3_C_EXPORT openWritableFile(String, ofstream&);
  ID3_Err ID3_C_EXPORT openReadableFile(String, fstream&);
  ID3_Err ID3_C_EXPORT openReadableFile(String, ifstream&);

};

#endif /* _ID3LIB_UTILS_H_ */

