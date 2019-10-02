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
#include "games/controllers/ControllerFeature.h"
#include "games/controllers/ControllerTypes.h"
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
      std::vector<CControllerFeature> features;
      /*!
       * True if this group is a button that allows the user to map a key of
       * their choosing.
       */
      bool bIsVirtualKey = false;
    };
    std::vector<FeatureGroup> GetFeatureGroups(const std::vector<CControllerFeature>& features) const;
    std::vector<CGUIButtonControl*> GetButtons(const std::vector<CControllerFeature>& features, unsigned int startIndex);
    CGUIButtonControl* GetSelectKeyButton(const std::vector<CControllerFeature>& features, unsigned int buttonIndex);

    // GUI stuff
    CGUIWindow* const       m_window;
    unsigned int            m_buttonCount = 0;
    CGUIControlGroupList*   m_guiList;
    CGUIButtonControl*      m_guiButtonTemplate;
    CGUILabelControl*       m_guiGroupTitle;
    CGUIImage*              m_guiFeatureSeparator;

    // Game window stuff
    GameClientPtr           m_gameClient;
    ControllerPtr           m_controller;
    IConfigurationWizard*   m_wizard;
  };
}
}
