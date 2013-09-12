/*
 *      Copyright (C) 2012-2013 Team XBMC
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
#pragma once

#include "RetroPlayerAudio.h"
#include "RetroPlayerInput.h"
#include "RetroPlayerVideo.h"
#include "cores/IPlayer.h"
#include "FileItem.h"
#include "games/GameClient.h"
#include "games/libretro/LibretroCallbacks.h"
#include "threads/Thread.h"
#include "threads/Event.h"

#include <stdint.h>
#include <string>

class CRetroPlayer : public IPlayer, public GAMES::ILibretroCallbacksAV, public CThread
{
public:
  CRetroPlayer(IPlayerCallback& callback);
  virtual ~CRetroPlayer() { CloseFile(); }

  // Inherited from IPlayer
  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions& options);
  virtual bool CloseFile();

  virtual bool IsPlaying() const { return !m_bStop && m_file && m_gameClient; }
  virtual void Pause();
  virtual bool IsPaused() const { return m_playSpeed == 0; }

  virtual bool HasVideo() const { return true; }
  virtual bool HasAudio() const { return true; }

  virtual IInputHandler *GetInputHandler() { return &m_input; }

  virtual void GetAudioInfo(CStdString& strAudioInfo) { strAudioInfo = "CRetroPlayer:GetAudioInfo"; }
  virtual void GetVideoInfo(CStdString& strVideoInfo) { strVideoInfo = "CRetroPlayer:GetVideoInfo"; }
  virtual void GetGeneralInfo(CStdString& strGeneralInfo) { strGeneralInfo = "CRetroPlayer:GetGeneralInfo"; }

  //virtual float GetActualFPS() { return 0.0f; }
  //virtual int GetSourceBitrate() { return 0; }
  virtual int  GetBitsPerSample() { return 8 * 2 * sizeof(int16_t); }
  virtual int  GetSampleRate() { return m_samplerate; }

  //virtual int  GetPictureWidth() { return 0; }
  //virtual int  GetPictureHeight() { return 0; }
  //virtual bool GetStreamDetails(CStreamDetails &details) { return false; }
  //virtual void GetVideoStreamInfo(SPlayerVideoStreamInfo &info) { }

  //virtual int  GetAudioStreamCount() { return 0; }
  //virtual int  GetAudioStream() { return -1; }
  //virtual void SetAudioStream(int iStream) { }
  //virtual void GetAudioStreamInfo(int index, SPlayerAudioStreamInfo &info) { }

  //virtual void  SetAVDelay(float fValue = 0.0f) { return; }
  //virtual float GetAVDelay() { return 0.0f;};

  //virtual bool CanRecord() { return false; }
  //virtual bool IsRecording() { return false; }
  //virtual bool Record(bool bOnOff) { return false; }

  virtual void ToFFRW(int iSpeed = 0);

  // A "back buffer" is used to store game history to enable rewinding Braid-style.
  // The time is computed from the number of frames avaiable in the buffer, and
  // allows seeking and rewinding over this time range. Fast-forwarding will
  // simply play the game at a faster speed, and will cause this buffer to fill
  // faster. When the buffer is full, the progress bar will reach 100% (60s by
  // default), which will look like a track being finished, but instead of
  // exiting the game will continue to play.
  virtual bool CanSeek() { return true; }
  virtual void Seek(bool bPlus = true, bool bLargeStep = false);
  virtual void SeekPercentage(float fPercent = 0);
  virtual float GetPercentage();
  virtual void SeekTime(int64_t iTime = 0);
  virtual int64_t GetTime();
  virtual int64_t GetTotalTime();

  // TODO: Skip to next disk in multi-disc games (e.g. PSX)
  virtual bool SkipNext() { return false; }

  /*
   * Inherited from ILibretroCallbacksAV. Used to send and receive data from
   * the game clients.
   */
  virtual void VideoFrame(const void *data, unsigned width, unsigned height, size_t pitch);
  virtual void AudioSample(int16_t left, int16_t right);
  virtual size_t AudioSampleBatch(const int16_t *data, size_t frames);
  virtual int16_t GetInputState(unsigned port, unsigned device, unsigned index, unsigned id);
  virtual void SetPixelFormat(LIBRETRO::retro_pixel_format pixelFormat);
  virtual void SetKeyboardCallback(LIBRETRO::retro_keyboard_event_t callback);

protected:
  virtual void Process();

private:
  /**
   * Dump game information (if any) to the debug log.
   */
  void PrintGameInfo(const CFileItem &file) const;

  /**
   * Create the audio component. Chooses a compatible samplerate and returns
   * a multiplier representing the framerate adjustment factor, allowing us to
   * sync the video clock to the audio.
   * @param  samplerate - the game client's reported audio sample rate
   * @return the framerate multiplier (chosen samplerate / specified samplerate)
   *         or 1.0 if no audio.
   */
  double CreateAudio(double samplerate);
  void   CreateVideo(double framerate);

  CRetroPlayerVideo    m_video;
  CRetroPlayerAudio    m_audio;
  CRetroPlayerInput    m_input;

  CFileItemPtr         m_file;
  GAMES::GameClientPtr m_gameClient;

  LIBRETRO::retro_keyboard_event_t m_keyboardCallback; // TODO

  CPlayerOptions       m_PlayerOptions;
  int                  m_playSpeed; // Normal play speed is PLAYSPEED_NORMAL (1000)
  CEvent               m_pauseEvent;
  CCriticalSection     m_critSection; // For synchronization of Open() and Close() calls

  unsigned int m_samplerate;
};
