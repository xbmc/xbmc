#ifndef SHNPLAY_LIBRARY_INTERFACE__H
#define SHNPLAY_LIBRARY_INTERFACE__H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIBSHNPLAY_EXPORTS
# define SHNPLAY_API _declspec(dllexport)
#else
# define SHNPLAY_API _declspec(dllimport)
#endif

typedef struct ShnPlay ShnPlay;
typedef struct ShnPlayInfo ShnPlayInfo;
typedef struct ShnPlayStream ShnPlayStream;

/* returns 1 if some data was retrieved (even if less data than requested was read), 0 on failure */
typedef int (*ShnPlayStream_Read)(ShnPlayStream *, void * buffer, int bytes, int * bytes_read);
/* returns 1 if seeking was successful, 0 otherwise */
typedef int (*ShnPlayStream_Seek)(ShnPlayStream *, int position);
/* returns 1 if stream is seekable, 0 otherwise */
typedef int (*ShnPlayStream_CanSeek)(ShnPlayStream *);
/* returns length of stream, -1 if length can not be determined */
typedef int (*ShnPlayStream_GetLength)(ShnPlayStream *);
/* returns current position, -1 if position can not be determined */
typedef int (*ShnPlayStream_GetPosition)(ShnPlayStream *);

struct ShnPlayStream {
	ShnPlayStream_Read        Read;
	ShnPlayStream_Seek        Seek;
	ShnPlayStream_CanSeek     CanSeek;
	ShnPlayStream_GetLength   GetLength;
	ShnPlayStream_GetPosition GetPosition;
};

struct ShnPlayInfo {
	int channels; /* from Shorten header */
	int sample_rate; /* from WAV header */
	int sample_count; /* from WAV header */
	int bits_per_sample; /* from WAV header */

	int file_version; /* from Shorten header */
	int file_type; /* from Shorten header */
	int waveformat; /* from WAV header */
	int block_align; /* from WAV header */

	int internal_seektable; /* 1 if internal seektable is present and valid, 0 otherwise */
};

/** Created state should support playback */
#define SHNPLAY_OPEN_PLAY 0
/** Created state need not support playback */
#define SHNPLAY_OPEN_NOPLAY 1

/** Created state should support seeking */
#define SHNPLAY_OPEN_SEEK 0
/** Created state need not support seeking */
#define SHNPLAY_OPEN_NOSEEK 2

/**
Open shorten file using built-in stream implementation.
Filename is encoded in ANSI.
Returns 1 on success, 0 on failure.
state is always set to a valid pointer and must be closed using ShnPlay_Close.
DEPRECATED
*/
//SHNPLAY_API int ShnPlay_OpenFileA(ShnPlay ** pstate, const char * filename, unsigned int flags);


/**
Open shorten file using given stream.
Returns 1 on success, 0 on failure.
state is always set to a valid pointer and must be closed using ShnPlay_Close;
the given stream must be closed manually after closing the state.
*/
 int ShnPlay_OpenStream(ShnPlay ** pstate, ShnPlayStream * stream, unsigned int flags);

/**
Closes shorten file.
Returns 1 on success, 0 on failure.
All resources associated with the state are freed; the state shoud not be used
after this call.
*/
SHNPLAY_API int ShnPlay_Close(ShnPlay * state);

/**
Retrieve information about Shorten file.
Returns 1 on success, 0 on failure.
*/
SHNPLAY_API int ShnPlay_GetInfo(ShnPlay * state, ShnPlayInfo * info);

/**
Read decoded PCM data.
Returns 0 on failure, do not use the contents of buffer in this case.
If the return value is 1, samples_read contains the number of successfully retrieved
samples. It may be lower than samples, if the end of the file has been reached.
The buffer must be big enough to hold samples*channels*(bytes per sample) bytes.
16 bit data is represented as signed 2 integer in machine byte order.
8 bit data is represented as unsigned integer.
The output data uses channel-interleaving.
*/
SHNPLAY_API int ShnPlay_Read(ShnPlay * state, void * buffer, int samples, int * samples_read);

/**
Seek to sample.
Position is given as zero-based sample offset.
*/
SHNPLAY_API int ShnPlay_Seek(ShnPlay * state, int position);

/**
Get textual representation of last error.
*/
SHNPLAY_API const char * ShnPlay_ErrorMessage(ShnPlay * state);

#ifdef __cplusplus
/* extern "C" */ }
#endif

#endif
