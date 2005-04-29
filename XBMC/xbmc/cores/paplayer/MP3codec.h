#pragma once
#include "ICodec.h"
#include "../../cores/DllLoader/dll.h"
#include "../../MusicInfoTagLoaderMP3.h"

#include "dec_if.h"

class MP3Codec : public ICodec
{
public:
  MP3Codec();
  virtual ~MP3Codec();

  virtual bool Init(const CStdString &strFile);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool HandlesType(const char *type);

private:
  // Decoding variables
  CFile   m_filePAP;          // Our MP3 file
  __int64 m_lastByteOffset;
  bool    m_eof;
  IAudioDecoder* m_pPAP;    // handle to the codec.
  bool    m_Decoding;
  bool    m_CallPAPAgain;

  // Input buffer to read our mp3 data into
  BYTE*         m_InputBuffer;
  unsigned int  m_InputBufferSize; 
  unsigned int  m_InputBufferPos;

  // Output buffer.  We require this, as mp3 decoding means keeping at least 2 frames (1152 * 2 samples)
  // of data in order to remove that data at the end as it may be surplus to requirements.
  BYTE*         m_OutputBuffer;
  unsigned int  m_OutputBufferSize;
  unsigned int  m_OutputBufferPos;    // position in our buffer

  // Seeking helpers
  CVBRMP3SeekHelper m_seekInfo;
  bool    m_bGuessByterate;
  DWORD   m_AverageInputBytesPerSecond;

  // Gapless playback
  bool m_IgnoreFirst;     // Ignore first samples if this is true (for gapless playback)
  bool m_IgnoreLast;      // Ignore first samples if this is true (for gapless playback)
  int m_IgnoredBytes;     // amount of samples ignored thus far

  DllLoader *m_pDll;                  // PAP DLL
  IAudioDecoder* (__cdecl* CreateDecoder)(unsigned int, IAudioOutput **); // our decode function
  bool LoadDLL();                     // load the DLL in question
  bool m_bDllLoaded;                  // whether our dll is loaded
};
