/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/windows/GUIWindowPVRBase.h"

#include <string>

namespace PVR
{
class CPVRProvidersPath;

class CGUIWindowPVRProvidersBase : public CGUIWindowPVRBase
{
public:
  CGUIWindowPVRProvidersBase(bool isRadio, int id, const std::string& xmlFile);
  ~CGUIWindowPVRProvidersBase() override;

  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;
  void UpdateButtons() override;

private:
  void ActivateChannelsWindow(const CPVRProvidersPath& selectedPath);
  void ActivateRecordingsWindow(const CPVRProvidersPath& selectedPath);
};

class CGUIWindowPVRTVProviders : public CGUIWindowPVRProvidersBase
{
public:
  CGUIWindowPVRTVProviders()
    : CGUIWindowPVRProvidersBase(false, WINDOW_TV_PROVIDERS, "MyPVRProviders.xml")
  {
  }
  std::string GetRootPath() const override;
  std::string GetDirectoryPath() override;
};

class CGUIWindowPVRRadioProviders : public CGUIWindowPVRProvidersBase
{
public:
  CGUIWindowPVRRadioProviders()
    : CGUIWindowPVRProvidersBase(true, WINDOW_RADIO_PROVIDERS, "MyPVRProviders.xml")
  {
  }
  std::string GetRootPath() const override;
  std::string GetDirectoryPath() override;
};
} // namespace PVR
