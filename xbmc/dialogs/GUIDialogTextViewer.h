/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

class CGUIDialogTextViewer :
      public CGUIDialog
{
public:
  CGUIDialogTextViewer(void);
  ~CGUIDialogTextViewer(void) override;
  bool OnMessage(CGUIMessage& message) override;
  void SetText(const std::string& strText) { m_strText = strText; }
  void SetHeading(const std::string& strHeading) { m_strHeading = strHeading; }
  void UseMonoFont(bool use);

  //! \brief Load a file into memory and show in dialog.
  //! \param path Path to file
  //! \param useMonoFont True to use monospace font
  static void ShowForFile(const std::string& path, bool useMonoFont);
protected:
  void OnDeinitWindow(int nextWindowID) override;
  bool OnAction(const CAction &action) override;

  std::string m_strText;
  std::string m_strHeading;
  bool m_mono = false;

  void SetText();
  void SetHeading();
};

