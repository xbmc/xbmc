/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <string>

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
    void UpdateProgress(const std::string& strText, float fProgress);

    /*!
     * @brief Update the progress dialogs's content.
     * @param strText The new progress text.
     * @param iCurrent The new current progress value, must be less or equal iMax.
     * @param iMax The new maximum progress value, must be greater or equal iCurrent.
     */
    void UpdateProgress(const std::string& strText, int iCurrent, int iMax);

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
