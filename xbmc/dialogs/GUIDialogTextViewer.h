#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guilib/GUIDialog.h"

class CGUIDialogTextViewer :
      public CGUIDialog
{
public:
  CGUIDialogTextViewer(void);
  ~CGUIDialogTextViewer(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  void SetText(const std::string& strText) { m_strText = strText; }
  void SetHeading(const std::string& strHeading) { m_strHeading = strHeading; }
protected:
  void OnDeinitWindow(int nextWindowID) override;

  std::string m_strText;
  std::string m_strHeading;

  void SetText();
  void SetHeading();
};

