#pragma once
#include "iplayer.h"
#include "dllLoader/dll.h"
#include "mplayer/mplayer.h"
#include "stdstring.h"
#include "../utils/thread.h"
#include "../utils/event.h"

#include <vector>
#include <string>
using namespace std;
class CMPlayer : public IPlayer, public CThread
{
public:
  class Options
  {
  public:
    Options();
    bool         GetNonInterleaved() const;
    void         SetNonInterleaved(bool bOnOff) ;

    float        GetVolumeAmplification() const;
    void         SetVolumeAmplification(float fDB) ;

    int          GetChannels() const;
    void         SetChannels(int iChannels);

    bool         GetAC3PassTru();
    void         SetAC3PassTru(bool bOnOff);

    const string GetChannelMapping() const;
    void         SetChannelMapping(const string& strMapping);
    
    void         GetOptions(int& argc, char* argv[]);

    private:
      int     m_iChannels;
      bool    m_bAC3PassTru;
      float   m_fVolumeAmplification;
      bool    m_bNonInterleaved;
      string  m_strChannelMapping;
      vector<string> m_vecOptions;

  };
	CMPlayer(IPlayerCallback& callback);
	virtual ~CMPlayer();
	virtual void		RegisterAudioCallback(IAudioCallback* pCallback);
	virtual void		UnRegisterAudioCallback();
	virtual bool		openfile(const CStdString& strFile);
	virtual bool		closefile();
	virtual bool		IsPlaying() const;
	virtual void		Pause();
	virtual bool		IsPaused() const;
	virtual void		Unload();
	virtual bool		HasVideo();
	virtual bool		HasAudio();
	virtual void		ToggleOSD();
	virtual void		SwitchToNextLanguage();

	virtual __int64	GetPTS() ;
	virtual void		ToggleSubtitles();
	virtual void		ToggleFrameDrop();
	virtual void		SubtitleOffset(bool bPlus=true);
	virtual void	  Seek(bool bPlus=true, bool bLargeStep=false);
	virtual void		SetVolume(bool bPlus=true);	
	virtual void		SetContrast(bool bPlus=true);	
	virtual void		SetBrightness(bool bPlus=true);	
	virtual void		SetHue(bool bPlus=true);	
	virtual void		SetSaturation(bool bPlus=true);	
	virtual void		GetAudioInfo( CStdString& strAudioInfo);
	virtual void		GetVideoInfo( CStdString& strVideoInfo);
	virtual void		GetGeneralInfo( CStdString& strVideoInfo);
	virtual void		Update(bool bPauseDrawing=false);
	virtual void		GetVideoRect(RECT& SrcRect, RECT& DestRect);
	virtual void		GetVideoAspectRatio(float& fAR);
	virtual void		AudioOffset(bool bPlus=true);
	virtual void		SwitchToNextAudioLanguage();
	virtual void		UpdateSubtitlePosition();
	virtual void		RenderSubtitles();
	virtual bool		CanRecord() ;
	virtual bool		IsRecording();
	virtual bool		Record(bool bOnOff) ;
  virtual void    SeekPercentage(int iPercent=0);
  virtual int     GetPercentage();
  virtual void    SetAVDelay(float fValue=0.0f);
  virtual float   GetAVDelay();
  virtual void    SetSubTittleDelay(float fValue=0.0f);
  virtual float   GetSubTitleDelay();
protected:
  bool                load();
	virtual void				OnStartup();
	virtual void				OnExit();
	virtual void				Process();
	bool								m_bPaused;
	bool								m_bIsPlaying;
	DllLoader*					m_pDLL;
	__int64							m_iPTS;
};