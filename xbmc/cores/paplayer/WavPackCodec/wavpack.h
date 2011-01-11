////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2005 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// wavpack.h

#ifndef WAVPACK_H
#define WAVPACK_H

#if defined(WIN32)
#define FASTCALL __fastcall
#else
#define FASTCALL
#define SetConsoleTitle(x)
#endif

#if defined(WIN32)
#ifdef WAVPACKDLL_EXPORTS
#define WAVPACKAPI __declspec(dllexport)
#else
#define WAVPACKAPI
#endif
#else
#define WAVPACKAPI
#endif

#include <sys/types.h>

// This header file contains all the definitions required by WavPack.

#if defined(_WIN32) && !defined(__MINGW32__)
#include <stdlib.h>
#include <stdint.h>
#else
#include <inttypes.h>
#endif

typedef unsigned char	uchar;

#if !defined(__GNUC__) || defined(WIN32)
typedef unsigned short	ushort;
typedef unsigned int	uint;
#endif

#ifndef PATH_MAX
#ifdef MAX_PATH
#define PATH_MAX MAX_PATH
#elif defined (MAXPATHLEN)
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif
#endif

// This structure is used to access the individual fields of 32-bit ieee
// floating point numbers. This will not be compatible with compilers that
// allocate bit fields from the most significant bits, although I'm not sure
// how common that is.

typedef struct {
    unsigned mantissa : 23;
    unsigned exponent : 8;
    unsigned sign : 1;
} f32;

#include <stdio.h>

#define FALSE 0
#define TRUE 1

#if defined(WIN32)
#undef VERSION_OS
#define VERSION_OS "Win32"
#endif
#define VERSION_STR "4.2 "
#define DATE_STR "2005-04-02"

// ID3v1 and APEv2 TAG formats (may occur at the end of WavPack files)

typedef struct {
    uchar tag_id [3], title [30], artist [30], album [30];
    uchar year [4], comment [30], genre;
} ID3_Tag;

typedef struct {
    char ID [8];
    int32_t version, length, item_count, flags;
    char res [8];
} APE_Tag_Hdr;

#define APE_Tag_Hdr_Format "8LLLL"

typedef struct {
    ID3_Tag id3_tag;
    APE_Tag_Hdr ape_tag_hdr;
    char *ape_tag_data;
} M_Tag;

// RIFF / wav header formats (these occur at the beginning of both wav files
// and pre-4.0 WavPack files that are not in the "raw" mode)

typedef struct {
    char ckID [4];
    uint32_t ckSize;
    char formType [4];
} RiffChunkHeader;

typedef struct {
    char ckID [4];
    uint32_t ckSize;
} ChunkHeader;

#define ChunkHeaderFormat "4L"

typedef struct {
    ushort FormatTag, NumChannels;
    uint32_t SampleRate, BytesPerSecond;
    ushort BlockAlign, BitsPerSample;
    ushort cbSize, ValidBitsPerSample;
    int32_t ChannelMask;
    ushort SubFormat;
    char GUID [14];
} WaveHeader;

#define WaveHeaderFormat "SSLLSSSSLS"

////////////////////////////// WavPack Header /////////////////////////////////

// Note that this is the ONLY structure that is written to (or read from)
// WavPack 4.0 files, and is the preamble to every block in both the .wv
// and .wvc files.

typedef struct {
    char ckID [4];
    uint32_t ckSize;
    short version;
    uchar track_no, index_no;
    uint32_t total_samples, block_index, block_samples, flags, crc;
} WavpackHeader;

#define WavpackHeaderFormat "4LS2LLLLL"

// or-values for "flags"

#define BYTES_STORED	3	// 1-4 bytes/sample
#define MONO_FLAG	4	// not stereo
#define HYBRID_FLAG	8	// hybrid mode
#define JOINT_STEREO	0x10	// joint stereo
#define CROSS_DECORR	0x20	// no-delay cross decorrelation
#define HYBRID_SHAPE	0x40	// noise shape (hybrid mode only)
#define FLOAT_DATA	0x80	// ieee 32-bit floating point data

#define INT32_DATA	0x100	// special extended int handling
#define HYBRID_BITRATE	0x200	// bitrate noise (hybrid mode only)
#define HYBRID_BALANCE	0x400	// balance noise (hybrid stereo mode only)

#define INITIAL_BLOCK	0x800	// initial block of multichannel segment
#define FINAL_BLOCK	0x1000	// final block of multichannel segment

#define SHIFT_LSB	13
#define SHIFT_MASK	(0x1fL << SHIFT_LSB)

#define MAG_LSB		18
#define MAG_MASK	(0x1fL << MAG_LSB)

#define SRATE_LSB	23
#define SRATE_MASK	(0xfL << SRATE_LSB)

#define IGNORED_FLAGS	0x18000000	// reserved, but ignore if encountered
#define NEW_SHAPING	0x20000000	// use IIR filter for negative shaping
#define UNKNOWN_FLAGS	0xC0000000	// also reserved, but refuse decode if
					//  encountered

//////////////////////////// WavPack Metadata /////////////////////////////////

// This is an internal representation of metadata.

typedef struct {
    int32_t byte_length;
    void *data;
    uchar id;
} WavpackMetadata;

#define ID_OPTIONAL_DATA	0x20
#define ID_ODD_SIZE		0x40
#define ID_LARGE		0x80

#define ID_DUMMY		0x0
#define ID_ENCODER_INFO		0x1
#define ID_DECORR_TERMS		0x2
#define ID_DECORR_WEIGHTS	0x3
#define ID_DECORR_SAMPLES	0x4
#define ID_ENTROPY_VARS		0x5
#define ID_HYBRID_PROFILE	0x6
#define ID_SHAPING_WEIGHTS	0x7
#define ID_FLOAT_INFO		0x8
#define ID_INT32_INFO		0x9
#define ID_WV_BITSTREAM		0xa
#define ID_WVC_BITSTREAM	0xb
#define ID_WVX_BITSTREAM	0xc
#define ID_CHANNEL_INFO		0xd

#define ID_RIFF_HEADER		(ID_OPTIONAL_DATA | 0x1)
#define ID_RIFF_TRAILER		(ID_OPTIONAL_DATA | 0x2)
#define ID_REPLAY_GAIN		(ID_OPTIONAL_DATA | 0x3)
#define ID_CUESHEET		(ID_OPTIONAL_DATA | 0x4)
#define ID_CONFIG_BLOCK		(ID_OPTIONAL_DATA | 0x5)
#define ID_MD5_CHECKSUM		(ID_OPTIONAL_DATA | 0x6)

///////////////////////// WavPack Configuration ///////////////////////////////

// This internal structure is used during encode to provide configuration to
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

#define CONFIG_BYTES_STORED	3	// 1-4 bytes/sample
#define CONFIG_MONO_FLAG	4	// not stereo
#define CONFIG_HYBRID_FLAG	8	// hybrid mode
#define CONFIG_JOINT_STEREO	0x10	// joint stereo
#define CONFIG_CROSS_DECORR	0x20	// no-delay cross decorrelation
#define CONFIG_HYBRID_SHAPE	0x40	// noise shape (hybrid mode only)
#define CONFIG_FLOAT_DATA	0x80	// ieee 32-bit floating point data

#define CONFIG_ADOBE_MODE	0x100	// "adobe" mode for 32-bit floats
#define CONFIG_FAST_FLAG	0x200	// fast mode
#define CONFIG_VERY_FAST_FLAG	0x400	// double fast
#define CONFIG_HIGH_FLAG	0x800	// high quality mode
#define CONFIG_VERY_HIGH_FLAG	0x1000	// double high (not used yet)
#define CONFIG_BITRATE_KBPS	0x2000	// bitrate is kbps, not bits / sample
#define CONFIG_AUTO_SHAPING	0x4000	// automatic noise shaping
#define CONFIG_SHAPE_OVERRIDE	0x8000	// shaping mode specified
#define CONFIG_JOINT_OVERRIDE	0x10000	// joint-stereo mode specified
#define CONFIG_COPY_TIME	0x20000	// copy file-time from source
#define CONFIG_CREATE_EXE	0x40000	// create executable
#define CONFIG_CREATE_WVC	0x80000	// create correction file
#define CONFIG_OPTIMIZE_WVC	0x100000 // maximize bybrid compression
#define CONFIG_QUALITY_MODE	0x200000 // psychoacoustic quality mode
#define CONFIG_RAW_FLAG		0x400000 // raw mode (not implemented yet)
#define CONFIG_CALC_NOISE	0x800000 // calc noise in hybrid mode
#define CONFIG_LOSSY_MODE	0x1000000 // obsolete (for information)
#define CONFIG_EXTRA_MODE	0x2000000 // extra processing mode
#define CONFIG_SKIP_WVX		0x4000000 // no wvx stream w/ floats & big ints
#define CONFIG_MD5_CHECKSUM	0x8000000 // compute & store MD5 signature
#define CONFIG_QUIET_MODE	0x10000000 // don't report progress %
#define CONFIG_IGNORE_LENGTH	0x20000000 // ignore length in wav header

#define EXTRA_SCAN_ONLY		1
#define EXTRA_STEREO_MODES	2
//#define EXTRA_CHECK_TERMS	4
#define EXTRA_TRY_DELTAS	8
#define EXTRA_ADJUST_DELTAS	16
#define EXTRA_SORT_FIRST	32
#define EXTRA_BRANCHES		0x1c0
#define EXTRA_SKIP_8TO16	512
#define EXTRA_TERMS		0x3c00
#define EXTRA_DUMP_TERMS	16384
#define EXTRA_SORT_LAST		32768

//////////////////////////////// WavPack Stream ///////////////////////////////

// This internal structure contains everything required to handle a WavPack
// "stream", which is defined as a stereo or mono stream of audio samples. For
// multichannel audio several of these would be required. Each stream contains
// pointers to hold a complete allocated block of WavPack data, although it's
// possible to decode WavPack blocks without buffering an entire block.

typedef struct bs {
    uchar *buf, *end, *ptr;
    void (*wrap)(struct bs *bs);
    int error, bc;
    uint32_t sr;
} Bitstream;

#define MAX_STREAMS 8
#define MAX_NTERMS 16
#define MAX_TERM 8

struct decorr_pass {
    int term, delta, weight_A, weight_B;
    int32_t samples_A [MAX_TERM], samples_B [MAX_TERM];
    int32_t aweight_A, aweight_B;
#ifdef PACK
    int32_t sum_A, sum_B, min, max;
#endif
};

typedef struct {
    WavpackHeader wphdr;

    uchar *blockbuff, *blockend;
    uchar *block2buff, *block2end;
    int32_t *sample_buffer;

    uint32_t sample_index, crc, crc_x, crc_wvx;
    Bitstream wvbits, wvcbits, wvxbits;
    int bits, num_terms, mute_error;
    float delta_decay;

    uchar int32_sent_bits, int32_zeros, int32_ones, int32_dups;
    uchar float_flags, float_shift, float_max_exp, float_norm_exp;
 
    struct {
	int32_t shaping_acc [2], shaping_delta [2], error [2];
	double noise_sum, noise_ave, noise_max;
    } dc;

    struct decorr_pass decorr_passes [MAX_NTERMS];

    struct {
	uint32_t bitrate_delta [2], bitrate_acc [2];
	uint32_t median [3] [2], slow_level [2], error_limit [2];
	uint32_t pend_data, holding_one, zeros_acc;
	int holding_zero, pend_count;
    } w;
} WavpackStream;

// flags for float_flags:

#define FLOAT_SHIFT_ONES 1	// bits left-shifted into float = '1'
#define FLOAT_SHIFT_SAME 2	// bits left-shifted into float are the same
#define FLOAT_SHIFT_SENT 4	// bits shifted into float are sent literally
#define FLOAT_ZEROS_SENT 8	// "zeros" are not all real zeros
#define FLOAT_NEG_ZEROS  0x10	// contains negative zeros
#define FLOAT_EXCEPTIONS 0x20	// contains exceptions (inf, nan, etc.)

/////////////////////////////// WavPack Context ///////////////////////////////

// This internal structure holds everything required to encode or decode WavPack
// files. It is recommended that direct access to this structure be minimized
// and the provided utilities used instead.

typedef struct {
    int32_t (*read_bytes)(void *id, void *data, int32_t bcount);
    uint32_t (*get_pos)(void *id);
    int (*set_pos_abs)(void *id, uint32_t pos);
    int (*set_pos_rel)(void *id, int32_t delta, int mode);
    int (*push_back_byte)(void *id, int c);
    uint32_t (*get_length)(void *id);
    int (*can_seek)(void *id);
} stream_reader;

typedef int (*blockout_f)(void *id, void *data, int32_t bcount);

typedef struct {
    WavpackConfig config;

    WavpackMetadata *metadata;
    uint32_t metabytes;
    int metacount;

    uchar *wrapper_data;
    uint32_t wrapper_bytes;

    blockout_f blockout;
    void *wv_out, *wvc_out;

    stream_reader *reader;
    void *wv_in, *wvc_in;

    uint32_t filelen, file2len, filepos, file2pos, total_samples, crc_errors, first_flags;
    int wvc_flag, open_flags, norm_offset, reduced_channels, lossy_blocks, close_files;
    int block_samples, max_samples, acc_samples;
    M_Tag m_tag;

    int current_stream, num_streams;
    WavpackStream *streams [8];
    void *stream3;

    char error_message [80];
} WavpackContext;

//////////////////////// function prototypes and macros //////////////////////

#define CLEAR(destin) memset (&destin, 0, sizeof (destin));

// these macros implement the weight application and update operations
// that are at the heart of the decorrelation loops

#define apply_weight_i(weight, sample) ((weight * sample + 512) >> 10)

#define apply_weight_f(weight, sample) (((((sample & 0xffff) * weight) >> 9) + \
    (((sample & ~0xffff) >> 9) * weight) + 1) >> 1)

#if 1	// PERFCOND
#define apply_weight(weight, sample) (sample != (short) sample ? \
    apply_weight_f (weight, sample) : apply_weight_i (weight, sample))
#else
#define apply_weight(weight, sample) ((int32_t)((weight * (int64_t) sample + 512) >> 10))
#endif

#if 1	// PERFCOND
#define update_weight(weight, delta, source, result) \
    if (source && result) weight -= ((((source ^ result) >> 30) & 2) - 1) * delta;
#else
#define update_weight(weight, delta, source, result) \
    if (source && result) (source ^ result) < 0 ? (weight -= delta) : (weight += delta);
#endif

#define update_weight_d1(weight, delta, source, result) \
    if (source && result) weight -= (((source ^ result) >> 30) & 2) - 1;

#define update_weight_d2(weight, delta, source, result) \
    if (source && result) weight -= (((source ^ result) >> 29) & 4) - 2;

#define update_weight_clip(weight, delta, source, result) \
    if (source && result && ((source ^ result) < 0 ? (weight -= delta) < -1024 : (weight += delta) > 1024)) \
	weight = weight < 0 ? -1024 : 1024;

#define update_weight_clip_d1(weight, delta, source, result) \
    if (source && result && abs (weight -= (((source ^ result) >> 30) & 2) - 1) > 1024) \
	weight = weight < 0 ? -1024 : 1024;

#define update_weight_clip_d2(weight, delta, source, result) \
    if (source && result && abs (weight -= (((source ^ result) >> 29) & 4) - 2) > 1024) \
	weight = weight < 0 ? -1024 : 1024;

// bits.c

void bs_open_read (Bitstream *bs, uchar *buffer_start, uchar *buffer_end);
void bs_open_write (Bitstream *bs, uchar *buffer_start, uchar *buffer_end);
uint32_t bs_close_read (Bitstream *bs);
uint32_t bs_close_write (Bitstream *bs);

int DoReadFile (FILE *hFile, void *lpBuffer, uint32_t nNumberOfBytesToRead, uint32_t *lpNumberOfBytesRead);
int DoWriteFile (FILE *hFile, void *lpBuffer, uint32_t nNumberOfBytesToWrite, uint32_t *lpNumberOfBytesWritten);
uint32_t DoGetFileSize (FILE *hFile), DoGetFilePosition (FILE *hFile);
int DoSetFilePositionRelative (FILE *hFile, int32_t pos, int mode);
int DoSetFilePositionAbsolute (FILE *hFile, uint32_t pos);
int DoUngetc (int c, FILE *hFile), DoDeleteFile (char *filename);
int DoCloseHandle (FILE *hFile), DoTruncateFile (FILE *hFile);

#define bs_is_open(bs) ((bs)->ptr != NULL)

#define getbit(bs) ( \
    (((bs)->bc) ? \
	((bs)->bc--, (bs)->sr & 1) : \
	    (((++((bs)->ptr) != (bs)->end) ? (void) 0 : (bs)->wrap (bs)), (bs)->bc = 7, ((bs)->sr = *((bs)->ptr)) & 1) \
    ) ? \
	((bs)->sr >>= 1, 1) : \
	((bs)->sr >>= 1, 0) \
)

#define getbits(value, nbits, bs) { \
    while ((nbits) > (bs)->bc) { \
	if (++((bs)->ptr) == (bs)->end) (bs)->wrap (bs); \
	(bs)->sr |= (int32_t)*((bs)->ptr) << (bs)->bc; \
	(bs)->bc += 8; \
    } \
    *(value) = (bs)->sr; \
    (bs)->sr >>= (nbits); \
    (bs)->bc -= (nbits); \
}

#define putbit(bit, bs) { if (bit) (bs)->sr |= (1 << (bs)->bc); \
    if (++((bs)->bc) == 8) { \
	*((bs)->ptr) = (bs)->sr; \
	(bs)->sr = (bs)->bc = 0; \
	if (++((bs)->ptr) == (bs)->end) (bs)->wrap (bs); \
    }}

#define putbit_0(bs) { \
    if (++((bs)->bc) == 8) { \
	*((bs)->ptr) = (bs)->sr; \
	(bs)->sr = (bs)->bc = 0; \
	if (++((bs)->ptr) == (bs)->end) (bs)->wrap (bs); \
    }}

#define putbit_1(bs) { (bs)->sr |= (1 << (bs)->bc); \
    if (++((bs)->bc) == 8) { \
	*((bs)->ptr) = (bs)->sr; \
	(bs)->sr = (bs)->bc = 0; \
	if (++((bs)->ptr) == (bs)->end) (bs)->wrap (bs); \
    }}

#define putbits(value, nbits, bs) { \
    (bs)->sr |= (int32_t)(value) << (bs)->bc; \
    if (((bs)->bc += (nbits)) >= 8) \
	do { \
	    *((bs)->ptr) = (bs)->sr; \
	    (bs)->sr >>= 8; \
	    if (++((bs)->ptr) == (bs)->end) (bs)->wrap (bs); \
	} while (((bs)->bc -= 8) >= 8); \
}

void little_endian_to_native (void *data, char *format);
void native_to_little_endian (void *data, char *format);

// pack.c

void pack_init (WavpackContext *wpc);
int pack_block (WavpackContext *wpc, int32_t *buffer);
double pack_noise (WavpackContext *wpc, double *peak);

// unpack.c

int unpack_init (WavpackContext *wpc);
int init_wv_bitstream (WavpackStream *wps, WavpackMetadata *wpmd);
int init_wvc_bitstream (WavpackStream *wps, WavpackMetadata *wpmd);
int init_wvx_bitstream (WavpackStream *wps, WavpackMetadata *wpmd);
int read_decorr_terms (WavpackStream *wps, WavpackMetadata *wpmd);
int read_decorr_weights (WavpackStream *wps, WavpackMetadata *wpmd);
int read_decorr_samples (WavpackStream *wps, WavpackMetadata *wpmd);
int read_shaping_info (WavpackStream *wps, WavpackMetadata *wpmd);
int read_float_info (WavpackStream *wps, WavpackMetadata *wpmd);
int read_int32_info (WavpackStream *wps, WavpackMetadata *wpmd);
int read_channel_info (WavpackContext *wpc, WavpackMetadata *wpmd);
int read_config_info (WavpackContext *wpc, WavpackMetadata *wpmd);
int read_wrapper_data (WavpackContext *wpc, WavpackMetadata *wpmd);
int32_t unpack_samples (WavpackContext *wpc, int32_t *buffer, uint32_t sample_count);
int check_crc_error (WavpackContext *wpc);

// unpack3.c

WavpackContext *open_file3 (WavpackContext *wpc, char *error);
int32_t unpack_samples3 (WavpackContext *wpc, int32_t *buffer, uint32_t sample_count);
int seek_sample3 (WavpackContext *wpc, uint32_t desired_index);
uint32_t get_sample_index3 (WavpackContext *wpc);
void free_stream3 (WavpackContext *wpc);
int get_version3 (WavpackContext *wpc);

// utils.c

int copy_timestamp (const char *src_filename, const char *dst_filename);
char *filespec_ext (char *filespec), *filespec_path (char *filespec);
char *filespec_name (char *filespec), *filespec_wild (char *filespec);
void error_line (char *error, ...), finish_line (void);
void setup_break (void);
int check_break (void);
char yna (void);
void AnsiToUTF8 (char *string, int len);

#define FN_FIT(fn) ((strlen (fn) > 30) ? filespec_name (fn) : fn)

// metadata.c stuff

int read_metadata_buff (WavpackMetadata *wpmd, uchar *blockbuff, uchar **buffptr);
int write_metadata_block (WavpackContext *wpc);
int copy_metadata (WavpackMetadata *wpmd, uchar *buffer_start, uchar *buffer_end);
int add_to_metadata (WavpackContext *wpc, void *data, uint32_t bcount, uchar id);
int process_metadata (WavpackContext *wpc, WavpackMetadata *wpmd);
void free_metadata (WavpackMetadata *wpmd);

// words.c stuff

void init_words (WavpackStream *wps);
void word_set_bitrate (WavpackStream *wps);
void write_entropy_vars (WavpackStream *wps, WavpackMetadata *wpmd);
void write_hybrid_profile (WavpackStream *wps, WavpackMetadata *wpmd);
int read_entropy_vars (WavpackStream *wps, WavpackMetadata *wpmd);
int read_hybrid_profile (WavpackStream *wps, WavpackMetadata *wpmd);
int32_t FASTCALL send_word (WavpackStream *wps, int32_t value, int chan);
void FASTCALL send_word_lossless (WavpackStream *wps, int32_t value, int chan);
int32_t FASTCALL get_word (WavpackStream *wps, int chan, int32_t *correction);
int32_t FASTCALL get_word_lossless (WavpackStream *wps, int chan);
void flush_word (WavpackStream *wps);
int32_t nosend_word (WavpackStream *wps, int32_t value, int chan);
void scan_word (WavpackStream *wps, int32_t *samples, uint32_t num_samples, int dir);

int log2s (int32_t value);
int32_t exp2s (int log);
uint32_t log2buffer (int32_t *samples, uint32_t num_samples);

char store_weight (int weight);
int restore_weight (char weight);

#define WORD_EOF (1L << 31)

// float.c

void write_float_info (WavpackStream *wps, WavpackMetadata *wpmd);
int scan_float_data (WavpackStream *wps, f32 *values, int32_t num_values);
void send_float_data (WavpackStream *wps, f32 *values, int32_t num_values);
int read_float_info (WavpackStream *wps, WavpackMetadata *wpmd);
void float_values (WavpackStream *wps, int32_t *values, int32_t num_values);
void float_normalize (int32_t *values, int32_t num_values, int delta_exp);

// analyze?.c

void analyze_stereo (WavpackContext *wpc, int32_t *samples);
void analyze_mono (WavpackContext *wpc, int32_t *samples);

// wputils.c

WAVPACKAPI WavpackContext *WavpackOpenFileInputEx (stream_reader *reader, void *wv_id, void *wvc_id, char *error, int flags, int norm_offset);
WAVPACKAPI WavpackContext *WavpackOpenFileInput (const char *infilename, char *error, int flags, int norm_offset);

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

WAVPACKAPI int WavpackGetVersion (WavpackContext *wpc);
WAVPACKAPI uint32_t WavpackUnpackSamples (WavpackContext *wpc, int32_t *buffer, uint32_t samples);
WAVPACKAPI uint32_t WavpackGetNumSamples (WavpackContext *wpc);
WAVPACKAPI uint32_t WavpackGetSampleIndex (WavpackContext *wpc);
WAVPACKAPI int WavpackGetNumErrors (WavpackContext *wpc);
WAVPACKAPI int WavpackLossyBlocks (WavpackContext *wpc);
WAVPACKAPI int WavpackSeekSample (WavpackContext *wpc, uint32_t sample);
WAVPACKAPI WavpackContext *WavpackCloseFile (WavpackContext *wpc);
WAVPACKAPI uint32_t WavpackGetSampleRate (WavpackContext *wpc);
WAVPACKAPI int WavpackGetBitsPerSample (WavpackContext *wpc);
WAVPACKAPI int WavpackGetBytesPerSample (WavpackContext *wpc);
WAVPACKAPI int WavpackGetNumChannels (WavpackContext *wpc);
WAVPACKAPI int WavpackGetReducedChannels (WavpackContext *wpc);
WAVPACKAPI int WavpackGetMD5Sum (WavpackContext *wpc, uchar data [16]);
WAVPACKAPI uint32_t WavpackGetWrapperBytes (WavpackContext *wpc);
WAVPACKAPI uchar *WavpackGetWrapperData (WavpackContext *wpc);
WAVPACKAPI void WavpackFreeWrapper (WavpackContext *wpc);
WAVPACKAPI double WavpackGetProgress (WavpackContext *wpc);
WAVPACKAPI uint32_t WavpackGetFileSize (WavpackContext *wpc);
WAVPACKAPI double WavpackGetRatio (WavpackContext *wpc);
WAVPACKAPI double WavpackGetAverageBitrate (WavpackContext *wpc, int count_wvc);
WAVPACKAPI double WavpackGetInstantBitrate (WavpackContext *wpc);
WAVPACKAPI int WavpackGetTagItem (WavpackContext *wpc, const char *item, char *value, int size);
WAVPACKAPI int WavpackAppendTagItem (WavpackContext *wpc, const char *item, const char *value);
WAVPACKAPI int WavpackWriteTag (WavpackContext *wpc);

WAVPACKAPI WavpackContext *WavpackOpenFileOutput (blockout_f blockout, void *wv_id, void *wvc_id);
WAVPACKAPI int WavpackSetConfiguration (WavpackContext *wpc, WavpackConfig *config, uint32_t total_samples);
WAVPACKAPI int WavpackAddWrapper (WavpackContext *wpc, void *data, uint32_t bcount);
WAVPACKAPI int WavpackStoreMD5Sum (WavpackContext *wpc, uchar data [16]);
WAVPACKAPI int WavpackPackInit (WavpackContext *wpc);
WAVPACKAPI int WavpackPackSamples (WavpackContext *wpc, int32_t *sample_buffer, uint32_t sample_count);
WAVPACKAPI int WavpackFlushSamples (WavpackContext *wpc);
WAVPACKAPI void WavpackUpdateNumSamples (WavpackContext *wpc, void *first_block);
WAVPACKAPI void *WavpackGetWrapperLocation (void *first_block);

#endif
