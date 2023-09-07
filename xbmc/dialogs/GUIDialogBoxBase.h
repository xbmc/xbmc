/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "threads/CriticalSection.h"

#include <vector>

#define CONTROL_CHOICES_START  10
#define CONTROL_NO_BUTTON      CONTROL_CHOICES_START
#define CONTROL_YES_BUTTON     CONTROL_CHOICES_START + 1
#define CONTROL_CUSTOM_BUTTON  CONTROL_CHOICES_START + 2
#define CONTROL_PROGRESS_BAR   20

#define DIALOG_MAX_LINES 3
#define DIALOG_MAX_CHOICES 3

class CVariant;

class CGUIDialogBoxBase :
      public CGUIDialog
{
public:
  CGUIDialogBoxBase(int id, const std::string &xmlFile);
  ~CGUIDialogBoxBase(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool IsConfirmed() const;
  void SetLine(unsigned int iLine, const CVariant& line);
  void SetText(const CVariant& text);
  bool HasText() const;
  void SetHeading(const CVariant& heading);
  bool HasHeading() const;
  void SetChoice(int iButton, const CVariant &choice);
protected:
  std::string GetDefaultLabel(int controlId) const;
  virtual int GetDefaultLabelID(int controlId) const;
  /*! \brief Get a localized string from a variant
   If the variant is already a string we return directly, else if it's an integer we return the corresponding
   localized string.
   \param var the variant to localize.
   */
  std::string GetLocalized(const CVariant &var) const;

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

  bool m_bConfirmed;
  bool m_hasTextbox;

  // actual strings
  mutable CCriticalSection m_section;
  std::string m_strHeading;
  std::string m_text;
  std::string m_strChoices[DIALOG_MAX_CHOICES];
};
