/*
 * vgmstream.h - definitions for VGMSTREAM, encapsulating a multi-channel, looped audio stream
 */

#ifndef _VGMSTREAM_H
#define _VGMSTREAM_H

/* Vorbis and MPEG decoding are done by external libraries.
 * If someone wants to do a standalone build, they can do it by simply
 * removing these defines (and the references to the libraries in the
 * Makefile) */
//#define VGM_USE_VORBIS
//#define VGM_USE_MPEG

#include "streamfile.h"
#include "coding/g72x_state.h"
#ifdef VGM_USE_VORBIS
#include <vorbis/vorbisfile.h>
#endif
#ifdef VGM_USE_MPEG
#include <mpg123.h>
#endif
#include "coding/acm_decoder.h"
#include "coding/nwa_decoder.h"

/* The encoding type specifies the format the sound data itself takes */
typedef enum {
    /* 16-bit PCM */
    coding_PCM16BE,         /* big endian 16-bit PCM */
    coding_PCM16LE,         /* little endian 16-bit PCM */
    coding_PCM16LE_int,		/* little endian 16-bit PCM with sample-level
                               interleave handled by the decoder */

    /* 8-bit PCM */
    coding_PCM8,            /* 8-bit PCM */
    coding_PCM8_int,		/* 8-Bit PCM with sample-level interleave handled
                               by the decoder */
    coding_PCM8_SB_int,     /* 8-bit PCM, sign bit (others are 2's complement),
                               sample-level interleave */
    coding_PCM8_U_int,      /* 8-bit PCM, unsigned (0x80 = 0), sample-level
                               interleave */

    /* 4-bit ADPCM */
    coding_NDS_IMA,         /* IMA ADPCM w/ NDS layout */
    coding_CRI_ADX,         /* CRI ADX */
    coding_CRI_ADX_enc,     /* encrypted CRI ADX */
    coding_NGC_DSP,         /* NGC ADPCM, called DSP */
    coding_NGC_DTK,         /* NGC hardware disc ADPCM, called DTK, TRK or ADP */
    coding_G721,            /* CCITT G.721 ADPCM */
    coding_NGC_AFC,         /* NGC ADPCM, called AFC */
    coding_PSX,				/* PSX & PS2 ADPCM */
    coding_invert_PSX,      /* PSX ADPCM with some weirdness */
    coding_PSX_badflags,    /* with garbage in the flags byte */
    coding_FFXI,            /* FF XI PSX-ish ADPCM */
    coding_XA,				/* PSX CD-XA */
    coding_XBOX,			/* XBOX IMA */
    coding_EAXA,			/* EA/XA ADPCM */
    coding_EA_ADPCM,		/* EA ADPCM */
    coding_NDS_PROCYON,     /* NDS Procyon Studio ADPCM */

#ifdef VGM_USE_VORBIS
    coding_ogg_vorbis,      /* vorbis */
#endif
    coding_SDX2,            /* SDX2 2:1 Squareroot-Delta-Exact compression */
    coding_SDX2_int,        /* SDX2 2:1 Squareroot-Delta-Exact compression,
                               with smaple-level interleave handled by the
                               decoder */
    coding_DVI_IMA,         /* DVI (bare IMA, high nibble first), aka ADP4 */
    coding_INT_DVI_IMA,		/* Interleaved DVI */
    coding_EACS_IMA,
    coding_IMA,             /* bare IMA, low nibble first */
    coding_INT_IMA,         /* */
    coding_WS,              /* Westwood Studios' custom VBR ADPCM */
#ifdef VGM_USE_MPEG
    coding_fake_MPEG2_L2,   /* MPEG-2 Layer 2 (AHX), with lying headers */
    /* I don't even know offhand if all these combinations exist... */
    coding_MPEG1_L1,
    coding_MPEG1_L2,
    coding_MPEG1_L3,        /* good ol' MPEG-1 Layer 3 (MP3) */
    coding_MPEG2_L1,
    coding_MPEG2_L2,
    coding_MPEG2_L3,
    coding_MPEG25_L1,
    coding_MPEG25_L2,
    coding_MPEG25_L3,
#endif

    coding_ACM,             /* InterPlay ACM */
    /* compressed NWA at various levels */
    coding_NWA0,
    coding_NWA1,
    coding_NWA2,
    coding_NWA3,
    coding_NWA4,
    coding_NWA5,

    coding_MSADPCM,         /* Microsoft ADPCM */
    coding_AICA,            /* Yamaha AICA ADPCM */
    coding_L5_555,          /* Level-5 0x555 */
} coding_t;

/* The layout type specifies how the sound data is laid out in the file */
typedef enum {
    /* generic */
    layout_none,            /* straight data */
    /* interleave */
    layout_interleave,      /* equal interleave throughout the stream */
    layout_interleave_shortblock, /* interleave with a short last block */

    layout_interleave_byte,  /* full byte interleave  */

    /* headered blocks */
    layout_ast_blocked,     /* .ast STRM with BLCK blocks*/
    layout_halpst_blocked,    /* blocks with HALPST-format header */
    layout_xa_blocked,
    layout_ea_blocked,
    layout_eacs_blocked,
    layout_caf_blocked,
    layout_wsi_blocked,
    layout_str_snds_blocked,
    layout_ws_aud_blocked,
    layout_matx_blocked,
    layout_de2_blocked,
    layout_xvas_blocked,
    layout_vs_blocked,
    layout_emff_ps2_blocked,
    layout_emff_ngc_blocked,
    layout_gsb_blocked,
    layout_thp_blocked,
    layout_filp_blocked,

#if 0
    layout_strm_blocked,    /* */
#endif
    /* otherwise odd */
    layout_dtk_interleave,  /* dtk interleaves channels by nibble */
#ifdef VGM_USE_VORBIS
    layout_ogg_vorbis,      /* ogg vorbis file */
#endif
#ifdef VGM_USE_MPEG
    layout_fake_mpeg,       /* MPEG audio stream with bad frame headers (AHX) */
    layout_mpeg,            /* proper MPEG audio stream */
#endif
    layout_acm,             /* dummy, let libacm handle layout */
    layout_mus_acm,         /* mus has multi-files to deal with */
    layout_aix,             /* CRI AIX's wheels within wheels */
    layout_aax,             /* CRI AAX's wheels within databases */
} layout_t;

/* The meta type specifies how we know what we know about the file. We may know because of a header we read, some of it may have been guessed from filenames, etc. */
typedef enum {
    /* DSP-specific */
    meta_DSP_STD,           /* standard GC ADPCM (DSP) header */
    meta_DSP_CSTR,          /* Star Fox Assault "Cstr" */
    meta_DSP_RS03,          /* Metroid Prime 2 "RS03" */
    meta_DSP_STM,           /* Paper Mario 2 STM */
    meta_DSP_HALP,          /* SSB:M "HALPST" */
    meta_DSP_AGSC,          /* Metroid Prime 2 title */
    meta_DSP_MPDSP,         /* Monopoly Party single header stereo */
    meta_DSP_JETTERS,       /* Bomberman Jetters .dsp */
    meta_DSP_MSS,
    meta_DSP_GCM,
    meta_DSP_STR,			/* Conan .str files */
    meta_DSP_SADB,          /* .sad */
    meta_DSP_WSI,           /* .wsi */
    meta_DSP_AMTS,			/* .amts */
    meta_DSP_WII_IDSP,		/* .gcm with IDSP header */
    meta_DSP_WII_MUS,       /* .mus */

    /* Nintendo */
    meta_STRM,              /* STRM */
    meta_RSTM,              /* RSTM (similar to STRM) */
    meta_AFC,               /* AFC */
    meta_AST,               /* AST */
    meta_RWSD,              /* single-stream RWSD */
    meta_RWAR,              /* single-stream RWAR */
    meta_RWAV,              /* contents of RWAR */
    meta_RSTM_SPM,          /* RSTM with 44->22khz hack */
    meta_THP,
    meta_RSTM_shrunken,     /* Atlus' mutant shortened RSTM */

    /* CRI ADX */
    meta_ADX_03,            /* ADX "type 03" */
    meta_ADX_04,            /* ADX "type 04" */
    meta_ADX_05,            /* ADX "type 05" */
    meta_AIX,               /* CRI AIX */
    meta_AAX,               /* CRI AAX */

    /* etc */
    meta_NGC_ADPDTK,        /* NGC DTK/ADP, no header (.adp) */
    meta_kRAW,              /* almost headerless PCM */
    meta_RSF,               /* Retro Studios RSF, no header (.rsf) */
    meta_HALPST,            /* HAL Labs HALPST */
    meta_GCSW,              /* GCSW (PCM) */
    meta_CFN,				/* Namco CAF Audio File */

    meta_PS2_SShd,			/* .ADS with SShd header */
    meta_PS2_NPSF,			/* Namco Production Sound File */
    meta_PS2_RXW,			/* Sony Arc The Lad Sound File */
    meta_PS2_RAW,			/* RAW Interleaved Format */
    meta_PS2_EXST,			/* Shadow of Colossus EXST */
    meta_PS2_SVAG,			/* Konami SVAG */
    meta_PS2_MIB,			/* MIB File */
    meta_PS2_MIB_MIH,		/* MIB File + MIH Header*/
    meta_PS2_MIC,			/* KOEI MIC File */
    meta_PS2_VAGi,			/* VAGi Interleaved File */
    meta_PS2_VAGp,			/* VAGp Mono File */
    meta_PS2_VAGm,			/* VAGp Mono File */
    meta_PS2_pGAV,			/* VAGp with Little Endian Header */
    meta_PSX_GMS,			/* GMS File (used in PS1 & PS2) */
    meta_PS2_STR,			/* Pacman STR+STH files */
    meta_PS2_ILD,			/* ILD File */
    meta_PS2_PNB,			/* PsychoNauts Bgm File */
    meta_PSX_XA,			/* CD-XA with RIFF header */
    meta_PS2_VAGs,			/* VAG Stereo from Kingdom Hearts */
    meta_PS2_VPK,			/* VPK Audio File */
    meta_PS2_BMDX,          /* Beatmania thing */
    meta_PS2_IVB,           /* Langrisser 3 IVB */
    meta_PS2_SVS,           /* Square SVS */
    meta_XSS,				/* Dino Crisis 3 */
    meta_SL3,				/* Test Drive Unlimited */
    meta_HGC1,				/* Knights of the Temple 2 */
    meta_AUS,				/* Variuos Capcom Games */
    meta_RWS,				/* Variuos Konami Games */
    meta_FSB1,              /* FMOD Sample Bank, version 1 */
    meta_FSB3,              /* FMOD Sample Bank, version 3 */
    meta_FSB4,              /* FMOD Sample Bank, version 4 */
    meta_RWX,				/* Air Force Delta Storm (XBOX) */
    meta_XWB,				/* King of Fighters (XBOX) */
    meta_XA30,				/* Driver - Parallel Lines (PS2) */
    meta_MUSC,				/* Spyro Games, possibly more */
    meta_MUSX_V004,			/* Spyro Games, possibly more */
    meta_MUSX_V006,			/* Spyro Games, possibly more */
    meta_MUSX_V010,			/* Spyro Games, possibly more */
    meta_MUSX_V201,			/* Sphinx and the cursed Mummy */
    meta_LEG,				/* Legaia 2 */
    meta_FILP,				/* Resident Evil - Dead Aim */
    meta_IKM,				/* Zwei! */
    meta_SFS,				/* Baroque */
    meta_BG00,				/* Ibara, Mushihimesama */
    meta_PS2_RSTM,			/* Midnight Club 3 */
    meta_PS2_KCES,			/* Dance Dance Revolution */
    meta_PS2_DXH,			/* Tokobot Plus - Myteries of the Karakuri */
    meta_PS2_PSH,			/* Dawn of Mana - Seiken Densetsu 4 */
    meta_PCM,				/* Ephemeral Fantasia, Lunar - Eternal Blue */
    meta_PS2_RKV,			/* Legacy of Kain - Blood Omen 2 */
    meta_PS2_PSW,			/* Rayman Raving Rabbids */
    meta_PS2_VAS,			/* Pro Baseball Spirits 5 */
    meta_PS2_TEC,			/* TECMO badflagged stream */
    meta_PS2_ENTH,			/* Enthusia */
    meta_SDT,				/* Baldur's Gate - Dark Alliance */
    meta_NGC_TYDSP,			/* Ty - The Tasmanian Tiger */
    meta_NGC_SWD,			/* Conflict - Desert Storm 1 & 2 */
    meta_CAPDSP,			/* Capcom DSP Header */
    meta_DC_STR,			/* SEGA Stream Asset Builder */
    meta_DC_STR_V2,			/* variant of SEGA Stream Asset Builder */
    meta_NGC_BH2PCM,		/* Bio Hazard 2 */
    meta_SAT_SAP,			/* Bubble Symphony */
    meta_DC_IDVI,			/* Eldorado Gate */
    meta_KRAW,				/* Geometry Wars - Galaxies */
    meta_PS2_OMU,			/* PS2 Int file with Header */
    meta_PS2_XA2,			/* XA2 XG3 file */
    meta_IDSP,				/* Chronicles of Narnia */
    meta_SPT_SPD,			/* Variouis */
    meta_ISH_ISD,			/* Various */
    meta_GSP_GSB,			/* Various */
    meta_YDSP,				/* WWE Day of Reckoning */
    meta_FFCC_STR,          /* Final Fantasy: Crystal Chronicles */

    meta_IDSP2,				/* Chronicles of Narnia */
    meta_WAA_WAC_WAD_WAM,	/* Beyond Good & Evil */
    meta_GCA,				/* Metal Slug Anthology */
    meta_MSVP,				/* Popcap Hits */
    meta_NGC_SSM,			/* Golden Gashbell Full Power */
    meta_PS2_JOE,			/* Wall-E / Pixar games */

    meta_NGC_YMF,			/* WWE WrestleMania X8 */
    meta_SADL,              /* .sad */
    meta_PS2_CCC,           /* Tokyo Xtreme Racer DRIFT 2 */
    meta_PSX_FAG,           /* Jackie Chan - Stuntmaster */
    meta_PS2_MIHB,          /* Merged MIH+MIB */
    meta_NGC_PDT,           /* Mario Party 6 */
    meta_DC_ASD,			/* Miss Moonligh */
    meta_NAOMI_SPSD,		/* Guilty Gear X */
    
    meta_RSD2VAG,			/* RSD2VAG */
    meta_RSD2PCMB,			/* RSD2PCMB */
    meta_RSD2XADP,			/* RSD2XADP */
    meta_RSD3PCM,			/* RSD3PCM */
    meta_RSD4PCMB,			/* RSD4PCMB */
    meta_RSD4PCM,			/* RSD4PCM */
    meta_RSD4VAG,			/* RSD4VAG */
    meta_RSD6VAG,			/* RSD6VAG */
    meta_RSD6WADP,			/* RSD6WADP */
    meta_RSD6XADP,			/* RSD6XADP */

    meta_PS2_ASS,			/* ASS */
    meta_PS2_SEG,			/* Eragon */
    meta_NDS_STRM_FFTA2,	/* Final Fantasy Tactics A2 */
    meta_STR_ASR,			/* Donkey Kong Jet Race */
    meta_ZWDSP,				/* Zack and Wiki */
    meta_VGS,				/* Guitar Hero Encore - Rocks the 80s */
    meta_DC_WAV_DCS,		/* Evil Twin - Cypriens Chronicles (DC) */
    meta_WII_SMP,			/* Mushroom Men - The Spore Wars */
    meta_WII_SNG,           /* Excite Trucks */
    meta_EMFF_PS2,			/* Eidos Music File Format for PS2*/
    meta_EMFF_NGC,			/* Eidos Music File Format for NGC/WII */
    
    meta_XBOX_WAVM,			/* XBOX WAVM File */
    meta_XBOX_RIFF,			/* XBOX RIFF/WAVE File */
    meta_XBOX_WVS,			/* XBOX WVS */
    meta_XBOX_STMA,			/* XBOX STMA */
    meta_XBOX_MATX,			/* XBOX MATX */
    meta_XBOX_XMU,			/* XBOX XMU */
    meta_XBOX_XVAS,			/* XBOX VAS */
    
    meta_EAXA_R2,			/* EA XA Release 2 */
    meta_EAXA_R3,			/* EA XA Release 3 */
    meta_EAXA_PSX,			/* EA with PSX ADPCM */
    meta_EACS_PC,			/* EACS PC */
    meta_EACS_PSX,			/* EACS PSX */
    meta_EACS_SAT,			/* EACS SATURN */
    meta_EA_ADPCM,			/* EA XA ADPCM */
    meta_EA_IMA,			/* EA IMA */
    meta_EA_PCM,			/* EA PCM */

    meta_RAW,				/* RAW PCM file */

    meta_GENH,              /* generic header */

#ifdef VGM_USE_VORBIS
    meta_ogg_vorbis,        /* ogg vorbis */
    meta_OGG_SLI,           /* Ogg Vorbis file w/ companion .sli for looping */
    meta_OGG_SLI2,          /* Ogg Vorbis file w/ different styled .sli for looping */
    meta_OGG_SFL,           /* Ogg Vorbis file w/ .sfl (RIFF SFPL) for looping */
    meta_um3_ogg,           /* Ogg Vorbis with first 0x800 bytes XOR 0xFF */
#endif

    meta_AIFC,              /* Audio Interchange File Format AIFF-C */
    meta_AIFF,              /* Audio Interchange File Format */
    meta_STR_SNDS,          /* .str with SNDS blocks and SHDR header */
    meta_WS_AUD,            /* Westwood Studios .aud */
    meta_WS_AUD_old,        /* Westwood Studios .aud, old style */
#ifdef VGM_USE_MPEG
    meta_AHX,               /* CRI AHX header (same structure as ADX) */
#endif
    meta_RIFF_WAVE,         /* RIFF, for WAVs */
    meta_RIFF_WAVE_POS,     /* .wav + .pos for looping */
    meta_RIFF_WAVE_labl_Marker, /* RIFF w/ loop Markers in LIST-adtl-labl */
    meta_RIFF_WAVE_smpl,    /* RIFF w/ loop data in smpl chunk */
    meta_RIFF_WAVE_MWV,     /* .mwv RIFF w/ loop data in ctrl chunk pflt */
    meta_NWA,               /* Visual Art's NWA */
    meta_NWA_NWAINFOINI,    /* NWA w/ NWAINFO.INI for looping */
    meta_NWA_GAMEEXEINI,    /* NWA w/ Gameexe.ini for looping */
    meta_DVI,				/* DVI Interleaved */
    meta_KCEY,				/* KCEYCOMP */
    meta_ACM,               /* InterPlay ACM header */
    meta_MUS_ACM,           /* MUS playlist of InterPlay ACM files */
    meta_DE2,               /* Falcom (Gurumin) .de2 */
    meta_VS,				/* Men in Black .vs */
    meta_FFXI_BGW,          /* FFXI BGW */
    meta_FFXI_SPW,          /* FFXI SPW */
    meta_STS_WII,			/* Shikigami No Shiro 3 STS Audio File */
    meta_PS2_P2BT,			/* Pop'n'Music 7 Audio File */
    meta_PS2_GBTS,			/* Pop'n'Music 9 Audio File */
    meta_NGC_IADP,			/* Gamecube Interleave DSP */

} meta_t;

typedef struct {
    STREAMFILE * streamfile; /* file used by this channel */
    off_t channel_start_offset; /* where data for this channel begins */
    off_t offset;           /* current location in the file */

    off_t frame_header_offset;  /* offset of the current frame header (for WS) */
    int samples_left_in_frame;  /* for WS */

    /* format specific */

    /* adpcm */
    int16_t adpcm_coef[16]; /* for formats with decode coefficients built in */
    int32_t adpcm_coef_3by32[0x60];     /* for Level-5 0x555 */
    union {
        int16_t adpcm_history1_16;  /* previous sample */
        int32_t adpcm_history1_32;
    };
    union {
        int16_t adpcm_history2_16;  /* previous previous sample */
        int32_t adpcm_history2_32;
    };
    union {
        int16_t adpcm_history3_16;
        int32_t adpcm_history3_32;
    };

    int adpcm_step_index;       /* for IMA */
    int adpcm_scale;            /* for MS ADPCM */

    struct g72x_state g72x_state; /* state for G.721 decoder, sort of big but we
                               might as well keep it around */

#ifdef DEBUG
    int samples_done;
    int16_t loop_history1,loop_history2;
#endif

    /* ADX encryption */
    int adx_channels;
    uint16_t adx_xor;
    uint16_t adx_mult;
    uint16_t adx_add;

    /* BMDX encryption */
    uint8_t bmdx_xor;
    uint8_t bmdx_add;
} VGMSTREAMCHANNEL;

typedef struct {
    /* basics */
    int32_t num_samples;    /* the actual number of samples in this stream */
    int32_t sample_rate;    /* sample rate in Hz */
    int channels;           /* number of channels */
    coding_t coding_type;   /* type of encoding */
    layout_t layout_type;   /* type of layout for data */
    meta_t meta_type;       /* how we know the metadata */

    /* looping */
    int loop_flag;          /* is this stream looped? */
    int32_t loop_start_sample; /* first sample of the loop (included in the loop) */
    int32_t loop_end_sample; /* last sample of the loop (not included in the loop) */

    /* channels */
    VGMSTREAMCHANNEL * ch;   /* pointer to array of channels */

    /* channel copies */
    VGMSTREAMCHANNEL * start_ch;    /* copies of channel status as they were at the beginning of the stream */
    VGMSTREAMCHANNEL * loop_ch;     /* copies of channel status as they were at the loop point */

    /* layout-specific */
    int32_t current_sample;         /* number of samples we've passed */
    int32_t samples_into_block;     /* number of samples into the current block */
    /* interleave */
    size_t interleave_block_size;   /* interleave for this file */
    size_t interleave_smallblock_size;  /* smaller interleave for last block */
    /* headered blocks */
    off_t current_block_offset;     /* start of this block (offset of block header) */
    size_t current_block_size;      /* size of the block we're in now */
    off_t next_block_offset;        /* offset of header of the next block */

    int hit_loop;                   /* have we seen the loop yet? */

    /* loop layout (saved values) */
    int32_t loop_sample;            /* saved from current_sample, should be loop_start_sample... */
    int32_t loop_samples_into_block;    /* saved from samples_into_block */
    off_t loop_block_offset;        /* saved from current_block_offset */
    size_t loop_block_size;         /* saved from current_block_size */
    off_t loop_next_block_offset;   /* saved from next_block_offset */

    uint8_t xa_channel;				/* Selected XA Channel */
    int32_t xa_sector_length;		/* XA block */
    int8_t get_high_nibble;

    uint8_t	ea_big_endian;			/* Big Endian ? */
    uint8_t	ea_compression_type;		
    uint8_t	ea_compression_version;	
    uint8_t	ea_platform;

    int32_t ws_output_size;         /* output bytes for this block */

    void * start_vgmstream;    /* a copy of the VGMSTREAM as it was at the beginning of the stream */

    int32_t thpNextFrameSize;

    /* Data the codec needs for the whole stream. This is for codecs too
     * different from vgmstream's structure to be reasonably shoehorned into
     * using the ch structures.
     * Note also that support must be added for resetting, looping and
     * closing for every codec that uses this, as it will not be handled. */
    void * codec_data;
} VGMSTREAM;

#ifdef VGM_USE_VORBIS
typedef struct {
    STREAMFILE *streamfile;
    ogg_int64_t offset;
    ogg_int64_t size;
} ogg_vorbis_streamfile;

typedef struct {
    OggVorbis_File ogg_vorbis_file;
    int bitstream;

    ogg_vorbis_streamfile ov_streamfile;
} ogg_vorbis_codec_data;
#endif

#ifdef VGM_USE_MPEG
#define AHX_EXPECTED_FRAME_SIZE 0x414
/* MPEG_BUFFER_SIZE should be >= AHX_EXPECTED_FRAME_SIZE */
#define MPEG_BUFFER_SIZE 0x1000
typedef struct {
    uint8_t buffer[MPEG_BUFFER_SIZE];
    int buffer_used;
    int buffer_full;
    size_t bytes_in_buffer;
    mpg123_handle *m;
} mpeg_codec_data;
#endif

/* with one file this is also used for just
   ACM */
typedef struct {
    int file_count;
    int current_file;
    /* the index we return to upon loop completion */
    int loop_start_file;
    /* one after the index of the last file, typically
     * will be equal to file_count */
    int loop_end_file;
    /* Upon exit from a loop, which file to play. */
    /* -1 if there is no such file */
    /*int end_file;*/
    ACMStream **files;
} mus_acm_codec_data;

#define AIX_BUFFER_SIZE 0x1000
/* AIXery */
typedef struct {
    sample buffer[AIX_BUFFER_SIZE];
    int segment_count;
    int stream_count;
    int current_segment;
    /* one per segment */
    int32_t *sample_counts;
    /* organized like:
     * segment1_stream1, segment1_stream2, segment2_stream1, segment2_stream2*/
    VGMSTREAM **adxs;
} aix_codec_data;

typedef struct {
    int segment_count;
    int current_segment;
    int loop_segment;
    /* one per segment */
    int32_t *sample_counts;
    VGMSTREAM **adxs;
} aax_codec_data;

/* for compressed NWA */
typedef struct {
    NWAData *nwa;
} nwa_codec_data;

/* do format detection, return pointer to a usable VGMSTREAM, or NULL on failure */
VGMSTREAM * init_vgmstream(const char * const filename);

VGMSTREAM * init_vgmstream_from_STREAMFILE(STREAMFILE *streamFile);

/* reset a VGMSTREAM to start of stream */
void reset_vgmstream(VGMSTREAM * vgmstream);

/* allocate a VGMSTREAM and channel stuff */
VGMSTREAM * allocate_vgmstream(int channel_count, int looped);

/* deallocate, close, etc. */
void close_vgmstream(VGMSTREAM * vgmstream);

/* calculate the number of samples to be played based on looping parameters */
int32_t get_vgmstream_play_samples(double looptimes, double fadeseconds, double fadedelayseconds, VGMSTREAM * vgmstream);

/* render! */
void render_vgmstream(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream);

/* smallest self-contained group of samples is a frame */
int get_vgmstream_samples_per_frame(VGMSTREAM * vgmstream);
/* number of bytes per frame */
int get_vgmstream_frame_size(VGMSTREAM * vgmstream);
/* in NDS IMA the frame size is the block size, so the last one is short */
int get_vgmstream_samples_per_shortframe(VGMSTREAM * vgmstream);
int get_vgmstream_shortframe_size(VGMSTREAM * vgmstream);

/* Assume that we have written samples_written into the buffer already, and we have samples_to_do consecutive
 * samples ahead of us. Decode those samples into the buffer. */
void decode_vgmstream(VGMSTREAM * vgmstream, int samples_written, int samples_to_do, sample * buffer);

/* Assume additionally that we have samples_to_do consecutive samples in "data",
 * and this this is for channel number "channel" */
void decode_vgmstream_mem(VGMSTREAM * vgmstream, int samples_written, int samples_to_do, sample * buffer, uint8_t * data, int channel);

/* calculate number of consecutive samples to do (taking into account stopping for loop start and end)  */
int vgmstream_samples_to_do(int samples_this_block, int samples_per_frame, VGMSTREAM * vgmstream);

/* Detect start and save values, also detect end and restore values. Only works on exact sample values.
 * Returns 1 if loop was done. */
int vgmstream_do_loop(VGMSTREAM * vgmstream);

/* Write a description of the stream into array pointed by desc,
 * which must be length bytes long. Will always be null-terminated if length > 0
 */
void describe_vgmstream(VGMSTREAM * vgmstream, char * desc, int length);

/* See if there is a second file which may be the second channel, given
 * already opened mono opened_stream which was opened from filename.
 * If a suitable file is found, open it and change opened_stream to a
 * stereo stream. */
void try_dual_file_stereo(VGMSTREAM * opened_stream, STREAMFILE *streamFile);

#endif
