/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialogExtendedProgressBar.h"
#include "guilib/GUIProgressControl.h"
#include "guilib/GUISliderControl.h"
#include "threads/SingleLock.h"
#include "threads/SystemClock.h"

#define CONTROL_LABELHEADER       30
#define CONTROL_LABELTITLE        31
#define CONTROL_PROGRESS          32

#define ITEM_SWITCH_TIME_MS       2000

using namespace std;

string CGUIDialogProgressBarHandle::Text(void) const
{
  CSingleLock lock(m_critSection);
  string retVal(m_strText);
  return retVal;
}

void CGUIDialogProgressBarHandle::SetText(const string &strText)
{
  CSingleLock lock(m_critSection);
  m_strText = strText;
}

void CGUIDialogProgressBarHandle::SetTitle(const string &strTitle)
{
  CSingleLock lock(m_critSection);
  m_strTitle = strTitle;
}

void CGUIDialogProgressBarHandle::SetProgress(int currentItem, int itemCount)
{
  float fPercentage = (float)((currentItem*100)/itemCount);
  if (fPercentage > 100.0F)
    fPercentage = 100.0F;

  m_fPercentage = fPercentage;
}

CGUIDialogExtendedProgressBar::CGUIDialogExtendedProgressBar(void)
  : CGUIDialog(WINDOW_DIALOG_EXT_PROGRESS, "DialogExtendedProgressBar.xml")
{
  m_loadType        = LOAD_ON_GUI_INIT;
  m_iLastSwitchTime = 0;
  m_iCurrentItem    = 0;
}

CGUIDialogProgressBarHandle *CGUIDialogExtendedProgressBar::GetHandle(const string &strTitle)
{
  CGUIDialogProgressBarHandle *handle = new CGUIDialogProgressBarHandle(strTitle);
  {
    CSingleLock lock(m_critSection);
    m_handles.push_back(handle);
  }

  Show();

  return handle;
}

bool CGUIDialogExtendedProgressBar::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
    {
      m_iLastSwitchTime = XbmcThreads::SystemClockMillis();
      CGUIDialog::OnMessage(message);

      UpdateState(0);
      return true;
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogExtendedProgressBar::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_active)
    UpdateState(currentTime);

  CGUIDialog::Process(currentTime, dirtyregions);
}

void CGUIDialogExtendedProgressBar::UpdateState(unsigned int currentTime)
{
  bool   bNoItemsLeft(false);
  string strHeader;
  string strTitle;
  float  fProgress(-1.0f);

  {
    CSingleLock lock(m_critSection);

    // delete finished items
    for (int iPtr = m_handles.size() - 1; iPtr >= 0; iPtr--)
    {
      if (m_handles.at(iPtr)->IsFinished())
      {
        delete m_handles.at(iPtr);
        m_handles.erase(m_handles.begin() + iPtr);

        // current item deleted, back one
        if ((int)m_iCurrentItem == iPtr && m_iCurrentItem > 0)
          m_iCurrentItem--;
      }
    }

    // update the current item ptr
    if (currentTime > m_iLastSwitchTime &&
        currentTime - m_iLastSwitchTime >= ITEM_SWITCH_TIME_MS)
    {
      m_iLastSwitchTime = currentTime;

      // select next item
      if (++m_iCurrentItem > m_handles.size() - 1)
        m_iCurrentItem = 0;
    }

    if (m_iCurrentItem < m_handles.size())
    {
      CGUIDialogProgressBarHandle *handle = m_handles.at(m_iCurrentItem);
      if (handle)
      {
        strTitle  = handle->Text();
        strHeader = handle->Title();
        fProgress = handle->Percentage();
      }
    }
    else
    {
      bNoItemsLeft = true;
    }
  }

  if (bNoItemsLeft)
  {
    Close(true, 0, true, false);
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_LABELHEADER, strHeader);
    SET_CONTROL_LABEL(CONTROL_LABELTITLE, strTitle);

    if (fProgress > -1.0f)
    {
      SET_CONTROL_VISIBLE(CONTROL_PROGRESS);
      CGUIProgressControl* pProgressCtrl=(CGUIProgressControl*)GetControl(CONTROL_PROGRESS);
      if (pProgressCtrl) pProgressCtrl->SetPercentage(fProgress);
    }
  }
}
