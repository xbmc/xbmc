/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "stdafx.h"
#include "GUIDialogTVTeletext.h"
#include "PVRManager.h"
#include "Application.h"
#include "Util.h"
#include "Picture.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogOK.h"
#include "GUIWindowManager.h"
#include "ViewState.h"
#include "Settings.h"
#include "FileItem.h"

using namespace std;

#define CONTROL_TELETEXTBOX       11
#define CONTROL_LABEL_PAGENUMBER  20
#define CONTROL_LABEL_STATUS      21

#define GET_HUNDREDS(x) ( ( (x) - ((x)%256) ) /256 )
#define GET_TENS(x)  ( (( (x) - ((x)%16) )%256 ) /16 )
#define GET_ONES(x)   ( (x)%16 )

#define GET_HUNDREDS_DECIMAL(x) ( ( (x) - ((x)%100) ) /100 )
#define GET_TENS_DECIMAL(x)  ( (( (x) - ((x)%10) )%100 ) /10 )
#define GET_ONES_DECIMAL(x)   ( (x)%10 )

#define PSEUDO_HEX_TO_DECIMAL(x) ( (GET_HUNDREDS_DECIMAL(x))*256 + (GET_TENS_DECIMAL(x))*16 + (GET_ONES_DECIMAL(x)) )

CGUIDialogTVTeletext::CGUIDialogTVTeletext()
    : CGUIDialog(WINDOW_DIALOG_TV_OSD_TELETEXT, "VideoOSDTVTeletext.xml")
{
  m_currentPage           = 0x100; //Believe it or not, the teletext numbers are somehow hexadecimal
  m_currentSubPage        = 0;
  m_cursorPos             = 0;
  m_previousPage          = m_currentPage;
  m_previousSubPage       = m_currentSubPage;
  m_pageBeforeNumberInput = m_currentPage;
  m_pageFound             = true;
  m_Boxed                 = false;
  m_Blinked               = false;
  m_Concealed             = false;
  m_TeletextSupported     = false;
  m_CurrentChannel        = -1;
}

CGUIDialogTVTeletext::~CGUIDialogTVTeletext()
{

}

void CGUIDialogTVTeletext::Process()
{
  while (!m_bStop)
  {
    if (m_pageFound && m_cursorPos==0)
    {
      ShowPage();
    } 
    else if (!m_pageFound && CheckFirstSubPage(0)) 
    {
      ShowPage();
    } 
    Sleep(1000);
  }
}

bool CGUIDialogTVTeletext::OnAction(const CAction& action)
{
  int number = -1;

  if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU)
  {
    StopThread();
    Close();
    return true;
  }
  else if (action.wID == ACTION_MOVE_UP)
  {
    if (m_cursorPos != 0) 
    {
      //fully revert cursor
      SetNumber(-3);
    }
    ChangePageRelative(DirectionForward);
    ShowPage();
    return true;
  }
  else if (action.wID == ACTION_MOVE_DOWN)
  {
    if (m_cursorPos != 0) 
    {
      //fully reset
      SetNumber(-3);
    }
    ChangePageRelative(DirectionBackward);
    ShowPage();
    return true;
  }
  else if (action.wID == ACTION_MOVE_RIGHT)
  {
    if (m_cursorPos != 0) 
    {
      //fully reset
      SetNumber(-3);
    }
    ChangeSubPageRelative(DirectionForward);
    ShowPage();
    return true;
  }
  else if (action.wID == ACTION_MOVE_LEFT)
  {
    if (m_cursorPos != 0) 
    {
      //fully reset
      SetNumber(-3);
    }
    ChangeSubPageRelative(DirectionBackward);
    ShowPage();
    return true;
  }
  else if (action.wID >= REMOTE_0 && action.wID <= REMOTE_9)
    number = action.wID - REMOTE_0;
  else if (action.wID >= KEY_ASCII) // FIXME make it KEY_UNICODE
  { // input from the keyboard
    if (action.unicode >= 48 && action.unicode < 58)  // number
      number = action.unicode - 48;
  }

  if (number == 0)
  {
    if (m_cursorPos == 0)
    {
      //swap variables
      int tempPage      = m_currentPage;
      int tempSubPage   = m_currentSubPage;
      m_currentPage     = m_previousPage;
      m_currentSubPage  = m_previousSubPage;
      m_previousPage    = tempPage;
      m_previousSubPage = tempSubPage;
      ShowPage();
    } 
    else
    {
      SetNumber(0);
    }
    return true;
  }
  else if (number > 0)
  {
    SetNumber(number);
    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogTVTeletext::OnMessage(CGUIMessage& message)
{
  unsigned int iMessage = message.GetMessage();
  if (iMessage == GUI_MSG_WINDOW_INIT)
  {
    const cPVRChannelInfoTag* tag = g_PVRManager.GetCurrentPlayingItem()->GetTVChannelInfoTag();
    if (!tag)
      return false;

    int channel = tag->ClientNumber();
    if (channel != m_CurrentChannel)
    {
      m_currentPage           = 0x100; //Believe it or not, the teletext numbers are somehow hexadecimal
      m_currentSubPage        = 0;
      m_cursorPos             = 0;
      m_previousPage          = m_currentPage;
      m_previousSubPage       = m_currentSubPage;
      m_pageBeforeNumberInput = m_currentPage;
      m_pageFound             = true;
      m_Boxed                 = false;
      m_Blinked               = false;
      m_Concealed             = false;
      m_CurrentChannel        = channel;
      m_TeletextSupported     = tag->HaveTeletext();
    }
  }
  else if (iMessage == GUI_MSG_WINDOW_DEINIT)
  {
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogTVTeletext::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  Create();
  SetName("Teletext Updater");
  SetPriority(-15);
}

void CGUIDialogTVTeletext::OnWindowUnload()
{
  StopThread();
  DirtyAll = true;
  CGUIDialog::OnWindowUnload();
}

void CGUIDialogTVTeletext::Update()
{
  ShowPageNumber();
  ShowPage();
  DrawDisplay();
}

void CGUIDialogTVTeletext::Clear()
{
  DirtyAll = true;
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_TELETEXTBOX);
  OnMessage(msg);
}

void CGUIDialogTVTeletext::ShowPageNumber()
{
  char str[8];
  sprintf(str, "%3x-%02x", m_currentPage, m_currentSubPage);
  if (m_cursorPos > 0) 
  {
    str[2]='*';
    if (m_cursorPos == 1)
      str[1]='*';
  }
  SET_CONTROL_LABEL(CONTROL_LABEL_PAGENUMBER, str);
}

void CGUIDialogTVTeletext::ShowPage()
{
  unsigned char cache[40*25+12+100];

  if (!m_TeletextSupported)
  {
    SET_CONTROL_LABEL(CONTROL_LABEL_STATUS, g_localizeStrings.Get(18138));
    SET_CONTROL_HIDDEN(CONTROL_TELETEXTBOX);
    SET_CONTROL_VISIBLE(CONTROL_LABEL_STATUS);
    return;
  }
  
  if (m_currentSubPage == 0)
  {
    if (!g_application.m_pPlayer->GetTeletextPagePresent(m_currentPage, m_currentSubPage))
    {
      // There is no subpage 0 so look if there is subpage 1
      m_currentSubPage++;
    } 
  }

  if (g_application.m_pPlayer->GetTeletextPage(m_currentPage, m_currentSubPage, cache))
  {
    cRenderTTPage::ReadTeletextHeader(cache);

    if (!m_Boxed && (Flags&0x60)!=0) 
    {
      m_Boxed = true;
      Clear();
    } 
    else if (m_Boxed && (Flags&0x60)==0) 
    {
      m_Boxed = false;
      Clear();
    }

    cRenderTTPage::RenderTeletextCode(cache+12);
    DrawDisplay();
    ShowPageNumber();
    SET_CONTROL_HIDDEN(CONTROL_LABEL_STATUS);
    SET_CONTROL_VISIBLE(CONTROL_TELETEXTBOX);
    m_pageFound = true;
  } 
  else 
  {
    // page doesn't exist
    m_currentSubPage--;
    Clear();
    ShowPageNumber();

    CStdString str;
    str.Format(g_localizeStrings.Get(18137), m_currentPage, m_currentSubPage);
    SET_CONTROL_LABEL(CONTROL_LABEL_STATUS, str);
    SET_CONTROL_HIDDEN(CONTROL_TELETEXTBOX);
    SET_CONTROL_VISIBLE(CONTROL_LABEL_STATUS);
    m_pageFound = false;
  }
  return;
}

void CGUIDialogTVTeletext::DrawDisplay()
{
  int x, y;

  CGUITeletextBox *m_TeletextBox = (CGUITeletextBox*)GetControl(CONTROL_TELETEXTBOX);
  if (m_TeletextBox)
  {
    for (y = 0; y < 25; y++)
    {
      for (x = 0; x < 40; x++)
      {
        if (IsDirty(x, y))
        {
          // Need to draw char to osd
          cTeletextChar c = Page[x][y];
          c.SetDirty(false);
          if ((m_Blinked && c.GetBlink()) || (m_Concealed && c.GetConceal()))
          {
            c.SetChar(0x20);
            c.SetCharset(CHARSET_LATIN_G0_DE);
          }
          if (!m_bStop)
            m_TeletextBox->SetCharacter(c, x, y);
          Page[x][y]=c;
        }
      }
    }
  }
}

void CGUIDialogTVTeletext::SetNumber(int i)
{
  //i<0 means revert cursor position
  if (i < 0) 
  {
    for (; i < 0; i++) {
      switch (m_cursorPos) 
      {
        case 0:
          return;
        case 1:
          m_currentPage = m_currentPage-256*GET_HUNDREDS(m_currentPage)+256*GET_HUNDREDS(m_pageBeforeNumberInput);
          break;
        case 2:
          m_currentPage = m_currentPage-16*GET_TENS(m_currentPage)+16*GET_TENS(m_pageBeforeNumberInput);
          break;
      }
      m_cursorPos--;
    }
    ShowPageNumber();
    return;
  }

  static int tempPage;
  switch (m_cursorPos) 
  {
    case 0:
      if (i < 1) i = 1;
      //accept no 9 when m_cursorPos==0
      if (i > 8) i = 8;
      tempPage = m_currentPage;
      m_pageBeforeNumberInput = m_currentPage;
      m_currentPage = m_currentPage-256*GET_HUNDREDS(m_currentPage)+256*i;
      break;
    case 1:
      if (i < 0) i = 0;
      if (i > 9) i = 9;
      m_currentPage = m_currentPage-16*GET_TENS(m_currentPage)+16*i;
      break;
    case 2:
      if (i < 0) i = 0;
      if (i > 9) i = 9;
      m_currentPage = m_currentPage-GET_ONES(m_currentPage)+i;
      m_pageBeforeNumberInput = m_currentPage;
      SetPreviousPage(tempPage, m_currentSubPage, m_currentPage);
      break;
  }

  m_pageFound = true; //so that "page ... not found" is not displayed, but e.g. 1**-00
  if (++m_cursorPos > 2)
  {
    m_cursorPos = 0;
    CheckFirstSubPage(0);
    ShowPage();
  } 
  else
  {
    ShowPageNumber();
  }
}

//sets the previousPage variables if and only if new page is different from old page
void CGUIDialogTVTeletext::SetPreviousPage(int oldPage, int oldSubPage, int newPage)  
{
  if (oldPage != newPage) 
  {
    m_previousPage = oldPage;
    m_previousSubPage = oldSubPage;
  }
}

bool CGUIDialogTVTeletext::CheckFirstSubPage(int startWith) 
{
  int oldsubpage = m_currentSubPage;

  do
  {
    if (g_application.m_pPlayer->GetTeletextPagePresent(m_currentPage, m_currentSubPage))
       return true;
    m_currentSubPage = nextValidPageNumber(m_currentSubPage, DirectionForward);

    if (m_currentSubPage > 0x99) m_currentSubPage = 0;
    if (m_currentSubPage < 0) m_currentSubPage = 0x99;

  } while (m_currentSubPage != oldsubpage);

  return false;
}

//returns whether x, when written in hexadecimal form,
//will only contain the digits 0...9 and not A...F
//in the first three digits.
static inline bool onlyDecimalDigits(int x)
{
  return ((  x      & 0xE) < 0xA) &&
         (( (x>>4)  & 0xE) < 0xA) &&
         (( (x>>8)  & 0xE) < 0xA);
}

//after 199 comes 1A0, but if these pages exist, they contain no useful data, so filter them out
int CGUIDialogTVTeletext::nextValidPageNumber(int start, Direction direction)
{
  do
  {
    switch (direction)
    {
      case DirectionForward:
        start++;
        break;
      case DirectionBackward:
        start--;
        break;
    }
  } while (!onlyDecimalDigits(start));
  return start;
}

void CGUIDialogTVTeletext::ChangePageRelative(Direction direction)
{
  int oldpage = m_currentPage;
  int oldSubPage = m_currentSubPage;

  do
  {
    m_currentPage = nextValidPageNumber(m_currentPage, direction);
    if (m_currentPage > 0x899) m_currentPage = 0x100;
    if (m_currentPage < 0x100) m_currentPage = 0x899;
    // sub page is always 0 if you change the page
    if (CheckFirstSubPage(0))
    {
      SetPreviousPage(oldpage, oldSubPage, m_currentPage);
      return;
    }
  } while (m_currentPage != oldpage);

  return;
}

void CGUIDialogTVTeletext::ChangeSubPageRelative(Direction direction)
{
  int oldsubpage = m_currentSubPage;
  do
  {
    m_currentSubPage = nextValidPageNumber(m_currentSubPage, direction);
    if (m_currentSubPage > 0x99) m_currentSubPage=0;
    if (m_currentSubPage < 0) m_currentSubPage=0x99;

    if (g_application.m_pPlayer->GetTeletextPagePresent(m_currentPage, m_currentSubPage))
      return;
  } while (m_currentSubPage != oldsubpage);

  return;
}
