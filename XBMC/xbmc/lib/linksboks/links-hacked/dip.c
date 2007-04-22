/* dip.c
 * Digital Image Processing routines
 * (c) 2000-2002 Karel 'Clock' Kulhavy
 * This file is a part of the Links program, released under GPL.
 * This does various utilities for digital image processing.
 */

#include "cfg.h"

#ifdef G

#include "links.h"

#ifdef HAVE_MATH_H
#include <math.h>
#endif /* HAVE_MATH_H */

#endif

double user_gamma; /* 1.0 for 64 lx. This is the number user directly changes in the menu */
			  
double display_red_gamma; /* Red gamma exponent of the display */
double display_green_gamma; /* Green gamma exponent of the display */
double display_blue_gamma; /* Blue gamma exponent of the display */
#ifdef G

/* #define this if you want to report missing letters to stderr.
 * #undef this if you don't */
#undef REPORT_UNKNOWN

double sRGB_gamma=0.45455;      /* For HTML, which runs
				 * according to sRGB standard. Number
				 * in HTML tag is linear to photons raised
				 * to this power.
				 */

unsigned long aspect=65536; /* aspect=65536 for 320x240
		        * aspect=157286 for 640x200
                        * Defines aspect ratio of screen pixels. 
		        * aspect=(196608*xsize+ysize<<1)/(ysize<<1);
		        * Default is 65536 because we assume square pixel
			* when not specified otherwise. */
unsigned long aspect_native=65536; /* Like aspect, but not influenced by
			* user, just determined by graphics driver.
			*/


/* Limitation: No input image's dimension may exceed 2^(32-1-8) pixels.
 */

/* Each input byte represents 1 byte (gray). The question whether 0 is
 * black or 255 is black doesn't matter.
 */

inline static void add_col_gray(unsigned *col_buf, unsigned char *ptr, int
		line_skip, int n, unsigned weight)
{
	for (;n;n--){
		*col_buf+=weight*(*ptr);
		ptr+=line_skip;
		col_buf++;
	}
}

/* line_skip is in pixels. The column contains the whole pixels (R G B)
 * We assume unsigned short holds at least 16 bits. */
inline static void add_col_color(unsigned *col_buf, unsigned short *ptr
	, int line_skip, int n, unsigned weight)
{
	for (;n;n--){
		*col_buf+=weight*(*ptr);
		col_buf[1]+=weight*ptr[1];
		col_buf[2]+=weight*ptr[2];
		ptr+=line_skip;
		col_buf+=3;
	}
}

 /* We assume unsigned short holds at least 16 bits. */
inline static void add_row_gray(unsigned *row_buf, unsigned char *ptr, int n,
	unsigned weight)
{
	for (;n;n--){
		*row_buf+=weight**ptr;
		ptr++;
		row_buf++;
	}
}

/* n is in pixels. pixel is 3 unsigned shorts in series */
 /* We assume unsigned short holds at least 16 bits. */
inline static void add_row_color(unsigned *row_buf, unsigned short *ptr, int n, unsigned weight)
{
	for (;n;n--){
		*row_buf+=weight**ptr;
		row_buf[1]+=weight*ptr[1];
		row_buf[2]+=weight*ptr[2];
		ptr+=3;
		row_buf+=3;
	}
}

/* We assume unsigned holds at least 32 bits */
inline static void emit_and_bias_col_gray(unsigned *col_buf, unsigned char *out, int
		line_skip, int n, unsigned weight)
{
	unsigned half=weight>>1;

	for (;n;n--){
		*out=*col_buf/weight;
		out+=line_skip;
		*col_buf++=half;
	}
}

/* We assume unsigned holds at least 32 bits */
static inline void bias_buf_gray(unsigned *col_buf, int n, unsigned half)
{
	for (;n;n--) *col_buf++=half;
}

/* We assume unsigned holds at least 32 bits */
inline static void bias_buf_color(unsigned *col_buf, int n, unsigned half)
{
	for (;n;n--){
		*col_buf=half;
		col_buf[1]=half;
		col_buf[2]=half;
		col_buf+=3;
	}
	/* System activated */
}
		
/* line skip is in pixels. Pixel is 3*unsigned short */
/* We assume unsigned holds at least 32 bits */
/* We assume unsigned short holds at least 16 bits. */
inline static void emit_and_bias_col_color(unsigned *col_buf
	, unsigned short *out, int line_skip, int n, unsigned weight)
{
	unsigned half=weight>>1;

	for (;n;n--){
		*out=(*col_buf)/weight;
		*col_buf=half;
		out[1]=col_buf[1]/weight;
		col_buf[1]=half;
		/* The following line is an enemy of the State and will be
		 * prosecuted according to the Constitution of The United States
		 * Cap. 20/3 ix. Sel. Bill 12/1920
		 * Moses 12/20 Erizea farizea 2:2:1:14
		 */
		out[2]=col_buf[2]/weight;
		col_buf[2]=half;
		out+=line_skip;
		col_buf+=3;
	}
}
		
/* We assume unsigned holds at least 32 bits */
inline static void emit_and_bias_row_gray(unsigned *row_buf, unsigned char *out, int n
		,unsigned weight)
{
	unsigned half=weight>>1;
	
	for (;n;n--){
		*out++=*row_buf/weight;
		*row_buf++=half;
	}
}

/* n is in pixels. pixel is 3 unsigned shorts in series. */
/* We assume unsigned holds at least 32 bits */
/* We assume unsigned short holds at least 16 bits. */
inline static void emit_and_bias_row_color(unsigned *row_buf, unsigned short
		*out, int n, unsigned weight)
{
	unsigned half=weight>>1;

	for (;n;n--){
		*out=*row_buf/weight;
		*row_buf=half;
		out[1]=row_buf[1]/weight;
		row_buf[1]=half;
		out[2]=row_buf[2]/weight;
		row_buf[2]=half;
		out+=3;
		row_buf+=3;
	}
}
		
/* For enlargement only -- does linear filtering.
 * Allocates output and frees input.
 * We assume unsigned holds at least 32 bits */
inline static void enlarge_gray_horizontal(unsigned char *in, int ix, int y
	,unsigned char ** out, int ox)
{
	unsigned *col_buf;
	int total;
	int out_pos,in_pos,in_begin,in_end;
	unsigned half=(ox-1)>>1;
	unsigned char *outptr;
	unsigned char *inptr;

	outptr=mem_alloc(ox*y);
	inptr=in;
	*out=outptr;
	if (ix==1){
		/* Dull copying */
		for (;y;y--){
			memset(outptr,*inptr,ox);
			outptr+=ox;
			inptr++;
		}
		mem_free(in);
	}else{
		total=(ix-1)*(ox-1);
		col_buf=mem_alloc(y*sizeof(*col_buf));
		bias_buf_gray(col_buf, y, half);
		out_pos=0;
		in_pos=0;
		again:
		in_begin=in_pos;
		in_end=in_pos+ox-1;
		add_col_gray(col_buf,inptr, ix, y, in_end-out_pos);
		add_col_gray(col_buf,inptr+1, ix, y, out_pos-in_begin);
		emit_and_bias_col_gray(col_buf,outptr,ox,y,ox-1);
		outptr++;
		out_pos+=ix-1;
		if (out_pos>in_end){
			in_pos=in_end;
			inptr++;
		}
		if (out_pos>total){
			mem_free(in);
			mem_free(col_buf);
			return;
		}
		goto again;
	}
	/* Rohan, oh Rohan... */
}

/* For enlargement only -- does linear filtering
 * Frees input and allocates output.
 * We assume unsigned holds at least 32 bits
 */
static inline void enlarge_color_horizontal(unsigned short *ina, int ix, int y,
	unsigned short ** outa, int ox)
{
	unsigned *col_buf;
	int total,a,out_pos,in_pos,in_begin,in_end;
	unsigned half=(ox-1)>>1;
	unsigned skip=3*ix;
	unsigned oskip=3*ox;
	unsigned short *out, *in;

	if (ix==ox){
		*outa=ina;
		return;
	}
	out=mem_alloc(sizeof(*out)*3*ox*y);
	*outa=out;
	in=ina;
	if (ix==1){
		for (;y;y--,in+=3) for (a=ox;a;a--,out+=3){
			*out=*in;
			out[1]=in[1];
			out[2]=in[2];
		}
		mem_free(ina);
		return;
	}
	total=(ix-1)*(ox-1);
	col_buf=mem_alloc(y*3*sizeof(*col_buf));
	bias_buf_color(col_buf,y,half);
	out_pos=0;
	in_pos=0;
	again:
	in_begin=in_pos;
	in_end=in_pos+ox-1;
	add_col_color(col_buf,in,skip,y
		,in_end-out_pos);
	add_col_color(col_buf,in+3,skip,y
		,out_pos-in_begin);
	emit_and_bias_col_color(col_buf,out,oskip,y,ox-1);
	out+=3;
	out_pos+=ix-1;
	if (out_pos>in_end){
		in_pos=in_end;
		in+=3;
	}
	if (out_pos>total){
		mem_free(col_buf);
		mem_free(ina);
		return;
	}
	goto again;
}

/* Works for both enlarging and diminishing. Linear resample, no low pass.
 * Automatically mem_frees the "in" and allocates "out". */
/* We assume unsigned holds at least 32 bits */
inline static void scale_gray_horizontal(unsigned char *in, int ix, int y
	,unsigned char ** out, int ox)
{
	unsigned *col_buf;
	int total=ix*ox;
	int out_pos,in_pos,in_begin,in_end,out_end;
	unsigned char *outptr;
	unsigned char *inptr;

	if (ix<ox){
		enlarge_gray_horizontal(in,ix,y,out,ox);
		return;
	}else if (ix==ox){
		*out=in;
		return;
	}
	outptr=mem_alloc(ox*y);
	inptr=in;
	*out=outptr;
	col_buf=mem_alloc(y*sizeof(*col_buf));
	bias_buf_gray(col_buf, y, ix>>1);
	out_pos=0;
	in_pos=0;
	again:
	in_begin=in_pos;
	in_end=in_pos+ox;
	out_end=out_pos+ix;
	if (in_begin<out_pos)in_begin=out_pos;
	if (in_end>out_end)in_end=out_end;
	add_col_gray(col_buf,inptr,ix,y,in_end-in_begin);
	in_end=in_pos+ox;
	if (out_end>=in_end){
		in_pos=in_end;
		inptr++;
	}
	if (out_end<=in_end){
			emit_and_bias_col_gray(col_buf,outptr,ox,y,ix);
			out_pos=out_pos+ix;
			outptr++;
	}
	if (out_pos==total) {
		mem_free(in);
		mem_free(col_buf);
		return;
	}
	goto again;
}

/* Works for both enlarging and diminishing. Linear resample, no low pass.
 * Does only one color component.
 * Frees ina and allocates outa.
 * If ox*3<=ix, and display_optimize, performs optimization for LCD.
 */
inline static void scale_color_horizontal(unsigned short *ina, int ix, int y,
		unsigned short **outa, int ox)
{
	unsigned *col_buf;
	int total=ix*ox;
	int out_pos,in_pos,in_begin,in_end,out_end;
	unsigned skip=3*ix;
	unsigned oskip=3*ox;
	unsigned short *in, *out;

	if (ix==ox){
		*outa=ina;
		return;
	}
	if (ix<ox){
		enlarge_color_horizontal(ina,ix,y,outa,ox);
		return;
	}else if (ix==ox){
		*outa=ina;
		return;
	}
	out=mem_alloc(sizeof(*out)*3*ox*y);
	*outa=out;
	in=ina;
	col_buf=mem_alloc(y*3*sizeof(*col_buf));
	bias_buf_color(col_buf,y,ix>>1);
	out_pos=0;
	in_pos=0;
	again:
	in_begin=in_pos;
	in_end=in_pos+ox;
	out_end=out_pos+ix;
	if (in_begin<out_pos)in_begin=out_pos;
	if (in_end>out_end)in_end=out_end;
	add_col_color(col_buf,in,skip,y,in_end-in_begin);
	in_end=in_pos+ox;
	if (out_end>=in_end){
		in_pos=in_end;
		in+=3;
	}
	if (out_end<=in_end){
			emit_and_bias_col_color(col_buf,out,oskip,y,ix);
			out_pos=out_pos+ix;
			out+=3;
	}
	if (out_pos==total) {
		mem_free(ina);
		mem_free(col_buf);
		return;
	}
	goto again;
}

/* For magnification only. Does linear filtering. */
/* We assume unsigned holds at least 32 bits */
inline static void enlarge_gray_vertical(unsigned char *in, int x, int iy,
	unsigned char ** out ,int oy)
{
	unsigned *row_buf;
	int total;
	int out_pos,in_pos,in_begin,in_end;
	int half=(oy-1)>>1;
	unsigned char *outptr;
	unsigned char *inptr;

	if (iy==1){
		outptr=mem_alloc(oy*x);
		*out=outptr;
		for(;oy;oy--,outptr+=x)
			memcpy(outptr,in,x);
		mem_free(in);
	}
	else if (iy==oy){
		*out=in;
	}else{
		outptr=mem_alloc(oy*x);
		inptr=in;
		*out=outptr;
		total=(iy-1)*(oy-1);
		row_buf=mem_alloc(x*sizeof(*row_buf));
		bias_buf_gray(row_buf, x, half);
		out_pos=0;
		in_pos=0;
		again:
		in_begin=in_pos;
		in_end=in_pos+oy-1;
		add_row_gray(row_buf, inptr, x, in_end-out_pos);
		add_row_gray(row_buf, inptr+x, x, out_pos-in_begin);
		emit_and_bias_row_gray(row_buf, outptr, x, oy-1);
		outptr+=x;
		out_pos+=iy-1;
		if (out_pos>in_end){
			in_pos=in_end;
			inptr+=x;
		}
		if (out_pos>total){
			mem_free(in);
			mem_free(row_buf);
			return;
		}
		goto again;
	}	
}

/* For magnification only. Does linear filtering */
/* We assume unsigned holds at least 32 bits */
inline static void enlarge_color_vertical(unsigned short *ina, int x, int iy,
	unsigned short **outa ,int oy)
{
	unsigned *row_buf;
	int total,out_pos,in_pos,in_begin,in_end;
	int half=(oy-1)>>1;
	unsigned short *out, *in;

	if (iy==oy){
		*outa=ina;
		return;
	}
	/* Rivendell */
	out=mem_alloc(sizeof(*out)*3*oy*x);
	*outa=out;
	in=ina;
	if (iy==1){
		for (;oy;oy--){
	       		memcpy(out,in,3*x*sizeof(*out));
	       		out+=3*x;
		}
		mem_free(ina);
		return;
	}
	total=(iy-1)*(oy-1);
	row_buf=mem_alloc(x*3*sizeof(*row_buf));
	bias_buf_color(row_buf,x,half);
	out_pos=0;
	in_pos=0;
	again:
	in_begin=in_pos;
	in_end=in_pos+oy-1;
	add_row_color(row_buf,in,x
		,in_end-out_pos);
	add_row_color(row_buf,in+3*x,x
		,out_pos-in_begin);
	emit_and_bias_row_color(row_buf,out,x,oy-1);
	out+=3*x;
	out_pos+=iy-1;
	if (out_pos>in_end){
		in_pos=in_end;
		in+=3*x;
	}
	if (out_pos>total){
		mem_free(ina);
		mem_free(row_buf);
		return;
	}
	goto again;
	
}	

/* Both enlarges and diminishes. Linear filtering.
 * Automatically allocates output and frees input.
 * We assume unsigned holds at least 32 bits */
inline static void scale_gray_vertical(unsigned char *in, int x, int iy,
	unsigned char ** out ,int oy)
{
	unsigned *row_buf;
	int total=iy*oy;
	int out_pos,in_pos,in_begin,in_end,out_end;
	unsigned char *outptr;
	unsigned char *inptr;

	/* Snow White, Snow White... */
	if (iy<oy){
		enlarge_gray_vertical(in,x,iy,out,oy);
		return;
	}
	if (iy==oy){
		*out=in;
		return;
	}
	outptr=mem_alloc(x*oy);
	inptr=in;
	*out=outptr;
	row_buf=mem_calloc(x*sizeof(*row_buf));
	bias_buf_gray(row_buf, x, iy>>1);
	out_pos=0;
	in_pos=0;
	again:
	in_begin=in_pos;
	in_end=in_pos+oy;
	out_end=out_pos+iy;
	if (in_begin<out_pos)in_begin=out_pos;
	if (in_end>out_end)in_end=out_end;
	add_row_gray(row_buf,inptr,x,in_end-in_begin);
	in_end=in_pos+oy;
	if (out_end>=in_end){
		in_pos=in_end;
		inptr+=x;
	}
	if (out_end<=in_end){
			emit_and_bias_row_gray(row_buf,outptr,x,iy);
			out_pos=out_pos+iy;
			outptr+=x;
	}
	if (out_pos==total){
		mem_free(in);
		mem_free(row_buf);
		return;
	}
	goto again;
}

/* Both enlarges and diminishes. Linear filtering. Sizes are
   in pixels. Sizes are not in bytes. 1 pixel=3 unsigned shorts.
   We assume unsigned short can hold at least 16 bits.
   We assume unsigned holds at least 32 bits.
 */
inline static void scale_color_vertical(unsigned short *ina, int x, int iy
	,unsigned short **outa, int oy)
{
	unsigned *row_buf;
	int total=iy*oy;
	int out_pos,in_pos,in_begin,in_end,out_end;
	unsigned short *in, *out;

	if (iy==oy){
		*outa=ina;
		return;
	}
	if (iy<oy){
		enlarge_color_vertical(ina,x,iy,outa,oy);
		return;
	}
	out=mem_alloc(sizeof(*out)*3*oy*x);
	*outa=out;
	in=ina;
	row_buf=mem_alloc(x*3*sizeof(*row_buf));
	bias_buf_color(row_buf,x,iy>>1);
	out_pos=0;
	in_pos=0;
	again:
	in_begin=in_pos;
	in_end=in_pos+oy;
	out_end=out_pos+iy;
	if (in_begin<out_pos)in_begin=out_pos;
	if (in_end>out_end)in_end=out_end;
	add_row_color(row_buf,in,x,in_end-in_begin);
	in_end=in_pos+oy;
	if (out_end>=in_end){
		in_pos=in_end;
		in+=3*x;
	}
	if (out_end<=in_end){
			emit_and_bias_row_color(row_buf,out,x,iy);
			out_pos=out_pos+iy;
			out+=3*x;
	}
	if (out_pos==total){
		mem_free(ina);
		mem_free(row_buf);
		return;
	}
	goto again;
}


/* Scales grayscale 8-bit map. Both enlarges and diminishes. Uses either low
 * pass or bilinear filtering. Automatically mem_frees the "in".
 * Automatically allocates "out".
 */
#ifndef __XBOX__
inline
#endif
void scale_gray(unsigned char *in, int ix, int iy, unsigned char **out
	,int ox, int oy)
{
	unsigned char *intermediate_buffer;

	if (!ix||!iy){
		if (in) mem_free(in);
		*out=mem_calloc(ox*oy);
		return;
	}
	if (ix*oy<ox*iy){
		scale_gray_vertical(in,ix,iy,&intermediate_buffer,oy);
		scale_gray_horizontal(intermediate_buffer,ix,oy,out,ox);
	}else{
		scale_gray_horizontal(in,ix,iy,&intermediate_buffer,ox);
		scale_gray_vertical(intermediate_buffer,ox,iy,out,oy);
	}
}

/* To be called only when global variable display_optimize is 1 or 2.
 * Performs a decimation according to this variable. Data shrink to 1/3
 * and x is the smaller width.
 * There must be 9*x*y unsigned shorts of data.
 * x must be >=1.
 * Performs realloc onto the buffer after decimation to save memory.
 */
void decimate_3(unsigned short **data0, int x, int y)
{
	unsigned short *data=*data0;
	unsigned short *ahead=data;
	int i, futuresize=x*y*3*sizeof(**data0);
	
#ifdef DEBUG
	if (!(x>0&&y>0)) internal("zero width or height in decimate_3");
#endif /* #Ifdef DEBUG */
	if (display_optimize==1){
		if (x==1){
			for (;y;y--,ahead+=9,data+=3){
				data[0]=(ahead[0]+ahead[0]+ahead[3])/3;
				data[1]=(ahead[1]+ahead[4]+ahead[7])/3;
				data[2]=(ahead[5]+ahead[8]+ahead[8])/3;
			}
		}else{
			for (;y;y--){
				data[0]=(ahead[0]+ahead[0]+ahead[3])/3;
				data[1]=(ahead[1]+ahead[4]+ahead[7])/3;
				data[2]=(ahead[5]+ahead[8]+ahead[11])/3;
				for (ahead+=9,data+=3,i=x-2;i;i--,ahead+=9,data+=3){
					data[0]=(ahead[-3]+ahead[0]+ahead[3])/3;
					data[1]=(ahead[1]+ahead[4]+ahead[7])/3;
					data[2]=(ahead[5]+ahead[8]+ahead[11])/3;
				}
				data[0]=(ahead[-3]+ahead[0]+ahead[3])/3;
				data[1]=(ahead[1]+ahead[4]+ahead[7])/3;
				data[2]=(ahead[5]+ahead[8]+ahead[8])/3;
				ahead+=9,data+=3;
			}
		}
	}else{
		/* display_optimize==2 */
		if (x==1){
			for (;y;y--,ahead+=9,data+=3){
				data[0]=(ahead[3]+ahead[6]+ahead[6])/3;
				data[1]=(ahead[1]+ahead[4]+ahead[7])/3;
				data[2]=(ahead[2]+ahead[2]+ahead[5])/3;
			}
		}else{
			for (;y;y--){
				data[0]=(ahead[3]+ahead[6]+ahead[9])/3;
				data[1]=(ahead[1]+ahead[4]+ahead[7])/3;
				data[2]=(ahead[2]+ahead[2]+ahead[5])/3;
				for (ahead+=9,data+=3,i=x-2;i;i--,ahead+=9,data+=3){
					data[0]=(ahead[3]+ahead[6]+ahead[9])/3;
					data[1]=(ahead[1]+ahead[4]+ahead[7])/3;
					data[2]=(ahead[-1]+ahead[2]+ahead[5])/3;
				}
				data[0]=(ahead[3]+ahead[6]+ahead[6])/3;
				data[1]=(ahead[1]+ahead[4]+ahead[7])/3;
				data[2]=(ahead[-1]+ahead[2]+ahead[5])/3;
				ahead+=9,data+=3;
			}
		}
	}	
	*data0=mem_realloc(*data0,futuresize);
}

/* Scales color 48-bits-per-pixel bitmap. Both enlarges and diminishes. Uses
 * either low pass or bilinear filtering. The memory organization for both
 * input and output are red, green, blue. All three of them are unsigned shorts 0-65535.
 * Allocates output and frees input
 * We assume unsigned short holds at least 16 bits.
 */
void scale_color(unsigned short *in, int ix, int iy, unsigned short **out,
	int ox, int oy)
{
	unsigned short *intermediate_buffer;
	int do_optimize;
	int ox0=ox;

	if (!ix||!iy){
		if (in) mem_free(in);
		*out=mem_calloc(ox*oy*sizeof(**out)*3);
		return;
	}
	if (display_optimize&&ox*3<=ix){
		do_optimize=1;
		ox0=ox;
		ox*=3;
	}else do_optimize=0;
	if (ix*oy<ox*iy){
		scale_color_vertical(in,ix,iy,&intermediate_buffer,oy);
		scale_color_horizontal(intermediate_buffer,ix,oy,out,ox);
	}else{
		scale_color_horizontal(in,ix,iy,&intermediate_buffer,ox);
		scale_color_vertical(intermediate_buffer,ox,iy,out,oy);
	}
	if (do_optimize) decimate_3(out, ox0, oy);
}

/* Fills a block with given color. length is number of pixels. pixel is a
 * tribyte. 24 bits per pixel.
 */
void mix_one_color_24(unsigned char *dest, int length,
		   unsigned char r, unsigned char g, unsigned char b)
{
	for (;length;length--){
		dest[0]=r;
		dest[1]=g;
		dest[2]=b;
		dest+=3;
	}
}

/* Fills a block with given color. length is number of pixels. pixel is a
 * tribyte. 48 bits per pixel.
 * We assume unsigned short holds at least 16 bits.
 */
void mix_one_color_48(unsigned short *dest, int length,
		   unsigned short r, unsigned short g, unsigned short b)
{
	for (;length;length--){
		dest[0]=r;
		dest[1]=g;
		dest[2]=b;
		dest+=3;
	}
}

/* Mixes ink and paper of a letter, using alpha as alpha mask.
 * Only mixing in photon space makes physical sense so that the input values
 * must always be equivalent to photons and not to electrons!
 * length is number of pixels. pixel is a tribyte
 * alpha is 8-bit, rgb are all 16-bit
 * We assume unsigned short holds at least 16 bits.
 */
#ifndef __XBOX__
inline
#endif
void mix_two_colors(unsigned short *dest, unsigned char *alpha, int length
	,unsigned short r0, unsigned short g0, unsigned short b0,
	unsigned short r255, unsigned short g255, unsigned short b255)
{
	unsigned mask,cmask;
	
	for (;length;length--){
		mask=*alpha++;
		if (((unsigned char)(mask+1))>=2){
			cmask=255-mask;
			dest[0]=(mask*r255+cmask*r0+127)/255;
			dest[1]=(mask*g255+cmask*g0+127)/255;
			dest[2]=(mask*b255+cmask*b0+127)/255;
		}else{
			if (mask){
				dest[0]=r255;
				dest[1]=g255;
				dest[2]=b255;
			}else{
				dest[0]=r0;
				dest[1]=g0;
				dest[2]=b0;
			}
		}
		dest+=3;
	}
}

/* We assume unsigned short holds at least 16 bits. */
void apply_gamma_exponent_and_undercolor_32_to_48_table(unsigned short *dest,
		unsigned char *src, int lenght, unsigned short *table
		,unsigned short rb, unsigned short gb, unsigned short bb)
{
	unsigned alpha, ri, gi, bi, calpha;

	for (;lenght;lenght--)
	{
		ri=table[src[0]];
		gi=table[src[1]+256];
		bi=table[src[2]+512];
		alpha=src[3];
		src+=4;
		if (((unsigned char)(alpha+1))>=2){
			calpha=255U-alpha;
			dest[0]=(ri*alpha+calpha*rb+127)/255;
			dest[1]=(gi*alpha+calpha*gb+127)/255;
			dest[2]=(bi*alpha+calpha*bb+127)/255;
		}else{
			if (alpha){
				dest[0]=ri;
				dest[1]=gi;
				dest[2]=bi;
			}else{
				dest[0]=rb;
				dest[1]=gb;
				dest[2]=bb;
			}
		}
		dest+=3;
	}
}

/* src is a block of four-bytes RGBA. All bytes are gamma corrected. length is
 * number of pixels. output is input powered to the given gamma, passed into
 * dest. src and dest may be identical and it will work. rb, gb, bb are 0-65535
 * in linear monitor output photon space
 */
/* We assume unsigned short holds at least 16 bits. */
void apply_gamma_exponent_and_undercolor_32_to_48(unsigned short *dest,
		unsigned char *src, int lenght, float red_gamma
		,float green_gamma, float blue_gamma, unsigned short rb, unsigned
		short gb, unsigned short bb)
{
	float r,g,b;
	unsigned alpha, calpha;
	unsigned ri,gi,bi;
	const float inv_255=1/255.0;

	for (;lenght;lenght--)
	{
		r=src[0];
		g=src[1];
		b=src[2];
		alpha=src[3];
		src+=4;
		r*=inv_255;
		g*=inv_255;
		b*=inv_255;
		r=pow(r,red_gamma);
		g=pow(g,green_gamma);
		b=pow(b,blue_gamma);
		ri=(r*65535)+0.5;
		gi=(g*65535)+0.5;
		bi=(b*65535)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
		/* To prevent segfaults in case of crappy floating arithmetics
		 */
		if (ri>=65536) ri=65535;
		if (gi>=65536) gi=65535;
		if (bi>=65536) bi=65535;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		if (((alpha+1U)&0xffU)>=2U){
			calpha=255U-alpha;
			*dest=(ri*alpha+calpha*rb+127U)/255U;
			dest[1]=(gi*alpha+calpha*gb+127U)/255U;
			dest[2]=(bi*alpha+calpha*bb+127U)/255U;
		}else{
			if (alpha){
				*dest=ri;
				dest[1]=gi;
				dest[2]=bi;
			}else{
				*dest=rb;
				dest[1]=gb;
				dest[2]=bb;
			}
		}
		dest+=3;
	}
}

/* src is a block of four-bytes RGBA. All bytes are gamma corrected. length is
 * number of pixels. output is input powered to the given gamma, passed into
 * dest. src and dest may be identical and it will work. rb, gb, bb are 0-65535
 * in linear monitor output photon space. alpha 255 means full image no background.
 */
/* We assume unsigned short holds at least 16 bits. */
void apply_gamma_exponent_and_undercolor_64_to_48(unsigned short *dest,
		unsigned short *src, int lenght, float red_gamma
		,float green_gamma, float blue_gamma, unsigned short rb, unsigned
		short gb, unsigned short bb)
{
	float r,g,b;
	unsigned alpha, calpha;
	unsigned short ri,gi,bi;
	const float inv_65535=1/((float)65535);

	for (;lenght;lenght--)
	{
		r=src[0];
		g=src[1];
		b=src[2];
		alpha=src[3];
		src+=4;
		r*=inv_65535;
		g*=inv_65535;
		b*=inv_65535;
		r=pow(r,red_gamma);
		g=pow(g,green_gamma);
		b=pow(b,blue_gamma);
		ri=r*65535+0.5;
		gi=g*65535+0.5;
		bi=b*65535+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
		/* To prevent segfaults in case of crappy floating arithmetics
		 */
		if (ri>=65536) ri=65535;
		if (gi>=65536) gi=65535;
		if (bi>=65536) bi=65535;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		if (((alpha+1U)&255U)>=2U){
			calpha=65535U-alpha;
			*dest=(ri*alpha+calpha*rb+32767U)/65535U;
			dest[1]=(gi*alpha+calpha*gb+32767U)/65535U;
			dest[2]=(bi*alpha+calpha*bb+32767U)/65535U;
		}else{
			if (alpha){
				*dest=ri;
				dest[1]=gi;
				dest[2]=bi;
			}else{
				*dest=rb;
				dest[1]=gb;
				dest[2]=bb;
			}
		}
		dest+=3;
	}
}

/* src is a block of four-bytes RGBA. All bytes are gamma corrected. length is
 * number of pixels. output is input powered to the given gamma, passed into
 * dest. src and dest may be identical and it will work. rb, gb, bb are 0-65535
 * in linear monitor output photon space. alpha 255 means full image no background.
 * We assume unsigned short holds at least 16 bits. */
void apply_gamma_exponent_and_undercolor_64_to_48_table(unsigned short *dest
		,unsigned short *src, int lenght, unsigned short *gamma_table
		,unsigned short rb, unsigned short gb, unsigned short bb)
{
	unsigned alpha, calpha;
	unsigned short ri,gi,bi;

	for (;lenght;lenght--)
	{
		ri=gamma_table[*src];
		gi=gamma_table[src[1]+65536];
		bi=gamma_table[src[2]+131072];
		alpha=src[3];
		src+=4;
		if (((alpha+1)&0xffff)>=2){
			calpha=65535-alpha;
			*dest=(ri*alpha+calpha*rb+32767)/65535;
			dest[1]=(gi*alpha+calpha*gb+32767)/65535;
			dest[2]=(bi*alpha+calpha*bb+32767)/65535;
		}else{
			if (alpha){
				*dest=ri;
				dest[1]=gi;
				dest[2]=bi;
			}else{
				*dest=rb;
				dest[1]=gb;
				dest[2]=bb;
			}
		}
		dest+=3;
	}
}

/* src is a block of three-bytes. All bytes are gamma corrected. length is
 * number of triplets. output is input powered to the given gamma, passed into
 * dest. src and dest may be identical and it will work.
 * We assume unsigned short holds at least 16 bits. */
void apply_gamma_exponent_48_to_48(unsigned short *dest,
		unsigned short *src, int lenght, float red_gamma
		,float green_gamma, float blue_gamma)
{
	float a, inv_65535=1/((float)65535);

	for (;lenght;lenght--,src+=3,dest+=3)
	{
		a=*src;
		a*=inv_65535;
		a=pow(a,red_gamma);
		*dest=(a*65535)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
		if (*dest>=0x10000) *dest=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		a=src[1];
		a*=inv_65535;
		a=pow(a,green_gamma);
		dest[1]=(a*65535)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
		if (dest[1]>=0x10000) dest[1]=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		a=src[2];
		a*=inv_65535;
		a=pow(a,blue_gamma);
		dest[2]=(a*65535)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
		if (dest[2]>=0x10000) dest[2]=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
	}
}

/* src is a block of three-bytes. All bytes are gamma corrected. length is
 * number of triples. output is input powered to the given gamma, passed into
 * dest. src and dest may be identical and it will work.
 * We assume unsigned short holds at least 16 bits. */
void apply_gamma_exponent_48_to_48_table(unsigned short *dest,
		unsigned short *src, int lenght, unsigned short *table)
{
	for (;lenght;lenght--,src+=3,dest+=3)
	{
		*dest=table[*src];
		dest[1]=table[src[1]+65536];
		dest[2]=table[src[2]+131072];
	}
}

/* src is a block of three-bytes. All bytes are gamma corrected. length is
 * number of triples. output is input powered to the given gamma, passed into
 * dest. src and dest may be identical and it will work.
 * We assume unsigned short holds at least 16 bits. */
void apply_gamma_exponent_24_to_48(unsigned short *dest, unsigned char *src, int
			  lenght, float red_gamma, float green_gamma, float
			  blue_gamma)
{
	float a;
	float inv_255=1/((float)255);
	
	for (;lenght;lenght--,src+=3,dest+=3)
	{
		a=*src;
		a*=inv_255;
		a=pow(a,red_gamma);
		*dest=(a*65535)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
		if (*dest>=0x10000) *dest=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		a=src[1];
		a*=inv_255;
		a=pow(a,green_gamma);
		dest[1]=(a*65535)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
		if (dest[1]>=0x10000) dest[1]=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		a=src[2];
		a*=inv_255;
		a=pow(a,blue_gamma);
		dest[2]=(a*65535)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
		if (dest[2]>=0x10000) dest[2]=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
	}
}

/* Allocates new gamma_table and fills it with mapping 8 bits ->
 * power to user_gamma/cimg->*_gamma -> 16 bits 
 * We assume unsigned short holds at least 16 bits. */
void make_gamma_table(struct cached_image *cimg)
{
	double rg=user_gamma/cimg->red_gamma;
	double gg=user_gamma/cimg->green_gamma;
	double bg=user_gamma/cimg->blue_gamma;
	double inv;
	int a;
	unsigned short *ptr_16;

	if (cimg->buffer_bytes_per_pixel<=4){
		/* 8-bit */
		inv=1/((double)255);
		ptr_16=mem_alloc(768*sizeof(*(cimg->gamma_table)));
		cimg->gamma_table=ptr_16;
		for (a=0;a<256;a++,ptr_16++){
			*ptr_16=65535*pow(((double)a)*inv,rg)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
		/* To test against crappy arithmetics */
			if (*ptr_16>=0x10000) *ptr_16=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		}
		for (a=0;a<256;a++,ptr_16++){
			*ptr_16=65535*pow(((double)a)*inv,gg)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
			if (*ptr_16>=0x10000) *ptr_16=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		}
		for (a=0;a<256;a++,ptr_16++){
			*ptr_16=65535*pow(((double)a)*inv,bg)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
			if (*ptr_16>=0x10000) *ptr_16=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		}
	}else{
		/* 16-bit */
		inv=1/((double)65535);
		ptr_16=mem_alloc(196608*sizeof(*(cimg->gamma_table)));
		cimg->gamma_table=ptr_16;
		for (a=0;a<0x10000;a++,ptr_16++){
			*ptr_16=65535*pow(((double)a)*inv,rg)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
			if (*ptr_16>=0x10000) *ptr_16=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		}
		for (a=0;a<0x10000;a++,ptr_16++){
			*ptr_16=65535*pow(((double)a)*inv,gg)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
			if (*ptr_16>=0x10000) *ptr_16=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		}
		for (a=0;a<0x10000;a++,ptr_16++){
			*ptr_16=65535*pow(((double)a)*inv,bg)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
			if (*ptr_16>=0x10000) *ptr_16=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
		}
	}
}

/* We assume unsigned short holds at least 16 bits. */
void apply_gamma_exponent_24_to_48_table(unsigned short *dest, unsigned char *src, int
			  lenght, unsigned short *table)
{
	for (;lenght;lenght--,src+=3,dest+=3)
	{
		dest[0]=table[src[0]];
		dest[1]=table[src[1]+256];
		dest[2]=table[src[2]+512];
	}
}

/* Input is 0-255 (8-bit). Output is 0-255 (8-bit)*/
unsigned char apply_gamma_single_8_to_8(unsigned char input, float gamma)
{
	return 255*pow(((float) input)/255,gamma)+0.5;
}

/* Input is 0-255 (8-bit). Output is 0-65535 (16-bit)*/
/* We assume unsigned short holds at least 16 bits. */
unsigned short apply_gamma_single_8_to_16(unsigned char input, float gamma)
{
	float a=input;
	unsigned short retval;

	a/=255;
	a=pow(a,gamma);
	a*=65535;
	retval = a+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
			if (retval>=0x10000) retval=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
	return retval;
}

/* Input is 0-65535 (16-bit). Output is 0-255 (8-bit)*/
/* We assume unsigned short holds at least 16 bits. */
unsigned char apply_gamma_single_16_to_8(unsigned short input, float gamma)
{
	return pow(((float)input)/65535,gamma)*255+0.5;
}

/* Input is 0-65535 (16-bit). Output is 0-255 (8-bit)*/
unsigned short apply_gamma_single_16_to_16(unsigned short input, float gamma)
{
	unsigned short retval;
	
	retval = 65535*pow(((float)input)/65535,gamma)+0.5;
#if SIZEOF_UNSIGNED_SHORT > 2
			if (retval>=0x10000) retval=0xffff;
#endif /* #if SIZEOF_UNSIGNED_SHORT > 2 */
	return retval;
}


void update_aspect(void)
{
        bfu_aspect=options_get_double("video_aspect");
        aspect=options_get_bool("video_aspect_on")
                ? (aspect_native*bfu_aspect+0.5)
                : 65536UL;
}

void init_dip()
{
	update_aspect();
        init_fonts();
}

void shutdown_dip()
{
        finalize_fonts();
}


long gamma_cache_color;
int gamma_cache_rgb = -2;

/* IEC 61966-2-1 
 * Input gamma: sRGB space (directly from HTML, i. e. unrounded)
 * Output: color index for graphics driver that is closest to the
 * given sRGB value.
 * We assume unsigned short holds at least 16 bits. */
long real_dip_get_color_sRGB(int rgb)
{
	unsigned short r,g,b;
	int new_rgb;

	round_color_sRGB_to_48(&r,&g,&b,rgb);
	r=apply_gamma_single_16_to_8(r,1/display_red_gamma);
	g=apply_gamma_single_16_to_8(g,1/display_green_gamma);
	b=apply_gamma_single_16_to_8(b,1/display_blue_gamma);
	new_rgb=b|(g<<8)|(r<<16);	
	gamma_cache_rgb = rgb;
	/* The get_color takes values with gamma of display_*_gamma */
	return gamma_cache_color = drv->get_color(new_rgb);
}

/* ATTENTION!!! allocates using malloc. Due to braindead Xlibe, which
 * frees it using free and thus it is not possible to use mem_alloc. */
void get_links_icon(unsigned char **data, int *width, int* height, int depth)
{
	struct bitmap b;
	unsigned short *tmp1;
	double g=user_gamma/sRGB_gamma;

	b.x=48;
	b.y=48;
	*width=b.x;
	*height=b.y;
	b.skip=b.x*(depth&7);
	b.data=*data=malloc(b.skip*b.y);
	tmp1=mem_alloc(6*b.y*b.x);
        apply_gamma_exponent_24_to_48(tmp1,links_icon,b.x*b.y,g,g,g);
	dither(tmp1, &b);
	mem_free(tmp1);
}

#endif /* G */
