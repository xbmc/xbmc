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
#if defined(WIN32)
  #include "wavpack.h"
#else
  #include <wavpack/wavpack.h>
#endif
#include "DynamicDll.h"
#include "utils/log.h"

#if defined(_LINUX)
// API changes from 4.2 to 4.60.1.
// Windows is using 4.2 from in tree source
// Linux/OSX/iOS is using 4.60.1 system lib from upstream.
// We can't tell the difference between them.
#define stream_reader WavpackStreamReader
#define blockout_f WavpackBlockOutput
#endif

class DllWavPackInterface
{
public:
  virtual ~DllWavPackInterface() {}
  virtual WavpackContext *WavpackOpenFileInputEx (stream_reader *reader, void *wv_id, void *wvc_id, char *error, int flags, int norm_offset)=0;
  virtual WavpackContext *WavpackOpenFileInput (const char *infilename, char *error, int flags, int norm_offset)=0;
  virtual int WavpackGetVersion (WavpackContext *wpc)=0;
  virtual unsigned int WavpackUnpackSamples (WavpackContext *wpc, int *buffer, unsigned int samples)=0;
  virtual unsigned int WavpackGetNumSamples (WavpackContext *wpc)=0;
  virtual unsigned int WavpackGetSampleIndex (WavpackContext *wpc)=0;
  virtual int WavpackGetNumErrors (WavpackContext *wpc)=0;
  virtual int WavpackLossyBlocks (WavpackContext *wpc)=0;
  virtual int WavpackSeekSample (WavpackContext *wpc, unsigned int sample)=0;
  virtual WavpackContext *WavpackCloseFile (WavpackContext *wpc)=0;
  virtual unsigned int WavpackGetSampleRate (WavpackContext *wpc)=0;
  virtual int WavpackGetBitsPerSample (WavpackContext *wpc)=0;
  virtual int WavpackGetBytesPerSample (WavpackContext *wpc)=0;
  virtual int WavpackGetNumChannels (WavpackContext *wpc)=0;
  virtual int WavpackGetReducedChannels (WavpackContext *wpc)=0;
  virtual int WavpackGetMD5Sum (WavpackContext *wpc, unsigned char data[16])=0;
  virtual unsigned int WavpackGetWrapperBytes (WavpackContext *wpc)=0;
  virtual unsigned char *WavpackGetWrapperData (WavpackContext *wpc)=0;
  virtual void WavpackFreeWrapper (WavpackContext *wpc)=0;
  virtual double WavpackGetProgress (WavpackContext *wpc)=0;
  virtual unsigned int WavpackGetFileSize (WavpackContext *wpc)=0;
  virtual double WavpackGetRatio (WavpackContext *wpc)=0;
  virtual double WavpackGetAverageBitrate (WavpackContext *wpc, int count_wvc)=0;
  virtual double WavpackGetInstantBitrate (WavpackContext *wpc)=0;
  virtual int WavpackGetTagItem (WavpackContext *wpc, const char *item, char *value, int size)=0;
  virtual int WavpackAppendTagItem (WavpackContext *wpc, const char *item, const char *value)=0;
  virtual int WavpackWriteTag (WavpackContext *wpc)=0;
  virtual WavpackContext *WavpackOpenFileOutput (blockout_f blockout, void *wv_id, void *wvc_id)=0;
  virtual int WavpackSetConfiguration (WavpackContext *wpc, WavpackConfig *config, unsigned int total_samples)=0;
  virtual int WavpackAddWrapper (WavpackContext *wpc, void *data, unsigned int bcount)=0;
  virtual int WavpackStoreMD5Sum (WavpackContext *wpc, unsigned char data[16])=0;
  virtual int WavpackPackInit (WavpackContext *wpc)=0;
  virtual int WavpackPackSamples (WavpackContext *wpc, int *sample_buffer, unsigned int sample_count)=0;
  virtual int WavpackFlushSamples (WavpackContext *wpc)=0;
  virtual void WavpackUpdateNumSamples (WavpackContext *wpc, void *first_block)=0;
  virtual void *WavpackGetWrapperLocation (void *first_block)=0;
};

class DllWavPack : public DllDynamic, DllWavPackInterface
{
  DECLARE_DLL_WRAPPER(DllWavPack, DLL_PATH_WAVPACK_CODEC)
  DEFINE_METHOD6(WavpackContext*, WavpackOpenFileInputEx, (stream_reader* p1, void* p2, void* p3, char* p4, int p5, int p6))
  DEFINE_METHOD4(WavpackContext*, WavpackOpenFileInput, (const char* p1, char* p2, int p3, int p4))
  DEFINE_METHOD1(int, WavpackGetVersion, (WavpackContext* p1))
  DEFINE_METHOD3(unsigned int, WavpackUnpackSamples, (WavpackContext * p1, int * p2, unsigned int p3))
  DEFINE_METHOD1(unsigned int, WavpackGetNumSamples, (WavpackContext* p1))
  DEFINE_METHOD1(unsigned int, WavpackGetSampleIndex, (WavpackContext * p1))
  DEFINE_METHOD1(int, WavpackGetNumErrors, (WavpackContext * p1))
  DEFINE_METHOD1(int, WavpackLossyBlocks, (WavpackContext *p1))
  DEFINE_METHOD2(int, WavpackSeekSample, (WavpackContext *p1, unsigned int p2))
  DEFINE_METHOD1(WavpackContext*, WavpackCloseFile, (WavpackContext *p1))
  DEFINE_METHOD1(unsigned int, WavpackGetSampleRate, (WavpackContext *p1))
  DEFINE_METHOD1(int, WavpackGetBitsPerSample, (WavpackContext *p1))
  DEFINE_METHOD1(int, WavpackGetBytesPerSample, (WavpackContext *p1))
  DEFINE_METHOD1(int, WavpackGetNumChannels, (WavpackContext *p1))
  DEFINE_METHOD1(int, WavpackGetReducedChannels, (WavpackContext *p1))
  DEFINE_METHOD2(int, WavpackGetMD5Sum, (WavpackContext *p1, unsigned char p2[16]))
  DEFINE_METHOD1(unsigned int, WavpackGetWrapperBytes, (WavpackContext *p1))
  DEFINE_METHOD1(unsigned char*, WavpackGetWrapperData, (WavpackContext *p1))
  DEFINE_METHOD1(void, WavpackFreeWrapper, (WavpackContext *p1))
  DEFINE_METHOD1(double, WavpackGetProgress, (WavpackContext *p1))
  DEFINE_METHOD1(unsigned int, WavpackGetFileSize, (WavpackContext *p1))
  DEFINE_METHOD1(double, WavpackGetRatio, (WavpackContext *p1))
  DEFINE_METHOD2(double, WavpackGetAverageBitrate, (WavpackContext *p1, int p2))
  DEFINE_METHOD1(double, WavpackGetInstantBitrate, (WavpackContext *p1))
  DEFINE_METHOD4(int, WavpackGetTagItem, (WavpackContext *p1, const char *p2, char *p3, int p4))
  DEFINE_METHOD3(int, WavpackAppendTagItem, (WavpackContext *p1, const char *p2, const char *p3))
  DEFINE_METHOD1(int, WavpackWriteTag, (WavpackContext *p1))
  DEFINE_METHOD3(WavpackContext*, WavpackOpenFileOutput, (blockout_f p1, void *p2, void *p3))
  DEFINE_METHOD3(int, WavpackSetConfiguration, (WavpackContext *p1, WavpackConfig *p2, unsigned int p3))
  DEFINE_METHOD3(int, WavpackAddWrapper, (WavpackContext *p1, void *p2, unsigned int p3))
  DEFINE_METHOD2(int, WavpackStoreMD5Sum, (WavpackContext *p1, unsigned char p2[16]))
  DEFINE_METHOD1(int, WavpackPackInit, (WavpackContext *p1))
  DEFINE_METHOD3(int, WavpackPackSamples, (WavpackContext *p1, int *p2, unsigned int p3))
  DEFINE_METHOD1(int, WavpackFlushSamples, (WavpackContext *p1))
  DEFINE_METHOD2(void, WavpackUpdateNumSamples, (WavpackContext *p1, void *p2))
  DEFINE_METHOD1(void*, WavpackGetWrapperLocation, (void *p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(WavpackOpenFileInputEx)
    RESOLVE_METHOD(WavpackOpenFileInput)
    RESOLVE_METHOD(WavpackGetVersion)
    RESOLVE_METHOD(WavpackUnpackSamples)
    RESOLVE_METHOD(WavpackGetNumSamples)
    RESOLVE_METHOD(WavpackGetSampleIndex)
    RESOLVE_METHOD(WavpackGetNumErrors)
    RESOLVE_METHOD(WavpackLossyBlocks)
    RESOLVE_METHOD(WavpackSeekSample)
    RESOLVE_METHOD(WavpackCloseFile)
    RESOLVE_METHOD(WavpackGetSampleRate)
    RESOLVE_METHOD(WavpackGetBitsPerSample)
    RESOLVE_METHOD(WavpackGetBytesPerSample)
    RESOLVE_METHOD(WavpackGetNumChannels)
    RESOLVE_METHOD(WavpackGetReducedChannels)
    RESOLVE_METHOD(WavpackGetMD5Sum)
    RESOLVE_METHOD(WavpackGetWrapperBytes)
    RESOLVE_METHOD(WavpackGetWrapperData)
    RESOLVE_METHOD(WavpackFreeWrapper)
    RESOLVE_METHOD(WavpackGetProgress)
    RESOLVE_METHOD(WavpackGetFileSize)
    RESOLVE_METHOD(WavpackGetRatio)
    RESOLVE_METHOD(WavpackGetAverageBitrate)
    RESOLVE_METHOD(WavpackGetInstantBitrate)
    RESOLVE_METHOD(WavpackGetTagItem)
    RESOLVE_METHOD(WavpackAppendTagItem)
    RESOLVE_METHOD(WavpackWriteTag)
    RESOLVE_METHOD(WavpackOpenFileOutput)
    RESOLVE_METHOD(WavpackSetConfiguration)
    RESOLVE_METHOD(WavpackAddWrapper)
    RESOLVE_METHOD(WavpackStoreMD5Sum)
    RESOLVE_METHOD(WavpackPackInit)
    RESOLVE_METHOD(WavpackPackSamples)
    RESOLVE_METHOD(WavpackFlushSamples)
    RESOLVE_METHOD(WavpackUpdateNumSamples)
    RESOLVE_METHOD(WavpackGetWrapperLocation)
  END_METHOD_RESOLVE()
};
