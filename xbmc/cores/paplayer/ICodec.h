#pragma once

#include "ReplayGain.h"
#include "FileSystem/File.h"

#define READ_EOF      -1
#define READ_SUCCESS   0
#define READ_ERROR     1

class ICodec
{
public:
  ICodec()
  {
    m_TotalTime = 0;
    m_SampleRate = 0;
    m_BitsPerSample = 0;
    m_Channels = 0;
    m_Bitrate = 0;
    m_CodecName = "";
  };
  virtual ~ICodec() {};

  // Virtual functions that all codecs should implement.  Note that these may need
  // enhancing and or refactoring at a later date.  It works currently well for MP3 and
  // APE codecs.

  // Init(filename)
  // This routine should handle any initialization necessary.  At a minimum it needs to:
  // 1.  Load any dlls and make sure any buffers etc. are allocated.
  // 2.  If it's using a filereader, initialize it with the appropriate cache size.
  // 3.  Load the file (or at least attempt to load it)
  // 4.  Fill in the m_TotalTime, m_SampleRate, m_BitsPerSample and m_Channels parameters.
  virtual bool Init(const CStdString &strFile, unsigned int filecache)=0;

  // DeInit()
  // Should just cleanup anything as necessary.  No need to free buffers here if they
  // are allocated and destroyed in the destructor.
  virtual void DeInit()=0;

  virtual bool CanSeek() {return true;}

  // Seek()
  // Should seek to the appropriate time (in ms) in the file, and return the
  // time to which we managed to seek (in the case where seeking is problematic)
  // This is used in FFwd/Rewd so can be called very often.
  virtual __int64 Seek(__int64 iSeekTime)=0;

  // ReadPCM()
  // Decodes audio into pBuffer up to size bytes.  The actual amount of returned data
  // is given in actualsize.  Returns READ_SUCCESS on success.  Returns READ_EOF when
  // the data has been exhausted, and READ_ERROR on error.
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize)=0;

  // ReadSamples()
  // Decodes audio into floats (normalized to 1) into pBuffer up to numsamples samples.
  // The actual amount of returned samples is given in actualsamples.  Samples are
  // total samples (ie distributed over channels).
  // Returns READ_SUCCESS on success.  Returns READ_EOF when the data has been exhausted,
  // and READ_ERROR on error.
  virtual int ReadSamples(float *pBuffer, int numsamples, int *actualsamples) { return READ_ERROR; };

  // CanInit()
  // Should return true if the codec can be initialized
  // eg. check if a dll needed for the codec exists
  virtual bool CanInit()=0;

  // SkipNext()
  // Skip to next track/item inside the current media (if supported).
  virtual bool SkipNext(){return false;}

  virtual bool IsCaching()    const    {return m_file.IsCaching();}
  virtual int GetCacheLevel() const    {return m_file.GetCacheLevel();}

  // true if we can retrieve normalized float data immediately
  virtual bool HasFloatData() const { return false; }

  __int64 m_TotalTime;  // time in ms
  int m_SampleRate;
  int m_BitsPerSample;
  int m_Channels;
  int m_Bitrate;
  CStdString m_CodecName;
  CReplayGain m_replayGain;
  XFILE::CFile m_file;
};

