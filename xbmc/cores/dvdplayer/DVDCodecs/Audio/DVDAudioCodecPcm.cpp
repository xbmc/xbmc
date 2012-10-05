/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DVDAudioCodecPcm.h"
#include "DVDStreamInfo.h"
#include "DVDCodecs/DVDCodecs.h"

/* from g711.c by SUN microsystems (unrestricted use) */
#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of A-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */

#define	BIAS		(0x84)		/* Bias for linear code. */

/*
 * alaw2linear() - Convert an A-law value to 16-bit linear PCM
 *
 */
static int alaw2linear(unsigned char	a_val)
{
	int		t;
	int		seg;

	a_val ^= 0x55;

	t = a_val & QUANT_MASK;
	seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
	if(seg) t= (t + t + 1 + 32) << (seg + 2);
	else    t= (t + t + 1     ) << 3;

	return ((a_val & SIGN_BIT) ? t : -t);
}

static int ulaw2linear(unsigned char	u_val)
{
	int		t;

	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;

	/*
	 * Extract and bias the quantization bits. Then
	 * shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & QUANT_MASK) << 3) + BIAS;
	t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

	return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

/**
 * \brief convert samples to 16 bit
 * \param bps byte per sample for the source format, must be >= 2
 * \param le 0 for big-, 1 for little-endian
 * \param us 0 for signed, 1 for unsigned input
 * \param src input samples
 * \param samples output samples
 * \param src_len number of bytes in src
 */
static inline void decode_to16(int bps, int le, int us, BYTE **src, short **samples, int src_len)
{
    register int n = src_len / bps;
    if (le) *src += bps - 2;
    for(;n>0;n--) {
        *(*samples)++ = ((*src)[le] << 8 | (*src)[1 - le]) - (us?0x8000:0);
        *src += bps;
    }
    if (le) *src -= bps - 2;
}

const BYTE ff_reverse[256] =
    {
      0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,0x10,0x90,0x50,0xD0,0x30,0xB0,0x70,0xF0,
      0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,0x18,0x98,0x58,0xD8,0x38,0xB8,0x78,0xF8,
      0x04,0x84,0x44,0xC4,0x24,0xA4,0x64,0xE4,0x14,0x94,0x54,0xD4,0x34,0xB4,0x74,0xF4,
      0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,0x1C,0x9C,0x5C,0xDC,0x3C,0xBC,0x7C,0xFC,
      0x02,0x82,0x42,0xC2,0x22,0xA2,0x62,0xE2,0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2,
      0x0A,0x8A,0x4A,0xCA,0x2A,0xAA,0x6A,0xEA,0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,
      0x06,0x86,0x46,0xC6,0x26,0xA6,0x66,0xE6,0x16,0x96,0x56,0xD6,0x36,0xB6,0x76,0xF6,
      0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,0x7E,0xFE,
      0x01,0x81,0x41,0xC1,0x21,0xA1,0x61,0xE1,0x11,0x91,0x51,0xD1,0x31,0xB1,0x71,0xF1,
      0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,0x19,0x99,0x59,0xD9,0x39,0xB9,0x79,0xF9,
      0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,
      0x0D,0x8D,0x4D,0xCD,0x2D,0xAD,0x6D,0xED,0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,
      0x03,0x83,0x43,0xC3,0x23,0xA3,0x63,0xE3,0x13,0x93,0x53,0xD3,0x33,0xB3,0x73,0xF3,
      0x0B,0x8B,0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,
      0x07,0x87,0x47,0xC7,0x27,0xA7,0x67,0xE7,0x17,0x97,0x57,0xD7,0x37,0xB7,0x77,0xF7,
      0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,0x1F,0x9F,0x5F,0xDF,0x3F,0xBF,0x7F,0xFF,
    };

CDVDAudioCodecPcm::CDVDAudioCodecPcm() : CDVDAudioCodec()
{
  m_iSourceChannels = 0;
  m_iSourceSampleRate = 0;
  m_iSourceBitrate = 0;
  m_decodedDataSize = 0;
  m_codecID = CODEC_ID_NONE;
  m_iOutputChannels = 0;

  memset(m_decodedData, 0, sizeof(m_decodedData));
  memset(table, 0, sizeof(table));
}

CDVDAudioCodecPcm::~CDVDAudioCodecPcm()
{
  Dispose();
}

bool CDVDAudioCodecPcm::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  SetDefault();

  m_codecID = hints.codec;
  m_iSourceChannels = hints.channels;
  m_iSourceSampleRate = hints.samplerate;
  m_iSourceBitrate = 16;

  switch (m_codecID)
  {
    case CODEC_ID_PCM_ALAW:
    {
      for (int i = 0; i < 256; i++) table[i] = alaw2linear(i);
      break;
    }
    case CODEC_ID_PCM_MULAW:
    {
      for (int i = 0; i < 256; i++) table[i] = ulaw2linear(i);
      break;
    }
    default:
    {
      break;
    }
  }

  // set desired output
  m_iOutputChannels = m_iSourceChannels;

  return true;
}

void CDVDAudioCodecPcm::Dispose()
{
}

int CDVDAudioCodecPcm::Decode(BYTE* pData, int iSize)
{
    int n;
    short *samples;
    BYTE *src;

    samples = (short*)m_decodedData;
    src = pData;
    int buf_size = iSize;

    if (iSize > AVCODEC_MAX_AUDIO_FRAME_SIZE / 2)
        iSize = AVCODEC_MAX_AUDIO_FRAME_SIZE / 2;

    switch (m_codecID)
    {
    case CODEC_ID_PCM_S32LE:
        decode_to16(4, 1, 0, &src, &samples, buf_size);
        break;
    case CODEC_ID_PCM_S32BE:
        decode_to16(4, 0, 0, &src, &samples, buf_size);
        break;
    case CODEC_ID_PCM_U32LE:
        decode_to16(4, 1, 1, &src, &samples, buf_size);
        break;
    case CODEC_ID_PCM_U32BE:
        decode_to16(4, 0, 1, &src, &samples, buf_size);
        break;
    case CODEC_ID_PCM_S24LE:
        decode_to16(3, 1, 0, &src, &samples, buf_size);
        break;
    case CODEC_ID_PCM_S24BE:
        decode_to16(3, 0, 0, &src, &samples, buf_size);
        break;
    case CODEC_ID_PCM_U24LE:
        decode_to16(3, 1, 1, &src, &samples, buf_size);
        break;
    case CODEC_ID_PCM_U24BE:
        decode_to16(3, 0, 1, &src, &samples, buf_size);
        break;
    case CODEC_ID_PCM_S24DAUD:
        n = buf_size / 3;
        for(;n>0;n--) {
          uint32_t v = src[0] << 16 | src[1] << 8 | src[2];
          v >>= 4; // sync flags are here
          *samples++ = ff_reverse[(v >> 8) & 0xff] +
                       (ff_reverse[v & 0xff] << 8);
          src += 3;
        }
        break;
    case CODEC_ID_PCM_S16LE:
        n = buf_size >> 1;
        for(;n>0;n--) {
            *samples++ = src[0] | (src[1] << 8);
            src += 2;
        }
        break;
    case CODEC_ID_PCM_S16BE:
        n = buf_size >> 1;
        for(;n>0;n--) {
            *samples++ = (src[0] << 8) | src[1];
            src += 2;
        }
        break;
    case CODEC_ID_PCM_U16LE:
        n = buf_size >> 1;
        for(;n>0;n--) {
            *samples++ = (src[0] | (src[1] << 8)) - 0x8000;
            src += 2;
        }
        break;
    case CODEC_ID_PCM_U16BE:
        n = buf_size >> 1;
        for(;n>0;n--) {
            *samples++ = ((src[0] << 8) | src[1]) - 0x8000;
            src += 2;
        }
        break;
    case CODEC_ID_PCM_S8:
        n = buf_size;
        for(;n>0;n--) {
            *samples++ = src[0] << 8;
            src++;
        }
        break;
    case CODEC_ID_PCM_U8:
        n = buf_size;
        for(;n>0;n--) {
            *samples++ = ((int)src[0] - 128) << 8;
            src++;
        }
        break;
    case CODEC_ID_PCM_ALAW:
    case CODEC_ID_PCM_MULAW:
        n = buf_size;
        for(;n>0;n--) {
            *samples++ = table[src[0]];
            src++;
        }
        break;
    default:
        return -1;
    }

    m_decodedDataSize = (BYTE*)samples - (BYTE*)m_decodedData;
    return iSize;
}

int CDVDAudioCodecPcm::GetData(BYTE** dst)
{
  *dst = (BYTE*)m_decodedData;
  return m_decodedDataSize;
}

void CDVDAudioCodecPcm::SetDefault()
{
  m_iSourceChannels = 0;
  m_iSourceSampleRate = 0;
  m_iSourceBitrate = 0;
  m_decodedDataSize = 0;
  m_codecID = CODEC_ID_NONE;
}

void CDVDAudioCodecPcm::Reset()
{
  //SetDefault();
}

int CDVDAudioCodecPcm::GetChannels()
{
  return m_iOutputChannels;
}

CAEChannelInfo CDVDAudioCodecPcm::GetChannelMap()
{
  assert(m_iOutputChannels > 0 && m_iOutputChannels <= 8);
  static enum AEChannel map[8][9] =
  {
    /* MONO   */ {AE_CH_FC, AE_CH_NULL,                                                                      },
    /* STEREO */ {AE_CH_FL, AE_CH_FR, AE_CH_NULL,                                                            },
    /* 3.0 ?  */ {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_NULL,                                                  },
    /* 4.0 ?  */ {AE_CH_FL, AE_CH_FR, AE_CH_BL, AE_CH_BR , AE_CH_NULL,                                       },
    /* 5.0    */ {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_BL , AE_CH_BR, AE_CH_NULL                              },
    /* 5.1    */ {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_LFE, AE_CH_BL, AE_CH_BR, AE_CH_NULL,                   },
    /* 7.0 ?  */ {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_BL , AE_CH_BR, AE_CH_SL, AE_CH_SR, AE_CH_NULL          },
    /* 7.1 ?  */ {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_LFE, AE_CH_BL, AE_CH_BR, AE_CH_SL, AE_CH_SR, AE_CH_NULL}
  };

  return map[m_iOutputChannels - 1];
}

int CDVDAudioCodecPcm::GetSampleRate()
{
  return m_iSourceSampleRate;
}

enum AEDataFormat CDVDAudioCodecPcm::GetDataFormat()
{
  return AE_FMT_S16NE;
}
