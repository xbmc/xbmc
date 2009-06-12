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

#ifndef __AUDIO_MANAGER_H__
#define __AUDIO_MANAGER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MasterAudioCore.h"
#include "DSPChain.h"
#include "AudioStream.h"
#include <vector>
#include "Thread.h"

typedef void* MA_STREAM_ID;

#define MA_MIXER_HARDWARE 1
#define MA_MIXER_SOFTWARE 2

typedef std::vector<CStreamDescriptor*> StreamDescriptorList;

class CAudioProfile
{
public:
  CAudioProfile();
  virtual ~CAudioProfile();
  void Clear();
  unsigned int GetDescriptorCount();
  CStreamDescriptor* GetDescriptor(unsigned int index);
  bool GetDescriptors(int format, StreamDescriptorList* pList); // TODO: Also allow a list of attributes and values to refine the search
  void AddDescriptor(CStreamDescriptor* pDesc);
protected:
  StreamDescriptorList m_DescriptorList; 
};



typedef std::map<MA_STREAM_ID,CAudioStream*>::iterator StreamIterator;

class CSyncCallData
{
public:
  HANDLE m_SyncWaitEvent;
  union
  {
    __int32 int32Val;
    unsigned __int32 uint32Val;
    float floatVal;
    void* ptrVal;
    __int64 int64Val;
    unsigned __int64 uint64Val;
    bool boolVal;
  } m_SyncResult;
};

class CAudioManagerMessage
{
public:
  enum MessageClass
  {
  // ASync
    ADD_DATA,
    FLUSH_STREAM,
    CONTROL_STREAM,
    CLOSE_STREAM,
    SET_STREAM_VOLUME,
    SET_STREAM_PARAMETER,
  // Sync
    GET_STREAM_DELAY, // TODO: This needs to be synchronous, but cannot use a wait event as it is called too frequently
    GET_STREAM_PARAMETER,
    DRAIN_STREAM,
    ADD_STREAM
  };

  CAudioManagerMessage(MA_STREAM_ID streamId, MessageClass msgClass) : 
    m_Class(msgClass), 
    m_StreamId(streamId),
    m_pSyncData(NULL)
    {
    }
  bool IsAsync() {return (m_pSyncData == NULL);}
  MessageClass GetClass() {return m_Class;}
  CSyncCallData* GetSyncData() {return m_pSyncData;}
  void SetSyncData(CSyncCallData* pSync) {m_pSyncData = pSync;}
  MA_STREAM_ID GetStreamId() {return m_StreamId;}
protected:
  MessageClass m_Class;
  CSyncCallData* m_pSyncData;
  MA_STREAM_ID m_StreamId;
};

class CTransportControlMessage : public CAudioManagerMessage
{
public:
  CTransportControlMessage(MA_STREAM_ID streamId, int command) : 
    CAudioManagerMessage(streamId, CONTROL_STREAM),
    m_Command(command) {}
  int GetCommand() {return m_Command;}
private:
  int m_Command;
};

class CAudioManagerMessageLong : public CAudioManagerMessage
{
public:
  CAudioManagerMessageLong(MA_STREAM_ID streamId, MessageClass msgClass, long param) : 
    CAudioManagerMessage(streamId, msgClass),
    m_Param(param) {}
  long GetParam() {return m_Param;}
private:
  long m_Param;
};

class CAudioManagerMessagePtr : public CAudioManagerMessage
{
public:
  CAudioManagerMessagePtr(MA_STREAM_ID streamId, MessageClass msgClass, void* param) : 
    CAudioManagerMessage(streamId, msgClass),
    m_Param(param) {}
  void* GetParam() {return m_Param;}
private:
  void* m_Param;
};

class CAddStreamDataMessage : public CAudioManagerMessage
{
public:
  CAddStreamDataMessage(MA_STREAM_ID streamId, void* pData, unsigned int len) : 
    CAudioManagerMessage(streamId, ADD_DATA),
    m_pData(pData),
    m_DataLen(len) {}
  void* GetData() {return m_pData;}
  unsigned int GetDataLen() {return m_DataLen;}
private:
  void* m_pData;
  unsigned int m_DataLen;
};

class CAudioManagerMessageQueue
{
public:
  void PushBack(CAudioManagerMessage* pMsg)
  {
    CSingleLock lock(m_Section);
    m_Queue.push_back(pMsg);
  }

  CAudioManagerMessage* PopFront() 
  {
    CSingleLock lock(m_Section);
    if (m_Queue.empty())
      return NULL;
    CAudioManagerMessage* pMsg = m_Queue.front();
    m_Queue.pop_front();
    return pMsg;
  }
  unsigned int GetMessageCount() {return m_Queue.size();}
private:
  std::deque<CAudioManagerMessage*> m_Queue;
  CCriticalSection m_Section;
};

class CAudioManager
{
public:
  CAudioManager();
  virtual ~CAudioManager();
  MA_STREAM_ID OpenStream(CStreamDescriptor* pDesc);
  void CloseStream(MA_STREAM_ID streamId);
  size_t AddDataToStream(MA_STREAM_ID streamId, void* pData, size_t len);
  void ControlStream(MA_STREAM_ID streamId, int controlCode);
  void SetStreamVolume(MA_STREAM_ID streamId, long vol);
  bool SetStreamProp(MA_STREAM_ID streamId, int propId, const void* pVal);
  bool GetStreamProp(MA_STREAM_ID streamId, int propId, void* pVal);
  float GetStreamDelay(MA_STREAM_ID streamId);
  bool DrainStream(MA_STREAM_ID streamId, unsigned int timeout); // Play all slices that have been received but not rendered yet (finish or give-up within timeout ms)
  void FlushStream(MA_STREAM_ID streamId);  // Drop any slices that have been received but not rendered yet
  bool SetMixerType(int mixerType);
  bool Initialize();
  
protected:
  std::map<MA_STREAM_ID,CAudioStream*> m_StreamList;
  IAudioMixer* m_pMixer;
  unsigned int m_MaxStreams;
  CAudioProfile m_DefaultProfile;
  bool m_Initialized;

  void LoadAudioProfiles();
  CAudioStream* GetInputStream(MA_STREAM_ID streamId);
  MA_STREAM_ID GetStreamId(CAudioStream* pStream);
  int GetOpenStreamCount();
  void CleanupStreamResources(CAudioStream* pStream);
  bool FindOutputDescriptors(CStreamDescriptor* pInputDesc, StreamDescriptorList* pList);

  CAudioManagerMessageQueue m_Queue;
  void PostMessage(CAudioManagerMessage* pMsg);
  void CallMethodAsync(CAudioManagerMessage* pMsg);
  bool CallMethodSync(CAudioManagerMessage* pMsg);

  // This class encapsulates the AudioManager processing thread
  friend class CAudioManagerThread;
  class CAudioManagerThread : public CThread
  {
  public:
    CAudioManagerThread();
    virtual ~CAudioManagerThread();
    void Wake();
    void Create(CAudioManager* pMgr);

  protected:
    virtual void OnStartup(){};
    virtual void OnExit(){};
    virtual void Process();

    void HandleMethodAsync(CAudioManagerMessage* pMsg);
    bool HandleMethodSync(CAudioManagerMessage* pMsg);

    // Message Handlers
  // Sync
// GetStreamParameter

  // ASync
// SetStreamParameter

    bool AddStream(MA_STREAM_ID streamId, CAudioStream* pStream);
    void ControlStream(MA_STREAM_ID streamId, int command);
    float GetStreamDelay(MA_STREAM_ID streamId);
    bool DrainStream(MA_STREAM_ID streamId, int timeout);
    void FlushStream(MA_STREAM_ID streamId);
    void SetStreamVolume(MA_STREAM_ID streamId, long volume);
    void CloseStream(MA_STREAM_ID streamId);
    unsigned int AddDataToStream(MA_STREAM_ID streamId, void* pData, size_t len);

    void RenderStreams();
    CAudioManager* m_pManager;
    HANDLE m_ProcessEvent;
  };
  CAudioManagerThread m_ProcessThread;
};

extern CAudioManager g_AudioLibManager;



#endif // __AUDIO_MANAGER_H__