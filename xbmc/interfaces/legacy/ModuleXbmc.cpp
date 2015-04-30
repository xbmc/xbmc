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

// TODO: Need a uniform way of returning an error status

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif
#include "network/Network.h"

#include "ModuleXbmc.h"

#include "Application.h"
#include "ApplicationMessenger.h"
#include "aojsonrpc.h"
#ifndef TARGET_WINDOWS
#include "XTimeUtils.h"
#endif
#include "guilib/LocalizeStrings.h"
#include "GUIInfoManager.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/GUIWindowManager.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/Crc32.h"
#include "FileItem.h"
#include "LangInfo.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "guilib/TextureManager.h"
#include "Util.h"
#include "cores/AudioEngine/AEFactory.h"
#include "storage/MediaManager.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "AddonUtils.h"

#include "LanguageHook.h"

#include "cores/VideoRenderers/RenderCapture.h"

#include "threads/SystemClock.h"
#include <vector>
#include "utils/log.h"

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
        level = LOGNOTICE;
      CLog::Log(level, "%s", msg);
    }

    void shutdown()
    {
      XBMC_TRACE;
      ThreadMessage tMsg = {TMSG_SHUTDOWN};
      CApplicationMessenger::Get().SendMessage(tMsg);
    }

    void restart()
    {
      XBMC_TRACE;
      ThreadMessage tMsg = {TMSG_RESTART};
      CApplicationMessenger::Get().SendMessage(tMsg);
    }

    void executescript(const char* script)
    {
      XBMC_TRACE;
      if (! script)
        return;

      ThreadMessage tMsg = {TMSG_EXECUTE_SCRIPT};
      tMsg.strParam = script;
      CApplicationMessenger::Get().SendMessage(tMsg);
    }

    void executebuiltin(const char* function, bool wait /* = false*/)
    {
      XBMC_TRACE;
      if (! function)
        return;
      CApplicationMessenger::Get().ExecBuiltIn(function,wait);
    }

    String executeJSONRPC(const char* jsonrpccommand)
    {
      XBMC_TRACE;
      DelayedCallGuard dg;
#ifdef HAS_JSONRPC
      String ret;

      if (! jsonrpccommand)
        return ret;

      //    String method = jsonrpccommand;

      CAddOnTransport transport;
      CAddOnTransport::CAddOnClient client;

      return JSONRPC::CJSONRPC::MethodCall(/*method*/ jsonrpccommand, &transport, &client);
#else
      THROW_UNIMP("executeJSONRPC");
#endif
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
          ::Sleep(nextSleep);
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
      return CSettings::Get().GetString("lookandfeel.skin");
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
          g_LangCodeExpander.ConvertToISO6392T(lang, langCode);
          if (region)
          {
            std::string region = g_langInfo.GetRegionLocale();
            std::string region3Code;
            g_LangCodeExpander.ConvertToISO6392T(region, region3Code);
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
      CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
      if (iface)
        return iface->GetCurrentIPAddress();

      return cTitleIP;
    }

    long getDVDState()
    {
      XBMC_TRACE;
      return g_mediaManager.GetDriveStatus();
    }

    long getFreeMem()
    {
      XBMC_TRACE;
      MEMORYSTATUSEX stat;
      stat.dwLength = sizeof(MEMORYSTATUSEX);
      GlobalMemoryStatusEx(&stat);
      return (long)(stat.ullAvailPhys  / ( 1024 * 1024 ));
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

      int ret = g_infoManager.TranslateString(cLine);
      //doesn't seem to be a single InfoTag?
      //try full blown GuiInfoLabel then
      if (ret == 0)
        return CGUIInfoLabel::GetLabel(cLine);
      else
        return g_infoManager.GetLabel(ret);
    }

    String getInfoImage(const char * infotag)
    {
      XBMC_TRACE;
      if (!infotag)
        {
          String ret;
          return ret;
        }

      int ret = g_infoManager.TranslateString(infotag);
      return g_infoManager.GetImage(ret, WINDOW_INVALID);
    }

    void playSFX(const char* filename, bool useCached)
    {
      XBMC_TRACE;
      if (!filename)
        return;

      if (XFILE::CFile::Exists(filename))
      {
        g_audioManager.PlayPythonSound(filename,useCached);
      }
    }

    void stopSFX()
    {
      XBMC_TRACE;
      DelayedCallGuard dg;
      g_audioManager.Stop();
    }
    
    void enableNavSounds(bool yesNo)
    {
      XBMC_TRACE;
      g_audioManager.Enable(yesNo);
    }

    bool getCondVisibility(const char *condition)
    {
      XBMC_TRACE;
      if (!condition)
        return false;

      bool ret;
      {
        LOCKGUI;

        int id = g_windowManager.GetTopMostModalDialogID();
        if (id == WINDOW_INVALID) id = g_windowManager.GetActiveWindow();
        ret = g_infoManager.EvaluateBool(condition,id);
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
      Crc32 crc;
      crc.ComputeFromLowerCase(path);
      return StringUtils::Format("%08x.tbn", (unsigned __int32)crc);;
    }

    String makeLegalFilename(const String& filename, bool fatX)
    {
      XBMC_TRACE;
      return CUtil::MakeLegalPath(filename);
    }

    String translatePath(const String& path)
    {
      XBMC_TRACE;
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

    String validatePath(const String& path)
    {
      XBMC_TRACE;
      return CUtil::ValidatePath(path, true);
    }

    String getRegion(const char* id)
    {
      XBMC_TRACE;
      std::string result;

      if (strcmpi(id, "datelong") == 0)
        {
          result = g_langInfo.GetDateFormat(true);
          StringUtils::Replace(result, "DDDD", "%A");
          StringUtils::Replace(result, "MMMM", "%B");
          StringUtils::Replace(result, "D", "%d");
          StringUtils::Replace(result, "YYYY", "%Y");
        }
      else if (strcmpi(id, "dateshort") == 0)
        {
          result = g_langInfo.GetDateFormat(false);
          StringUtils::Replace(result, "MM", "%m");
          StringUtils::Replace(result, "DD", "%d");
          StringUtils::Replace(result, "YYYY", "%Y");
        }
      else if (strcmpi(id, "tempunit") == 0)
        result = g_langInfo.GetTemperatureUnitString();
      else if (strcmpi(id, "speedunit") == 0)
        result = g_langInfo.GetSpeedUnitString();
      else if (strcmpi(id, "time") == 0)
        {
          result = g_langInfo.GetTimeFormat();
          StringUtils::Replace(result, "H", "%H");
          StringUtils::Replace(result, "h", "%I");
          StringUtils::Replace(result, "mm", "%M");
          StringUtils::Replace(result, "ss", "%S");
          StringUtils::Replace(result, "xx", "%p");
        }
      else if (strcmpi(id, "meridiem") == 0)
        result = StringUtils::Format("%s/%s",
                                     g_langInfo.GetMeridiemSymbol(MeridiemSymbolAM).c_str(),
                                     g_langInfo.GetMeridiemSymbol(MeridiemSymbolPM).c_str());

      return result;
    }

    // TODO: Add a mediaType enum
    String getSupportedMedia(const char* mediaType)
    {
      XBMC_TRACE;
      String result;
      if (strcmpi(mediaType, "video") == 0)
        result = g_advancedSettings.m_videoExtensions;
      else if (strcmpi(mediaType, "music") == 0)
        result = g_advancedSettings.GetMusicExtensions();
      else if (strcmpi(mediaType, "picture") == 0)
        result = g_advancedSettings.m_pictureExtensions;

      // TODO:
      //    else
      //      return an error

      return result;
    }

    bool skinHasImage(const char* image)
    {
      XBMC_TRACE;
      return g_TextureManager.HasTexture(image);
    }


    bool startServer(int iTyp, bool bStart, bool bWait)
    {
      XBMC_TRACE;
      DelayedCallGuard dg;
      return g_application.StartServer((CApplication::ESERVERS)iTyp, bStart != 0, bWait != 0);
    }

    void audioSuspend()
    {  
      CAEFactory::Suspend();
    }

    void audioResume()
    { 
      CAEFactory::Resume();
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
            g_LangCodeExpander.ConvertToISO6392T(language, convertedLanguage);
            g_LangCodeExpander.Lookup(convertedLanguage, convertedLanguage);
          }
          break;
        }
      case CLangCodeExpander::ISO_639_1:
        g_LangCodeExpander.ConvertToISO6391(language, convertedLanguage);
        break;
      case CLangCodeExpander::ISO_639_2:
        g_LangCodeExpander.ConvertToISO6392T(language, convertedLanguage);
        break;
      default:
        return "";
      }
      return convertedLanguage;
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
    int getPLAYER_CORE_AUTO() { return EPC_NONE; }
    int getPLAYER_CORE_DVDPLAYER() { return EPC_DVDPLAYER; }
    int getPLAYER_CORE_MPLAYER() { return EPC_MPLAYER; }
    int getPLAYER_CORE_PAPLAYER() { return EPC_PAPLAYER; }
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

    // render capture user states
    int getCAPTURE_STATE_WORKING() { return CAPTURESTATE_WORKING; }
    int getCAPTURE_STATE_DONE(){ return CAPTURESTATE_DONE; }
    int getCAPTURE_STATE_FAILED() { return CAPTURESTATE_FAILED; }

    // render capture flags
    int getCAPTURE_FLAG_CONTINUOUS() { return (int)CAPTUREFLAG_CONTINUOUS; }
    int getCAPTURE_FLAG_IMMEDIATELY() { return (int)CAPTUREFLAG_IMMEDIATELY; }

    // language string formats
    int getISO_639_1() { return CLangCodeExpander::ISO_639_1; } 
    int getISO_639_2(){ return CLangCodeExpander::ISO_639_2; }
    int getENGLISH_NAME() { return CLangCodeExpander::ENGLISH_NAME; }

    const int lLOGNOTICE = LOGNOTICE;
  }
}
