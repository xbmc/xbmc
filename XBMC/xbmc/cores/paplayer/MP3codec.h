#pragma once
#include "ICodec.h"
#include "../../MusicInfoTagLoaderMP3.h"
#include "FileReader.h"
#include "Dllmadcodec.h"

class MP3Codec : public ICodec
{
public:
  MP3Codec();
  virtual ~MP3Codec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();
  virtual bool SkipNext();
private:
  // Decoding variables
  CFileReader   m_file;          // Our MP3 file
  __int64 m_lastByteOffset;
  bool    m_eof;
  IAudioDecoder* m_pDecoder;    // handle to the codec.
  bool    m_Decoding;
  bool    m_CallAgainWithSameBuffer;

  // Input buffer to read our mp3 data into
  BYTE*         m_InputBuffer;
  unsigned int  m_InputBufferSize; 
  unsigned int  m_InputBufferPos;

  // Output buffer.  We require this, as mp3 decoding means keeping at least 2 frames (1152 * 2 samples)
  // of data in order to remove that data at the end as it may be surplus to requirements.
  BYTE*         m_OutputBuffer;
  unsigned int  m_OutputBufferSize;
  unsigned int  m_OutputBufferPos;    // position in our buffer

  unsigned int m_Formatdata[8];

  // Seeking helpers
  CVBRMP3SeekHelper m_seekInfo;

  // Gapless playback
  bool m_IgnoreFirst;     // Ignore first samples if this is true (for gapless playback)
  bool m_IgnoreLast;      // Ignore first samples if this is true (for gapless playback)
  int m_IgnoredBytes;     // amount of samples ignored thus far

  DllMadCodec m_dll;
};
