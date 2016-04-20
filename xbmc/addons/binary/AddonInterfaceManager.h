#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "cores/IPlayerCallback.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <memory>
#include <vector>

namespace ADDON
{

  template <class T> struct LockableType : public T, public CCriticalSection
  { bool hadSomethingRemoved; };

  class CAddonInterfaceManager
    : public IPlayerCallback
  {
  public:
    CAddonInterfaceManager();
    virtual ~CAddonInterfaceManager();

    bool StartManager();
    void StopManager();

    void RegisterPlayerCallBack(IPlayerCallback* pCallback);
    void UnregisterPlayerCallBack(IPlayerCallback* pCallback);

    // IPlayerCallback
    virtual void OnPlayBackEnded() override;
    virtual void OnPlayBackStarted() override;
    virtual void OnPlayBackPaused() override;
    virtual void OnPlayBackResumed() override;
    virtual void OnPlayBackStopped() override;
    virtual void OnPlayBackSpeedChanged(int iSpeed) override;
    virtual void OnPlayBackSeek(int iTime, int seekOffset) override;
    virtual void OnPlayBackSeekChapter(int iChapter) override;
    virtual void OnQueueNextItem() override;

  private:
    bool                m_bInitialized;

    typedef LockableType<std::vector<void*> > PlayerCallbackList;
    PlayerCallbackList  m_vecPlayerCallbackList;
  };

} /* namespace ADDON */
