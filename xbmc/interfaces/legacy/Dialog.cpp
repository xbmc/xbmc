 /*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "Dialog.h"

#include "FileItemList.h"
#include "LanguageHook.h"
#include "ListItem.h"
#include "ModuleXbmcgui.h"
#include "ServiceBroker.h"
#include "WindowException.h"
#include "dialogs/GUIDialogColorPicker.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogTextViewer.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "settings/MediaSourceSettings.h"
#include "storage/MediaManager.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"
#include "video/dialogs/GUIDialogVideoInfo.h"

#include <memory>

 using namespace KODI::MESSAGING;

#define ACTIVE_WINDOW CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow()

namespace XBMCAddon
{
  namespace xbmcgui
  {
    Dialog::~Dialog() = default;

    bool Dialog::yesno(const String& heading,
                       const String& message,
                       const String& nolabel,
                       const String& yeslabel,
                       int autoclose,
                       int defaultbutton)
    {
      return yesNoCustomInternal(heading, message, nolabel, yeslabel, emptyString, autoclose,
                                 defaultbutton) == 1;
    }

    int Dialog::yesnocustom(const String& heading,
                            const String& message,
                            const String& customlabel,
                            const String& nolabel,
                            const String& yeslabel,
                            int autoclose,
                            int defaultbutton)
    {
      return yesNoCustomInternal(heading, message, nolabel, yeslabel, customlabel, autoclose,
                                 defaultbutton);
    }

    int Dialog::yesNoCustomInternal(const String& heading,
                                    const String& message,
                                    const String& nolabel,
                                    const String& yeslabel,
                                    const String& customlabel,
                                    int autoclose,
                                    int defaultbutton)
    {
      DelayedCallGuard dcguard(languageHook);
      CGUIDialogYesNo* pDialog =
          CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogYesNo>(
              WINDOW_DIALOG_YES_NO);
      if (pDialog == nullptr)
        throw WindowException("Error: Window is null");

      return pDialog->ShowAndGetInput(CVariant{heading}, CVariant{message}, CVariant{nolabel},
                                      CVariant{yeslabel}, CVariant{customlabel}, autoclose,
                                      defaultbutton);
    }

    bool Dialog::info(const ListItem* item)
    {
      DelayedCallGuard dcguard(languageHook);
      const AddonClass::Ref<xbmcgui::ListItem> listitem(item);
      if (listitem->item->HasVideoInfoTag())
      {
        CGUIDialogVideoInfo::ShowFor(*listitem->item);
        return true;
      }
      else if (listitem->item->HasMusicInfoTag())
      {
        CGUIDialogMusicInfo::ShowFor(listitem->item.get());
        return true;
      }
      return false;
    }

    int Dialog::contextmenu(const std::vector<String>& list)
    {
      DelayedCallGuard dcguard(languageHook);
      CGUIDialogContextMenu* pDialog= CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogContextMenu>(WINDOW_DIALOG_CONTEXT_MENU);
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
      CGUIDialogSelect* pDialog= CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
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
      CGUIDialogSelect* pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
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
        return std::make_unique<std::vector<int>>(pDialog->GetSelectedItems());
      else
        return std::unique_ptr<std::vector<int>>();
    }

    bool Dialog::ok(const String& heading, const String& message)
    {
      DelayedCallGuard dcguard(languageHook);
      return HELPERS::ShowOKDialogText(CVariant{heading}, CVariant{message});
    }

    void Dialog::textviewer(const String& heading, const String& text, bool usemono)
    {
      DelayedCallGuard dcguard(languageHook);

      CGUIDialogTextViewer* pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogTextViewer>(WINDOW_DIALOG_TEXT_VIEWER);
      if (pDialog == NULL)
        throw WindowException("Error: Window is NULL, this is not possible :-)");
      if (!heading.empty())
        pDialog->SetHeading(heading);
      if (!text.empty())
        pDialog->SetText(text);
      pDialog->UseMonoFont(usemono);
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

      VECSOURCES localShares;
      if (!shares)
      {
        CServiceBroker::GetMediaManager().GetLocalDrives(localShares);
        if (StringUtils::CompareNoCase(s_shares, "local") != 0)
          CServiceBroker::GetMediaManager().GetNetworkLocations(localShares);
      }
      else // always append local drives
      {
        localShares = *shares;
        CServiceBroker::GetMediaManager().GetLocalDrives(localShares);
      }

      if (useFileDirectories && !maskparam.empty())
        mask += "|.rar|.zip";

      value = defaultt;
      if (type == 1)
          CGUIDialogFileBrowser::ShowAndGetFile(localShares, mask, heading, value, useThumbs, useFileDirectories);
      else if (type == 2)
        CGUIDialogFileBrowser::ShowAndGetImage(localShares, heading, value);
      else
        CGUIDialogFileBrowser::ShowAndGetDirectory(localShares, heading, value, type != 0);
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

      VECSOURCES localShares;
      if (!shares)
      {
        CServiceBroker::GetMediaManager().GetLocalDrives(localShares);
        if (StringUtils::CompareNoCase(s_shares, "local") != 0)
          CServiceBroker::GetMediaManager().GetNetworkLocations(localShares);
      }
      else // always append local drives
      {
        localShares = *shares;
        CServiceBroker::GetMediaManager().GetLocalDrives(localShares);
      }

      if (useFileDirectories && !lmask.empty())
        lmask += "|.rar|.zip";

      if (type == 1)
        CGUIDialogFileBrowser::ShowAndGetFileList(localShares, lmask, heading, valuelist, useThumbs, useFileDirectories);
      else if (type == 2)
        CGUIDialogFileBrowser::ShowAndGetImageList(localShares, heading, valuelist);
      else
        throw WindowException("Error: Cannot retrieve multiple directories using browse %s is NULL.",s_shares.c_str());

      return valuelist;
    }

    String Dialog::numeric(int inputtype, const String& heading, const String& defaultt, bool bHiddenInput)
    {
      DelayedCallGuard dcguard(languageHook);
      std::string value;
      KODI::TIME::SystemTime timedate;
      KODI::TIME::GetLocalTime(&timedate);

      if (!heading.empty())
      {
        if (inputtype == 1)
        {
          if (!defaultt.empty() && defaultt.size() == 10)
          {
            const std::string& sDefault = defaultt;
            timedate.day = atoi(sDefault.substr(0, 2).c_str());
            timedate.month = atoi(sDefault.substr(3, 4).c_str());
            timedate.year = atoi(sDefault.substr(sDefault.size() - 4).c_str());
          }
          if (CGUIDialogNumeric::ShowAndGetDate(timedate, heading))
            value =
                StringUtils::Format("{:2}/{:2}/{:4}", timedate.day, timedate.month, timedate.year);
          else
            return emptyString;
        }
        else if (inputtype == 2)
        {
          if (!defaultt.empty() && defaultt.size() == 5)
          {
            const std::string& sDefault = defaultt;
            timedate.hour = atoi(sDefault.substr(0, 2).c_str());
            timedate.minute = atoi(sDefault.substr(3, 2).c_str());
          }
          if (CGUIDialogNumeric::ShowAndGetTime(timedate, heading))
            value = StringUtils::Format("{:2}:{:02}", timedate.hour, timedate.minute);
          else
            return emptyString;
        }
        else if (inputtype == 3)
        {
          value = defaultt;
          if (!CGUIDialogNumeric::ShowAndGetIPAddress(value, heading))
            return emptyString;
        }
        else if (inputtype == 4)
        {
          value = defaultt;
          if (!CGUIDialogNumeric::ShowAndVerifyNewPassword(value))
            return emptyString;
        }
        else
        {
          value = defaultt;
          if (!CGUIDialogNumeric::ShowAndGetNumber(value, heading, 0, bHiddenInput))
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
      KODI::TIME::SystemTime timedate;
      KODI::TIME::GetLocalTime(&timedate);

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
              const std::string& sDefault = defaultt;
              timedate.day = atoi(sDefault.substr(0, 2).c_str());
              timedate.month = atoi(sDefault.substr(3, 4).c_str());
              timedate.year = atoi(sDefault.substr(sDefault.size() - 4).c_str());
            }
            if (CGUIDialogNumeric::ShowAndGetDate(timedate, heading))
              value = StringUtils::Format("{:2}/{:2}/{:4}", timedate.day, timedate.month,
                                          timedate.year);
            else
              value = emptyString;
          }
          break;
        case INPUT_TIME:
          {
            if (!defaultt.empty() && defaultt.size() == 5)
            {
              const std::string& sDefault = defaultt;
              timedate.hour = atoi(sDefault.substr(0, 2).c_str());
              timedate.minute = atoi(sDefault.substr(3, 2).c_str());
            }
            if (CGUIDialogNumeric::ShowAndGetTime(timedate, heading))
              value = StringUtils::Format("{:2}:{:02}", timedate.hour, timedate.minute);
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

    String Dialog::colorpicker(const String& heading,
                               const String& selectedcolor,
                               const String& colorfile,
                               const std::vector<const ListItem*>& colorlist)
    {
      DelayedCallGuard dcguard(languageHook);
      std::string value = emptyString;
      CGUIDialogColorPicker* pDialog =
          CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogColorPicker>(
              WINDOW_DIALOG_COLOR_PICKER);
      if (pDialog == nullptr)
        throw WindowException("Error: Window is NULL, this is not possible :-)");

      pDialog->Reset();
      if (!heading.empty())
        pDialog->SetHeading(CVariant{heading});

      if (!colorlist.empty())
      {
        CFileItemList items;
        for (const auto& coloritem : colorlist)
        {
          items.Add(coloritem->item);
        }
        pDialog->SetItems(items);
      }
      else if (!colorfile.empty())
        pDialog->LoadColors(colorfile);
      else
        pDialog->LoadColors();

      if (!selectedcolor.empty())
        pDialog->SetSelectedColor(selectedcolor);

      pDialog->Open();

      if (pDialog->IsConfirmed())
        value = pDialog->GetSelectedColor();
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

    void DialogProgress::create(const String& heading, const String& message)
    {
      DelayedCallGuard dcguard(languageHook);
      CGUIDialogProgress* pDialog= CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);

      if (pDialog == NULL)
        throw WindowException("Error: Window is NULL, this is not possible :-)");

      dlg = pDialog;
      open = true;

      pDialog->SetHeading(CVariant{heading});

      if (!message.empty())
        pDialog->SetText(CVariant{message});

      pDialog->Open();
    }

    void DialogProgress::update(int percent, const String& message)
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

      if (!message.empty())
        pDialog->SetText(CVariant{message});
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
          CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogExtendedProgressBar>(WINDOW_DIALOG_EXT_PROGRESS);

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
