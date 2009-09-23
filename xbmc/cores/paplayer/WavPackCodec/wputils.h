////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2005 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// wputils.h

#ifndef WPUTILS_H
#define WPUTILS_H

// This header file contains all the definitions required to use the
// functions in "wputils.c" to read and write WavPack files and streams.

#include <sys/types.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include <stdlib.h>
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;
typedef __int64 int64_t;
typedef __int32 int32_t;
typedef __int16 int16_t;
typedef __int8  int8_t;
typedef float float32_t;
#else
#include <inttypes.h>
#endif

typedef unsigned char	uchar;

#if !defined(__GNUC__) || defined(WIN32)
typedef unsigned short	ushort;
typedef unsigned int	uint;
#endif

///////////////////////// WavPack Configuration ///////////////////////////////

// This external structure is used during encode to provide configuration to
// the encoding engine and during decoding to provide fle information back to
// the higher level functions. Not all fields are used in both modes.

typedef struct {
    float bitrate, shaping_weight;
    int bits_per_sample, bytes_per_sample;
    int qmode, flags, xmode, num_channels, float_norm_exp;
    int32_t block_samples, extra_flags, sample_rate, channel_mask;
    uchar md5_checksum [16], md5_read;
    int num_tag_strings;
    char **tag_strings;
} WavpackConfig;

#define CONFIG_HYBRID_FLAG	8	// hybrid mode
#define CONFIG_JOINT_STEREO	0x10	// joint stereo
#define CONFIG_HYBRID_SHAPE	0x40	// noise shape (hybrid mode only)
#define CONFIG_FAST_FLAG	0x200	// fast mode
#define CONFIG_HIGH_FLAG	0x800	// high quality mode
#define CONFIG_BITRATE_KBPS	0x2000	// bitrate is kbps, not bits / sample
#define CONFIG_SHAPE_OVERRIDE	0x8000	// shaping mode specified
#define CONFIG_JOINT_OVERRIDE	0x10000	// joint-stereo mode specified
#define CONFIG_CREATE_WVC	0x80000	// create correction file
#define CONFIG_OPTIMIZE_WVC	0x100000 // maximize bybrid compression
#define CONFIG_CALC_NOISE	0x800000 // calc noise in hybrid mode
#define CONFIG_EXTRA_MODE	0x2000000 // extra processing mode
#define CONFIG_SKIP_WVX		0x4000000 // no wvx stream w/ floats & big ints

////////////// Callbacks used for reading & writing WavPack streams //////////

typedef struct {
    int32_t (*read_bytes)(void *id, void *data, int32_t bcount);
    uint32_t (*get_pos)(void *id);
    int (*set_pos_abs)(void *id, uint32_t pos);
    int (*set_pos_rel)(void *id, int32_t delta, int mode);
    int (*push_back_byte)(void *id, int c);
    uint32_t (*get_length)(void *id);
    int (*can_seek)(void *id);
} stream_reader;

typedef int (*blockout)(void *id, void *data, int32_t bcount);

//////////////////////// function prototypes and macros //////////////////////

typedef void WavpackContext;

#ifdef __cplusplus
extern "C" {
#endif

WavpackContext *WavpackOpenFileInputEx (stream_reader *reader, void *wv_id, void *wvc_id, char *error, int flags, int norm_offset);
WavpackContext *WavpackOpenFileInput (const char *infilename, char *error, int flags, int norm_offset);

#define OPEN_WVC	0x1	// open/read "correction" file
#define OPEN_TAGS	0x2	// read ID3v1 / APEv2 tags (seekable file)
#define OPEN_WRAPPER	0x4	// make audio wrapper available (i.e. RIFF)
#define OPEN_2CH_MAX	0x8	// open multichannel as stereo (no downmix)
#define OPEN_NORMALIZE	0x10	// normalize floating point data to +/- 1.0
#define OPEN_STREAMING	0x20	// "streaming" mode blindly unpacks blocks
				// w/o regard to header file position info

int WavpackGetMode (WavpackContext *wpc);

#define MODE_WVC	0x1
#define MODE_LOSSLESS	0x2
#define MODE_HYBRID	0x4
#define MODE_FLOAT	0x8
#define MODE_VALID_TAG	0x10
#define MODE_HIGH	0x20
#define MODE_FAST	0x40
#define MODE_EXTRA	0x80
#define MODE_APETAG	0x100
#define MODE_SFX	0x200

int WavpackGetVersion (WavpackContext *wpc);
uint32_t WavpackUnpackSamples (WavpackContext *wpc, int32_t *buffer, uint32_t samples);
uint32_t WavpackGetNumSamples (WavpackContext *wpc);
uint32_t WavpackGetSampleIndex (WavpackContext *wpc);
int WavpackGetNumErrors (WavpackContext *wpc);
int WavpackLossyBlocks (WavpackContext *wpc);
int WavpackSeekSample (WavpackContext *wpc, uint32_t sample);
WavpackContext *WavpackCloseFile (WavpackContext *wpc);
uint32_t WavpackGetSampleRate (WavpackContext *wpc);
int WavpackGetBitsPerSample (WavpackContext *wpc);
int WavpackGetBytesPerSample (WavpackContext *wpc);
int WavpackGetNumChannels (WavpackContext *wpc);
int WavpackGetReducedChannels (WavpackContext *wpc);
int WavpackGetFloatNormExp (WavpackContext *wpc);
int WavpackGetMD5Sum (WavpackContext *wpc, uchar data [16]);
uint32_t WavpackGetWrapperBytes (WavpackContext *wpc);
uchar *WavpackGetWrapperData (WavpackContext *wpc);
void WavpackFreeWrapper (WavpackContext *wpc);
double WavpackGetProgress (WavpackContext *wpc);
uint32_t WavpackGetFileSize (WavpackContext *wpc);
double WavpackGetRatio (WavpackContext *wpc);
double WavpackGetAverageBitrate (WavpackContext *wpc, int count_wvc);
double WavpackGetInstantBitrate (WavpackContext *wpc);
int WavpackGetTagItem (WavpackContext *wpc, const char *item, char *value, int size);
int WavpackAppendTagItem (WavpackContext *wpc, const char *item, const char *value);
int WavpackWriteTag (WavpackContext *wpc);


WavpackContext *WavpackOpenFileOutput (blockout blockout, void *wv_id, void *wvc_id);
int WavpackSetConfiguration (WavpackContext *wpc, WavpackConfig *config, uint32_t total_samples);
int WavpackAddWrapper (WavpackContext *wpc, void *data, uint32_t bcount);
int WavpackStoreMD5Sum (WavpackContext *wpc, uchar data [16]);
int WavpackPackInit (WavpackContext *wpc);
int WavpackPackSamples (WavpackContext *wpc, int32_t *sample_buffer, uint32_t sample_count);
int WavpackFlushSamples (WavpackContext *wpc);
void WavpackUpdateNumSamples (WavpackContext *wpc, void *first_block);
void *WavpackGetWrapperLocation (void *first_block);

// this function is not actually in wputils.c, but is generally useful

void float_normalize (int32_t *values, int32_t num_values, int delta_exp);

#ifdef __cplusplus
}
#endif

#endif
