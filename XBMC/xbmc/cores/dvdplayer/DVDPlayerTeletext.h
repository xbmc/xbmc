#pragma once

/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "../../utils/Thread.h"
#include "DVDStreamInfo.h"
#include "DVDMessageQueue.h"
#include <map>

#define DVDPLAYERDATA_TELETEXT    1

struct PageID 
{
  PageID() { page=subPage=0; }
  PageID(int chanId, int p, int s) { set(chanId, p, s); }
  void set(int chanId, int p, int s) { channelId=chanId; page=p; subPage=s; }
  int channelId;
  int page;
  int subPage;
};

class CDVDTeletextData;

class cTelePage
{
  friend class CDVDTeletextData;

private:
  int mag;
  unsigned char flags;
  unsigned char lang;
  PageID page;
  unsigned char pagebuf[27*40];

public:
  cTelePage(PageID page, unsigned char flags, unsigned char lang, int mag);
  ~cTelePage();
  void SetLine(int, unsigned char*);
};

class CDVDTeletextData : public CThread
{
public:
  CDVDTeletextData();
  ~CDVDTeletextData();

  bool CheckStream(CDVDStreamInfo &hints, int type);
  bool OpenStream(CDVDStreamInfo &hints, int type);
  void CloseStream(bool bWaitForBuffers);
  void Flush();

  // waits until all available data has been rendered
  void WaitForBuffers() { m_messageQueue.WaitUntilEmpty(); }
  bool AcceptsData() { return !m_messageQueue.IsFull(); }
  void SendMessage(CDVDMsg* pMsg) { m_messageQueue.Put(pMsg); }

  int GetTeletextPageCount();
  void ResetTeletextCache();
  bool GetTeletextPagePresent(int Page, int subPage);
  bool GetTeletextPage(int Page, int subPage, BYTE* buf);

  CDVDMessageQueue m_messageQueue;

protected:
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

private:
  void DecodeTXT(unsigned char* ptr);
  
  std::map<long, cTelePage*> m_pageStorage;
  cTelePage *m_TxtPage;
  int m_speed;
  int m_channel;
  CRITICAL_SECTION m_critSection;

};
