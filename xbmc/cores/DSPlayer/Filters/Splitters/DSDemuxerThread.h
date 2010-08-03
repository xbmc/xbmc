/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#pragma once

#include "Thread.h"

#include "DSDemuxerFilter.h"
#include "DVDPlayer/DVDMessageQueue.h"
#include "DVDPlayer/DVDClock.h"
#include "DSOutputpin.h"
#include "DSStreamInfo.h"
#include "DVDPlayer/DVDInputStreams/DVDInputStream.h"
#include "DVDPlayer/DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDPlayer/DVDDemuxers/DVDDemux.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "DVDPlayer/DVDDemuxers/DVDFactoryDemuxer.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxFFmpeg.h"

#include "TimeUtils.h"



class CDSDemuxerFilter;

#define DSPLAYER_AUDIO    1
#define DSPLAYER_VIDEO    2
#define DSPLAYER_SUBTITLE 3
#define DSPLAYER_TELETEXT 4




class CDSOutputPin;

[uuid("B98D13E7-55DB-4385-A33D-09FD1BA26338")]
class CDSDemuxerThread
  : public CAMThread
{
public:
  // IFileSourceFilter
  CDSDemuxerThread(CDSDemuxerFilter* pFilter ,LPWSTR pFileName);
  virtual ~CDSDemuxerThread();

  virtual BOOL Create(void);
	void Stop();
  __int64 GetDuration() {return (DVD_MSEC_TO_TIME(m_pDemuxer->GetStreamLength()) * 10); }

  void SeekTo(REFERENCE_TIME rt);
  void SetPlaySpeed(int speed);
protected:
  enum WorkState {
		CMD_STOP,
		CMD_SLEEP,
		CMD_WAKEUP,
	};

  int m_playSpeed;
	virtual DWORD ThreadProc();
	HRESULT CreateOutputPin();

  CDSDemuxerFilter* m_pFilter;

  LPOLESTR m_filenameW;
  CStdString m_filenameA; // holds the actual filename
  //CDVDMessageQueue m_messenger;     // thread messenger

  CDVDInputStream*    m_pInputStream;  // input stream for current playing file
  CDVDDemux*          m_pDemuxer;            // demuxer for current playing file
  CDVDDemux*          m_pSubtitleDemuxer;
  CDVDMessageQueue    m_messenger;     // thread messenger
  void                HandleMessages();
  //Output pins
  int        m_streamsCount;
  struct StreamList {
    UINT       pin_index;
    CDSOutputPin *pin;
	};
  StreamList *m_pStreamList;

  CDSOutputPin *GetOutputPinIndex(UINT index) const;

  CAMEvent m_pSeekProtect;
	HANDLE   m_hSeekProtection;
	BOOL     m_bTrustCRC;


  bool ReadPacket(DemuxPacket*& packet, CDemuxStream*& stream);
  /*bool OpenAudioStream(int iStream, int source);
  bool OpenVideoStream(int iStream, int source);
  bool OpenSubtitleStream(int iStream, int source);
  bool CloseAudioStream(bool bWaitForBuffers);
  bool CloseVideoStream(bool bWaitForBuffers);
  bool CloseSubtitleStream();

  void ProcessPacket(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessAudioData(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessVideoData(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessSubData(CDemuxStream* pStream, DemuxPacket* pPacket);
  
  

  HRESULT DemuxNextPacket();
  bool ReadPacket(DemuxPacket*& packet, CDemuxStream*& stream);

  bool OpenInputStream();
  bool OpenDemuxStream();
  void OpenDefaultStreams();
  DSPLAYER::CCurrentStream m_CurrentAudio;
  DSPLAYER::CCurrentStream m_CurrentVideo;
  DSPLAYER::CCurrentStream m_CurrentSubtitle;

  CSelectionStreams m_SelectionStreams;*/
  
  
};
