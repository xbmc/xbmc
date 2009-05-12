#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIDialog.h"
#include "visualizations/Visualisation.h"
#include "GUISpinControlEx.h"
#include "GUIRadioButtonControl.h"
#include "GUIButtonControl.h"
#include "settings/AddonSettings.h"

class CGUIDialogVisualisationSettings :
      public CGUIDialog
{
public:
  CGUIDialogVisualisationSettings(void);
  virtual ~CGUIDialogVisualisationSettings(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Render();
protected:
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  void SetVisualisation(CVisualisation *pVisualisation);
  void OnClick(int iControl);

  CVisualisation *m_pVisualisation;
  CAddonSettings m_pSettings;

private:
  bool ShowVirtualKeyboard(int iControl);
  void CreateControls();
  void FreeControls();
  void EnableControls();
  bool GetCondition(const CStdString &condition, const int controlId);
  bool TranslateSingleString(const CStdString &strCondition, std::vector<CStdString> &enableVec);
};
