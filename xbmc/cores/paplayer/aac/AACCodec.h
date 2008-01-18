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
typedef unsigned __int32 (*AACReadCallback)(void *userData, void *pBuffer, unsigned long nBytesToRead);
typedef __int32 (*AACSeekCallback)(void *userData, unsigned __int64 pos);
typedef __int64 (*AACFileSizeCallback)(void *userData);

/* io callback structure */
typedef struct AACIOCallbacks
{
  AACOpenCallback Open;
  AACCloseCallback Close;
  AACReadCallback Read;
  AACSeekCallback Seek;
  AACFileSizeCallback Filesize;
  void *userData;
} AACIOCallbacks;

typedef enum AAC_OBJECT_TYPE
{
  AAC_MAIN=1, /* MAIN */
  AAC_LC=2, /* Low Complexity */
  AAC_SSR=3, /* Scalable SampleRate */
  AAC_LTP=4, /* Long Term Predition */
  AAC_HE=5, /* High Efficiency (SBR) */
  AAC_ER_LC=17, /* Error Resilient Low Complexity */
  AAC_ER_LTP=19, /* Error Resilient Long Term Prediction */
  AAC_LD=23, /* Low Delay */
  ALAC=24   /* ALAC codec */
} AAC_OBJECT_TYPE;

/* info about a file */
typedef struct AACInfo
{
  int samplerate;
  int channels;
  int bitspersample;
  int totaltime;
  int bitrate;

  AAC_OBJECT_TYPE objecttype;

  char* replaygain_track_gain;
  char* replaygain_album_gain;
  char* replaygain_track_peak;
  char* replaygain_album_peak;
} AACInfo;

/* possible return values of AACRead */
#define AAC_READ_EOF              -1
#define AAC_READ_ERROR            -2
#define AAC_READ_BUFFER_TO_SMALL  -3

/* A decode call can eat up to AAC_PCM_SIZE bytes per decoded channel,
   so at least so much bytes per channel should be available */
#define AAC_PCM_SIZE 2048*sizeof(short)

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

