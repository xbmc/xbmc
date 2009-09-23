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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "timidity.h"
#include "common.h"
#include "wrd.h"
#include "mac_wrdwindow.h"
#include "mac_mag.h"

#pragma mark ==mag_header

typedef struct {
	uint8	header;
	uint8	machine_code;
	uint8	machine_depend_flag;
	uint8	screen_mode;
	uint16	x1,y1,x2,y2;
	uint32	offset_flagA;
	uint32	offset_flagB;
	uint32	size_flagB;
	uint32	offset_pixel;
	uint32	size_pixel;

	char	filename[256];
	int		width, hight;
	long	header_pos;
	uint8	flag[1280]; //for safty
	uint8	*flagA_work, *flagB_work;
	int		flagA_pos, flagB_pos, pixel_pos;
	Ptr		bitMap;
	int		rowBytes;
} Mag_Header;

static void read_flagA_setup(Mag_Header * mh, struct timidity_file *tf )
{
	int	 readbytes;
	
	readbytes=mh->offset_flagB - mh->offset_flagA;
	mh->flagA_work=(uint8*)safe_malloc( readbytes );
	tf_seek(tf, mh->header_pos + mh->offset_flagA, SEEK_SET);
	tf_read(mh->flagA_work, 1, readbytes, tf);
	mh->flagA_pos=0;
}

static void read_flagB_setup(Mag_Header * mh, struct timidity_file *tf )
{	
	mh->flagB_work=(uint8*)safe_malloc( mh->size_flagB );
	tf_seek(tf, mh->header_pos + mh->offset_flagB, SEEK_SET);
	tf_read(mh->flagB_work, 1, mh->size_flagB, tf);
	mh->flagB_pos=0;
}

static void read_flag_1line(Mag_Header * mh )
{
	int		x, flagA,flagB;
	
	for( x=mh->x1; x<=mh->x2; ){
		flagA= mh->flagA_work[mh->flagA_pos/8] & (0x80 >> (mh->flagA_pos & 0x07) );
		mh->flagA_pos++;
		if( flagA ){
			flagB= mh->flagB_work[mh->flagB_pos++];
		}else{
			flagB= 0;
		}
		mh->flag[x] ^= flagB>>4;	 x+=4;
		mh->flag[x] ^= flagB & 0x0F; x+=4;
	}
}

static void load_pixel(Mag_Header * mh, struct timidity_file *tf )
{
	int 		fpos,x,y,i, dx,dy;
	uint16		pixels;
	const int	DX[]={0,-4,-8,-16,  0,-4,  0,-4,-8,  0,-4,-8,  0,-4,-8, 0},
				DY[]={0, 0, 0,  0, -1,-1, -2,-2,-2, -4,-4,-4, -8,-8,-8, -16};
#define OUT_OF_DISP (x>=640 || y>=400)

	read_flagA_setup(mh, tf );
	read_flagB_setup(mh, tf );
	fpos= mh->header_pos + mh->offset_pixel;
	tf_seek(tf, fpos, SEEK_SET);
	for( y=mh->y1; y<=mh->y2; y++ ){
		read_flag_1line( mh );
		for( x=mh->x1; x<=mh->x2; x+=4 ){
			if( mh->flag[x]==0 ){
				tf_read(&pixels, 1, 2, tf);
				if( OUT_OF_DISP ) continue;
				for( i=3; i>=0; i-- ){
					*(uint8*)(&mh->bitMap[y*mh->rowBytes+x+i])= (pixels & 0x000F);
					pixels >>= 4;
				}
			} else {
				if( OUT_OF_DISP ) continue;
				dx=DX[mh->flag[x]];
				dy=DY[mh->flag[x]];
				 //*(uint32*)(&mh->bitMap[y*mh->rowBytes+x]) =  //copy 4bytes, danger?
				 //		 *(uint32*)(&mh->bitMap[(y+dy)*mh->rowBytes+ x+dx]);
				 mh->bitMap[y*mh->rowBytes+x  ]= mh->bitMap[(y+dy)*mh->rowBytes+ x+dx  ];
				 mh->bitMap[y*mh->rowBytes+x+1]= mh->bitMap[(y+dy)*mh->rowBytes+ x+dx+1];
				 mh->bitMap[y*mh->rowBytes+x+2]= mh->bitMap[(y+dy)*mh->rowBytes+ x+dx+2];
				 mh->bitMap[y*mh->rowBytes+x+3]= mh->bitMap[(y+dy)*mh->rowBytes+ x+dx+3];			
			}
		}
	}
}

int mac_mag_load(const char* fn, int dx, int dy, PixMapHandle pixmap,
							int mode, Rect *imageRect)
{										// pixmap must be locked by caller
										// store imagerect in this function
										// no err -> return 0; else return 1;
	uint8	buf[80];
	int		ret,i, err=0;
	struct timidity_file	*tf;
	static Mag_Header mag_header;
	RGBColor	color;
	
//	if( strcmp( mag_header.filename, fn)==0 ){
//		SetRect(imageRect, mag_header.x1, mag_header.y1, mag_header.x2+1, mag_header.y2+1);
//		return; //nothing to do
//	}
	
	if( (tf=wrd_open_file((char *)fn))==0 )
		return 1;
	
	// initialize table
	memset(&mag_header, 0, sizeof(Mag_Header) );
	strcpy( mag_header.filename, fn );
	mag_header.bitMap = GetPixBaseAddr(pixmap);
	mag_header.rowBytes= (**pixmap).rowBytes & 0x1FFF;
	
	// magic string check
	ret=tf_read(buf, 1, 8,tf);
	if( ret!=8 || memcmp(buf, "MAKI02  ",8)!=0 ){
		err=1;
		goto mac_mag_load_exit;
	}
	
	
	while( tf_getc(tf) != 0x1A ) //skip machine code,user name, comment
		/*nothing*/;
	
	mag_header.header_pos=tf_tell(tf); //get header position
	
	// read header	
	ret=tf_read(&mag_header, 1, 32, tf);
	if( ret!=32 ) goto mac_mag_load_exit; //unexpected end of file
	
	//transrate endian
	mag_header.x1=LE_SHORT(mag_header.x1);
	mag_header.y1=LE_SHORT(mag_header.y1);
	mag_header.x2=LE_SHORT(mag_header.x2);
	mag_header.y2=LE_SHORT(mag_header.y2);
	mag_header.offset_flagA=LE_LONG(mag_header.offset_flagA);
	mag_header.offset_flagB=LE_LONG(mag_header.offset_flagB);
	mag_header.size_flagB=LE_LONG(mag_header.size_flagB);
	mag_header.offset_pixel=LE_LONG(mag_header.offset_pixel);
	mag_header.size_pixel=LE_LONG(mag_header.size_pixel);
	
	mag_header.x1 &= ~0x7;
	mag_header.x2 |= 0x7;	

	if( dx!=WRD_NOARG ){
		int width= mag_header.x2-mag_header.x1;
		mag_header.x1= dx;
		mag_header.x2= mag_header.x1+width;
	}

	if( dy!=WRD_NOARG ){
		int hight= mag_header.y2-mag_header.y1;
		mag_header.y1= dy;
		mag_header.y2= mag_header.y1+hight;
	}
	
	
	
	mag_header.width=mag_header.x2-mag_header.x1+1;
	mag_header.hight=mag_header.y2-mag_header.y1+1;
	SetRect(imageRect, mag_header.x1, mag_header.y1, mag_header.x2+1, mag_header.y2+1);
	
	if( mag_header.screen_mode != 0 ){
		goto mac_mag_load_exit; //not support mode
	}
	
	//read pallet
	for( i=0; i<16; i++){
		ret=tf_read(buf, 1, 3, tf);
		if( ret!=3 ) goto mac_mag_load_exit; //unexpected end of file
		color.green=buf[0]*256;
		color.red=buf[1]*256;
		color.blue=buf[2]*256;
		dev_palette[17][i]=color; //(buf[1]>>4)*0x100 + (buf[0]>>4)*0x10 + (buf[2]>>4);
	}
	
	//ActivatePalette(qd.thePort);
	if( mode==0 || mode==1 )
		load_pixel(&mag_header, tf );
	
 mac_mag_load_exit:
	free(mag_header.flagA_work);
	free(mag_header.flagB_work);
	close_file(tf);
	return err;
}

// **********************************************************************
#pragma mark -
int mac_pho_load(const char* fn, PixMapHandle pm)
{
										// pixmap must be locked by caller
	uint8	buf[80];
	int		ret,i,j,x,y,rowBytes;
	struct timidity_file	*tf;
	Ptr		bitMap;

	SET_G_COLOR(0,graphicWorld[activeGraphics]);
	PaintRect(&portRect);

	if( (tf=wrd_open_file((char *)fn))==0 )
		return 1;	
	rowBytes= (**pm).rowBytes & 0x1FFF;
	bitMap = GetPixBaseAddr(pm);

	for( j=0; j<=3; j++){
		for( y=0; y<400; y++){
			for( x=0; x<640;  ){
				ret=tf_read(buf, 1, 1, tf);
				if( ret!=1 ) goto mac_mag_load_exit; //unexpected end of file
				//bitMap[y*rowBytes+x] &= 0x1F;
				for( i=7; i>=0; i--){
					bitMap[y*rowBytes+x+i] |= ((buf[0] & 0x01)<<j);
					buf[0]>>=1;
				}
				x+=8;
			}
		}
	}

 mac_mag_load_exit:
	close_file(tf);
	return 0;
}


