/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WindowXML.h"

#include "FileItemList.h"
#include "ServiceBroker.h"
#include "WindowException.h"
#include "WindowInterceptor.h"
#include "addons/Addon.h"
#include "addons/Skin.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/TextureManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <mutex>

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
     *  add behavior for a few more virtual functions that were unnecessary
     *  in the Window or WindowDialog.
     */
#define checkedb(methcall) ( window.isNotNull() ? xwin-> methcall : false )
#define checkedv(methcall) { if (window.isNotNull()) xwin-> methcall ; }


    //! @todo This should be done with template specialization
    class WindowXMLInterceptor : public InterceptorDialog<CGUIMediaWindow>
    {
      WindowXML* xwin;
    public:
      WindowXMLInterceptor(WindowXML* _window, int windowid,const char* xmlfile) :
        InterceptorDialog<CGUIMediaWindow>("CGUIMediaWindow",_window,windowid,xmlfile), xwin(_window)
      { }

      void AllocResources(bool forceLoad = false) override
      { XBMC_TRACE; if(up()) CGUIMediaWindow::AllocResources(forceLoad); else checkedv(AllocResources(forceLoad)); }
       void FreeResources(bool forceUnLoad = false) override
      { XBMC_TRACE; if(up()) CGUIMediaWindow::FreeResources(forceUnLoad); else checkedv(FreeResources(forceUnLoad)); }
      bool OnClick(int iItem, const std::string &player = "") override { XBMC_TRACE; return up() ? CGUIMediaWindow::OnClick(iItem, player) : checkedb(OnClick(iItem)); }

      void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override
      { XBMC_TRACE; if(up()) CGUIMediaWindow::Process(currentTime,dirtyregions); else checkedv(Process(currentTime,dirtyregions)); }

      // this is a hack to SKIP the CGUIMediaWindow
      bool OnAction(const CAction &action) override
      { XBMC_TRACE; return up() ? CGUIWindow::OnAction(action) : checkedb(OnAction(action)); }

    protected:
      // CGUIWindow
      bool LoadXML(const std::string &strPath, const std::string &strPathLower) override
      { XBMC_TRACE; return up() ? CGUIMediaWindow::LoadXML(strPath,strPathLower) : xwin->LoadXML(strPath,strPathLower); }

      // CGUIMediaWindow
      void GetContextButtons(int itemNumber, CContextButtons &buttons) override
      { XBMC_TRACE; if (up()) CGUIMediaWindow::GetContextButtons(itemNumber,buttons); else xwin->GetContextButtons(itemNumber,buttons); }
      bool Update(const std::string &strPath, bool) override
      { XBMC_TRACE; return up() ? CGUIMediaWindow::Update(strPath) : xwin->Update(strPath); }
      void SetupShares() override { XBMC_TRACE; if(up()) CGUIMediaWindow::SetupShares(); else checkedv(SetupShares()); }

      friend class WindowXML;
      friend class WindowXMLDialog;

    };

    WindowXML::~WindowXML() { XBMC_TRACE; deallocating();  }

    WindowXML::WindowXML(const String& xmlFilename,
                         const String& scriptPath,
                         const String& defaultSkin,
                         const String& defaultRes,
                         bool isMedia) :
      Window(true)
    {
      XBMC_TRACE;
      RESOLUTION_INFO res;
      std::string strSkinPath = g_SkinInfo->GetSkinPath(xmlFilename, &res);
      m_isMedia = isMedia;

      if (!CFileUtils::Exists(strSkinPath))
      {
        std::string str("none");
        ADDON::AddonInfoPtr addonInfo =
            std::make_shared<ADDON::CAddonInfo>(str, ADDON::AddonType::SKIN);
        ADDON::CSkinInfo::TranslateResolution(defaultRes, res);

        // Check for the matching folder for the skin in the fallback skins folder
        std::string fallbackPath = URIUtils::AddFileToFolder(scriptPath, "resources", "skins");
        std::string basePath = URIUtils::AddFileToFolder(fallbackPath, g_SkinInfo->ID());

        strSkinPath = g_SkinInfo->GetSkinPath(xmlFilename, &res, basePath);

        // Check for the matching folder for the skin in the fallback skins folder (if it exists)
        if (CFileUtils::Exists(basePath))
        {
          addonInfo->SetPath(basePath);
          std::shared_ptr<ADDON::CSkinInfo> skinInfo = std::make_shared<ADDON::CSkinInfo>(addonInfo, res);
          skinInfo->Start();
          strSkinPath = skinInfo->GetSkinPath(xmlFilename, &res);
        }

        if (!CFileUtils::Exists(strSkinPath))
        {
          // Finally fallback to the DefaultSkin as it didn't exist in either the XBMC Skin folder or the fallback skin folder
          addonInfo->SetPath(URIUtils::AddFileToFolder(fallbackPath, defaultSkin));
          std::shared_ptr<ADDON::CSkinInfo> skinInfo = std::make_shared<ADDON::CSkinInfo>(addonInfo, res);

          skinInfo->Start();
          strSkinPath = skinInfo->GetSkinPath(xmlFilename, &res);
          if (!CFileUtils::Exists(strSkinPath))
            throw WindowException("XML File for Window is missing");
        }
      }

      m_scriptPath = scriptPath;
//      sXMLFileName = strSkinPath;

      interceptor = new WindowXMLInterceptor(this, lockingGetNextAvailableWindowId(),strSkinPath.c_str());
      setWindow(interceptor);
      interceptor->SetCoordsRes(res);
    }

    int WindowXML::lockingGetNextAvailableWindowId()
    {
      XBMC_TRACE;
      std::unique_lock<CCriticalSection> lock(CServiceBroker::GetWinSystem()->GetGfxContext());
      return getNextAvailableWindowId();
    }

    void WindowXML::addItem(const Alternative<String, const ListItem*>& item, int position)
    {
      XBMC_TRACE;
      // item could be deleted if the reference count is 0.
      //   so I MAY need to check prior to using a Ref just in
      //   case this object is managed by Python. I'm not sure
      //   though.
      AddonClass::Ref<ListItem> ritem = item.which() == XBMCAddon::first ? ListItem::fromString(item.former()) : AddonClass::Ref<ListItem>(item.later());

      // Tells the window to add the item to FileItem vector
      {
        XBMCAddonUtils::GuiLock lock(languageHook, false);

        //----------------------------------------------------
        // Former AddItem call
        //AddItem(ritem->item, pos);
        {
          CFileItemPtr& fileItem = ritem->item;
          if (position == INT_MAX || position > A(m_vecItems)->Size())
          {
            A(m_vecItems)->Add(fileItem);
          }
          else if (position <  -1 &&  !(position*-1 < A(m_vecItems)->Size()))
          {
            A(m_vecItems)->AddFront(fileItem,0);
          }
          else
          {
            A(m_vecItems)->AddFront(fileItem,position);
          }
          A(m_viewControl).SetItems(*(A(m_vecItems)));
        }
        //----------------------------------------------------
      }
    }

    void WindowXML::addItems(const std::vector<Alternative<String, const XBMCAddon::xbmcgui::ListItem* > > & items)
    {
    XBMC_TRACE;
    XBMCAddonUtils::GuiLock lock(languageHook, false);
    for (auto item : items)
      {
        AddonClass::Ref<ListItem> ritem = item.which() == XBMCAddon::first ? ListItem::fromString(item.former()) : AddonClass::Ref<ListItem>(item.later());
        CFileItemPtr& fileItem = ritem->item;
        A(m_vecItems)->Add(fileItem);
      }
      A(m_viewControl).SetItems(*(A(m_vecItems)));
    }


    void WindowXML::removeItem(int position)
    {
      XBMC_TRACE;
      // Tells the window to remove the item at the specified position from the FileItem vector
      XBMCAddonUtils::GuiLock lock(languageHook, false);
      A(m_vecItems)->Remove(position);
      A(m_viewControl).SetItems(*(A(m_vecItems)));
    }

    int WindowXML::getCurrentListPosition()
    {
      XBMC_TRACE;
      XBMCAddonUtils::GuiLock lock(languageHook, false);
      int listPos = A(m_viewControl).GetSelectedItem();
      return listPos;
    }

    void WindowXML::setCurrentListPosition(int position)
    {
      XBMC_TRACE;
      XBMCAddonUtils::GuiLock lock(languageHook, false);
      A(m_viewControl).SetSelectedItem(position);
    }

    ListItem* WindowXML::getListItem(int position)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, false);
      //CFileItemPtr fi = pwx->GetListItem(listPos);
      CFileItemPtr fi;
      {
        if (position < 0 || position >= A(m_vecItems)->Size())
          return new ListItem();
        fi = A(m_vecItems)->Get(position);
      }

      if (fi == NULL)
      {
        throw WindowException("Index out of range (%i)",position);
      }

      ListItem* sListItem = new ListItem();
      sListItem->item = fi;

      // let's hope someone reference counts this.
      return sListItem;
    }

    int WindowXML::getListSize()
    {
      XBMC_TRACE;
      return A(m_vecItems)->Size();
    }

    void WindowXML::clearList()
    {
      XBMC_TRACE;
      XBMCAddonUtils::GuiLock lock(languageHook, false);
      A(ClearFileItems());

      A(m_viewControl).SetItems(*(A(m_vecItems)));
    }

    void WindowXML::setContainerProperty(const String& key, const String& value)
    {
      XBMC_TRACE;
      A(m_vecItems)->SetProperty(key, value);
    }

    void WindowXML::setContent(const String& value)
    {
      XBMC_TRACE;
      XBMCAddonUtils::GuiLock lock(languageHook, false);
      A(m_vecItems)->SetContent(value);
    }

    int WindowXML::getCurrentContainerId()
    {
      XBMC_TRACE;
      XBMCAddonUtils::GuiLock lock(languageHook, false);
      return A(m_viewControl.GetCurrentControl());
    }

    bool WindowXML::OnAction(const CAction &action)
    {
      XBMC_TRACE;
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
#ifdef ENABLE_XBMC_TRACE_API
      XBMC_TRACE;
      CLog::Log(LOGDEBUG, "{}Message id:{}", _tg.getSpaces(), (int)message.GetMessage());
#endif

      //! @todo We shouldn't be dropping down to CGUIWindow in any of this ideally.
      //!       We have to make up our minds about what python should be doing and
      //!       what this side of things should be doing
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
              A(m_viewControl).GetCurrentControl() != message.GetControlId())
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

      case GUI_MSG_NOTIFY_ALL:
        // most messages from GUI_MSG_NOTIFY_ALL break container content, whitelist working ones.
        if (message.GetParam1() == GUI_MSG_PAGE_CHANGE || message.GetParam1() == GUI_MSG_WINDOW_RESIZE)
          return A(CGUIMediaWindow::OnMessage(message));
        return true;

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

          if(iControl && iControl != interceptor->GetID()) // pCallbackWindow &&  != this->GetID())
          {
            CGUIControl* controlClicked = interceptor->GetControl(iControl);

            // The old python way used to check list AND SELECITEM method
            //   or if its a button, radiobutton.
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
              else if (controlClicked->IsContainer() && message.GetParam1() == ACTION_MOUSE_DOUBLE_CLICK)
              {
                invokeCallback(new CallbackFunction<WindowXML,int>(this,&WindowXML::onDoubleClick,iControl));
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
              // the core context menu can lead to all sort of issues right now when used with WindowXMLs, so lets intercept the corresponding message
              else if (controlClicked->IsContainer() && message.GetParam1() == ACTION_CONTEXT_MENU)
                return true;
            }
          }
        }
        break;
      }

      return A(CGUIMediaWindow::OnMessage(message));
    }

    void WindowXML::AllocResources(bool forceLoad /*= false */)
    {
      XBMC_TRACE;
      std::string tmpDir = URIUtils::GetDirectory(ref(window)->GetProperty("xmlfile").asString());
      std::string fallbackMediaPath;
      URIUtils::GetParentPath(tmpDir, fallbackMediaPath);
      URIUtils::RemoveSlashAtEnd(fallbackMediaPath);
      m_mediaDir = fallbackMediaPath;

      //CLog::Log(LOGDEBUG, "CGUIPythonWindowXML::AllocResources called: {}", fallbackMediaPath);
      CServiceBroker::GetGUI()->GetTextureManager().AddTexturePath(m_mediaDir);
      ref(window)->AllocResources(forceLoad);
      CServiceBroker::GetGUI()->GetTextureManager().RemoveTexturePath(m_mediaDir);
    }

    void WindowXML::FreeResources(bool forceUnLoad /*= false */)
    {
      XBMC_TRACE;

      ref(window)->FreeResources(forceUnLoad);
    }

    void WindowXML::Process(unsigned int currentTime, CDirtyRegionList &regions)
    {
      XBMC_TRACE;
      CServiceBroker::GetGUI()->GetTextureManager().AddTexturePath(m_mediaDir);
      ref(window)->Process(currentTime, regions);
      CServiceBroker::GetGUI()->GetTextureManager().RemoveTexturePath(m_mediaDir);
    }

    bool WindowXML::OnClick(int iItem)
    {
      XBMC_TRACE;
      // Hook Over calling  CGUIMediaWindow::OnClick(iItem) results in it trying to PLAY the file item
      // which if its not media is BAD and 99 out of 100 times undesirable.
      return false;
    }

    bool WindowXML::OnDoubleClick(int iItem)
    {
      XBMC_TRACE;
      return false;
    }

    void WindowXML::GetContextButtons(int itemNumber, CContextButtons &buttons)
    {
      XBMC_TRACE;
      // maybe on day we can make an easy way to do this context menu
      // with out this method overriding the MediaWindow version, it will display 'Add to Favorites'
    }

    bool WindowXML::LoadXML(const String &strPath, const String &strLowerPath)
    {
      XBMC_TRACE;
      return A(CGUIWindow::LoadXML(strPath, strLowerPath));
    }

    void WindowXML::SetupShares()
    {
      XBMC_TRACE;
    }

    bool WindowXML::Update(const String &strPath)
    {
      XBMC_TRACE;
      return true;
    }

    WindowXMLDialog::WindowXMLDialog(const String& xmlFilename, const String& scriptPath,
                                     const String& defaultSkin,
                                     const String& defaultRes) :
      WindowXML(xmlFilename, scriptPath, defaultSkin, defaultRes),
      WindowDialogMixin(this)
    { XBMC_TRACE; }

    WindowXMLDialog::~WindowXMLDialog() { XBMC_TRACE; deallocating(); }

    bool WindowXMLDialog::OnMessage(CGUIMessage &message)
    {
      XBMC_TRACE;
      if (message.GetMessage() == GUI_MSG_WINDOW_DEINIT)
        return A(CGUIWindow::OnMessage(message));

      return WindowXML::OnMessage(message);
    }

    bool WindowXMLDialog::OnAction(const CAction &action)
    {
      XBMC_TRACE;
      return WindowDialogMixin::OnAction(action) ? true : WindowXML::OnAction(action);
    }

    void WindowXMLDialog::OnDeinitWindow(int nextWindowID)
    {
      XBMC_TRACE;
      CServiceBroker::GetGUI()->GetWindowManager().RemoveDialog(interceptor->GetID());
      WindowXML::OnDeinitWindow(nextWindowID);
    }

    bool WindowXMLDialog::LoadXML(const String &strPath, const String &strLowerPath)
    {
      XBMC_TRACE;
      if (WindowXML::LoadXML(strPath, strLowerPath))
      {
        // Set the render order to the dialog's default in case it's not specified in the skin xml
        // because this dialog is mapped to CGUIMediaWindow instead of CGUIDialog.
        // This must be done here, because the render order will be reset before loading the skin xml.
        if (ref(window)->GetRenderOrder() == RENDER_ORDER_WINDOW)
          window->SetRenderOrder(RENDER_ORDER_DIALOG);
        return true;
      }
      return false;
    }

  }

}
