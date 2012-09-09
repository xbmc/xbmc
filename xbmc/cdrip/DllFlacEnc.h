#pragma once

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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif

#include <FLAC/stream_encoder.h>
#include <FLAC/metadata.h>

#include "DynamicDll.h"

class DllFlacEncInterface
{
public:
  virtual FLAC__StreamEncoder *FLAC__stream_encoder_new()=0;
  virtual FLAC__bool FLAC__stream_encoder_set_verify(FLAC__StreamEncoder *encoder, FLAC__bool value)=0;
  virtual FLAC__bool FLAC__stream_encoder_set_compression_level(FLAC__StreamEncoder *encoder, unsigned value)=0;
  virtual FLAC__bool FLAC__stream_encoder_set_channels(FLAC__StreamEncoder *encoder, unsigned value)=0;
  virtual FLAC__bool FLAC__stream_encoder_set_bits_per_sample(FLAC__StreamEncoder *encoder, unsigned value)=0;
  virtual FLAC__bool FLAC__stream_encoder_set_sample_rate(FLAC__StreamEncoder *encoder, unsigned value)=0;
  virtual FLAC__bool FLAC__stream_encoder_set_total_samples_estimate(FLAC__StreamEncoder *encoder, FLAC__uint64 value)=0;
  virtual FLAC__bool FLAC__stream_encoder_set_metadata(FLAC__StreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks)=0;
  virtual FLAC__StreamEncoderInitStatus FLAC__stream_encoder_init_stream(FLAC__StreamEncoder *encoder, FLAC__StreamEncoderWriteCallback write_callback, FLAC__StreamEncoderSeekCallback seek_callback, FLAC__StreamEncoderTellCallback tell_callback, FLAC__StreamEncoderMetadataCallback metadata_callback, void *client_data)=0;
  virtual FLAC__bool FLAC__stream_encoder_process_interleaved(FLAC__StreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples)=0;
  virtual FLAC__StreamEncoderState FLAC__stream_encoder_get_state(FLAC__StreamEncoder *encoder)=0;
  virtual FLAC__bool FLAC__stream_encoder_finish(FLAC__StreamEncoder *encoder)=0;
  virtual void FLAC__stream_encoder_delete(FLAC__StreamEncoder *encoder)=0;
  virtual FLAC__StreamMetadata *FLAC__metadata_object_new(FLAC__MetadataType type)=0;
  virtual FLAC__bool FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(FLAC__StreamMetadata_VorbisComment_Entry *entry, const char *field_name, const char *field_value)=0;
  virtual FLAC__bool FLAC__metadata_object_vorbiscomment_append_comment(FLAC__StreamMetadata *object, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy)=0;
  virtual void FLAC__metadata_object_delete(FLAC__StreamMetadata *object)=0;
  virtual ~DllFlacEncInterface() {}
};

class DllFlacEnc : public DllDynamic, DllFlacEncInterface
{
  DECLARE_DLL_WRAPPER(DllFlacEnc, DLL_PATH_FLAC_CODEC)
  DEFINE_METHOD0(FLAC__StreamEncoder*, FLAC__stream_encoder_new)
  DEFINE_METHOD2(FLAC__bool, FLAC__stream_encoder_set_verify, (FLAC__StreamEncoder *p1, FLAC__bool p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__stream_encoder_set_compression_level, (FLAC__StreamEncoder *p1, unsigned p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__stream_encoder_set_channels, (FLAC__StreamEncoder *p1, unsigned p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__stream_encoder_set_bits_per_sample, (FLAC__StreamEncoder *p1, unsigned p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__stream_encoder_set_sample_rate, (FLAC__StreamEncoder *p1, unsigned p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__stream_encoder_set_total_samples_estimate, (FLAC__StreamEncoder *p1, FLAC__uint64 p2))
  DEFINE_METHOD3(FLAC__bool, FLAC__stream_encoder_set_metadata, (FLAC__StreamEncoder *p1, FLAC__StreamMetadata **p2, unsigned p3))
  DEFINE_METHOD6(FLAC__StreamEncoderInitStatus, FLAC__stream_encoder_init_stream, (FLAC__StreamEncoder *p1, FLAC__StreamEncoderWriteCallback p2, FLAC__StreamEncoderSeekCallback p3, FLAC__StreamEncoderTellCallback p4, FLAC__StreamEncoderMetadataCallback p5, void *p6))
  DEFINE_METHOD3(FLAC__bool, FLAC__stream_encoder_process_interleaved, (FLAC__StreamEncoder *p1, const FLAC__int32 p2[], unsigned p3))
  DEFINE_METHOD1(FLAC__StreamEncoderState, FLAC__stream_encoder_get_state, (FLAC__StreamEncoder *p1))
  DEFINE_METHOD1(FLAC__bool, FLAC__stream_encoder_finish, (FLAC__StreamEncoder *p1))
  DEFINE_METHOD1(void, FLAC__stream_encoder_delete, (FLAC__StreamEncoder *p1))
  DEFINE_METHOD1(FLAC__StreamMetadata *, FLAC__metadata_object_new, (FLAC__MetadataType p1))
  DEFINE_METHOD3(FLAC__bool, FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair, (FLAC__StreamMetadata_VorbisComment_Entry *p1, const char *p2, const char *p3))
  DEFINE_METHOD3(FLAC__bool, FLAC__metadata_object_vorbiscomment_append_comment, (FLAC__StreamMetadata *p1, FLAC__StreamMetadata_VorbisComment_Entry p2, FLAC__bool p3))
  DEFINE_METHOD1(void, FLAC__metadata_object_delete, (FLAC__StreamMetadata *p1))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(FLAC__stream_encoder_new)
    RESOLVE_METHOD(FLAC__stream_encoder_set_verify)
    RESOLVE_METHOD(FLAC__stream_encoder_set_compression_level)
    RESOLVE_METHOD(FLAC__stream_encoder_set_channels)
	RESOLVE_METHOD(FLAC__stream_encoder_set_bits_per_sample)
    RESOLVE_METHOD(FLAC__stream_encoder_set_sample_rate)
    RESOLVE_METHOD(FLAC__stream_encoder_set_total_samples_estimate)
    RESOLVE_METHOD(FLAC__stream_encoder_set_metadata)
    RESOLVE_METHOD(FLAC__stream_encoder_init_stream)
    RESOLVE_METHOD(FLAC__stream_encoder_process_interleaved)
	RESOLVE_METHOD(FLAC__stream_encoder_get_state)
	RESOLVE_METHOD(FLAC__stream_encoder_finish)
    RESOLVE_METHOD(FLAC__stream_encoder_delete)
    RESOLVE_METHOD(FLAC__metadata_object_new)
    RESOLVE_METHOD(FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair)
    RESOLVE_METHOD(FLAC__metadata_object_vorbiscomment_append_comment)
    RESOLVE_METHOD(FLAC__metadata_object_delete)
  END_METHOD_RESOLVE()
};
