/*
 *      Copyright (C) 2009 phi2039
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

#include "SimpleBuffer.h"
#include <map>

using namespace std;

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

#define MA_STREAM_NONE NULL
#define MA_MAX_INPUT_STREAMS 4

#define MA_UNKNOWN_CONTROL 0x0000
#define MA_CONTROL_STOP 0x0001
#define MA_CONTROL_PLAY 0x0002
#define MA_CONTROL_PAUSE 0x0003
#define MA_CONTROL_RESUME 0x0004
#define MA_MAX_CONTROL AM_CONTROL_RESUME

typedef unsigned int MA_ATTRIB_ID;

// Attribute Types
#define MA_ATT_TYPE_STREAM_FORMAT   0x0001
#define MA_ATT_TYPE_BITDEPTH        0x0002
#define MA_ATT_TYPE_CHANNELS        0x0003
#define MA_ATT_TYPE_SAMPLESPERSEC   0x0004

// MA_ATT_TYPE_STREAM_FORMAT Values
#define MA_STREAM_FORMAT_PCM      0x0001
#define MA_STREAM_FORMAT_FLOAT    0x0002
#define MA_STREAM_FORMAT_ENCODED  0x0003

enum stream_attribute_type
{
  stream_attribute_int,
  stream_attribute_float,
  stream_attribute_string,
  stream_attribute_ptr
};

struct stream_attribute
{
  stream_attribute_type type;
  union
  {
    int intVal;
    float floatVal;
    char* stringVal;
    void* ptrVal;
  };
};

// TODO: This is temporary
extern unsigned __int64 audio_slice_id;
struct audio_slice
{
  // TODO: Determine appropriate information to pass along with the slice
  // Slice header
  struct _tag_header{
    unsigned __int64 id;
    unsigned __int64 timestamp;
    size_t data_len;
  } header;
  unsigned int data[1]; // Placeholder (TODO: This is hackish)

  // TODO: All of this is a hack
  unsigned char* get_data() {return (unsigned char*)&data;}
  static audio_slice* create_slice(size_t data_len)
  {
    audio_slice* p = (audio_slice*)(new BYTE[sizeof(audio_slice::_tag_header) + data_len]);
    p->header.data_len = data_len;
    p->header.id = ++audio_slice_id;
    return p;
  }
};

typedef map<MA_ATTRIB_ID,stream_attribute>::iterator StreamAttributeIterator; 
class CStreamAttributeCollection
{
public:
  CStreamAttributeCollection();
  virtual ~CStreamAttributeCollection();
  MA_RESULT GetInt(MA_ATTRIB_ID id, int* pVal);
  MA_RESULT GetFloat(MA_ATTRIB_ID id, float* pVal);
  MA_RESULT GetString(MA_ATTRIB_ID id, char** pVal);
  MA_RESULT GetPtr(MA_ATTRIB_ID id, void** pVal);

  MA_RESULT SetInt(MA_ATTRIB_ID id, int val);
  MA_RESULT SetFloat(MA_ATTRIB_ID id, float val);
  MA_RESULT SetString(MA_ATTRIB_ID id, char* val);
  MA_RESULT SetPtr(MA_ATTRIB_ID id, void* val);
protected:
  map<MA_ATTRIB_ID,stream_attribute> m_Attributes;
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
class IAudioSink
{
public:
  virtual MA_RESULT TestInputFormat(CStreamDescriptor* pDesc) = 0;
  virtual MA_RESULT SetInputFormat(CStreamDescriptor* pDesc) = 0;
  virtual MA_RESULT AddSlice(audio_slice* pSlice) = 0;
protected:
  IAudioSink() {}
};

class IAudioSource
{
public:
  virtual MA_RESULT TestOutputFormat(CStreamDescriptor* pDesc) = 0;
  virtual MA_RESULT SetOutputFormat(CStreamDescriptor* pDesc) = 0;
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
protected:
  IAudioMixer() {}
};

// Input Stage
////////////////////////////////////////////
class CStreamInput
{
public:
  CStreamInput(size_t inputBlockSize);
  virtual ~CStreamInput();

  // IAudioSource
  MA_RESULT TestOutputFormat(CStreamDescriptor* pDesc);
  MA_RESULT SetOutputFormat(CStreamDescriptor* pDesc);
  MA_RESULT GetSlice(audio_slice** pSlice);

  MA_RESULT Initialize(CStreamDescriptor* pDesc);
  void SetInputBlockSize(size_t size);
  void SetOutputSize(size_t size);
  bool CanAcceptData();
  MA_RESULT AddData(void* pBuffer, size_t bufLen);  // Writes all or nothing
  IAudioSource* GetSource();
protected:
  size_t m_InputBlockSize;
  size_t m_OutputSize;
  CSimpleBuffer m_Buffer;
  unsigned int m_BufferOffset;
  void ConfigureBuffer();
};

// Processing Stage
////////////////////////////////////////////
class IDSPFilter : public IAudioSource, public IAudioSink
{
public:
protected:
  IDSPFilter() {}
};

class CDSPChain : public IDSPFilter
{
public:
  CDSPChain();
  virtual ~CDSPChain();

  IAudioSink* GetSink();
  IAudioSource* GetSource();

  // IAudioSource
  MA_RESULT TestOutputFormat(CStreamDescriptor* pDesc);
  MA_RESULT SetOutputFormat(CStreamDescriptor* pDesc);
  MA_RESULT GetSlice(audio_slice** pSlice);

  // IAudioSink
  MA_RESULT TestInputFormat(CStreamDescriptor* pDesc);
  MA_RESULT SetInputFormat(CStreamDescriptor* pDesc);
  MA_RESULT AddSlice(audio_slice* pSlice);

  MA_RESULT CreateFilterGraph(CStreamDescriptor* pDesc);
protected:
  audio_slice* m_pInputSlice;
};

// Output Stage
////////////////////////////////////////////
class CPassthroughMixer : public IAudioMixer
{
public:
  CPassthroughMixer(int maxChannels);
  virtual ~CPassthroughMixer();
  int OpenChannel(CStreamDescriptor* pDesc);
  void CloseChannel(int channel);
  MA_RESULT ControlChannel(int channel, int controlCode);
  MA_RESULT SetChannelVolume(int channel, long vol);
  IAudioSink* GetChannelSink(int channel);
  int GetActiveChannels() {return m_ActiveChannels;}
  int GetMaxChannels() {return m_MaxChannels;}
protected:
  int m_MaxChannels;
  int m_ActiveChannels;
  IMixerChannel** m_pChannel;
};

// Utility Classes
////////////////////////////////////////////


#endif // __AUDIO_LIB_CORE_H__