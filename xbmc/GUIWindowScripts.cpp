#include "stdafx.h"
#include "GUIWindowScripts.h"
#include "Util.h"
#include "lib/libPython/XBPython.h"
#include "GUIWindowScriptsInfo.h"

using namespace XFILE;

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
      if (m_vecItems.m_strPath == "?")
        m_vecItems.m_strPath = "Q:\\scripts"; //g_stSettings.m_szDefaultScripts;

      return CGUIMediaWindow::OnMessage(message);
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

bool CGUIWindowScripts::OnPlayMedia(int iItem)
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
      return true;
    }
  }
  g_pythonParser.evalFile(strPath);

  return true;
}

void CGUIWindowScripts::OnInfo()
{
  CGUIWindowScriptsInfo* pDlgInfo = (CGUIWindowScriptsInfo*)m_gWindowManager.GetWindow(WINDOW_SCRIPTS_INFO);
  if (pDlgInfo) pDlgInfo->DoModal();
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

bool CGUIWindowScripts::GetDirectory(const CStdString& strDirectory, CFileItemList& items)
{
  if (!CGUIMediaWindow::GetDirectory(strDirectory,items))
    return false;

  // flatten any folders
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItem *item = items[i];
    if (item->m_bIsFolder && !item->IsParentFolder() && !item->m_bIsShareOrDrive)
    { // folder item - let's check for a default.py file, and flatten if we have one
      CStdString defaultPY;
      CUtil::AddFileToFolder(item->m_strPath, "default.py", defaultPY);

      if (CFile::Exists(defaultPY))
      { // yes, format the item up
        item->m_strPath = defaultPY;
        item->m_bIsFolder = false;
        item->FillInDefaultIcon();
        item->SetLabelPreformated(true);
      }
    }
    if (item->GetLabel().Equals("autoexec.py"))
    {
      items.Remove(i);
      i--;
    }
  }

  items.SetProgramThumbs();

  return true;
}

void CGUIWindowScripts::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
  buttons.Add(CONTEXT_BUTTON_INFO, 654);
}

bool CGUIWindowScripts::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (button == CONTEXT_BUTTON_INFO)
  {
    OnInfo();
    return true;
  }
  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

