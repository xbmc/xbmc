/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIFeatureList.h"

#include "GUIConfigurationWizard.h"
#include "GUIControllerDefines.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/controllers/Controller.h"
#include "games/controllers/guicontrols/GUIFeatureButton.h"
#include "games/controllers/guicontrols/GUIFeatureControls.h"
#include "games/controllers/guicontrols/GUIFeatureFactory.h"
#include "games/controllers/guicontrols/GUIFeatureTranslator.h"
#include "games/controllers/input/PhysicalFeature.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIImage.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIWindow.h"
#include "guilib/LocalizeStrings.h"

using namespace KODI;
using namespace GAME;

CGUIFeatureList::CGUIFeatureList(CGUIWindow* window, GameClientPtr gameClient)
  : m_window(window), m_gameClient(std::move(gameClient)), m_wizard(new CGUIConfigurationWizard)
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
  m_guiButtonTemplate =
      dynamic_cast<CGUIButtonControl*>(m_window->GetControl(CONTROL_FEATURE_BUTTON_TEMPLATE));
  m_guiGroupTitle =
      dynamic_cast<CGUILabelControl*>(m_window->GetControl(CONTROL_FEATURE_GROUP_TITLE));
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
  const std::vector<CPhysicalFeature>& features = controller->Features();

  // Split into groups
  auto featureGroups = GetFeatureGroups(features);

  // Create controls
  m_buttonCount = 0;
  for (auto itGroup = featureGroups.begin(); itGroup != featureGroups.end(); ++itGroup)
  {
    const std::string& groupName = itGroup->groupName;
    const bool bIsVirtualKey = itGroup->bIsVirtualKey;

    std::vector<CGUIButtonControl*> buttons;

    // Create buttons
    if (bIsVirtualKey)
    {
      CGUIButtonControl* button = GetSelectKeyButton(itGroup->features, m_buttonCount);
      if (button != nullptr)
        buttons.push_back(button);
    }
    else
    {
      buttons = GetButtons(itGroup->features, m_buttonCount);
    }

    // Just in case
    if (m_buttonCount + buttons.size() >= MAX_FEATURE_COUNT)
      break;

    // Add a separator if the group list isn't empty
    if (m_guiFeatureSeparator && m_guiList->GetTotalSize() > 0)
    {
      CGUIFeatureSeparator* pSeparator =
          new CGUIFeatureSeparator(*m_guiFeatureSeparator, m_buttonCount);
      m_guiList->AddControl(pSeparator);
    }

    // Add the group title
    if (m_guiGroupTitle && !groupName.empty())
    {
      CGUIFeatureGroupTitle* pGroupTitle =
          new CGUIFeatureGroupTitle(*m_guiGroupTitle, groupName, m_buttonCount);
      m_guiList->AddControl(pGroupTitle);
    }

    // Add the buttons
    for (CGUIButtonControl* pButton : buttons)
      m_guiList->AddControl(pButton);

    m_buttonCount += static_cast<unsigned int>(buttons.size());
  }
}

void CGUIFeatureList::OnSelect(unsigned int buttonIndex)
{
  // Generate list of buttons for the wizard
  std::vector<IFeatureButton*> buttons;
  for (; buttonIndex < m_buttonCount; buttonIndex++)
  {
    IFeatureButton* control = GetButtonControl(buttonIndex);
    if (control == nullptr)
      continue;

    if (control->AllowWizard())
      buttons.push_back(control);
    else
    {
      // Only map this button if it's the only one
      if (buttons.empty())
        buttons.push_back(control);
      break;
    }
  }

  m_wizard->Run(m_controller->ID(), buttons);
}

IFeatureButton* CGUIFeatureList::GetButtonControl(unsigned int buttonIndex)
{
  CGUIControl* control = m_guiList->GetControl(CONTROL_FEATURE_BUTTONS_START + buttonIndex);

  return static_cast<IFeatureButton*>(dynamic_cast<CGUIFeatureButton*>(control));
}

void CGUIFeatureList::CleanupButtons(void)
{
  m_buttonCount = 0;

  m_wizard->Abort(true);
  m_wizard->UnregisterKeys();

  if (m_guiList)
    m_guiList->ClearAll();
}

std::vector<CGUIFeatureList::FeatureGroup> CGUIFeatureList::GetFeatureGroups(
    const std::vector<CPhysicalFeature>& features) const
{
  std::vector<FeatureGroup> groups;

  // Get group names
  std::vector<std::string> groupNames;
  for (const CPhysicalFeature& feature : features)
  {
    // Skip features not supported by the game client
    if (m_gameClient)
    {
      if (!m_gameClient->Input().HasFeature(m_controller->ID(), feature.Name()))
        continue;
    }

    bool bAdded = false;

    if (!groups.empty())
    {
      FeatureGroup& previousGroup = *groups.rbegin();
      if (feature.CategoryLabel() == previousGroup.groupName)
      {
        // Add feature to previous group
        previousGroup.features.emplace_back(feature);
        bAdded = true;

        // If feature is a key, add it to the preceding virtual group as well
        if (feature.Category() == JOYSTICK::FEATURE_CATEGORY::KEY && groups.size() >= 2)
        {
          FeatureGroup& virtualGroup = *(groups.rbegin() + 1);
          if (virtualGroup.bIsVirtualKey)
            virtualGroup.features.emplace_back(feature);
        }
      }
    }

    if (!bAdded)
    {
      // If feature is a key, create a virtual group that allows the user to
      // select which key to map
      if (feature.Category() == JOYSTICK::FEATURE_CATEGORY::KEY)
      {
        FeatureGroup virtualGroup;
        virtualGroup.groupName = g_localizeStrings.Get(35166); // "All keys"
        virtualGroup.bIsVirtualKey = true;
        virtualGroup.features.emplace_back(feature);
        groups.emplace_back(std::move(virtualGroup));
      }

      // Create new group and add feature
      FeatureGroup group;
      group.groupName = feature.CategoryLabel();
      group.features.emplace_back(feature);
      groups.emplace_back(std::move(group));
    }
  }

  // If there are no features, add an empty group
  if (groups.empty())
  {
    FeatureGroup group;
    group.groupName = g_localizeStrings.Get(35022); // "Nothing to map"
    groups.emplace_back(std::move(group));
  }

  return groups;
}

bool CGUIFeatureList::HasButton(JOYSTICK::FEATURE_TYPE type) const
{
  return CGUIFeatureTranslator::GetButtonType(type) != BUTTON_TYPE::UNKNOWN;
}

std::vector<CGUIButtonControl*> CGUIFeatureList::GetButtons(
    const std::vector<CPhysicalFeature>& features, unsigned int startIndex)
{
  std::vector<CGUIButtonControl*> buttons;

  // Create buttons
  unsigned int buttonIndex = startIndex;
  for (const CPhysicalFeature& feature : features)
  {
    BUTTON_TYPE buttonType = CGUIFeatureTranslator::GetButtonType(feature.Type());

    CGUIButtonControl* pButton = CGUIFeatureFactory::CreateButton(buttonType, *m_guiButtonTemplate,
                                                                  m_wizard, feature, buttonIndex);

    // If successful, add button to result
    if (pButton != nullptr)
    {
      buttons.push_back(pButton);
      buttonIndex++;
    }
  }

  return buttons;
}

CGUIButtonControl* CGUIFeatureList::GetSelectKeyButton(
    const std::vector<CPhysicalFeature>& features, unsigned int buttonIndex)
{
  // Expose keycodes to the wizard
  for (const CPhysicalFeature& feature : features)
  {
    if (feature.Type() == JOYSTICK::FEATURE_TYPE::KEY)
      m_wizard->RegisterKey(feature);
  }

  return CGUIFeatureFactory::CreateButton(BUTTON_TYPE::SELECT_KEY, *m_guiButtonTemplate, m_wizard,
                                          CPhysicalFeature(), buttonIndex);
}
