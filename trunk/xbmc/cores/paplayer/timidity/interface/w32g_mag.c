/* 
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	Macintosh interface for TiMidity
	by T.Nogami	<t-nogami@happy.email.ne.jp>
		
    mac_mag.c
    Macintosh mag loader
*/

/*
 *X mag loader By Takanori Watanabe. Based on Mac mag loader.
 */

/*
 * W32G Interface mag loader By Daisuke Aoki on X mag loader.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timidity.h"
#include "common.h"
#include "w32g_mag.h"
#include "controls.h"
#include "wrd.h"
#define BUFSIZE 512
static const char *MAGSIG="MAKI02  ";

#define READSHORT(p) ((int)*(uint8*)((p)+1)*256+(int)*(uint8 *)(p))
#define READLONG(p) (\
		     (int)*(uint8*)((p)+3)*256*256*256+\
		     (int)*(uint8*)((p)+2)*256*256+\
		     (int)*(uint8*)((p)+1)*256+\
		     (int)*(uint8 *)(p)\
		     )
#define MAGHDRLEN 31

static void mag_delete(magdata *mg);

static magdata *top=NULL;
struct magfilehdr{
	char machine;
	char mflag;
	char scrmode;
	char xorig[2];
	char yorig[2];
	char xend[2];
	char yend[2];
	char flagaoff[4];
	char flagboff[4];
	char flagbsize[4];
	char pxloff[4];
	char pxlsize[4];
};

static int read_flag_1line(magdata * mh,uint8 *flag )
{
	int		x, flagA,flagB,j;
	j=0;
	for( x=0; x<=mh->xend-(mh->xorig/8*8); ){
		flagA= mh->flagadata[mh->flagapos/8] & (0x80 >> (mh->flagapos & 0x07) );
		mh->flagapos++;
		if( flagA ){
			flagB= mh->flagbdata[mh->flagbpos++];
		}else{
			flagB= 0;
		}
		flag[x] ^= flagB>>4;	 x+=4;j++;
		flag[x] ^= flagB & 0x0F; x+=4;j++;
	}
	return j;
}

int pho_load_pixel ( char *image, int width, int height, char *filename)
{
	int i,j,k,l;
//	static const int shift[4]={0,2,1,3};/*B,R,G,E*/
	static const int shift[4]={0,1,2,3};
	struct timidity_file *fp;
	char buffer[640/8];
	
	if((fp=wrd_open_file(filename))==NULL)
		return 0;

	for(l=0;l<4;l++){
		for(i=0;i<400;i++){
			if(tf_read(buffer,sizeof(buffer),1,fp)==0){
				goto close;
			}
			for(j=0;j<sizeof(buffer);j++){
				for(k=0;k<8;k++) {
					if ( j * 8 + k + i * width >= width * height ) {
						close_file(fp);
						return 0;
					}
					if ( ( buffer[j]<<k ) & 0x80 )
						image[ j * 8 + k + i * width ] = image[ j * 8 + k + i * width] | ( 1<< shift[l] );
					else
						image[ j * 8 + k + i * width ] = image[ j * 8 + k + i * width ] | 0;
				}
			}
		}
	}
close:
	close_file(fp);
	return 1;
}

void mag_load_pixel ( char *image, int width, int height, magdata *mh )
{
	int 		x,y,i, dx,dy,pxlpos,w,j,repl;
	uint8 flag[800];
	long pixels;
	const int	DX[]={0,-4,-8,-16,  0,-4,  0,-4,-8,  0,-4,-8,  0,-4,-8, 0},
		DY[]={0, 0, 0,  0, -1,-1, -2,-2,-2, -4,-4,-4, -8,-8,-8, -16};
	memset(flag,0,800);
	mh->flagapos=mh->flagbpos=pxlpos=0;
	w = width;
	for( y=0; y<=mh->yend-mh->yorig; y++ ){
		repl=read_flag_1line( mh,flag );
		x=0;
		for( j=0; j<repl; j++ ){
			if( flag[x]==0 ){
				pixels=mh->pxldata[pxlpos]*256+mh->pxldata[pxlpos+1];
				pxlpos+=2;
				for( i=3; i>=0; i-- ){
					if(x+i < w)
						image [ x + i + y * width ] = pixels & 0x000F;
					pixels >>= 4;
				}
			} else {
				dx=DX[flag[x]];
				dy=DY[flag[x]];
				for(i=0;i<4;i++) {
					if(x+i < w)
						image [ x + i + y * width ] = image [ x+dx+i + (y+dy) * width ];
				}
			}
			x+=4;
		}
	}
}

void mag_deletetab(void)
{
	if(top!=NULL){
		mag_delete(top);
	}
	top=NULL;
}

static void mag_delete(magdata *mg){
	if(mg->next!=NULL)
		mag_delete(mg->next);
	free(mg->filename);
	free(mg->flagadata);
	free(mg->flagbdata);
	free(mg->pxldata);
	free(mg);
}

magdata *mag_search (char *filename )
{
	magdata *m;
	for(m=top;m!=NULL;m=m->next)
		if(strcmp(filename,m->filename)==0)
			return m;
		return NULL;
}

magdata *mag_create ( char *file )
{
	struct timidity_file *fp=NULL;
	int i;
	uint8 buffer[BUFSIZE];
	int *pal;
	magdata mg,*res;
	struct magfilehdr mfh;
	int len,c,header_pos;
	char *flaga=NULL,*flagb=NULL,*pixels=NULL;
	if((res=mag_search(file))){
		return res;
	}
	mg.filename=safe_malloc(strlen(file)+2);
	strcpy(mg.filename,file);
	fp=wrd_open_file(file);
	if(fp==NULL){
		goto error;
	}
	header_pos=0;
	while((c=tf_getc(fp))!=0){
		len=strlen(MAGSIG);
		if(header_pos<len)
			buffer[header_pos]=c;
		else if(header_pos==len){
			if(memcmp(MAGSIG,buffer,len)!=0){
				ctl->cmsg(CMSG_INFO,VERB_VERBOSE,"BAD SIGNATURE\n");
				goto error;
			}
		}
		header_pos++;
	}
	/*READ FHDR*/
	{
		if(tf_read(&mfh,sizeof(mfh),1,fp)==0){
			fprintf(stderr,"FOO!\n");
			goto error;
		}
		mg.machine=mfh.machine;
		mg.mflag=mfh.mflag;
		mg.scrmode=mfh.scrmode;
		mg.xorig=READSHORT(mfh.xorig);
		mg.yorig=READSHORT(mfh.yorig);
		mg.xend=READSHORT(mfh.xend);
		mg.yend=READSHORT(mfh.yend);
		mg.flagaoff=READLONG(mfh.flagaoff);
		mg.flagboff=READLONG(mfh.flagboff);
		mg.flagbsize=READLONG(mfh.flagbsize);
		mg.pxloff=READLONG(mfh.pxloff);
		mg.pxlsize=READLONG(mfh.pxlsize);
	}
	/*READ PALLET*/
//	pal=mg.pal+1;
	pal=mg.pal;
	for(i=0;i<PALSIZE;i++){
		if(tf_read(buffer,1,3,fp)<3){
			goto error;
		}
		pal[i] = 0;
		pal[i] |=  ( (int)( buffer[0] / 16 ) & 0x0F ) << 8;
		pal[i] |=  ( (int)( buffer[1] / 16 ) & 0x0F ) << 4;
		pal[i] |=  ( (int)( buffer[2] / 16 ) & 0x0F ) << 0;
	} 
	
	{
		int flaga_size,res;
		flaga_size=mg.flagboff-mg.flagaoff;
		if(tf_seek(fp,mg.flagaoff+header_pos,SEEK_SET)==-1)
			goto error;
		flaga=safe_malloc(flaga_size+100);
		if((res=tf_read(flaga,1,flaga_size,fp))<flaga_size)
			goto error;
		mg.flagadata=flaga;
	}
	{
		flagb=safe_malloc(mg.flagbsize+100);
		if(tf_seek(fp,mg.flagboff+header_pos,SEEK_SET)==-1)
			goto error;
		if(tf_read(flagb,1,mg.flagbsize,fp)<mg.flagbsize)
			goto error;
		mg.flagbdata=flagb;
	}
	{
		pixels=safe_malloc(mg.pxlsize+100);
		if(tf_seek(fp,mg.pxloff+header_pos,SEEK_SET)==-1)
			goto error;
		if(tf_read(pixels,1,mg.pxlsize,fp)<mg.pxlsize)
			goto error;
		mg.pxldata=pixels;
	}
	res=safe_malloc(sizeof(magdata));
	
	*res=mg;
	close_file(fp);
	res->next=top;
	top=res;
	return res;
error:
	if(fp!=NULL)
	{
		close_file(fp);
		ctl->cmsg(CMSG_INFO,VERB_VERBOSE,"Mag error: %s\n", file);
	}
	free(mg.filename);
	if(flaga != NULL)
		free(flaga);
	if(flagb != NULL)
		free(flagb);
	if(pixels != NULL)
		free(pixels);
	return NULL;
}
