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

    ~CPVRGUIProgressHandler() override = default;

    /*!
     * @brief Update the progress dialogs's content.
     * @param strText The new progress text.
     * @param fProgress The new progress value, in a range from 0.0 to 100.0.
     */
    void UpdateProgress(const std::string& strText, float fProgress);

    /*!
     * @brief Update the progress dialogs's content.
     * @param text The new progress text.
     * @param currentValue The new current progress value, must be less or equal iMax.
     * @param maxValue The new maximum progress value, must be greater or equal iCurrent.
     */
    void UpdateProgress(const std::string& text, size_t currentValue, size_t maxValue);

  protected:
    // CThread implementation
    void Process() override;

  private:
    CCriticalSection m_critSection;
    const std::string m_strTitle;
    std::string m_strText;
    float m_fProgress{0.0f};
    bool m_bChanged{false};
    bool m_bCreated{false};
  };

} // namespace PVR
