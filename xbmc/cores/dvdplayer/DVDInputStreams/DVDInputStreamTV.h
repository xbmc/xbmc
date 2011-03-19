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

#include "DVDInputStream.h"

namespace XFILE {
class IFile;
class ILiveTVInterface;
class IRecordable;
}

class CDVDInputStreamTV
  : public CDVDInputStream
  , public CDVDInputStream::IChannel
{
public:
  CDVDInputStreamTV();
  virtual ~CDVDInputStreamTV();
  virtual bool    Open(const char* strFile, const std::string &content);
  virtual void    Close();
  virtual int     Read(BYTE* buf, int buf_size);
  virtual __int64 Seek(__int64 offset, int whence);
  virtual bool Pause(double dTime) { return false; };
  virtual bool    IsEOF();
  virtual __int64 GetLength();

  virtual bool    NextStream();
  virtual int     GetBlockSize();

  bool            NextChannel(bool preview = false);
  bool            PrevChannel(bool preview = false);
  bool            SelectChannelByNumber(unsigned int channel);
  bool            SelectChannel(const CPVRChannel &channel);
  bool            GetSelectedChannel(CPVRChannel *channel) {return false; }

  int             GetTotalTime();
  int             GetStartTime();

  bool            SeekTime(int iTimeInMsec);

  bool            CanRecord();
  bool            IsRecording();
  bool            Record(bool bOnOff);

  bool            UpdateItem(CFileItem& item);

protected:
  XFILE::IFile*            m_pFile;
  XFILE::ILiveTVInterface* m_pLiveTV;
  XFILE::IRecordable*      m_pRecordable;
  bool m_eof;
};

