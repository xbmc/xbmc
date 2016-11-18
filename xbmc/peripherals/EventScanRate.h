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

#include <memory>

namespace PERIPHERALS
{
  class CEventRateHandle;
  typedef std::shared_ptr<CEventRateHandle> EventRateHandle;

  /*!
   * \brief Callback implemented by event scanner
   */
  class IEventRateCallback
  {
  public:
    virtual ~IEventRateCallback(void) { }

    /*!
     * \brief Release the specified handle
     */
    virtual void Release(CEventRateHandle* handle) = 0;
  };

  /*!
   * \brief Handle returned by the event scanner when a scan rate is requested
   */
  class CEventRateHandle
  {
  public:
    CEventRateHandle(double rateHz, IEventRateCallback* callback);

    ~CEventRateHandle(void) { }

    /*!
     * \brief Get the rate this handle represents
     */
    double GetRateHz(void) const { return m_rateHz; }

    /*!
     * \brief Release the handle
     */
    void Release(void);

  private:
    const double              m_rateHz;
    IEventRateCallback* const m_callback;
  };
}
