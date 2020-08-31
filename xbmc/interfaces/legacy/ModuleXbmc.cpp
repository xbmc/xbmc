/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

//! @todo Need a uniform way of returning an error status

#include "ModuleXbmc.h"

#include "AddonUtils.h"
#include "Application.h"
#include "FileItem.h"
#include "GUIInfoManager.h"
#include "LangInfo.h"
#include "LanguageHook.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "aojsonrpc.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h" //! @todo remove me when dropping translatePath from this file
#include "guilib/GUIAudioManager.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/TextureManager.h"
#include "input/WindowTranslator.h"
#include "messaging/ApplicationMessenger.h"
#include "network/Network.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "threads/SystemClock.h"
#include "utils/Crc32.h"
#include "utils/FileExtensionProvider.h"
#include "utils/LangCodeExpander.h"
#include "utils/MemUtils.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include <vector>

using namespace KODI;
using namespace KODI::MESSAGING;

namespace XBMCAddon
{

  namespace xbmc
  {
    /*****************************************************************
     * start of xbmc methods
     *****************************************************************/
    void log(const char* msg, int level)
    {
      // check for a valid loglevel
      if (level < LOGDEBUG || level > LOGNONE)
        level = LOGDEBUG;
      CLog::Log(level, "%s", msg);
    }

    void shutdown()
    {
      XBMC_TRACE;
      CApplicationMessenger::GetInstance().PostMsg(TMSG_SHUTDOWN);
    }

    void restart()
    {
      XBMC_TRACE;
      CApplicationMessenger::GetInstance().PostMsg(TMSG_RESTART);
    }

    void executescript(const char* script)
    {
      XBMC_TRACE;
      if (! script)
        return;

      CApplicationMessenger::GetInstance().PostMsg(TMSG_EXECUTE_SCRIPT, -1, -1, nullptr, script);
    }

    void executebuiltin(const char* function, bool wait /* = false*/)
    {
      XBMC_TRACE;
      if (! function)
        return;

      // builtins is no anarchy
      // enforce some rules here
      // DialogBusy must not be activated, it is modal dialog
      std::string execute;
      std::vector<std::string> params;
      CUtil::SplitExecFunction(function, execute, params);
      StringUtils::ToLower(execute);
      if (StringUtils::EqualsNoCase(execute, "activatewindow") ||
          StringUtils::EqualsNoCase(execute, "closedialog"))
      {
        int win = CWindowTranslator::TranslateWindow(params[0]);
        if (win == WINDOW_DIALOG_BUSY)
        {
          CLog::Log(LOGWARNING, "addons must not activate DialogBusy");
          return;
        }
      }

      if (wait)
        CApplicationMessenger::GetInstance().SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, function);
      else
        CApplicationMessenger::GetInstance().PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, function);
    }

    String executeJSONRPC(const char* jsonrpccommand)
    {
      XBMC_TRACE;
      DelayedCallGuard dg;
      String ret;

      if (! jsonrpccommand)
        return ret;

      //    String method = jsonrpccommand;

      CAddOnTransport transport;
      CAddOnTransport::CAddOnClient client;

      return JSONRPC::CJSONRPC::MethodCall(/*method*/ jsonrpccommand, &transport, &client);
    }

    void sleep(long timemillis)
    {
      XBMC_TRACE;

      XbmcThreads::EndTime endTime(timemillis);
      while (!endTime.IsTimePast())
      {
        LanguageHook* lh = NULL;
        {
          DelayedCallGuard dcguard;
          lh = dcguard.getLanguageHook(); // borrow this
          long nextSleep = endTime.MillisLeft();
          if (nextSleep > 100)
            nextSleep = 100; // only sleep for 100 millis
          KODI::TIME::Sleep(nextSleep);
        }
        if (lh != NULL)
          lh->MakePendingCalls();
      }
    }

    String getLocalizedString(int id)
    {
      XBMC_TRACE;
      String label;
      if (id >= 30000 && id <= 30999)
        label = g_localizeStringsTemp.Get(id);
      else if (id >= 32000 && id <= 32999)
        label = g_localizeStringsTemp.Get(id);
      else
        label = g_localizeStrings.Get(id);

      return label;
    }

    String getSkinDir()
    {
      XBMC_TRACE;
      return CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOOKANDFEEL_SKIN);
    }

    String getLanguage(int format /* = CLangCodeExpander::ENGLISH_NAME */, bool region /*= false*/)
    {
      XBMC_TRACE;
      std::string lang = g_langInfo.GetEnglishLanguageName();

      switch (format)
      {
      case CLangCodeExpander::ENGLISH_NAME:
        {
          if (region)
          {
            std::string region = "-" + g_langInfo.GetCurrentRegion();
            return (lang += region);
          }
          return lang;
        }
      case CLangCodeExpander::ISO_639_1:
        {
          std::string langCode;
          g_LangCodeExpander.ConvertToISO6391(lang, langCode);
          if (region)
          {
            std::string region = g_langInfo.GetRegionLocale();
            std::string region2Code;
            g_LangCodeExpander.ConvertToISO6391(region, region2Code);
            region2Code = "-" + region2Code;
            return (langCode += region2Code);
          }
          return langCode;
        }
      case CLangCodeExpander::ISO_639_2:
        {
          std::string langCode;
          g_LangCodeExpander.ConvertToISO6392B(lang, langCode);
          if (region)
          {
            std::string region = g_langInfo.GetRegionLocale();
            std::string region3Code;
            g_LangCodeExpander.ConvertToISO6392B(region, region3Code);
            region3Code = "-" + region3Code;
            return (langCode += region3Code);
          }

          return langCode;
        }
      default:
        return "";
      }
    }

    String getIPAddress()
    {
      XBMC_TRACE;
      char cTitleIP[32];
      sprintf(cTitleIP, "127.0.0.1");
      CNetworkInterface* iface = CServiceBroker::GetNetwork().GetFirstConnectedInterface();
      if (iface)
        return iface->GetCurrentIPAddress();

      return cTitleIP;
    }

    long getDVDState()
    {
      XBMC_TRACE;
      return CServiceBroker::GetMediaManager().GetDriveStatus();
    }

    long getFreeMem()
    {
      XBMC_TRACE;
      KODI::MEMORY::MemoryStatus stat;
      KODI::MEMORY::GetMemoryStatus(&stat);
      return static_cast<long>(stat.availPhys  / ( 1024 * 1024 ));
    }

    // getCpuTemp() method
    // ## Doesn't work right, use getInfoLabel('System.CPUTemperature') instead.
    /*PyDoc_STRVAR(getCpuTemp__doc__,
      "getCpuTemp() -- Returns the current cpu temperature as an integer."
      ""
      "example:"
      "  - cputemp = xbmc.getCpuTemp()");

      PyObject* XBMC_GetCpuTemp(PyObject *self, PyObject *args)
      {
      unsigned short cputemp;
      unsigned short cpudec;

      _outp(0xc004, (0x4c<<1)|0x01);
      _outp(0xc008, 0x01);
      _outpw(0xc000, _inpw(0xc000));
      _outp(0xc002, (0) ? 0x0b : 0x0a);
      while ((_inp(0xc000) & 8));
      cputemp = _inpw(0xc006);

      _outp(0xc004, (0x4c<<1)|0x01);
      _outp(0xc008, 0x10);
      _outpw(0xc000, _inpw(0xc000));
      _outp(0xc002, (0) ? 0x0b : 0x0a);
      while ((_inp(0xc000) & 8));
      cpudec = _inpw(0xc006);

      if (cpudec<10) cpudec = cpudec * 100;
      if (cpudec<100) cpudec = cpudec *10;

      return PyInt_FromLong((long)(cputemp + cpudec / 1000.0f));
      }*/

    String getInfoLabel(const char* cLine)
    {
      XBMC_TRACE;
      if (!cLine)
      {
        String ret;
        return ret;
      }

      CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
      int ret = infoMgr.TranslateString(cLine);
      //doesn't seem to be a single InfoTag?
      //try full blown GuiInfoLabel then
      if (ret == 0)
        return GUILIB::GUIINFO::CGUIInfoLabel::GetLabel(cLine);
      else
        return infoMgr.GetLabel(ret);
    }

    String getInfoImage(const char * infotag)
    {
      XBMC_TRACE;
      if (!infotag)
        {
          String ret;
          return ret;
        }

      CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
      int ret = infoMgr.TranslateString(infotag);
      return infoMgr.GetImage(ret, WINDOW_INVALID);
    }

    void playSFX(const char* filename, bool useCached)
    {
      XBMC_TRACE;
      if (!filename)
        return;

      CGUIComponent* gui = CServiceBroker::GetGUI();
      if (XFILE::CFile::Exists(filename) && gui)
      {
        gui->GetAudioManager().PlayPythonSound(filename,useCached);
      }
    }

    void stopSFX()
    {
      XBMC_TRACE;
      DelayedCallGuard dg;
      CGUIComponent* gui = CServiceBroker::GetGUI();
      if (gui)
        gui->GetAudioManager().Stop();
    }

    void enableNavSounds(bool yesNo)
    {
      XBMC_TRACE;
      CGUIComponent* gui = CServiceBroker::GetGUI();
      if (gui)
        gui->GetAudioManager().Enable(yesNo);
    }

    bool getCondVisibility(const char *condition)
    {
      XBMC_TRACE;
      if (!condition)
        return false;

      bool ret;
      {
        XBMCAddonUtils::GuiLock lock(nullptr, false);

        int id = CServiceBroker::GetGUI()->GetWindowManager().GetTopmostModalDialog();
        if (id == WINDOW_INVALID)
          id = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
        ret = CServiceBroker::GetGUI()->GetInfoManager().EvaluateBool(condition,id);
      }

      return ret;
    }

    int getGlobalIdleTime()
    {
      XBMC_TRACE;
      return g_application.GlobalIdleTime();
    }

    String getCacheThumbName(const String& path)
    {
      XBMC_TRACE;
      auto crc = Crc32::ComputeFromLowerCase(path);
      return StringUtils::Format("%08x.tbn", crc);
    }

    String translatePath(const String& path)
    {
      XBMC_TRACE;
      CLog::Log(LOGWARNING, "xbmc.translatePath is deprecated and might be removed in future kodi "
                            "versions. Please use xbmcvfs.translatePath instead.");
      return CSpecialProtocol::TranslatePath(path);
    }

    Tuple<String,String> getCleanMovieTitle(const String& path, bool usefoldername)
    {
      XBMC_TRACE;
      CFileItem item(path, false);
      std::string strName = item.GetMovieName(usefoldername);

      std::string strTitleAndYear;
      std::string strTitle;
      std::string strYear;
      CUtil::CleanString(strName, strTitle, strTitleAndYear, strYear, usefoldername);
      return Tuple<String,String>(strTitle,strYear);
    }

    String getRegion(const char* id)
    {
      XBMC_TRACE;
      std::string result;

      if (StringUtils::CompareNoCase(id, "datelong") == 0)
      {
        result = g_langInfo.GetDateFormat(true);
        StringUtils::Replace(result, "DDDD", "%A");
        StringUtils::Replace(result, "MMMM", "%B");
        StringUtils::Replace(result, "D", "%d");
        StringUtils::Replace(result, "YYYY", "%Y");
        }
        else if (StringUtils::CompareNoCase(id, "dateshort") == 0)
        {
          result = g_langInfo.GetDateFormat(false);
          StringUtils::Replace(result, "MM", "%m");
          StringUtils::Replace(result, "DD", "%d");
#ifdef TARGET_WINDOWS
          StringUtils::Replace(result, "M", "%#m");
          StringUtils::Replace(result, "D", "%#d");
#else
          StringUtils::Replace(result, "M", "%-m");
          StringUtils::Replace(result, "D", "%-d");
#endif
          StringUtils::Replace(result, "YYYY", "%Y");
        }
        else if (StringUtils::CompareNoCase(id, "tempunit") == 0)
          result = g_langInfo.GetTemperatureUnitString();
        else if (StringUtils::CompareNoCase(id, "speedunit") == 0)
          result = g_langInfo.GetSpeedUnitString();
        else if (StringUtils::CompareNoCase(id, "time") == 0)
        {
          result = g_langInfo.GetTimeFormat();
          if (StringUtils::StartsWith(result, "HH"))
            StringUtils::Replace(result, "HH", "%H");
          else
            StringUtils::Replace(result, "H", "%H");
          StringUtils::Replace(result, "h", "%I");
          StringUtils::Replace(result, "mm", "%M");
          StringUtils::Replace(result, "ss", "%S");
          StringUtils::Replace(result, "xx", "%p");
        }
        else if (StringUtils::CompareNoCase(id, "meridiem") == 0)
          result =
              StringUtils::Format("%s/%s", g_langInfo.GetMeridiemSymbol(MeridiemSymbolAM).c_str(),
                                  g_langInfo.GetMeridiemSymbol(MeridiemSymbolPM).c_str());

        return result;
    }

    //! @todo Add a mediaType enum
    String getSupportedMedia(const char* mediaType)
    {
      XBMC_TRACE;
      String result;
      if (StringUtils::CompareNoCase(mediaType, "video") == 0)
        result = CServiceBroker::GetFileExtensionProvider().GetVideoExtensions();
      else if (StringUtils::CompareNoCase(mediaType, "music") == 0)
        result = CServiceBroker::GetFileExtensionProvider().GetMusicExtensions();
      else if (StringUtils::CompareNoCase(mediaType, "picture") == 0)
        result = CServiceBroker::GetFileExtensionProvider().GetPictureExtensions();

      //! @todo implement
      //    else
      //      return an error

      return result;
    }

    bool skinHasImage(const char* image)
    {
      XBMC_TRACE;
      return CServiceBroker::GetGUI()->GetTextureManager().HasTexture(image);
    }


    bool startServer(int iTyp, bool bStart, bool bWait)
    {
      XBMC_TRACE;
      DelayedCallGuard dg;
      return g_application.StartServer((CApplication::ESERVERS)iTyp, bStart != 0, bWait != 0);
    }

    void audioSuspend()
    {
      IAE *ae = CServiceBroker::GetActiveAE();
      if (ae)
        ae->Suspend();
    }

    void audioResume()
    {
      IAE *ae = CServiceBroker::GetActiveAE();
      if (ae)
        ae->Resume();
    }

    String convertLanguage(const char* language, int format)
    {
      std::string convertedLanguage;
      switch (format)
      {
      case CLangCodeExpander::ENGLISH_NAME:
        {
          g_LangCodeExpander.Lookup(language, convertedLanguage);
          // maybe it's a check whether the language exists or not
          if (convertedLanguage.empty())
          {
            g_LangCodeExpander.ConvertToISO6392B(language, convertedLanguage);
            g_LangCodeExpander.Lookup(convertedLanguage, convertedLanguage);
          }
          break;
        }
      case CLangCodeExpander::ISO_639_1:
        g_LangCodeExpander.ConvertToISO6391(language, convertedLanguage);
        break;
      case CLangCodeExpander::ISO_639_2:
        g_LangCodeExpander.ConvertToISO6392B(language, convertedLanguage);
        break;
      default:
        return "";
      }
      return convertedLanguage;
    }

    String getUserAgent()
    {
      return CSysInfo::GetUserAgent();
    }

    int getSERVER_WEBSERVER() { return CApplication::ES_WEBSERVER; }
    int getSERVER_AIRPLAYSERVER() { return CApplication::ES_AIRPLAYSERVER; }
    int getSERVER_UPNPSERVER() { return CApplication::ES_UPNPSERVER; }
    int getSERVER_UPNPRENDERER() { return CApplication::ES_UPNPRENDERER; }
    int getSERVER_EVENTSERVER() { return CApplication::ES_EVENTSERVER; }
    int getSERVER_JSONRPCSERVER() { return CApplication::ES_JSONRPCSERVER; }
    int getSERVER_ZEROCONF() { return CApplication::ES_ZEROCONF; }

    int getPLAYLIST_MUSIC() { return PLAYLIST_MUSIC; }
    int getPLAYLIST_VIDEO() { return PLAYLIST_VIDEO; }
    int getTRAY_OPEN() { return TRAY_OPEN; }
    int getDRIVE_NOT_READY() { return DRIVE_NOT_READY; }
    int getTRAY_CLOSED_NO_MEDIA() { return TRAY_CLOSED_NO_MEDIA; }
    int getTRAY_CLOSED_MEDIA_PRESENT() { return TRAY_CLOSED_MEDIA_PRESENT; }
    int getLOGDEBUG() { return LOGDEBUG; }
    int getLOGINFO() { return LOGINFO; }
    int getLOGNOTICE() { return LOGNOTICE; }
    int getLOGWARNING() { return LOGWARNING; }
    int getLOGERROR() { return LOGERROR; }
    int getLOGSEVERE() { return LOGSEVERE; }
    int getLOGFATAL() { return LOGFATAL; }
    int getLOGNONE() { return LOGNONE; }

    // language string formats
    int getISO_639_1() { return CLangCodeExpander::ISO_639_1; }
    int getISO_639_2(){ return CLangCodeExpander::ISO_639_2; }
    int getENGLISH_NAME() { return CLangCodeExpander::ENGLISH_NAME; }

    const int lLOGDEBUG = LOGDEBUG;
  }
}
