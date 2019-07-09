/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"
#include "threads/CriticalSection.h"

#include <map>
#include <string>
#include <vector>

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
  static CRssManager& GetInstance();

  void OnSettingsLoaded() override;
  void OnSettingsUnloaded() override;

  void OnSettingAction(std::shared_ptr<const CSetting> setting) override;

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
  ~CRssManager() override;

private:
  CRssManager(const CRssManager&) = delete;
  CRssManager& operator=(const CRssManager&) = delete;
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
