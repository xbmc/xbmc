#pragma once

/*
 *      Copyright (C) 2015 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guilib/GUIDialog.h"

class CGUIDialogTextViewer :
      public CGUIDialog
{
public:
  CGUIDialogTextViewer(void);
  virtual ~CGUIDialogTextViewer(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  void SetText(const std::string& strText) { m_strText = strText; }
  void SetHeading(const std::string& strHeading) { m_strHeading = strHeading; }
protected:
  virtual void OnDeinitWindow(int nextWindowID);

  std::string m_strText;
  std::string m_strHeading;

  void SetText();
  void SetHeading();
};

