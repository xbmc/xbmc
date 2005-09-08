#ifndef NSF_CODEC_H_
#define NSF_CODEC_H_

#include "ICodec.h"

class NSFCodec : public ICodec
{
  struct NSFDLL 
  {
    int (__cdecl *DLL_LoadNSF)(const char* szFileName);
    void (__cdecl *DLL_FreeNSF)(int);
    int (__cdecl *DLL_StartPlayback)(int nsf, int track);
    long (__cdecl *DLL_FillBuffer)(int nsf, char* buffer, int size);
    void (__cdecl *DLL_FrameAdvance)(int nsf);
    int (__cdecl *DLL_GetPlaybackRate)(int nsf);
    int (__cdecl *DLL_GetNumberOfSongs)(int nsf);
  };

public:
  NSFCodec();
  virtual ~NSFCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

private:
  bool LoadDLL();

  int m_iTrack;
  int m_nsf;
  bool m_bDllLoaded;
  bool m_bIsPlaying;

  NSFDLL m_dll;
  char* m_szBuffer;
  char* m_szStartOfBuffer; // never allocated
  int m_iDataInBuffer;
  __int64 m_iDataPos;
};

#endif