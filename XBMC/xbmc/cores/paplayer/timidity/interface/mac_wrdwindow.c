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
		
    mac_wrdwindow.c
    Macintosh graphics driver for WRD
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdlib.h>
#include <string.h>
#include "timidity.h"

#define ENTITY 1
#include "mac_wrdwindow.h"

void dev_change_1_palette(int code, RGBColor color)
{
	(**(**dispWorld->portPixMap).pmTable).ctTable[code].rgb=color;	
	(**(**dispWorld->portPixMap).pmTable).ctSeed++;
	//CTabChanged( (**dispWorld->portPixMap).pmTable );
	dev_palette[0][code]=color;
	//if( wrd_palette ){
	//	SetEntryUsage(wrd_palette, code, pmAnimated, 0x0000);
	//	//SetEntryColor(wrd_palette, code, &color);
	//	AnimateEntry(wrd_palette, code, &color);
	//	SetEntryUsage(wrd_palette, code, pmTolerant, 0x0000);		
	//}
	pallette_exist |= color.red;
	pallette_exist |= color.green;
	pallette_exist |= color.blue;
}

void dev_change_palette(RGBColor pal[16])
{					// don't update graphics
	int i;
	//RGBColor color;
	pallette_exist=0;
	
	for( i=0; i<16; i++ ){
		pallette_exist |= pal[i].red;
		pallette_exist |= pal[i].green;
		pallette_exist |= pal[i].blue;
		dev_change_1_palette( i, pal[i]);
	}
	//dev_remake_disp(portRect);
	//dev_redisp(portRect);
}

void dev_init_text_color()
{
	int code;
	for( code=0; code<=7; code++){
		//(**(**graphicWorld[0]->portPixMap).pmTable).ctTable[TCODE2INDEX(code)].rgb=textcolor[code];
		//(**(**graphicWorld[1]->portPixMap).pmTable).ctTable[TCODE2INDEX(code)].rgb=textcolor[code];
		(**(**dispWorld->portPixMap).pmTable).ctTable[TCOLOR_INDEX_SHIFT+code].rgb=textcolor[code];
	
		//(**(**graphicWorld[0]->portPixMap).pmTable).ctSeed++;
		//(**(**graphicWorld[1]->portPixMap).pmTable).ctSeed++;
		(**(**dispWorld->portPixMap).pmTable).ctSeed++;
	}
}

static void expand_horizontality( PixMapHandle pixmap, int width, int height )
{			//pixmap must be locked	//for @TON(2)
	int	x,y;
	Ptr	baseAdr= GetPixBaseAddr(pixmap), xbase;
	int	rowBytes= (**pixmap).rowBytes & 0x1FFF;
	
	for( y=0; y<height; y++){
		xbase= baseAdr+ rowBytes*y;
		for( x=width-1; x>=0; x-- ){
			xbase[x*2]= xbase[x*2+1]= xbase[x];
		}
	}
}

void dev_draw_text_gmode(PixMapHandle pixmap, int x, int y, const char* s, int len,
		int pmask, int mode, int fgcolor, int bgcolor, int ton_mode)
{			//pixmap must be already locked
	GDHandle	oldGD;
	GWorldPtr	oldGW;
	int		color, trans, width;
	Rect		rect= {0,0,16,32},
			destrect;
	
	GetGWorld(&oldGW, &oldGD);
	LockPixels(charbufWorld->portPixMap);
	SetGWorld(charbufWorld,0);
		trans=0;
		if( fgcolor==trans ) trans++;
		if( bgcolor==trans ) trans++;
		if( fgcolor==trans ) trans++;
		
		color= ( (mode&2)? bgcolor:trans );
		dev_box(charbufWorld->portPixMap, rect, color, 0xFF);
		
		color= ( (mode&1)? fgcolor:trans );
		charbufWorld->fgColor= color;
		TextMode(srcOr);
		MoveTo(0,13);
		DrawText(s,0,len);
		if( ton_mode==2 ){
			expand_horizontality(charbufWorld->portPixMap, len*8, 16);
		}
		
		width= len*8;
		if( ton_mode==2 ) width*=2;
		
		rect.right=width;
		destrect.left=x; destrect.top=y;
		destrect.right=x+width; destrect.bottom=destrect.top+16;
		
		MyCopyBits(charbufWorld->portPixMap, pixmap,
			rect, destrect, 0x11/*trans*/, trans, pmask,
			0, 0, NULL);
		
		
	SetGWorld(oldGW, oldGD);
	UnlockPixels(charbufWorld->portPixMap);
}

static void BlockMoveData_transparent(const void* srcPtr, void *destPtr,
				Size byteCount, int pmask, int trans)
{
	int i, tmp;
	
	if( srcPtr>destPtr ){
		for( i=0; i<byteCount; i++){
			if( ((char*)srcPtr)[i]!=trans ){
				tmp=((char*)destPtr)[i];
				tmp &= ~pmask;
				tmp |= ( ((char*)srcPtr)[i] & pmask);
				((char*)destPtr)[i]= tmp;
			}
		}
	}else{
		for( i=byteCount-1; i>=0; i--){
			if( ((char*)srcPtr)[i]!=trans ){
				tmp=((char*)destPtr)[i];
				tmp &= ~pmask;
				tmp |= ( ((char*)srcPtr)[i] & pmask);
				((char*)destPtr)[i]= tmp;
			}
		}
	}
}

#define PMASK_COPY(s,d,pmask) ((d)=((d)&~(pmask))|((s)&(pmask)))

static pascal void BlockMoveData_gmode(const void* srcPtr, void *destPtr,
								 Size 	byteCount)
{
	int i;
	const char*	src=srcPtr;
	char*		dest=destPtr;
	
	if( srcPtr>destPtr ){
		for( i=0; i<byteCount; i++){
			PMASK_COPY(src[i],dest[i], gmode_mask);
		}
	}else{
		for( i=byteCount-1; i>=0; i--){
			PMASK_COPY(src[i],dest[i], gmode_mask);
		}
	}
}

static pascal void BlockMoveData_masktrans(const uint8*	srcPtr,	 uint8 *destPtr,
			Size 	byteCount, int trans, int maskx, const uint8 maskdata[])
{
#define  BITON(x, data) (data[(x)/8]&(0x80>>((x)%8)))
	int i;

	if( srcPtr>destPtr ){
		for( i=0; i<byteCount; i++){
			if(  srcPtr[i]!=trans  && BITON(i%maskx, maskdata)  ){
				PMASK_COPY(srcPtr[i],destPtr[i], gmode_mask);
			}
		}
	}else{
		for( i=byteCount-1; i>=0; i--){
			if( BITON(i%maskx, maskdata)  ){
				PMASK_COPY(srcPtr[i],destPtr[i], gmode_mask);
			}
		}
	}
}

#if __MC68K__
static pascal void mymemmove(const void* srcPtr, void * destPtr,Size byteCount)
{
	memmove(destPtr, srcPtr, byteCount);
}
 #define BlockMoveData mymemmove
#endif

void MyCopyBits(PixMapHandle srcPixmap, PixMapHandle dstPixmap,
		Rect srcRect, Rect dstRect, short mode, int trans, int pmask,
		int maskx, int masky, const uint8 maskdata[])
{														//I ignore destRect.right,bottom
	int srcRowBytes= (**srcPixmap).rowBytes & 0x1FFF,
		destRowBytes= (**dstPixmap).rowBytes & 0x1FFF,
		y1, y2, width,hight, cut, dy, maskwidth;
	Ptr	srcAdr= GetPixBaseAddr(srcPixmap),
		dstAdr= GetPixBaseAddr(dstPixmap);	
	Rect	srcBounds=  (**srcPixmap).bounds,
			dstBounds=  (**dstPixmap).bounds;

	
	//check params
	//chech src top
	if( srcRect.top<srcBounds.top ){
		cut= srcBounds.top-srcRect.top;
		srcRect.top+=cut; dstRect.top+=cut;
	}
	if( srcRect.top>srcBounds.bottom ) return;
	//check left
	if( srcRect.left  <srcBounds.left ){
		cut= srcBounds.left-srcRect.left;
		srcRect.left+= cut; dstRect.left+=cut;
	}
	if( srcRect.left>srcBounds.right ) return;
	//chech src bottom
	if( srcRect.bottom>srcBounds.bottom ){
		cut= srcRect.bottom-srcBounds.bottom;
		srcRect.bottom-= cut; dstRect.bottom-=cut;
	}
	if( srcRect.bottom<srcBounds.top ) return;
	//check right
	if( srcRect.right >srcBounds.right ){
		cut= srcRect.right-srcBounds.right;
		srcRect.right-= cut; srcBounds.right-= cut;
	}
	if( srcRect.right<srcBounds.left ) return;
	
	width=srcRect.right-srcRect.left;
	hight=srcRect.bottom-srcRect.top;
	
	//check dest
	//check top
	if( dstRect.top  <dstBounds.top ){
		cut= dstBounds.top-dstRect.top;
		srcRect.top+=cut; dstRect.top+=cut;
	}
	if( dstRect.top>dstBounds.bottom ) return;
	//check hight
	if( dstRect.top+hight>dstBounds.bottom ){	
		hight=dstBounds.bottom-dstRect.top;
		srcRect.bottom=srcRect.top+hight;
	}
	//check left
	if( dstRect.left <dstBounds.left ){
		cut= dstBounds.left-dstRect.left;
		srcRect.left+= cut; dstRect.left+=cut;
	}
	if( dstRect.left>dstBounds.right ) return;
	//check width
	if( dstRect.left+width>dstBounds.right )
		width=dstBounds.right-dstRect.left;
	
	switch( mode ){
	case 0://srcCopy
	case 0x10:
		{
			pascal void (*func)(const void* srcPtr, void *	destPtr,Size byteCount);
			if( pmask==0xFF ) func=BlockMoveData;
				else func= BlockMoveData_gmode;
			if( srcRect.top >= dstRect.top ){
				for( y1=srcRect.top, y2=dstRect.top; y1<srcRect.bottom; y1++,y2++ ){
					func( &(srcAdr[y1*srcRowBytes+srcRect.left]),
									&(dstAdr[y2*destRowBytes+dstRect.left]), width);
				}
			}else{
				for( y1=srcRect.bottom-1, y2=dstRect.top+hight-1; y1>=srcRect.top; y1--, y2-- ){
					func( &(srcAdr[y1*srcRowBytes+srcRect.left]),
									&(dstAdr[y2*destRowBytes+dstRect.left]), width);
				}
			}
		}
		break;
	case 0x11://transparent
		if( srcRect.top >= dstRect.top ){
			for( y1=srcRect.top, y2=dstRect.top; y1<srcRect.bottom; y1++,y2++ ){
				BlockMoveData_transparent( &(srcAdr[y1*srcRowBytes+srcRect.left]),
							&(dstAdr[y2*destRowBytes+dstRect.left]), width, pmask, trans);
			}
		}else{
			for( y1=srcRect.bottom-1, y2=dstRect.top+hight-1; y1>=srcRect.top; y1--, y2-- ){
				BlockMoveData_transparent( &(srcAdr[y1*srcRowBytes+srcRect.left]),
							&(dstAdr[y2*destRowBytes+dstRect.left]), width, pmask, trans);
			}
		}
		break;
	case 0x30:
	case 0x31: // masking & transparent //sherry op=0x62
		if( maskx<=0 ) break;
		maskwidth= ((maskx+7)& ~0x07)/8; //kiriage
		if( srcRect.top >= dstRect.top ){
			for( y1=srcRect.top, y2=dstRect.top, dy=0; y1<srcRect.bottom; y1++,y2++,dy++,dy%=masky ){
				BlockMoveData_masktrans( (uint8 *)&(srcAdr[y1*srcRowBytes+srcRect.left]),
					(uint8 *)&(dstAdr[y2*destRowBytes+dstRect.left]), width, trans,
					maskx, &maskdata[maskwidth*dy]);
			}
		}else{
			for( y1=srcRect.bottom-1, y2=dstRect.top+hight-1,dy=hight-1; y1>=srcRect.top; y1--, y2--,dy+=masky-1, dy%=masky ){
				BlockMoveData_masktrans( (uint8 *)&(srcAdr[y1*srcRowBytes+srcRect.left]),
					(uint8 *)&(dstAdr[y2*destRowBytes+dstRect.left]), width, trans,
					maskx, &maskdata[maskwidth*dy]);
			}
		}
		break;
	}
}

void dev_line(int x1, int y1, int x2, int y2, int color, int style,
	int pmask, PixMapHandle pixmap )
{
	int	i, dx, dy, s, step;
	int	rowBytes= (**pixmap).rowBytes & 0x1FFF;
	Ptr	baseAdr= GetPixBaseAddr(pixmap);
	Rect	bounds=  (**pixmap).bounds;
	Point	pt;
	static const int mask[8]={0x80,0x40,0x20,0x10, 0x08,0x04,0x02,0x01};
	int	style_count=0;

#define DOT(x,y,col) {Ptr p=&baseAdr[y*rowBytes+x]; pt.h=x;pt.v=y;    \
		if(PtInRect(pt,&bounds)){(*p)&=~pmask; (*p)|=col;} }
	
	color &= pmask;
	step= ( (x1<x2)==(y1<y2) ) ? 1:-1;
	dx= abs(x2-x1); dy=abs(y2-y1);
	
	if( dx>dy ){
		if( x1>x2 ){ x1=x2; y1=y2; }
		if(style & mask[style_count]){ DOT(x1,y1,color); }
					//else { DOT(x1,y1,0); }
		style_count= (style_count+1)%8;
		s= dx/2;
		for(i=x1+1; i<x1+dx; i++){
			s-= dy;
			if( s<0 ){ s+=dx; y1+=step;}
			if(style & mask[style_count]){ DOT(i,y1,color); }
						//else{ DOT(i,y1,0); }
			style_count= (style_count+1)%8;
		}
	}else{
		if( y1>y2 ){ x1=x2; y1=y2; }
		if(style & mask[style_count]){ DOT(x1,y1,color); }
					//else{ DOT(x1,y1,0); }
		style_count= (style_count+1)%8;
		s= dy/2;
		for(i=y1+1; i<y1+dy; i++){
			s-= dx;
			if( s<0 ){ s+=dy; x1+=step;}
			if(style & mask[style_count]){ DOT(x1,i,color); }
						//else{ DOT(x1,i,0); }
			style_count= (style_count+1)%8;
		}
	}
}

void dev_box(PixMapHandle pixmap, Rect rect, int color, int pmask)
{
	int		rowBytes= (**pixmap).rowBytes & 0x1FFF,
			x, y1, width,hight, tmp;
	Ptr		baseAdr= GetPixBaseAddr(pixmap);
	Rect	bounds=  (**pixmap).bounds;
	
	//check params
	//chech src top
	if( rect.top<bounds.top ){
		rect.top=bounds.top;
	}
	if( rect.top>bounds.bottom ) return;
	//check left
	if( rect.left  <bounds.left ){
		rect.left= bounds.left;
	}
	if( rect.left>bounds.right ) return;
	//chech src bottom
	if( rect.bottom>bounds.bottom ){
		rect.bottom= bounds.bottom;
	}
	if( rect.bottom<bounds.top ) return;
	//check right
	if( rect.right >bounds.right ){
		rect.right= bounds.right;
	}
	if( rect.right<bounds.left ) return;
	
	width=rect.right-rect.left;
	hight=rect.bottom-rect.top;
	color &= pmask;

		for( y1=rect.top; y1<rect.bottom; y1++ ){
			for( x=rect.left; x<rect.right; x++){
				tmp=baseAdr[y1*rowBytes+x];
				tmp &= ~pmask;
				tmp |= color;
				baseAdr[y1*rowBytes+x]=tmp;
			}
		}
	}

void mac_setfont(GWorldPtr world, Str255 fontname)
{
	GDHandle	oldGD;
	GWorldPtr	oldGW;
	
	GetGWorld(&oldGW, &oldGD);
	LockPixels(world->portPixMap);
	{
		short		fontID;
		SetGWorld( world, 0);
		GetFNum(fontname, &fontID);
		TextFont(fontID);
		TextSize(14);
		TextFace(extend/*|bold*/);
	}
	SetGWorld(oldGW, oldGD);
	UnlockPixels(world->portPixMap);
}
