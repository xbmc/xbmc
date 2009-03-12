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

#include "Util/SimpleBuffer.h"
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
  }

  void lap_start()
  {
    QueryPerformanceCounter((LARGE_INTEGER*)&last_sample_start);
  }

  void lap_end()
  {
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

#define MA_ERROR            0
#define MA_SUCCESS          1
#define MA_NOT_IMPLEMENTED  2
#define MA_BUSYORFULL       3
#define MA_NEED_DATA        4
#define MA_TYPE_MISMATCH    5
#define MA_NOTFOUND         6
#define MA_NOT_SUPPORTED    7

#define MA_STREAM_NONE NULL
#define MA_MAX_INPUT_STREAMS 4

#define MA_UNKNOWN_CONTROL 0x0000
#define MA_CONTROL_STOP 0x0001
#define MA_CONTROL_PLAY 0x0002
#define MA_CONTROL_PAUSE 0x0003
#define MA_CONTROL_RESUME 0x0004
#define MA_MAX_CONTROL AM_CONTROL_RESUME

typedef unsigned int MA_ATTRIB_ID;

// TODO: Document Attributes and points of use

// Attribute Types
#define MA_ATT_TYPE_STREAM_FORMAT   0x0001
#define MA_ATT_TYPE_BITDEPTH        0x0002
#define MA_ATT_TYPE_CHANNELS        0x0003
#define MA_ATT_TYPE_SAMPLESPERSEC   0x0004
#define MA_ATT_TYPE_ENCODING        0x0005
#define MA_ATT_TYPE_PASSTHROUGH     0x0006

// MA_ATT_TYPE_STREAM_FORMAT Values
#define MA_STREAM_FORMAT_PCM      0x0001
#define MA_STREAM_FORMAT_FLOAT    0x0002
#define MA_STREAM_FORMAT_ENCODED  0x0003

// MA_ATT_TYPE_ENCODING Values
#define MA_STREAM_ENCODING_AC3    0x0001
#define MA_STREAM_ENCODING_DTS    0x0002


enum stream_attribute_type
{
  stream_attribute_int,
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
    bool boolVal;
    int intVal;
    float floatVal;
    char* stringVal;
    void* ptrVal;
  };
};

// TODO: This is temporary. Remove when global allocator is implemented.
extern unsigned __int64 audio_slice_id;
struct audio_slice
{
  // TODO: Determine appropriate information to pass along with the slice
  struct _tag_header{
    unsigned __int64 id;
    unsigned __int64 timestamp;
    size_t data_len;
  } header;
  unsigned int data[1]; // Placeholder (TODO: This is hackish)

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

typedef std::map<MA_ATTRIB_ID,stream_attribute>::iterator StreamAttributeIterator; 
class CStreamAttributeCollection
{
public:
  CStreamAttributeCollection();
  virtual ~CStreamAttributeCollection();
  MA_RESULT GetInt(MA_ATTRIB_ID id, int* pVal);
  MA_RESULT GetFloat(MA_ATTRIB_ID id, float* pVal);
  MA_RESULT GetString(MA_ATTRIB_ID id, char** pVal);
  MA_RESULT GetPtr(MA_ATTRIB_ID id, void** pVal);
  MA_RESULT GetBool(MA_ATTRIB_ID id, bool* pVal);

  MA_RESULT SetInt(MA_ATTRIB_ID id, int val);
  MA_RESULT SetFloat(MA_ATTRIB_ID id, float val);
  MA_RESULT SetString(MA_ATTRIB_ID id, char* val);
  MA_RESULT SetPtr(MA_ATTRIB_ID id, void* val);
  MA_RESULT SetBool(MA_ATTRIB_ID id, bool val);
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

struct audio_data_transfer_props
{
  size_t transfer_alignment; // Minimum transfer amount, increment
  size_t preferred_transfer_size;
  size_t max_transfer_size;
};

// Common Interfaces
////////////////////////////////////////////
class IAudioSink
{
public:
  virtual MA_RESULT TestInputFormat(CStreamDescriptor* pDesc) = 0;
  virtual MA_RESULT SetInputFormat(CStreamDescriptor* pDesc) = 0;
  virtual MA_RESULT GetInputProperties(audio_data_transfer_props* pProps) = 0;
  virtual MA_RESULT AddSlice(audio_slice* pSlice) = 0;
  virtual float GetMaxLatency() = 0;
  virtual void Flush() = 0;
protected:
  IAudioSink() {}
};

class IAudioSource
{
public:
  virtual MA_RESULT TestOutputFormat(CStreamDescriptor* pDesc) = 0;
  virtual MA_RESULT SetOutputFormat(CStreamDescriptor* pDesc) = 0;
  virtual MA_RESULT GetOutputProperties(audio_data_transfer_props* pProps) = 0;
  virtual MA_RESULT GetSlice(audio_slice** ppSlice) = 0;
protected:
  IAudioSource() {}
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
protected:
  IAudioMixer() {}
};

// Input Stage
////////////////////////////////////////////
class CStreamInput : IAudioSource
{
public:
  CStreamInput();
  virtual ~CStreamInput();

  // IAudioSource
  MA_RESULT TestOutputFormat(CStreamDescriptor* pDesc);
  MA_RESULT SetOutputFormat(CStreamDescriptor* pDesc);
  MA_RESULT GetOutputProperties(audio_data_transfer_props* pProps);
  MA_RESULT GetSlice(audio_slice** pSlice);

  IAudioSource* GetSource();
  MA_RESULT AddData(void* pBuffer, size_t bufLen);  // Writes all or nothing
  void Reset();
protected:
  audio_slice* m_pSlice;
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

struct dsp_filter_node
{
  IDSPFilter* filter;
  dsp_filter_node* prev;
  dsp_filter_node* next;
};

class CDSPChain : public IDSPFilter
{
public:
  CDSPChain();
  virtual ~CDSPChain();

  IAudioSink* GetSink();
  IAudioSource* GetSource();

  // IDSPFilter
  void Close();

  // IAudioSource
  MA_RESULT TestOutputFormat(CStreamDescriptor* pDesc);
  MA_RESULT SetOutputFormat(CStreamDescriptor* pDesc);
  MA_RESULT GetOutputProperties(audio_data_transfer_props* pProps);
  MA_RESULT GetSlice(audio_slice** pSlice);

  // IAudioSink
  MA_RESULT TestInputFormat(CStreamDescriptor* pDesc);
  MA_RESULT SetInputFormat(CStreamDescriptor* pDesc);
  MA_RESULT GetInputProperties(audio_data_transfer_props* pProps);
  MA_RESULT AddSlice(audio_slice* pSlice);
  float GetMaxLatency();
  void Flush();

  MA_RESULT CreateFilterGraph(CStreamDescriptor* pInDesc, CStreamDescriptor* pOutDesc);
protected:
  void DisposeGraph();
  audio_slice* m_pInputSlice;
  bool m_Passthrough;
  dsp_filter_node* m_pHead;
  dsp_filter_node* m_pTail;
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
protected:
  int m_MaxChannels;
  int m_ActiveChannels;
  IMixerChannel** m_pChannel;
};

// Interconnects
////////////////////////////////////////////
class CSliceQueue
{
public:
  CSliceQueue();
  virtual ~CSliceQueue();
  void Push(audio_slice* pSlice);
  audio_slice* GetSlice(size_t align, size_t maxSize);
  size_t GetTotalBytes() {return m_TotalBytes + m_RemainderSize;}
protected:
  audio_slice* Pop(); // Does not respect remainder, so it must be private
  std::queue<audio_slice*> m_Slices;
  size_t m_TotalBytes;
  audio_slice* m_pPartialSlice;
  size_t m_RemainderSize;
};

class CAudioDataInterconnect
{
public:
  CAudioDataInterconnect();
  virtual ~CAudioDataInterconnect();
  MA_RESULT Link(IAudioSource* pSource, IAudioSink* pSink);
  void Unlink();
  MA_RESULT Process();
  void Flush();
  size_t GetCacheSize();
protected:
  IAudioSource* m_pSource; // Input
  IAudioSink* m_pSink; // Output
  audio_slice* m_pSlice; // Holding
  CSliceQueue m_Queue;
  audio_data_transfer_props m_InputProps;
  audio_data_transfer_props m_OutputProps;
};

// Performance Monitoring
////////////////////////////////////////////
class CAudioPerformanceMonitor
{
public:
  bool RegisterCollectionPoint(CStdString name,void* pObj);
protected:

};

#endif // __AUDIO_LIB_CORE_H__