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

#include "Thread.h"

class CInfoLoader;

class CBackgroundLoader : public CThread
{
public:
  CBackgroundLoader(CInfoLoader *callback);
  ~CBackgroundLoader();

  void Start();

protected:
  void Process();
  virtual void GetInformation() {};
  CInfoLoader *m_callback;
};

class CInfoLoader
{
public:
  CInfoLoader(const char *type);
  virtual ~CInfoLoader();
  const char *GetInfo(DWORD dwInfo);
  void Refresh();
  void LoaderFinished();
  void ResetTimer();
protected:
  virtual const char *TranslateInfo(DWORD dwInfo);
  virtual const char *BusyInfo(DWORD dwInfo);
  virtual DWORD TimeToNextRefreshInMs() { return 300000; }; // default to 5 minutes
private:
  DWORD m_refreshTime;
  CBackgroundLoader *m_backgroundLoader;
  bool m_busy;
  CStdString m_busyText;
  CStdString m_type;
};
