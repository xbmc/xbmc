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

/*
* for DESCRIPTION see 'DVDInputStreamPVRManager.cpp'
*/

#include "DVDInputStream.h"
#include "FileItem.h"

namespace XFILE {
class IFile;
class ILiveTVInterface;
class IRecordable;
}

class IDVDPlayer;

class CDVDInputStreamPVRManager
  : public CDVDInputStream
  , public CDVDInputStream::IChannel
{
public:
  CDVDInputStreamPVRManager(IDVDPlayer* pPlayer);
  virtual ~CDVDInputStreamPVRManager();
  virtual bool Open(const char* strFile, const std::string &content);
  virtual void Close();
  virtual int Read(BYTE* buf, int buf_size);
  virtual __int64 Seek(__int64 offset, int whence);
  virtual bool Pause(double dTime) { return false; }
  virtual bool IsEOF();
  virtual __int64 GetLength();

  virtual bool    NextStream();

  bool            SelectChannelByNumber(unsigned int iChannel);
  bool            SelectChannel(const PVR::CPVRChannel &channel);
  bool            NextChannel(bool preview = false);
  bool            PrevChannel(bool preview = false);
  bool            GetSelectedChannel(PVR::CPVRChannel &channel) const;

  int             GetTotalTime();
  int             GetStartTime();

  bool            CanRecord();
  bool            IsRecording();
  bool            Record(bool bOnOff);

  bool            UpdateItem(CFileItem& item);

  /* overloaded is streamtype to support m_pOtherStream */
  bool            IsStreamType(DVDStreamType type) const;

  /*! \brief Get the input format from the Backend
   If it is empty ffmpeg scanning the stream to find the right input format.
   See "xbmc/cores/dvdplayer/Codecs/ffmpeg/libavformat/allformats.c" for a
   list of the input formats.
   \return The name of the input format
   */
  CStdString      GetInputFormat();

  /* returns m_pOtherStream */
  CDVDInputStream* GetOtherStream();

protected:
  IDVDPlayer*               m_pPlayer;
  CDVDInputStream*          m_pOtherStream;
  XFILE::IFile*             m_pFile;
  XFILE::ILiveTVInterface*  m_pLiveTV;
  XFILE::IRecordable*       m_pRecordable;
  bool                      m_eof;
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

