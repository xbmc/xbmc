#pragma once
#include "ICodec.h"
#include "../../cores/DllLoader/dll.h"

#define MAC_VERSION_NUMBER 3990       // From MAC SDK
typedef void * APE_DECOMPRESS_HANDLE; // From MAC SDK
typedef char str_ansi;
enum APE_DECOMPRESS_FIELDS
{
    APE_INFO_FILE_VERSION = 1000,               // version of the APE file * 1000 (3.93 = 3930) [ignored, ignored]
    APE_INFO_COMPRESSION_LEVEL = 1001,          // compression level of the APE file [ignored, ignored]
    APE_INFO_FORMAT_FLAGS = 1002,               // format flags of the APE file [ignored, ignored]
    APE_INFO_SAMPLE_RATE = 1003,                // sample rate (Hz) [ignored, ignored]
    APE_INFO_BITS_PER_SAMPLE = 1004,            // bits per sample [ignored, ignored]
    APE_INFO_BYTES_PER_SAMPLE = 1005,           // number of bytes per sample [ignored, ignored]
    APE_INFO_CHANNELS = 1006,                   // channels [ignored, ignored]
    APE_INFO_BLOCK_ALIGN = 1007,                // block alignment [ignored, ignored]
    APE_INFO_BLOCKS_PER_FRAME = 1008,           // number of blocks in a frame (frames are used internally)  [ignored, ignored]
    APE_INFO_FINAL_FRAME_BLOCKS = 1009,         // blocks in the final frame (frames are used internally) [ignored, ignored]
    APE_INFO_TOTAL_FRAMES = 1010,               // total number frames (frames are used internally) [ignored, ignored]
    APE_INFO_WAV_HEADER_BYTES = 1011,           // header bytes of the decompressed WAV [ignored, ignored]
    APE_INFO_WAV_TERMINATING_BYTES = 1012,      // terminating bytes of the decompressed WAV [ignored, ignored]
    APE_INFO_WAV_DATA_BYTES = 1013,             // data bytes of the decompressed WAV [ignored, ignored]
    APE_INFO_WAV_TOTAL_BYTES = 1014,            // total bytes of the decompressed WAV [ignored, ignored]
    APE_INFO_APE_TOTAL_BYTES = 1015,            // total bytes of the APE file [ignored, ignored]
    APE_INFO_TOTAL_BLOCKS = 1016,               // total blocks of audio data [ignored, ignored]
    APE_INFO_LENGTH_MS = 1017,                  // length in ms (1 sec = 1000 ms) [ignored, ignored]
    APE_INFO_AVERAGE_BITRATE = 1018,            // average bitrate of the APE [ignored, ignored]
    APE_INFO_FRAME_BITRATE = 1019,              // bitrate of specified APE frame [frame index, ignored]
    APE_INFO_DECOMPRESSED_BITRATE = 1020,       // bitrate of the decompressed WAV [ignored, ignored]
    APE_INFO_PEAK_LEVEL = 1021,                 // peak audio level (obsolete) (-1 is unknown) [ignored, ignored]
    APE_INFO_SEEK_BIT = 1022,                   // bit offset [frame index, ignored]
    APE_INFO_SEEK_BYTE = 1023,                  // byte offset [frame index, ignored]
    APE_INFO_WAV_HEADER_DATA = 1024,            // error code [buffer *, max bytes]
    APE_INFO_WAV_TERMINATING_DATA = 1025,       // error code [buffer *, max bytes]
    APE_INFO_WAVEFORMATEX = 1026,               // error code [waveformatex *, ignored]
    APE_INFO_IO_SOURCE = 1027,                  // I/O source (CIO *) [ignored, ignored]
    APE_INFO_FRAME_BYTES = 1028,                // bytes (compressed) of the frame [frame index, ignored]
    APE_INFO_FRAME_BLOCKS = 1029,               // blocks in a given frame [frame index, ignored]
    APE_INFO_TAG = 1030,                        // point to tag (CAPETag *) [ignored, ignored]
    
    APE_DECOMPRESS_CURRENT_BLOCK = 2000,        // current block location [ignored, ignored]
    APE_DECOMPRESS_CURRENT_MS = 2001,           // current millisecond location [ignored, ignored]
    APE_DECOMPRESS_TOTAL_BLOCKS = 2002,         // total blocks in the decompressors range [ignored, ignored]
    APE_DECOMPRESS_LENGTH_MS = 2003,            // total blocks in the decompressors range [ignored, ignored]
    APE_DECOMPRESS_CURRENT_BITRATE = 2004,      // current bitrate [ignored, ignored]
    APE_DECOMPRESS_AVERAGE_BITRATE = 2005,      // average bitrate (works with ranges) [ignored, ignored]

    APE_INTERNAL_INFO = 3000,                   // for internal use -- don't use (returns APE_FILE_INFO *) [ignored, ignored]
};

class APECodec : public ICodec
{
  struct APEdll
  {
    int (__stdcall* GetVersionNumber)();
    int (__stdcall* Seek)(APE_DECOMPRESS_HANDLE, int);
    void (__stdcall* Destroy)(APE_DECOMPRESS_HANDLE);
    int (__stdcall* GetData)(APE_DECOMPRESS_HANDLE, char *, int, int *);
    int (__stdcall* GetInfo)(APE_DECOMPRESS_HANDLE, APE_DECOMPRESS_FIELDS, int, int);
    APE_DECOMPRESS_HANDLE (__stdcall* Create)(const str_ansi *, int *);
  };

public:
  APECodec();
  virtual ~APECodec();

  virtual bool Init(const CStdString &strFile);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool HandlesType(const char *type);

protected:
  int m_BytesPerBlock;

private:
  APEdll m_dll;
  APE_DECOMPRESS_HANDLE m_handle;
  DllLoader *m_pDll;                  // PAP DLL
  bool LoadDLL();                     // load the DLL in question
  bool m_bDllLoaded;                  // whether our dll is loaded
};
