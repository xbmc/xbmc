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

#include "pyutil.h"
#include "GUIPythonWindowXML.h"
#include "window.h"
#include "control.h"
#include "action.h"
#include "utils/URIUtils.h"
#include "guilib/GUIWindowManager.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "guilib/TextureManager.h"
#include "../XBPython.h"
#include "settings/GUISettings.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "tinyXML/tinyxml.h"

using namespace std;

#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY       3
#define CONTROL_BTNSORTASC      4
#define CONTROL_LABELFILES      12

using namespace PYXBMC;

CGUIPythonWindowXML::CGUIPythonWindowXML(int id, CStdString strXML, CStdString strFallBackPath)
  : CGUIMediaWindow(id, strXML), m_actionEvent(true)
{
  pCallbackWindow = NULL;
  m_threadState = NULL;
  m_loadOnDemand = false;
  m_scriptPath = strFallBackPath;
  m_destroyAfterDeinit = false;
}

CGUIPythonWindowXML::~CGUIPythonWindowXML(void)
{
}

bool CGUIPythonWindowXML::Update(const CStdString &strPath)
{
  return true;
}

bool CGUIPythonWindowXML::OnAction(const CAction &action)
{
  // call the base class first, then call python
  bool ret = CGUIWindow::OnAction(action);
  if(pCallbackWindow)
  {
    PyXBMCAction* inf = new PyXBMCAction(pCallbackWindow);
    inf->pObject = Action_FromAction(action);

    // aquire lock?
    PyXBMC_AddPendingCall((PyThreadState*)m_threadState, Py_XBMC_Event_OnAction, inf);
    PulseActionEvent();
  }
  return ret;
}

bool CGUIPythonWindowXML::OnBack(int actionID)
{
  // if we have a callback window then python handles the closing
  if (!pCallbackWindow)
    return CGUIWindow::OnBack(actionID);
  return true;
}

bool CGUIPythonWindowXML::OnClick(int iItem) {
  // Hook Over calling  CGUIMediaWindow::OnClick(iItem) results in it trying to PLAY the file item
  // which if its not media is BAD and 99 out of 100 times undesireable.
  return false;
}

// SetupShares();
/*
 CGUIMediaWindow::OnWindowLoaded() calls SetupShares() so override it
and just call UpdateButtons();
*/
void CGUIPythonWindowXML::SetupShares()
{
  UpdateButtons();
}

bool CGUIPythonWindowXML::OnMessage(CGUIMessage& message)
{
  // TODO: We shouldn't be dropping down to CGUIWindow in any of this ideally.
  //       We have to make up our minds about what python should be doing and
  //       what this side of things should be doing
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      return CGUIMediaWindow::OnMessage(message);
    }
    break;

    case GUI_MSG_WINDOW_INIT:
    {
      CGUIMediaWindow::OnMessage(message);
      if(pCallbackWindow)
      {
        PyXBMC_AddPendingCall((PyThreadState*)m_threadState, Py_XBMC_Event_OnInit, new PyXBMCAction(pCallbackWindow));
        PulseActionEvent();
      }
      return true;
    }
    break;

    case GUI_MSG_FOCUSED:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != (int)message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
        // check if our focused control is one of our category buttons
        int iControl=message.GetControlId();
        if(pCallbackWindow)
        {
          PyXBMCAction* inf = new PyXBMCAction(pCallbackWindow);
          inf->controlId = iControl;
          // aquire lock?
          PyXBMC_AddPendingCall((PyThreadState*)m_threadState, Py_XBMC_Event_OnFocus, inf);
          PulseActionEvent();
        }
    }
    break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      // Handle Sort/View internally. Scripters shouldn't use ID 2, 3 or 4.
      if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        CLog::Log(LOGINFO, "WindowXML: Internal asc/dsc button not implemented");
        /*if (m_guiState.get())
          m_guiState->SetNextSortOrder();
        UpdateFileList();*/
        return true;
      }
      else if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        CLog::Log(LOGINFO, "WindowXML: Internal sort button not implemented");
        /*if (m_guiState.get())
          m_guiState->SetNextSortMethod();
        UpdateFileList();*/
        return true;
      }
      else if (iControl == CONTROL_BTNVIEWASICONS)
      { // base class handles this one
        break;
      }
      if(pCallbackWindow && iControl && iControl != (int)this->GetID()) // pCallbackWindow &&  != this->GetID())
      {
        CGUIControl* controlClicked = (CGUIControl*)this->GetControl(iControl);

        // The old python way used to check list AND SELECITEM method or if its a button, checkmark.
        // Its done this way for now to allow other controls without a python version like togglebutton to still raise a onAction event
        if (controlClicked) // Will get problems if we the id is not on the window and we try to do GetControlType on it. So check to make sure it exists
        {
          if ((controlClicked->IsContainer() && (message.GetParam1() == ACTION_SELECT_ITEM || message.GetParam1() == ACTION_MOUSE_LEFT_CLICK)) || !controlClicked->IsContainer())
          {
            PyXBMCAction* inf = new PyXBMCAction(pCallbackWindow);
            inf->controlId = iControl;
            // aquire lock?
            PyXBMC_AddPendingCall((PyThreadState*)m_threadState, Py_XBMC_Event_OnClick, inf);
            PulseActionEvent();
            return true;
          }
          else if (controlClicked->IsContainer() && message.GetParam1() == ACTION_MOUSE_RIGHT_CLICK)
          {
            PyXBMCAction* inf = new PyXBMCAction(pCallbackWindow);
            inf->pObject = Action_FromAction(CAction(ACTION_CONTEXT_MENU));

            // aquire lock?
            PyXBMC_AddPendingCall((PyThreadState*)m_threadState, Py_XBMC_Event_OnAction, inf);
            PulseActionEvent();
            return true;
          }
        }
      }
    }
    break;
  }

  return CGUIMediaWindow::OnMessage(message);
}

void CGUIPythonWindowXML::OnDeinitWindow(int nextWindowID /*= 0*/)
{
  CGUIMediaWindow::OnDeinitWindow(nextWindowID);
  if (m_destroyAfterDeinit)
    g_windowManager.Delete(GetID());
}

void CGUIPythonWindowXML::SetDestroyAfterDeinit(bool destroy /*= true*/)
{
  m_destroyAfterDeinit = destroy;
}

void CGUIPythonWindowXML::AddItem(CFileItemPtr fileItem, int itemPosition)
{
  if (itemPosition == INT_MAX || itemPosition > m_vecItems->Size())
  {
    m_vecItems->Add(fileItem);
  }
  else if (itemPosition <  -1 &&  !(itemPosition*-1 < m_vecItems->Size()))
  {
    m_vecItems->AddFront(fileItem,0);
  }
  else
  {
    m_vecItems->AddFront(fileItem,itemPosition);
  }
  m_viewControl.SetItems(*m_vecItems);
  UpdateButtons();
}

void CGUIPythonWindowXML::RemoveItem(int itemPosition)
{
  m_vecItems->Remove(itemPosition);
  m_viewControl.SetItems(*m_vecItems);
  UpdateButtons();
}

int CGUIPythonWindowXML::GetListSize()
{
  return m_vecItems->Size();
}

int CGUIPythonWindowXML::GetCurrentListPosition()
{
  return m_viewControl.GetSelectedItem();
}

void CGUIPythonWindowXML::SetCurrentListPosition(int item)
{
  m_viewControl.SetSelectedItem(item);
}

void CGUIPythonWindowXML::SetProperty(const CStdString& key, const CStdString& value)
{
  m_vecItems->SetProperty(key, value);
}

CFileItemPtr CGUIPythonWindowXML::GetListItem(int position)
{
  if (position < 0 || position >= m_vecItems->Size()) return CFileItemPtr();
  return m_vecItems->Get(position);
}

void CGUIPythonWindowXML::ClearList()
{
  ClearFileItems();

  m_viewControl.SetItems(*m_vecItems);
  UpdateButtons();
}

void CGUIPythonWindowXML::WaitForActionEvent(unsigned int timeout)
{
  g_pythonParser.WaitForEvent(m_actionEvent, timeout);
  m_actionEvent.Reset();
}

void CGUIPythonWindowXML::PulseActionEvent()
{
  m_actionEvent.Set();
}

void CGUIPythonWindowXML::AllocResources(bool forceLoad /*= FALSE */)
{
  CStdString tmpDir;
  URIUtils::GetDirectory(GetProperty("xmlfile").asString(), tmpDir);
  CStdString fallbackMediaPath;
  URIUtils::GetParentPath(tmpDir, fallbackMediaPath);
  URIUtils::RemoveSlashAtEnd(fallbackMediaPath);
  m_mediaDir = fallbackMediaPath;

  //CLog::Log(LOGDEBUG, "CGUIPythonWindowXML::AllocResources called: %s", fallbackMediaPath.c_str());
  g_TextureManager.AddTexturePath(m_mediaDir);
  CGUIMediaWindow::AllocResources(forceLoad);
  g_TextureManager.RemoveTexturePath(m_mediaDir);
}

bool CGUIPythonWindowXML::LoadXML(const CStdString &strPath, const CStdString &strLowerPath)
{
  // load our window
  XFILE::CFile file;
  if (!file.Open(strPath) && !file.Open(CStdString(strPath).ToLower()) && !file.Open(strLowerPath))
  {
    // fail - can't load the file
    CLog::Log(LOGERROR, "%s: Unable to load skin file %s", __FUNCTION__, strPath.c_str());
    return false;
  }
  // load the strings in
  unsigned int offset = LoadScriptStrings();

  CStdString xml;
  char *buffer = new char[(unsigned int)file.GetLength()+1];
  if(buffer == NULL)
    return false;
  int size = file.Read(buffer, file.GetLength());
  if (size > 0)
  {
    buffer[size] = 0;
    xml = buffer;
    if (offset)
    {
      // replace the occurences of SCRIPT### with offset+###
      // not particularly efficient, but it works
      int pos = xml.Find("SCRIPT");
      while (pos != (int)CStdString::npos)
      {
        CStdString num = xml.Mid(pos + 6, 4);
        int number = atol(num.c_str());
        CStdString oldNumber, newNumber;
        oldNumber.Format("SCRIPT%d", number);
        newNumber.Format("%lu", offset + number);
        xml.Replace(oldNumber, newNumber);
        pos = xml.Find("SCRIPT", pos + 6);
      }
    }
  }
  delete[] buffer;

  TiXmlDocument xmlDoc;
  xmlDoc.Parse(xml.c_str());

  if (xmlDoc.Error())
    return false;

  return Load(xmlDoc);
}

void CGUIPythonWindowXML::FreeResources(bool forceUnLoad /*= FALSE */)
{
  // Unload temporary language strings
  ClearScriptStrings();

  CGUIMediaWindow::FreeResources(forceUnLoad);
}

void CGUIPythonWindowXML::Process(unsigned int currentTime, CDirtyRegionList &regions)
{
  g_TextureManager.AddTexturePath(m_mediaDir);
  CGUIMediaWindow::Process(currentTime, regions);
  g_TextureManager.RemoveTexturePath(m_mediaDir);
}

int Py_XBMC_Event_OnClick(void* arg)
{
  if(!arg)
    return 0;

  PyXBMCAction* action = (PyXBMCAction*)arg;
  if (action->pCallbackWindow)
  {
    PyObject *ret = PyObject_CallMethod((PyObject*)action->pCallbackWindow, (char*)"onClick", (char*)"(i)", action->controlId);
    if (ret)
    {
      Py_DECREF(ret);
    }
  }
  delete action;
  return 0;
}

int Py_XBMC_Event_OnFocus(void* arg)
{
  if(!arg)
    return 0;

  PyXBMCAction* action = (PyXBMCAction*)arg;
  if (action->pCallbackWindow)
  {
    PyObject *ret = PyObject_CallMethod((PyObject*)action->pCallbackWindow, (char*)"onFocus", (char*)"(i)", action->controlId);
    if (ret)
    {
      Py_DECREF(ret);
    }
    delete action;
  }
  return 0;
}

int Py_XBMC_Event_OnInit(void* arg)
{
  if(!arg)
    return 0;

  PyXBMCAction* action = (PyXBMCAction*)arg;
  if (action->pCallbackWindow)
  {
    PyObject *ret = PyObject_CallMethod((PyObject*)action->pCallbackWindow, (char*)"onInit", (char*)"()"); //, (char*)"O", &self);
    if (ret)
    {
      Py_XDECREF(ret);
    }
  }
  delete action;
  return 0;
}

void CGUIPythonWindowXML::SetCallbackWindow(void *state, void *object)
{
  pCallbackWindow = object;
  m_threadState   = state;
}

void CGUIPythonWindowXML::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  // maybe on day we can make an easy way to do this context menu
  // with out this method overriding the MediaWindow version, it will display 'Add to Favorites'
}

unsigned int CGUIPythonWindowXML::LoadScriptStrings()
{
  // Path where the language strings reside
  CStdString pathToLanguageFile = m_scriptPath;
  CStdString pathToFallbackLanguageFile = m_scriptPath;
  URIUtils::AddFileToFolder(pathToLanguageFile, "resources", pathToLanguageFile);
  URIUtils::AddFileToFolder(pathToFallbackLanguageFile, "resources", pathToFallbackLanguageFile);
  URIUtils::AddFileToFolder(pathToLanguageFile, "language", pathToLanguageFile);
  URIUtils::AddFileToFolder(pathToFallbackLanguageFile, "language", pathToFallbackLanguageFile);
  URIUtils::AddFileToFolder(pathToLanguageFile, g_guiSettings.GetString("locale.language"), pathToLanguageFile);
  URIUtils::AddFileToFolder(pathToFallbackLanguageFile, "english", pathToFallbackLanguageFile);
  URIUtils::AddFileToFolder(pathToLanguageFile, "strings.xml", pathToLanguageFile);
  URIUtils::AddFileToFolder(pathToFallbackLanguageFile, "strings.xml", pathToFallbackLanguageFile);

  // allocate a bunch of strings
  return g_localizeStrings.LoadBlock(m_scriptPath, pathToLanguageFile, pathToFallbackLanguageFile);
}

void CGUIPythonWindowXML::ClearScriptStrings()
{
  // Unload temporary language strings
  g_localizeStrings.ClearBlock(m_scriptPath);
}


