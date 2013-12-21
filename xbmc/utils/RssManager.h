#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <map>
#include <vector>
#include <string>

#include "threads/CriticalSection.h"

#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"

class CRssReader;
class IRssObserver;

typedef struct
{
  bool rtl;
  std::vector<int> interval;
  std::vector<std::string> url;
} RssSet;
typedef std::map<int, RssSet> RssUrls;

class CRssManager : public ISettingCallback, public ISettingsHandler
{
public:
  static CRssManager& Get();

  virtual void OnSettingsLoaded();
  virtual void OnSettingsUnloaded();

  virtual void OnSettingAction(const CSetting *setting);

  void Start();
  void Stop();
  bool Load();
  bool Reload();
  void Clear();
  bool IsActive() const { return m_bActive; }

  bool GetReader(int controlID, int windowID, IRssObserver* observer, CRssReader *&reader);
  const RssUrls& GetUrls() const { return m_mapRssUrls; }

protected:
  CRssManager();
  ~CRssManager();

private:
  CRssManager(const CRssManager&);
  CRssManager& operator=(const CRssManager&);
  struct READERCONTROL
  {
    int controlID;
    int windowID;
    CRssReader *reader;
  };

  std::vector<READERCONTROL> m_readers;
  RssUrls m_mapRssUrls;
  bool m_bActive;
  CCriticalSection m_critical;
};
