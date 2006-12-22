#pragma once
#include "ICodec.h"
#include "FileReader.h"
#include "Dlllibflac.h"

class FLACCodec : public ICodec
{
public:
  FLACCodec();
  virtual ~FLACCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

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

  DllLibFlac  m_dll;
  CFileReader m_file;
  BYTE* m_pBuffer;                    //  buffer to hold the decoded audio data
  int m_BufferSize;                   //  size of buffer is filled with decoded audio data
  int m_MaxFrameSize;                 //  size of a single decoded frame
  FLAC__SeekableStreamDecoder* m_pFlacDecoder;
};
