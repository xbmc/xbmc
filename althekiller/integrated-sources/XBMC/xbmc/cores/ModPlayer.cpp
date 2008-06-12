/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "stdafx.h"
#include "modplayer.h"
#include "Settings.h"
#include "FileItem.h"
#include "FileSystem/File.h"

using namespace XFILE;

#pragma comment(linker,"/merge:MOD_RD=MOD_RX")
#pragma comment(linker,"/section:MOD_RX,REN")
#pragma comment(linker,"/section:MOD_RW,RWN")

static IAudioCallback* m_pCallback = NULL;

bool ModPlayer::IsSupportedFormat(const CStdString& strFmt)
{
  CStdString fmt(strFmt);
  fmt.Normalize();
  if (fmt == "amf" ||
      fmt == "669" ||
      fmt == "dmf" ||
      fmt == "dsm" ||
      fmt == "far" ||
      fmt == "gdm" ||
      fmt == "imf" ||
      fmt == "m15" ||
      fmt == "med" ||
      fmt == "okt" ||
      fmt == "stm" ||
      fmt == "sfx" ||
      fmt == "ult" ||
      fmt == "uni")// ||
      //fmt == "xm")
  {
    return true;
  }
  return false;
}

void ModCallback(unsigned char* p, int s)
{
  static bool call = true;
  if (m_pCallback)
  {
    if (call)
      m_pCallback->OnAudioData(p, s);
    call = !call;
  }
}

ModPlayer::ModPlayer(IPlayerCallback& callback) : IPlayer(callback)
{
  m_bIsPlaying = false;
  m_bPaused = false;

  CSectionLoader::Load("MOD_RX");
  CSectionLoader::Load("MOD_RW");

  if (!mikxboxInit())
  {
    CLog::Log(LOGERROR, "ModPlayer: Could not initialize sound, reason: %s", MikMod_strerror(mikxboxGetErrno()));
  }

  SetVolume(g_stSettings.m_nVolumeLevel);
  // mikxboxSetMusicVolume(127);
  mikxboxSetCallback(ModCallback);
}

ModPlayer::~ModPlayer()
{
  CloseFile();
  mikxboxExit();

  CSectionLoader::Unload("MOD_RX");
  CSectionLoader::Unload("MOD_RW");
}

bool ModPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  CloseFile();

  char* str = NULL;
  if (!file.IsHD())
  {
    if (!CFile::Cache(file.m_strPath.c_str(), "Z:\\cachedmod", NULL, NULL))
    {
      ::DeleteFile("Z:\\cachedmod");
      CLog::Log(LOGERROR, "ModPlayer: Unable to cache file %s\n", file.m_strPath.c_str());
      return false;
    }
    str = strdup("Z:\\cachedmod");
  }
  else
    str = strdup(file.m_strPath.c_str());

  m_pModule = Mod_Player_Load(str, 127, 0);
  free(str);

  if (m_pModule)
  {
    Mod_Player_Start(m_pModule);
  }
  else
  {
    CLog::Log(LOGERROR, "ModPlayer: Could not load module %s: %s\n", file.m_strPath.c_str(), MikMod_strerror(mikxboxGetErrno()));
    return false;
  }

  m_bIsPlaying = true;
  m_bPaused = false;
  m_bStopPlaying = false;

  if ( ThreadHandle() == NULL)
  {
    Create();
  }

  return true;
}

bool ModPlayer::CloseFile()
{
  if (IsPaused())
    Pause();

  m_bStopPlaying = true;
  // bit nasty but trying to do it with events locks the xbox hard
  while (m_bIsPlaying)
    Sleep(5);
  Mod_Player_Stop();
  StopThread();
  return true;
}

bool ModPlayer::IsPlaying() const
{
  return m_bIsPlaying;
}


void ModPlayer::Pause()
{
  if (!m_bIsPlaying) return ;
  m_bPaused = !m_bPaused;
  Mod_Player_TogglePause();
}

bool ModPlayer::IsPaused() const
{
  return m_bPaused;
}

#ifdef _DEBUG
static DWORD AveUpdate = 0;
static DWORD NumUpdates = 0;
static DWORD LastUpdate = 0;
#endif // _DEBUG

__int64 ModPlayer::GetTime()
{
  if (!m_bIsPlaying) return 0;
  return mikxboxGetPTS();
}

bool ModPlayer::HasVideo()
{
  return false;
}

bool ModPlayer::HasAudio()
{
  return true;
}

void ModPlayer::Seek(bool bPlus, bool bLargeStep)
{}

void ModPlayer::ToggleFrameDrop()
{}

void ModPlayer::SetVolume(long nVolume)
{
  // mikxbox uses a range of (0 -> 127).  Convert our mB range to this...
  float fVolume = (float(nVolume - VOLUME_MINIMUM)) / (float)(VOLUME_MAXIMUM - VOLUME_MINIMUM) * 127.0f;
  mikxboxSetMusicVolume((int)(fVolume + 0.5f));
}

void ModPlayer::GetAudioInfo( CStdString& strAudioInfo)
{}

void ModPlayer::GetVideoInfo( CStdString& strVideoInfo)
{}


void ModPlayer::GetGeneralInfo( CStdString& strVideoInfo)
{}

void ModPlayer::Update(bool bPauseDrawing)
{}

void ModPlayer::OnStartup()
{
  m_callback.OnPlayBackStarted();
}

void ModPlayer::OnExit()
{
  m_bIsPlaying = false;
  OutputDebugString("Free module\n");
  Mod_Player_Free(m_pModule);
  if (!m_bStopPlaying)
    m_callback.OnPlayBackEnded();
}

void ModPlayer::Process()
{
  while (!m_bStopPlaying && Mod_Player_Active())
    MikMod_Update();
}
void ModPlayer::RegisterAudioCallback(IAudioCallback* pCallback)
{
  m_pCallback = pCallback;
  if (m_bIsPlaying)
    m_pCallback->OnInitialize(2, 48000, 16);
}

void ModPlayer::UnRegisterAudioCallback()
{
  m_pCallback = NULL;
}
