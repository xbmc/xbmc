/***************************************************************************
                         null.cpp  -  Null audio driver used for hardsid
                                      and songlength detection
                            -------------------
   begin                : Mon Nov 6 2000
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
/***************************************************************************
 *  $Log$
 *  Revision 1.2  2005/03/19 17:33:05  jmarshallnz
 *  Formatting for tabs -> 2 spaces
 *
 *    - 19-03-2005 added: <autodetectFG> tag to XBoxMediaCenter.xml.  Set to false if you have an old bios that causes a crash on autodetection.
 *
 *  Revision 1.1  2004/03/09 00:18:24  butcheruk
 *  Sid playback support
 *
 *  Revision 1.8  2002/03/07 07:55:59  s_a_white
 *  Removed bad define.
 *
 *  Revision 1.7  2002/03/04 19:07:48  s_a_white
 *  Fix C++ use of nothrow.
 *
 *  Revision 1.6  2001/12/11 19:38:13  s_a_white
 *  More GCC3 Fixes.
 *
 *  Revision 1.5  2001/08/21 21:57:14  jpaana
 *  Don't try to resize the buffer if the size is given (confuses other audiodrivers)
 *
 *  Revision 1.4  2001/07/14 16:55:51  s_a_white
 *  Cast _sampleBuffer from void * for delete to avoid warning
 *  message.
 *
 *  Revision 1.3  2001/07/03 17:54:50  s_a_white
 *  Support for new audio interface for better compatibility.
 *
 *  Revision 1.2  2001/01/23 21:22:31  s_a_white
 *  Changed to array delete.
 *
 *  Revision 1.1  2001/01/08 16:41:43  s_a_white
 *  App and Library Seperation
 *
 ***************************************************************************/

#include "null.h"
#include "config.h"
#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

Audio_Null::Audio_Null ()
{
  isOpen = false;
}

Audio_Null::~Audio_Null ()
{
  close();
}

void *Audio_Null::open (AudioConfig &cfg, const char *)
{
  uint_least32_t bufSize = cfg.bufSize;

  if (isOpen)
  {
    _errorString = "NULL ERROR: Audio device already open.";
    return NULL;
  }

  if (bufSize == 0)
  {
    bufSize = cfg.frequency * cfg.precision / 8 * cfg.channels;
    bufSize /= 4;
  }

  // We need to make a buffer for the user
#if defined(HAVE_EXCEPTIONS)
  _sampleBuffer = new(std::nothrow) uint_least8_t[bufSize];
#else
  _sampleBuffer = new uint_least8_t[bufSize];
#endif
  if (!_sampleBuffer)
    return NULL;

  isOpen = true;
  cfg.bufSize = bufSize;
  _settings = cfg;
  return _sampleBuffer;
}

void *Audio_Null::write ()
{
  if (!isOpen)
  {
    _errorString = "NULL ERROR: Audio device not open.";
    return NULL;
  }
  return _sampleBuffer;
}

void *Audio_Null::reset (void)
{
  if (!isOpen)
    return NULL;
  return _sampleBuffer;
}

void Audio_Null::close (void)
{
  if (!isOpen)
    return ;
  delete [] (uint_least8_t *) _sampleBuffer;
  _sampleBuffer = NULL;
  isOpen = false;
}
