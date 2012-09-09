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

#include "IFile.h"
#include "ILiveTV.h"

#include <sys/socket.h>

class CVTPSession;

namespace XFILE {

class CVTPFile
  : public  IFile
  ,         ILiveTVInterface
{
public:
  CVTPFile();
  virtual ~CVTPFile();
  virtual bool          Open(const CURL& url);
  virtual int64_t       Seek(int64_t pos, int whence=SEEK_SET);
  virtual int64_t       GetPosition()                                  { return -1; }
  virtual int64_t       GetLength()                                    { return -1; }
  virtual int           Stat(const CURL& url, struct __stat64* buffer) { return -1; }
  virtual void          Close();
  virtual unsigned int  Read(void* buffer, int64_t size);
  virtual CStdString    GetContent()                                   { return ""; }
  virtual bool          SkipNext()                                     { return m_socket ? true : false; }

  virtual bool          Delete(const CURL& url)                        { return false; }
  virtual bool          Exists(const CURL& url)                        { return false; }

  virtual ILiveTVInterface* GetLiveTV() {return (ILiveTVInterface*)this;}

  virtual bool           NextChannel(bool preview = false);
  virtual bool           PrevChannel(bool preview = false);
  virtual bool           SelectChannel(unsigned int channel);

  virtual int            GetTotalTime()              { return 0; }
  virtual int            GetStartTime()              { return 0; }
  virtual bool           UpdateItem(CFileItem& item) { return false; }

  virtual int            IoControl(EIoControl request, void* param);
protected:
  CVTPSession* m_session;
  SOCKET       m_socket;
  int          m_channel;
};

}


