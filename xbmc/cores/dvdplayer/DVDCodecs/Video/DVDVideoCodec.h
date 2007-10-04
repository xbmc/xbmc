
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
  double pts; // timestamp in seconds, used in the CDVDPlayer class to keep track of pts
  BYTE* data[4];      // [4] = alpha channel, currently not used
  int iLineSize[4];   // [4] = alpha channel, currently not used

  unsigned int iFlags;
  
  unsigned int iRepeatPicture;
  double       iDuration;
  unsigned int iFrameType         : 4; // see defines above // 1->I, 2->P, 3->B, 0->Undef
  unsigned int color_matrix       : 4;
  unsigned int color_range        : 1; // 1 indicate if we have a full range of color
  int iGroupId;

  unsigned int iWidth;
  unsigned int iHeight;
  unsigned int iDisplayWidth;  // width of the picture without black bars
  unsigned int iDisplayHeight; // height of the picture without black bars
}
DVDVideoPicture;

typedef struct stDVDVideoUserData
{
  BYTE* data;
  int size;
}
DVDVideoUserData;

#define DVP_FLAG_TOP_FIELD_FIRST    0x00000001
#define DVP_FLAG_REPEAT_TOP_FIELD   0x00000002 //Set to indicate that the top field should be repeated
#define DVP_FLAG_ALLOCATED          0x00000004 //Set to indicate that this has allocated data
#define DVP_FLAG_INTERLACED         0x00000008 //Set to indicate that this frame is interlaced

#define DVP_FLAG_NOSKIP             0x00000010 // indicate this picture should never be dropped
#define DVP_FLAG_DROPPED            0x00000020 // indicate that this picture has been dropped in decoder stage, will have no data
#define DVP_FLAG_NOAUTOSYNC         0x00000040 // disregard any smooth syncing on this picture

// DVP_FLAG 0x00000100 - 0x00000f00 is in use by libmpeg2!

class CDVDStreamInfo;
class CDVDCodecOption;
typedef std::vector<CDVDCodecOption> CDVDCodecOptions;

// VC_ messages, messages can be combined
#define VC_ERROR    0x00000001  // an error occured, no other messages will be returned
#define VC_BUFFER   0x00000002  // the decoder needs more data
#define VC_PICTURE  0x00000004  // the decoder got a picture, call Decode(NULL, 0) again to parse the rest of the data
#define VC_USERDATA 0x00000008  // the decoder found some userdata,  call Decode(NULL, 0) again to parse the rest of the data

class CDVDVideoCodec
{
public:

  CDVDVideoCodec() {}
  virtual ~CDVDVideoCodec() {}
  
  /*
   * Open the decoder, returns true on success
   */
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) = 0;
  
  /*
   * Dispose, Free all resources
   */
  virtual void Dispose() = 0;
  
  /*
   * returns one or a combination of VC_ messages
   * pData and iSize can be NULL, this means we should flush the rest of the data.
   */
  virtual int Decode(BYTE* pData, int iSize, double pts) = 0;
  
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

  /*
   * returns true if successfull
   * the data is valid until the next Decode call
   * userdata can be anything, for now we use it for closed captioning
   */
  virtual bool GetUserData(DVDVideoUserData* pDvdVideoUserData)
  {
    pDvdVideoUserData->data = NULL;
    pDvdVideoUserData->size = 0;
    return false;
  }
   
  /*
   * will be called by video player indicating if a frame will eventually be dropped
   * codec can then skip actually decoding the data, just consume the data set picture headers
   */
  virtual void SetDropState(bool bDrop) = 0;

  /*
   *
   * should return codecs name
   */
  virtual const char* GetName() = 0;
};
