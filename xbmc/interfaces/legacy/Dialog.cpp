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
#include "LanguageHook.h"

#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogTextViewer.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogNumeric.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "settings/MediaSourceSettings.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "ModuleXbmcgui.h"
#include "guilib/GUIKeyboardFactory.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "WindowException.h"
#include "messaging/ApplicationMessenger.h"
#include "Dialog.h"
#include "ListItem.h"
#ifdef TARGET_POSIX
#include "linux/XTimeUtils.h"
#endif

using namespace KODI::MESSAGING;

#define ACTIVE_WINDOW g_windowManager.GetActiveWindow()

namespace XBMCAddon
{
  namespace xbmcgui
  {
    Dialog::~Dialog() {}

    bool Dialog::yesno(const String& heading, const String& line1, 
                       const String& line2,
                       const String& line3,
                       const String& nolabel,
                       const String& yeslabel,
                       int autoclose)
    {
      DelayedCallGuard dcguard(languageHook);
      CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
      if (pDialog == NULL)
        throw WindowException("Error: Window is NULL, this is not possible :-)");

      // get lines, last 4 lines are optional.
      if (!heading.empty())
        pDialog->SetHeading(CVariant{heading});
      if (!line1.empty())
        pDialog->SetLine(0, CVariant{line1});
      if (!line2.empty())
        pDialog->SetLine(1, CVariant{line2});
      if (!line3.empty())
        pDialog->SetLine(2, CVariant{line3});

      if (!nolabel.empty())
        pDialog->SetChoice(0, CVariant{nolabel});
      if (!yeslabel.empty())
        pDialog->SetChoice(1, CVariant{yeslabel});

      if (autoclose > 0)
        pDialog->SetAutoClose(autoclose);

      pDialog->Open();

      return pDialog->IsConfirmed();
    }

    bool Dialog::info(const ListItem* item)
    {
      const AddonClass::Ref<xbmcgui::ListItem> listitem(item);
      if (listitem->item->HasVideoInfoTag())
      {
        CGUIDialogVideoInfo::ShowFor(*listitem->item);
        return true;
      }
      else if (listitem->item->HasMusicInfoTag())
      {
        CGUIDialogMusicInfo::ShowFor(*listitem->item);
        return true;
      }
      return false;
    }

    int Dialog::contextmenu(const std::vector<String>& list)
    {
      DelayedCallGuard dcguard(languageHook);
      CGUIDialogContextMenu* pDialog= (CGUIDialogContextMenu*)g_windowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
      if (pDialog == NULL)
        throw WindowException("Error: Window is NULL, this is not possible :-)");

      CContextButtons choices;
      for(unsigned int i = 0; i < list.size(); i++)
      {
        choices.Add(i, list[i]);
      }
      return pDialog->Show(choices);
    }


    int Dialog::select(const String& heading, const std::vector<Alternative<String, const ListItem* > > & list, int autoclose, int preselect, bool useDetails)
    {
      DelayedCallGuard dcguard(languageHook);
      CGUIDialogSelect* pDialog= (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
      if (pDialog == NULL)
        throw WindowException("Error: Window is NULL, this is not possible :-)");

      pDialog->Reset();
      if (!heading.empty())
        pDialog->SetHeading(CVariant{heading});
      for (const auto& item : list)
      {
        AddonClass::Ref<ListItem> ritem = item.which() == XBMCAddon::first ? ListItem::fromString(item.former()) : AddonClass::Ref<ListItem>(item.later());
        CFileItemPtr& fileItem = ritem->item;
        pDialog->Add(*fileItem);
      }
      if (preselect > -1)
        pDialog->SetSelected(preselect);
      if (autoclose > 0)
        pDialog->SetAutoClose(autoclose);
      pDialog->SetUseDetails(useDetails);
      pDialog->Open();

      return pDialog->GetSelectedItem();
    }


    std::unique_ptr<std::vector<int>> Dialog::multiselect(const String& heading,
        const std::vector<Alternative<String, const ListItem* > > & options, int autoclose, const std::vector<int>& preselect, bool useDetails)
    {
      DelayedCallGuard dcguard(languageHook);
      CGUIDialogSelect* pDialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
      if (pDialog == nullptr)
        throw WindowException("Error: Window is NULL");

      pDialog->Reset();
      pDialog->SetMultiSelection(true);
      pDialog->SetHeading(CVariant{heading});

      for (const auto& item : options)
      {
        AddonClass::Ref<ListItem> ritem = item.which() == XBMCAddon::first ? ListItem::fromString(item.former()) : AddonClass::Ref<ListItem>(item.later());
        CFileItemPtr& fileItem = ritem->item;
        pDialog->Add(*fileItem);
      }
      if (autoclose > 0)
        pDialog->SetAutoClose(autoclose);
      pDialog->SetUseDetails(useDetails);
      pDialog->SetSelected(preselect);
      pDialog->Open();

      if (pDialog->IsConfirmed())
        return std::unique_ptr<std::vector<int>>(new std::vector<int>(pDialog->GetSelectedItems()));
      else
        return std::unique_ptr<std::vector<int>>();
    }

    bool Dialog::ok(const String& heading, const String& line1, 
                    const String& line2,
                    const String& line3)
    {
      DelayedCallGuard dcguard(languageHook);
      CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
      if (pDialog == NULL)
        throw WindowException("Error: Window is NULL, this is not possible :-)");

      if (!heading.empty())
        pDialog->SetHeading(CVariant{heading});
      if (!line1.empty())
        pDialog->SetLine(0, CVariant{line1});
      if (!line2.empty())
        pDialog->SetLine(1, CVariant{line2});
      if (!line3.empty())
        pDialog->SetLine(2, CVariant{line3});

      pDialog->Open();

      return pDialog->IsConfirmed();
    }

    void Dialog::textviewer(const String& heading, const String& text)
    {
      DelayedCallGuard dcguard(languageHook);
      const int window = WINDOW_DIALOG_TEXT_VIEWER;

      CGUIDialogTextViewer* pDialog = (CGUIDialogTextViewer*)g_windowManager.GetWindow(window);
      if (pDialog == NULL)
        throw WindowException("Error: Window is NULL, this is not possible :-)");
      if (!heading.empty())
        pDialog->SetHeading(heading);
      if (!text.empty())
        pDialog->SetText(text);
      pDialog->Open();
    }


    Alternative<String, std::vector<String> > Dialog::browse(int type, const String& heading, 
                                const String& s_shares, const String& maskparam, bool useThumbs, 
                                bool useFileDirectories, const String& defaultt,
                                bool enableMultiple)
    {
      Alternative<String, std::vector<String> > ret;
      if (enableMultiple)
        ret.later() = browseMultiple(type,heading,s_shares,maskparam,useThumbs,useFileDirectories,defaultt);
      else
        ret.former() = browseSingle(type,heading,s_shares,maskparam,useThumbs,useFileDirectories,defaultt);
      return ret;
    }

    String Dialog::browseSingle(int type, const String& heading, const String& s_shares,
                                const String& maskparam, bool useThumbs, 
                                bool useFileDirectories, 
                                const String& defaultt )
    {
      DelayedCallGuard dcguard(languageHook);
      std::string value;
      std::string mask = maskparam;
      VECSOURCES *shares = CMediaSourceSettings::GetInstance().GetSources(s_shares);
      if (!shares) 
        throw WindowException("Error: GetSources given %s is NULL.",s_shares.c_str());

      if (useFileDirectories && !maskparam.empty())
        mask += "|.rar|.zip";

      value = defaultt;
      if (type == 1)
          CGUIDialogFileBrowser::ShowAndGetFile(*shares, mask, heading, value, useThumbs, useFileDirectories);
      else if (type == 2)
        CGUIDialogFileBrowser::ShowAndGetImage(*shares, heading, value);
      else
        CGUIDialogFileBrowser::ShowAndGetDirectory(*shares, heading, value, type != 0);
      return value;
    }

    std::vector<String> Dialog::browseMultiple(int type, const String& heading, const String& s_shares,
                          const String& mask, bool useThumbs, 
                          bool useFileDirectories, const String& defaultt )
    {
      DelayedCallGuard dcguard(languageHook);
      VECSOURCES *shares = CMediaSourceSettings::GetInstance().GetSources(s_shares);
      std::vector<String> valuelist;
      String lmask = mask;
      if (!shares) 
        throw WindowException("Error: GetSources given %s is NULL.",s_shares.c_str());

      if (useFileDirectories && !lmask.empty())
        lmask += "|.rar|.zip";

      if (type == 1)
        CGUIDialogFileBrowser::ShowAndGetFileList(*shares, lmask, heading, valuelist, useThumbs, useFileDirectories);
      else if (type == 2)
        CGUIDialogFileBrowser::ShowAndGetImageList(*shares, heading, valuelist);
      else
        throw WindowException("Error: Cannot retreive multuple directories using browse %s is NULL.",s_shares.c_str());

      return valuelist;
    }

    String Dialog::numeric(int inputtype, const String& heading, const String& defaultt)
    {
      DelayedCallGuard dcguard(languageHook);
      std::string value;
      SYSTEMTIME timedate;
      GetLocalTime(&timedate);

      if (!heading.empty())
      {
        if (inputtype == 1)
        {
          if (!defaultt.empty() && defaultt.size() == 10)
          {
            std::string sDefault = defaultt;
            timedate.wDay = atoi(sDefault.substr(0, 2).c_str());
            timedate.wMonth = atoi(sDefault.substr(3, 4).c_str());
            timedate.wYear = atoi(sDefault.substr(sDefault.size() - 4).c_str());
          }
          if (CGUIDialogNumeric::ShowAndGetDate(timedate, heading))
            value = StringUtils::Format("%2d/%2d/%4d", timedate.wDay, timedate.wMonth, timedate.wYear);
          else
            return emptyString;
        }
        else if (inputtype == 2)
        {
          if (!defaultt.empty() && defaultt.size() == 5)
          {
            std::string sDefault = defaultt;
            timedate.wHour = atoi(sDefault.substr(0, 2).c_str());
            timedate.wMinute = atoi(sDefault.substr(3, 2).c_str());
          }
          if (CGUIDialogNumeric::ShowAndGetTime(timedate, heading))
            value = StringUtils::Format("%2d:%02d", timedate.wHour, timedate.wMinute);
          else
            return emptyString;
        }
        else if (inputtype == 3)
        {
          value = defaultt;
          if (!CGUIDialogNumeric::ShowAndGetIPAddress(value, heading))
            return emptyString;
        }
        else
        {
          value = defaultt;
          if (!CGUIDialogNumeric::ShowAndGetNumber(value, heading))
            return emptyString;
        }
      }
      return value;
    }

    void Dialog::notification(const String& heading, const String& message, const String& icon, int time, bool sound)
    {
      DelayedCallGuard dcguard(languageHook);

      std::string strIcon = getNOTIFICATION_INFO();
      int iTime = TOAST_DISPLAY_TIME;

      if (time > 0)
        iTime = time;
      if (!icon.empty())
        strIcon = icon;
      
      if (strIcon == getNOTIFICATION_INFO())
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, heading, message, iTime, sound);
      else if (strIcon == getNOTIFICATION_WARNING())
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, heading, message, iTime, sound);
      else if (strIcon == getNOTIFICATION_ERROR())
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, heading, message, iTime, sound);
      else
        CGUIDialogKaiToast::QueueNotification(strIcon, heading, message, iTime, sound);
    }
    
    String Dialog::input(const String& heading, const String& defaultt, int type, int option, int autoclose)
    {
      DelayedCallGuard dcguard(languageHook);
      std::string value(defaultt);
      SYSTEMTIME timedate;
      GetLocalTime(&timedate);

      switch (type)
      {
        case INPUT_ALPHANUM:
          {
            bool bHiddenInput = (option & ALPHANUM_HIDE_INPUT) == ALPHANUM_HIDE_INPUT;
            if (!CGUIKeyboardFactory::ShowAndGetInput(value, CVariant{heading}, true, bHiddenInput, autoclose))
              value = emptyString;
          }
          break;
        case INPUT_NUMERIC:
          {
            if (!CGUIDialogNumeric::ShowAndGetNumber(value, heading, autoclose))
              value = emptyString;
          }
          break;
        case INPUT_DATE:
          {
            if (!defaultt.empty() && defaultt.size() == 10)
            {
              std::string sDefault = defaultt;
              timedate.wDay = atoi(sDefault.substr(0, 2).c_str());
              timedate.wMonth = atoi(sDefault.substr(3, 4).c_str());
              timedate.wYear = atoi(sDefault.substr(sDefault.size() - 4).c_str());
            }
            if (CGUIDialogNumeric::ShowAndGetDate(timedate, heading))
              value = StringUtils::Format("%2d/%2d/%4d", timedate.wDay, timedate.wMonth, timedate.wYear);
            else
              value = emptyString;
          }
          break;
        case INPUT_TIME:
          {
            if (!defaultt.empty() && defaultt.size() == 5)
            {
              std::string sDefault = defaultt;
              timedate.wHour = atoi(sDefault.substr(0, 2).c_str());
              timedate.wMinute = atoi(sDefault.substr(3, 2).c_str());
            }
            if (CGUIDialogNumeric::ShowAndGetTime(timedate, heading))
              value = StringUtils::Format("%2d:%02d", timedate.wHour, timedate.wMinute);
            else
              value = emptyString;
          }
          break;
        case INPUT_IPADDRESS:
          {
            if (!CGUIDialogNumeric::ShowAndGetIPAddress(value, heading))
              value = emptyString;
          }
          break;
        case INPUT_PASSWORD:
          {
            bool bResult = false;

            if (option & PASSWORD_VERIFY)
              bResult = CGUIKeyboardFactory::ShowAndVerifyPassword(value, heading, 0, autoclose) == 0 ? true : false;
            else
              bResult = CGUIKeyboardFactory::ShowAndVerifyNewPassword(value, heading, true, autoclose);

            if (!bResult)
              value = emptyString;
          }
          break;
        default:
          value = emptyString;
          break;
      }

      return value;
    }

    DialogProgress::~DialogProgress() { XBMC_TRACE; deallocating(); }

    void DialogProgress::deallocating()
    {
      XBMC_TRACE;

      if (dlg && open)
      {
        DelayedCallGuard dg;
        dlg->Close();
      }
    }

    void DialogProgress::create(const String& heading, const String& line1, 
                                const String& line2,
                                const String& line3)
    {
      DelayedCallGuard dcguard(languageHook);
      CGUIDialogProgress* pDialog= (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      if (pDialog == NULL)
        throw WindowException("Error: Window is NULL, this is not possible :-)");

      dlg = pDialog;
      open = true;

      pDialog->SetHeading(CVariant{heading});

      if (!line1.empty())
        pDialog->SetLine(0, CVariant{line1});
      if (!line2.empty())
        pDialog->SetLine(1, CVariant{line2});
      if (!line3.empty())
        pDialog->SetLine(2, CVariant{line3});

      pDialog->Open();
    }

    void DialogProgress::update(int percent, const String& line1, 
                                const String& line2,
                                const String& line3)
    {
      DelayedCallGuard dcguard(languageHook);
      CGUIDialogProgress* pDialog = dlg;

      if (pDialog == NULL)
        throw WindowException("Dialog not created.");

      if (percent >= 0 && percent <= 100)
      {
        pDialog->SetPercentage(percent);
        pDialog->ShowProgressBar(true);
      }
      else
      {
        pDialog->ShowProgressBar(false);
      }

      if (!line1.empty())
        pDialog->SetLine(0, CVariant{line1});
      if (!line2.empty())
        pDialog->SetLine(1, CVariant{line2});
      if (!line3.empty())
        pDialog->SetLine(2, CVariant{line3});
    }

    void DialogProgress::close()
    {
      DelayedCallGuard dcguard(languageHook);
      if (dlg == NULL)
        throw WindowException("Dialog not created.");
      dlg->Close();
      open = false;
    }

    bool DialogProgress::iscanceled()
    {
      if (dlg == NULL)
        throw WindowException("Dialog not created.");
      return dlg->IsCanceled();
    }

    DialogProgressBG::~DialogProgressBG() { XBMC_TRACE; deallocating(); }

    void DialogProgressBG::deallocating()
    {
      XBMC_TRACE;

      if (dlg && open)
      {
        DelayedCallGuard dg;
        dlg->Close();
      }
    }

    void DialogProgressBG::create(const String& heading, const String& message)
    {
      DelayedCallGuard dcguard(languageHook);
      CGUIDialogExtendedProgressBar* pDialog = 
          (CGUIDialogExtendedProgressBar*)g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS);

      if (pDialog == NULL)
        throw WindowException("Error: Window is NULL, this is not possible :-)");

      CGUIDialogProgressBarHandle* pHandle = pDialog->GetHandle(heading);

      dlg = pDialog;
      handle = pHandle;
      open = true;

      pHandle->SetTitle(heading);
      if (!message.empty())
        pHandle->SetText(message);
    }

    void DialogProgressBG::update(int percent, const String& heading, const String& message)
    {
      DelayedCallGuard dcguard(languageHook);
      CGUIDialogProgressBarHandle* pHandle = handle;

      if (pHandle == NULL)
        throw WindowException("Dialog not created.");

      if (percent >= 0 && percent <= 100)
        pHandle->SetPercentage((float)percent);
      if (!heading.empty())
        pHandle->SetTitle(heading);
      if (!message.empty())
        pHandle->SetText(message);
    }

    void DialogProgressBG::close()
    {
      DelayedCallGuard dcguard(languageHook);
      if (handle == NULL)
        throw WindowException("Dialog not created.");
      handle->MarkFinished();
      open = false;
    }

    bool DialogProgressBG::isFinished()
    {
      if (handle == NULL)
        throw WindowException("Dialog not created.");
      return handle->IsFinished();
    }

  }
}
