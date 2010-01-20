////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2005 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// wavpack3.h

// This header file contains all the additional definitions required for
// decoding old (versions 1, 2 & 3) WavPack files.

typedef struct {
    ushort FormatTag, NumChannels;
    uint32_t SampleRate, BytesPerSecond;
    ushort BlockAlign, BitsPerSample;
} WaveHeader3;

#define WaveHeader3Format "SSLLSS"

typedef struct {
    char ckID [4];
    int32_t ckSize;
    short version;
    short bits;			// added for version 2.00
    short flags, shift;		// added for version 3.00
    int32_t total_samples, crc, crc2;
    char extension [4], extra_bc, extras [3];
} WavpackHeader3;

#define WavpackHeader3Format "4LSSSSLLL4L"

// these flags added for version 3

#undef MONO_FLAG		// these definitions changed for WavPack 4.0
#undef CROSS_DECORR
#undef JOINT_STEREO

#define MONO_FLAG	1	// not stereo
#define FAST_FLAG	2	// non-adaptive predictor and stereo mode
#define RAW_FLAG	4	// raw mode (no .wav header)
#define CALC_NOISE	8	// calc noise in lossy mode (no longer stored)
#define HIGH_FLAG	0x10	// high quality mode (all modes)
#define BYTES_3		0x20	// files have 3-byte samples
#define OVER_20		0x40	// samples are over 20 bits
#define WVC_FLAG	0x80	// create/use .wvc (no longer stored)
#define LOSSY_SHAPE	0x100	// noise shape (lossy mode only)
#define VERY_FAST_FLAG	0x200	// double fast (no longer stored)
#define NEW_HIGH_FLAG	0x400	// new high quality mode (lossless only)
#define CANCEL_EXTREME	0x800	// cancel EXTREME_DECORR
#define CROSS_DECORR	0x1000	// decorrelate chans (with EXTREME_DECORR flag)
#define NEW_DECORR_FLAG	0x2000	// new high-mode decorrelator
#define JOINT_STEREO	0x4000	// joint stereo (lossy and high lossless)
#define EXTREME_DECORR	0x8000	// extra decorrelation (+ enables other flags)

#define STORED_FLAGS	0xfd77	// these are only flags that affect unpacking
#define NOT_STORED_FLAGS (~STORED_FLAGS & 0xffff)

// BitStream stuff (bits.c)

typedef struct bs3 {
    void (*wrap)(struct bs3 *bs);
    uchar *buf, *end, *ptr;
    uint32_t bufsiz, fpos, sr;
    stream_reader *reader;
    int error, bc;
    void *id;
} Bitstream3;

#define K_DEPTH 3
#define MAX_NTERMS3 18

typedef struct {
    WavpackHeader3 wphdr;
    Bitstream3 wvbits, wvcbits;
    uint32_t sample_index;
    int num_terms;

#ifdef SEEKING
    struct index_point {
	char saved;
	uint32_t sample_index;
    } index_points [256];

    uchar *unpack_data;
    uint32_t unpack_size;
#endif

    struct {
	int32_t sum_level, left_level, right_level, diff_level;
	int last_extra_bits, extra_bits_count, m;
	int32_t error [2], crc;
	int32_t sample [2] [2];
	int weight [2] [1];
    } dc;

    struct decorr_pass decorr_passes [MAX_NTERMS3];

    struct {
	uint index [2], k_value [2], ave_k [2];
	uint32_t zeros_acc, ave_level [K_DEPTH] [2];
    } w1;

    struct { int last_dbits [2], last_delta_sign [2], bit_limit; } w2;

    struct { int ave_dbits [2], bit_limit; } w3;

    struct {
	uint32_t fast_level [2], slow_level [2];
	int bits_acc [2], bitrate;
    } w4;
} WavpackStream3;
