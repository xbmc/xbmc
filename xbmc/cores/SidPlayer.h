#pragma once

#include "../lib/libsidplay/libsidplay/include/sidplay/sidplay2.h"
#include <sidplay/utils/SidDatabase.h>
#include "sidplayer/audio/AudioDrv.h"
#include "sidplayer/IniConfig.h"
#include "iplayer.h"
#include "../utils/thread.h"

class SidPlayer : public IPlayer, public CThread, private Event
{
public:
	SidPlayer(IPlayerCallback& callback);
	virtual ~SidPlayer();
	virtual void		RegisterAudioCallback(IAudioCallback* pCallback);
	virtual void		UnRegisterAudioCallback();

	virtual bool		OpenFile(const CFileItem& file, __int64 iStartTime);
	virtual bool		CloseFile();
	virtual bool		IsPlaying() const;
	virtual void		Pause();
	virtual bool		IsPaused() const;
	virtual bool		HasVideo();
	virtual bool		HasAudio();
	virtual void		ToggleOSD();
	virtual void		SwitchToNextLanguage();
	virtual __int64 GetTime();
	virtual void		ToggleSubtitles();
	virtual void		ToggleFrameDrop();
	virtual void		SubtitleOffset(bool bPlus=true);	
	virtual void	  Seek(bool bPlus=true, bool bLargeStep=false);
	virtual void		SetVolume(long nVolume);	
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

protected:
	virtual void		OnStartup();
	virtual void		OnExit();
	virtual void		Process();

	enum player_state_t {
		playerError = 0, playerRunning, playerPaused, playerStopped,
		playerRestart, playerExit, playerFast = 128,
		playerFastRestart = playerRestart | playerFast,
		playerFastExit = playerExit | playerFast
	};

	enum SIDEMUS {
		EMU_NONE = 0,
		/* The following require a soundcard */
		EMU_DEFAULT, EMU_RESID, EMU_SIDPLAY1,
		/* The following should disable the soundcard */
		EMU_HARDSID, EMU_SIDSTATION, EMU_COMMODORE,
		EMU_SIDSYN, EMU_END
	};

	enum OUTPUTS {/* Define possible output sources */
		OUT_NULL = 0,
		/* Hardware */
		OUT_SOUNDCARD,
		/* File creation support */
		OUT_WAV, OUT_AU, OUT_END
	};

	const char* const  m_name; 
	sidplay2           m_engine;
	sid2_config_t      m_engCfg;
	SidTuneMod         m_tune;
	player_state_t     m_state;
	EventContext      *m_context;
	char*							 m_filename;

	IniConfig          m_iniCfg;
	SidDatabase        m_database;

	// Display parameters
	uint_least8_t      m_quietLevel;
	uint_least8_t      m_verboseLevel;

	struct m_filter_t
	{
		SidFilter      definition;
		bool           enabled;
	} m_filter;

	struct m_driver_t
	{
		OUTPUTS        output;   // Selected output type
		SIDEMUS        sid;      // Sid emulation
		bool           file;     // File based driver
		AudioConfig    cfg;
		AudioBase*     selected; // Selected Output Driver
		AudioBase*     device;   // HW/File Driver
		Audio_Null     null;     // Used for everything
	} m_driver;

	struct m_timer_t
	{   // secs
		uint_least32_t start;
		uint_least32_t current;
		uint_least32_t stop;
		uint_least32_t length;
		bool           valid;
	} m_timer;

	struct m_track_t
	{
		uint_least16_t first;
		uint_least16_t selected;
		uint_least16_t songs;
		bool           loop;
		bool           single;
	} m_track;

	struct m_speed_t
	{
		uint_least8_t current;
		uint_least8_t max;
	} m_speed;

	bool createOutput(OUTPUTS driver, const SidTuneInfo *tuneInfo);
	bool createSidEmu(SIDEMUS emu);
	void displayError (const char *error);
	void event(void);
	bool starttrack();

	bool						m_bPaused;
	bool						m_bIsPlaying;
	bool						m_bStopPlaying;
	__int64					m_PTS;
	CRITICAL_SECTION	m_CS;
};