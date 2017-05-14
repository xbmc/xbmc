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

  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions& options) override;
  virtual bool QueueNextFile(const CFileItem &file) override;
  virtual bool CloseFile(bool reopen = false) override;
  virtual bool IsPlaying() const override;
  virtual void Pause() override;
  virtual bool HasVideo() const override { return false; }
  virtual bool HasAudio() const override { return false; }
  virtual void Seek(bool bPlus, bool bLargeStep, bool bChapterOverride) override;
  virtual void SeekPercentage(float fPercent = 0) override;
  virtual float GetPercentage() override;
  virtual void SetVolume(float volume) override;
  virtual bool CanRecord() override { return false; }
  virtual bool IsRecording() override { return false; }
  virtual bool Record(bool bOnOff) override { return false; }

  virtual int  GetChapterCount() override                               { return 0; }
  virtual int  GetChapter() override                                    { return -1; }
  virtual void GetChapterName(std::string& strChapterName, int chapterIdx = -1) override { }
  virtual int  SeekChapter(int iChapter) override                       { return -1; }

  virtual void SeekTime(int64_t iTime = 0) override;
  virtual int64_t GetTime() override;
  virtual int64_t GetTotalTime() override;
  virtual void SetSpeed(float speed = 0) override;
  virtual float GetSpeed() override;

  virtual bool IsCaching() const override { return false; }
  virtual int GetCacheLevel() const override { return -1; }
  virtual void DoAudioWork() override;
  virtual bool OnAction(const CAction &action) override;

  virtual std::string GetPlayingTitle() override;

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
