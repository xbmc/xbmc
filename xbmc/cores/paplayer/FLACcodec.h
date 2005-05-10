#pragma once
#include "ICodec.h"
#include "flac/seekable_stream_decoder.h"

class FLACCodec : public ICodec
{
  struct FLACdll
  {
    FLAC__SeekableStreamDecoder* (__cdecl* FLAC__seekable_stream_decoder_new)();
    void 	(__cdecl* FLAC__seekable_stream_decoder_delete)(FLAC__SeekableStreamDecoder *decoder);

    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_set_md5_checking)(FLAC__SeekableStreamDecoder *decoder, FLAC__bool value);

    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_set_read_callback)(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderReadCallback value);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_set_seek_callback)(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderSeekCallback value);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_set_tell_callback)(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderTellCallback value);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_set_length_callback)(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderLengthCallback value);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_set_eof_callback)(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderEofCallback value);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_set_write_callback)(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderWriteCallback value);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_set_metadata_callback)(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderMetadataCallback value);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_set_error_callback)(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderErrorCallback value);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_set_client_data)(FLAC__SeekableStreamDecoder *decoder, void *value);

    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_set_metadata_respond)(FLAC__SeekableStreamDecoder *decoder, FLAC__MetadataType type);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_set_metadata_respond_application)(FLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4]);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_set_metadata_respond_all)(FLAC__SeekableStreamDecoder *decoder);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_set_metadata_ignore)(FLAC__SeekableStreamDecoder *decoder, FLAC__MetadataType type);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_set_metadata_ignore_application)(FLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4]);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_set_metadata_ignore_all)(FLAC__SeekableStreamDecoder *decoder);

    FLAC__SeekableStreamDecoderState (__cdecl* FLAC__seekable_stream_decoder_get_state)(const FLAC__SeekableStreamDecoder *decoder);
    FLAC__StreamDecoderState (__cdecl* FLAC__seekable_stream_decoder_get_stream_decoder_state)(const FLAC__SeekableStreamDecoder *decoder);
    const char * (__cdecl* FLAC__seekable_stream_decoder_get_resolved_state_string)(const FLAC__SeekableStreamDecoder *decoder);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_get_md5_checking)(const FLAC__SeekableStreamDecoder *decoder);
    unsigned (__cdecl* FLAC__seekable_stream_decoder_get_channels)(const FLAC__SeekableStreamDecoder *decoder);
    FLAC__ChannelAssignment (__cdecl* FLAC__seekable_stream_decoder_get_channel_assignment)(const FLAC__SeekableStreamDecoder *decoder);
    unsigned (__cdecl* FLAC__seekable_stream_decoder_get_bits_per_sample)(const FLAC__SeekableStreamDecoder *decoder);
    unsigned (__cdecl* FLAC__seekable_stream_decoder_get_sample_rate)(const FLAC__SeekableStreamDecoder *decoder);
    unsigned (__cdecl* FLAC__seekable_stream_decoder_get_blocksize)(const FLAC__SeekableStreamDecoder *decoder);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_get_decode_position)(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *position);

    FLAC__SeekableStreamDecoderState (__cdecl* FLAC__seekable_stream_decoder_init)(FLAC__SeekableStreamDecoder *decoder);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_finish)(FLAC__SeekableStreamDecoder *decoder);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_flush)(FLAC__SeekableStreamDecoder *decoder);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_reset)(FLAC__SeekableStreamDecoder *decoder);

    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_process_single)(FLAC__SeekableStreamDecoder *decoder);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_process_until_end_of_metadata)(FLAC__SeekableStreamDecoder *decoder);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_process_until_end_of_stream)(FLAC__SeekableStreamDecoder *decoder);

    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_skip_single_frame)(FLAC__SeekableStreamDecoder *decoder);
    FLAC__bool (__cdecl* FLAC__seekable_stream_decoder_seek_absolute)(FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 sample);
  };

public:
  FLACCodec();
  virtual ~FLACCodec();

  virtual bool Init(const CStdString &strFile);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool HandlesType(const char *type);

private:
  //  I/O callbacks for the flac decoder
  static FLAC__SeekableStreamDecoderReadStatus DecoderReadCallback(const FLAC__SeekableStreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
  static FLAC__SeekableStreamDecoderSeekStatus DecoderSeekCallback(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
  static FLAC__SeekableStreamDecoderTellStatus DecoderTellCallback(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
  static FLAC__SeekableStreamDecoderLengthStatus DecoderLengthCallback(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
  static FLAC__bool DecoderEofCallback(const FLAC__SeekableStreamDecoder *decoder, void *client_data);
  static FLAC__StreamDecoderWriteStatus DecoderWriteCallback(const FLAC__SeekableStreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
  static void DecoderMetadataCallback(const FLAC__SeekableStreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
  static void DecoderErrorCallback(const FLAC__SeekableStreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

  void FreeDecoder();
  bool LoadDLL();                     // load the DLL in question

  bool m_bDllLoaded;                  // whether our dll is loaded
  FLACdll  m_dll;
  CFile m_file;
  BYTE* m_pBuffer;                    //  buffer to hold the decoded audio data
  int m_BufferSize;                   //  size of buffer is filled with decoded audio data
  int m_MaxFrameSize;                 //  size of a single decoded frame
  FLAC__SeekableStreamDecoder* m_pFlacDecoder;
};
