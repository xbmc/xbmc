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

		bool         GetNoCache() const;
		void         SetNoCache(bool bOnOff) ;

		bool         GetNoIdx() const;
		void         SetNoIdx(bool bOnOff) ;

		float        GetVolumeAmplification() const;
		void         SetVolumeAmplification(float fDB) ;

		int          GetAudioStream() const;
		void         SetAudioStream(int iStream);

		int          GetSubtitleStream() const;
		void         SetSubtitleStream(int iStream);

		int          GetChannels() const;
		void         SetChannels(int iChannels);

		bool         GetAC3PassTru();
		void         SetAC3PassTru(bool bOnOff);

    bool         GetDTSPassTru();
		void         SetDTSPassTru(bool bOnOff);

    bool         GetLimitedHWAC3();
		void         SetLimitedHWAC3(bool bOnOff);

		const string GetChannelMapping() const;
		void         SetChannelMapping(const string& strMapping);
		void         SetSpeed(float fSpeed);
		float        GetSpeed() const; 
		void         SetFPS(float fFPS);
		float        GetFPS() const; 
		void         GetOptions(int& argc, char* argv[]);
		void         SetDVDDevice(const string & strDevice);
		void         SetFlipBiDiCharset(const string& strCharset);
    void         SetRawAudioFormat(const string& strHexRawAudioFormat);
	private:
		bool    m_bResampleAudio;
		bool    m_bNoCache;
		bool    m_bNoIdx;
		float   m_fSpeed;
		float   m_fFPS;
		int     m_iChannels;
		int     m_iAudioStream;
		int     m_iSubtitleStream;
		int     m_iCacheSizeBackBuffer; // percent of cache used for back buffering
		bool    m_bAC3PassTru;
    bool    m_bDTSPassTru;
		float   m_fVolumeAmplification;
		bool    m_bNonInterleaved;
    bool    m_bLimitedHWAC3;
		string  m_strChannelMapping;
		string  m_strDvdDevice;
		string  m_strFlipBiDiCharset;
    string  m_strHexRawAudioFormat;
		vector<string> m_vecOptions;

	};
	CMPlayer(IPlayerCallback& callback);
	virtual ~CMPlayer();
	virtual void            RegisterAudioCallback(IAudioCallback* pCallback);
	virtual void            UnRegisterAudioCallback();
	virtual bool            openfile(const CStdString& strFile, __int64 iStartTime);
	virtual bool            closefile();
	virtual bool            IsPlaying() const;
	virtual void            Pause();
	virtual bool            IsPaused() const;
	virtual void            Unload();
	virtual bool            HasVideo();
	virtual bool            HasAudio();
	virtual void            ToggleOSD();
	virtual void            SwitchToNextLanguage();

	virtual __int64 GetPTS() ;
	virtual void            ToggleSubtitles();
	virtual void            ToggleFrameDrop();
	virtual void            SubtitleOffset(bool bPlus=true);
	virtual void      Seek(bool bPlus=true, bool bLargeStep=false);
	virtual void            SetVolume(long nVolume);     
	virtual int     GetVolume();  
	virtual void            SetContrast(bool bPlus=true);   
	virtual void            SetBrightness(bool bPlus=true); 
	virtual void            SetHue(bool bPlus=true);        
	virtual void            SetSaturation(bool bPlus=true); 
	virtual void            GetAudioInfo( CStdString& strAudioInfo);
	virtual void            GetVideoInfo( CStdString& strVideoInfo);
	virtual void            GetGeneralInfo( CStdString& strVideoInfo);
	virtual void            Update(bool bPauseDrawing=false);
	virtual void            GetVideoRect(RECT& SrcRect, RECT& DestRect);
	virtual void            GetVideoAspectRatio(float& fAR);
	virtual void            AudioOffset(bool bPlus=true);
	virtual void            SwitchToNextAudioLanguage();
//	virtual void            UpdateSubtitlePosition();
//	virtual void            RenderSubtitles();
	virtual bool            CanRecord() ;
	virtual bool            IsRecording();
	virtual bool            Record(bool bOnOff) ;
	virtual void    SeekPercentage(int iPercent=0);
	virtual int     GetPercentage();
	virtual void    SetAVDelay(float fValue=0.0f);
	virtual float   GetAVDelay();


	virtual void    SetSubTitleDelay(float fValue=0.0f);
	virtual float   GetSubTitleDelay();

	virtual int     GetSubtitleCount();
	virtual int     GetSubtitle();
	virtual void    GetSubtitleName(int iStream, CStdString &strStreamName);
	virtual void    SetSubtitle(int iStream);
	virtual bool    GetSubtitleVisible();
	virtual void    SetSubtitleVisible(bool bVisible);
  virtual bool    GetSubtitleExtension(CStdString &strSubtitleExtension); 

	virtual int     GetAudioStreamCount();
	virtual int     GetAudioStream();
	virtual void     GetAudioStreamName(int iStream, CStdString& strStreamName);
	virtual void     SetAudioStream(int iStream);

	virtual void    SeekTime(__int64 iTime=0);
	virtual int      GetTotalTime();
	virtual __int64 GetTime();
	virtual void    ToFFRW(int iSpeed=0);
	virtual void    ShowOSD(bool bOnoff);
  virtual void   DoAudioWork();
  
  CStdString _SubtitleExtension;
protected:
	int GetCacheSize(bool bFileOnHD,bool bFileOnISO,bool bFileOnUDF,bool bFileOnInternet,bool bFileOnLAN, bool bIsVideo, bool bIsAudio, bool bIsDVD);
	CStdString GetDVDArgument(const CStdString& strFile);
	bool                load();
	virtual void                            OnStartup();
	virtual void                            OnExit();
	virtual void                            Process();
	bool                                                            m_bPaused;
	bool                                                            m_bIsPlaying;
	DllLoader*                                      m_pDLL;
	__int64                                                 m_iPTS;
	Options                                 options;
	bool									m_bSubsVisibleTTF;
};
