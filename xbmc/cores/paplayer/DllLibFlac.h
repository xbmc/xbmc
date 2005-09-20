#pragma once
#include "../../DynamicDll.h"
#include "flac/seekable_stream_decoder.h"

class DllLibFlacInterface
{
public:
    virtual FLAC__SeekableStreamDecoder* FLAC__seekable_stream_decoder_new()=0;
    virtual void 	FLAC__seekable_stream_decoder_delete(FLAC__SeekableStreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_set_md5_checking(FLAC__SeekableStreamDecoder *decoder, FLAC__bool value)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_set_read_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderReadCallback value)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_set_seek_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderSeekCallback value)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_set_tell_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderTellCallback value)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_set_length_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderLengthCallback value)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_set_eof_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderEofCallback value)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_set_write_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderWriteCallback value)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_set_metadata_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderMetadataCallback value)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_set_error_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderErrorCallback value)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_set_client_data(FLAC__SeekableStreamDecoder *decoder, void *value)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_set_metadata_respond(FLAC__SeekableStreamDecoder *decoder, FLAC__MetadataType type)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_set_metadata_respond_application(FLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4])=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_set_metadata_respond_all(FLAC__SeekableStreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_set_metadata_ignore(FLAC__SeekableStreamDecoder *decoder, FLAC__MetadataType type)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_set_metadata_ignore_application(FLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4])=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_set_metadata_ignore_all(FLAC__SeekableStreamDecoder *decoder)=0;
    virtual FLAC__SeekableStreamDecoderState FLAC__seekable_stream_decoder_get_state(const FLAC__SeekableStreamDecoder *decoder)=0;
    virtual FLAC__StreamDecoderState FLAC__seekable_stream_decoder_get_stream_decoder_state(const FLAC__SeekableStreamDecoder *decoder)=0;
    virtual const char * FLAC__seekable_stream_decoder_get_resolved_state_string(const FLAC__SeekableStreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_get_md5_checking(const FLAC__SeekableStreamDecoder *decoder)=0;
    virtual unsigned FLAC__seekable_stream_decoder_get_channels(const FLAC__SeekableStreamDecoder *decoder)=0;
    virtual FLAC__ChannelAssignment FLAC__seekable_stream_decoder_get_channel_assignment(const FLAC__SeekableStreamDecoder *decoder)=0;
    virtual unsigned FLAC__seekable_stream_decoder_get_bits_per_sample(const FLAC__SeekableStreamDecoder *decoder)=0;
    virtual unsigned FLAC__seekable_stream_decoder_get_sample_rate(const FLAC__SeekableStreamDecoder *decoder)=0;
    virtual unsigned FLAC__seekable_stream_decoder_get_blocksize(const FLAC__SeekableStreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_get_decode_position(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *position)=0;
    virtual FLAC__SeekableStreamDecoderState FLAC__seekable_stream_decoder_init(FLAC__SeekableStreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_finish(FLAC__SeekableStreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_flush(FLAC__SeekableStreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_reset(FLAC__SeekableStreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_process_single(FLAC__SeekableStreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_process_until_end_of_metadata(FLAC__SeekableStreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_process_until_end_of_stream(FLAC__SeekableStreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_skip_single_frame(FLAC__SeekableStreamDecoder *decoder)=0;
    virtual FLAC__bool FLAC__seekable_stream_decoder_seek_absolute(FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 sample)=0;
};

class DllLibFlac : public DllDynamic, DllLibFlacInterface
{
  DECLARE_DLL_WRAPPER(DllLibFlac, Q:\\system\\players\\PAPlayer\\libFlac.dll)
  DEFINE_METHOD0(FLAC__SeekableStreamDecoder*, FLAC__seekable_stream_decoder_new)
  DEFINE_METHOD1(void, FLAC__seekable_stream_decoder_delete, (FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD2(FLAC__bool, FLAC__seekable_stream_decoder_set_md5_checking, (FLAC__SeekableStreamDecoder *p1, FLAC__bool p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__seekable_stream_decoder_set_read_callback, (FLAC__SeekableStreamDecoder *p1, FLAC__SeekableStreamDecoderReadCallback p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__seekable_stream_decoder_set_seek_callback, (FLAC__SeekableStreamDecoder *p1, FLAC__SeekableStreamDecoderSeekCallback p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__seekable_stream_decoder_set_tell_callback, (FLAC__SeekableStreamDecoder *p1, FLAC__SeekableStreamDecoderTellCallback p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__seekable_stream_decoder_set_length_callback, (FLAC__SeekableStreamDecoder *p1, FLAC__SeekableStreamDecoderLengthCallback p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__seekable_stream_decoder_set_eof_callback, (FLAC__SeekableStreamDecoder *p1, FLAC__SeekableStreamDecoderEofCallback p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__seekable_stream_decoder_set_write_callback, (FLAC__SeekableStreamDecoder *p1, FLAC__SeekableStreamDecoderWriteCallback p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__seekable_stream_decoder_set_metadata_callback, (FLAC__SeekableStreamDecoder *p1, FLAC__SeekableStreamDecoderMetadataCallback p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__seekable_stream_decoder_set_error_callback, (FLAC__SeekableStreamDecoder *p1, FLAC__SeekableStreamDecoderErrorCallback p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__seekable_stream_decoder_set_client_data, (FLAC__SeekableStreamDecoder *p1, void *p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__seekable_stream_decoder_set_metadata_respond, (FLAC__SeekableStreamDecoder *p1, FLAC__MetadataType p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__seekable_stream_decoder_set_metadata_respond_application, (FLAC__SeekableStreamDecoder *p1, const FLAC__byte p2[4]))
  DEFINE_METHOD1(FLAC__bool, FLAC__seekable_stream_decoder_set_metadata_respond_all, (FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD2(FLAC__bool, FLAC__seekable_stream_decoder_set_metadata_ignore, (FLAC__SeekableStreamDecoder *p1, FLAC__MetadataType p2))
  DEFINE_METHOD2(FLAC__bool, FLAC__seekable_stream_decoder_set_metadata_ignore_application, (FLAC__SeekableStreamDecoder *p1, const FLAC__byte p2[4]))
  DEFINE_METHOD1(FLAC__bool, FLAC__seekable_stream_decoder_set_metadata_ignore_all, (FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD1(FLAC__SeekableStreamDecoderState, FLAC__seekable_stream_decoder_get_state, (const FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD1(FLAC__StreamDecoderState, FLAC__seekable_stream_decoder_get_stream_decoder_state, (const FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD1(const char *, FLAC__seekable_stream_decoder_get_resolved_state_string, (const FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD1(FLAC__bool, FLAC__seekable_stream_decoder_get_md5_checking, (const FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD1(unsigned, FLAC__seekable_stream_decoder_get_channels, (const FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD1(FLAC__ChannelAssignment, FLAC__seekable_stream_decoder_get_channel_assignment, (const FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD1(unsigned, FLAC__seekable_stream_decoder_get_bits_per_sample, (const FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD1(unsigned, FLAC__seekable_stream_decoder_get_sample_rate, (const FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD1(unsigned, FLAC__seekable_stream_decoder_get_blocksize, (const FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD2(FLAC__bool, FLAC__seekable_stream_decoder_get_decode_position, (const FLAC__SeekableStreamDecoder *p1, FLAC__uint64 *p2))
  DEFINE_METHOD1(FLAC__SeekableStreamDecoderState, FLAC__seekable_stream_decoder_init, (FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD1(FLAC__bool, FLAC__seekable_stream_decoder_finish, (FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD1(FLAC__bool, FLAC__seekable_stream_decoder_flush, (FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD1(FLAC__bool, FLAC__seekable_stream_decoder_reset, (FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD1(FLAC__bool, FLAC__seekable_stream_decoder_process_single, (FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD1(FLAC__bool, FLAC__seekable_stream_decoder_process_until_end_of_metadata, (FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD1(FLAC__bool, FLAC__seekable_stream_decoder_process_until_end_of_stream, (FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD1(FLAC__bool, FLAC__seekable_stream_decoder_skip_single_frame, (FLAC__SeekableStreamDecoder *p1))
  DEFINE_METHOD2(FLAC__bool, FLAC__seekable_stream_decoder_seek_absolute, (FLAC__SeekableStreamDecoder *p1, FLAC__uint64 p2))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_new)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_delete)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_set_md5_checking)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_set_read_callback)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_set_seek_callback)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_set_tell_callback)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_set_length_callback)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_set_eof_callback)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_set_write_callback)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_set_metadata_callback)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_set_error_callback)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_set_client_data)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_set_metadata_respond)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_set_metadata_respond_application)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_set_metadata_respond_all)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_set_metadata_ignore)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_set_metadata_ignore_application)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_set_metadata_ignore_all)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_get_state)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_get_stream_decoder_state)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_get_resolved_state_string)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_get_md5_checking)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_get_channels)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_get_channel_assignment)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_get_bits_per_sample)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_get_sample_rate)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_get_blocksize)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_get_decode_position)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_init)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_finish)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_flush)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_reset)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_process_single)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_process_until_end_of_metadata)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_process_until_end_of_stream)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_skip_single_frame)
    RESOLVE_METHOD(FLAC__seekable_stream_decoder_seek_absolute)
  END_METHOD_RESOLVE()
};
