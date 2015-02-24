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

#ifdef HAS_DS_PLAYER

#include "GUIDialogShaderList.h"
#include "guilib/GUIListGroup.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIRadioButtonControl.h"
#include "PixelShaderList.h"
#include "Filters\RendererSettings.h"

#define CONTROL_LBL_NOSHADER        3
#define CONTROL_AREA                4
#define CONTROL_SCROLLBAR           60

#define CONTROL_DEFAULT_GRP         1000

#define CONTROL_START_IDS           2000
#define CONTROL_RADIOBUTTON         2100
#define CONTROL_BTN_UP              2200
#define CONTROL_BTN_DOWN            2300

#if 0

CGUIDialogShaderList::CGUIDialogShaderList(void)
  : CGUIDialog(WINDOW_DIALOG_SHADER_LIST, "DialogShaderList.xml")
{

}

CGUIDialogShaderList::~CGUIDialogShaderList(void)
{
}

bool CGUIDialogShaderList::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
  {

  }
    break;

  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();
    int iAction = ((int)iControl / 100) * 100;
    uint16_t index = iControl - iAction;

    CGUIListGroup * grp = (CGUIListGroup *)m_mainGrp->GetControl(CONTROL_START_IDS + index);
    if (!grp)
      return true;

    CGUIControl* pControl = NULL;

    switch (iAction)
    {
    case CONTROL_BTN_UP:
      g_dsSettings.pixelShaderList->MoveUp(index);
      UpdateControls();
      return true;
      break;
    case CONTROL_BTN_DOWN:
      g_dsSettings.pixelShaderList->MoveDown(index);
      UpdateControls();
      return true;
      break;
    }

    CGUIRadioButtonControl *pRadio = (CGUIRadioButtonControl *)grp->GetControl(CONTROL_RADIOBUTTON);
    if (!pRadio)
      return true;

    g_dsSettings.pixelShaderList->GetPixelShaders()[index]->SetEnabled(pRadio->IsSelected());
    g_dsSettings.pixelShaderList->UpdateActivatedList();
    UpdateControls();

  }
    break;
  default:
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogShaderList::OnInitWindow()
{
  UpdateControls();
  CGUIDialog::OnInitWindow();
}

void CGUIDialogShaderList::UpdateControls()
{
  CPixelShaderList* psList = g_dsSettings.pixelShaderList.get();

  if (!psList->GetPixelShaders().empty())
    SET_CONTROL_HIDDEN(CONTROL_LBL_NOSHADER);
  else
    return;

  m_mainGrp = (CGUIListGroup *)GetControl(CONTROL_AREA);
  CGUIListGroup * defaultGrp = (CGUIListGroup *)GetControl(CONTROL_DEFAULT_GRP);
  if (!defaultGrp || !m_mainGrp)
    return;

  defaultGrp->SetVisible(false);
  m_mainGrp->ClearAll();

  bool isFirst = true;

  PixelShaderVector& psVector = psList->GetPixelShaders();
  CGUIListGroup * pControl = NULL;

  uint16_t index = 0;
  int right, up, down;

#define NEXT_RADIOBUTTON  (down + 100)
#define PREVIOUS_RADIOBUTTON  (up + 100)
#define NEXT_UPBUTTON (down + 200)
#define PREVIOUS_UPBUTTON (up + 200)
#define NEXT_DOWNBUTTON (down + 300)
#define PREVIOUS_DOWNBUTTON (up + 300)
#define CURRENT_RADIOBUTTON (pRadio->GetID())
#define CURRENT_UPBUTTON (CURRENT_RADIOBUTTON + 100)
#define CURRENT_DOWNBUTTON (CURRENT_RADIOBUTTON + 200)

  for (PixelShaderVector::iterator it = psVector.begin();
    it != psVector.end(); ++it)
  {
    CExternalPixelShader* ps = (*it);

    pControl = new CGUIListGroup(*defaultGrp);
    if (!pControl)
      continue;

    pControl->SetID(CONTROL_START_IDS + index);
    if (index == 0)
    {
      up = CONTROL_START_IDS + psVector.size() - 1;
      down = CONTROL_START_IDS + index + 1;
    }
    else if (index == psVector.size() - 1)
    {
      up = CONTROL_START_IDS + index - 1;
      down = CONTROL_START_IDS;
    }
    else
    {
      up = CONTROL_START_IDS + index - 1;
      down = CONTROL_START_IDS + index + 1;
    }
    pControl->SetNavigation(up, down, 60, 60);

    CGUIRadioButtonControl* pRadio = (CGUIRadioButtonControl *)pControl->GetControl(CONTROL_RADIOBUTTON);
    if (!pRadio)
      return;

    pRadio->SetLabel(ps->GetName());
    pRadio->SetID(pRadio->GetID() + index);
    pRadio->SetSelected(ps->IsEnabled());

    CGUIButtonControl * upBtn = (CGUIButtonControl *)pControl->GetControl(CONTROL_BTN_UP);
    if (!upBtn)
      continue;

    upBtn->SetID(upBtn->GetID() + index);
    if (isFirst || !ps->IsEnabled())
    {
      upBtn->SetVisible(false);
      if (isFirst)
      {
        pControl->SetFocus(true);
        isFirst = false;
      }
    }

    CGUIButtonControl *downBtn = (CGUIButtonControl *)pControl->GetControl(CONTROL_BTN_DOWN);
    if (!downBtn)
      continue;

    downBtn->SetID(downBtn->GetID() + index);

    PixelShaderVector::iterator it2 = it + 1;
    if (!ps->IsEnabled() || (it2 != psVector.end()
      && !(*it2)->IsEnabled()))
      downBtn->SetVisible(false); // Only visible if pixel shader is enabled

    if (upBtn->IsVisible())
      right = upBtn->GetID();
    else if (downBtn->IsVisible())
      right = downBtn->GetID();
    else
      right = CONTROL_SCROLLBAR;
    pRadio->SetNavigation(PREVIOUS_RADIOBUTTON, NEXT_RADIOBUTTON, CONTROL_SCROLLBAR, right);

    if (downBtn->IsVisible())
      upBtn->SetNavigation(PREVIOUS_DOWNBUTTON, CURRENT_DOWNBUTTON, CURRENT_RADIOBUTTON, CONTROL_SCROLLBAR);
    else
      upBtn->SetNavigation(PREVIOUS_DOWNBUTTON, NEXT_RADIOBUTTON, CURRENT_RADIOBUTTON, CONTROL_SCROLLBAR); // fallback on RadioButton

    if (upBtn->IsVisible())
      downBtn->SetNavigation(CURRENT_UPBUTTON, NEXT_UPBUTTON, CURRENT_RADIOBUTTON, CONTROL_SCROLLBAR);
    else
      downBtn->SetNavigation(PREVIOUS_RADIOBUTTON, NEXT_UPBUTTON, CURRENT_RADIOBUTTON, CONTROL_SCROLLBAR);

    pControl->SetVisible(true);
    m_mainGrp->AddControl(pControl);

    index++;
  }
}

#endif
#endif