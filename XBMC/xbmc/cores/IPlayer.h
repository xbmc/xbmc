#pragma once
#include "stdstring.h"
#include "IAudioCallback.h"
class IPlayerCallback
{
public:
	virtual void OnPlayBackEnded()=0;
	virtual void OnPlayBackStarted()=0;
};

class IPlayer
{
public:
	IPlayer(IPlayerCallback& callback):m_callback(callback){};
	virtual ~IPlayer(){};
	virtual void		RegisterAudioCallback(IAudioCallback* pCallback)=0;
	virtual void		UnRegisterAudioCallback()=0;
	virtual bool		openfile(const CStdString& strFile){return false;};
	virtual bool		closefile(){return true;};
	virtual bool		IsPlaying() const {return false;} ;
	virtual void		Pause()=0;
	virtual bool		IsPaused() const=0;
	virtual bool		HasVideo()=0;
	virtual bool		HasAudio()=0;
	virtual void		ToggleOSD()=0;
	virtual void		SwitchToNextLanguage()=0;
	virtual __int64	GetPTS() =0;
	virtual void		ToggleSubtitles()=0;
	virtual void		ToggleFrameDrop()=0;
	virtual void		SubtitleOffset(bool bPlus=true)=0;	
	virtual void	  Seek(bool bPlus=true, bool bLargeStep=false)=0;
  virtual void    SeekPercentage(int iPercent=0){};
  virtual int     GetPercentage(){return 0;};
	virtual void		SetVolume(bool bPlus=true)=0;	
	virtual void		SetContrast(bool bPlus=true)=0;	
	virtual void		SetBrightness(bool bPlus=true)=0;	
	virtual void		SetHue(bool bPlus=true)=0;	
	virtual void		SetSaturation(bool bPlus=true)=0;	
	virtual void		GetAudioInfo( CStdString& strAudioInfo)=0;
	virtual void		GetVideoInfo( CStdString& strVideoInfo)=0;
	virtual void		GetGeneralInfo( CStdString& strVideoInfo)=0;
	virtual void		Update(bool bPauseDrawing=false)=0;
	virtual void		GetVideoRect(RECT& SrcRect, RECT& DestRect)=0;
	virtual void		GetVideoAspectRatio(float& fAR)=0;
	virtual void		AudioOffset(bool bPlus=true){};
	virtual void		SwitchToNextAudioLanguage(){};
	virtual void		UpdateSubtitlePosition(){};
	virtual void		RenderSubtitles(){};
	virtual bool		CanRecord() {return false;};
	virtual bool		IsRecording() {return false;};
	virtual bool		Record(bool bOnOff) {return false;};
  virtual void    SetAVDelay(float fValue=0.0f){};
  virtual float   GetAVDelay(){return 0.0f;};
  virtual void    SetSubTittleDelay(float fValue=0.0f){};
  virtual float   GetSubTitleDelay(){return 0.0f;};
  virtual int     GetAudioStreamCount(){return 1;};
  virtual void	  SeekTime(int iTime=0){};
  virtual int	  GetTime(){return 0;};
  virtual void	  ToFFRW(int iSpeed=0){};
protected:
	IPlayerCallback& m_callback;
};