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

#include "GUIFeatureList.h"

#include "GUIConfigurationWizard.h"
#include "GUIControllerDefines.h"
#include "games/controllers/guicontrols/GUIAnalogStickButton.h"
#include "games/controllers/guicontrols/GUIFeatureControls.h"
#include "games/controllers/guicontrols/GUIScalarFeatureButton.h"
#include "games/controllers/Controller.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIImage.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIWindow.h"

using namespace GAME;

CGUIFeatureList::CGUIFeatureList(CGUIWindow* window) :
  m_window(window),
  m_guiList(nullptr),
  m_guiButtonTemplate(nullptr),
  m_guiGroupTitle(nullptr),
  m_guiFeatureSeparator(nullptr),
  m_wizard(new CGUIConfigurationWizard)
{
}

CGUIFeatureList::~CGUIFeatureList(void)
{
  Deinitialize();
  delete m_wizard;
}

bool CGUIFeatureList::Initialize(void)
{
  m_guiList = dynamic_cast<CGUIControlGroupList*>(m_window->GetControl(CONTROL_FEATURE_LIST));
  m_guiButtonTemplate = dynamic_cast<CGUIButtonControl*>(m_window->GetControl(CONTROL_FEATURE_BUTTON_TEMPLATE));
  m_guiGroupTitle = dynamic_cast<CGUILabelControl*>(m_window->GetControl(CONTROL_FEATURE_GROUP_TITLE));
  m_guiFeatureSeparator = dynamic_cast<CGUIImage*>(m_window->GetControl(CONTROL_FEATURE_SEPARATOR));

  if (m_guiButtonTemplate)
    m_guiButtonTemplate->SetVisible(false);

  if (m_guiGroupTitle)
    m_guiGroupTitle->SetVisible(false);

  if (m_guiFeatureSeparator)
    m_guiFeatureSeparator->SetVisible(false);

  return m_guiList != nullptr && m_guiButtonTemplate != nullptr;
}

void CGUIFeatureList::Deinitialize(void)
{
  CleanupButtons();

  m_guiList = nullptr;
  m_guiButtonTemplate = nullptr;
  m_guiGroupTitle = nullptr;
  m_guiFeatureSeparator = nullptr;
}

void CGUIFeatureList::Load(const ControllerPtr& controller)
{
  if (m_controller && m_controller->ID() == controller->ID())
    return; // Already loaded

  CleanupButtons();

  // Set new controller
  m_controller = controller;

  // Get features
  const std::vector<CControllerFeature>& features = controller->Layout().Features();

  // Split into groups
  auto featureGroups = GetFeatureGroups(features);

  // Create controls
  unsigned int featureIndex = 0;
  for (auto itGroup = featureGroups.begin(); itGroup != featureGroups.end(); ++itGroup)
  {
    const std::string& groupName = itGroup->groupName;

    // Create buttons
    std::vector<CGUIButtonControl*> buttons = GetButtons(itGroup->features, featureIndex);
    if (!buttons.empty())
    {
      // Add a separator if the group list isn't empty
      if (m_guiFeatureSeparator && m_guiList->GetTotalSize() > 0)
      {
        CGUIFeatureSeparator* pSeparator = new CGUIFeatureSeparator(*m_guiFeatureSeparator, featureIndex);
        m_guiList->AddControl(pSeparator);
      }

      // Add the group title
      if (m_guiGroupTitle && !groupName.empty())
      {
        CGUIFeatureGroupTitle* pGroupTitle = new CGUIFeatureGroupTitle(*m_guiGroupTitle, groupName, featureIndex);
        m_guiList->AddControl(pGroupTitle);
      }

      // Add the buttons
      for (CGUIButtonControl* pButton : buttons)
        m_guiList->AddControl(pButton);

      featureIndex += itGroup->features.size();
    }

    // Just in case
    if (featureIndex >= MAX_FEATURE_COUNT)
      break;
  }
}

void CGUIFeatureList::OnSelect(unsigned int index)
{
  const unsigned int featureCount = m_controller->Layout().FeatureCount();

  // Generate list of buttons for the wizard
  std::vector<IFeatureButton*> buttons;
  for ( ; index < featureCount; index++)
  {
    IFeatureButton* control = GetButtonControl(index);
    if (control)
      buttons.push_back(control);
  }

  m_wizard->Run(m_controller->ID(), buttons);
}

IFeatureButton* CGUIFeatureList::GetButtonControl(unsigned int featureIndex)
{
  CGUIControl* control = m_guiList->GetControl(CONTROL_FEATURE_BUTTONS_START + featureIndex);

  return dynamic_cast<CGUIFeatureButton*>(control);
}

void CGUIFeatureList::CleanupButtons(void)
{
  m_wizard->Abort(true);

  if (m_guiList)
    m_guiList->ClearAll();
}

std::vector<CGUIFeatureList::FeatureGroup> CGUIFeatureList::GetFeatureGroups(const std::vector<CControllerFeature>& features)
{
  std::vector<CGUIFeatureList::FeatureGroup> groups;

  // Get group names
  std::vector<std::string> groupNames;
  for (const CControllerFeature& feature : features)
  {
    if (std::find(groupNames.begin(), groupNames.end(), feature.Group()) == groupNames.end())
      groupNames.push_back(feature.Group());
  }

  // Divide features into groups
  for (std::string& groupName : groupNames)
  {
    FeatureGroup group = { groupName };
    for (const CControllerFeature& feature : features)
    {
      if (feature.Group() == groupName)
        group.features.push_back(feature);
    }
    groups.emplace_back(std::move(group));
  }

  return groups;
}

std::vector<CGUIButtonControl*> CGUIFeatureList::GetButtons(const std::vector<CControllerFeature>& features, unsigned int startIndex)
{
  std::vector<CGUIButtonControl*> buttons;

  // Create buttons
  unsigned int featureIndex = startIndex;
  for (const CControllerFeature& feature : features)
  {
    CGUIButtonControl* pButton = nullptr;

    // Create button
    switch (feature.Type())
    {
      case JOYSTICK::FEATURE_TYPE::SCALAR:
      {
        pButton = new CGUIScalarFeatureButton(*m_guiButtonTemplate, m_wizard, feature, featureIndex);
        break;
      }
      case JOYSTICK::FEATURE_TYPE::ANALOG_STICK:
      {
        pButton = new CGUIAnalogStickButton(*m_guiButtonTemplate, m_wizard, feature, featureIndex);
        break;
      }
      default:
        break;
    }

    // If successful, add button to result
    if (pButton)
      buttons.push_back(pButton);

    featureIndex++;
  }

  return buttons;
}
