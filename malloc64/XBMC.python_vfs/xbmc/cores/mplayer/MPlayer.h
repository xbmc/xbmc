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

#include "cores/iplayer.h"
#include "DllMPlayer.h"
#include "Edl.h"

class CMPlayer : public IPlayer, public CThread
{
public:
  class Options
  {
  public:
    Options();
    bool GetNonInterleaved() const;
    void SetNonInterleaved(bool bOnOff) ;

    bool GetForceIndex() const;
    void SetForceIndex(bool bOnOff);

    bool GetNoCache() const;
    void SetNoCache(bool bOnOff);

    void SetPrefil(float percent) { m_fPrefil = percent; }

    bool GetNoIdx() const;
    void SetNoIdx(bool bOnOff) ;

    float GetVolumeAmplification() const;
    void SetVolumeAmplification(float fDB) ;

    int GetAudioStream() const;
    void SetAudioStream(int iStream);

    int GetSubtitleStream() const;
    void SetSubtitleStream(int iStream);

    int GetChannels() const;
    void SetChannels(int iChannels);

    bool GetAC3PassTru();
    void SetAC3PassTru(bool bOnOff);

    bool GetDTSPassTru();
    void SetDTSPassTru(bool bOnOff);

    bool GetLimitedHWAC3();
    void SetLimitedHWAC3(bool bOnOff);

    inline bool GetDeinterlace() { return m_bDeinterlace; };
    inline void SetDeinterlace(bool mDeint) { m_bDeinterlace = mDeint; };
      
    const std::string& GetSubtitleCharset() { return m_subcp; };
    void SetSubtitleCharset(const std::string& subcp) { m_subcp = subcp; };
      
    const std::string GetChannelMapping() const;
    void SetChannelMapping(const std::string& strMapping);
    void SetSpeed(float fSpeed);
    float GetSpeed() const;
    void SetFPS(float fFPS);
    float GetFPS() const;
    void GetOptions(int& argc, char* argv[]);
    void SetDVDDevice(const std::string & strDevice);
    void SetFlipBiDiCharset(const std::string& strCharset);
    void SetRawAudioFormat(const std::string& strHexRawAudioFormat);

    void SetAutoSync(int iAutoSync);

    void SetEdl(const std::string& strEdl);

    void SetAudioOutput(const std::string& output) { m_videooutput = output; }
    void SetVideoOutput(const std::string& output) { m_audiooutput = output; }
    void SetAudioCodec(const std::string& codec) { m_videocodec = codec; }
    void SetVideoCodec(const std::string& codec) { m_audiocodec = codec; }
    void SetDemuxer(const std::string& demuxer) { m_demuxer = demuxer; }
    void SetSyncSpeed(const float synccomp) { m_synccomp = synccomp; }
    void SetFullscreen(bool fullscreen) { m_fullscreen = fullscreen; }

  private:
    bool m_bDeinterlace;
    bool m_bResampleAudio;
    bool m_bNoCache;
    float m_fPrefil;
    bool m_bNoIdx;
    float m_fSpeed;
    float m_fFPS;
    int m_iChannels;
    int m_iAudioStream;
    int m_iSubtitleStream;
    int m_iAutoSync;
    bool m_bAC3PassTru;
    bool m_bDTSPassTru;
    float m_fVolumeAmplification;
    bool m_bNonInterleaved;
    bool m_bForceIndex;
    bool m_bLimitedHWAC3;
    std::string m_videooutput;
    std::string m_audiooutput;
    std::string m_videocodec;
    std::string m_audiocodec;
    std::string m_demuxer;
    bool m_bDisableAO;
    std::string m_subcp;
    std::string m_strEdl;
    std::string m_strChannelMapping;
    std::string m_strDvdDevice;
    std::string m_strFlipBiDiCharset;
    std::string m_strHexRawAudioFormat;
    float m_synccomp;
    std::vector<std::string> m_vecOptions;
    bool m_fullscreen;
  };
  CMPlayer(IPlayerCallback& callback);
  virtual ~CMPlayer();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual void UnRegisterAudioCallback();
  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions& initoptions);
  virtual bool CloseFile();
  virtual bool IsPlaying() const;
  virtual void Pause();
  virtual bool IsPaused() const;  
  virtual void Unload();
  virtual bool HasVideo();
  virtual bool HasAudio();

  virtual void ToggleFrameDrop();
  virtual void Seek(bool bPlus = true, bool bLargeStep = false);
  virtual bool SeekScene(bool bPlus = true);
  virtual void SetVolume(long nVolume);
  virtual void SetDynamicRangeCompression(long drc);
  virtual void GetAudioInfo( CStdString& strAudioInfo);
  virtual void GetVideoInfo( CStdString& strVideoInfo);
  virtual void GetGeneralInfo( CStdString& strVideoInfo);
  virtual void Update(bool bPauseDrawing = false);
  virtual void GetVideoRect(RECT& SrcRect, RECT& DestRect);
  virtual void GetVideoAspectRatio(float& fAR);
  virtual bool CanRecord() ;
  virtual bool IsRecording();
  virtual bool Record(bool bOnOff) ;
  virtual void SeekPercentage(float fPercent = 0);
  virtual float GetPercentage();
  virtual void SetAVDelay(float fValue = 0.0f);
  virtual float GetAVDelay();
  virtual float GetActualFPS();

  virtual void SetSubTitleDelay(float fValue = 0.0f);
  virtual float GetSubTitleDelay();

  virtual int GetSubtitleCount();
  virtual int GetSubtitle();
  virtual bool AddSubtitle(const CStdString& strFileName);
  virtual void GetSubtitleName(int iStream, CStdString &strStreamName);
  virtual void SetSubtitle(int iStream);
  virtual bool GetSubtitleVisible();
  virtual void SetSubtitleVisible(bool bVisible);
  virtual bool GetSubtitleExtension(CStdString &strSubtitleExtension);

  virtual int GetAudioStreamCount();
  virtual int GetAudioStream();
  virtual void GetAudioStreamName(int iStream, CStdString& strStreamName);
  virtual void SetAudioStream(int iStream);

  virtual bool CanSeek();
  virtual void SeekTime(__int64 iTime = 0);
  virtual int GetTotalTime();
  virtual __int64 GetTime();
  virtual void ToFFRW(int iSpeed = 0);
  virtual void DoAudioWork();

  virtual bool IsCaching() const {return m_bCaching;};
  virtual int GetCacheLevel() const {return m_CacheLevel;};

  virtual bool GetCurrentSubtitle(CStdString& strSubtitle);
  virtual bool OnAction(const CAction &action);

  CStdString _SubtitleExtension;
protected:
  int GetCacheSize(bool bFileOnHD, bool bFileOnISO, bool bFileOnUDF, bool bFileOnInternet, bool bFileOnLAN, bool bIsVideo, bool bIsAudio, bool bIsDVD);
  CStdString GetDVDArgument(const CStdString& strFile);
  void WaitOnCommand();
  bool load();
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();
  void SeekRelativeTime(int iSeconds = 0);
  bool m_bPaused;
  bool m_bIsPlaying;
  bool m_bCaching;
  bool m_bUseFullRecaching;
  int m_CacheLevel;
  float m_fAVDelay;
  DllLoader* m_pDLL;
  __int64 m_iPTS;
  Options options;
  bool m_bSubsVisibleTTF;
  bool m_bIsMplayeropenfile;
  CEvent m_evProcessDone;
  CEdl m_Edl;
};
