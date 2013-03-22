
#include "Dialog.h"
#include "LanguageHook.h"

#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogNumeric.h"
#include "settings/Settings.h"

#define ACTIVE_WINDOW g_windowManager.GetActiveWindow()

namespace XBMCAddon
{
  namespace xbmcgui
  {

    static void XBMCWaitForThreadMessage(int message, int param1, int param2)
    {
      ThreadMessage tMsg = {(DWORD)message, (DWORD)param1, (DWORD)param2};
      CApplicationMessenger::Get().SendMessage(tMsg, true);
    }

    Dialog::~Dialog() {}

    bool Dialog::yesno(const String& heading, const String& line1, 
                       const String& line2,
                       const String& line3,
                       const String& nolabel,
                       const String& yeslabel) throw (WindowException)
    {
      DelayedCallGuard dcguard(languageHook);
      const int window = WINDOW_DIALOG_YES_NO;
      CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(window);
      if (pDialog == NULL)
        throw WindowException("Error: Window is NULL, this is not possible :-)");

      // get lines, last 4 lines are optional.
      if (!heading.empty())
        pDialog->SetHeading(heading);
      if (!line1.empty())
        pDialog->SetLine(0, line1);
      if (!line2.empty())
        pDialog->SetLine(1, line2);
      if (!line3.empty())
        pDialog->SetLine(2, line3);

      if (!nolabel.empty())
        pDialog->SetChoice(0,nolabel);
      if (!yeslabel.empty())
        pDialog->SetChoice(1,yeslabel);

      //send message and wait for user input
      XBMCWaitForThreadMessage(TMSG_DIALOG_DOMODAL, window, ACTIVE_WINDOW);

      return pDialog->IsConfirmed();
    }

    int Dialog::select(const String& heading, const std::vector<String>& list, int autoclose) throw (WindowException)
    {
      DelayedCallGuard dcguard(languageHook);
      const int window = WINDOW_DIALOG_SELECT;
      CGUIDialogSelect* pDialog= (CGUIDialogSelect*)g_windowManager.GetWindow(window);
      if (pDialog == NULL)
        throw WindowException("Error: Window is NULL, this is not possible :-)");

      pDialog->Reset();
      if (!heading.empty())
        pDialog->SetHeading(heading);

      String listLine;
      for(unsigned int i = 0; i < list.size(); i++)
      {
        listLine = list[i];
          pDialog->Add(listLine);
      }
      if (autoclose > 0)
        pDialog->SetAutoClose(autoclose);

      //send message and wait for user input
      XBMCWaitForThreadMessage(TMSG_DIALOG_DOMODAL, window, ACTIVE_WINDOW);

      return pDialog->GetSelectedLabel();
    }

    bool Dialog::ok(const String& heading, const String& line1, 
                    const String& line2,
                    const String& line3) throw (WindowException)
    {
      DelayedCallGuard dcguard(languageHook);
      const int window = WINDOW_DIALOG_OK;

      CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(window);
      if (pDialog == NULL)
        throw WindowException("Error: Window is NULL, this is not possible :-)");

      if (!heading.empty())
        pDialog->SetHeading(heading);
      if (!line1.empty())
        pDialog->SetLine(0, line1);
      if (!line2.empty())
        pDialog->SetLine(1, line2);
      if (!line3.empty())
        pDialog->SetLine(2, line3);

      //send message and wait for user input
      XBMCWaitForThreadMessage(TMSG_DIALOG_DOMODAL, window, ACTIVE_WINDOW);

      return pDialog->IsConfirmed();
    }

    Alternative<String, std::vector<String> > Dialog::browse(int type, const String& heading, 
                                const String& s_shares, const String& maskparam, bool useThumbs, 
                                bool useFileDirectories, const String& defaultt,
                                bool enableMultiple) throw (WindowException)
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
                                const String& defaultt ) throw (WindowException)
    {
      DelayedCallGuard dcguard(languageHook);
      CStdString value;
      std::string mask = maskparam;
      VECSOURCES *shares = g_settings.GetSourcesFromType(s_shares);
      if (!shares) 
        throw WindowException("Error: GetSourcesFromType given %s is NULL.",s_shares.c_str());

      if (useFileDirectories && (!maskparam.empty() && !maskparam.size() == 0))
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
                          bool useFileDirectories, const String& defaultt ) throw (WindowException)
    {
      DelayedCallGuard dcguard(languageHook);
      VECSOURCES *shares = g_settings.GetSourcesFromType(s_shares);
      CStdStringArray tmpret;
      String lmask = mask;
      if (!shares) 
        throw WindowException("Error: GetSourcesFromType given %s is NULL.",s_shares.c_str());

      if (useFileDirectories && (!lmask.empty() && !(lmask.size() == 0)))
        lmask += "|.rar|.zip";

      if (type == 1)
        CGUIDialogFileBrowser::ShowAndGetFileList(*shares, lmask, heading, tmpret, useThumbs, useFileDirectories);
      else if (type == 2)
        CGUIDialogFileBrowser::ShowAndGetImageList(*shares, heading, tmpret);
      else
        throw WindowException("Error: Cannot retreive multuple directories using browse %s is NULL.",s_shares.c_str());

      std::vector<String> valuelist;
      int index = 0;
      for (CStdStringArray::iterator iter = tmpret.begin(); iter != tmpret.end(); iter++)
        valuelist[index++] = (*iter);

      return valuelist;
    }

    String Dialog::numeric(int inputtype, const String& heading, const String& defaultt)
    {
      DelayedCallGuard dcguard(languageHook);
      CStdString value;
      SYSTEMTIME timedate;
      GetLocalTime(&timedate);

      if (!heading.empty())
      {
        if (inputtype == 1)
        {
          if (!defaultt.empty() && defaultt.size() == 10)
          {
            CStdString sDefault = defaultt;
            timedate.wDay = atoi(sDefault.Left(2));
            timedate.wMonth = atoi(sDefault.Mid(3,4));
            timedate.wYear = atoi(sDefault.Right(4));
          }
          if (CGUIDialogNumeric::ShowAndGetDate(timedate, heading))
            value.Format("%2d/%2d/%4d", timedate.wDay, timedate.wMonth, timedate.wYear);
          else
            return emptyString;
        }
        else if (inputtype == 2)
        {
          if (!defaultt.empty() && defaultt.size() == 5)
          {
            CStdString sDefault = defaultt;
            timedate.wHour = atoi(sDefault.Left(2));
            timedate.wMinute = atoi(sDefault.Right(2));
          }
          if (CGUIDialogNumeric::ShowAndGetTime(timedate, heading))
            value.Format("%2d:%02d", timedate.wHour, timedate.wMinute);
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

    DialogProgress::~DialogProgress() { TRACE; deallocating(); }

    void DialogProgress::deallocating()
    {
      TRACE;

      if (dlg)
      {
        DelayedCallGuard dg;
        dlg->Close();
      }
    }

    void DialogProgress::create(const String& heading, const String& line1, 
                                const String& line2,
                                const String& line3) throw (WindowException)
    {
      DelayedCallGuard dcguard(languageHook);
      CGUIDialogProgress* pDialog= (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      if (pDialog == NULL)
        throw WindowException("Error: Window is NULL, this is not possible :-)");

      dlg = pDialog;

      pDialog->SetHeading(heading);

      if (!line1.empty())
        pDialog->SetLine(0, line1);
      if (!line2.empty())
        pDialog->SetLine(1, line2);
      if (!line3.empty())
        pDialog->SetLine(2, line3);

      pDialog->StartModal();
    }

    void DialogProgress::update(int percent, const String& line1, 
                                const String& line2,
                                const String& line3) throw (WindowException)
    {
      DelayedCallGuard dcguard(languageHook);
      CGUIDialogProgress* pDialog= dlg;

      if (pDialog == NULL)
        throw WindowException("Error: Window is NULL, this is not possible :-)");

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
        pDialog->SetLine(0, line1);
      if (!line2.empty())
        pDialog->SetLine(1, line2);
      if (!line3.empty())
        pDialog->SetLine(2, line3);
    }

    void DialogProgress::close()
    {
      DelayedCallGuard dcguard(languageHook);
      dlg->Close();
    }

    bool DialogProgress::iscanceled()
    {
      return dlg->IsCanceled();
    }
  }
}

