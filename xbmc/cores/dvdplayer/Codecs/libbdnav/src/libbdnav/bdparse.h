#if !defined(_BDPARSE_H_)
#define _BDPARSE_H_

#include "mpls_parse.h"
#include "clpi_parse.h"
#include "navigation.h"

#define BD_STREAM_TYPE_VIDEO_MPEG1          0x01
#define BD_STREAM_TYPE_VIDEO_MPEG2          0x02
#define BD_STREAM_TYPE_AUDIO_MPEG1          0x03
#define BD_STREAM_TYPE_AUDIO_MPEG2          0x04
#define BD_STREAM_TYPE_AUDIO_LPCM           0x80
#define BD_STREAM_TYPE_AUDIO_AC3            0x81
#define BD_STREAM_TYPE_AUDIO_DTS            0x82
#define BD_STREAM_TYPE_AUDIO_TRUHD          0x83
#define BD_STREAM_TYPE_AUDIO_AC3PLUS        0x84
#define BD_STREAM_TYPE_AUDIO_DTSHD          0x85
#define BD_STREAM_TYPE_AUDIO_DTSHD_MASTER   0x86
#define BD_STREAM_TYPE_VIDEO_VC1            0xea
#define BD_STREAM_TYPE_VIDEO_H264           0x1b
#define BD_STREAM_TYPE_SUB_PG               0x90
#define BD_STREAM_TYPE_SUB_IG               0x91
#define BD_STREAM_TYPE_SUB_TEXT             0x92

#define BD_VIDEO_FORMAT_480I                1   // ITU-R BT.601-5
#define BD_VIDEO_FORMAT_576I                2   // ITU-R BT.601-4
#define BD_VIDEO_FORMAT_480P                3   // SMPTE 293M
#define BD_VIDEO_FORMAT_1080I               4   // SMPTE 274M
#define BD_VIDEO_FORMAT_720P                5   // SMPTE 296M
#define BD_VIDEO_FORMAT_1080P               6   // SMPTE 274M
#define BD_VIDEO_FORMAT_576P                7   // ITU-R BT.1358

#define BD_VIDEO_RATE_24000_1001            1   // 23.976
#define BD_VIDEO_RATE_24                    2
#define BD_VIDEO_RATE_25                    3
#define BD_VIDEO_RATE_30000_1001            4   // 29.97
#define BD_VIDEO_RATE_50                    6
#define BD_VIDEO_RATE_60000_1001            7   // 59.94

#define BD_ASPECT_RATIO_4_3                 2
#define BD_ASPECT_RATIO_16_9                3

#define BD_AUDIO_FORMAT_MONO                1
#define BD_AUDIO_FORMAT_STEREO              3
#define BD_AUDIO_FORMAT_MULTI_CHAN          6
#define BD_AUDIO_FORMAT_COMBO               12  // Stereo ac3/dts, 
                                                // multi mlp/dts-hd

#define BD_AUDIO_RATE_48                    1
#define BD_AUDIO_RATE_96                    4
#define BD_AUDIO_RATE_192                   5
#define BD_AUDIO_RATE_192_COMBO             12  // 48 or 96 ac3/dts
                                                // 192 mpl/dts-hd
#define BD_AUDIO_RATE_96_COMBO              14  // 48 ac3/dts
                                                // 96 mpl/dts-hd

#define BD_TEXT_CHAR_CODE_UTF8              0x01
#define BD_TEXT_CHAR_CODE_UTF16BE           0x02
#define BD_TEXT_CHAR_CODE_SHIFT_JIS         0x03
#define BD_TEXT_CHAR_CODE_EUC_KR            0x04
#define BD_TEXT_CHAR_CODE_GB18030_20001     0x05
#define BD_TEXT_CHAR_CODE_CN_GB             0x06
#define BD_TEXT_CHAR_CODE_BIG5              0x07

#endif // _BDPARSE_H_
