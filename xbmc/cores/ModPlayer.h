#pragma once

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

#include "iplayer.h"
#include "utils/thread.h"
#include "lib/mikxbox/mikmod.h"
#include "lib/mikxbox/mikxbox.h"

class ModPlayer : public IPlayer, public CThread
{
public:
  ModPlayer(IPlayerCallback& callback);
  virtual ~ModPlayer();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual void UnRegisterAudioCallback();

  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions &options);
  virtual bool CloseFile();
  virtual bool IsPlaying() const;
  virtual void Pause();
  virtual bool IsPaused() const;
  virtual bool HasVideo();
  virtual bool HasAudio();
  virtual void ToggleFrameDrop();
  virtual void Seek(bool bPlus = true, bool bLargeStep = false);
  virtual void SetVolume(long nVolume);
  virtual void GetAudioInfo( CStdString& strAudioInfo);
  virtual void GetVideoInfo( CStdString& strVideoInfo);
  virtual void GetGeneralInfo( CStdString& strVideoInfo);
  virtual void Update(bool bPauseDrawing = false);
  virtual void GetVideoRect(RECT& SrcRect, RECT& DestRect){};
  virtual void GetVideoAspectRatio(float& fAR) {};
  virtual __int64 GetTime();

  static bool IsSupportedFormat(const CStdString& strFmt);

protected:
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  MODULE* m_pModule;
  bool m_bPaused;
  volatile bool m_bIsPlaying;
  volatile bool m_bStopPlaying;
};
