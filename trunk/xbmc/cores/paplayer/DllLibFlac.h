#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#if defined(_LINUX) && !defined(__APPLE__)
  #include <FLAC/stream_decoder.h>
#else
  #include "FLACCodec/flac-1.2.1/include/FLAC/stream_decoder.h"
#endif
#include "DynamicDll.h"
#include "utils/log.h"

class DllLibFlacInterface
{
public:
    virtual ~DllLibFlacInterface() {}
    virtual FLAC__StreamDecoder *FLAC__stream_decoder_new()=0;
    virtual void   FLAC__stream_decoder_delete(FLAC__StreamDecoder *decoder)=0;
    virtual FLAC__StreamDecoderInitStatus FLAC__stream_decoder_init_stream(
        FLAC__StreamDecoder *decoder,
        FLAC__StreamDecoderReadCallback read_callback,
        FLAC__StreamDecoderSeekCallback seek_callback,
        FLAC__StreamDecoderTellCallback tell_callback,
        FLAC__StreamDecoderLengthCallback length_callback,
        FLAC__StreamDecoderEofCallback eof_callback,
        FLAC__StreamDecoderWriteCallback write_callback,
        FLAC__StreamDecoderMetadataCallback metadata_callback,
        FLAC__StreamDecoderErrorCallback error_callback,
        void *client_data
      )=0;
    virtual FLAC__bool FLAC__stream_decoder_set_md5_checking(FLAC__StreamDecoder *decoder, FLAC__bool value)=0;
    virtual FLAC__bool FLAC__stream_decoder_set_metadata_respond(FLAC__StreamDecoder *decoder, FLAC__MetadataType type)=0;
    virtual FLAC__bool FLAC__stream_decoder_set_metadata_respond_application(FLAC__StreamDecoder *decoder, const FLAC__byte id[4])=0;
    virtual FLAC__bool FLAC__stream_decoder_set_metadata_respond_all(FLAC__StreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__stream_decoder_set_metadata_ignore(FLAC__StreamDecoder *decoder, FLAC__MetadataType type)=0;
    virtual FLAC__bool FLAC__stream_decoder_set_metadata_ignore_application(FLAC__StreamDecoder *decoder, const FLAC__byte id[4])=0;
    virtual FLAC__bool FLAC__stream_decoder_set_metadata_ignore_all(FLAC__StreamDecoder *decoder)=0;
    virtual FLAC__StreamDecoderState FLAC__stream_decoder_get_state(const FLAC__StreamDecoder *decoder)=0;
    virtual const char *FLAC__stream_decoder_get_resolved_state_string(const FLAC__StreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__stream_decoder_get_md5_checking(const FLAC__StreamDecoder *decoder)=0;
    virtual FLAC__uint64 FLAC__stream_decoder_get_total_samples(const FLAC__StreamDecoder *decoder)=0;
    virtual unsigned FLAC__stream_decoder_get_channels(const FLAC__StreamDecoder *decoder)=0;
    virtual FLAC__ChannelAssignment FLAC__stream_decoder_get_channel_assignment(const FLAC__StreamDecoder *decoder)=0;
    virtual unsigned FLAC__stream_decoder_get_bits_per_sample(const FLAC__StreamDecoder *decoder)=0;
    virtual unsigned FLAC__stream_decoder_get_sample_rate(const FLAC__StreamDecoder *decoder)=0;
    virtual unsigned FLAC__stream_decoder_get_blocksize(const FLAC__StreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__stream_decoder_get_decode_position(const FLAC__StreamDecoder *decoder, FLAC__uint64 *position)=0;
    virtual FLAC__bool FLAC__stream_decoder_finish(FLAC__StreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__stream_decoder_flush(FLAC__StreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__stream_decoder_reset(FLAC__StreamDecoder *decoder)=0;
    virtual FLAC_API FLAC__bool FLAC__stream_decoder_process_single(FLAC__StreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__stream_decoder_process_until_end_of_metadata(FLAC__StreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__stream_decoder_process_until_end_of_stream(FLAC__StreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__stream_decoder_skip_single_frame(FLAC__StreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__stream_decoder_seek_absolute(FLAC__StreamDecoder *decoder, FLAC__uint64 sample)=0;
};

class DllLibFlac : public DllDynamic, DllLibFlacInterface
{
  DECLARE_DLL_WRAPPER(DllLibFlac, DLL_PATH_FLAC_CODEC)
  DEFINE_METHOD0(FLAC__StreamDecoder*, FLAC__stream_decoder_new)
  DEFINE_METHOD1(void, FLAC__stream_decoder_delete, (FLAC__StreamDecoder *p1))
  DEFINE_METHOD10(FLAC__StreamDecoderInitStatus, FLAC__stream_decoder_init_stream,
                 (FLAC__StreamDecoder *p1,
                  FLAC__StreamDecoderReadCallback p2,
                  FLAC__StreamDecoderSeekCallback p3,
                  FLAC__StreamDecoderTellCallback p4,
                  FLAC__StreamDecoderLengthCallback p5,
                  FLAC__StreamDecoderEofCallback p6,
                  FLAC__StreamDecoderWriteCallback p7,
                  FLAC__StreamDecoderMetadataCallback p8,
                  FLAC__StreamDecoderErrorCallback p9,
                  void *p10))
  DEFINE_METHOD2(FLAC__bool, FLAC__stream_decoder_set_md5_checking, (FLAC__StreamDecoder *p1, FLAC__bool p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__stream_decoder_set_metadata_respond, (FLAC__StreamDecoder *p1, FLAC__MetadataType p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__stream_decoder_set_metadata_respond_application, (FLAC__StreamDecoder *p1, const FLAC__byte p2[4]))
  DEFINE_METHOD1(FLAC__bool, FLAC__stream_decoder_set_metadata_respond_all, (FLAC__StreamDecoder *p1))
  DEFINE_METHOD2(FLAC__bool, FLAC__stream_decoder_set_metadata_ignore, (FLAC__StreamDecoder *p1, FLAC__MetadataType p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__stream_decoder_set_metadata_ignore_application, (FLAC__StreamDecoder *p1, const FLAC__byte p2[4]))
  DEFINE_METHOD1(FLAC__bool, FLAC__stream_decoder_set_metadata_ignore_all, (FLAC__StreamDecoder *p1))
  DEFINE_METHOD1(FLAC__StreamDecoderState, FLAC__stream_decoder_get_state, (const FLAC__StreamDecoder *p1))
  DEFINE_METHOD1(const char *, FLAC__stream_decoder_get_resolved_state_string, (const FLAC__StreamDecoder *p1))
  DEFINE_METHOD1(FLAC__bool, FLAC__stream_decoder_get_md5_checking, (const FLAC__StreamDecoder *p1))
  DEFINE_METHOD1(FLAC__uint64, FLAC__stream_decoder_get_total_samples, (const FLAC__StreamDecoder *p1))
  DEFINE_METHOD1(unsigned, FLAC__stream_decoder_get_channels, (const FLAC__StreamDecoder *p1))
  DEFINE_METHOD1(FLAC__ChannelAssignment, FLAC__stream_decoder_get_channel_assignment, (const FLAC__StreamDecoder *p1))
  DEFINE_METHOD1(unsigned, FLAC__stream_decoder_get_bits_per_sample, (const FLAC__StreamDecoder *p1))
  DEFINE_METHOD1(unsigned, FLAC__stream_decoder_get_sample_rate, (const FLAC__StreamDecoder *p1))
  DEFINE_METHOD1(unsigned, FLAC__stream_decoder_get_blocksize, (const FLAC__StreamDecoder *p1))
  DEFINE_METHOD2(FLAC__bool, FLAC__stream_decoder_get_decode_position, (const FLAC__StreamDecoder *p1, FLAC__uint64 *p2))
  DEFINE_METHOD1(FLAC__bool, FLAC__stream_decoder_finish, (FLAC__StreamDecoder *p1))
  DEFINE_METHOD1(FLAC__bool, FLAC__stream_decoder_flush, (FLAC__StreamDecoder *p1))
  DEFINE_METHOD1(FLAC__bool, FLAC__stream_decoder_reset, (FLAC__StreamDecoder *p1))
  DEFINE_METHOD1(FLAC__bool, FLAC__stream_decoder_process_single, (FLAC__StreamDecoder *p1))
  DEFINE_METHOD1(FLAC__bool, FLAC__stream_decoder_process_until_end_of_metadata, (FLAC__StreamDecoder *p1))
  DEFINE_METHOD1(FLAC__bool, FLAC__stream_decoder_process_until_end_of_stream, (FLAC__StreamDecoder *p1))
  DEFINE_METHOD1(FLAC__bool, FLAC__stream_decoder_skip_single_frame, (FLAC__StreamDecoder *p1))
  DEFINE_METHOD2(FLAC__bool, FLAC__stream_decoder_seek_absolute, (FLAC__StreamDecoder *p1, FLAC__uint64 p2))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(FLAC__stream_decoder_new)
    RESOLVE_METHOD(FLAC__stream_decoder_delete)
    RESOLVE_METHOD(FLAC__stream_decoder_init_stream)
    RESOLVE_METHOD(FLAC__stream_decoder_set_md5_checking)
    RESOLVE_METHOD(FLAC__stream_decoder_set_metadata_respond)
    RESOLVE_METHOD(FLAC__stream_decoder_set_metadata_respond_application)
    RESOLVE_METHOD(FLAC__stream_decoder_set_metadata_respond_all)
    RESOLVE_METHOD(FLAC__stream_decoder_set_metadata_ignore)
    RESOLVE_METHOD(FLAC__stream_decoder_set_metadata_ignore_application)
    RESOLVE_METHOD(FLAC__stream_decoder_set_metadata_ignore_all)
    RESOLVE_METHOD(FLAC__stream_decoder_get_state)
    RESOLVE_METHOD(FLAC__stream_decoder_get_resolved_state_string)
    RESOLVE_METHOD(FLAC__stream_decoder_get_md5_checking)
    RESOLVE_METHOD(FLAC__stream_decoder_get_total_samples)
    RESOLVE_METHOD(FLAC__stream_decoder_get_channels)
    RESOLVE_METHOD(FLAC__stream_decoder_get_channel_assignment)
    RESOLVE_METHOD(FLAC__stream_decoder_get_bits_per_sample)
    RESOLVE_METHOD(FLAC__stream_decoder_get_sample_rate)
    RESOLVE_METHOD(FLAC__stream_decoder_get_blocksize)
    RESOLVE_METHOD(FLAC__stream_decoder_get_decode_position)
    RESOLVE_METHOD(FLAC__stream_decoder_finish)
    RESOLVE_METHOD(FLAC__stream_decoder_flush)
    RESOLVE_METHOD(FLAC__stream_decoder_reset)
    RESOLVE_METHOD(FLAC__stream_decoder_process_single)
    RESOLVE_METHOD(FLAC__stream_decoder_process_until_end_of_metadata)
    RESOLVE_METHOD(FLAC__stream_decoder_process_until_end_of_stream)
    RESOLVE_METHOD(FLAC__stream_decoder_skip_single_frame)
    RESOLVE_METHOD(FLAC__stream_decoder_seek_absolute)
  END_METHOD_RESOLVE()
};

