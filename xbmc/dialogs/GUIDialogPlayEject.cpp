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

#include "Autorun.h"
#include "filesystem/EFileFile.h"
#include "GUIDialogPlayEject.h"
#include "guilib/GUIWindowManager.h"
#include "storage/MediaManager.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"

#include <utility>

#define ID_BUTTON_PLAY      11
#define ID_BUTTON_EJECT     10

using namespace XFILE;

std::string stubOriginal;
bool isDisc;

CGUIDialogPlayEject::CGUIDialogPlayEject()
    : CGUIDialogYesNo(WINDOW_DIALOG_PLAY_EJECT)
{
}

CGUIDialogPlayEject::~CGUIDialogPlayEject()
{
}

bool CGUIDialogPlayEject::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    int iControl = message.GetSenderId();
    if (iControl == ID_BUTTON_PLAY)
    {
      if (CFile::Exists(stubOriginal))
      {
        m_bConfirmed = true;
        Close();
      }

      return true;
    }
    if (iControl == ID_BUTTON_EJECT)
    {
#ifdef HAS_DVD_DRIVE
      if (isDisc)
        g_mediaManager.ToggleTray();
      else
#endif
        Close();

      return true;
    }
  }

  return CGUIDialogYesNo::OnMessage(message);
}

void CGUIDialogPlayEject::FrameMove()
{
  if (isDisc)
    stubOriginal = URIUtils::GetOpticalMediaPath();
  CONTROL_ENABLE_ON_CONDITION(ID_BUTTON_PLAY, CFile::Exists(stubOriginal));

  CGUIDialogYesNo::FrameMove();
}

void CGUIDialogPlayEject::OnInitWindow()
{
  if (CFile::Exists(stubOriginal))
  {
    m_defaultControl = ID_BUTTON_PLAY;
  }
  else
  {
    CONTROL_DISABLE(ID_BUTTON_PLAY);
    m_defaultControl = ID_BUTTON_EJECT;
  }

  CGUIDialogYesNo::OnInitWindow();
}

bool CGUIDialogPlayEject::ShowAndGetInput(const CFileItem & item,
  unsigned int uiAutoCloseTime /* = 0 */)
{
  // Make sure we're actually dealing with a Stub
  if (!item.IsEFileStub() && !item.IsDiscStub())
    return false;

  CURL stubUrl = item.GetURL();
  isDisc = item.IsDiscStub();

  int headingStr;
  int line0Str;
  int ejectButtonStr;

  if (isDisc)
  {
    headingStr = 219;
    line0Str = 429;
    ejectButtonStr = 13391;
  }
  else
  {
    stubOriginal = CEFileFile::GetTranslatedPath(stubUrl);
    if (CFile::Exists(stubOriginal))
      return true;
    headingStr = 295;
    line0Str = 472;
    ejectButtonStr = 13460;
  }

  // Create the dialog
  CGUIDialogPlayEject * pDialog = (CGUIDialogPlayEject *)g_windowManager.
    GetWindow(WINDOW_DIALOG_PLAY_EJECT);
  if (!pDialog)
    return false;

  // Figure out Lines 1 and 2 of the dialog
  std::string strLine1, strLine2;
  CEFileFile::GetMessages(stubUrl, strLine1, strLine2);


  // Use the label for Line 1 if not defined
  if (strLine1.empty())
    strLine1 = item.GetLabel();

  // Setup dialog parameters
  pDialog->SetHeading(CVariant{headingStr});
  pDialog->SetLine(0, CVariant{line0Str});
  pDialog->SetLine(1, CVariant{std::move(strLine1)});
  pDialog->SetLine(2, CVariant{std::move(strLine2)});
  pDialog->SetChoice(ID_BUTTON_PLAY - 10, 208);
  pDialog->SetChoice(ID_BUTTON_EJECT - 10, ejectButtonStr);
  if (uiAutoCloseTime)
    pDialog->SetAutoClose(uiAutoCloseTime);

  // Display the dialog
  pDialog->Open();

  return pDialog->IsConfirmed();
}
