#include "stdafx.h"
#include "mplayer.h"
#include "mplayer/mplayer.h"
#include "../util.h"
#include "../utils/log.h"
#include "../settings.h"
#include "../url.h"
#include "../filesystem/fileshoutcast.h"
#include "EMUkernel32.h"
#include "dlgcache.h"
#include "../FileSystem/FileSmb.h"
#include "../FileSystem/File.h"

#define KEY_ENTER 13
#define KEY_TAB 9

#define KEY_BASE 0x100

/*  Function keys  */
#define KEY_F (KEY_BASE+64)

/* Control keys */
#define KEY_CTRL (KEY_BASE)
#define KEY_BACKSPACE (KEY_CTRL+0)
#define KEY_DELETE (KEY_CTRL+1)
#define KEY_INSERT (KEY_CTRL+2)
#define KEY_HOME (KEY_CTRL+3)
#define KEY_END (KEY_CTRL+4)
#define KEY_PAGE_UP (KEY_CTRL+5)
#define KEY_PAGE_DOWN (KEY_CTRL+6)
#define KEY_ESC (KEY_CTRL+7)

/* Control keys short name */
#define KEY_BS KEY_BACKSPACE
#define KEY_DEL KEY_DELETE
#define KEY_INS KEY_INSERT
#define KEY_PGUP KEY_PAGE_UP
#define KEY_PGDOWN KEY_PAGE_DOWN
#define KEY_PGDWN KEY_PAGE_DOWN

/* Cursor movement */
#define KEY_CRSR (KEY_BASE+16)
#define KEY_RIGHT (KEY_CRSR+0)
#define KEY_LEFT (KEY_CRSR+1)
#define KEY_DOWN (KEY_CRSR+2)
#define KEY_UP (KEY_CRSR+3)

/* XF86 Multimedia keyboard keys */
#define KEY_XF86_BASE (0x100+384)
#define KEY_XF86_PAUSE (KEY_XF86_BASE+1)
#define KEY_XF86_STOP (KEY_XF86_BASE+2)
#define KEY_XF86_PREV (KEY_XF86_BASE+3)
#define KEY_XF86_NEXT (KEY_XF86_BASE+4)

/* Keypad keys */
#define KEY_KEYPAD (KEY_BASE+32)
#define KEY_KP0 (KEY_KEYPAD+0)
#define KEY_KP1 (KEY_KEYPAD+1)
#define KEY_KP2 (KEY_KEYPAD+2)
#define KEY_KP3 (KEY_KEYPAD+3)
#define KEY_KP4 (KEY_KEYPAD+4)
#define KEY_KP5 (KEY_KEYPAD+5)
#define KEY_KP6 (KEY_KEYPAD+6)
#define KEY_KP7 (KEY_KEYPAD+7)
#define KEY_KP8 (KEY_KEYPAD+8)
#define KEY_KP9 (KEY_KEYPAD+9)
#define KEY_KPDEC (KEY_KEYPAD+10)
#define KEY_KPINS (KEY_KEYPAD+11)
#define KEY_KPDEL (KEY_KEYPAD+12)
#define KEY_KPENTER (KEY_KEYPAD+13)

//Transforms a string into a language code used by dvd's
#define DVDLANGCODE(x) ((int)(x[1]|(x[0]<<8)))

void xbox_audio_do_work();
void xbox_audio_wait_completion();
void audio_pause();
void audio_resume();

extern "C" void free_registry(void);
extern void xbox_video_wait();
extern void xbox_video_CheckScreenSaver();	// Screensaver check
extern CFileShoutcast* m_pShoutCastRipper;
extern "C" void dllReleaseAll( );

const char * dvd_audio_stream_types[8] =
{ "ac3","unknown","mpeg1","mpeg2ext","lpcm","unknown","dts" };

const char * dvd_audio_stream_channels[6] =
{ "mono", "stereo", "unknown", "unknown", "5.1/6.1", "5.1" };

#define DVDLANGUAGES 144
const char * dvd_audio_stream_langs[DVDLANGUAGES][2] = 
{ 	{"--", "(Not detected)"},
	{"cc", "Closed Caption"},
	{"aa", "Afar"},
	{"ab", "Abkhazian"},
	{"af", "Afrikaans"},
	{"am", "Amharic"},
	{"ar", "Arabic"},
	{"as", "Assamese"},
	{"ay", "Aymara"},
	{"az", "Azerbaijani"},
	{"ba", "Bashkir"},
	{"be", "Byelorussian"},
	{"bg", "Bulgarian"},
	{"bh", "Bihari"},
	{"bi", "Bislama"},
	{"bn", "Bengali; Bangla"},
	{"bo", "Tibetan"},
	{"br", "Breton"},
	{"ca", "Catalan"},
	{"co", "Corsican"},
	{"cs", "Czech"},
	{"cy", "Welsh"},
	{"da", "Dansk"},
	{"de", "Deutsch"},
	{"dz", "Bhutani"},
	{"el", "Greek"},
	{"en", "English"},
	{"eo", "Esperanto"},
	{"es", "Espanol"},
	{"et", "Estonian"},
	{"eu", "Basque"},
	{"fa", "Persian"},
	{"fi", "Finnish"},
	{"fj", "Fiji"},
	{"fo", "Faroese"},
	{"fr", "Francais"},
	{"fy", "Frisian"},
	{"ga", "Irish"},
	{"gd", "Scots Gaelic"},
	{"gl", "Galician"},
	{"gn", "Guarani"},
	{"gu", "Gujarati"},
	{"ha", "Hausa"},
	{"he", "Hebrew"},
	{"hi", "Hindi"},
	{"hr", "Hrvatski"},
	{"hu", "Hungarian"},
	{"hy", "Armenian"},
	{"ia", "Interlingua"},
	{"id", "Indonesian"},
	{"ie", "Interlingue"},
	{"ik", "Inupiak"},
	{"in", "Indonesian"},
	{"is", "Islenska"},
	{"it", "Italiano"},
	{"iu", "Inuktitut"},
	{"iw", "Hebrew"},
	{"ja", "Japanese"},
	{"ji", "Yiddish"},
	{"jw", "Javanese"},
	{"ka", "Georgian"},
	{"kk", "Kazakh"},
	{"kl", "Greenlandic"},
	{"km", "Cambodian"},
	{"kn", "Kannada"},
	{"ko", "Korean"},
	{"ks", "Kashmiri"},
	{"ku", "Kurdish"},
	{"ky", "Kirghiz"},
	{"la", "Latin"},
	{"ln", "Lingala"},
	{"lo", "Laothian"},
	{"lt", "Lithuanian"},
	{"lv", "Latvian, Lettish"},
	{"mg", "Malagasy"},
	{"mi", "Maori"},
	{"mk", "Macedonian"},
	{"ml", "Malayalam"},
	{"mn", "Mongolian"},
	{"mo", "Moldavian"},
	{"mr", "Marathi"},
	{"ms", "Malay"},
	{"mt", "Maltese"},
	{"my", "Burmese"},
	{"na", "Nauru"},
	{"ne", "Nepali"},
	{"nl", "Nederlands"},
	{"no", "Norsk"},
	{"oc", "Occitan"},
	{"om", "(Afan) Oromo"},
	{"or", "Oriya"},
	{"pa", "Punjabi"},
	{"pl", "Polish"},
	{"ps", "Pashto, Pushto"},
	{"pt", "Portugues"},
	{"qu", "Quechua"},
	{"rm", "Rhaeto-Romance"},
	{"rn", "Kirundi"},
	{"ro", "Romanian"},
	{"ru", "Russian"},
	{"rw", "Kinyarwanda"},
	{"sa", "Sanskrit"},
	{"sd", "Sindhi"},
	{"sg", "Sangho"},
	{"sh", "Serbo-Croatian"},
	{"si", "Sinhalese"},
	{"sk", "Slovak"},
	{"sl", "Slovenian"},
	{"sm", "Samoan"},
	{"sn", "Shona"},
	{"so", "Somali"},
	{"sq", "Albanian"},
	{"sr", "Serbian"},
	{"ss", "Siswati"},
	{"st", "Sesotho"},
	{"su", "Sundanese"},
	{"sv", "Svenska"},
	{"sw", "Swahili"},
	{"ta", "Tamil"},
	{"te", "Telugu"},
	{"tg", "Tajik"},
	{"th", "Thai"},
	{"ti", "Tigrinya"},
	{"tk", "Turkmen"},
	{"tl", "Tagalog"},
	{"tn", "Setswana"},
	{"to", "Tonga"},
	{"tr", "Turkish"},
	{"ts", "Tsonga"},
	{"tt", "Tatar"},
	{"tw", "Twi"},
	{"ug", "Uighur"},
	{"uk", "Ukrainian"},
	{"ur", "Urdu"},
	{"uz", "Uzbek"},
	{"vi", "Vietnamese"},
	{"vo", "Volapuk"},
	{"wo", "Wolof"},
	{"xh", "Xhosa"},
	{"yi", "Yiddish"},				// formerly ji
	{"yo", "Yoruba"},
	{"za", "Zhuang"},
	{"zh", "Chinese"},
	{"zu", "Zulu"}
};





bool   m_bCanceling=false;
static CDlgCache* m_dlgCache=NULL;
CMPlayer::Options::Options()
{
	m_bResampleAudio=false;
	m_bNoCache=false;
	m_bNoIdx=false;
	m_iChannels=0;
	m_bAC3PassTru=false;
	m_strChannelMapping="";
	m_fVolumeAmplification=0.0f;
	m_bNonInterleaved=false;
	m_fSpeed=1.0f;

	m_iAudioStream=-1;
	m_iSubtitleStream=-1;

	m_strDvdDevice="";
	m_strFlipBiDiCharset="";
  m_strHexRawAudioFormat="";
}
void  CMPlayer::Options::SetFPS(float fFPS)
{
	m_fFPS=fFPS;
}
float CMPlayer::Options::GetFPS() const
{
	return m_fFPS;
}

bool CMPlayer::Options::GetNoCache() const
{
	return m_bNoCache;
}
void CMPlayer::Options::SetNoCache(bool bOnOff) 
{
	m_bNoCache=bOnOff;
}

bool CMPlayer::Options::GetNoIdx() const
{
	return m_bNoIdx;
}

void CMPlayer::Options::SetNoIdx(bool bOnOff) 
{
	m_bNoIdx=bOnOff;
}

void  CMPlayer::Options::SetSpeed(float fSpeed)
{
	m_fSpeed=fSpeed;
}
float CMPlayer::Options::GetSpeed() const
{
	return m_fSpeed;
}

bool CMPlayer::Options::GetNonInterleaved() const
{
	return m_bNonInterleaved;
}
void CMPlayer::Options::SetNonInterleaved(bool bOnOff)
{
	m_bNonInterleaved=bOnOff;
}


int CMPlayer::Options::GetAudioStream() const
{
	return m_iAudioStream;
}
void CMPlayer::Options::SetAudioStream(int iStream)
{
	m_iAudioStream=iStream;
}

int CMPlayer::Options::GetSubtitleStream() const
{
	return m_iSubtitleStream;
}
void CMPlayer::Options::SetSubtitleStream(int iStream)
{
	m_iSubtitleStream=iStream;
}

float CMPlayer::Options::GetVolumeAmplification() const
{
	return m_fVolumeAmplification;
}

void CMPlayer::Options::SetVolumeAmplification(float fDB) 
{
	if (fDB < -200.0f) fDB=-200.0f;
	if (fDB >   60.0f) fDB=60.0f;
	m_fVolumeAmplification=fDB;
}


int CMPlayer::Options::GetChannels() const
{
	return m_iChannels;
}
void CMPlayer::Options::SetChannels(int iChannels)
{
	m_iChannels=iChannels;
}

bool CMPlayer::Options::GetAC3PassTru()
{
	return m_bAC3PassTru;
}
void CMPlayer::Options::SetAC3PassTru(bool bOnOff)
{
	m_bAC3PassTru=bOnOff;
}

const string CMPlayer::Options::GetChannelMapping() const
{
	return m_strChannelMapping;
}

void CMPlayer::Options::SetChannelMapping(const string& strMapping)
{
	m_strChannelMapping=strMapping;
}

void CMPlayer::Options::SetDVDDevice(const string & strDevice)
{
	m_strDvdDevice=strDevice;
}

void CMPlayer::Options::SetFlipBiDiCharset(const string& strCharset)
{
	m_strFlipBiDiCharset=strCharset;
}

void CMPlayer::Options::SetRawAudioFormat(const string& strHexRawAudioFormat)
{
	m_strHexRawAudioFormat=strHexRawAudioFormat;
}

void CMPlayer::Options::GetOptions(int& argc, char* argv[])
{
	CStdString strTmp;
	m_vecOptions.erase(m_vecOptions.begin(),m_vecOptions.end());
	m_vecOptions.push_back("xbmc.exe");

	// enable direct rendering (mplayer directly draws on xbmc's overlay texture)
	m_vecOptions.push_back("-dr");    
  if (m_strHexRawAudioFormat.length()>0) {
	  m_vecOptions.push_back("-rawaudio");    
    m_vecOptions.push_back("on:format=" + m_strHexRawAudioFormat);//0x2000
  }
	//m_vecOptions.push_back("-verbose");    
	//m_vecOptions.push_back("1");    
	if (g_stSettings.m_mplayerDebug)
	{
		m_vecOptions.push_back("-verbose");    
		m_vecOptions.push_back("1");    
	}

	if (m_bNoCache)
	{
		m_vecOptions.push_back("-nocache");    
	}

	if (m_bNoIdx)
	{
		m_vecOptions.push_back("-noidx");    
	}
	
	//MOVED TO mplayer.conf
	//Enable mplayer's internal highly accurate sleeping.
	//m_vecOptions.push_back("-softsleep");    

	//limit A-V sync correction in order to get smoother playback.
	//defaults to 0.01 but for high quality videos 0.0001 results in 
	// much smoother playback but slow reaction time to fix A-V desynchronization
	//m_vecOptions.push_back("-mc");
	//m_vecOptions.push_back("0.0001");

	// smooth out audio driver timer (audio drivers arent perect)
	//Higher values mean more smoothing,but avoid using numbers too high, 
	//as they will cause independent timing from the sound card and may result in 
	//an A-V desync
	//m_vecOptions.push_back("-autosync");
	//m_vecOptions.push_back("30");

	if (m_fSpeed != 1.0f)
	{
		// set playback speed
		m_vecOptions.push_back("-speed");
		strTmp.Format("%f", m_fSpeed);
		m_vecOptions.push_back(strTmp);
		m_vecOptions.push_back("-fps");
		strTmp.Format("%f", m_fFPS);
		m_vecOptions.push_back(strTmp);

		// set subtitle fps
		m_vecOptions.push_back("-subfps");
		strTmp.Format("%f", m_fFPS);
		m_vecOptions.push_back(strTmp);

	}

	if ( m_iAudioStream >=0)
	{
		CLog::Log(" Playing audio stream: %d", m_iAudioStream);
		m_vecOptions.push_back("-aid");
		strTmp.Format("%i", m_iAudioStream);
		m_vecOptions.push_back(strTmp);
	}

	if ( m_iSubtitleStream >=0)
	{
		CLog::Log(" Playing subtitle stream: %d", m_iSubtitleStream);
		m_vecOptions.push_back("-sid");
		strTmp.Format("%i", m_iSubtitleStream);
		m_vecOptions.push_back(strTmp);
	}
	//MOVED TO mplayer.conf to allow it to be overridden on a per file basis
	//	else
	//	{
		//Force mplayer to allways allocate a subtitle demuxer, otherwise we 
		//might not be able to enable it later. Will make sure later that it isn't visible.
		//For after 1.0 add command to ask for a specific language as an alternative.
		//m_vecOptions.push_back("-sid");
		//m_vecOptions.push_back("0");
	//	}

	if ( m_iChannels) 
	{
		// set number of audio channels
		m_vecOptions.push_back("-channels");
		strTmp.Format("%i", m_iChannels);
		m_vecOptions.push_back(strTmp);
	}
	if ( m_strChannelMapping.size()) 
	{
		// set audio channel mapping
		m_vecOptions.push_back("-af");
		m_vecOptions.push_back(m_strChannelMapping);
	}
	if ( m_bAC3PassTru)
	{
		// this is nice, we can ask mplayer to try hwac3 filter (used for ac3/dts pass-through) first
		// and if it fails try a52 filter (used for ac3 software decoding) and if that fails
		// try the other audio codecs (mp3, wma,...)
		m_vecOptions.push_back("-ac");
		m_vecOptions.push_back("hwac3,a52,");
	}

	if (m_strFlipBiDiCharset.length()>0)
	{
			CLog::Log("Flipping bi-directional subtitles in charset %s", g_stSettings.m_szFlipBiDiCharset);
			m_vecOptions.push_back("-fribidi-charset");
			m_vecOptions.push_back(m_strFlipBiDiCharset);
			m_vecOptions.push_back("-flip-hebrew");
	}
	else
	{
		CLog::Log("Flipping bi-directional subtitles disabled");
		m_vecOptions.push_back("-noflip-hebrew");
	}

	if ( g_stSettings.m_bPostProcessing )
	{
		if (g_stSettings.m_bPPAuto && !g_stSettings.m_bDeInterlace)
		{
			// enable auto quality &postprocessing 
			m_vecOptions.push_back("-autoq");
			m_vecOptions.push_back("100");
			m_vecOptions.push_back("-vop");
			m_vecOptions.push_back("pp");      
		}
		else
		{
			// manual postprocessing
			CStdString strOpt;
			strTmp = "pp=";
			bool bAddComma(false);
			m_vecOptions.push_back("-vop");
			if ( g_stSettings.m_bDeInterlace)
			{
				// add deinterlace filter
				if (bAddComma) strTmp +="/";
				strOpt="ci";
				bAddComma=true;
				strTmp += strOpt;
			}
			if (g_stSettings.m_bPPdering)
			{
				// add dering filter
				if (bAddComma) strTmp +="/";
				if (g_stSettings.m_iPPVertical>0) strOpt.Format("vb:%i",g_stSettings.m_iPPVertical);
				else strOpt="dr";
				bAddComma=true;
				strTmp += strOpt;
			}
			if (g_stSettings.m_bPPVertical)
			{
				// add vertical deblocking filter
				if (bAddComma) strTmp +="/";
				if (g_stSettings.m_iPPVertical>0) strOpt.Format("vb:%i",g_stSettings.m_iPPVertical);
				else strOpt="vb:a";
				bAddComma=true;
				strTmp += strOpt;
			}
			if (g_stSettings.m_bPPHorizontal)
			{
				// add horizontal deblocking filter
				if (bAddComma) strTmp +="/";
				if (g_stSettings.m_iPPHorizontal>0) strOpt.Format("hb:%i",g_stSettings.m_iPPHorizontal);
				else strOpt="hb:a";
				bAddComma=true;

				strTmp += strOpt;
			}
			if (g_stSettings.m_bPPAutoLevels)
			{
				// add auto brightness/contrast levels
				if (bAddComma) strTmp +="/";
				strOpt="al";
				bAddComma=true;
				strTmp += strOpt;
			}
			m_vecOptions.push_back(strTmp);
		}
	}

	if (m_fVolumeAmplification > 0.1f || m_fVolumeAmplification < -0.1f)
	{
		//add volume amplification audio filter
		strTmp.Format("volume=%2.2f:0",m_fVolumeAmplification);
		m_vecOptions.push_back("-af");
		m_vecOptions.push_back(strTmp);
	}
	//  if (m_bResampleAudio)
	//{
	//m_vecOptions.push_back("-af");
	// 48kHz resampling
	// format is: rate=sample_rate:bSloppy:quality
	// where bSloppy is 1 if we don't care about accurate output rate
	// and quality is:
	//     0   - linear interpolation (fast, but bad quality)
	//     1   - polyphase filtered with integer math
	//     2   - polyphase filtered with float math (slowest)
	//	m_vecOptions.push_back("resample=48000:0:1");//,format=2unsignedint");
	//}

	if (m_bNonInterleaved)
	{
		// open file in non-interleaved way
		m_vecOptions.push_back("-ni");
	}

	if(m_strDvdDevice.length()>0)
	{
		CStdString strDevice;
		strDevice = """" + m_strDvdDevice;

		//Make sure we only use forward slashes for path 
		//since mplayer is manly *nix based this causes less problems.
		//Our standard file system handles this aswell.
		strDevice.Replace("\\", "/"); 
		strDevice.TrimRight("/"); 
		strDevice += """";

		m_vecOptions.push_back("-dvd-device");
		m_vecOptions.push_back(strDevice);	
	}
	
	//Only display forced subs for dvds.
	//m_vecOptions.push_back("-forcedsubsonly");

	m_vecOptions.push_back("1.avi");

	argc=(int)m_vecOptions.size();
	for (int i=0; i < argc; ++i)
	{
		argv[i]=(char*)m_vecOptions[i].c_str();
	}
	argv[argc]=NULL;
}


CMPlayer::CMPlayer(IPlayerCallback& callback)
:IPlayer(callback)
{
	criticalsection_head = NULL;
	m_pDLL=NULL;
	m_bIsPlaying=false;
	m_bPaused=false;
}

CMPlayer::~CMPlayer()
{
	m_bIsPlaying=false;
	StopThread();
	Unload();
	while( criticalsection_head )
	{
		CriticalSection_List * entry = criticalsection_head;
		criticalsection_head = entry->Next;
		delete entry;
	}	//fix winnt and xbox critical data mismatch issue.
	free_registry();	//free memory take by registry structures
	smb.Purge(); // close any open smb sessions
}
bool CMPlayer::load()
{
	m_bIsPlaying=false;
	StopThread();
	Unload();
	if (!m_pDLL)
	{
		m_pDLL = new DllLoader("Q:\\mplayer\\mplayer.dll");
		if( !m_pDLL->Parse() )
		{
			CLog::Log("cmplayer::load() parse failed");
			delete m_pDLL;
			m_pDLL=NULL;
			return false;
		}
		if( !m_pDLL->ResolveImports()  )
		{
			CLog::Log("cmplayer::load() resolve imports failed");
		}
		mplayer_load_dll(*m_pDLL);
	}
	return true;
}
void update_cache_dialog(const char* tmp)
{
	if (m_dlgCache)
	{
		m_dlgCache->SetMessage(tmp);
		m_dlgCache->Update();
		if(m_dlgCache->IsCanceled() && !m_bCanceling)
		{
			m_bCanceling = true;
			mplayer_exit_player();
		}
	}
}
bool CMPlayer::openfile(const CStdString& strFile)
{
	int iRet=-1;
	m_bCanceling=false;
	int iCacheSize=1024;
	int iCacheSizeBackBuffer = 20; // 50 % backbuffer is mplayers default
	closefile();
	bool bFileOnHD(false);
	bool bFileOnISO(false);
	bool bFileOnUDF(false);
	bool bFileOnInternet(false);
	bool bFileOnLAN(false);
	bool bFileIsDVDImage(false);
	bool bFileIsDVDIfoFile(false);

	CURL url(strFile);
	if ( CUtil::IsHD(strFile) )           bFileOnHD=true;
	else if ( CUtil::IsISO9660(strFile) ) bFileOnISO=true;
	else if ( CUtil::IsDVD(strFile) )     bFileOnUDF=true;
	else if (url.GetProtocol()=="http")   bFileOnInternet=true;
	else if (url.GetProtocol()=="shout")  bFileOnInternet=true;
	else if (url.GetProtocol()=="mms")    bFileOnInternet=true;
	else if (url.GetProtocol()=="rtp")    bFileOnInternet=true;
	else bFileOnLAN=true;

	bool bIsVideo =CUtil::IsVideo(strFile);
	bool bIsAudio =CUtil::IsAudio(strFile);
	bool bIsDVD(false);

	bFileIsDVDImage = CUtil::IsDVDImage(strFile);
	bFileIsDVDIfoFile = CUtil::IsDVDFile(strFile, false, true);

	CLog::DebugLog("file:%s IsDVDImage:%i IsDVDIfoFile:%i", strFile.c_str(),bFileIsDVDImage ,bFileIsDVDIfoFile);
	if (strFile.Find("dvd://") >=0 || bFileIsDVDImage || bFileIsDVDIfoFile)
	{
		bIsDVD=true;
		bIsVideo=true;
	}

	iCacheSize=GetCacheSize(bFileOnHD,bFileOnISO,bFileOnUDF,bFileOnInternet,bFileOnLAN, bIsVideo, bIsAudio, bIsDVD);
	try
	{
		if (bFileOnInternet)
		{
			m_dlgCache = new CDlgCache();
			m_dlgCache->Update();
		}

		CLog::Log("mplayer play:%s cachesize:%i", strFile.c_str(), iCacheSize);

		// cache (remote) subtitles to HD
		if (!bFileOnInternet && bIsVideo && !bIsDVD)
		{
			CUtil::CacheSubtitles(strFile, _SubtitleExtension);
			CUtil::PrepareSubtitleFonts();
		}
		else
			CUtil::ClearSubtitles();
		m_iPTS			= 0;
		m_bPaused	  = false;

		// first init mplayer. This is needed 2 find out all information
		// like audio channels, fps etc
		load();

		char *argv[30];
		int argc=8;
		//Options options;
		if (CUtil::IsVideo(strFile))
		{
			options.SetNonInterleaved(g_stSettings.m_bNonInterleaved);

		}
		options.SetNoCache(g_stSettings.m_bNoCache);


		if (g_stSettings.m_szFlipBiDiCharset != NULL && strlen(g_stSettings.m_szFlipBiDiCharset) > 0)
		{
			options.SetFlipBiDiCharset(g_stSettings.m_szFlipBiDiCharset);
		}


		bool bSupportsAC3Out=(XGetAudioFlags() & DSSPEAKER_ENABLE_AC3) != 0;
		bool bSupportsDTSOut=(XGetAudioFlags() & DSSPEAKER_ENABLE_DTS) != 0;

		// shoutcast is always stereo
		if (CUtil::IsShoutCast(strFile) ) 
		{
			options.SetChannels(0);
			bSupportsAC3Out = false;
			bSupportsDTSOut = false;
		}
		else 
		{
			// if we are using analog output, then we only got 2 stereo output
			options.SetChannels(0);

			// if we're using digital out & ac3/dts pass-through is enabled
			// then try opening file with ac3/dts pass-through 
			if (g_stSettings.m_bUseDigitalOutput && (bSupportsAC3Out || bSupportsDTSOut))
			{
				if (g_stSettings.m_bDDStereoPassThrough || g_stSettings.m_bDD_DTSMultiChannelPassThrough)
				{
					options.SetChannels(6);
				}
			}
		}

		if (1 /* bIsVideo*/) 
		{
			options.SetVolumeAmplification(g_stSettings.m_fVolumeAmplification);
		}

		//Make sure we set the dvd-device parameter if we are playing dvdimages or dvdfolders
		if(bFileIsDVDImage)
		{
			options.SetDVDDevice(strFile);
			CLog::Log(" dvddevice: %s", strFile.c_str());
		}
		else if(bFileIsDVDIfoFile)
		{
			CStdString strPath;
			CUtil::GetParentPath(strFile, strPath);
			options.SetDVDDevice(strPath);
			CLog::Log(" dvddevice: %s", strPath.c_str());
		}	

 		CStdString strExtension;
		CUtil::GetExtension(strFile,strExtension);
		strExtension.MakeLower();

		CFile* pFile = new CFile();
		__int64 len = 0;
		if (pFile->Open(strFile.c_str(), true))
		{
			len = pFile->GetLength();
		}
		delete pFile;

		if (len > 0x7fffffff)
		{
			if (strExtension == ".avi")
			{
				// fixes large opendml avis - mplayer can't handle big indices
				options.SetNoIdx(true);
				CLog::Log("Trying to play a large avi file. Setting -noidx");
			}
		}

		if (CUtil::IsRAR(strFile))
		{
			options.SetNoIdx(true);
			CLog::Log("Trying to play a rar file (%s). Setting -noidx", strFile.c_str());
			// -noidx enables mplayer to play an .avi file from a streaming source that is not seekable.
			// it tells mplayer to _not_ try getting the avi index from the end of the file.
			// this option is enabled if we try play video from a rar file, as there is a modified version
			// of ccxstream which sends unrared data when you request the first .rar of a movie.
			// This means you can play a movie gaplessly from 50 rar files without unraring, which is neat.
		}


		//Make sure we remeber what subtitle stream and audiostream we where playing so that stacked items gets the same.
		//These will be reset in Application.Playfile if the restart parameter isn't set.
		if(g_stSettings.m_iAudioStream >= 0)
			options.SetAudioStream(g_stSettings.m_iAudioStream);
		
		if(g_stSettings.m_iSubtitleStream >= 0)
			options.SetSubtitleStream(g_stSettings.m_iSubtitleStream);


    //force mplayer to play ac3 and dts files with correct codec
    if (strExtension == ".ac3") {
      options.SetRawAudioFormat("0x2000");
    }
    else if (strExtension == ".dts") {
      options.SetRawAudioFormat("0x2001");
    }

		options.GetOptions(argc,argv);
		

		//CLog::Log("  open 1st time");
		mplayer_init(argc,argv);
		mplayer_setcache_size(iCacheSize);
		mplayer_setcache_backbuffer(iCacheSizeBackBuffer);
		if(bFileIsDVDImage || bFileIsDVDIfoFile)
			iRet=mplayer_open_file(GetDVDArgument(strFile).c_str());
		else
			iRet=mplayer_open_file(strFile.c_str());
		if (iRet <= 0 || m_bCanceling)
		{
      throw iRet;
		}

    if (bFileOnInternet) 
		{
			// for streaming we're done.
		}
		else
		{
			// for other files, check the codecs/audio channels etc etc...
			char          strFourCC[10],strVidFourCC[10];
			char          strAudioCodec[128],strVideoCodec[128];
			long          lBitRate;
			long          lSampleRate;
			int	          iChannels;
			BOOL          bVBR;
			float         fFPS;
			unsigned int  iWidth;
			unsigned int  iHeight;
			long          lFrames2Early;
			long          lFrames2Late;
			bool          bNeed2Restart=false;

			// get the audio & video info from the file
			mplayer_GetAudioInfo(strFourCC,strAudioCodec, &lBitRate, &lSampleRate, &iChannels, &bVBR);
			mplayer_GetVideoInfo(strVidFourCC,strVideoCodec, &fFPS, &iWidth,&iHeight, &lFrames2Early, &lFrames2Late);


			// do we need 2 do frame rate conversions ?
			if (g_stSettings.m_bFrameRateConversions && CUtil::IsVideo(strFile) )
			{
				DWORD dwVideoStandard=XGetVideoStandard();
				if (dwVideoStandard==XC_VIDEO_STANDARD_PAL_I)
				{
					// PAL. Framerate for pal=25.0fps
					// do frame rate conversions for NTSC movie playback under PAL
					if (fFPS >= 23.0 && fFPS <= 24.5f )
					{
						// 23.978  fps -> 25fps frame rate conversion
						options.SetSpeed(25.0f / fFPS); 
						options.SetFPS(25.0f);
						bNeed2Restart=true;
						CLog::Log("  --restart cause we use ntsc->pal framerate conversion");
					}
				}
				else
				{
					//NTSC framerate=23.976 fps
					// do frame rate conversions for PAL movie playback under NTSC
					if (fFPS>=24 && fFPS <= 25.5)
					{
						options.SetSpeed(23.976f / fFPS); 
						options.SetFPS(23.976f);
						bNeed2Restart=true;
						CLog::Log("  --restart cause we use pal->ntsc framerate conversion");
					}
				}
			}
			// check if the codec is AC3 or DTS
			bool bDTS = false;
			if (strstr(strAudioCodec,"AC3")==NULL &&
				!(bDTS = !(strstr(strAudioCodec,"DTS")==NULL)))
			{
				// no make sure AC3 passthru is off (used below)
				options.SetAC3PassTru(false);
			}
			else
			{
				// should we be using passthrough??
				// if the sample rate is not 48kHz, it could be a DTS file
				// DTS files are reported as AC3 in mplayer as at 30/4/2004
				// This may be changed at a later date, so it'll pay to keep an eye
				// on things.
				if (bDTS || (lSampleRate!=48000 && g_stSettings.m_bUseDigitalOutput && bSupportsDTSOut))
				{
					if (bDTS)
					{
						if (lSampleRate==48000 && g_stSettings.m_bUseDigitalOutput && bSupportsDTSOut)
						{
							// DTS, woohoo, change to passthrough mode
							options.SetAC3PassTru(true);
							bNeed2Restart=true;
						}
					}
					else
					{
						// Could be DTS - lets restart to see
						options.SetAC3PassTru(true);
						options.SetChannels(6);
						CLog::Log("  --------------- restart to test for DTS ---------------");
						mplayer_close_file();
						options.GetOptions(argc,argv);
						load();
						mplayer_init(argc,argv);
						mplayer_setcache_size(iCacheSize);
						mplayer_setcache_backbuffer(iCacheSizeBackBuffer);
						if(bFileIsDVDImage || bFileIsDVDIfoFile)
							iRet=mplayer_open_file(GetDVDArgument(strFile).c_str());
						else
							iRet=mplayer_open_file(strFile.c_str());
						if (iRet <= 0 || m_bCanceling)
						{
							throw iRet;
						}
						// OK, now check what we've got now...
						long lNewSampleRate;
						int iNewChannels;
						mplayer_GetAudioInfo(strFourCC,strAudioCodec, &lBitRate, &lNewSampleRate, &iNewChannels, &bVBR);
						if (lNewSampleRate==48000)
						{	// yep - was DTS after all!
							options.SetAC3PassTru(true);
							//options.SetChannels(2);
							bNeed2Restart=false;
						}
						else
						{	// nope - must be non-48kHz AC3 or something else...
							options.SetAC3PassTru(false);
							iChannels = iNewChannels;
							bNeed2Restart=true;
						}
					}
				}
				else if (lSampleRate==48000 && g_stSettings.m_bUseDigitalOutput && bSupportsAC3Out)
				{	// sample rate is OK
					if (iChannels == 2 && g_stSettings.m_bDDStereoPassThrough)
					{	// YES - change to pass through mode
						options.SetAC3PassTru(true);
						//options.SetChannels(2);
						bNeed2Restart=true;
					}
					if (iChannels > 2 && g_stSettings.m_bDD_DTSMultiChannelPassThrough)
					{	// YES - change to pass through mode
						options.SetAC3PassTru(true);
						//options.SetChannels(2);
						bNeed2Restart=true;
					}
				}
			}
			// if we dont have ac3 passtru enabled
			if (!options.GetAC3PassTru())
			{
				// if DMO filter is used we need 2 remap the audio speaker layout (MS does things differently)
				// FL, FR, C, LFE, SL, SR
				if( strstr(strAudioCodec,"DMO") && (iChannels==6) )
				{
					options.SetChannels(6);
					options.SetChannelMapping("channels=6:6:0:0:1:1:2:4:3:5:4:2:5:3");
					bNeed2Restart=true;
					CLog::Log("  --restart cause speaker mapping needs fixing");
				}	
				// AAC has a different channel mapping.. C, FL, FR, SL, SR, LFE
				if ( strstr(strAudioCodec, "AAC") && (iChannels==6))
				{
					if(g_stSettings.m_bUseDigitalOutput)
					{
						options.SetChannels(6);
						options.SetChannelMapping("channels=6:6:0:4:1:0:2:1:3:2:4:3:5:5");
					}
					else
					{
						options.SetChannels(0);
						options.SetChannelMapping("channels=6:2:0:0:0:1:1:0:2:1:3:0:4:1:5:0:5:1");
					}
					
					bNeed2Restart=true;
					CLog::Log("  --restart cause speaker mapping needs fixing");		
				}
				// OGG has yet another channel mapping.. we need a better way for this FL, C, FR, SL, SR, LFE
				if ( strstr(strAudioCodec, "OggVorbis") && (iChannels==6))
				{
					if(g_stSettings.m_bUseDigitalOutput)
					{
						options.SetChannels(6);
						options.SetChannelMapping("channels=6:6:0:0:1:4:2:1:3:2:4:3:5:5");
					}
					else
					{
						//Doesn't seem to be working here.. only left and right channel seems to be mixed
						options.SetChannels(0);
						options.SetChannelMapping("channels=6:2:0:0:2:1:1:0:1:1:3:0:4:1:5:0:5:1");
					}
					
					bNeed2Restart=true;
					CLog::Log("  --restart cause speaker mapping needs fixing");		
				}


				// if xbox only got stereo output, then limit number of channels to 2
				// same if xbox got digital output, but we're listening to the analog output
				if (!bSupportsAC3Out || !g_stSettings.m_bUseDigitalOutput)
				{
					if (iChannels > 2) 
					{
						iChannels=2;
					}
				}
				// remap audio speaker layout for files with 5 audio channels 
				if (iChannels==5)
				{
					options.SetChannels(6);
					options.SetChannelMapping("channels=6:5:0:0:1:1:2:2:3:3:4:4:5:5");
					bNeed2Restart=true;
					CLog::Log("  --restart cause audio channels changed:5");
				}
				// remap audio speaker layout for files with 3 audio channels 
				if (iChannels==3)
				{
					options.SetChannels(4);
					options.SetChannelMapping("channels=4:4:0:0:1:1:2:2:2:3");
					bNeed2Restart=true;
					CLog::Log("  --restart cause audio channels changed:3");
				}

				if (iChannels==1 || iChannels==2||iChannels==4)
				{
					int iChan=options.GetChannels();
					if ( iChannels ==2) iChannels=0;
					if (iChan!=iChannels)
					{
						CLog::Log("  --restart cause audio channels changed");
						options.SetChannels(iChannels);
						bNeed2Restart=true; 
					}
				}
			}

			if (bNeed2Restart)
			{
				CLog::Log("  --------------- restart ---------------");
				//CLog::Log("  open 2nd time");
				mplayer_close_file();
				options.GetOptions(argc,argv);
				load();
				mplayer_init(argc,argv);
				mplayer_setcache_size(iCacheSize);
				mplayer_setcache_backbuffer(iCacheSizeBackBuffer);
				if(bFileIsDVDImage || bFileIsDVDIfoFile)
					iRet=mplayer_open_file(GetDVDArgument(strFile).c_str());
				else
					iRet=mplayer_open_file(strFile.c_str());
				if (iRet <= 0 || m_bCanceling)
				{
          throw iRet;
				}

			}
		}
		m_bIsPlaying= true;

		if(bIsDVD) //Default subtitles for dvd's off
			mplayer_showSubtitle(false);
		
		bIsVideo=HasVideo();
		bIsAudio=HasAudio();
		int iNewCacheSize=GetCacheSize(bFileOnHD,bFileOnISO,bFileOnUDF,bFileOnInternet,bFileOnLAN, bIsVideo, bIsAudio, bIsDVD);
		if (iNewCacheSize > iCacheSize)
		{
			CLog::Log("detected video. Cachesize is now %i, (was %i)", iNewCacheSize,iCacheSize);
			mplayer_setcache_size(iCacheSize);
		}

		if ( ThreadHandle() == NULL)
		{
			Create();
		}

	}
	catch(...)
	{
		CLog::Log("cmplayer::openfile() %s failed",strFile.c_str());
		if (m_dlgCache) delete m_dlgCache;
		m_dlgCache=NULL;  
		closefile();
		return false;
	}

	if (m_dlgCache) delete m_dlgCache;
	m_dlgCache=NULL;

	//	mplayer return values: 
	//	-1	internal error
	//	0		try next file (eg. file doesn't exitst)
	//	1		successfully started playing
	return (iRet>0);
}

bool CMPlayer::closefile()
{
	m_bIsPlaying=false;
	StopThread();

	return true;
}

bool CMPlayer::IsPlaying() const
{
	return m_bIsPlaying;
}


void CMPlayer::OnStartup()
{
}

void CMPlayer::OnExit()
{
}

void CMPlayer::Process()
{
	if (m_pDLL && m_bIsPlaying)
	{
		m_callback.OnPlayBackStarted();

		int exceptionCount = 0;
		time_t mark = time(NULL);
		bool FirstLoop = true;

		do 
		{
			try
			{
				if (!m_bPaused)
				{
					// we're playing
					int iRet=mplayer_process();
					
					if (iRet < 0)
					{
						m_bIsPlaying=false;
					}

					__int64 iPTS=mplayer_get_pts();
					if (iPTS)
					{
						m_iPTS=iPTS;
					}

					FirstLoop = false;
				}
				else // we're paused
				{
					if (HasVideo())
					{
						xbox_video_CheckScreenSaver();		// Check for screen saver
						Sleep(100);
					}
					else
						Sleep(10);
				}
			}
			catch(...)
			{
				char module[100];
				mplayer_get_current_module(module, sizeof(module));
				CLog::Log("mplayer generated exception in %s", module);
				//CLog::Log("mplayer generated exception!");
				exceptionCount++;

				// bad codec detection
				if (FirstLoop)
					m_bIsPlaying = false;
			}

			if (exceptionCount>0)
			{
				time_t t = time(NULL);
				if (t != mark)
				{
					mark = t;
					exceptionCount--;
				}
			}

		} while ((m_bIsPlaying && !m_bStop) && (exceptionCount<5));

		if (!m_bStop)
		{
			xbox_audio_wait_completion();
		}
		_SubtitleExtension.Empty();
		mplayer_close_file();
	}
	m_bIsPlaying=false;
	if (!m_bStop)
	{
		m_callback.OnPlayBackEnded();
	}
}

void CMPlayer::Unload()
{
	if (m_pDLL)
	{
		// if m_pDLL is not NULL we asume mplayer_open_file has already been called
		// and thus we can call mplayer_close_file now.
		mplayer_close_file();
		delete m_pDLL;
		dllReleaseAll( );
		m_pDLL=NULL;
	}
}

void  CMPlayer::Pause()
{
	if (!m_bPaused)
	{
		m_bPaused=true;
		if (!HasVideo())
			audio_pause();
	}
	else
	{
		m_bPaused=false;
		if (!HasVideo())
			audio_resume();
	}
}

bool CMPlayer::IsPaused() const
{
	return m_bPaused;
}


__int64	CMPlayer::GetPTS()
{
	if (!m_pDLL) return 0;
	if (!m_bIsPlaying) return 0;
	return m_iPTS;
}

bool CMPlayer::HasVideo()
{
	return (mplayer_HasVideo()==TRUE);
}

bool CMPlayer::HasAudio()
{
	return (mplayer_HasAudio()==TRUE);
}

void CMPlayer::ToggleOSD()
{
	OutputDebugString("toggle mplayer OSD\n");
	mplayer_put_key('o');
}

void CMPlayer::SwitchToNextLanguage()
{
	mplayer_put_key('l');
}

void CMPlayer::ToggleSubtitles()
{
	mplayer_put_key('s');
}


void CMPlayer::SubtitleOffset(bool bPlus)
{
	if (bPlus)
		mplayer_put_key('x');
	else
		mplayer_put_key('z');
}

void CMPlayer::Seek(bool bPlus, bool bLargeStep)
{
	if (bLargeStep)
	{
		if (bPlus)
			mplayer_put_key(KEY_PAGE_UP);
		else
			mplayer_put_key(KEY_PAGE_DOWN);
	}
	else
	{
		if (bPlus)
			mplayer_put_key(KEY_UP);
		else
			mplayer_put_key(KEY_DOWN);
	}
}

void CMPlayer::ToggleFrameDrop()
{
	mplayer_put_key('d');
}

int CMPlayer::GetVolume()
{
	return mplayer_getVolume();
}
void CMPlayer::SetVolume(int iPercentage)
{
	mplayer_setVolume(iPercentage);
}

void CMPlayer::SetContrast(bool bPlus)
{
	if (bPlus)
		mplayer_put_key('2');
	else
		mplayer_put_key('1');
}

void CMPlayer::SetBrightness(bool bPlus)
{
	if (bPlus)
		mplayer_put_key('4');
	else
		mplayer_put_key('3');
}
void CMPlayer::SetHue(bool bPlus)
{
	if (bPlus)
		mplayer_put_key('6');
	else
		mplayer_put_key('5');
}
void CMPlayer::SetSaturation(bool bPlus)
{
	if (bPlus)
		mplayer_put_key('8');
	else
		mplayer_put_key('7');
}



void CMPlayer::GetAudioInfo( CStdString& strAudioInfo)
{
	char strFourCC[10];
	char strAudioCodec[128];
	long lBitRate;
	long lSampleRate;
	int	 iChannels;
	BOOL bVBR;
	if (!m_bIsPlaying||!mplayer_HasAudio())
	{
		strAudioInfo="";
		return;
	}
	mplayer_GetAudioInfo(strFourCC,strAudioCodec, &lBitRate, &lSampleRate, &iChannels, &bVBR);
	float fSampleRate=((float)lSampleRate) / 1000.0f;
	if (bVBR)
	{
		strAudioInfo.Format("audio:(%s) VBR br:%i sr:%02.2f khz chns:%i",
			strAudioCodec,lBitRate,fSampleRate,iChannels);
	}
	else
	{
		strAudioInfo.Format("audio:(%s) CBR br:%i sr:%02.2f khz chns:%i",
			strAudioCodec,lBitRate,fSampleRate,iChannels);
	}
}

void CMPlayer::GetVideoInfo( CStdString& strVideoInfo)
{

	char  strFourCC[10];
	char  strVideoCodec[128];
	float fFPS;
	unsigned int   iWidth;
	unsigned int   iHeight;
	long  lFrames2Early;
	long  lFrames2Late;
	if (!m_bIsPlaying)
	{
		strVideoInfo="";
		return;
	}
	mplayer_GetVideoInfo(strFourCC,strVideoCodec, &fFPS, &iWidth,&iHeight, &lFrames2Early, &lFrames2Late);
	strVideoInfo.Format("video:%s fps:%02.2f %ix%i early/late:%i/%i", 
		strVideoCodec,fFPS,iWidth,iHeight,lFrames2Early,lFrames2Late);
}


void CMPlayer::GetGeneralInfo( CStdString& strVideoInfo)
{
	long lFramesDropped;
	int iQuality;
	int iCacheFilled;
	float fTotalCorrection;
	float fAVDelay;
	if (!m_bIsPlaying)
	{
		strVideoInfo="";
		return;
	}
	mplayer_GetGeneralInfo(&lFramesDropped, &iQuality, &iCacheFilled, &fTotalCorrection, &fAVDelay);
	strVideoInfo.Format("dropped:%i Q:%i cache:%i%% ct:%2.2f av:%2.2f", 
		lFramesDropped, iQuality, iCacheFilled, fTotalCorrection, fAVDelay);
}

extern void xbox_audio_registercallback(IAudioCallback* pCallback);
extern void xbox_audio_unregistercallback();
extern void xbox_video_getRect(RECT& SrcRect, RECT& DestRect);
extern void xbox_video_getAR(float& fAR);
extern void xbox_video_update(bool bPauseDrawing);
//extern void xbox_video_update_subtitle_position();
//extern void xbox_video_render_subtitles();

void CMPlayer::Update(bool bPauseDrawing)
{
	xbox_video_update(bPauseDrawing);
}

void CMPlayer::GetVideoRect(RECT& SrcRect, RECT& DestRect)
{
	xbox_video_getRect(SrcRect, DestRect);
}

void CMPlayer::GetVideoAspectRatio(float& fAR)
{
	xbox_video_getAR(fAR);
}

void CMPlayer::RegisterAudioCallback(IAudioCallback* pCallback)
{
	xbox_audio_registercallback(pCallback);
}

void CMPlayer::UnRegisterAudioCallback()
{
	xbox_audio_unregistercallback();
}
void CMPlayer::AudioOffset(bool bPlus)
{
	if (bPlus)
		mplayer_put_key('+');
	else
		mplayer_put_key('-');
}
void CMPlayer::SwitchToNextAudioLanguage()
{

}

//void CMPlayer::UpdateSubtitlePosition()
//{
//	xbox_video_update_subtitle_position();
//}
//
//void CMPlayer::RenderSubtitles()
//{
//	xbox_video_render_subtitles();
//}

bool CMPlayer::CanRecord() 
{
	if (!m_pShoutCastRipper) return false;
	return m_pShoutCastRipper->CanRecord();
}
bool CMPlayer::IsRecording() 
{
	if (!m_pShoutCastRipper) return false;
	return m_pShoutCastRipper->IsRecording();
}
bool CMPlayer::Record(bool bOnOff) 
{
	if (!m_pShoutCastRipper) return false;
	if (bOnOff && IsRecording()) return true;
	if (bOnOff==false && IsRecording()==false) return true;
	if (bOnOff) 
		return m_pShoutCastRipper->Record();

	m_pShoutCastRipper->StopRecording();
	return true;
}


void CMPlayer::SeekPercentage(int iPercent)
{
	mplayer_setPercentage( iPercent);
	if (HasVideo())
	{
		SwitchToThread();
		xbox_video_wait();
	}
}

int CMPlayer::GetPercentage()
{
	return mplayer_getPercentage( );
}


void    CMPlayer::SetAVDelay(float fValue)
{
	mplayer_setAVDelay(fValue);
}

float   CMPlayer::GetAVDelay()
{
	return mplayer_getAVDelay();
}

void    CMPlayer::SetSubTitleDelay(float fValue)
{
	mplayer_setSubtitleDelay(fValue);
}

float   CMPlayer::GetSubTitleDelay()
{
	return mplayer_getSubtitleDelay();
}

int     CMPlayer::GetSubtitleCount()
{
	return mplayer_getSubtitleCount();
}

int     CMPlayer::GetSubtitle()
{
	return mplayer_getSubtitle();
};

void      CMPlayer::GetSubtitleName(int iStream, CStdString &strStreamName)
{
	stream_language_t slt;
	mplayer_getSubtitleStreamInfo(iStream, &slt);
	CLog::Log("Stream:%d language:%d", iStream, slt.language);
	if(slt.language != 0)
	{               
		char lang[3];
		lang[2]=0;
		lang[1]=(slt.language&255);
		lang[0]=(slt.language>>8);
		CStdString strName;
		for(int i=0;i<DVDLANGUAGES;i++)
		{
			if(stricmp(lang,dvd_audio_stream_langs[i][0])==0)
			{
				strName = dvd_audio_stream_langs[i][1];
				break;
			}
		}
		if(strName.length() == 0)
		{
			strName = "UNKNOWN:";
			strName += (char)(slt.language>>8);
			strName += (char)(slt.language&255);
		}

		strStreamName = strName;

	}
	else
	{
		strStreamName = "";
	}

};

void    CMPlayer::SetSubtitle(int iStream)
{
	mplayer_setSubtitle(iStream);
	options.SetSubtitleStream(iStream);
	g_stSettings.m_iSubtitleStream = iStream;
};

bool    CMPlayer::GetSubtitleVisible()
{
	if(mplayer_SubtitleVisible())
		return true;
	else
		return false;
}
void    CMPlayer::SetSubtitleVisible(bool bVisible)
{
	mplayer_showSubtitle(bVisible);
}

int     CMPlayer::GetAudioStreamCount()
{
	return mplayer_getAudioStreamCount();
}

int     CMPlayer::GetAudioStream()
{
	return mplayer_getAudioStream();
}

void     CMPlayer::GetAudioStreamName(int iStream, CStdString& strStreamName)
{
	stream_language_t slt;
	mplayer_getAudioStreamInfo(iStream, &slt);
	if(slt.language != 0)
	{               
		char lang[3];
		lang[2]=0;
		lang[1]=(slt.language&255);
		lang[0]=(slt.language>>8);
		CStdString strName;
		for(int i=0;i<DVDLANGUAGES;i++)
		{
			if(stricmp(lang,dvd_audio_stream_langs[i][0])==0)
			{
				strName = dvd_audio_stream_langs[i][1];
				break;
			}
		}
		if(strName.length() == 0)
		{
			strName = "UNKNOWN:";
			strName += (char)(slt.language>>8);
			strName += (char)(slt.language&255);
		}

		strStreamName.Format("%s - %s(%s)",strName,dvd_audio_stream_types[slt.type],dvd_audio_stream_channels[slt.channels]);

	}
	else
	{
		strStreamName = "";
	}
	//char lang[3];
	//code=lang[1]|(lang[0]<<8);
	//lang[2]=0;
	//lang[1]=(slt.language&255);
	//lang[0]=(slt.language>>8);
	//strStreamName = lang;
}

void     CMPlayer::SetAudioStream(int iStream)
{
	//Make sure we get the correct aid for the stream
	//Really bad way cause we need to restart and there is no good way currently to restart mplayer without onloading it first
	g_stSettings.m_iAudioStream = mplayer_getAudioStreamInfo(iStream, NULL);
	options.SetAudioStream(g_stSettings.m_iAudioStream);
	//we need to restart after here for change to take effect
}


void CMPlayer::SeekTime(int iTime)
{
	mplayer_setTime( iTime);
	if (HasVideo())
	{
		SwitchToThread();
		xbox_video_wait();
	}
}

__int64 CMPlayer::GetTime()
{
	return mplayer_getCurrentTime();
}

int CMPlayer::GetTotalTime()
{
	return mplayer_getTime();
}

void CMPlayer::ToFFRW(int iSpeed)
{
	mplayer_ToFFRW( iSpeed);
	//SwitchToThread();
	//xbox_video_wait();
}


int CMPlayer::GetCacheSize(bool bFileOnHD,bool bFileOnISO,bool bFileOnUDF,bool bFileOnInternet,bool bFileOnLAN, bool bIsVideo, bool bIsAudio, bool bIsDVD)
{
	if (g_stSettings.m_bNoCache) return 0;

	if (bFileOnHD)
	{
		if ( bIsDVD  ) return g_stSettings.m_iCacheSizeHD[CACHE_VOB];
		if ( bIsVideo) return g_stSettings.m_iCacheSizeHD[CACHE_VIDEO];
		if ( bIsAudio) return g_stSettings.m_iCacheSizeHD[CACHE_AUDIO];
	}
	if (bFileOnISO)
	{
		if ( bIsDVD  ) return g_stSettings.m_iCacheSizeISO[CACHE_VOB];
		if ( bIsVideo) return g_stSettings.m_iCacheSizeISO[CACHE_VIDEO];
		if ( bIsAudio) return g_stSettings.m_iCacheSizeISO[CACHE_AUDIO];
	}
	if (bFileOnUDF)
	{
		if ( bIsDVD  ) return g_stSettings.m_iCacheSizeUDF[CACHE_VOB];
		if ( bIsVideo) return g_stSettings.m_iCacheSizeUDF[CACHE_VIDEO];
		if ( bIsAudio) return g_stSettings.m_iCacheSizeUDF[CACHE_AUDIO];
	}
	if (bFileOnInternet)
	{
		if ( bIsDVD  ) return g_stSettings.m_iCacheSizeInternet[CACHE_VOB];
		if ( bIsVideo) return g_stSettings.m_iCacheSizeInternet[CACHE_VIDEO];
		if ( bIsAudio) return g_stSettings.m_iCacheSizeInternet[CACHE_AUDIO];
		//File is on internet however we don't know what type.
		//Apperently fixes DreamBox playback.
		return 4096; 
	}
	if (bFileOnLAN)
	{
		if ( bIsDVD  ) return g_stSettings.m_iCacheSizeLAN[CACHE_VOB];
		if ( bIsVideo) return g_stSettings.m_iCacheSizeLAN[CACHE_VIDEO];
		if ( bIsAudio) return g_stSettings.m_iCacheSizeLAN[CACHE_AUDIO];
	}
	return 1024;
}

CStdString CMPlayer::GetDVDArgument(const CStdString& strFile)
{

	int iTitle = CUtil::GetDVDIfoTitle(strFile);
	if(iTitle==0)
		return CStdString("dvd://");
	else
	{
		CStdString strBuf;
		strBuf.Format("dvd://%i",iTitle);
		return strBuf;
	}
}
void CMPlayer::ShowOSD(bool bOnoff)
{
	if (bOnoff) mplayer_showosd(1);
	else mplayer_showosd(0);
}

void CMPlayer::DoAudioWork()
{
  xbox_audio_do_work();
}

bool CMPlayer::GetSubtitleExtension(CStdString &strSubtitleExtension) 
{
  strSubtitleExtension = _SubtitleExtension;
  return (!_SubtitleExtension.IsEmpty());
}

