
#pragma once

// when modifying these structures, make sure you update all codecs accordingly
#define FRAME_TYPE_UNDEF 0
#define FRAME_TYPE_I 1
#define FRAME_TYPE_P 2
#define FRAME_TYPE_B 3
#define FRAME_TYPE_D 4

// video structure with PIX_FMT_YUV420P data
// should be entirely filled by all codecs
typedef struct stDVDVideoPicture
{
  __int64 pts; // timestamp in seconds, used in the CDVDPlayer class to keep track of pts
  BYTE* data[4];      // [4] = alpha channel, currently not used
  int iLineSize[4];   // [4] = alpha channel, currently not used

  unsigned int iFlags;
  
  unsigned int iRepeatPicture;
  unsigned int iDuration;
  int iFrameType; // see defines above // 1->I, 2->P, 3->B, 0->Undef


  unsigned int iWidth;
  unsigned int iHeight;
  unsigned int iDisplayWidth;  // width of the picture without black bars
  unsigned int iDisplayHeight; // height of the picture without black bars
}
DVDVideoPicture;

#define DVP_FLAG_TOP_FIELD_FIRST    0x00000001
#define DVP_FLAG_REPEAT_TOP_FIELD   0x00000002 //Set to indicate that the top field should be repeated
#define DVP_FLAG_ALLOCATED          0x00000004 //Set to indicate that this has allocated data
#define DVP_FLAG_INTERLACED         0x00000008 //Set to indicate that this frame is interlaced

#define DVP_FLAG_NOSKIP             0x00000010 // indicate this picture should never be dropped

// DVP_FLAG 0x00000100 - 0x00000f00 is in use by libmpeg2!

class CDemuxStreamVideo;
enum CodecID;
struct AVStream;

// VC_ messages, messages can be combined
#define VC_ERROR   0x00000001 // an error occured, no other messages will be returned
#define VC_BUFFER  0x00000002  // the decoder needs more data
#define VC_PICTURE 0x00000004  // the decoder got a picture, call Decode(NULL, 0) again to parse the rest of the data

class CDVDVideoCodec
{
public:

  CDVDVideoCodec() {}
  virtual ~CDVDVideoCodec() {}
  
  /*
   * Open the decoder, returns true on success
   */
  virtual bool Open(CodecID codecID, int iWidth, int iHeight) = 0;
  
  /*
   * Dispose, Free all resources
   */
  virtual void Dispose() = 0;
  
  /*
   * returns one or a combination of VC_ messages
   * pData and iSize can be NULL, this means we should flush the rest of the data.
   */
  virtual int Decode(BYTE* pData, int iSize) = 0;
  
 /*
   * Reset the decoder.
   * Should be the same as calling Dispose and Open after each other
   */
  virtual void Reset() = 0;
  
  /*
   * returns true if successfull
   * the data is valid until the next Decode call
   */
  virtual bool GetPicture(DVDVideoPicture* pDvdVideoPicture) = 0;
};
