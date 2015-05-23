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

#include "IFile.h"
#include "ILiveTV.h"


class CPVRSession;

namespace XFILE {

class CPVRFile
  : public  IFile
  ,         ILiveTVInterface
  ,         IRecordable
{
public:
  CPVRFile();
  virtual ~CPVRFile();
  virtual bool          Open(const CURL& url);
  virtual int64_t       Seek(int64_t pos, int whence=SEEK_SET);
  virtual int64_t       GetPosition();
  virtual int64_t       GetLength();
  virtual int           Stat(const CURL& url, struct __stat64* buffer) { return -1; }
  virtual void          Close();
  virtual ssize_t       Read(void* buffer, size_t size);
  virtual std::string   GetContent()                                   { return ""; }
  virtual bool          SkipNext()                                     { return !m_isPlayRecording; }

  virtual bool          Delete(const CURL& url);
  virtual bool          Rename(const CURL& url, const CURL& urlnew);
  virtual bool          Exists(const CURL& url);

  virtual ILiveTVInterface* GetLiveTV() {return (ILiveTVInterface*)this;}

  virtual bool           NextChannel(bool preview = false);
  virtual bool           PrevChannel(bool preview = false);
  virtual bool           SelectChannel(unsigned int channel);

  virtual int            GetTotalTime();
  virtual int            GetStartTime();
  virtual bool           UpdateItem(CFileItem& item);

  virtual IRecordable* GetRecordable() {return (IRecordable*)this;}

  virtual bool           CanRecord();
  virtual bool           IsRecording();
  virtual bool           Record(bool bOnOff);

  virtual int            IoControl(EIoControl request, void *param);

  static std::string     TranslatePVRFilename(const std::string& pathFile);

protected:
  bool            m_isPlayRecording;
  int             m_playingItem;
};

}


