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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "dialogs/GUIDialogSlider.h"
#include "dialogs/IGUIVolumeBarCallback.h"
#include "guilib/ISliderCallback.h"
#include "interfaces/IAnnouncer.h"

#include <set>

namespace KODI
{

namespace GAME
{
  class CDialogGameVolume : public CGUIDialogSlider, // GUI interface
                            public ISliderCallback, // GUI callback
                            public IGUIVolumeBarCallback, // Volume bar dialog callback
                            public ANNOUNCEMENT::IAnnouncer // Application callback
  {
  public:
    CDialogGameVolume();
    ~CDialogGameVolume() override = default;

    // implementation of CGUIControl via CGUIDialogSlider
    bool OnMessage(CGUIMessage &message) override;

    // implementation of CGUIWindow via CGUIDialogSlider
    void OnDeinitWindow(int nextWindowID) override;

    // implementation of ISliderCallback
    void OnSliderChange(void *data, CGUISliderControl *slider) override;

    // implementation of IGUIVolumeBarCallback
    bool IsShown() const override;

    // implementation of IAnnouncer
    void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data) override;

  protected:
    // implementation of CGUIWindow via CGUIDialogSlider
    void OnInitWindow() override;

  private:
    /*!
     * \brief Call when state change message is received
     */
    void OnStateChanged();

    /*!
     * \brief Get the volume of the first callback
     *
     * \return The volume, as a fraction of maximum volume
     */
    float GetVolumePercent() const;

    /*!
     * \brief Get the volume bar label
     */
    static std::string GetLabel();

    // Volume parameters
    const float VOLUME_MIN = 0.0f;
    const float VOLUME_DELTA = 10.0f;
    const float VOLUME_MAX = 100.0f;
    float m_volumePercent = 100.0f;
    float m_oldVolumePercent = 100.0f;
  };
}
}
