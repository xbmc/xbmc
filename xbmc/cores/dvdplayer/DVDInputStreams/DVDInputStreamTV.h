#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DVDInputStream.h"

namespace XFILE {
class IFile;
class ILiveTVInterface;
class IRecordable;
}

class CDVDInputStreamTV
  : public CDVDInputStream
  , public CDVDInputStream::IChannel
  , public CDVDInputStream::IDisplayTime
{
public:
  CDVDInputStreamTV();
  virtual ~CDVDInputStreamTV();
  virtual bool    Open(const char* strFile, const std::string &content);
  virtual void    Close();
  virtual int     Read(BYTE* buf, int buf_size);
  virtual int64_t Seek(int64_t offset, int whence);
  virtual bool Pause(double dTime) { return false; };
  virtual bool    IsEOF();
  virtual int64_t GetLength();

  virtual ENextStream NextStream();
  virtual int     GetBlockSize();

  bool            NextChannel(bool preview = false);
  bool            PrevChannel(bool preview = false);
  bool            SelectChannelByNumber(unsigned int channel);

  int             GetTotalTime();
  int             GetTime();

  bool            SeekTime(int iTimeInMsec);

  bool            CanRecord();
  bool            IsRecording();
  bool            Record(bool bOnOff);

  bool            CanPause() { return false; };
  bool            CanSeek() { return false; };

  bool            UpdateItem(CFileItem& item);

protected:
  XFILE::IFile*            m_pFile;
  XFILE::ILiveTVInterface* m_pLiveTV;
  XFILE::IRecordable*      m_pRecordable;
  bool m_eof;
};

