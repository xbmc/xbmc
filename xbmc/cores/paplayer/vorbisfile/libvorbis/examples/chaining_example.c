/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2007             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: illustrate simple use of chained bitstream and vorbisfile.a
 last mod: $Id: chaining_example.c 13293 2007-07-24 00:09:47Z xiphmont $

 ********************************************************************/

#include <stdlib.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#ifdef _WIN32 /* We need the following two to set stdin/stdout to binary */
#include <io.h>
#include <fcntl.h>
#endif

int main(){
  OggVorbis_File ov;
  int i;

#ifdef _WIN32 /* We need to set stdin to binary mode. Damn windows. */
  /* Beware the evil ifdef. We avoid these where we can, but this one we 
     cannot. Don't add any more, you'll probably go to hell if you do. */
  _setmode( _fileno( stdin ), _O_BINARY );
#endif

  /* open the file/pipe on stdin */
  if(ov_open_callbacks(stdin,&ov,NULL,-1,OV_CALLBACKS_NOCLOSE)<0){
    printf("Could not open input as an OggVorbis file.\n\n");
    exit(1);
  }
  
  /* print details about each logical bitstream in the input */
  if(ov_seekable(&ov)){
    printf("Input bitstream contained %ld logical bitstream section(s).\n",
	   ov_streams(&ov));
    printf("Total bitstream samples: %ld\n\n",
	   (long)ov_pcm_total(&ov,-1));
    printf("Total bitstream playing time: %ld seconds\n\n",
	   (long)ov_time_total(&ov,-1));

  }else{
    printf("Standard input was not seekable.\n"
	   "First logical bitstream information:\n\n");
  }

  for(i=0;i<ov_streams(&ov);i++){
    vorbis_info *vi=ov_info(&ov,i);
    printf("\tlogical bitstream section %d information:\n",i+1);
    printf("\t\t%ldHz %d channels bitrate %ldkbps serial number=%ld\n",
	   vi->rate,vi->channels,ov_bitrate(&ov,i)/1000,
	   ov_serialnumber(&ov,i));
    printf("\t\theader length: %ld bytes\n",(long)
	   (ov.dataoffsets[i]-ov.offsets[i]));
    printf("\t\tcompressed length: %ld bytes\n",(long)(ov_raw_total(&ov,i)));
    printf("\t\tplay time: %lds\n",(long)ov_time_total(&ov,i));
  }

  ov_clear(&ov);
  return 0;
}

