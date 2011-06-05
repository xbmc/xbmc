/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "AppParamParser.h"
#include "AdvancedSettings.h"
#include "Application.h"
#include "log.h"
#ifdef _WIN32
#include "WIN32Util.h"
#endif

CAppParamParser::CAppParamParser()
{
}

#ifdef _WIN32
void CAppParamParser::Parse(LPWSTR *szArglist, int nArgs)
{
  if(szArglist != NULL)
  {
    for(int i=0;i<nArgs;i++)
    {
      CStdStringW strArgW(szArglist[i]);
      if(strArgW.Equals(L"-fs") || strArgW.Equals(L"--fullscreen"))
        SetStartFullScreen();
      else if(strArgW.Equals(L"-h") || strArgW.Equals(L"--help"))
        DisplayHelp();
      else if (strArgW.Equals(L"--standalone"))
        SetIsStandalone();
      else if(strArgW.Equals(L"-p") || strArgW.Equals(L"--portable"))
        SetPortable();
      else if(strArgW.Equals(L"-d"))
      {
        if(++i < nArgs)
        {
          int iSleep = _wtoi(szArglist[i]);
          if(iSleep > 0 && iSleep < 360)
            Sleep(iSleep*1000);
          else
            --i;
        }
      }
      else if(strArgW.Equals(L"--debug"))
        EnableDebugMode();
    }
    LocalFree(szArglist);
  }
}
#else
void CAppParamParser::Parse(char* argv[])
{
  if (argc > 1)
  {
    for (int i = 1; i < argc; i++)
    {
      if (strnicmp(argv[i], "-fs", 3) == 0 || strnicmp(argv[i], "--fullscreen", 12) == 0)
        SetStartFullScreen();
      else if (strnicmp(argv[i], "-h", 2) == 0 || strnicmp(argv[i], "--help", 6) == 0)
        DisplayHelp();
      else if (strnicmp(argv[i], "--standalone", 12) == 0)
        SetIsStandalone();
      else if (strnicmp(argv[i], "-p", 2) == 0 || strnicmp(argv[i], "--portable", 10) == 0)
        SetPortable();
      else if (strnicmp(argv[i], "--legacy-res", 12) == 0)
        g_application.SetEnableLegacyRes(true);
      else if (strnicmp(argv[i], "--test", 6) == 0)
        testmode=1;
#ifdef HAS_LIRC
      else if (strnicmp(argv[i], "-l", 2) == 0 || strnicmp(argv[i], "--lircdev", 9) == 0)
      {
        // check the next arg with the proper value.
        int next=i+1;
        if (next < argc)
        {
          if ((argv[next][0] != '-' ) && (argv[next][0] == '/' ))
          {
            g_RemoteControl.setDeviceName(argv[next]);
            i++;
          }
        }
      }
      else if (strnicmp(argv[i], "-n", 2) == 0 || strnicmp(argv[i], "--nolirc", 8) == 0)
         g_RemoteControl.setUsed(false);
#endif
      else if (strnicmp(argv[i], "--debug", 7) == 0)
        EnableDebugMode();
      else if (strlen(argv[i]) != 0 && argv[i][0] != '-')
      {
        CFileItemPtr pItem(new CFileItem(argv[i]));
        pItem->m_strPath = argv[i];
        if (testmode) 
          g_application.SetEnableTestMode(true);
        playlist.Add(pItem);
      }
    }
  }
}
#endif

void CAppParamParser::DisplayHelp()
{
  printf("Usage: xbmc [OPTION]... [FILE]...\n\n");
  printf("Arguments:\n");
  printf("  -fs\t\t\tRuns XBMC in full screen\n");
  printf("  --standalone\t\tXBMC runs in a stand alone environment without a window \n");
  printf("\t\t\tmanager and supporting applications. For example, that\n");
  printf("\t\t\tenables network settings.\n");
  printf("  -p or --portable\tXBMC will look for configurations in install folder instead of ~/.xbmc\n");
  printf("  --legacy-res\t\tEnables screen resolutions such as PAL, NTSC, etc.\n");
#ifdef HAS_LIRC
  printf("  -l or --lircdev\tLircDevice to use default is "LIRC_DEVICE" .\n");
  printf("  -n or --nolirc\tdo not use Lirc, aka no remote input.\n");
#endif
  printf("  --debug\t\tEnable debug logging\n");
  printf("  --test\t\tEnable test mode. [FILE] required.\n");
  printf("  --settings=<filename>\t\tLoads specified file after advancedsettings.xml replacing any settings specified\n");
  printf("  \t\t\t\tspecified file must exist in special://xbmc/system/\n");
  exit(0);
}

void CAppParamParser::EnableDebugMode()
{
  g_advancedSettings.m_logLevel     = LOG_LEVEL_DEBUG;
  g_advancedSettings.m_logLevelHint = LOG_LEVEL_DEBUG;
  CLog::SetLogLevel(g_advancedSettings.m_logLevel);
}

void CAppParamParser::SetStartFullScreen()
{
  g_advancedSettings.SetStartFullScreen(true);
}

void CAppParamParser::SetIsStandalone()
{
  g_application.SetStandAlone(true);
}

void CAppParamParser::SetPortable()
{
  g_application.EnablePlatformDirectories(false);
}