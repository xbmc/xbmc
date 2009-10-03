#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#if defined(_LINUX) && !defined(__APPLE__)
  #include <neaacdec.h>
#else
  #include "libfaad/neaacdec.h"
#endif
#include "DynamicDll.h"
#include "utils/log.h"

class DllLibFaadInterface
{
public:
  virtual ~DllLibFaadInterface() {}
  virtual NeAACDecHandle NeAACDecOpen(void)=0;
  virtual NeAACDecConfigurationPtr NeAACDecGetCurrentConfiguration(NeAACDecHandle hDecoder)=0;
  virtual unsigned char NeAACDecSetConfiguration(NeAACDecHandle hDecoder, NeAACDecConfigurationPtr config)=0;
  virtual void NeAACDecClose(NeAACDecHandle hDecoder)=0;
  virtual void* NeAACDecDecode(NeAACDecHandle hDecoder, NeAACDecFrameInfo *hInfo, unsigned char *buffer, unsigned long buffer_size)=0;
  virtual long NeAACDecInit(NeAACDecHandle hDecoder, unsigned char *buffer, unsigned long buffer_size, unsigned long *samplerate, unsigned char *channels)=0;
  virtual char NeAACDecInit2(NeAACDecHandle hDecoder, unsigned char *pBuffer, unsigned long SizeOfDecoderSpecificInfo, unsigned long *samplerate, unsigned char *channels)=0;
  virtual char* NeAACDecGetErrorMessage(unsigned char errcode)=0;
  virtual void NeAACDecPostSeekReset(NeAACDecHandle hDecoder, long frame)=0;
};

class DllLibFaad : public DllDynamic, DllLibFaadInterface
{
  DECLARE_DLL_WRAPPER(DllLibFaad, DLL_PATH_LIBFAAD)
  DEFINE_METHOD0(NeAACDecHandle, NeAACDecOpen)
  DEFINE_METHOD1(NeAACDecConfigurationPtr, NeAACDecGetCurrentConfiguration, (NeAACDecHandle p1))
  DEFINE_METHOD2(unsigned char, NeAACDecSetConfiguration, (NeAACDecHandle p1, NeAACDecConfigurationPtr p2))
  DEFINE_METHOD1(void, NeAACDecClose, (NeAACDecHandle p1))
  DEFINE_METHOD4(void*, NeAACDecDecode, (NeAACDecHandle p1, NeAACDecFrameInfo *p2, unsigned char *p3, unsigned long p4))
  DEFINE_METHOD5(long, NeAACDecInit, (NeAACDecHandle p1, unsigned char *p2, unsigned long p3, unsigned long *p4, unsigned char *p5))
  DEFINE_METHOD5(char, NeAACDecInit2, (NeAACDecHandle p1, unsigned char *p2, unsigned long p3, unsigned long *p4, unsigned char *p5))
  DEFINE_METHOD1(char*, NeAACDecGetErrorMessage, (unsigned char p1))
  DEFINE_METHOD2(void, NeAACDecPostSeekReset, (NeAACDecHandle p1, long p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(NeAACDecOpen)
    RESOLVE_METHOD(NeAACDecGetCurrentConfiguration)
    RESOLVE_METHOD(NeAACDecSetConfiguration)
    RESOLVE_METHOD(NeAACDecClose)
    RESOLVE_METHOD(NeAACDecDecode)
    RESOLVE_METHOD(NeAACDecInit)
    RESOLVE_METHOD(NeAACDecInit2)
    RESOLVE_METHOD(NeAACDecGetErrorMessage)
    RESOLVE_METHOD(NeAACDecPostSeekReset)
  END_METHOD_RESOLVE()
};

