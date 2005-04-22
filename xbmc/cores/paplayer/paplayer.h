#pragma once
#include "../../Application.h"
#include "../iplayer.h"
#include "../mplayer/ASyncDirectSound.h"
#include "../../utils/thread.h"
#include "../../cores/DllLoader/dll.h"
#include "../../MusicInfoTagLoaderMP3.h"

#include "dec_if.h" // Found in nsv_codec_sdk.zip @ http://www.nullsoft.com/nsv/nsv_codec_sdk.zip
                    // SDK has a readme file that explains everything. Here is the text about decoding.
                    //  Decoding plugins are generally scanned from Program Files\Common Files\NSV,
                    //  as well as Plugins\ for Winamp. Generally you should name your plug-in 
                    //  nsvdec_*.dll, but in the case of Winamp plugins you can also name it in_*.dll,
                    //  to have a DLL that supports native Winamp decoding as well as NSV decoding 
                    //  (our own in_mp3.dll does this).

class PAPlayer : public IPlayer, public CThread
{
public:
  PAPlayer(IPlayerCallback& callback);
  virtual ~PAPlayer();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual void UnRegisterAudioCallback();

  virtual bool OpenFile(const CFileItem& file, __int64 iStartTime);
  virtual bool CloseFile();
  virtual bool IsPlaying() const { return m_bIsPlaying; }
  virtual void Pause();
  virtual bool IsPaused() const { return m_bPaused; }
  virtual bool HasVideo() { return false; }
  virtual bool HasAudio() { return true; }
  virtual void ToggleOSD() {}
  virtual void SwitchToNextLanguage() {}
  virtual void ToggleSubtitles() {}
  virtual void ToggleFrameDrop() {}
  virtual void SubtitleOffset(bool bPlus = true) {}
  virtual void Seek(bool bPlus = true, bool bLargeStep = false) {}
  virtual void SetVolume(long nVolume);
  virtual void SetContrast(bool bPlus = true) {}
  virtual void SetBrightness(bool bPlus = true) {}
  virtual void SetHue(bool bPlus = true) {}
  virtual void SetSaturation(bool bPlus = true) {}
  virtual void GetAudioInfo( CStdString& strAudioInfo) {}
  virtual void GetVideoInfo( CStdString& strVideoInfo) {}
  virtual void GetGeneralInfo( CStdString& strVideoInfo) {}
  virtual void Update(bool bPauseDrawing = false) {}
  virtual void GetVideoRect(RECT& SrcRect, RECT& DestRect){}
  virtual void GetVideoAspectRatio(float& fAR) {}
  virtual void OnInitialize(int iChannels, int iSamplesPerSec, int iBitsPerSample);
  virtual void OnAudioData(const unsigned char* pAudioData, int iAudioDataLength);
  virtual void ToFFRW(int iSpeed = 0);
  virtual int GetTotalTime();
  virtual __int64 GetTime();
  virtual void SeekTime(__int64 iTime = 0);

protected:
  virtual void OnStartup() {}
  virtual void Process();
  virtual void OnExit();

  bool m_bPaused;
  bool m_bIsPlaying;
  bool m_bStopPlaying;

  CEvent m_startEvent;

  CFile m_filePAP;

  int m_iSpeed;   // current playing speed

private:
  

  DllLoader *m_pDll;                  // PAP DLL
  bool m_bDllLoaded;                  // whether our dll is loaded
  bool LoadDLL();                     // load the DLL in question
  IAudioDecoder* m_pPAP;            // handle to the codec.

  CASyncDirectSound* m_pAudioDevice;  // our output device
  DWORD m_dwAudioBufferSize; // size of the buffer in use
  DWORD m_dwAudioBufferMin;  // minimum size of our buffer before we need more
  DWORD m_dwAudioBufferPos;  // position in the buffer
  DWORD m_dwAudioMaxSize;
  
  
  BYTE* m_pPcm;     // our temporary pcm buffer so we can learn the format of the data
  BYTE* m_pInputBuffer;   // our PAP input buffer
  int CreateBuffer();     // initializes the above
  void KillBuffer();      // kills the above

  bool m_IgnoreFirst;     // Ignore first samples if this is true (for gapless playback)
  bool m_IgnoreLast;     // Ignore first samples if this is true (for gapless playback)
  BYTE *m_pIgnoredFirstBytes;  // amount of samples ignored thus far

  bool ProcessPAP();    // does the actual reading and decode from our PAP dll
 
  bool    m_Decoding;
  bool    m_CallPAPAgain;
  bool    m_BufferingPcm;
  bool    m_eof;
  DWORD   m_dwInputBufferSize; // size of the input buffer in use
  DWORD   m_AverageInputBytesPerSecond;
  __int64 m_SeekTime;
  int     m_cantSeek;
  DWORD   m_InputBytesWanted;
  DWORD   m_Channels;
  DWORD   m_SampleRate;
  DWORD   m_SampleSize;
  unsigned int     m_PcmSize;  
  unsigned int     m_PcmPos;
  DWORD   m_dwBytesReadIn;
  __int64 m_startOffset;
  __int64 m_dwBytesSentOut;
  __int64 m_lastByteOffset;
  int     m_iLastSpeed;
  bool    m_bGuessByterate;

  CVBRMP3SeekHelper m_seekInfo;
};
