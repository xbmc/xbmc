/* Copyright (C) 2002 Jean-Marc Valin */
/**
   @file speex_header.h
   @brief Describes the Speex header
*/
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


#ifndef SPEEX_HEADER_H
#define SPEEX_HEADER_H

#ifdef __cplusplus
extern "C" {
#endif

struct SpeexMode;

/** Maximum number of characters for encoding the Speex version number in the header */
#define SPEEX_HEADER_VERSION_LENGTH 20

/** Speex header info for file-based formats */
typedef struct SpeexHeader {
   char speex_string[8];       /**< Identifies a Speex bit-stream, always set to "Speex   " */
   char speex_version[SPEEX_HEADER_VERSION_LENGTH]; /**< Speex version */
   int speex_version_id;       /**< Version for Speex (for checking compatibility) */
   int header_size;            /**< Total size of the header ( sizeof(SpeexHeader) ) */
   int rate;                   /**< Sampling rate used */
   int mode;                   /**< Mode used (0 for narrowband, 1 for wideband) */
   int mode_bitstream_version; /**< Version ID of the bit-stream */
   int nb_channels;            /**< Number of channels encoded */
   int bitrate;                /**< Bit-rate used */
   int frame_size;             /**< Size of frames */
   int vbr;                    /**< 1 for a VBR encoding, 0 otherwise */
   int frames_per_packet;      /**< Number of frames stored per Ogg packet */
   int extra_headers;          /**< Number of additional headers after the comments */
   int reserved1;              /**< Reserved for future use, must be zero */
   int reserved2;              /**< Reserved for future use, must be zero */
} SpeexHeader;

/** Initializes a SpeexHeader using basic information */
void speex_init_header(SpeexHeader *header, int rate, int nb_channels, const struct SpeexMode *m);

/** Creates the header packet from the header itself (mostly involves endianness conversion) */
char *speex_header_to_packet(SpeexHeader *header, int *size);

/** Creates a SpeexHeader from a packet */
SpeexHeader *speex_packet_to_header(char *packet, int size);

#ifdef __cplusplus
}
#endif


#endif
