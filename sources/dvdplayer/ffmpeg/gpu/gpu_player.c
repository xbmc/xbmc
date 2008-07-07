//Lightweight player to test gpu assisted h264 decoding
//Renders a couple frames using libavcodec
//ode based on a tutorial by Martin Bohme (boehme@inb.uni-luebeckREMOVETHIS.de)

#include "avcodec.h"
#include "avformat.h"
#include <stdio.h>


int main(int argc, char *argv[])
{
  AVFormatContext *pFormatCtx;
  int             i, err, videoStream;
  AVCodecContext  *pCodecCtx;
  AVCodec         *pCodec;
  AVFrame         *pFrame; 
  AVFrame         *pFrameRGB;
  AVPacket        packet;
  int             frameFinished;
  int             numBytes;
  uint8_t         *buffer;
  int numFrames = 5;

  if(argc < 2)
  {
    printf("Usage: gpu_player file [frames test]\n");
    return -1;
  }

  av_register_all();

  if(av_open_input_file(&pFormatCtx,argv[1], NULL, 0, NULL)!=0)
    {
      printf("Couldn't open video file\n");
      return -1;
    }


  if(av_find_stream_info(pFormatCtx)<0)
    return -1; // Couldn't find stream information
  
  // Dump information about file onto standard error
  dump_format(pFormatCtx, 0, argv[1], 0);
  
  videoStream=-1;
  for(i=0; i<pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO) {
      videoStream=i;
      break;
    }

  if(videoStream==-1)
  {
    printf("Didn't find a video stream\n");
    return -1;
  }

  pCodecCtx=pFormatCtx->streams[videoStream]->codec;

  if(argc > 2)
  {
    if(argc > 3)
    {
      if(!strcmp("dct", argv[3]))
	pCodecCtx->dct_test = 1;
      else if(!strcmp("motion", argv[3]))
	pCodecCtx->mo_comp_test = 1;
      else
      {
	printf("invalid test specified. Valid tests are: dct, motion\n");
	return -1;
      }
    }
  }
  pCodecCtx->gpu = 1;


  pCodec=avcodec_find_decoder(pCodecCtx->codec_id);

  if(pCodec==NULL)
  {
    fprintf(stderr, "Unsupported codec!\n");
    return -1;
  }


  if(avcodec_open(pCodecCtx, pCodec)<0)
  {
    printf("Could not open codec\n");
    return -1;
  }

  // Allocate video frame
  pFrame=avcodec_alloc_frame();

  // Allocate an AVFrame structure
  pFrameRGB=avcodec_alloc_frame();
  if(pFrameRGB==NULL)
    return -1;

  // Determine required buffer size and allocate buffer
  numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
			      pCodecCtx->height);
  buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

  // Assign appropriate parts of buffer to image planes in pFrameRGB
  // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
  // of AVPicture
  avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
		 pCodecCtx->width, pCodecCtx->height);

  //Decode a couple frames
  for(i=0; i < numFrames; i++)
  {
    av_read_frame(pFormatCtx, &packet);
    if(packet.stream_index==videoStream)
    {
      avcodec_decode_video(pCodecCtx, pFrame, &frameFinished,
                           packet.data, packet.size);
    }


  
  }
  // Close the codec
  avcodec_close(pCodecCtx);

  // Close the video file
  av_close_input_file(pFormatCtx);
  return 0;
}

