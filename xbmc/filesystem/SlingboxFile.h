#pragma once
/*
 *      Copyright (C) 2011-2012 Team XBMC
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

class CSlingbox;

namespace XFILE
{
  class CSlingboxFile
  : public IFile, ILiveTVInterface
  {
  public:
    CSlingboxFile();
    virtual ~CSlingboxFile();
    virtual bool Open(const CURL& url);
    virtual unsigned int Read(void * buffer, int64_t size);
    virtual void Close();
    virtual bool SkipNext();
    
    virtual int GetStartTime()                                 { return 0; }
    virtual int GetTotalTime()                                 { return 0; }
    virtual int64_t GetLength()                                { return -1; }
    virtual int64_t GetPosition()                              { return -1; }
    virtual int64_t Seek(int64_t pos, int whence)              { return -1; }
    virtual bool UpdateItem(CFileItem& item)                   { return false; }
    
    virtual bool Exists(const CURL& url)                       { return false; }
    virtual int Stat(const CURL& url, struct __stat64* buffer) { return -1; }
    
    virtual ILiveTVInterface * GetLiveTV()                     { return (ILiveTVInterface *)this; }
    
    virtual bool NextChannel(bool bPreview = false); // TODO bPreview is not implemented
    virtual bool PrevChannel(bool bPreview = false); // TODO bPreview is not implemented
    virtual bool SelectChannel(unsigned int uiChannel);
    
  protected:
    struct
    {
      CStdString strHostname;
      int iVideoWidth;
      int iVideoHeight;
      int iVideoResolution;
      int iVideoBitrate;
      int iVideoFramerate;
      int iVideoSmoothing;
      int iAudioBitrate;
      int iIFrameInterval;
      unsigned int uiCodeChannelUp;
      unsigned int uiCodeChannelDown;
      unsigned int uiCodeNumber[10];
    } m_sSlingboxSettings;
    
    void LoadSettings(const CStdString& strSlingbox);
    CSlingbox * m_pSlingbox;
  };
  
}
