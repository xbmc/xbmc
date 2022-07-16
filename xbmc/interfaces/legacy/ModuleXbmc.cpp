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
#include "guilib/GUIAudioManager.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/TextureManager.h"
#include "input/WindowTranslator.h"
#include "messaging/ApplicationMessenger.h"
#include "network/Network.h"
#include "network/NetworkServices.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "storage/discs/IDiscDriveHandler.h"
#include "threads/SystemClock.h"
#include "utils/Crc32.h"
#include "utils/FileExtensionProvider.h"
#include "utils/LangCodeExpander.h"
#include "utils/MemUtils.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/Unicode.h"
#include "utils/UnicodeUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include <vector>

using namespace KODI;

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
      CLog::Log(level, "{}", msg);
    }

    void shutdown()
    {
      XBMC_TRACE;
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_SHUTDOWN);
    }

    void restart()
    {
      XBMC_TRACE;
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_RESTART);
    }

    void executescript(const char* script)
    {
      XBMC_TRACE;
      if (! script)
        return;

      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_EXECUTE_SCRIPT, -1, -1, nullptr, script);
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
      if (UnicodeUtils::EqualsNoCase(execute, "activatewindow") ||
          UnicodeUtils::EqualsNoCase(execute, "closedialog"))
      {
        int win = CWindowTranslator::TranslateWindow(params[0]);
        if (win == WINDOW_DIALOG_BUSY)
        {
          CLog::Log(LOGWARNING, "addons must not activate DialogBusy");
          return;
        }
      }

      if (wait)
        CServiceBroker::GetAppMessenger()->SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr,
                                                   function);
      else
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr,
                                                   function);
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

      XbmcThreads::EndTime<> endTime{std::chrono::milliseconds(timemillis)};
      while (!endTime.IsTimePast())
      {
        LanguageHook* lh = NULL;
        {
          DelayedCallGuard dcguard;
          lh = dcguard.getLanguageHook(); // borrow this
          long nextSleep = endTime.GetTimeLeft().count();
          if (nextSleep > 100)
            nextSleep = 100; // only sleep for 100 millis
          KODI::TIME::Sleep(std::chrono::milliseconds(nextSleep));
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

std::vector<Tuple<String, String>> getICULanguage(std::vector<String>& propertyNames)
{
  // I would have much preferred to return a Dictionary, but I'm not 
  // yet skilled enough to extend swig Dictionary for output
  
  XBMC_TRACE;
  std::vector<Tuple<String, String>> result = std::vector<Tuple<String, String>>();
  icu::Locale currentLocale = Unicode::GetDefaultICULocale();
  for (auto propertyName : propertyNames)
  {
    String value;
    if (propertyName == xbmc::LOCALE_LANGUAGE_ISO_639_1)
    {
      value = std::string(currentLocale.getLanguage());
    }
    else if (propertyName == xbmc::LOCALE_LANGUAGE_ISO_639_2)
    {
      value = std::string(currentLocale.getISO3Language());
    }
    else if (propertyName == xbmc::LOCALE_LANGUAGE_ISO_639_2)
    {
      value = std::string(currentLocale.getScript());
    }
    else if (propertyName == xbmc::LOCALE_COUNTRY_ISO_3166_1_ALPHA_2)
    {
      value = std::string(currentLocale.getCountry());
    }
    else if (propertyName == xbmc::LOCALE_COUNTRY_ISO_3166_1_ALPHA_3)
    {
      value = std::string(currentLocale.getISO3Country());
    }
    else if (propertyName == xbmc::LOCALE_SCRIPT_ISO_15924)
    {
      value = std::string(currentLocale.getScript());
    }
    else if (propertyName == xbmc::LOCALE_PROGRAMATIC_NAME)
    {
      value = std::string(currentLocale.getName());
    }
    else if (propertyName == xbmc::LOCALE_SHORT_PROGRAMATIC_NAME)
    {
      value = std::string(currentLocale.getBaseName());
    }
    else if (propertyName == xbmc::LOCALE_VARIANT)
    {
      value = std::string(currentLocale.getVariant());
    }
    else if (propertyName == xbmc::LOCALE_WINDOWS_LCID)
    {
      value = std::to_string(currentLocale.getLCID());
    }
    else if (propertyName == xbmc::LOCALE_TRANSLATED_LANGUAGE_NAME)
    {
      icu::UnicodeString lang = icu::UnicodeString();
      currentLocale.getDisplayLanguage(lang);
      value = String();
      lang.toUTF8String(value);
    }
    else if (propertyName == xbmc::LOCALE_ENGLISH_LANGUAGE_NAME)
    {
      icu::UnicodeString lang = icu::UnicodeString();
      currentLocale.getDisplayLanguage(icu::Locale::getEnglish(), lang);
      value = String();
      lang.toUTF8String(value);
    }
    else if (propertyName == xbmc::LOCALE_TRANSLATED_COUNTRY)
    {
      icu::UnicodeString country = icu::UnicodeString();
      currentLocale.getDisplayCountry(country);
      value = String();
      country.toUTF8String(value);
    }
    else if (propertyName == xbmc::LOCALE_ENGLISH_COUNTRY)
    {
      icu::UnicodeString country = icu::UnicodeString();
      currentLocale.getDisplayCountry(icu::Locale::getEnglish(), country);
      value = String();
      country.toUTF8String(value);
    }
    else if (propertyName == xbmc::LOCALE_TRANSLATED_VARIANT)
    {
      icu::UnicodeString variant = icu::UnicodeString();
      currentLocale.getDisplayVariant(variant);
      value = String();
      variant.toUTF8String(value);
    }
    else if (propertyName == xbmc::LOCALE_ENGLISH_VARIANT)
    {
      icu::UnicodeString variant = icu::UnicodeString();
      currentLocale.getDisplayVariant(icu::Locale::getEnglish(),variant);
      value = String();
      variant.toUTF8String(value);
    }
    else if (propertyName == xbmc::LOCALE_TRANSLATED_NAME)
    {
      icu::UnicodeString name = icu::UnicodeString();
      currentLocale.getDisplayName(name);
      value = String();
      name.toUTF8String(value);

    }
    else if (propertyName == xbmc::LOCALE_ENGLISH_NAME)
    {
      icu::UnicodeString name = icu::UnicodeString();
      currentLocale.getDisplayCountry(icu::Locale::getEnglish(), name);
      value = String();
      name.toUTF8String(value);
    }

    Tuple<String, String> entry = Tuple<String, String>(propertyName, value);
    result.push_back(entry);
  }
  return result;
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
      case CLangCodeExpander::ISO_639_1: // Two-letter language code
        {
          std::string iso_639_1_code;
          g_LangCodeExpander.ConvertToISO6391(lang, iso_639_1_code);
          if (region)
             {
               std::string region = g_langInfo.GetRegionLocale(); // iso_3166 alpha-2 2 letter country code
               region = "-" + region;
               return (iso_639_1_code += region);
             }
          return iso_639_1_code;
        }
      case CLangCodeExpander::ISO_639_2: // Three letter language code
        {
          std::string iso_639_2_code;
          g_LangCodeExpander.ConvertToISO6392B(lang, iso_639_2_code);
          if (region)
          {
            std::string region = g_langInfo.GetRegionLocale(); // iso_3166 alpha-2 2 letter country code
            region = "-" + region;
            return (iso_639_2_code += region);
          }

          return iso_639_2_code;
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
      return static_cast<long>(CServiceBroker::GetMediaManager().GetDriveStatus());
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
        return GUILIB::GUIINFO::CGUIInfoLabel::GetLabel(cLine, INFO::DEFAULT_CONTEXT);
      else
        return infoMgr.GetLabel(ret, INFO::DEFAULT_CONTEXT);
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
      return StringUtils::Format("{:08x}.tbn", crc);
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

      if (UnicodeUtils::CompareNoCase(id, "datelong") == 0)
      {
        result = g_langInfo.GetDateFormat(true);
        UnicodeUtils::Replace(result, "DDDD", "%A");
        UnicodeUtils::Replace(result, "MMMM", "%B");
        UnicodeUtils::Replace(result, "D", "%d");
        UnicodeUtils::Replace(result, "YYYY", "%Y");
        }
        else if (UnicodeUtils::CompareNoCase(id, "dateshort") == 0)
        {
          result = g_langInfo.GetDateFormat(false);
          UnicodeUtils::Replace(result, "MM", "%m");
          UnicodeUtils::Replace(result, "DD", "%d");
#ifdef TARGET_WINDOWS
          UnicodeUtils::Replace(result, "M", "%#m");
          UnicodeUtils::Replace(result, "D", "%#d");
#else
          UnicodeUtils::Replace(result, "M", "%-m");
          UnicodeUtils::Replace(result, "D", "%-d");
#endif
          UnicodeUtils::Replace(result, "YYYY", "%Y");
        }
        else if (UnicodeUtils::CompareNoCase(id, "tempunit") == 0)
          result = g_langInfo.GetTemperatureUnitString();
        else if (UnicodeUtils::CompareNoCase(id, "speedunit") == 0)
          result = g_langInfo.GetSpeedUnitString();
        else if (UnicodeUtils::CompareNoCase(id, "time") == 0)
        {
          result = g_langInfo.GetTimeFormat();
          if (UnicodeUtils::StartsWith(result, "HH"))
            UnicodeUtils::Replace(result, "HH", "%H");
          else
            UnicodeUtils::Replace(result, "H", "%H");
          UnicodeUtils::Replace(result, "h", "%I");
          UnicodeUtils::Replace(result, "mm", "%M");
          UnicodeUtils::Replace(result, "ss", "%S");
          UnicodeUtils::Replace(result, "xx", "%p");
        }
        else if (UnicodeUtils::CompareNoCase(id, "meridiem") == 0)
          result = StringUtils::Format("{}/{}", g_langInfo.GetMeridiemSymbol(MeridiemSymbolAM),
                                       g_langInfo.GetMeridiemSymbol(MeridiemSymbolPM));

        return result;
    }

    //! @todo Add a mediaType enum
    String getSupportedMedia(const char* mediaType)
    {
      XBMC_TRACE;
      String result;
      if (UnicodeUtils::CompareNoCase(mediaType, "video") == 0)
        result = CServiceBroker::GetFileExtensionProvider().GetVideoExtensions();
      else if (UnicodeUtils::CompareNoCase(mediaType, "music") == 0)
        result = CServiceBroker::GetFileExtensionProvider().GetMusicExtensions();
      else if (UnicodeUtils::CompareNoCase(mediaType, "picture") == 0)
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


    bool startServer(int iTyp, bool bStart)
    {
      XBMC_TRACE;
      DelayedCallGuard dg;
      return CServiceBroker::GetNetwork().GetServices().StartServer(
          static_cast<CNetworkServices::ESERVERS>(iTyp), bStart != 0);
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

    int getSERVER_WEBSERVER()
    {
      return CNetworkServices::ES_WEBSERVER;
    }
    int getSERVER_AIRPLAYSERVER()
    {
      return CNetworkServices::ES_AIRPLAYSERVER;
    }
    int getSERVER_UPNPSERVER()
    {
      return CNetworkServices::ES_UPNPSERVER;
    }
    int getSERVER_UPNPRENDERER()
    {
      return CNetworkServices::ES_UPNPRENDERER;
    }
    int getSERVER_EVENTSERVER()
    {
      return CNetworkServices::ES_EVENTSERVER;
    }
    int getSERVER_JSONRPCSERVER()
    {
      return CNetworkServices::ES_JSONRPCSERVER;
    }
    int getSERVER_ZEROCONF()
    {
      return CNetworkServices::ES_ZEROCONF;
    }

    int getPLAYLIST_MUSIC() { return PLAYLIST_MUSIC; }
    int getPLAYLIST_VIDEO() { return PLAYLIST_VIDEO; }
    int getTRAY_OPEN()
    {
      return static_cast<int>(TrayState::OPEN);
    }
    int getDRIVE_NOT_READY()
    {
      return static_cast<int>(DriveState::NOT_READY);
    }
    int getTRAY_CLOSED_NO_MEDIA()
    {
      return static_cast<int>(TrayState::CLOSED_NO_MEDIA);
    }
    int getTRAY_CLOSED_MEDIA_PRESENT()
    {
      return static_cast<int>(TrayState::CLOSED_MEDIA_PRESENT);
    }
    int getLOGDEBUG() { return LOGDEBUG; }
    int getLOGINFO() { return LOGINFO; }
    int getLOGWARNING() { return LOGWARNING; }
    int getLOGERROR() { return LOGERROR; }
    int getLOGFATAL() { return LOGFATAL; }
    int getLOGNONE() { return LOGNONE; }

    // language string formats
    int getISO_639_1() { return CLangCodeExpander::ISO_639_1; }
    int getISO_639_2(){ return CLangCodeExpander::ISO_639_2; }
    int getENGLISH_NAME() { return CLangCodeExpander::ENGLISH_NAME; }

    const int lLOGDEBUG = LOGDEBUG;
  }
}
