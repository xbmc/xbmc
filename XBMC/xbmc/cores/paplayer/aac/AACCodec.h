#ifndef AACCODEC_INCLUDED
#define AACCODEC_INCLUDED

#ifdef USE_DLL
#define AACAPI __declspec(dllimport)
#else
#define AACAPI __declspec(dllexport)
#endif

/* represents a file */
typedef void* AACHandle;

#define AAC_INVALID_HANDLE ((AACHandle)(LONG_PTR)-1)

/* function definitions for io via callbacks */
typedef unsigned __int32 (*AACOpenCallback)(const char *pName, const char *mode, void *userData);
typedef void (*AACCloseCallback)(void *userData);
typedef unsigned __int32 (*AACReadCallback)(void *pBuffer, unsigned int nBytesToRead, void *userData);
typedef unsigned __int32 (*AACWriteCallback)(void *pBuffer, unsigned int nBytesToWrite, void *userData);
typedef __int32 (*AACSetposCallback)(unsigned __int32 pos, void *userData);
typedef __int64 (*AACGetposCallback)(void *userData);
typedef __int64 (*AACFilesizeCallback)(void *userData);

/* io callback structure */
typedef struct AACIOCallbacks
{
  AACOpenCallback Open;
  AACCloseCallback Close;
  AACReadCallback Read;
  AACWriteCallback Write;
  AACSetposCallback Setpos;
  AACGetposCallback Getpos;
  AACFilesizeCallback Filesize;
  void *userData;
} AACIOCallbacks;

/* info about a file */
typedef struct AACInfo
{
  int samplerate;
  int channels;
  int bitspersample;
  int totaltime;
  int bitrate;
} AACInfo;

/* possible return values of AACRead */
#define AAC_READ_EOF              -1
#define AAC_READ_ERROR            -2
#define AAC_READ_BUFFER_TO_SMALL  -3

#if defined(__cplusplus)
extern "C"
{
#endif

AACHandle AACAPI AACOpen(const char *fn, AACIOCallbacks callbacks);
int AACAPI AACRead(AACHandle handle, BYTE* pBuffer, int iSize);
int AACAPI AACSeek(AACHandle handle, int iTimeMs);
void AACAPI AACClose(AACHandle handle);
AACAPI const char* AACGetErrorMessage();
int AACAPI AACGetInfo(AACHandle handle, AACInfo* info);

#if defined(__cplusplus)
}
#endif
#endif