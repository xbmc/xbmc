/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: illustrate seeking, and test it too
 last mod: $Id: seeking_example.c,v 1.15 2002/07/11 06:40:47 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"

#ifdef _WIN32 /* We need the following two to set stdin/stdout to binary */
# include <io.h>
# include <fcntl.h>
#endif

void _verify(OggVorbis_File *ov,ogg_int64_t pos,
	     ogg_int64_t val,ogg_int64_t pcmval,
	     ogg_int64_t pcmlength,
	     char *bigassbuffer){
  int j;
  long bread;
  char buffer[4096];
  int dummy;

  /* verify the raw position, the pcm position and position decode */
  if(val!=-1 && ov_raw_tell(ov)<val){
    printf("raw position out of tolerance: requested %ld, got %ld\n",
	   (long)val,(long)ov_raw_tell(ov));
    exit(1);
  }
  if(pcmval!=-1 && ov_pcm_tell(ov)>pcmval){
    printf("pcm position out of tolerance: requested %ld, got %ld\n",
	   (long)pcmval,(long)ov_pcm_tell(ov));
    exit(1);
  }
  pos=ov_pcm_tell(ov);
  if(pos<0 || pos>pcmlength){
    printf("pcm position out of bounds: got %ld\n",(long)pos);
    exit(1);
  }
  bread=ov_read(ov,buffer,4096,1,1,1,&dummy);
  for(j=0;j<bread;j++){
    if(buffer[j]!=bigassbuffer[j+pos*2]){
      printf("data position after seek doesn't match pcm position\n");

      {
	FILE *f=fopen("a.m","w");
	for(j=0;j<bread;j++)fprintf(f,"%d\n",(int)buffer[j]);
	fclose(f);
	f=fopen("b.m","w");
	for(j=0;j<bread;j++)fprintf(f,"%d\n",(int)bigassbuffer[j+pos*2]);
	fclose(f);
      }

      exit(1);
    }
  }
}

int main(){
  OggVorbis_File ov;
  int i,ret;
  ogg_int64_t pcmlength;
  char *bigassbuffer;
  int dummy;

#ifdef _WIN32 /* We need to set stdin/stdout to binary mode. Damn windows. */
  _setmode( _fileno( stdin ), _O_BINARY );
  _setmode( _fileno( stdout ), _O_BINARY );
#endif


  /* open the file/pipe on stdin */
  if(ov_open(stdin,&ov,NULL,-1)<0){
    printf("Could not open input as an OggVorbis file.\n\n");
    exit(1);
  }

  if(ov_seekable(&ov)){

    /* to simplify our own lives, we want to assume the whole file is
       stereo.  Verify this to avoid potentially mystifying users
       (pissing them off is OK, just don't confuse them) */
    for(i=0;i<ov.links;i++){
      vorbis_info *vi=ov_info(&ov,i);
      if(vi->channels!=2){
	printf("Sorry; right now seeking_test can only use Vorbis files\n"
	       "that are entirely stereo.\n\n");
	exit(1);
      }
    }
    
    /* because we want to do sample-level verification that the seek
       does what it claimed, decode the entire file into memory */
    fflush(stdout);
    pcmlength=ov_pcm_total(&ov,-1);
    bigassbuffer=malloc(pcmlength*2); /* w00t */
    i=0;
    while(i<pcmlength*2){
      int ret=ov_read(&ov,bigassbuffer+i,pcmlength*2-i,1,1,1,&dummy);
      if(ret<0)continue;
      if(ret){
	i+=ret;
      }else{
	pcmlength=i/2;
      }
      fprintf(stderr,"\rloading.... [%ld left]              ",
	      (long)(pcmlength*2-i));
    }
    
    /* Exercise all the real seeking cases; ov_raw_seek,
       ov_pcm_seek_page and ov_pcm_seek.  time seek is just a wrapper
       on pcm_seek */
    {
      ogg_int64_t length=ov.end;
      printf("\rtesting raw seeking to random places in %ld bytes....\n",
	     (long)length);
    
      for(i=0;i<1000;i++){
	ogg_int64_t val=(double)rand()/RAND_MAX*length;
	ogg_int64_t pos;
	printf("\r\t%d [raw position %ld]...     ",i,(long)val);
	fflush(stdout);
	ret=ov_raw_seek(&ov,val);
	if(ret<0){
	  printf("seek failed: %d\n",ret);
	  exit(1);
	}

	_verify(&ov,pos,val,-1,pcmlength,bigassbuffer);

      }
    }

    printf("\r");
    {
      printf("testing pcm page seeking to random places in %ld samples....\n",
	     (long)pcmlength);
    
      for(i=0;i<1000;i++){
	ogg_int64_t val=(double)rand()/RAND_MAX*pcmlength;
	ogg_int64_t pos;
	printf("\r\t%d [pcm position %ld]...     ",i,(long)val);
	fflush(stdout);
	ret=ov_pcm_seek_page(&ov,val);
	if(ret<0){
	  printf("seek failed: %d\n",ret);
	  exit(1);
	}

	_verify(&ov,pos,-1,val,pcmlength,bigassbuffer);

      }
    }
    
    printf("\r");
    {
      ogg_int64_t length=ov.end;
      printf("testing pcm exact seeking to random places in %ld samples....\n",
	     (long)pcmlength);
    
      for(i=0;i<1000;i++){
	ogg_int64_t val=(double)rand()/RAND_MAX*pcmlength;
	ogg_int64_t pos;
	printf("\r\t%d [pcm position %ld]...     ",i,(long)val);
	fflush(stdout);
	ret=ov_pcm_seek(&ov,val);
	if(ret<0){
	  printf("seek failed: %d\n",ret);
	  exit(1);
	}
	if(ov_pcm_tell(&ov)!=val){
	  printf("Declared position didn't perfectly match request: %ld != %ld\n",
		 (long)val,(long)ov_pcm_tell(&ov));
	  exit(1);
	}

	_verify(&ov,pos,-1,val,pcmlength,bigassbuffer);

      }
    }
    
    printf("\r                                           \nOK.\n\n");


  }else{
    printf("Standard input was not seekable.\n");
  }

  ov_clear(&ov);
  return 0;
}













