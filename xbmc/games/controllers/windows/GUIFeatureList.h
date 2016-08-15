/*
 *      Copyright (C) 2014-2016 Team Kodi
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

class CGUIButtonControl;
class CGUIControlGroupList;
class CGUIImage;
class CGUILabelControl;
class CGUIWindow;

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
    virtual void Load(const ControllerPtr& controller) override;
    virtual void OnFocus(unsigned int index) override { }
    virtual void OnSelect(unsigned int index) override;

  private:
    IFeatureButton* GetButtonControl(unsigned int featureIndex);

    void CleanupButtons(void);

    // Helper functions
    struct FeatureGroup
    {
      std::string groupName;
      std::vector<CControllerFeature> features;
    };
    static std::vector<FeatureGroup> GetFeatureGroups(const std::vector<CControllerFeature>& features);
    std::vector<CGUIButtonControl*> GetButtons(const std::vector<CControllerFeature>& features, unsigned int startIndex);

    // GUI stuff
    CGUIWindow* const       m_window;
    CGUIControlGroupList*   m_guiList;
    CGUIButtonControl*      m_guiButtonTemplate;
    CGUILabelControl*       m_guiGroupTitle;
    CGUIImage*              m_guiFeatureSeparator;

    // Game window stuff 
    ControllerPtr           m_controller;
    IConfigurationWizard*   m_wizard;
  };
}
