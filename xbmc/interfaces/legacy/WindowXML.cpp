 /*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "WindowXML.h"

#include "WindowInterceptor.h"
#include "guilib/GUIWindowManager.h"
#include "settings/GUISettings.h"
#include "addons/Skin.h"
#include "filesystem/File.h"
#include "utils/URIUtils.h"
#include "addons/Addon.h"

// These #defs are for WindowXML
#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY       3
#define CONTROL_BTNSORTASC      4
#define CONTROL_LABELFILES      12

#define A(x) interceptor->x

namespace XBMCAddon
{
  namespace xbmcgui
  {
    template class Interceptor<CGUIMediaWindow>;

    /**
     * This class extends the Interceptor<CGUIMediaWindow> in order to 
     *  add behavior for a few more virtual functions that were unneccessary
     *  in the Window or WindowDialog.
     */
#define checkedb(methcall) ( window.isNotNull() ? xwin-> methcall : false )
#define checkedv(methcall) { if (window.isNotNull()) xwin-> methcall ; }


    // TODO: This should be done with template specialization
    class WindowXMLInterceptor : public InterceptorDialog<CGUIMediaWindow>
    {
      WindowXML* xwin;
    public:
      WindowXMLInterceptor(WindowXML* _window, int windowid,const char* xmlfile) :
        InterceptorDialog<CGUIMediaWindow>("CGUIMediaWindow",_window,windowid,xmlfile), xwin(_window) 
      { }

      virtual void AllocResources(bool forceLoad = false)
      { TRACE; if(up()) CGUIMediaWindow::AllocResources(forceLoad); else checkedv(AllocResources(forceLoad)); }
      virtual  void FreeResources(bool forceUnLoad = false)
      { TRACE; if(up()) CGUIMediaWindow::FreeResources(forceUnLoad); else checkedv(FreeResources(forceUnLoad)); }
      virtual bool OnClick(int iItem) { TRACE; return up() ? CGUIMediaWindow::OnClick(iItem) : checkedb(OnClick(iItem)); }

      virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
      { TRACE; if(up()) CGUIMediaWindow::Process(currentTime,dirtyregions); else checkedv(Process(currentTime,dirtyregions)); }

      // this is a hack to SKIP the CGUIMediaWindow
      virtual bool OnAction(const CAction &action) 
      { TRACE; return up() ? CGUIWindow::OnAction(action) : checkedb(OnAction(action)); }

    protected:
      // CGUIWindow
      virtual bool LoadXML(const CStdString &strPath, const CStdString &strPathLower)
      { TRACE; return up() ? CGUIMediaWindow::LoadXML(strPath,strPathLower) : xwin->LoadXML(strPath,strPathLower); }

      // CGUIMediaWindow
      virtual void GetContextButtons(int itemNumber, CContextButtons &buttons)
      { TRACE; if (up()) CGUIMediaWindow::GetContextButtons(itemNumber,buttons); else xwin->GetContextButtons(itemNumber,buttons); }
      virtual bool Update(const CStdString &strPath)
      { TRACE; return up() ? CGUIMediaWindow::Update(strPath) : xwin->Update(strPath); }
      virtual void SetupShares() { TRACE; if(up()) CGUIMediaWindow::SetupShares(); else checkedv(SetupShares()); }

      friend class WindowXML;
      friend class WindowXMLDialog;

    };

    WindowXML::WindowXML(const String& xmlFilename,
                         const String& scriptPath,
                         const String& defaultSkin,
                         const String& defaultRes) throw(WindowException) :
      Window("WindowXML")
    {
      initialize(xmlFilename,scriptPath,defaultSkin,defaultRes);
    }

    WindowXML::WindowXML(const char* classname, 
                         const String& xmlFilename,
                         const String& scriptPath,
                         const String& defaultSkin,
                         const String& defaultRes) throw(WindowException) :
      Window(classname)
    {
      TRACE;
      initialize(xmlFilename,scriptPath,defaultSkin,defaultRes);
    }

    WindowXML::~WindowXML() { TRACE; deallocating();  }

    void WindowXML::initialize(const String& xmlFilename,
                         const String& scriptPath,
                         const String& defaultSkin,
                         const String& defaultRes)
    {
      TRACE;
      RESOLUTION_INFO res;
      CStdString strSkinPath = g_SkinInfo->GetSkinPath(xmlFilename, &res);

      if (!XFILE::CFile::Exists(strSkinPath))
      {
        CStdString str("none");
        ADDON::AddonProps props(str, ADDON::ADDON_SKIN, "", "");
        ADDON::CSkinInfo::TranslateResolution(defaultRes, res);

        // Check for the matching folder for the skin in the fallback skins folder
        CStdString fallbackPath = URIUtils::AddFileToFolder(scriptPath, "resources");
        fallbackPath = URIUtils::AddFileToFolder(fallbackPath, "skins");
        CStdString basePath = URIUtils::AddFileToFolder(fallbackPath, g_SkinInfo->ID());

        strSkinPath = g_SkinInfo->GetSkinPath(xmlFilename, &res, basePath);

        // Check for the matching folder for the skin in the fallback skins folder (if it exists)
        if (XFILE::CFile::Exists(basePath))
        {
          props.path = basePath;
          ADDON::CSkinInfo skinInfo(props, res);
          skinInfo.Start();
          strSkinPath = skinInfo.GetSkinPath(xmlFilename, &res);
        }

        if (!XFILE::CFile::Exists(strSkinPath))
        {
          // Finally fallback to the DefaultSkin as it didn't exist in either the XBMC Skin folder or the fallback skin folder
          props.path = URIUtils::AddFileToFolder(fallbackPath, defaultSkin);
          ADDON::CSkinInfo skinInfo(props, res);

          skinInfo.Start();
          strSkinPath = skinInfo.GetSkinPath(xmlFilename, &res);
          if (!XFILE::CFile::Exists(strSkinPath))
            throw WindowException("XML File for Window is missing");
        }
      }

      m_scriptPath = scriptPath;
//      sXMLFileName = strSkinPath;

      interceptor = new WindowXMLInterceptor(this, lockingGetNextAvailalbeWindowId(),strSkinPath.c_str());
      setWindow(interceptor);
      interceptor->SetCoordsRes(res);
    }

    int WindowXML::lockingGetNextAvailalbeWindowId() throw (WindowException)
    {
      TRACE;
      CSingleLock lock(g_graphicsContext);
      return getNextAvailalbeWindowId();
    }

    void WindowXML::addItem(const String& item, int pos)
    {
      TRACE;
      AddonClass::Ref<ListItem> ritem(ListItem::fromString(item));
      addListItem(ritem.get(),pos);
    }

    void WindowXML::addListItem(ListItem* item, int pos)
    {
      TRACE;
      // item could be deleted if the reference count is 0.
      //   so I MAY need to check prior to using a Ref just in
      //   case this object is managed by Python. I'm not sure
      //   though.
      AddonClass::Ref<ListItem> ritem(item);

      // Tells the window to add the item to FileItem vector
      {
        LOCKGUI;

        //----------------------------------------------------
        // Former AddItem call
        //AddItem(ritem->item, pos);
        {
          CFileItemPtr& fileItem = ritem->item;
          if (pos == INT_MAX || pos > A(m_vecItems)->Size())
          {
            A(m_vecItems)->Add(fileItem);
          }
          else if (pos <  -1 &&  !(pos*-1 < A(m_vecItems)->Size()))
          {
            A(m_vecItems)->AddFront(fileItem,0);
          }
          else
          {
            A(m_vecItems)->AddFront(fileItem,pos);
          }
          A(m_viewControl).SetItems(*(A(m_vecItems)));
          A(UpdateButtons());
        }
        //----------------------------------------------------
      }
    }

    void WindowXML::removeItem(int position)
    {
      TRACE;
      // Tells the window to remove the item at the specified position from the FileItem vector
      LOCKGUI;
      A(m_vecItems)->Remove(position);
      A(m_viewControl).SetItems(*(A(m_vecItems)));
      A(UpdateButtons());
    }

    int WindowXML::getCurrentListPosition()
    {
      TRACE;
      LOCKGUI;
      int listPos = A(m_viewControl).GetSelectedItem();
      return listPos;
    }

    void WindowXML::setCurrentListPosition(int position)
    {
      TRACE;
      LOCKGUI;
      A(m_viewControl).SetSelectedItem(position);
    }

    ListItem* WindowXML::getListItem(int position) throw (WindowException)
    {
      LOCKGUI;
      //CFileItemPtr fi = pwx->GetListItem(listPos);
      CFileItemPtr fi;
      {
        if (position < 0 || position >= A(m_vecItems)->Size()) 
          return new ListItem();
        fi = A(m_vecItems)->Get(position);
      }

      if (fi == NULL)
      {
        XBMCAddonUtils::guiUnlock();
        throw WindowException("Index out of range (%i)",position);
      }

      ListItem* sListItem = new ListItem();
      sListItem->item = fi;

      // let's hope someone reference counts this.
      return sListItem;
    }

    int WindowXML::getListSize()
    {
      TRACE;
      return A(m_vecItems)->Size();
    }

    void WindowXML::clearList()
    {
      TRACE;
      A(ClearFileItems());

      A(m_viewControl).SetItems(*(A(m_vecItems)));
      A(UpdateButtons());
    }

    void WindowXML::setProperty(const String& key, const String& value)
    {
      TRACE;
      A(m_vecItems)->SetProperty(key, value);
    }

    bool WindowXML::OnAction(const CAction &action)
    {
      TRACE;
      // do the base class window first, and the call to python after this
      bool ret = ref(window)->OnAction(action);  // we don't currently want the mediawindow actions here
                                                 //  look at the WindowXMLInterceptor onAction, it skips
                                                 //  the CGUIMediaWindow::OnAction and calls directly to
                                                 //  CGUIWindow::OnAction
      AddonClass::Ref<Action> inf(new Action(action));
      invokeCallback(new CallbackFunction<WindowXML,AddonClass::Ref<Action> >(this,&WindowXML::onAction,inf.get()));
      PulseActionEvent();
      return ret;
    }

    bool WindowXML::OnMessage(CGUIMessage& message)
    {
#ifdef ENABLE_TRACE_API
      TRACE;
      CLog::Log(LOGDEBUG,"%sMessage id:%d",_tg.getSpaces(),(int)message.GetMessage());
#endif

      // TODO: We shouldn't be dropping down to CGUIWindow in any of this ideally.
      //       We have to make up our minds about what python should be doing and
      //       what this side of things should be doing
      switch (message.GetMessage())
      {
      case GUI_MSG_WINDOW_DEINIT:
        {
          return ref(window)->OnMessage(message);
        }
        break;

      case GUI_MSG_WINDOW_INIT:
        {
          ref(window)->OnMessage(message);
          invokeCallback(new CallbackFunction<WindowXML>(this,&WindowXML::onInit));
          PulseActionEvent();
          return true;
        }
        break;

      case GUI_MSG_FOCUSED:
        {
          if (A(m_viewControl).HasControl(message.GetControlId()) && 
              A(m_viewControl).GetCurrentControl() != (int)message.GetControlId())
          {
            A(m_viewControl).SetFocused();
            return true;
          }
          // check if our focused control is one of our category buttons
          int iControl=message.GetControlId();

          invokeCallback(new CallbackFunction<WindowXML,int>(this,&WindowXML::onFocus,iControl));
          PulseActionEvent();
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

          if(iControl && iControl != (int)interceptor->GetID()) // pCallbackWindow &&  != this->GetID())
          {
            CGUIControl* controlClicked = (CGUIControl*)interceptor->GetControl(iControl);

            // The old python way used to check list AND SELECITEM method 
            //   or if its a button, checkmark.
            // Its done this way for now to allow other controls without a 
            //  python version like togglebutton to still raise a onAction event
            if (controlClicked) // Will get problems if we the id is not on the window 
                                //   and we try to do GetControlType on it. So check to make sure it exists
            {
              if ((controlClicked->IsContainer() && (message.GetParam1() == ACTION_SELECT_ITEM || message.GetParam1() == ACTION_MOUSE_LEFT_CLICK)) || !controlClicked->IsContainer())
              {
                invokeCallback(new CallbackFunction<WindowXML,int>(this,&WindowXML::onClick,iControl));
                PulseActionEvent();
                return true;
              }
              else if (controlClicked->IsContainer() && message.GetParam1() == ACTION_MOUSE_RIGHT_CLICK)
              {
                AddonClass::Ref<Action> inf(new Action(CAction(ACTION_CONTEXT_MENU)));
                invokeCallback(new CallbackFunction<WindowXML,AddonClass::Ref<Action> >(this,&WindowXML::onAction,inf.get()));
                PulseActionEvent();
                return true;
              }
            }
          }
        }
        break;
      }

      return A(CGUIMediaWindow::OnMessage(message));
    }

    void WindowXML::AllocResources(bool forceLoad /*= FALSE */)
    {
      TRACE;
      CStdString tmpDir;
      URIUtils::GetDirectory(ref(window)->GetProperty("xmlfile").asString(), tmpDir);
      CStdString fallbackMediaPath;
      URIUtils::GetParentPath(tmpDir, fallbackMediaPath);
      URIUtils::RemoveSlashAtEnd(fallbackMediaPath);
      m_mediaDir = fallbackMediaPath;

      //CLog::Log(LOGDEBUG, "CGUIPythonWindowXML::AllocResources called: %s", fallbackMediaPath.c_str());
      g_TextureManager.AddTexturePath(m_mediaDir);
      ref(window)->AllocResources(forceLoad);
      g_TextureManager.RemoveTexturePath(m_mediaDir);
    }

    void WindowXML::FreeResources(bool forceUnLoad /*= FALSE */)
    {
      TRACE;
      // Unload temporary language strings
      ClearScriptStrings();

      ref(window)->FreeResources(forceUnLoad);
    }

    void WindowXML::Process(unsigned int currentTime, CDirtyRegionList &regions)
    {
      TRACE;
      g_TextureManager.AddTexturePath(m_mediaDir);
      ref(window)->Process(currentTime, regions);
      g_TextureManager.RemoveTexturePath(m_mediaDir);
    }

    bool WindowXML::OnClick(int iItem) 
    {
      TRACE;
      // Hook Over calling  CGUIMediaWindow::OnClick(iItem) results in it trying to PLAY the file item
      // which if its not media is BAD and 99 out of 100 times undesireable.
      return false;
    }

    void WindowXML::GetContextButtons(int itemNumber, CContextButtons &buttons)
    {
      TRACE;
      // maybe on day we can make an easy way to do this context menu
      // with out this method overriding the MediaWindow version, it will display 'Add to Favorites'
    }

    bool WindowXML::LoadXML(const String &strPath, const String &strLowerPath)
    {
      TRACE;
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

      CXBMCTinyXML xmlDoc;
      xmlDoc.Parse(xml.c_str());

      if (xmlDoc.Error())
        return false;

      return interceptor->Load(xmlDoc.RootElement());
    }

    unsigned int WindowXML::LoadScriptStrings()
    {
      TRACE;
      // Path where the language strings reside
      CStdString pathToLanguageFile = m_scriptPath;
      URIUtils::AddFileToFolder(pathToLanguageFile, "resources", pathToLanguageFile);
      URIUtils::AddFileToFolder(pathToLanguageFile, "language", pathToLanguageFile);
      URIUtils::AddSlashAtEnd(pathToLanguageFile);

      // allocate a bunch of strings
      return g_localizeStrings.LoadBlock(m_scriptPath, pathToLanguageFile, g_guiSettings.GetString("locale.language"));
    }

    void WindowXML::ClearScriptStrings()
    {
      TRACE;
      // Unload temporary language strings
      g_localizeStrings.ClearBlock(m_scriptPath);
    }

    void WindowXML::SetupShares()
    {
      TRACE;
      A(UpdateButtons());
    }

    bool WindowXML::Update(const String &strPath)
    {
      TRACE;
      return true;
    }

    WindowXMLDialog::WindowXMLDialog(const String& xmlFilename, const String& scriptPath,
                                     const String& defaultSkin,
                                     const String& defaultRes) throw(WindowException) :
      WindowXML("WindowXMLDialog",xmlFilename, scriptPath, defaultSkin, defaultRes),
      WindowDialogMixin(this)
    { TRACE; }

    WindowXMLDialog::~WindowXMLDialog() { TRACE; deallocating(); }

    bool WindowXMLDialog::OnMessage(CGUIMessage &message)
    {
      TRACE;
      if (message.GetMessage() == GUI_MSG_WINDOW_DEINIT)
      {
        CGUIWindow *pWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
        if (pWindow)
          g_windowManager.ShowOverlay(pWindow->GetOverlayState());
        return A(CGUIWindow::OnMessage(message));
      }
      return WindowXML::OnMessage(message);
    }

    bool WindowXMLDialog::OnAction(const CAction &action)
    {
      TRACE;
      return WindowDialogMixin::OnAction(action) ? true : WindowXML::OnAction(action);
    }
    
    void WindowXMLDialog::OnDeinitWindow(int nextWindowID)
    {
      TRACE;
      g_windowManager.RemoveDialog(interceptor->GetID());
      WindowXML::OnDeinitWindow(nextWindowID);
    }
  
  }

}
