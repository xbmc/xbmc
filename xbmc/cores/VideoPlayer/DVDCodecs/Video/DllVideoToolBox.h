#pragma once

/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DynamicDll.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#pragma pack(push, 4)

//-----------------------------------------------------------------------------------
// /System/Library/PrivateFrameworks/VideoToolbox.framework
enum VTFormat {
  kVTFormatJPEG         = 'jpeg', // kCMVideoCodecType_JPEG
  kVTFormatH264         = 'avc1', // kCMVideoCodecType_H264 (MPEG-4 Part 10))
  kVTFormatMPEG4Video   = 'mp4v', // kCMVideoCodecType_MPEG4Video (MPEG-4 Part 2)
  kVTFormatMPEG2Video   = 'mp2v'  // kCMVideoCodecType_MPEG2Video
};
enum {
  kVTDecoderNoErr = 0,
  kVTDecoderHardwareNotSupportedErr = -12470,
  kVTDecoderFormatNotSupportedErr = -12471,
  kVTDecoderConfigurationError = -12472,
  kVTDecoderDecoderFailedErr = -12473,
};
enum {
  kVTDecodeInfo_Asynchronous = 1UL << 0,
  kVTDecodeInfo_FrameDropped = 1UL << 1
};
enum {
  // tells the decoder not to bother returning a CVPixelBuffer
  // in the outputCallback. The output callback will still be called.
  kVTDecoderDecodeFlags_DontEmitFrame = 1 << 1,
};
enum {
  // decode and return buffers for all frames currently in flight.
  kVTDecoderFlush_EmitFrames = 1 << 0		
};

typedef UInt32 VTFormatId;
typedef CFTypeRef VTDecompressionSessionRef;

typedef void (*VTDecompressionOutputCallbackFunc)(
  void            *refCon,
  CFDictionaryRef frameInfo,
  OSStatus        status,
  UInt32          infoFlags,
  CVBufferRef     imageBuffer);

typedef struct _VTDecompressionOutputCallback VTDecompressionOutputCallback;
struct _VTDecompressionOutputCallback {
  VTDecompressionOutputCallbackFunc callback;
  void *refcon;
};

extern CFStringRef kVTVideoDecoderSpecification_EnableSandboxedVideoDecoder;

OSStatus VTDecompressionSessionCreate(
  CFAllocatorRef allocator,
  CMFormatDescriptionRef videoFormatDescription,
  CFTypeRef sessionOptions,
  CFDictionaryRef destinationPixelBufferAttributes,
  VTDecompressionOutputCallback *outputCallback,
  VTDecompressionSessionRef *session);

OSStatus VTDecompressionSessionDecodeFrame(
  VTDecompressionSessionRef session, CMSampleBufferRef sbuf,
  uint32_t decoderFlags, CFDictionaryRef frameInfo, uint32_t unk1);

OSStatus VTDecompressionSessionCopyProperty(VTDecompressionSessionRef session, CFTypeRef key, void* unk, CFTypeRef * value);
OSStatus VTDecompressionSessionCopySupportedPropertyDictionary(VTDecompressionSessionRef session, CFDictionaryRef * dict);
OSStatus VTDecompressionSessionSetProperty(VTDecompressionSessionRef session, CFStringRef propName, CFTypeRef propValue);
void VTDecompressionSessionInvalidate(VTDecompressionSessionRef session);
OSStatus VTDecompressionSessionWaitForAsynchronousFrames(VTDecompressionSessionRef session);
#pragma pack(pop)
    
#if defined(__cplusplus)
}
#endif

class DllVideoToolBoxInterface
{
public:
  virtual ~DllVideoToolBoxInterface() {}

  virtual OSStatus VTDecompressionSessionCreate(CFAllocatorRef allocator, CMFormatDescriptionRef videoFormatDescription, CFTypeRef sessionOptions, CFDictionaryRef destinationPixelBufferAttributes, VTDecompressionOutputCallback *outputCallback, VTDecompressionSessionRef *session) = 0;
  virtual OSStatus VTDecompressionSessionDecodeFrame(VTDecompressionSessionRef session, CMSampleBufferRef sbuf, uint32_t decoderFlags, CFDictionaryRef frameInfo, uint32_t unk1) = 0;
  virtual OSStatus VTDecompressionSessionCopyProperty(VTDecompressionSessionRef session, CFTypeRef key, void* unk, CFTypeRef * value) = 0;
  virtual OSStatus VTDecompressionSessionCopySupportedPropertyDictionary(VTDecompressionSessionRef session, CFDictionaryRef * dict) = 0;
  virtual OSStatus VTDecompressionSessionSetProperty(VTDecompressionSessionRef session, CFStringRef propName, CFTypeRef propValue) = 0;
  virtual void VTDecompressionSessionInvalidate(VTDecompressionSessionRef session) = 0;
  virtual OSStatus VTDecompressionSessionWaitForAsynchronousFrames(VTDecompressionSessionRef session) = 0;
};

// load from private framework path - needed for ios 5.x
class DllVideoToolBoxPrivate : public DllDynamic, public DllVideoToolBoxInterface
{
  DECLARE_DLL_WRAPPER(DllVideoToolBoxPrivate, "/System/Library/PrivateFrameworks/VideoToolbox.framework/VideoToolbox")
  DEFINE_METHOD6(OSStatus, VTDecompressionSessionCreate, (CFAllocatorRef p1, CMFormatDescriptionRef p2, CFTypeRef p3, CFDictionaryRef p4, VTDecompressionOutputCallback *p5, VTDecompressionSessionRef *p6))
  DEFINE_METHOD5(OSStatus, VTDecompressionSessionDecodeFrame, (VTDecompressionSessionRef p1, CMSampleBufferRef p2, uint32_t p3, CFDictionaryRef p4, uint32_t p5))
  DEFINE_METHOD4(OSStatus, VTDecompressionSessionCopyProperty, (VTDecompressionSessionRef p1, CFTypeRef p2, void* p3, CFTypeRef * p4))
  DEFINE_METHOD2(OSStatus, VTDecompressionSessionCopySupportedPropertyDictionary, (VTDecompressionSessionRef p1, CFDictionaryRef * p2))
  DEFINE_METHOD3(OSStatus, VTDecompressionSessionSetProperty, (VTDecompressionSessionRef p1, CFStringRef p2, CFTypeRef p3))
  DEFINE_METHOD1(void, VTDecompressionSessionInvalidate, (VTDecompressionSessionRef p1))
  DEFINE_METHOD1(OSStatus, VTDecompressionSessionWaitForAsynchronousFrames, (VTDecompressionSessionRef p1))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(VTDecompressionSessionCreate, VTDecompressionSessionCreate)
    RESOLVE_METHOD_RENAME(VTDecompressionSessionDecodeFrame, VTDecompressionSessionDecodeFrame)
    RESOLVE_METHOD_RENAME(VTDecompressionSessionCopyProperty, VTDecompressionSessionCopyProperty)
    RESOLVE_METHOD_RENAME(VTDecompressionSessionCopySupportedPropertyDictionary, VTDecompressionSessionCopySupportedPropertyDictionary)
    RESOLVE_METHOD_RENAME(VTDecompressionSessionSetProperty, VTDecompressionSessionSetProperty)
    RESOLVE_METHOD_RENAME(VTDecompressionSessionInvalidate, VTDecompressionSessionInvalidate)
    RESOLVE_METHOD_RENAME(VTDecompressionSessionWaitForAsynchronousFrames, VTDecompressionSessionWaitForAsynchronousFrames)
  END_METHOD_RESOLVE()
};

// load from public framework path -> available for >= ios 6.0
class DllVideoToolBoxPublic : public DllDynamic, public DllVideoToolBoxInterface
{
  DECLARE_DLL_WRAPPER(DllVideoToolBoxPublic, "/System/Library/Frameworks/VideoToolbox.framework/VideoToolbox")
  DEFINE_METHOD6(OSStatus, VTDecompressionSessionCreate, (CFAllocatorRef p1, CMFormatDescriptionRef p2, CFTypeRef p3, CFDictionaryRef p4, VTDecompressionOutputCallback *p5, VTDecompressionSessionRef *p6))
  DEFINE_METHOD5(OSStatus, VTDecompressionSessionDecodeFrame, (VTDecompressionSessionRef p1, CMSampleBufferRef p2, uint32_t p3, CFDictionaryRef p4, uint32_t p5))
  DEFINE_METHOD4(OSStatus, VTDecompressionSessionCopyProperty, (VTDecompressionSessionRef p1, CFTypeRef p2, void* p3, CFTypeRef * p4))
  DEFINE_METHOD2(OSStatus, VTDecompressionSessionCopySupportedPropertyDictionary, (VTDecompressionSessionRef p1, CFDictionaryRef * p2))
  DEFINE_METHOD3(OSStatus, VTDecompressionSessionSetProperty, (VTDecompressionSessionRef p1, CFStringRef p2, CFTypeRef p3))
  DEFINE_METHOD1(void, VTDecompressionSessionInvalidate, (VTDecompressionSessionRef p1))
  DEFINE_METHOD1(OSStatus, VTDecompressionSessionWaitForAsynchronousFrames, (VTDecompressionSessionRef p1))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(VTDecompressionSessionCreate, VTDecompressionSessionCreate)
    RESOLVE_METHOD_RENAME(VTDecompressionSessionDecodeFrame, VTDecompressionSessionDecodeFrame)
    RESOLVE_METHOD_RENAME(VTDecompressionSessionCopyProperty, VTDecompressionSessionCopyProperty)
    RESOLVE_METHOD_RENAME(VTDecompressionSessionCopySupportedPropertyDictionary, VTDecompressionSessionCopySupportedPropertyDictionary)
    RESOLVE_METHOD_RENAME(VTDecompressionSessionSetProperty, VTDecompressionSessionSetProperty)
    RESOLVE_METHOD_RENAME(VTDecompressionSessionInvalidate, VTDecompressionSessionInvalidate)
    RESOLVE_METHOD_RENAME(VTDecompressionSessionWaitForAsynchronousFrames, VTDecompressionSessionWaitForAsynchronousFrames)
  END_METHOD_RESOLVE()
};

