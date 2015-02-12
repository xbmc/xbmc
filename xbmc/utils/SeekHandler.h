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

#include <vector>
#include "settings/Settings.h"
#include "settings/lib/ISettingCallback.h"
#include "utils/Stopwatch.h"

class CSeekHandler : public ISettingCallback
{
public:
  static CSeekHandler& Get();

  static void SettingOptionsSeekStepsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
  
  virtual void OnSettingChanged(const CSetting *setting);

  void Seek(bool forward, float amount, float duration = 0, bool analogSeek = false);
  void Process();
  void Reset();

  float GetPercent() const;
  bool InProgress() const;

protected:
  CSeekHandler();
  CSeekHandler(const CSeekHandler&);
  CSeekHandler& operator=(CSeekHandler const&);
  virtual ~CSeekHandler();

private:
  static const int time_for_display = 2000; // TODO: WTF?
  int        GetSeekSeconds(bool forward);
  int        m_seekDelay;
  bool       m_requireSeek;
  float      m_percent;
  float      m_percentPlayTime;
  bool       m_analogSeek;
  int        m_seekStep;
  std::vector<int> m_forwardSeekSteps;
  std::vector<int> m_backwardSeekSteps;
  CStopWatch m_timer;
};
