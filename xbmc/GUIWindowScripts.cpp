#include "stdafx.h"
#include "guiwindowscripts.h"
#include "util.h"
#include "lib/libPython/XBPython.h"
#include "GUIWindowScriptsInfo.h"
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
#include "SkinInfo.h"
#endif

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_LABELFILES        12


CGUIWindowScripts::CGUIWindowScripts()
    : CGUIMediaWindow(WINDOW_SCRIPTS, "MyScripts.xml")
{
  m_bViewOutput = false;
  m_scriptSize = 0;
}

CGUIWindowScripts::~CGUIWindowScripts()
{
}

bool CGUIWindowScripts::OnAction(const CAction &action)
{
  if (action.wID == ACTION_SHOW_INFO)
  {
    OnInfo();
    return true;
  }
  return CGUIMediaWindow::OnAction(action);
}

bool CGUIWindowScripts::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      int iLastControl = m_iLastControl;
      CGUIWindow::OnMessage(message);
      m_shares.erase(m_shares.begin(), m_shares.end());
      if (m_vecItems.m_strPath == "?") m_vecItems.m_strPath = "Q:\\scripts"; //g_stSettings.m_szDefaultScripts;

      m_rootDir.SetMask("*.py");

      CShare share;
      share.strName = "Q Drive";
      share.strPath = "Q:\\scripts";
      share.m_iBufferSize = 0;
      share.m_iDriveType = SHARE_TYPE_LOCAL;
      m_shares.push_back(share);

      m_rootDir.SetShares(m_shares);

      Update(m_vecItems.m_strPath);

      if (iLastControl > -1)
      {
        SET_CONTROL_FOCUS(iLastControl, 0);
      }
      else
      {
        SET_CONTROL_FOCUS(m_dwDefaultFocusControlID, 0);
      }

      if (m_iSelectedItem > -1)
        m_viewControl.SetSelectedItem(m_iSelectedItem);

      return true;
    }
    break;
  }
  return CGUIMediaWindow::OnMessage(message);
}

bool CGUIWindowScripts::Update(const CStdString &strDirectory)
{
  // Look if baseclass can handle it
  if (!CGUIMediaWindow::Update(strDirectory))
    return false;

  /* check if any python scripts are running. If true, place "(Running)" after the item.
   * since stopping a script can take up to 10 seconds or more,we display 'stopping'
   * after the filename for now.
   */
  int iSize = g_pythonParser.ScriptsSize();
  for (int i = 0; i < iSize; i++)
  {
    int id = g_pythonParser.GetPythonScriptId(i);
    if (g_pythonParser.isRunning(id))
    {
      const char* filename = g_pythonParser.getFileName(id);

      for (int i = 0; i < m_vecItems.Size(); i++)
      {
        CFileItem* pItem = m_vecItems[i];
        if (pItem->m_strPath == filename)
        {
          char tstr[1024];
          strcpy(tstr, pItem->GetLabel());
          if (g_pythonParser.isStopping(id))
            strcat(tstr, " (Stopping)");
          else
            strcat(tstr, " (Running)");
          pItem->SetLabel(tstr);
        }
      }
    }
  }

  return true;
}

void CGUIWindowScripts::OnPlayMedia(int iItem)
{
  CFileItem* pItem=m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;

  /* execute script...
    * if script is already running do not run it again but stop it.
    */
  int id = g_pythonParser.getScriptId(strPath);
  if (id != -1)
  {
    /* if we are here we already know that this script is running.
      * But we will check it again to be sure :)
      */
    if (g_pythonParser.isRunning(id))
    {
      g_pythonParser.stopScript(id);

      // update items
      int selectedItem = m_viewControl.GetSelectedItem();
      Update(m_vecItems.m_strPath);
      m_viewControl.SetSelectedItem(selectedItem);
      return;
    }
  }
  g_pythonParser.evalFile(strPath);
}

void CGUIWindowScripts::OnInfo()
{
  CGUIWindowScriptsInfo* pDlgInfo = (CGUIWindowScriptsInfo*)m_gWindowManager.GetWindow(WINDOW_SCRIPTS_INFO);
  if (pDlgInfo) pDlgInfo->DoModal(GetID());
}

void CGUIWindowScripts::Render()
{
  // update control_list / control_thumbs if one or more scripts have stopped / started
  if (g_pythonParser.ScriptsSize() != m_scriptSize)
  {
    int selectedItem = m_viewControl.GetSelectedItem();
    Update(m_vecItems.m_strPath);
    m_viewControl.SetSelectedItem(selectedItem);
    m_scriptSize = g_pythonParser.ScriptsSize();
  }
  CGUIWindow::Render();
}

void CGUIWindowScripts::OnWindowLoaded()
{
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
  if (g_SkinInfo.GetVersion() < 1.8)
  {
    ChangeControlID(10, CONTROL_LIST, CGUIControl::GUICONTROL_LIST);
    ChangeControlID(11, CONTROL_THUMBS, CGUIControl::GUICONTROL_THUMBNAIL);
  }
#endif
  CGUIMediaWindow::OnWindowLoaded();
}
