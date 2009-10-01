/*
 *
 * utils for AAC informations
*/
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define ADTS_HEADER_SIZE        8
#define SEEK_TABLE_CHUNK        60
#define MPEG4_TYPE              0
#define MPEG2_TYPE              1

// Read ADTS header, the file descriptor must be at
// the begining of the aac frame not at the id3tag

int	getAacInfo(FILE *fd)
{
  unsigned char	header[ADTS_HEADER_SIZE];
  unsigned int	id;
  unsigned long	originPosition;

  originPosition = ftell(fd);
  if(fread(header, 1, ADTS_HEADER_SIZE, fd) != ADTS_HEADER_SIZE){
    fseek(fd, originPosition, SEEK_SET);
    return(-1);
  }
  if(!((header[0]==0xFF)&&((header[1]& 0xF6)==0xF0))){
    printf("Bad header\n");
    return(-1);
  }
  id = header[1]&0x08;
  if(id==0){//MPEG-4 AAC
    fseek(fd, originPosition, SEEK_SET);
    return(MPEG4_TYPE);
  }else{
    fseek(fd, originPosition, SEEK_SET);
    return(MPEG2_TYPE);
  }
  fseek(fd, originPosition, SEEK_SET);
  return(-1);
}

// as AAC is VBR we need to check all ADTS header
// to enable seeking...
// there is no other solution
void	checkADTSForSeeking(FILE *fd,
			    unsigned long **seekTable,
			    unsigned long *seekTableLength)
{
  unsigned long	originPosition;
  unsigned long	position;
  unsigned char	header[ADTS_HEADER_SIZE];
  unsigned int	frameCount, frameLength, frameInsec;
  unsigned int	id=0, seconds=0;

  originPosition = ftell(fd);

  for(frameCount=0,frameInsec=0;; frameCount++,frameInsec++){
    position = ftell(fd);
    if(fread(header, 1, ADTS_HEADER_SIZE, fd)!=ADTS_HEADER_SIZE){
      break;
    }
    if(!strncmp(header, "ID3", 3)){
      break;
    }
    if(!((header[0]==0xFF)&&((header[1]& 0xF6)==0xF0))){
      printf("error : Bad 1st header, file may be corrupt !\n");
      break;
    }
    if(!frameCount){
      id=header[1]&0x08;
      if(((*seekTable) = malloc(SEEK_TABLE_CHUNK * sizeof(unsigned long)))==0){
	printf("malloc error\n");
	return;
      }
      (*seekTableLength) = SEEK_TABLE_CHUNK;
    }

    //if(id==0){//MPEG-4
    //frameLength = ((unsigned int)header[4]<<5)|((unsigned int)header[5]>>3);
    //}else{//MPEG-2
      frameLength = (((unsigned int)header[3]&0x3)<<11)|((unsigned int)header[4]<<3)|(header[5]>>5);
      //}
    if(frameInsec==43){//???
      frameInsec=0;
    }
    if(frameInsec==0){
      if(seconds == (*seekTableLength)){
	(*seekTable) = realloc((*seekTable), (seconds+SEEK_TABLE_CHUNK)*sizeof(unsigned long));
	(*seekTableLength) = seconds+SEEK_TABLE_CHUNK;
      }
      (*seekTable)[seconds] = position;
      seconds++;
    }
    if(fseek(fd, frameLength-ADTS_HEADER_SIZE, SEEK_CUR)==-1){
      break;
    }
  }
  (*seekTableLength) = seconds;
  fseek(fd, originPosition, SEEK_SET);
}
