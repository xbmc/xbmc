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
#if (defined USE_EXTERNAL_LIBFAAD)
  #include <neaacdec.h>
#else
  #include "libfaad/neaacdec.h"
#endif
#include "DynamicDll.h"

#if (defined HAVE_LIBFAAD_DEBIAN_ABI)
  #define FAAD_SAMPLERATE_TYPE uint32_t
  #define FAAD_GETERROR_TYPE int8_t
#else
  #define FAAD_SAMPLERATE_TYPE unsigned long
  #define FAAD_GETERROR_TYPE char
#endif

#if (defined USE_EXTERNAL_LIBFAAD)

class DllLibFaadInterface
{
public:
    virtual ~DllLibFaadInterface() {}
    virtual FAAD_GETERROR_TYPE* NeAACDecGetErrorMessage(unsigned char errcode)=0;
    virtual uint32_t NeAACDecGetCapabilities(void)=0;
    virtual NeAACDecHandle NeAACDecOpen(void)=0;
    virtual NeAACDecConfigurationPtr NeAACDecGetCurrentConfiguration(NeAACDecHandle hDecoder)=0;
    virtual uint8_t NeAACDecSetConfiguration(NeAACDecHandle hDecoder,
                                                   NeAACDecConfigurationPtr config)=0;
    virtual long NeAACDecInit(NeAACDecHandle hDecoder,
                              unsigned char *buffer,
                              unsigned long buffer_size,
                              FAAD_SAMPLERATE_TYPE *samplerate,
                              unsigned char *channels)=0;
    virtual char NeAACDecInit2(NeAACDecHandle hDecoder,
                               unsigned char *pBuffer,
                               unsigned long SizeOfDecoderSpecificInfo,
                               FAAD_SAMPLERATE_TYPE *samplerate,
                               unsigned char *channels)=0;
    virtual void NeAACDecPostSeekReset(NeAACDecHandle hDecoder, int32_t frame)=0;
    virtual void NeAACDecClose(NeAACDecHandle hDecoder)=0;
    virtual void* NeAACDecDecode(NeAACDecHandle hDecoder,
                                 NeAACDecFrameInfo *hInfo,
                                 uint8_t *buffer,
                                 uint32_t buffer_size)=0;
    virtual void* NeAACDecDecode2(NeAACDecHandle hDecoder,
                                  NeAACDecFrameInfo *hInfo,
                                  uint8_t *buffer,
                                  uint32_t buffer_size,
                                  void **sample_buffer,
                                  uint32_t sample_buffer_size)=0;
    virtual int8_t NeAACDecAudioSpecificConfig(uint8_t *pBuffer,
                                             uint32_t buffer_size,
                                             mp4AudioSpecificConfig *mp4ASC)=0;
    #if (defined HAVE_LIBFAAD_NEAACDECINITDRM)
    virtual int8_t NeAACDecInitDRM(NeAACDecHandle *hDecoder, uint32_t samplerate,
                                 uint8_t channels)=0;
    #endif
};

class DllLibFaad : public DllDynamic, DllLibFaadInterface
{
public:
    virtual ~DllLibFaad() {}
    virtual FAAD_GETERROR_TYPE* NeAACDecGetErrorMessage(unsigned char errcode)
        { return ::NeAACDecGetErrorMessage(errcode); }
    virtual uint32_t NeAACDecGetCapabilities(void)
        { return ::NeAACDecGetCapabilities(); }
    virtual NeAACDecHandle NeAACDecOpen(void)
        { return ::NeAACDecOpen(); }
    virtual NeAACDecConfigurationPtr NeAACDecGetCurrentConfiguration(NeAACDecHandle hDecoder)
        { return ::NeAACDecGetCurrentConfiguration(hDecoder); }
    virtual uint8_t NeAACDecSetConfiguration(NeAACDecHandle hDecoder,
                                                   NeAACDecConfigurationPtr config)
        { return ::NeAACDecSetConfiguration(hDecoder, config); }
    virtual long NeAACDecInit(NeAACDecHandle hDecoder,
                              unsigned char *buffer,
                              unsigned long buffer_size,
                              FAAD_SAMPLERATE_TYPE *samplerate,
                              unsigned char *channels)
        { return ::NeAACDecInit(hDecoder, buffer, buffer_size, samplerate, channels); }
    virtual char NeAACDecInit2(NeAACDecHandle hDecoder,
                               unsigned char *pBuffer,
                               unsigned long SizeOfDecoderSpecificInfo,
                               FAAD_SAMPLERATE_TYPE *samplerate,
                               unsigned char *channels)
        { return ::NeAACDecInit2(hDecoder, pBuffer, SizeOfDecoderSpecificInfo, samplerate, channels); }
    virtual void NeAACDecPostSeekReset(NeAACDecHandle hDecoder, int32_t frame)
        { return ::NeAACDecPostSeekReset(hDecoder, frame); }
    virtual void NeAACDecClose(NeAACDecHandle hDecoder)
        { return ::NeAACDecClose(hDecoder); }
    virtual void* NeAACDecDecode(NeAACDecHandle hDecoder,
                                 NeAACDecFrameInfo *hInfo,
                                 uint8_t *buffer,
                                 uint32_t buffer_size)
        { return ::NeAACDecDecode(hDecoder, hInfo, buffer, buffer_size); }
    virtual void* NeAACDecDecode2(NeAACDecHandle hDecoder,
                                  NeAACDecFrameInfo *hInfo,
                                  uint8_t *buffer,
                                  uint32_t buffer_size,
                                  void **sample_buffer,
                                  uint32_t sample_buffer_size)
        { return ::NeAACDecDecode2(hDecoder, hInfo, buffer, buffer_size, sample_buffer, sample_buffer_size); }
    virtual int8_t NeAACDecAudioSpecificConfig(uint8_t *pBuffer,
                                             uint32_t buffer_size,
                                             mp4AudioSpecificConfig *mp4ASC)
        { return ::NeAACDecAudioSpecificConfig(pBuffer, buffer_size, mp4ASC); }
    #if (defined HAVE_LIBFAAD_NEAACDECINITDRM)
    virtual int8_t NeAACDecInitDRM(NeAACDecHandle *hDecoder, uint32_t samplerate,
                                 uint8_t channels)
        { return ::NeAACDecInitDRM(hDecoder, samplerate, channels); }
    #endif

    // DLL faking.
    virtual bool ResolveExports() { return true; }
    virtual bool Load() {
        CLog::Log(LOGDEBUG, "DllLibFaad: Using libfaad system library");
        return true;
    }
    virtual void Unload() {}
};

#else

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

#endif
