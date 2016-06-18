/*
 *      Copyright (C) 2016 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <vector>

#include "EventScanRate.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/Thread.h"

namespace PERIPHERALS
{
  class IEventScannerCallback
  {
  public:
    virtual ~IEventScannerCallback(void) { }

    virtual void ProcessEvents(void) = 0;
  };

  /*!
   * \brief Class to scan for peripheral events
   *
   * A default rate of 60 Hz is used. This can be overridden by calling
   * SetRate(). The scanner will run at this new rate until the handle it
   * returns has been released.
   *
   * If two instances hold handles from SetRate(), the one with the higher
   * rate wins.
   */
  class CEventScanner : public IEventRateCallback,
                        protected CThread
  {
  public:
    CEventScanner(IEventScannerCallback* callback);

    virtual ~CEventScanner(void) { }

    void Start(void);
    void Stop(void);

    EventRateHandle SetRate(float rateHz);

    // implementation of IEventRateCallback
    virtual void Release(CEventRateHandle* handle) override;

  protected:
    // implementation of CThread
    virtual void Process(void) override;

  private:
    float GetRateHz(void) const;
    float GetScanIntervalMs(void) const { return 1000.0f / GetRateHz(); }

    IEventScannerCallback* const m_callback;
    std::vector<EventRateHandle> m_handles;
    CEvent                       m_scanEvent;
    CCriticalSection             m_mutex;
  };
}
