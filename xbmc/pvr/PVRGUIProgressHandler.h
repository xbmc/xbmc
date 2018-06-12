/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <string>

#include "threads/CriticalSection.h"
#include "threads/Thread.h"

namespace PVR
{
  class CPVRGUIProgressHandler : private CThread
  {
  public:
    CPVRGUIProgressHandler() = delete;

    /*!
     * @brief Creates and asynchronously shows a progress dialog with the given title.
     * @param strTitle The title for the progress dialog.
     */
    explicit CPVRGUIProgressHandler(const std::string& strTitle);

    /*!
     * @brief Update the progress dialogs's content.
     * @param strText The new progress text.
     * @param fProgress The new progress value, in a range from 0.0 to 100.0.
     */
    void UpdateProgress(const std::string &strText, float fProgress);

    /*!
     * @brief Update the progress dialogs's content.
     * @param strText The new progress text.
     * @param iCurrent The new current progress value, must be less or equal iMax.
     * @param iMax The new maximum progress value, must be greater or equal iCurrent.
     */
    void UpdateProgress(const std::string &strText, int iCurrent, int iMax);

    /*!
     * @brief Destroy the progress dialog. This happens asynchrounous, instance must not be touched anymore after calling this method.
     */
    void DestroyProgress();

    // CThread implementation
    void Process() override;

  private:
    ~CPVRGUIProgressHandler() override = default; // prevent creation of stack instances

    CCriticalSection m_critSection;
    const std::string m_strTitle;
    std::string m_strText;
    float m_fProgress;
    bool m_bChanged;
  };

} // namespace PVR
