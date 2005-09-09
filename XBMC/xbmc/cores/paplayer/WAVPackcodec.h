#pragma once
#include "ICodec.h"
#include "FileReader.h"
#include "WavPack/WavPack.h"

class WAVPackCodec : public ICodec
{
  struct WAVPackdll
  {
    WavpackContext *(__cdecl*WavpackOpenFileInputEx) (stream_reader *reader, void *wv_id, void *wvc_id, char *error, int flags, int norm_offset);
    WavpackContext *(__cdecl*WavpackOpenFileInput) (const char *infilename, char *error, int flags, int norm_offset);

    int (__cdecl*WavpackGetVersion) (WavpackContext *wpc);
    unsigned int (__cdecl*WavpackUnpackSamples) (WavpackContext *wpc, int *buffer, unsigned int samples);
    unsigned int (__cdecl*WavpackGetNumSamples) (WavpackContext *wpc);
    unsigned int (__cdecl*WavpackGetSampleIndex) (WavpackContext *wpc);
    int (__cdecl*WavpackGetNumErrors) (WavpackContext *wpc);
    int (__cdecl*WavpackLossyBlocks) (WavpackContext *wpc);
    int (__cdecl*WavpackSeekSample) (WavpackContext *wpc, unsigned int sample);
    WavpackContext *(*WavpackCloseFile) (WavpackContext *wpc);
    unsigned int (__cdecl*WavpackGetSampleRate) (WavpackContext *wpc);
    int (__cdecl*WavpackGetBitsPerSample) (WavpackContext *wpc);
    int (__cdecl*WavpackGetBytesPerSample) (WavpackContext *wpc);
    int (__cdecl*WavpackGetNumChannels) (WavpackContext *wpc);
    int (__cdecl*WavpackGetReducedChannels) (WavpackContext *wpc);
    int (__cdecl*WavpackGetMD5Sum) (WavpackContext *wpc, unsigned char data [16]);
    unsigned int (__cdecl*WavpackGetWrapperBytes) (WavpackContext *wpc);
    unsigned char *(*WavpackGetWrapperData) (WavpackContext *wpc);
    void (__cdecl*WavpackFreeWrapper) (WavpackContext *wpc);
    double (__cdecl*WavpackGetProgress) (WavpackContext *wpc);
    unsigned int (__cdecl*WavpackGetFileSize) (WavpackContext *wpc);
    double (__cdecl*WavpackGetRatio) (WavpackContext *wpc);
    double (__cdecl*WavpackGetAverageBitrate) (WavpackContext *wpc, int count_wvc);
    double (__cdecl*WavpackGetInstantBitrate) (WavpackContext *wpc);
    int (__cdecl*WavpackGetTagItem) (WavpackContext *wpc, const char *item, char *value, int size);
    int (__cdecl*WavpackAppendTagItem) (WavpackContext *wpc, const char *item, const char *value);
    int (__cdecl*WavpackWriteTag) (WavpackContext *wpc);

    WavpackContext *(__cdecl*WavpackOpenFileOutput) (blockout blockout, void *wv_id, void *wvc_id);
    int (__cdecl*WavpackSetConfiguration) (WavpackContext *wpc, WavpackConfig *config, unsigned int total_samples);
    int (__cdecl*WavpackAddWrapper) (WavpackContext *wpc, void *data, unsigned int bcount);
    int (__cdecl*WavpackStoreMD5Sum) (WavpackContext *wpc, unsigned char data [16]);
    int (__cdecl*WavpackPackInit) (WavpackContext *wpc);
    int (__cdecl*WavpackPackSamples) (WavpackContext *wpc, int *sample_buffer, unsigned int sample_count);
    int (__cdecl*WavpackFlushSamples) (WavpackContext *wpc);
    void (__cdecl*WavpackUpdateNumSamples) (WavpackContext *wpc, void *first_block);
    void *(__cdecl*WavpackGetWrapperLocation) (void *first_block);
  };

public:
  WAVPackCodec();
  virtual ~WAVPackCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

private:
  static int ReadCallback(void *id, void *data, int bcount);
  static unsigned int GetPosCallback(void *id);
  static int SetPosAbsCallback(void *id, unsigned int pos);
  static int SetPosRelCallback(void *id, int delta, int mode);
  static unsigned int GetLenghtCallback(void *id);
  static int CanSeekCallback(void *id);
  static int PushBackByteCallback(void *id, int c);

  void FormatSamples (BYTE *dst, int bps, long *src, unsigned long samcnt);

  char m_errormsg[512];
  WavpackContext* m_Handle;
  stream_reader m_Callbacks;

  BYTE*     m_Buffer;
  int       m_BufferSize; 
  int       m_BufferPos;
  BYTE*     m_ReadBuffer;

  CFileReader m_file;
  WAVPackdll m_dll;
  bool LoadDLL();                     // load the DLL in question
  bool m_bDllLoaded;                  // whether our dll is loaded
};
