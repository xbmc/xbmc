/*
  Copyright (c) 2005, The Musepack Development Team
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the following
  disclaimer in the documentation and/or other materials provided
  with the distribution.

  * Neither the name of the The Musepack Development Team nor the
  names of its contributors may be used to endorse or promote
  products derived from this software without specific prior
  written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/// \file streaminfo.c
/// Implementation of streaminfo reading functions.

#include <mpcdec/mpcdec.h>
#include <mpcdec/internal.h>

static const char *
Stringify(mpc_uint32_t profile) // profile is 0...15, where 7...13 is used
{
    static const char na[] = "n.a.";
    static const char *Names[] = {
        na, "'Unstable/Experimental'", na, na,
        na, "'quality 0'", "'quality 1'", "'Telephone'",
        "'Thumb'", "'Radio'", "'Standard'", "'Xtreme'",
        "'Insane'", "'BrainDead'", "'quality 9'", "'quality 10'"
    };

    return profile >= sizeof(Names) / sizeof(*Names) ? na : Names[profile];
}

void
mpc_streaminfo_init(mpc_streaminfo * si)
{
    memset(si, 0, sizeof(mpc_streaminfo));
}

// read information from SV8 header
// not yet implemented
static mpc_int32_t
streaminfo_read_header_sv8(mpc_streaminfo * si, mpc_reader * fp)
{
    (void) si;
    (void) fp;
    return 0;
}

/// Reads streaminfo from SV7 header. 
static mpc_int32_t
streaminfo_read_header_sv7(mpc_streaminfo * si, mpc_uint32_t HeaderData[8])
{
    const mpc_int32_t samplefreqs[4] = { 44100, 48000, 37800, 32000 };

    //mpc_uint32_t    HeaderData [8];
    mpc_uint16_t Estimatedpeak_title = 0;

    if (si->stream_version > 0x71) {
        //        Update (si->stream_version);
        return 0;
    }

    /*
       if ( !fp->seek ( si->header_position ) )         // seek to header start
       return ERROR_CODE_FILE;
       if ( fp->read ( HeaderData, sizeof HeaderData) != sizeof HeaderData )
       return ERROR_CODE_FILE;
     */

    si->bitrate = 0;
    si->frames = HeaderData[1];
    si->is = 0;
    si->ms = (HeaderData[2] >> 30) & 0x0001;
    si->max_band = (HeaderData[2] >> 24) & 0x003F;
    si->block_size = 1;
    si->profile = (HeaderData[2] << 8) >> 28;
    si->profile_name = Stringify(si->profile);
    si->sample_freq = samplefreqs[(HeaderData[2] >> 16) & 0x0003];
    Estimatedpeak_title = (mpc_uint16_t) (HeaderData[2] & 0xFFFF);   // read the ReplayGain data
    si->gain_title = (mpc_uint16_t) ((HeaderData[3] >> 16) & 0xFFFF);
    si->peak_title = (mpc_uint16_t) (HeaderData[3] & 0xFFFF);
    si->gain_album = (mpc_uint16_t) ((HeaderData[4] >> 16) & 0xFFFF);
    si->peak_album = (mpc_uint16_t) (HeaderData[4] & 0xFFFF);
    si->is_true_gapless = (HeaderData[5] >> 31) & 0x0001; // true gapless: used?
    si->last_frame_samples = (HeaderData[5] >> 20) & 0x07FF;  // true gapless: valid samples for last frame
    si->encoder_version = (HeaderData[6] >> 24) & 0x00FF;

    if (si->encoder_version == 0) {
        sprintf(si->encoder, "Buschmann 1.7.0...9, Klemm 0.90...1.05");
    }
    else {
        switch (si->encoder_version % 10) {
        case 0:
            sprintf(si->encoder, "Release %u.%u", si->encoder_version / 100,
                    si->encoder_version / 10 % 10);
            break;
        case 2:
        case 4:
        case 6:
        case 8:
            sprintf(si->encoder, "Beta %u.%02u", si->encoder_version / 100,
                    si->encoder_version % 100);
            break;
        default:
            sprintf(si->encoder, "--Alpha-- %u.%02u",
                    si->encoder_version / 100, si->encoder_version % 100);
            break;
        }
    }

    //    if ( si->peak_title == 0 )                                      // there is no correct peak_title contained within header
    //        si->peak_title = (mpc_uint16_t)(Estimatedpeak_title * 1.18);
    //    if ( si->peak_album == 0 )
    //        si->peak_album = si->peak_title;                          // no correct peak_album, use peak_title

    //si->sample_freq    = 44100;                                     // AB: used by all files up to SV7
    si->channels = 2;

    return ERROR_CODE_OK;
}

// read information from SV4-SV6 header
static mpc_int32_t
streaminfo_read_header_sv6(mpc_streaminfo * si, mpc_uint32_t HeaderData[8])
{
    //mpc_uint32_t    HeaderData [8];

    /*
       if ( !fp->seek (  si->header_position ) )         // seek to header start
       return ERROR_CODE_FILE;
       if ( fp->read ( HeaderData, sizeof HeaderData ) != sizeof HeaderData )
       return ERROR_CODE_FILE;
     */

    si->bitrate = (HeaderData[0] >> 23) & 0x01FF;   // read the file-header (SV6 and below)
    si->is = (HeaderData[0] >> 22) & 0x0001;
    si->ms = (HeaderData[0] >> 21) & 0x0001;
    si->stream_version = (HeaderData[0] >> 11) & 0x03FF;
    si->max_band = (HeaderData[0] >> 6) & 0x001F;
    si->block_size = (HeaderData[0]) & 0x003F;
    si->profile = 0;
    si->profile_name = Stringify((mpc_uint32_t) (-1));
    if (si->stream_version >= 5)
        si->frames = HeaderData[1]; // 32 bit
    else
        si->frames = (HeaderData[1] >> 16); // 16 bit

    si->gain_title = 0;          // not supported
    si->peak_title = 0;
    si->gain_album = 0;
    si->peak_album = 0;

    si->last_frame_samples = 0;
    si->is_true_gapless = 0;

    si->encoder_version = 0;
    si->encoder[0] = '\0';

    if (si->stream_version == 7)
        return ERROR_CODE_SV7BETA;  // are there any unsupported parameters used?
    if (si->bitrate != 0)
        return ERROR_CODE_CBR;
    if (si->is != 0)
        return ERROR_CODE_IS;
    if (si->block_size != 1)
        return ERROR_CODE_BLOCKSIZE;

    if (si->stream_version < 6) // Bugfix: last frame was invalid for up to SV5
        si->frames -= 1;

    si->sample_freq = 44100;     // AB: used by all files up to SV7
    si->channels = 2;

    if (si->stream_version < 4 || si->stream_version > 7)
        return ERROR_CODE_INVALIDSV;

    return ERROR_CODE_OK;
}

// reads file header and tags
mpc_int32_t
mpc_streaminfo_read(mpc_streaminfo * si, mpc_reader * r)
{
    mpc_uint32_t HeaderData[8];
    mpc_int32_t Error = 0;

    // get header position
    if ((si->header_position = JumpID3v2(r)) < 0) {
        return ERROR_CODE_FILE;
    }
    // seek to first byte of mpc data
    if (!r->seek(r->data, si->header_position)) {
        return ERROR_CODE_FILE;
    }
    if (r->read(r->data, HeaderData, 8 * 4) != 8 * 4) {
        return ERROR_CODE_FILE;
    }
    if (!r->seek(r->data, si->header_position + 6 * 4)) {
        return ERROR_CODE_FILE;
    }

    si->total_file_length = r->get_size(r->data);
    si->tag_offset = si->total_file_length;
    if (memcmp(HeaderData, "MP+", 3) == 0) {
#ifndef MPC_LITTLE_ENDIAN
        mpc_uint32_t ptr;
        for (ptr = 0; ptr < 8; ptr++) {
            HeaderData[ptr] = mpc_swap32(HeaderData[ptr]);
        }
#endif
        si->stream_version = HeaderData[0] >> 24;

        // stream version 8
        if ((si->stream_version & 15) >= 8) {
            Error = streaminfo_read_header_sv8(si, r);
        }
        // stream version 7
        else if ((si->stream_version & 15) == 7) {
            Error = streaminfo_read_header_sv7(si, HeaderData);
        }
    }
    else {
        // stream version 4-6
        Error = streaminfo_read_header_sv6(si, HeaderData);
    }

    // estimation, exact value needs too much time
    si->pcm_samples = 1152 * si->frames - 576;

    if (si->pcm_samples > 0) {
        si->average_bitrate =
            (si->tag_offset -
             si->header_position) * 8.0 * si->sample_freq / si->pcm_samples;
    }
    else {
        si->average_bitrate = 0;
    }

    return Error;
}

double
mpc_streaminfo_get_length(mpc_streaminfo * si)
{
    return (double)mpc_streaminfo_get_length_samples(si) /
        (double)si->sample_freq;
}

mpc_int64_t
mpc_streaminfo_get_length_samples(mpc_streaminfo * si)
{
    mpc_int64_t samples = (mpc_int64_t) si->frames * MPC_FRAME_LENGTH;
    if (si->is_true_gapless) {
        samples -= (MPC_FRAME_LENGTH - si->last_frame_samples);
    }
    else {
        samples -= MPC_DECODER_SYNTH_DELAY;
    }
    return samples;
}
