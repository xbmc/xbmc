/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IConfigurationWindow.h"
#include "games/GameTypes.h"
#include "games/controllers/ControllerTypes.h"
#include "games/controllers/input/PhysicalFeature.h"
#include "input/joysticks/JoystickTypes.h"

class CGUIButtonControl;
class CGUIControlGroupList;
class CGUIImage;
class CGUILabelControl;
class CGUIWindow;

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CGUIFeatureList : public IFeatureList
{
public:
  CGUIFeatureList(CGUIWindow* window, GameClientPtr gameClient);
  ~CGUIFeatureList() override;

  // implementation of IFeatureList
  bool Initialize() override;
  void Deinitialize() override;
  bool HasButton(JOYSTICK::FEATURE_TYPE type) const override;
  void Load(const ControllerPtr& controller) override;
  void OnFocus(unsigned int buttonIndex) override {}
  void OnSelect(unsigned int buttonIndex) override;

private:
  IFeatureButton* GetButtonControl(unsigned int buttonIndex);

  void CleanupButtons(void);

  // Helper functions
  struct FeatureGroup
  {
    std::string groupName;
    std::vector<CPhysicalFeature> features;
    /*!
     * True if this group is a button that allows the user to map a key of
     * their choosing.
     */
    bool bIsVirtualKey = false;
  };
  std::vector<FeatureGroup> GetFeatureGroups(const std::vector<CPhysicalFeature>& features) const;
  std::vector<CGUIButtonControl*> GetButtons(const std::vector<CPhysicalFeature>& features,
                                             unsigned int startIndex);
  CGUIButtonControl* GetSelectKeyButton(const std::vector<CPhysicalFeature>& features,
                                        unsigned int buttonIndex);

  // GUI stuff
  CGUIWindow* const m_window;
  unsigned int m_buttonCount = 0;
  CGUIControlGroupList* m_guiList = nullptr;
  CGUIButtonControl* m_guiButtonTemplate = nullptr;
  CGUILabelControl* m_guiGroupTitle = nullptr;
  CGUIImage* m_guiFeatureSeparator = nullptr;

  // Game window stuff
  GameClientPtr m_gameClient;
  ControllerPtr m_controller;
  IConfigurationWizard* m_wizard;
};
} // namespace GAME
} // namespace KODI
