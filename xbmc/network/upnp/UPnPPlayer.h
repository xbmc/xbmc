/*
 *      Copyright (c) 2006 elupus (Joakim Plate)
 *      Copyright (C) 2006-2013 Team XBMC
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

#include "cores/IPlayer.h"
#include <string>

class PLT_MediaController;
class CGUIDialogBusy;

namespace XbmcThreads { class EndTime; }


namespace UPNP
{

class CUPnPPlayerController;

class CUPnPPlayer
  : public IPlayer
{
public:
  CUPnPPlayer(IPlayerCallback& callback, const char* uuid);
  virtual ~CUPnPPlayer();

  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions& options);
  virtual bool QueueNextFile(const CFileItem &file);
  virtual bool CloseFile(bool reopen = false);
  virtual bool IsPlaying() const;
  virtual void Pause() override;
  virtual bool HasVideo() const { return false; }
  virtual bool HasAudio() const { return false; }
  virtual void Seek(bool bPlus, bool bLargeStep, bool bChapterOverride);
  virtual void SeekPercentage(float fPercent = 0);
  virtual float GetPercentage();
  virtual void SetVolume(float volume);
  virtual void GetAudioInfo(std::string& strAudioInfo) {};
  virtual void GetVideoInfo(std::string& strVideoInfo) {};
  virtual bool CanRecord() { return false;};
  virtual bool IsRecording() { return false;};
  virtual bool Record(bool bOnOff) { return false;};

  virtual int  GetChapterCount()                               { return 0; }
  virtual int  GetChapter()                                    { return -1; }
  virtual void GetChapterName(std::string& strChapterName)     { return; }
  virtual int  SeekChapter(int iChapter)                       { return -1; }

  virtual void SeekTime(__int64 iTime = 0);
  virtual int64_t GetTime();
  virtual int64_t GetTotalTime();
  virtual void SetSpeed(float speed = 0) override;
  virtual float GetSpeed() override;

  virtual bool SkipNext(){return false;}
  virtual bool IsCaching() const {return false;};
  virtual int GetCacheLevel() const {return -1;};
  virtual void DoAudioWork();
  virtual bool OnAction(const CAction &action);

  virtual std::string GetPlayingTitle();

  int PlayFile(const CFileItem& file, const CPlayerOptions& options, CGUIDialogBusy*& dialog, XbmcThreads::EndTime& timeout);

private:
  bool IsPaused() const;

  PLT_MediaController*   m_control;
  CUPnPPlayerController* m_delegate;
  std::string            m_current_uri;
  std::string            m_current_meta;
  bool                   m_started;
  bool                   m_stopremote;
};

} /* namespace UPNP */
