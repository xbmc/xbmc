#pragma once

/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

/*
* for DESCRIPTION see 'DVDInputStreamPVRManager.cpp'
*/

#include "DVDInputStream.h"
#include "FileItem.h"
#include "threads/SystemClock.h"

namespace XFILE {
class IFile;
class ILiveTVInterface;
class IRecordable;
}

class IDVDPlayer;

class CDVDInputStreamPVRManager
  : public CDVDInputStream
  , public CDVDInputStream::IChannel
  , public CDVDInputStream::IDisplayTime
  , public CDVDInputStream::ISeekable
{
public:
  CDVDInputStreamPVRManager(IDVDPlayer* pPlayer);
  virtual ~CDVDInputStreamPVRManager();
  virtual bool Open(const char* strFile, const std::string &content);
  virtual void Close();
  virtual int Read(uint8_t* buf, int buf_size);
  virtual int64_t Seek(int64_t offset, int whence);
  virtual bool Pause(double dTime) { return false; }
  virtual bool IsEOF();
  virtual int64_t GetLength();

  virtual ENextStream NextStream();

  bool                SelectChannelByNumber(unsigned int iChannel);
  bool                SelectChannel(const PVR::CPVRChannelPtr &channel);
  bool                NextChannel(bool preview = false);
  bool                PrevChannel(bool preview = false);
  PVR::CPVRChannelPtr GetSelectedChannel();

  int             GetTotalTime();
  int             GetTime();

  bool            CanRecord();
  bool            IsRecording();
  bool            Record(bool bOnOff);
  bool            CanSeek();
  bool            CanPause();
  void            Pause(bool bPaused);

  bool            UpdateItem(CFileItem& item);

  /* overloaded is streamtype to support m_pOtherStream */
  bool            IsStreamType(DVDStreamType type) const;

  /*! \brief Get the input format from the Backend
   If it is empty ffmpeg scanning the stream to find the right input format.
   See "xbmc/cores/dvdplayer/Codecs/ffmpeg/libavformat/allformats.c" for a
   list of the input formats.
   \return The name of the input format
   */
  std::string      GetInputFormat();

  /* returns m_pOtherStream */
  CDVDInputStream* GetOtherStream();

  void ResetScanTimeout(unsigned int iTimeoutMs);
protected:
  bool CloseAndOpen(const char* strFile);
  static bool SupportsChannelSwitch(void);

  IDVDPlayer*               m_pPlayer;
  CDVDInputStream*          m_pOtherStream;
  XFILE::IFile*             m_pFile;
  XFILE::ILiveTVInterface*  m_pLiveTV;
  XFILE::IRecordable*       m_pRecordable;
  bool                      m_eof;
  std::string               m_strContent;
  XbmcThreads::EndTime      m_ScanTimeout;
};


inline bool CDVDInputStreamPVRManager::IsStreamType(DVDStreamType type) const
{
  if (m_pOtherStream)
    return m_pOtherStream->IsStreamType(type);

  return m_streamType == type;
}

inline CDVDInputStream* CDVDInputStreamPVRManager::GetOtherStream()
{
  return m_pOtherStream;
};

