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
#include "PlayListPlayer.h"
#include "FileItem.h"
#include "Application.h"
#include "log.h"
#include "utils/CharsetConverter.h"
#ifdef _WIN32
#include "WIN32Util.h"
#endif
#ifdef HAS_LIRC
#include "input/linux/LIRC.h"
#endif

CAppParamParser::CAppParamParser()
{
  m_testmode = false;
}

#ifdef _WIN32
void CAppParamParser::Parse(LPWSTR *szArglist, int nArgs)
{
  if (szArglist != NULL)
  {
    for (int i=0;i<nArgs;i++)
    {
      CStdString strArg;
      g_charsetConverter.wToUTF8(szArglist[i], strArg);
      ParseArg(strArg);
      if (strArg.Equals("-d"))
      {
        if (++i < nArgs)
        {
          int iSleep = _wtoi(szArglist[i]);
          if (iSleep > 0 && iSleep < 360)
            Sleep(iSleep*1000);
          else
            --i;
        }
      }
    }
    LocalFree(szArglist);
  }
  PlayPlaylist();
}
#else
void CAppParamParser::Parse(char* argv[], int nArgs)
{
  if (nArgs > 1)
  {
    for (int i = 1; i < nArgs; i++)
    {
      ParseArg(argv[i]);
#ifdef HAS_LIRC
      if (strnicmp(argv[i], "-l", 2) == 0 || strnicmp(argv[i], "--lircdev", 9) == 0)
      {
        // check the next arg with the proper value.
        int next=i+1;
        if (next < nArgs)
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
    }
  }
  PlayPlaylist();
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
  printf("  -n or --nolirc\tdo not use Lirc, i.e. no remote input.\n");
#endif
  printf("  --debug\t\tEnable debug logging\n");
  printf("  --test\t\tEnable test mode. [FILE] required.\n");
  printf("  --settings=<filename>\t\tLoads specified file after advancedsettings.xml replacing any settings specified\n");
  printf("  \t\t\t\tspecified file must exist in special://xbmc/system/\n");
  exit(0);
}

void CAppParamParser::EnableDebugMode()
{
  g_advancedSettings.SystemSettings->SetLogLevelAndHint(LOG_LEVEL_DEBUG);
  CLog::SetLogLevel(g_advancedSettings.SystemSettings->LogLevel());
}

void CAppParamParser::SetStartFullScreen()
{
  g_advancedSettings.SystemSettings->SetStartFullScreen(true);
}

void CAppParamParser::SetIsStandalone()
{
  g_application.SetStandAlone(true);
}

void CAppParamParser::SetPortable()
{
  g_application.EnablePlatformDirectories(false);
}

void CAppParamParser::ParseArg(CStdString arg)
{
  if (arg == "-fs" || arg == "--fullscreen")
    SetStartFullScreen();
  else if (arg == "-h" || arg == "--help")
    DisplayHelp();
  else if (arg == "--standalone")
    SetIsStandalone();
  else if (arg == "-p" || arg  == "--portable")
    SetPortable();
  else if (arg == "--debug")
    EnableDebugMode();
  else if (arg == "--legacy-res")
    g_application.SetEnableLegacyRes(true);
  else if (arg == "--test")
    m_testmode = true;
  else if (arg.substr(0, 11) == "--settings=")
  {
    g_advancedSettings.AddSettingsFile(arg.substr(11));
  }
  else if (arg.length() != 0 && arg[0] != '-')
  {
    if (m_testmode)
      g_application.SetEnableTestMode(true);
    CFileItemPtr pItem(new CFileItem(arg));
    pItem->m_strPath = arg;
    m_playlist.Add(pItem);
  }
}

void CAppParamParser::PlayPlaylist()
{
  if (m_playlist.Size() > 0)
  {
    g_playlistPlayer.Add(0, m_playlist);
    g_playlistPlayer.SetCurrentPlaylist(0);
  }

  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_PLAY, (DWORD) -1};
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
}
