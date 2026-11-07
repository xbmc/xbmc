/* SPDX-License-Identifier: MIT */
/*
 * kodi-web-audio: the audio of a web page, played by Kodi's audio engine.
 *
 * The web engine's audio sink connects to the AF_UNIX SOCK_SEQPACKET socket
 * Kodi names in KODI_WEB_AUDIO_SOCKET, one connection per sink. Every message
 * starts with a uint32_t type; all fields are in host byte order, both ends
 * run on the same host.
 *
 *  sink -> Kodi  HELLO          version
 *  Kodi -> sink  HELLO_REPLY    version, the PCM encodings Kodi takes in its
 *                               order of preference, for each compressed
 *                               format the sample rates Kodi passes through,
 *                               maximum channels; carries two descriptors
 *                               (SCM_RIGHTS): the shared state page and an
 *                               eventfd the sink signals after changing it
 *  sink -> Kodi  FORMAT         generation, encoding, rate, channel positions
 *  Kodi -> sink  FORMAT_REPLY   accepted or not
 *  sink -> Kodi  DATA           generation, then the payload
 *
 * PCM payload is interleaved samples in the encoding of the last FORMAT,
 * which names its byte order.
 * AC3, E-AC3 and DTS payload is a stream of IEC 61937 bursts as Kodi's
 * CAEPackIEC61937 writes them, which is what gst_audio_iec61937_payload()
 * writes for G_BIG_ENDIAN: the preamble Pa, Pb, Pc, Pd and the codec frame's
 * 16-bit words, all in the host's byte order, then zeros up to the burst
 * length.
 * Pd counts bytes for E-AC3 and bits otherwise. The bursts keep the sink's
 * clock at the burst rate; Kodi plays the frames through its own
 * passthrough path.
 *
 * The sink bumps the generation in the shared state when it drops what it
 * has sent (a flush or a new format) and signals the eventfd; Kodi then drops
 * every DATA of an older generation. Pausing works the same way through the
 * paused field. Kodi publishes how far it got under a sequence counter, odd
 * while it writes: the bytes of the current generation it has handed to the
 * audio engine, the engine's delay and when it measured it, so the sink can
 * report its delay without asking.
 */

#ifndef KODI_WEB_AUDIO_H
#define KODI_WEB_AUDIO_H

#include <stdint.h>

#define KODI_WEB_AUDIO_VERSION 1
#define KODI_WEB_AUDIO_ENV "KODI_WEB_AUDIO_SOCKET"
#define KODI_WEB_AUDIO_MAX_CHANNELS 8
#define KODI_WEB_AUDIO_MAX_MESSAGE 65536
#define KODI_WEB_AUDIO_MAX_PCM 16

enum kodi_web_audio_type
{
  KODI_WEB_AUDIO_HELLO = 1,
  KODI_WEB_AUDIO_HELLO_REPLY,
  KODI_WEB_AUDIO_FORMAT,
  KODI_WEB_AUDIO_FORMAT_REPLY,
  KODI_WEB_AUDIO_DATA,
};

enum kodi_web_audio_encoding
{
  KODI_WEB_AUDIO_U8 = 1,
  KODI_WEB_AUDIO_S16LE,
  KODI_WEB_AUDIO_S16BE,
  /* 24 bits in 3 bytes */
  KODI_WEB_AUDIO_S24LE,
  KODI_WEB_AUDIO_S24BE,
  /* 24 bits in the low 3 bytes of 32-bit words */
  KODI_WEB_AUDIO_S24_32LE,
  KODI_WEB_AUDIO_S24_32BE,
  KODI_WEB_AUDIO_S32LE,
  KODI_WEB_AUDIO_S32BE,
  KODI_WEB_AUDIO_F32LE,
  KODI_WEB_AUDIO_F32BE,
  KODI_WEB_AUDIO_F64LE,
  KODI_WEB_AUDIO_F64BE,
  KODI_WEB_AUDIO_AC3,
  KODI_WEB_AUDIO_EAC3,
  KODI_WEB_AUDIO_DTS,
};

/* index of a compressed format in HELLO_REPLY's passthrough */
enum kodi_web_audio_passthrough
{
  KODI_WEB_AUDIO_PT_AC3 = 0,
  KODI_WEB_AUDIO_PT_EAC3,
  KODI_WEB_AUDIO_PT_DTS,
  KODI_WEB_AUDIO_PT_COUNT,
};

/* bit n of a passthrough rate mask stands for entry n of this list */
#define KODI_WEB_AUDIO_RATES \
  { \
    8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000, 128000, \
        176400, 192000 \
  }

enum kodi_web_audio_position
{
  KODI_WEB_AUDIO_POS_NONE = 0,
  KODI_WEB_AUDIO_POS_FL,
  KODI_WEB_AUDIO_POS_FR,
  KODI_WEB_AUDIO_POS_FC,
  KODI_WEB_AUDIO_POS_LFE,
  KODI_WEB_AUDIO_POS_BL,
  KODI_WEB_AUDIO_POS_BR,
  KODI_WEB_AUDIO_POS_FLOC,
  KODI_WEB_AUDIO_POS_FROC,
  KODI_WEB_AUDIO_POS_BC,
  KODI_WEB_AUDIO_POS_SL,
  KODI_WEB_AUDIO_POS_SR,
  KODI_WEB_AUDIO_POS_TFL,
  KODI_WEB_AUDIO_POS_TFR,
  KODI_WEB_AUDIO_POS_TFC,
  KODI_WEB_AUDIO_POS_TC,
  KODI_WEB_AUDIO_POS_TBL,
  KODI_WEB_AUDIO_POS_TBR,
  KODI_WEB_AUDIO_POS_TBC,
};

struct kodi_web_audio_hello
{
  uint32_t type;
  uint32_t version;
};

struct kodi_web_audio_hello_reply
{
  uint32_t type;
  uint32_t version;
  uint32_t pcm_count;
  uint32_t pcm[KODI_WEB_AUDIO_MAX_PCM];
  uint32_t passthrough[KODI_WEB_AUDIO_PT_COUNT];
  uint32_t max_channels;
};

struct kodi_web_audio_format
{
  uint32_t type;
  uint32_t generation;
  uint32_t encoding;
  uint32_t rate;
  uint32_t channels;
  uint8_t positions[KODI_WEB_AUDIO_MAX_CHANNELS];
};

struct kodi_web_audio_format_reply
{
  uint32_t type;
  uint32_t accepted;
};

struct kodi_web_audio_data
{
  uint32_t type;
  uint32_t generation;
};

struct kodi_web_audio_shared
{
  /* written by the sink */
  uint32_t generation;
  uint32_t paused;
  /* written by Kodi */
  uint32_t sequence;
  uint32_t applied_generation;
  uint64_t consumed;
  int64_t delay_ns;
  int64_t stamp_ns;
  uint32_t playing;
  uint32_t reserved;
};

#endif
