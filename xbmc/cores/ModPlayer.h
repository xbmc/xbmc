#pragma once
#include "stdstring.h"
#include "iplayer.h"
#include "../utils/thread.h"
#include "../utils/event.h"
#include "../lib/mikxbox/mikmod.h"
#include "../lib/mikxbox/mikxbox.h"

class ModPlayer : public IPlayer, public CThread
{
public:
	ModPlayer(IPlayerCallback& callback);
	virtual ~ModPlayer();
	virtual void		RegisterAudioCallback(IAudioCallback* pCallback);
	virtual void		UnRegisterAudioCallback();

	virtual bool		openfile(const CStdString& strFile);
	virtual bool		closefile();
	virtual bool		IsPlaying() const;
	virtual void		Pause();
	virtual bool		IsPaused() const;
	virtual bool		HasVideo();
	virtual bool		HasAudio();
	virtual void		ToggleOSD();
	virtual void		SwitchToNextLanguage();
	virtual __int64	GetPTS();
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
	virtual void    Update(bool bPauseDrawing=false);
	virtual void		GetVideoRect(RECT& SrcRect, RECT& DestRect){};
	virtual void		GetVideoAspectRatio(float& fAR) {};

	static bool     IsSupportedFormat(const CStdString& strFmt);

protected:
	virtual void		OnStartup();
	virtual void		OnExit();
	virtual void		Process();

	MODULE*					m_pModule;
	bool						m_bPaused;
	bool						m_bIsPlaying;
	bool						m_bStopPlaying;
};