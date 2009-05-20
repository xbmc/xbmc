/*
 *      Copyright (C) 2009 Team XBMC
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

#ifndef __AUDIO_LIB_CORE_H__
#define __AUDIO_LIB_CORE_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <map>
#include <queue>

// Util
////////////////////////////////////////////

// Lap timer with a stored running average
struct lap_timer
{
  float average_time;
  unsigned int sample_count;
  unsigned __int64 last_sample_start;
  unsigned __int64 last_sample_end;
  unsigned __int64 last_lap_time;

  void reset() 
  {
    average_time = 0.0f;
    sample_count = 0;
    last_sample_start = 0;
    last_sample_end = 0;
    last_lap_time = 0;
  }

  void lap_start()
  {
    QueryPerformanceCounter((LARGE_INTEGER*)&last_sample_start);
  }

  void lap_end()
  {
    if (!last_sample_start) return;
    QueryPerformanceCounter((LARGE_INTEGER*)&last_sample_end);
    last_lap_time = last_sample_end - last_sample_start;
    average_time += ((float)last_lap_time - average_time)/(float)++sample_count;
  }

  unsigned __int64 elapsed_time()
  {
    __int64 now = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&now);
    return now - last_sample_start;
  }
};  // All times in ns

// Primitives
////////////////////////////////////////////
typedef unsigned int MA_RESULT;

// MA_RESULT Values (Error Codes)
enum
{
 MA_ERROR             = 0,
 MA_SUCCESS           = 1,
 MA_NOT_IMPLEMENTED   = 2,
 MA_BUSYORFULL        = 3,
 MA_NEED_DATA         = 4,
 MA_TYPE_MISMATCH     = 5,
 MA_NOTFOUND          = 6,
 MA_NOT_SUPPORTED     = 7,
 MA_MISSING_ATTRIBUTE = 8,
 MA_INVALID_BUS       = 9,
 MA_NO_SOURCE         =10
};

#define MA_STREAM_NONE        0
#define MA_MAX_INPUT_STREAMS  4

// Transport Control Codes
enum
{
  MA_UNKNOWN_CONTROL = 0x0000,
  MA_CONTROL_STOP    = 0x0001,
  MA_CONTROL_PLAY    = 0x0002,
  MA_CONTROL_PAUSE   = 0x0003,
  MA_CONTROL_RESUME  = 0x0004
};

typedef unsigned int MA_ATTRIB_ID;

// TODO: Document Attributes and points of use

////////////////////////////////////////////////////////////////////////////////
// Attribute Types
////////////////////////////////////////////////////////////////////////////////
// Each AudioStream must have the following attibutes set to be considered valid.
enum
{
  MA_ATT_TYPE_STREAM_FLAGS,     // type: int (bitfield) 
  MA_ATT_TYPE_BYTES_PER_SEC,    // type: int
  MA_ATT_TYPE_BYTES_PER_FRAME,  // type: int
  MA_ATT_TYPE_STREAM_FORMAT,    // type: int (The value of the this attribute defines what other attibutes are valid/required)
  
  // Linear PCM Format Attributes
  MA_ATT_TYPE_LPCM_FLAGS,       // type: int (bitfield)
  MA_ATT_TYPE_SAMPLE_TYPE,      // type: int
  MA_ATT_TYPE_BITDEPTH,         // type: int
  MA_ATT_TYPE_SAMPLERATE,       // type: int
  MA_ATT_TYPE_CHANNEL_COUNT,    // type: int
  MA_ATT_TYPE_CHANNEL_MAP,      // type: int64
  
  // IEC61937(AC3/DTS over S/PDIF) Format Attributes
  MA_ATT_TYPE_ENCODING          // type: int
};

// MA_ATT_TYPE_STREAM_FORMAT Values
enum
{
  MA_STREAM_FORMAT_LPCM,
  MA_STREAM_FORMAT_IEC61937
};

// MA_ATT_TYPE_STREAM_FLAGS Values
enum
{
  MA_STREAM_FLAG_NONE     = (1L << 0),
  MA_STREAM_FLAG_LOCKED   = (1L << 1),  // The stream data is not to be modified by the audio chain in any way 
  MA_STREAM_FLAG_VBR      = (1L << 2)
};

// MA_ATT_TYPE_LPCM_FLAGS Values
enum
{
  MA_LPCM_FLAG_NONE         = (1L << 0),
  MA_LPCM_FLAG_INTERLEAVED  = (1L << 1)
};

// MA_ATT_TYPE_SAMPLE_TYPE Values
enum
{
  MA_SAMPLE_TYPE_SINT,
  MA_SAMPLE_TYPE_UINT,
  MA_SAMPLE_TYPE_FLOAT
};

// MA_ATT_TYPE_ENCODING Values
enum
{
  MA_STREAM_ENCODING_AC3,
  MA_STREAM_ENCODING_DTS
};

// Audio Channel Locations
#define MA_CHANNEL_FRONT_LEFT     0x0
#define MA_CHANNEL_FRONT_RIGHT    0x1
#define MA_CHANNEL_REAR_LEFT      0x2
#define MA_CHANNEL_REAR_RIGHT     0x3
#define MA_CHANNEL_FRONT_CENTER   0x4
#define MA_CHANNEL_LFE            0x5
#define MA_CHANNEL_SIDE_LEFT      0x6
#define MA_CHANNEL_SIDE_RIGHT     0x7
#define MA_CHANNEL_REAR_CENTER    0x8
#define MA_CHANNEL_FRONT_LOC      MA_CHANNEL_SIDE_LEFT // Front Left of Center (7.1 Variant)
#define MA_CHANNEL_FRONT_ROC      MA_CHANNEL_SIDE_RIGHT // Front Right of Center (7.1 Variant)
#define MA_MAX_CHANNELS           0x9

enum stream_attribute_type
{
  stream_attribute_bitfield,
  stream_attribute_int,
  stream_attribute_int64,
  stream_attribute_float,
  stream_attribute_string,
  stream_attribute_ptr,
  stream_attribute_bool
};

struct stream_attribute
{
  stream_attribute_type type;
  union
  {
    unsigned int bitfieldVal;
    bool boolVal;
    int intVal;
    __int64 int64Val;
    float floatVal;
    char* stringVal;
    void* ptrVal;
  };
};

typedef unsigned __int64 ma_timestamp; // In nanoseconds

// TODO: This is temporary. Remove when global allocator is implemented.
extern unsigned __int64 audio_slice_id;
struct audio_slice
{
  // TODO: Determine appropriate information to pass along with the slice
  struct _tag_header{
    unsigned __int64 id;
    ma_timestamp timestamp;
    size_t data_len;
  } header;
  unsigned int data[1]; // Ensure compatibility with compilers that don't support variable length arrays

  // TODO: All of this is a hack
  unsigned char* get_data() {return (unsigned char*)&data;}
  void copy_to(unsigned char* pBuf) {memcpy(pBuf,&data,header.data_len);}
  static audio_slice* create_slice(size_t data_len)
  {
    audio_slice* p = (audio_slice*)(new BYTE[sizeof(audio_slice::_tag_header) + data_len]);
    p->header.data_len = data_len;
    p->header.id = ++audio_slice_id;
    return p;
  }
};

// An ma_audio_buffer is used to move data between elements in the framework.
struct ma_audio_buffer
{
  size_t allocated_len;
  size_t data_len;
  unsigned char data[1]; // Ensure compatibility with compilers that don't support variable length arrays
};

// The ma_audio_container structure provides a means to package multiple ma_audio_buffer
// structures together for transport.
struct ma_audio_container
{
  unsigned int buffer_count;
  ma_audio_buffer buffer[1]; // Ensure compatibility with compilers that don't support variable length arrays
};

typedef std::map<MA_ATTRIB_ID,stream_attribute>::iterator StreamAttributeIterator; 
class CStreamAttributeCollection
{
public:
  CStreamAttributeCollection();
  virtual ~CStreamAttributeCollection();
  MA_RESULT GetInt(MA_ATTRIB_ID id, int* pVal);
  MA_RESULT GetInt64(MA_ATTRIB_ID id, __int64* pVal);
  MA_RESULT GetFloat(MA_ATTRIB_ID id, float* pVal);
  MA_RESULT GetString(MA_ATTRIB_ID id, char** pVal);
  MA_RESULT GetPtr(MA_ATTRIB_ID id, void** pVal);
  MA_RESULT GetBool(MA_ATTRIB_ID id, bool* pVal);
  MA_RESULT GetFlag(MA_ATTRIB_ID id, int flag, bool* pVal);
  MA_RESULT SetInt(MA_ATTRIB_ID id, int val);
  MA_RESULT SetInt64(MA_ATTRIB_ID id, __int64 val);
  MA_RESULT SetFloat(MA_ATTRIB_ID id, float val);
  MA_RESULT SetString(MA_ATTRIB_ID id, char* val);
  MA_RESULT SetPtr(MA_ATTRIB_ID id, void* val);
  MA_RESULT SetBool(MA_ATTRIB_ID id, bool val);
  MA_RESULT SetFlag(MA_ATTRIB_ID id, int flag, bool val);
protected:
  std::map<MA_ATTRIB_ID,stream_attribute> m_Attributes;
  stream_attribute* FindAttribute(MA_ATTRIB_ID id);
};

class CStreamDescriptor
{
public:
  CStreamAttributeCollection* GetAttributes() {return &m_Attributes;}
protected:
  CStreamAttributeCollection m_Attributes;
};

// Common Interfaces
////////////////////////////////////////////
class IAudioSource
{
public:
  virtual MA_RESULT TestOutputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0) = 0;
  virtual MA_RESULT SetOutputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0) = 0;
  virtual MA_RESULT Render(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus = 0) = 0;
protected:
  IAudioSource() {}
};

class IAudioSink
{
public:
  virtual MA_RESULT TestInputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0) = 0;
  virtual MA_RESULT SetInputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0) = 0;
  virtual MA_RESULT SetSource(IAudioSource* pSource, unsigned int sourceBus = 0, unsigned int sinkBus = 0) = 0;
  virtual float GetMaxLatency() = 0; // TODO: This is the wrong place for this
  virtual void Flush() = 0; // TODO: This is the wrong place for this
protected:
  IAudioSink() {}
};

class IRenderingControl
{
public:
  virtual void Stop() = 0;
  virtual void Play() = 0;
  virtual void Pause() = 0;
  virtual void Resume() = 0;
  virtual void SetVolume(long vol) = 0;
protected:
  IRenderingControl() {}
};

class IMixerChannel : public IAudioSink, public IRenderingControl
{
public:
  virtual void Close() = 0;
  virtual bool IsIdle() = 0;
  virtual bool Drain(unsigned int timeout) = 0;
  virtual void Render() = 0;
protected:
  IMixerChannel() {};
};

class IAudioMixer
{
public:
  virtual int OpenChannel(CStreamDescriptor* pDesc) = 0;
  virtual void CloseChannel(int channel) = 0;
  virtual MA_RESULT ControlChannel(int channel, int controlCode) = 0;
  virtual MA_RESULT SetChannelVolume(int channel, long vol) = 0;
  virtual int GetActiveChannels() = 0;
  virtual int GetMaxChannels() = 0;
  virtual IAudioSink* GetChannelSink(int channel) = 0;
  virtual float GetMaxChannelLatency(int channel) = 0;
  virtual void FlushChannel(int channel) = 0;
  virtual bool DrainChannel(int channel, unsigned int timeout) = 0;
  virtual void Render() = 0;
protected:
  IAudioMixer() {}
};

// Input Stage
////////////////////////////////////////////
class CStreamInput : public IAudioSource
{
public:
  CStreamInput();
  virtual ~CStreamInput();

  // IAudioSource
  MA_RESULT TestOutputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  MA_RESULT SetOutputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  MA_RESULT Render(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus = 0);

  MA_RESULT AddData(void* pBuffer, size_t bufLen);  // Writes all or nothing
  void Reset();
protected:
};

// Processing Stage
////////////////////////////////////////////
class IDSPFilter : public IAudioSource, public IAudioSink
{
public:
  virtual void Close() = 0;
protected:
  IDSPFilter() {}
};

// Output Stage
////////////////////////////////////////////
class CHardwareMixer : public IAudioMixer
{
public:
  CHardwareMixer(int maxChannels);
  virtual ~CHardwareMixer();
  int OpenChannel(CStreamDescriptor* pDesc);
  void CloseChannel(int channel);
  MA_RESULT ControlChannel(int channel, int controlCode);
  MA_RESULT SetChannelVolume(int channel, long vol);
  IAudioSink* GetChannelSink(int channel);
  int GetActiveChannels() {return m_ActiveChannels;}
  int GetMaxChannels() {return m_MaxChannels;}
  float GetMaxChannelLatency(int channel);
  void FlushChannel(int channel);
  bool DrainChannel(int channel, unsigned int timeout);
  void Render();
protected:
  int m_MaxChannels;
  int m_ActiveChannels;
  IMixerChannel** m_pChannel;
};


// Base Classes
////////////////////////////////////////////



// Performance Monitoring
////////////////////////////////////////////
class CAudioPerformanceMonitor
{
public:
  bool RegisterCollectionPoint(CStdString name,void* pObj);
protected:

};

#endif // __AUDIO_LIB_CORE_H__