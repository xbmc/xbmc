// Most of this based on playsid

#include "stdafx.h"
#include "sidplayer.h"
#include "../utils/log.h"
#include <sidplay/builders/resid.h>
#include "../SectionLoader.h"
#include "../util.h"
#include "../FileSystem/File.h"

#pragma comment(linker,"/merge:SID_RD=SID_RX")
#pragma comment(linker,"/section:SID_RX,REN")
#pragma comment(linker,"/section:SID_RW,RWN")

#include <crtdbg.h>

static const char RESID_ID[] = "ReSID";

static IAudioCallback* m_pCallback=NULL;

#ifdef _DEBUG
_CrtMemState m_MemStateStart;
_CrtMemState m_MemStateEnd;
#endif // _DEBUG

SidPlayer::SidPlayer(IPlayerCallback& callback) :IPlayer(callback), m_name("SidPlay"), m_tune(0), Event("External Timer")
{
#ifdef _DEBUG
	if (m_MemStateStart.pBlockHeader)
	{
		_CrtMemCheckpoint(&m_MemStateEnd);

		_CrtMemState diff;
		_CrtMemDifference(&diff, &m_MemStateStart, &m_MemStateEnd);
		_CrtMemDumpStatistics(&diff);
	}

	_CrtMemCheckpoint(&m_MemStateStart);
#endif // _DEBUG

	CSectionLoader::Load("SID_RX");
	CSectionLoader::Load("SID_RW");
	// WTF half my data is getting merged into here by the stupid VC linker
	CSectionLoader::Load("PYTHON");

	m_bIsPlaying=false;
	m_bPaused=false;

	m_state = playerStopped;
	m_context = NULL;
	m_quietLevel = 0;
	m_verboseLevel = 0;

	m_filename       = NULL;
	m_filter.enabled = true;
	m_driver.device  = NULL;
	m_timer.start    = 0;
	m_timer.length   = 0; // FOREVER
	m_timer.valid    = false;
	m_track.first    = 0;
	m_track.selected = 0;
	m_track.loop     = false;
	m_track.single   = false;
	m_speed.current  = 1;
	m_speed.max      = 32;

	// Read default configuration
	m_engCfg = m_engine.config ();

	// Load ini settings
	IniConfig::audio_section     audio     = m_iniCfg.audio();
	IniConfig::emulation_section emulation = m_iniCfg.emulation();

	// INI Configuration Settings
	m_engCfg.clockForced  = emulation.clockForced;
	m_engCfg.clockSpeed   = emulation.clockSpeed;
	m_engCfg.clockDefault = SID2_CLOCK_CORRECT;
	m_engCfg.frequency    = audio.frequency;
	m_engCfg.optimisation = emulation.optimiseLevel;
	m_engCfg.playback     = audio.playback;
	m_engCfg.precision    = audio.precision;
	m_engCfg.sidModel     = emulation.sidModel;
	m_engCfg.sidDefault   = SID2_MOS6581;
	m_engCfg.sidSamples   = emulation.sidSamples;
	m_filter.enabled      = emulation.filter;

	// Copy default setting to audio configuration
	m_driver.cfg.channels = 1; // Mono
	if (m_engCfg.playback == sid2_stereo)
		m_driver.cfg.channels = 2;
	m_driver.cfg.frequency = m_engCfg.frequency;
	m_driver.cfg.precision = m_engCfg.precision;

	createOutput(OUT_NULL, NULL);
	createSidEmu(EMU_NONE);

	InitializeCriticalSection(&m_CS);
}

SidPlayer::~SidPlayer()
{
	closefile();

	m_driver.null.close();

	DeleteCriticalSection(&m_CS);

	CSectionLoader::Unload("SID_RX");
	CSectionLoader::Unload("SID_RW");
	CSectionLoader::Unload("PYTHON");
}

void SidPlayer::displayError(const char *error)
{
	CLog::Log("%s: %s\n", m_name, error);
}

// Create the output object to process sound buffer
bool SidPlayer::createOutput (OUTPUTS driver, const SidTuneInfo *tuneInfo)
{
	char *name = NULL;

	// Remove old audio driver
	m_driver.null.close ();
	m_driver.selected = &m_driver.null;
	if (m_driver.device != NULL)
	{
		if (m_driver.device != &m_driver.null)
			delete m_driver.device;
		m_driver.device = NULL;         
	}

	// Create audio driver
	switch (driver)
	{
	case OUT_NULL:
		m_driver.device = &m_driver.null;
		break;

	case OUT_SOUNDCARD:
		m_driver.device = new AudioDriver;
		break;

	default:
		break;
	}

	// Audio driver failed
	if (!m_driver.device)
	{
		m_driver.device = &m_driver.null;
		displayError("Out of memory.");
		return false;
	}

	// Configure with user settings
	m_driver.cfg.frequency = m_engCfg.frequency;
	m_driver.cfg.precision = m_engCfg.precision;
	m_driver.cfg.channels  = 1; // Mono
	if (m_engCfg.playback == sid2_stereo)
		m_driver.cfg.channels = 2;

	{   // Open the hardware
		bool err = false;
		if (m_driver.device->open (m_driver.cfg, "") == NULL)
			err = true;
		// Can't open the same driver twice
		if (driver != OUT_NULL)
		{
			if (m_driver.null.open (m_driver.cfg, "") == NULL)
				err = true;;
		}
		if (name != NULL)
			delete [] name;
		if (err)
			return false;
	}

	// See what we got
	m_engCfg.frequency = m_driver.cfg.frequency;
	m_engCfg.precision = m_driver.cfg.precision;
	switch (m_driver.cfg.channels)
	{
	case 1:
		if (m_engCfg.playback == sid2_stereo)
			m_engCfg.playback  = sid2_mono;
		break;
	case 2:
		if (m_engCfg.playback != sid2_stereo)
			m_engCfg.playback  = sid2_stereo;
		break;
	default:
//		cerr << m_name << ": " << "ERROR: " << m_driver.cfg.channels
//			<< " audio channels not supported" << endl;
		return false;
	}
	return true;
}


// Create the sid emulation
bool SidPlayer::createSidEmu (SIDEMUS emu)
{
	// Remove old driver and emulation
	if (m_engCfg.sidEmulation)
	{
		sidbuilder *builder   = m_engCfg.sidEmulation;
		m_engCfg.sidEmulation = NULL;
		m_engine.config (m_engCfg);
		delete builder;
	}

	// Now setup the sid emulation
	switch (emu)
	{
#ifdef HAVE_RESID_BUILDER
		case EMU_RESID:
			{
//#ifdef HAVE_EXCEPTIONS
//				ReSIDBuilder *rs = new(std::nothrow) ReSIDBuilder( RESID_ID );
//#else
				ReSIDBuilder *rs = new ReSIDBuilder( RESID_ID );
//#endif
				if (rs)
				{
					m_engCfg.sidEmulation = rs;
					if (!*rs) goto createSidEmu_error;
				}

				// Setup the emulation
				rs->create ((m_engine.info ()).maxsids);
				if (!*rs) goto createSidEmu_error;
				rs->filter (m_filter.enabled);
				if (!*rs) goto createSidEmu_error;
				rs->sampling (m_driver.cfg.frequency);
				if (!*rs) goto createSidEmu_error;
				if (m_filter.enabled && m_filter.definition)
				{   // Setup filter
					rs->filter (m_filter.definition.provide ());
					if (!*rs) goto createSidEmu_error;
				}
				break;
			}
#endif // HAVE_RESID_BUILDER

		default:
			// Emulation Not yet handled
			// This default case results in the default
			// emulation
			break;
	}

	if (!m_engCfg.sidEmulation)
	{
		if (emu > EMU_DEFAULT)
		{   // No sid emulation?
			displayError ("Out of memory.");
			return false;
		}
	}
	return true;

createSidEmu_error:
	displayError (m_engCfg.sidEmulation->error ());
	delete m_engCfg.sidEmulation;
	m_engCfg.sidEmulation = NULL;
	return false;
}

bool SidPlayer::starttrack()
{
	if ((m_state & ~playerFast) == playerRestart)
	{
		if (m_state & playerFast)
			m_driver.selected->reset ();
		m_state = playerStopped;
	}

	m_track.selected = m_tune.selectSong (m_track.selected);

	// Select the required song
	if (m_engine.load (&m_tune) < 0)
	{
		displayError (m_engine.error ());
		return false;
	}

	// Get tune details
	const SidTuneInfo *tuneInfo;
	tuneInfo = (m_engine.info ()).tuneInfo;
	if (!m_track.single)
		m_track.songs = tuneInfo->songs;
	if (!createOutput (m_driver.output, tuneInfo))
		return false;
	if (!createSidEmu (m_driver.sid))
		return false;

	// Configure engine with settings
	if (m_engine.config (m_engCfg) < 0)
	{   // Config failed
		displayError (m_engine.error ());
		return false;
	}

	// Start the player.  Do this by fast
	// forwarding to the start position
	m_driver.selected = &m_driver.null;
	m_speed.current   = m_speed.max;
	m_engine.fastForward (100 * m_speed.current);

	// As yet we don't have a required songlength
	// so try the songlength database
	if (!m_timer.valid)
	{
		int_least32_t length = m_database.length (m_tune);
		if (length > 0)
			m_timer.length = length;
	}

	// Set up the play timer
	m_context = (m_engine.info()).eventContext;
	m_timer.stop  = 0;
	m_timer.stop += m_timer.length;

	if (m_timer.valid)
	{   // Length relative to start
		m_timer.stop += m_timer.start;
	}
	else
	{   // Check to make start time dosen't exceed end
		if (m_timer.stop & (m_timer.start >= m_timer.stop))
		{
			displayError ("ERROR: Start time exceeds song length!");
			return false;
		}
	}

	m_track.loop = false;

	m_timer.current = ~0;
	m_state = playerRunning;

	return true;
}

bool SidPlayer::openfile(const CStdString& strFile)
{
	closefile();

	if (!CUtil::IsHD(strFile))
	{
		CFile file;
		if (!file.Cache(strFile.c_str(),"Z:\\cachedsid",NULL,NULL))
		{
			::DeleteFile("Z:\\cachedsid");
			CLog::Log("ModPlayer: Unable to cache file %s\n", strFile.c_str());
			return false;
		}
		m_filename = strdup("Z:\\cachedsid");
	}
	else
		m_filename = strdup(strFile.c_str());

	m_driver.output = OUT_SOUNDCARD;
	m_driver.file   = false;
	m_driver.sid    = EMU_RESID;

	// Load the tune
	m_tune.load (m_filename);
	if (!m_tune)
	{
		displayError ((m_tune.getInfo ()).statusString);
		return false;
	}

	// Select the desired track
	m_track.first    = m_tune.selectSong (m_track.first);
	m_track.selected = m_track.first;
	if (m_track.single)
		m_track.songs = 1;

	// If user provided no time then load songlength database
	// and set default lengths incase it's not found in there.
	{
		if (m_driver.file && m_timer.valid && !m_timer.length)
		{   // Time of 0 provided for wav generation
			displayError ("ERROR: -t0 invalid in record mode");
			return false;
		}
		if (!m_timer.valid)
		{
			const char *database = (m_iniCfg.sidplay2()).database;
			m_timer.length = (m_iniCfg.sidplay2()).playLength;
			if (m_driver.file)
				m_timer.length = (m_iniCfg.sidplay2()).recordLength;
			if (database && (*database != '\0'))
			{   // Try loading the database specificed by the user
				if (m_database.open (database) < 0)
				{
					displayError (m_database.error ());
					return false;
				}
			}
		}
	}

	if (!m_filter.definition)
		m_filter.definition = m_iniCfg.filter (m_engCfg.sidModel);

	// Configure engine with settings
	if (m_engine.config (m_engCfg) < 0)
	{   // Config failed
		displayError (m_engine.error ());
		return false;
	}

	if (!starttrack())
		return false;

	m_bIsPlaying=true;
	m_bPaused=false;
	m_bStopPlaying=false;
	
	if ( ThreadHandle() == NULL)
	{
		Create();
	}

	return true;
}

bool SidPlayer::closefile()
{
	m_bStopPlaying=true;
	StopThread();

	m_engine.stop();

	// Shutdown drivers, etc
	createOutput    (OUT_NULL, NULL);
	createSidEmu    (EMU_NONE);
	m_engine.load   (NULL);
	m_engine.config (m_engCfg);

	if (m_filename)
	{
		free(m_filename);
		m_filename = NULL;
	}
	
	return true;
}

bool SidPlayer::IsPlaying() const
{
	return m_bIsPlaying;
}


void  SidPlayer::Pause()
{
	if (!m_bIsPlaying) return;
	m_bPaused=!m_bPaused;
}

bool SidPlayer::IsPaused() const
{
	return m_bPaused;
}

#ifdef _DEBUG
static DWORD AveUpdate = 0;
static DWORD NumUpdates = 0;
static DWORD LastUpdate = 0;
#endif // _DEBUG

__int64	SidPlayer::GetPTS()
{
	if (!m_bIsPlaying) return 0;
	return m_PTS;
}

bool SidPlayer::HasVideo()
{
	return false;
}

bool SidPlayer::HasAudio()
{
	return true;
}

void SidPlayer::ToggleOSD()
{
}

void SidPlayer::SwitchToNextLanguage()
{
}

void SidPlayer::ToggleSubtitles()
{
}


void SidPlayer::SubtitleOffset(bool bPlus)
{
}

void SidPlayer::Seek(bool bPlus, bool bLargeStep)
{
	EnterCriticalSection(&m_CS);

	if (bPlus)
	{
		m_track.selected++;
		if (m_track.selected > m_track.songs)
			m_track.selected = 1;
	}
	else
	{
		m_track.selected--;
		if (m_track.selected < 1)
			m_track.selected = m_track.songs;
	}
	m_state = playerRestart;

	LeaveCriticalSection(&m_CS);
}

void SidPlayer::ToggleFrameDrop()
{
}


void SidPlayer::SetVolume(bool bPlus)
{
}

void SidPlayer::SetContrast(bool bPlus)
{
}

void SidPlayer::SetBrightness(bool bPlus)
{
}
void SidPlayer::SetHue(bool bPlus)
{
}
void SidPlayer::SetSaturation(bool bPlus)
{
}



void SidPlayer::GetAudioInfo( CStdString& strAudioInfo)
{
}

void SidPlayer::GetVideoInfo( CStdString& strVideoInfo)
{
}


void SidPlayer::GetGeneralInfo( CStdString& strVideoInfo)
{
}

void SidPlayer::Update(bool bPauseDrawing)
{
}

void SidPlayer::OnStartup()
{
	m_callback.OnPlayBackStarted();
}

void SidPlayer::OnExit()
{
	m_bIsPlaying = false;
	if (!m_bStopPlaying)
		m_callback.OnPlayBackEnded();
}

void SidPlayer::Process()
{
	do
	{
		if ((m_state & ~playerFast) == playerRestart)
			starttrack();

		event();
		while (!m_bStopPlaying)
		{
			EnterCriticalSection(&m_CS);

			void *buffer = m_driver.selected->buffer ();
			uint_least32_t length = m_driver.cfg.bufSize;

			if (m_state == playerRunning)
			{
				// Fill buffer
				uint_least32_t ret;
				ret = m_engine.play (buffer, length);
				if (ret < length)
				{
					if (m_engine.state () != sid2_stopped)
					{
						m_state = playerError;
						break;
					}
					break;
				}
			}

			switch (m_state)
			{
			case playerRunning:
				m_driver.selected->write();
				// Deliberate run on
			case playerPaused:
				LeaveCriticalSection(&m_CS);
				DirectSoundDoWork();
				Sleep(50);
				continue;
			default:
				LeaveCriticalSection(&m_CS);
				break;
			}

			break;
		}
		EnterCriticalSection(&m_CS);
		((AudioDriver*)m_driver.selected)->Eof();
		LeaveCriticalSection(&m_CS);
	} while (!m_bStopPlaying && (m_state & ~playerFast) == playerRestart);
}

void SidPlayer::event (void)
{
	uint_least32_t seconds = m_engine.time() / 10;
	m_PTS = m_engine.time();

	if (seconds != m_timer.current)
	{
		if (seconds == m_timer.start)
		{   // Switch audio drivers.
			m_driver.selected = m_driver.device;
			m_speed.current = 1;
			m_engine.fastForward (100);
			m_engine.debug (true);
		}
		else if (m_timer.stop && (seconds == m_timer.stop))
		{
			m_state = playerExit;
			for (;;)
			{
				if (m_track.single)
					break;
				// Move to next track
				m_track.selected++;
				if (m_track.selected > m_track.songs)
					m_track.selected = 1;
				if (m_track.selected == m_track.first)
					break;
				m_state = playerRestart;
				break;
			}
			if (m_track.loop)
				m_state = playerRestart;
		}
		m_timer.current = seconds;
	}

	// Units in C64 clock cycles
	m_context->schedule (this, 900000);
}

void SidPlayer::RegisterAudioCallback(IAudioCallback* pCallback)
{
	m_pCallback=pCallback;
	if (m_bIsPlaying)
		m_pCallback->OnInitialize(2, 44100, 16);
}

void SidPlayer::UnRegisterAudioCallback()
{
	m_pCallback=NULL;
}
