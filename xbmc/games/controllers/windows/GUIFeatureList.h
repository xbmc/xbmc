/*
 *      Copyright (C) 2014-2017 Team Kodi
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

#include "IConfigurationWindow.h"
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
    CGUIFeatureList(CGUIWindow* window);
    virtual ~CGUIFeatureList(void);

    // implementation of IFeatureList
    virtual bool Initialize(void) override;
    virtual void Deinitialize(void) override;
    virtual bool HasButton(JOYSTICK::FEATURE_TYPE type) const override;
    virtual void Load(const ControllerPtr& controller) override;
    virtual void OnFocus(unsigned int buttonIndex) override { }
    virtual void OnSelect(unsigned int buttonIndex) override;

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
    static std::vector<FeatureGroup> GetFeatureGroups(const std::vector<CControllerFeature>& features);
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
    ControllerPtr           m_controller;
    IConfigurationWizard*   m_wizard;
  };
}
}
