/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
/*!
 * \ingroup games
 */
class CDialogGameVolume : public CGUIDialogSlider, // GUI interface
                          public ISliderCallback, // GUI callback
                          public IGUIVolumeBarCallback, // Volume bar dialog callback
                          public ANNOUNCEMENT::IAnnouncer // Application callback
{
public:
  CDialogGameVolume();
  ~CDialogGameVolume() override = default;

  // implementation of CGUIControl via CGUIDialogSlider
  bool OnMessage(CGUIMessage& message) override;

  // implementation of CGUIWindow via CGUIDialogSlider
  void OnDeinitWindow(int nextWindowID) override;

  // implementation of ISliderCallback
  void OnSliderChange(void* data, CGUISliderControl* slider) override;

  // implementation of IGUIVolumeBarCallback
  bool IsShown() const override;

  // implementation of IAnnouncer
  void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                const std::string& sender,
                const std::string& message,
                const CVariant& data) override;

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
} // namespace GAME
} // namespace KODI
