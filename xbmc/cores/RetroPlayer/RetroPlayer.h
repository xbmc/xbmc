/*
 *      Copyright (C) 2012-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "cores/RetroPlayer/guibridge/IGameCallback.h"
#include "cores/IPlayer.h"
#include "games/GameTypes.h"
#include "guilib/DispResource.h"
#include "threads/CriticalSection.h"

#include <memory>

namespace KODI
{
namespace GAME
{
  class CGameServices;
}

namespace RETRO
{
  class CRetroPlayerAutoSave;
  class CRetroPlayerInput;
  class CRPProcessInfo;
  class CRPRenderManager;
  class CRPStreamManager;

  class CRetroPlayer : public IPlayer, public IRenderLoop, public IGameCallback
  {
  public:
    explicit CRetroPlayer(IPlayerCallback& callback);
    ~CRetroPlayer() override;

    // implementation of IPlayer
    //virtual bool Initialize(TiXmlElement* pConfig) override { return true; }
    bool OpenFile(const CFileItem& file, const CPlayerOptions& options) override;
    //virtual bool QueueNextFile(const CFileItem &file) override { return false; }
    //virtual void OnNothingToQueueNotify() override { }
    bool CloseFile(bool reopen = false) override;
    bool IsPlaying() const override;
    bool CanPause() override;
    void Pause() override;
    bool HasVideo() const override { return true; }
    bool HasAudio() const override { return true; }
    bool HasGame() const override { return true; }
    //virtual bool HasRDS() const override { return false; }
    //virtual bool IsPassthrough() const override { return false;}
    bool CanSeek() override;
    void Seek(bool bPlus = true, bool bLargeStep = false, bool bChapterOverride = false) override;
    //virtual bool SeekScene(bool bPlus = true) override { return false; }
    void SeekPercentage(float fPercent = 0) override;
    float GetCachePercentage() override;
    void SetMute(bool bOnOff) override;
    //virtual void SetVolume(float volume) override { }
    //virtual void SetDynamicRangeCompression(long drc) override { }
    //virtual void SetAVDelay(float fValue = 0.0f) override { return; }
    //virtual float GetAVDelay() override { return 0.0f; }
    //virtual void SetSubTitleDelay(float fValue = 0.0f) override { }
    //virtual float GetSubTitleDelay() override { return 0.0f; }
    //virtual int GetSubtitleCount() override { return 0; }
    //virtual int GetSubtitle() override { return -1; }
    //virtual void GetSubtitleStreamInfo(int index, SubtitleStreamInfo &info) override { }
    //virtual void SetSubtitle(int iStream) override { }
    //virtual bool GetSubtitleVisible() override { return false; }
    //virtual void SetSubtitleVisible(bool bVisible) override { }
    //virtual void AddSubtitle(const std::string& strSubPath) override { }
    //virtual int GetAudioStreamCount() override { return 0; }
    //virtual int GetAudioStream() override { return -1; }
    //virtual void SetAudioStream(int iStream) override { }
    //virtual void GetAudioStreamInfo(int index, AudioStreamInfo &info) override { }
    //virtual int GetVideoStream() const override { return -1; }
    //virtual int GetVideoStreamCount() const override { return 0; }
    //virtual void GetVideoStreamInfo(int streamId, VideoStreamInfo &info) override { }
    //virtual void SetVideoStream(int iStream) override { }
    //virtual TextCacheStruct_t* GetTeletextCache() override { return NULL; }
    //virtual void LoadPage(int p, int sp, unsigned char* buffer) override { }
    //virtual std::string GetRadioText(unsigned int line) override { return ""; }
    //virtual int GetChapterCount() override { return 0; }
    //virtual int GetChapter() override { return -1; }
    //virtual void GetChapterName(std::string& strChapterName, int chapterIdx = -1) override { return; }
    //virtual int64_t GetChapterPos(int chapterIdx = -1) override { return 0; }
    //virtual int SeekChapter(int iChapter) override { return -1; }
    //virtual float GetActualFPS() override { return 0.0f; }
    void SeekTime(int64_t iTime = 0) override;
    bool SeekTimeRelative(int64_t iTime) override;
    //virtual void SetTotalTime(int64_t time) override { } // Only used by Air Tunes Server
    void SetSpeed(float speed) override;
    //virtual bool IsCaching() const override { return false; }
    //virtual int GetCacheLevel() const override { return -1; }
    //virtual bool IsInMenu() const override { return false; }
    //virtual bool HasMenu() const override { return false; }
    //virtual void DoAudioWork() override { }
    //virtual bool OnAction(const CAction &action) override { return false; }
    bool OnAction(const CAction &action) override;
    std::string GetPlayerState() override;
    bool SetPlayerState(const std::string& state) override;
    //virtual bool SwitchChannel(const PVR::CPVRChannelPtr &channel) override { return false; }
    //virtual void GetAudioCapabilities(std::vector<int> &audioCaps) override { audioCaps.assign(1,IPC_AUD_ALL); }
    //virtual void GetSubtitleCapabilities(std::vector<int> &subCaps) override { subCaps.assign(1,IPC_SUBS_ALL); }
    void FrameMove() override;
    void Render(bool clear, uint32_t alpha = 255, bool gui = true) override;
    void FlushRenderer() override;
    //void SetRenderViewMode(int mode) override { } // Must go through render callback
    //float GetRenderAspectRatio() override { return 1.0f; }
    void TriggerUpdateResolution() override;
    bool IsRenderingVideo() override;
    //bool Supports(EINTERLACEMETHOD method) override { return false; } // Must go through render callback
    //EINTERLACEMETHOD GetDeinterlacingMethodDefault() override { return EINTERLACEMETHOD::VS_INTERLACEMETHOD_NONE; } // Must go through render callback
    //bool Supports(ESCALINGMETHOD method) override { return false; } // Must go through render callback
    //bool Supports(ERENDERFEATURE feature) override { return false; } // Must go through render callback
    //unsigned int RenderCaptureAlloc() override { return 0; }
    //void RenderCaptureRelease(unsigned int captureId) override { }
    //void RenderCapture(unsigned int captureId, unsigned int width, unsigned int height, int flags) override { }
    //bool RenderCaptureGetPixels(unsigned int captureId, unsigned int millis, uint8_t *buffer, unsigned int size) override { return false; }

    // Implementation of IGameCallback
    std::string GameClientID() const override;

  private:
    void SetSpeedInternal(double speed);

    /*!
     * \brief Called when the speed changes
     * \param newSpeed The new speed, possibly equal to the previous speed
     */
    void OnSpeedChange(double newSpeed);

    /*!
     * \brief Closes the OSD and shows the FullscreenGame window
     */
    void CloseOSD();

    void RegisterWindowCallbacks();
    void UnregisterWindowCallbacks();

    /**
     * \brief Dump game information (if any) to the debug log.
     */
    void PrintGameInfo(const CFileItem &file) const;

    uint64_t GetTime();
    uint64_t GetTotalTime();

    // Construction parameters
    GAME::CGameServices &m_gameServices;

    enum class State
    {
      STARTING,
      FULLSCREEN,
      BACKGROUND,
    };

    State                              m_state = State::STARTING;
    double                             m_priorSpeed = 0.0f; // Speed of gameplay before entering OSD
    std::unique_ptr<CRPProcessInfo>    m_processInfo;
    std::unique_ptr<CRPRenderManager>  m_renderManager;
    std::unique_ptr<CRPStreamManager>  m_streamManager;
    std::unique_ptr<CRetroPlayerInput> m_input;
    std::unique_ptr<CRetroPlayerAutoSave> m_autoSave;
    GAME::GameClientPtr                m_gameClient;
    CCriticalSection                   m_mutex;
  };
}
}
