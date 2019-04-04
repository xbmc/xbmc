/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <vector>
#include "guilib/GUIDialog.h"
#include "Network.h"

class CFileItemList;

class CGUIDialogAccessPoints : public CGUIDialog
{
public:
  CGUIDialogAccessPoints(void);
  ~CGUIDialogAccessPoints(void) override;
  void OnInitWindow() override;
  bool OnAction(const CAction &action) override;
  void SetInterfaceName(std::string interfaceName);
  std::string GetSelectedAccessPointEssId();
  EncMode GetSelectedAccessPointEncMode();
  bool WasItemSelected();

private:
  std::vector<NetworkAccessPoint> m_aps;
  std::string m_interfaceName;
  std::string m_selectedAPEssId;
  EncMode m_selectedAPEncMode;
  bool m_wasItemSelected;
  CFileItemList *m_accessPoints;
};
