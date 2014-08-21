/*
  libdcr version 0.1.8.89 11/Jan/2009

  libdcr : copyright (C) 2007-2009, Davide Pizzolato

  based on dcraw.c -- Dave Coffin's raw photo decoder
  Copyright 1997-2009 by Dave Coffin, dcoffin a cybercom o net

  Covered code is provided under this license on an "as is" basis, without warranty
  of any kind, either expressed or implied, including, without limitation, warranties
  that the covered code is free of defects, merchantable, fit for a particular purpose
  or non-infringing. The entire risk as to the quality and performance of the covered
  code is with you. Should any covered code prove defective in any respect, you (not
  the initial developer or any other contributor) assume the cost of any necessary
  servicing, repair or correction. This disclaimer of warranty constitutes an essential
  part of this license. No use of any covered code is authorized hereunder except under
  this disclaimer.

  No license is required to download and use libdcr.  However,
  to lawfully redistribute libdcr, you must either (a) offer, at
  no extra charge, full source code for all executable files
  containing RESTRICTED functions, (b) distribute this code under
  the GPL Version 2 or later, (c) remove all RESTRICTED functions,
  re-implement them, or copy them from an earlier, unrestricted
  revision of dcraw.c, or (d) purchase a license from the author
  of dcraw.c.

  --------------------------------------------------------------------------------

  dcraw.c home page: http://cybercom.net/~dcoffin/dcraw/
  libdcr  home page: http://www.xdp.it/libdcr/

 */

#define _GNU_SOURCE
#define _USE_MATH_DEFINES
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#include "libdcr.h"

// XYZ from RGB
const double xyz_rgb[3][3] = {
	{ 0.412453, 0.357580, 0.180423 },
	{ 0.212671, 0.715160, 0.072169 },
	{ 0.019334, 0.119193, 0.950227 } };

const float d65_white[3] = { 0.950456f, 1.0f, 1.088754f };

static int   dcr_sfile_read(dcr_stream_obj *obj, void *buf, int size, int cnt);
static int   dcr_sfile_write(dcr_stream_obj *obj, void *buf, int size, int cnt);
static long  dcr_sfile_seek(dcr_stream_obj *obj, long offset, int origin);
static int   dcr_sfile_close(dcr_stream_obj *obj);
static char* dcr_sfile_gets(dcr_stream_obj *obj, char *string, int n);
static int   dcr_sfile_eof(dcr_stream_obj *obj);
static long  dcr_sfile_tell(dcr_stream_obj *obj);
static int   dcr_sfile_getc(dcr_stream_obj *obj);
static int   dcr_sfile_scanf(dcr_stream_obj *obj,const char *format, void* output);

static dcr_stream_ops dcr_stream_fileops = {
	dcr_sfile_read,
	dcr_sfile_write,
	dcr_sfile_seek,
	dcr_sfile_close,
	dcr_sfile_gets,
	dcr_sfile_eof,
	dcr_sfile_tell,
	dcr_sfile_getc,
	dcr_sfile_scanf
};

	/*
	NO_JPEG disables decoding of compressed Kodak DC120 files.
	NO_LCMS disables the "-p" option.
	*/
#ifndef NO_JPEG
  #ifdef WIN32
    #include "../jpeg/jpeglib.h"
  #else
    #include <jpeglib.h>
  #endif
#endif

#ifndef NO_LCMS
  #ifdef WIN32
    #include "../lcms/lcms.h"
  #else
    #include <lcms.h>
  #endif
#endif

#ifdef LOCALEDIR
#include <libintl.h>
#define _(String) gettext(String)
#else
#define _(String) (String)
#endif

#ifdef __CYGWIN__
#include <io.h>
#endif
#ifdef WIN32
#include <sys/utime.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#define snprintf _snprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
	typedef __int64 INT64;
	typedef unsigned __int64 UINT64;
#else
#include <unistd.h>
#include <utime.h>
#include <netinet/in.h>
	typedef long long INT64;
	typedef unsigned long long UINT64;
	#define __int64		long long
#endif

#ifdef LJPEG_DECODE
#error Please compile dcraw.c by itself.
#error Do not link it with ljpeg_decode.
#endif


/*
   In order to inline this calculation, I make the risky
   assumption that all filter patterns can be described
   by a repeating pattern of eight rows and two columns

   Do not use the FC or BAYER macros with the Leaf CatchLight,
   because its pattern is 16x16, not 2x8.

   Return values are either 0/1/2/3 = G/M/C/Y or 0/1/2/3 = R/G1/B/G2

	PowerShot 600	PowerShot A50	PowerShot Pro70	Pro90 & G1
	0xe1e4e1e4:	0x1b4e4b1e:	0x1e4b4e1b:	0xb4b4b4b4:

	  0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5
	0 G M G M G M	0 C Y C Y C Y	0 Y C Y C Y C	0 G M G M G M
	1 C Y C Y C Y	1 M G M G M G	1 M G M G M G	1 Y C Y C Y C
	2 M G M G M G	2 Y C Y C Y C	2 C Y C Y C Y
	3 C Y C Y C Y	3 G M G M G M	3 G M G M G M
			4 C Y C Y C Y	4 Y C Y C Y C
	PowerShot A5	5 G M G M G M	5 G M G M G M
	0x1e4e1e4e:	6 Y C Y C Y C	6 C Y C Y C Y
			7 M G M G M G	7 M G M G M G
	  0 1 2 3 4 5
	0 C Y C Y C Y
	1 G M G M G M
	2 C Y C Y C Y
	3 M G M G M G

   All RGB cameras use one of these Bayer grids:

	0x16161616:	0x61616161:	0x49494949:	0x94949494:

	  0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5
	0 B G B G B G	0 G R G R G R	0 G B G B G B	0 R G R G R G
	1 G R G R G R	1 B G B G B G	1 R G R G R G	1 G B G B G B
	2 B G B G B G	2 G R G R G R	2 G B G B G B	2 R G R G R G
	3 G R G R G R	3 B G B G B G	3 R G R G R G	3 G B G B G B
 */

#define FC(row,col) \
	(p->filters >> ((((row) << 1 & 14) + ((col) & 1)) << 1) & 3)

#define BAYER(row,col) \
	p->image[((row) >> p->shrink)*p->iwidth + ((col) >> p->shrink)][FC(row,col)]

#define BAYER2(row,col) \
	p->image[((row) >> p->shrink)*p->iwidth + ((col) >> p->shrink)][dcr_fc(p,row,col)]

int DCR_CLASS dcr_fc (DCRAW* p, int row, int col)
{
	static const char filter[16][16] =
	{ { 2,1,1,3,2,3,2,0,3,2,3,0,1,2,1,0 },
	{ 0,3,0,2,0,1,3,1,0,1,1,2,0,3,3,2 },
	{ 2,3,3,2,3,1,1,3,3,1,2,1,2,0,0,3 },
	{ 0,1,0,1,0,2,0,2,2,0,3,0,1,3,2,1 },
	{ 3,1,1,2,0,1,0,2,1,3,1,3,0,1,3,0 },
	{ 2,0,0,3,3,2,3,1,2,0,2,0,3,2,2,1 },
	{ 2,3,3,1,2,1,2,1,2,1,1,2,3,0,0,1 },
	{ 1,0,0,2,3,0,0,3,0,3,0,3,2,1,2,3 },
	{ 2,3,3,1,1,2,1,0,3,2,3,0,2,3,1,3 },
	{ 1,0,2,0,3,0,3,2,0,1,1,2,0,1,0,2 },
	{ 0,1,1,3,3,2,2,1,1,3,3,0,2,1,3,2 },
	{ 2,3,2,0,0,1,3,0,2,0,1,2,3,0,1,0 },
	{ 1,3,1,2,3,2,3,2,0,2,0,1,1,0,3,0 },
	{ 0,2,0,3,1,0,0,1,1,3,3,2,3,2,2,1 },
	{ 2,1,3,2,3,1,2,1,0,3,0,2,0,2,0,2 },
	{ 0,3,1,0,0,2,0,3,2,1,3,1,1,3,1,3 } };

	if (p->filters != 1) return FC(row,col);
	return filter[(row+p->top_margin) & 15][(col+p->left_margin) & 15];
}

#ifndef __GLIBC__
char *dcr_memmem (char *haystack, size_t haystacklen,
	char *needle, size_t needlelen)
{
	char *c;
	for (c = haystack; c <= haystack + haystacklen - needlelen; c++)
		if (!memcmp (c, needle, needlelen))
			return c;
		return 0;
}
#define memmem dcr_memmem
#endif

void DCR_CLASS dcr_merror (DCRAW* p, void *ptr, char *where)
{
	if (ptr) return;
	if (p->sz_error){
		sprintf (p->sz_error,_("%s: Out of memory in %s\n"), p->ifname, where);
	} else {
		fprintf (stderr,_("%s: Out of memory in %s\n"), p->ifname, where);
	}
	longjmp (p->failure, 1);
}

void DCR_CLASS dcr_derror(DCRAW* p)
{
	if (!p->data_error) {
		fprintf (stderr, "%s: ", p->ifname);
		if (dcr_feof(p->obj_))
			fprintf (stderr,_("Unexpected end of file\n"));
		else
			fprintf (stderr,_("Corrupt data near 0x%llx\n"), (INT64) dcr_ftell(p->obj_));
	}
	p->data_error = 1;
}

ushort DCR_CLASS dcr_sget2 (DCRAW* p,uchar *s)
{
	if (p->order == 0x4949)		/* "II" means little-endian */
		return s[0] | s[1] << 8;
	else				/* "MM" means big-endian */
		return s[0] << 8 | s[1];
}

ushort DCR_CLASS dcr_get2(DCRAW* p)
{
	uchar str[2] = { 0xff,0xff };
	dcr_fread(p->obj_, str, 1, 2);
	return dcr_sget2(p, str);
}

unsigned DCR_CLASS dcr_sget4 (DCRAW* p,uchar *s)
{
	if (p->order == 0x4949)
		return s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24;
	else
		return s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3];
}
#define dcr_sget4(p, s) dcr_sget4(p, (uchar *)s)

unsigned DCR_CLASS dcr_get4(DCRAW* p)
{
	uchar str[4] = { 0xff,0xff,0xff,0xff };
	dcr_fread(p->obj_, str, 1, 4);
	return dcr_sget4(p, str);
}

unsigned DCR_CLASS dcr_getint (DCRAW* p,int type)
{
	return type == 3 ? dcr_get2(p) : dcr_get4(p);
}

float DCR_CLASS dcr_int_to_float (int i)
{
	union { int i; float f; } u;
	u.i = i;
	return u.f;
}

double DCR_CLASS dcr_getreal (DCRAW* p,int type)
{
	union { char c[8]; double d; } u;
	int i, rev;

	switch (type) {
	case 3: return (unsigned short) dcr_get2(p);
	case 4: return (unsigned int) dcr_get4(p);
	case 5:  u.d = (unsigned int) dcr_get4(p);
		return u.d / (unsigned int) dcr_get4(p);
	case 8: return (signed short) dcr_get2(p);
	case 9: return (signed int) dcr_get4(p);
	case 10: u.d = (signed int) dcr_get4(p);
		return u.d / (signed int) dcr_get4(p);
	case 11: return dcr_int_to_float (dcr_get4(p));
	case 12:
		rev = 7 * ((p->order == 0x4949) == (ntohs(0x1234) == 0x1234));
		for (i=0; i < 8; i++)
			u.c[i ^ rev] = dcr_fgetc(p->obj_);
		return u.d;
	default: return dcr_fgetc(p->obj_);
	}
}

void DCR_CLASS dcr_read_shorts (DCRAW* p,ushort *pixel, int count)
{
	if ((int)dcr_fread(p->obj_, pixel, 2, count) < count) dcr_derror(p);
	if ((p->order == 0x4949) == (ntohs(0x1234) == 0x1234))
		_swab ((char*)pixel, (char*)pixel, count*2);
}

void DCR_CLASS dcr_canon_black (DCRAW* p, double dark[2])
{
  int c, diff, row, col;

  if (p->raw_width < p->width+4) return;
  FORC(2) dark[c] /= (p->raw_width-p->width-2) * p->height >> 1;
  if ((diff = (int)(dark[0] - dark[1])))
    for (row=0; row < p->height; row++)
      for (col=1; col < p->width; col+=2)
	BAYER(row,col) += diff;
  dark[1] += diff;
  p->black = (unsigned int)((dark[0] + dark[1] + 1) / 2);
}

void DCR_CLASS dcr_canon_600_fixed_wb (DCRAW* p,int temp)
{
	static const short mul[4][5] = {
		{  667, 358,397,565,452 },
		{  731, 390,367,499,517 },
		{ 1119, 396,348,448,537 },
		{ 1399, 485,431,508,688 } };
	int lo, hi, i;
	float frac=0;

	for (lo=4; --lo; )
		if (*mul[lo] <= temp) break;
	for (hi=0; hi < 3; hi++)
		if (*mul[hi] >= temp) break;
	if (lo != hi)
		frac = (float) (temp - *mul[lo]) / (*mul[hi] - *mul[lo]);
	for (i=1; i < 5; i++)
		p->pre_mul[i-1] = 1 / (frac * mul[hi][i] + (1-frac) * mul[lo][i]);
}

/* Return values:  0 = p->white  1 = near p->white  2 = not p->white */
int DCR_CLASS dcr_canon_600_color (DCRAW* p,int ratio[2], int mar)
{
	int clipped=0, target, miss;

	if (p->flash_used) {
		if (ratio[1] < -104)
		{ ratio[1] = -104; clipped = 1; }
		if (ratio[1] >   12)
		{ ratio[1] =   12; clipped = 1; }
	} else {
		if (ratio[1] < -264 || ratio[1] > 461) return 2;
		if (ratio[1] < -50)
		{ ratio[1] = -50; clipped = 1; }
		if (ratio[1] > 307)
		{ ratio[1] = 307; clipped = 1; }
	}
	target = p->flash_used || ratio[1] < 197
		? -38 - (398 * ratio[1] >> 10)
		: -123 + (48 * ratio[1] >> 10);
	if (target - mar <= ratio[0] &&
		target + 20  >= ratio[0] && !clipped) return 0;
	miss = target - ratio[0];
	if (abs(miss) >= mar*4) return 2;
	if (miss < -20) miss = -20;
	if (miss > mar) miss = mar;
	ratio[0] = target - miss;
	return 1;
}

void DCR_CLASS dcr_canon_600_auto_wb(DCRAW* p)
{
	int mar, row, col, i, j, st, count[] = { 0,0 };
	int test[8], total[2][8], ratio[2][2], stat[2];

	memset (&total, 0, sizeof total);
	i = (int)(p->canon_ev + 0.5);
	if      (i < 10) mar = 150;
	else if (i > 12) mar = 20;
	else mar = 280 - 20 * i;
	if (p->flash_used) mar = 80;
	for (row=14; row < p->height-14; row+=4)
		for (col=10; col < p->width; col+=2) {
			for (i=0; i < 8; i++)
				test[(i & 4) + FC(row+(i >> 1),col+(i & 1))] =
				BAYER(row+(i >> 1),col+(i & 1));
			for (i=0; i < 8; i++)
				if (test[i] < 150 || test[i] > 1500) goto next;
			for (i=0; i < 4; i++)
				if (abs(test[i] - test[i+4]) > 50) goto next;
			for (i=0; i < 2; i++) {
				for (j=0; j < 4; j+=2)
					ratio[i][j >> 1] = ((test[i*4+j+1]-test[i*4+j]) << 10) / test[i*4+j];
				stat[i] = dcr_canon_600_color (p,ratio[i], mar);
			}
			if ((st = stat[0] | stat[1]) > 1) goto next;
			for (i=0; i < 2; i++)
				if (stat[i])
					for (j=0; j < 2; j++)
						test[i*4+j*2+1] = test[i*4+j*2] * (0x400 + ratio[i][j]) >> 10;
			for (i=0; i < 8; i++)
				total[st][i] += test[i];
			count[st]++;
next: ;
		}
	if (count[0] | count[1]) {
		st = count[0]*200 < count[1];
		for (i=0; i < 4; i++)
			p->pre_mul[i] = 1.0f / (total[st][i] + total[st][i+4]);
	}
}

void DCR_CLASS dcr_canon_600_coeff(DCRAW* p)
{
	static const short table[6][12] = {
		{ -190,702,-1878,2390,   1861,-1349,905,-393, -432,944,2617,-2105  },
		{ -1203,1715,-1136,1648, 1388,-876,267,245,  -1641,2153,3921,-3409 },
		{ -615,1127,-1563,2075,  1437,-925,509,3,     -756,1268,2519,-2007 },
		{ -190,702,-1886,2398,   2153,-1641,763,-251, -452,964,3040,-2528  },
		{ -190,702,-1878,2390,   1861,-1349,905,-393, -432,944,2617,-2105  },
		{ -807,1319,-1785,2297,  1388,-876,769,-257,  -230,742,2067,-1555  } };
	int t=0, i, c;
	float mc, yc;

	mc = p->pre_mul[1] / p->pre_mul[2];
	yc = p->pre_mul[3] / p->pre_mul[2];
	if (mc > 1 && mc <= 1.28 && yc < 0.8789) t=1;
	if (mc > 1.28 && mc <= 2) {
		if  (yc < 0.8789) t=3;
		else if (yc <= 2) t=4;
	}
	if (p->flash_used) t=5;
	for (p->raw_color = i=0; i < 3; i++)
		FORCC(p) p->rgb_cam[i][c] = table[t][i*4 + c] / 1024.0f;
}

void DCR_CLASS dcr_canon_600_load_raw(DCRAW* p)
{
	uchar  data[1120], *dp;
	ushort pixel[896], *pix;
	int irow, row, col, val;
	static const short mul[4][2] =
	{ { 1141,1145 }, { 1128,1109 }, { 1178,1149 }, { 1128,1109 } };

	for (irow=row=0; irow < p->height; irow++) {
		if ((long)dcr_fread(p->obj_, data, 1, p->raw_width*5/4) < (long)(p->raw_width*5/4)) dcr_derror(p);
		for (dp=data, pix=pixel; dp < data+1120; dp+=10, pix+=8) {
			pix[0] = (dp[0] << 2) + (dp[1] >> 6    );
			pix[1] = (dp[2] << 2) + (dp[1] >> 4 & 3);
			pix[2] = (dp[3] << 2) + (dp[1] >> 2 & 3);
			pix[3] = (dp[4] << 2) + (dp[1]      & 3);
			pix[4] = (dp[5] << 2) + (dp[9]      & 3);
			pix[5] = (dp[6] << 2) + (dp[9] >> 2 & 3);
			pix[6] = (dp[7] << 2) + (dp[9] >> 4 & 3);
			pix[7] = (dp[8] << 2) + (dp[9] >> 6    );
		}
		for (col=0; col < p->width; col++)
			BAYER(row,col) = pixel[col];
		for (col=p->width; col < p->raw_width; col++)
			p->black += pixel[col];
		if ((row+=2) > p->height) row = 1;
	}
	if (p->raw_width > p->width)
		p->black = p->black / ((p->raw_width - p->width) * p->height) - 4;
	for (row=0; row < p->height; row++)
		for (col=0; col < p->width; col++) {
			if ((val = BAYER(row,col) - p->black) < 0) val = 0;
			val = val * mul[row & 3][col & 1] >> 9;
			BAYER(row,col) = val;
		}
	dcr_canon_600_fixed_wb(p,1311);
	dcr_canon_600_auto_wb(p);
	dcr_canon_600_coeff(p);
	p->maximum = (0x3ff - p->black) * 1109 >> 9;
	p->black = 0;
}

void DCR_CLASS dcr_remove_zeroes(DCRAW* p)
{
	unsigned row, col, tot, n, r, c;

	for (row=0; row < p->height; row++)
		for (col=0; col < p->width; col++)
			if (BAYER(row,col) == 0) {
				tot = n = 0;
				for (r = row-2; r <= row+2; r++)
					for (c = col-2; c <= col+2; c++)
						if (r < p->height && c < p->width &&
							FC(r,c) == FC(row,col) && BAYER(r,c))
							tot += (n++,BAYER(r,c));
						if (n) BAYER(row,col) = tot/n;
			}
}

int DCR_CLASS dcr_canon_s2is(DCRAW* p)
{
	unsigned row;

	for (row=0; row < 100; row++) {
		dcr_fseek(p->obj_, row*3340 + 3284, SEEK_SET);
		if (dcr_fgetc(p->obj_) > 15) return 1;
	}
	return 0;
}

void DCR_CLASS dcr_canon_a5_load_raw(DCRAW* p)
{
	ushort data[2565], *dp, pixel;
	int vbits=0, buf=0, row, col, bc=0;

	p->order = 0x4949;
	for (row=-p->top_margin; row < p->raw_height-p->top_margin; row++) {
		dcr_read_shorts (p,dp=data, p->raw_width * 10 / 16);
		for (col=-p->left_margin; col < p->raw_width-p->left_margin; col++) {
			if ((vbits -= 10) < 0)
				buf = (vbits += 16, (buf << 16) + *dp++);
			pixel = buf >> vbits & 0x3ff;
			if ((unsigned) row < p->height && (unsigned) col < p->width)
				BAYER(row,col) = pixel;
			else if (col > 1-p->left_margin && col != p->width)
				p->black += (bc++,pixel);
		}
	}
	if (bc) p->black /= bc;
	p->maximum = 0x3ff;
	if (p->raw_width > 1600) dcr_remove_zeroes(p);
}

/*
dcr_getbits(p, -1) initializes the buffer
dcr_getbits(p, n) where 0 <= n <= 25 returns an n-bit integer
*/
unsigned DCR_CLASS dcr_getbits (DCRAW* p, int nbits)
{
	unsigned c;

	if (nbits == -1)
		return p->getbits_bitbuf = p->getbits_vbits = p->getbits_reset = 0;
	if (nbits == 0 || p->getbits_reset) return 0;
	while (p->getbits_vbits < nbits) {
		if ((c = dcr_fgetc(p->obj_)) == EOF) dcr_derror(p);
		if ((p->getbits_reset = p->zero_after_ff && c == 0xff && dcr_fgetc(p->obj_))) return 0;
		p->getbits_bitbuf = (p->getbits_bitbuf << 8) + (uchar) c;
		p->getbits_vbits += 8;
	}
	p->getbits_vbits -= nbits;
	return p->getbits_bitbuf << (32-nbits-p->getbits_vbits) >> (32-nbits);
}

void DCR_CLASS dcr_init_decoder(DCRAW* p)
{
	memset (p->first_decode, 0, sizeof p->first_decode);
	p->free_decode = p->first_decode;
}

/*
   Construct a decode tree according the specification in *source.
   The first 16 bytes specify how many codes should be 1-bit, 2-bit
   3-bit, etc.  Bytes after that are the leaf values.

   For example, if the source is

    { 0,1,4,2,3,1,2,0,0,0,0,0,0,0,0,0,
      0x04,0x03,0x05,0x06,0x02,0x07,0x01,0x08,0x09,0x00,0x0a,0x0b,0xff  },

   then the code is

	00		0x04
	010		0x03
	011		0x05
	100		0x06
	101		0x02
	1100		0x07
	1101		0x01
	11100		0x08
	11101		0x09
	11110		0x00
	111110		0x0a
	1111110		0x0b
	1111111		0xff
 */
uchar * DCR_CLASS dcr_make_decoder (DCRAW* p, const uchar *source, int level)
{
	struct dcr_decode *cur;
	int i, next;

	if (level==0) p->make_decoder_leaf=0;
	cur = p->free_decode++;
	if (p->free_decode > p->first_decode+2048) {
		fprintf (stderr,_("%s: decoder table overflow\n"), p->ifname);
		longjmp (p->failure, 2);
	}
	for (i=next=0; i <= p->make_decoder_leaf && next < 16; )
		i += source[next++];
	if (i > p->make_decoder_leaf) {
		if (level < next) {
			cur->branch[0] = p->free_decode;
			dcr_make_decoder (p, source, level+1);
			cur->branch[1] = p->free_decode;
			dcr_make_decoder (p, source, level+1);
		} else
			cur->leaf = source[16 + p->make_decoder_leaf++];
	}
	return (uchar *) source + 16 + p->make_decoder_leaf;
}

void DCR_CLASS dcr_crw_init_tables (DCRAW* p, unsigned table)
{
	static const uchar first_tree[3][29] = {
		{ 0,1,4,2,3,1,2,0,0,0,0,0,0,0,0,0,
			0x04,0x03,0x05,0x06,0x02,0x07,0x01,0x08,0x09,0x00,0x0a,0x0b,0xff  },
		{ 0,2,2,3,1,1,1,1,2,0,0,0,0,0,0,0,
		0x03,0x02,0x04,0x01,0x05,0x00,0x06,0x07,0x09,0x08,0x0a,0x0b,0xff  },
		{ 0,0,6,3,1,1,2,0,0,0,0,0,0,0,0,0,
		0x06,0x05,0x07,0x04,0x08,0x03,0x09,0x02,0x00,0x0a,0x01,0x0b,0xff  },
	};
	static const uchar second_tree[3][180] = {
		{ 0,2,2,2,1,4,2,1,2,5,1,1,0,0,0,139,
		0x03,0x04,0x02,0x05,0x01,0x06,0x07,0x08,
		0x12,0x13,0x11,0x14,0x09,0x15,0x22,0x00,0x21,0x16,0x0a,0xf0,
		0x23,0x17,0x24,0x31,0x32,0x18,0x19,0x33,0x25,0x41,0x34,0x42,
		0x35,0x51,0x36,0x37,0x38,0x29,0x79,0x26,0x1a,0x39,0x56,0x57,
		0x28,0x27,0x52,0x55,0x58,0x43,0x76,0x59,0x77,0x54,0x61,0xf9,
		0x71,0x78,0x75,0x96,0x97,0x49,0xb7,0x53,0xd7,0x74,0xb6,0x98,
		0x47,0x48,0x95,0x69,0x99,0x91,0xfa,0xb8,0x68,0xb5,0xb9,0xd6,
		0xf7,0xd8,0x67,0x46,0x45,0x94,0x89,0xf8,0x81,0xd5,0xf6,0xb4,
		0x88,0xb1,0x2a,0x44,0x72,0xd9,0x87,0x66,0xd4,0xf5,0x3a,0xa7,
		0x73,0xa9,0xa8,0x86,0x62,0xc7,0x65,0xc8,0xc9,0xa1,0xf4,0xd1,
		0xe9,0x5a,0x92,0x85,0xa6,0xe7,0x93,0xe8,0xc1,0xc6,0x7a,0x64,
		0xe1,0x4a,0x6a,0xe6,0xb3,0xf1,0xd3,0xa5,0x8a,0xb2,0x9a,0xba,
		0x84,0xa4,0x63,0xe5,0xc5,0xf3,0xd2,0xc4,0x82,0xaa,0xda,0xe4,
		0xf2,0xca,0x83,0xa3,0xa2,0xc3,0xea,0xc2,0xe2,0xe3,0xff,0xff  },
		{ 0,2,2,1,4,1,4,1,3,3,1,0,0,0,0,140,
		0x02,0x03,0x01,0x04,0x05,0x12,0x11,0x06,
		0x13,0x07,0x08,0x14,0x22,0x09,0x21,0x00,0x23,0x15,0x31,0x32,
		0x0a,0x16,0xf0,0x24,0x33,0x41,0x42,0x19,0x17,0x25,0x18,0x51,
		0x34,0x43,0x52,0x29,0x35,0x61,0x39,0x71,0x62,0x36,0x53,0x26,
		0x38,0x1a,0x37,0x81,0x27,0x91,0x79,0x55,0x45,0x28,0x72,0x59,
		0xa1,0xb1,0x44,0x69,0x54,0x58,0xd1,0xfa,0x57,0xe1,0xf1,0xb9,
		0x49,0x47,0x63,0x6a,0xf9,0x56,0x46,0xa8,0x2a,0x4a,0x78,0x99,
		0x3a,0x75,0x74,0x86,0x65,0xc1,0x76,0xb6,0x96,0xd6,0x89,0x85,
		0xc9,0xf5,0x95,0xb4,0xc7,0xf7,0x8a,0x97,0xb8,0x73,0xb7,0xd8,
		0xd9,0x87,0xa7,0x7a,0x48,0x82,0x84,0xea,0xf4,0xa6,0xc5,0x5a,
		0x94,0xa4,0xc6,0x92,0xc3,0x68,0xb5,0xc8,0xe4,0xe5,0xe6,0xe9,
		0xa2,0xa3,0xe3,0xc2,0x66,0x67,0x93,0xaa,0xd4,0xd5,0xe7,0xf8,
		0x88,0x9a,0xd7,0x77,0xc4,0x64,0xe2,0x98,0xa5,0xca,0xda,0xe8,
		0xf3,0xf6,0xa9,0xb2,0xb3,0xf2,0xd2,0x83,0xba,0xd3,0xff,0xff  },
		{ 0,0,6,2,1,3,3,2,5,1,2,2,8,10,0,117,
		0x04,0x05,0x03,0x06,0x02,0x07,0x01,0x08,
		0x09,0x12,0x13,0x14,0x11,0x15,0x0a,0x16,0x17,0xf0,0x00,0x22,
		0x21,0x18,0x23,0x19,0x24,0x32,0x31,0x25,0x33,0x38,0x37,0x34,
		0x35,0x36,0x39,0x79,0x57,0x58,0x59,0x28,0x56,0x78,0x27,0x41,
		0x29,0x77,0x26,0x42,0x76,0x99,0x1a,0x55,0x98,0x97,0xf9,0x48,
		0x54,0x96,0x89,0x47,0xb7,0x49,0xfa,0x75,0x68,0xb6,0x67,0x69,
		0xb9,0xb8,0xd8,0x52,0xd7,0x88,0xb5,0x74,0x51,0x46,0xd9,0xf8,
		0x3a,0xd6,0x87,0x45,0x7a,0x95,0xd5,0xf6,0x86,0xb4,0xa9,0x94,
		0x53,0x2a,0xa8,0x43,0xf5,0xf7,0xd4,0x66,0xa7,0x5a,0x44,0x8a,
		0xc9,0xe8,0xc8,0xe7,0x9a,0x6a,0x73,0x4a,0x61,0xc7,0xf4,0xc6,
		0x65,0xe9,0x72,0xe6,0x71,0x91,0x93,0xa6,0xda,0x92,0x85,0x62,
		0xf3,0xc5,0xb2,0xa4,0x84,0xba,0x64,0xa5,0xb3,0xd2,0x81,0xe5,
		0xd3,0xaa,0xc4,0xca,0xf2,0xb1,0xe4,0xd1,0x83,0x63,0xea,0xc3,
		0xe2,0x82,0xf1,0xa3,0xc2,0xa1,0xc1,0xe3,0xa2,0xe1,0xff,0xff  }
	};
	if (table > 2) table = 2;
	dcr_init_decoder(p);
	dcr_make_decoder (p, first_tree[table], 0);
	p->second_decode = p->free_decode;
	dcr_make_decoder (p, second_tree[table], 0);
}

/*
Return 0 if the image starts with compressed data,
1 if it starts with uncompressed low-order bits.

  In Canon compressed data, 0xff is always followed by 0x00.
*/
int DCR_CLASS dcr_canon_has_lowbits(DCRAW* p)
{
	uchar test[0x4000];
	int ret=1, i;

	dcr_fseek(p->obj_, 0, SEEK_SET);
	dcr_fread(p->obj_, test, 1, sizeof test);
	for (i=540; i < sizeof test - 1; i++)
		if (test[i] == 0xff) {
			if (test[i+1]) return 1;
			ret=0;
		}
	return ret;
}

void DCR_CLASS dcr_canon_compressed_load_raw(DCRAW* p)
{
	ushort *pixel, *prow;
	int nblocks, lowbits, i, row, r, col, save, val;
	unsigned irow, icol;
	struct dcr_decode *decode, *dindex;
	int block, diffbuf[64], leaf, len, diff, carry=0, pnum=0, base[2];
	double dark[2] = { 0,0 };
	uchar c;

	dcr_crw_init_tables (p, p->tiff_compress);
	pixel = (ushort *) calloc (p->raw_width*8, sizeof *pixel);
	dcr_merror (p, pixel, "canon_compressed_load_raw()");
	lowbits = dcr_canon_has_lowbits(p);
	if (!lowbits) p->maximum = 0x3ff;
	dcr_fseek(p->obj_, 540 + lowbits*p->raw_height*p->raw_width/4, SEEK_SET);
	p->zero_after_ff = 1;
	dcr_getbits(p, -1);
	for (row=0; row < p->raw_height; row+=8) {
		nblocks = MIN (8, p->raw_height-row) * p->raw_width >> 6;
		for (block=0; block < nblocks; block++) {
			memset (diffbuf, 0, sizeof diffbuf);
			decode = p->first_decode;
			for (i=0; i < 64; i++ ) {
				for (dindex=decode; dindex->branch[0]; )
					dindex = dindex->branch[dcr_getbits(p, 1)];
				leaf = dindex->leaf;
				decode = p->second_decode;
				if (leaf == 0 && i) break;
				if (leaf == 0xff) continue;
				i  += leaf >> 4;
				len = leaf & 15;
				if (len == 0) continue;
				diff = dcr_getbits(p, len);
				if ((diff & (1 << (len-1))) == 0)
					diff -= (1 << len) - 1;
				if (i < 64) diffbuf[i] = diff;
			}
			diffbuf[0] += carry;
			carry = diffbuf[0];
			for (i=0; i < 64; i++ ) {
				if (pnum++ % p->raw_width == 0)
					base[0] = base[1] = 512;
				if ((pixel[(block << 6) + i] = base[i & 1] += diffbuf[i]) >> 10)
					dcr_derror(p);
			}
		}
		if (lowbits) {
			save = dcr_ftell(p->obj_);
			dcr_fseek(p->obj_, 26 + row*p->raw_width/4, SEEK_SET);
			for (prow=pixel, i=0; i < p->raw_width*2; i++) {
				c = dcr_fgetc(p->obj_);
				for (r=0; r < 8; r+=2, prow++) {
					val = (*prow << 2) + ((c >> r) & 3);
					if (p->raw_width == 2672 && val < 512) val += 2;
					*prow = val;
				}
			}
			dcr_fseek(p->obj_, save, SEEK_SET);
		}
		for (r=0; r < 8; r++) {
			irow = row - p->top_margin + r;
			if (irow >= p->height) continue;
			for (col=0; col < p->raw_width; col++) {
				icol = col - p->left_margin;
				if (icol < p->width)
					BAYER(irow,icol) = pixel[r*p->raw_width+col];
				else if (col > 1)
					dark[icol & 1] += pixel[r*p->raw_width+col];
			}
		}
	}
	free (pixel);
	dcr_canon_black (p, dark);
}

/*
Not a full implementation of Lossless JPEG, just
enough to decode Canon, Kodak and Adobe DNG images.
*/

int DCR_CLASS dcr_ljpeg_start (DCRAW* p, struct dcr_jhead *jh, int info_only)
{
	int c, tag, len;
	uchar data[0x10000], *dp;

	dcr_init_decoder(p);
	memset (jh, 0, sizeof *jh);
	FORC(6) jh->huff[c] = p->free_decode;
	jh->restart = INT_MAX;
	dcr_fread(p->obj_, data, 2, 1);
	if (data[1] != 0xd8) return 0;
	do {
		dcr_fread(p->obj_, data, 2, 2);
		tag =  data[0] << 8 | data[1];
		len = (data[2] << 8 | data[3]) - 2;
		if (tag <= 0xff00) return 0;
		dcr_fread(p->obj_, data, 1, len);
		switch (tag) {
		case 0xffc3:
			jh->sraw = ((data[7] >> 4) * (data[7] & 15) - 1) & 3;
		case 0xffc0:
			jh->bits = data[0];
			jh->high = data[1] << 8 | data[2];
			jh->wide = data[3] << 8 | data[4];
			jh->clrs = data[5] + jh->sraw;
			if (len == 9 && !p->dng_version) dcr_fgetc(p->obj_);
			break;
		case 0xffc4:
			if (info_only) break;
			for (dp = data; dp < data+len && *dp < 4; ) {
				jh->huff[*dp] = p->free_decode;
				dp = dcr_make_decoder (p, ++dp, 0);
			}
			break;
		case 0xffda:
			jh->psv = data[1+data[0]*2];
			jh->bits -= data[3+data[0]*2] & 15;
			break;
		case 0xffdd:
			jh->restart = data[0] << 8 | data[1];
		}
	} while (tag != 0xffda);
	if (info_only) return 1;
	if (jh->sraw) {
		FORC(4)        jh->huff[2+c] = jh->huff[1];
		FORC(jh->sraw) jh->huff[1+c] = jh->huff[0];
	}
	jh->row = (ushort *) calloc (jh->wide*jh->clrs, 4);
	dcr_merror (p, jh->row, "dcr_ljpeg_start()");
	return p->zero_after_ff = 1;
}

int DCR_CLASS dcr_ljpeg_diff (DCRAW* p, struct dcr_decode *dindex)
{
	int len, diff;

	if (!dindex)
		longjmp (p->failure, 2);

	while (dindex->branch[0])
		dindex = dindex->branch[dcr_getbits(p, 1)];
	len = dindex->leaf;
	if (len == 16 && (!p->dng_version || p->dng_version >= 0x1010000))
		return -32768;
	diff = dcr_getbits(p, len);
	if ((diff & (1 << (len-1))) == 0)
		diff -= (1 << len) - 1;
	return diff;
}

ushort * DCR_CLASS dcr_ljpeg_row (DCRAW* p, int jrow, struct dcr_jhead *jh)
{
	int col, c, diff, pred, spred=0;
	ushort mark=0, *row[3];

	if (jrow * jh->wide % jh->restart == 0) {
		FORC(6) jh->vpred[c] = 1 << (jh->bits-1);
		if (jrow)
			do mark = (mark << 8) + (c = dcr_fgetc(p->obj_));
			while (c != EOF && mark >> 4 != 0xffd);
			dcr_getbits(p, -1);
	}
	FORC3 row[c] = jh->row + jh->wide*jh->clrs*((jrow+c) & 1);
	for (col=0; col < jh->wide; col++)
		FORC(jh->clrs) {
			diff = dcr_ljpeg_diff (p, jh->huff[c]);
			if (jh->sraw && c <= jh->sraw && (col | c))
				pred = spred;
			else if (col) pred = row[0][-jh->clrs];
			else	    pred = (jh->vpred[c] += diff) - diff;
			if (jrow && col) switch (jh->psv) {
				case 1:	break;
				case 2: pred = row[1][0];					break;
				case 3: pred = row[1][-jh->clrs];				break;
				case 4: pred = pred +   row[1][0] - row[1][-jh->clrs];		break;
				case 5: pred = pred + ((row[1][0] - row[1][-jh->clrs]) >> 1);	break;
				case 6: pred = row[1][0] + ((pred - row[1][-jh->clrs]) >> 1);	break;
				case 7: pred = (pred + row[1][0]) >> 1;				break;
				default: pred = 0;
			}
			if ((**row = pred + diff) >> jh->bits) dcr_derror(p);
			if (c <= jh->sraw) spred = **row;
			row[0]++; row[1]++;
		}
		return row[2];
}

void DCR_CLASS dcr_lossless_jpeg_load_raw(DCRAW* p)
{
	int jwide, jrow, jcol, val, jidx, i, j, row=0, col=0;
	double dark[2] = { 0,0 };
	struct dcr_jhead jh;
	int min=INT_MAX;
	ushort *rp;

	if (!dcr_ljpeg_start (p,&jh, 0)) return;

	if (jh.wide<1 || jh.high<1 || jh.clrs<1 || jh.bits<1)
		longjmp (p->failure, 2);

	jwide = jh.wide * jh.clrs;

	for (jrow=0; jrow < jh.high; jrow++) {
		rp = dcr_ljpeg_row (p, jrow, &jh);
		for (jcol=0; jcol < jwide; jcol++) {
			val = *rp++;
			if (jh.bits <= 12)
				val = p->curve[val & 0xfff];
			if (p->cr2_slice[0]) {
				jidx = jrow*jwide + jcol;
				i = jidx / (p->cr2_slice[1]*jh.high);
				if ((j = i >= p->cr2_slice[0]))
					i  = p->cr2_slice[0];
				jidx -= i * (p->cr2_slice[1]*jh.high);
				row = jidx / p->cr2_slice[1+j];
				col = jidx % p->cr2_slice[1+j] + i*p->cr2_slice[1];
			}
			if (p->raw_width == 3984 && (col -= 2) < 0)
				col += (row--,p->raw_width);
			if (row > p->raw_height)
				longjmp (p->failure, 3);
			if ((unsigned) (row-p->top_margin) < p->height) {
				if ((unsigned) (col-p->left_margin) < p->width) {
					BAYER(row-p->top_margin,col-p->left_margin) = val;
					if (min > val) min = val;
				} else if (col > 1)
					dark[(col-p->left_margin) & 1] += val;
			}
			if (++col >= p->raw_width)
				col = (row++,0);
		}
	}
	free (jh.row);
	dcr_canon_black (p, dark);
	if (!strcasecmp(p->make,"KODAK"))
		p->black = min;
}

void DCR_CLASS dcr_canon_sraw_load_raw(DCRAW* p)
{
	struct dcr_jhead jh;
	short *rp=0, (*ip)[4];
	int jwide, slice, scol, ecol, row, col, jrow=0, jcol=0, pix[3], c;
	
	if (!dcr_ljpeg_start (p, &jh, 0)) return;
	jwide = (jh.wide >>= 1) * jh.clrs;
	
	for (ecol=slice=0; slice <= p->cr2_slice[0]; slice++) {
		scol = ecol;
		ecol += p->cr2_slice[1] * 2 / jh.clrs;
		if (!p->cr2_slice[0] || ecol > p->raw_width-1) ecol = p->raw_width & -2;
		for (row=0; row < p->height; row += (jh.clrs >> 1) - 1) {
			ip = (short (*)[4]) p->image + row*p->width;
			for (col=scol; col < ecol; col+=2, jcol+=jh.clrs) {
				if ((jcol %= jwide) == 0)
					rp = (short *) dcr_ljpeg_row (p, jrow++, &jh);
				if (col >= p->width) continue;
				FORC (jh.clrs-2)
					ip[col + (c >> 1)*p->width + (c & 1)][0] = rp[jcol+c];
				ip[col][1] = rp[jcol+jh.clrs-2] - 16384;
				ip[col][2] = rp[jcol+jh.clrs-1] - 16384;
			}
		}
	}
	ip = (short (*)[4]) p->image;
	rp = ip[0];
	for (row=0; row < p->height; row++, ip+=p->width) {
		if (row & (jh.sraw >> 1))
			for (col=0; col < p->width; col+=2)
				for (c=1; c < 3; c++)
					if (row == p->height-1)
						ip[col][c] =  ip[col-p->width][c];
					else ip[col][c] = (ip[col-p->width][c] + ip[col+p->width][c] + 1) >> 1;
					for (col=1; col < p->width; col+=2)
						for (c=1; c < 3; c++)
							if (col == p->width-1)
								ip[col][c] =  ip[col-1][c];
							else ip[col][c] = (ip[col-1][c] + ip[col+1][c] + 1) >> 1;
	}
	for ( ; rp < ip[0]; rp+=4) {
		if (p->unique_id < 0x80000200) {
			pix[0] = rp[0] + rp[2] - 512;
			pix[2] = rp[0] + rp[1] - 512;
			pix[1] = rp[0] + ((-778*rp[1] - (rp[2] << 11)) >> 12) - 512;
		} else {
			rp[1] += jh.sraw+1;
			rp[2] += jh.sraw+1;
			pix[0] = rp[0] + ((  200*rp[1] + 22929*rp[2]) >> 12);
			pix[1] = rp[0] + ((-5640*rp[1] - 11751*rp[2]) >> 12);
			pix[2] = rp[0] + ((29040*rp[1] -   101*rp[2]) >> 12);
		}
		FORC3 rp[c] = CLIP(pix[c] * p->sraw_mul[c] >> 10);
	}
	free (jh.row);
	p->maximum = 0x3fff;
}

void DCR_CLASS dcr_adobe_copy_pixel (DCRAW* p, int row, int col, ushort **rp)
{
	unsigned r, c;

	r = row -= p->top_margin;
	c = col -= p->left_margin;
	if (p->is_raw == 2 && p->opt.shot_select) (*rp)++;
	if (p->filters) {
		if (p->fuji_width) {
			r = row + p->fuji_width - 1 - (col >> 1);
			c = row + ((col+1) >> 1);
		}
		if (r < p->height && c < p->width)
			BAYER(r,c) = **rp < 0x1000 ? p->curve[**rp] : **rp;
		*rp += p->is_raw;
	} else {
		if (r < p->height && c < p->width)
			FORC(p->tiff_samples)
				p->image[row*p->width+col][c] = (*rp)[c] < 0x1000 ? p->curve[(*rp)[c]]:(*rp)[c];
			*rp += p->tiff_samples;
	}
	if (p->is_raw == 2 && p->opt.shot_select) (*rp)--;
}

void DCR_CLASS dcr_adobe_dng_load_raw_lj(DCRAW* p)
{
	unsigned save, trow=0, tcol=0, jwide, jrow, jcol, row, col;
	struct dcr_jhead jh;
	ushort *rp;

	while (trow < p->raw_height) {
		save = dcr_ftell(p->obj_);
		if (p->tile_length < INT_MAX)
			dcr_fseek(p->obj_, dcr_get4(p), SEEK_SET);
		if (!dcr_ljpeg_start (p,&jh, 0)) break;
		jwide = jh.wide;
		if (p->filters) jwide *= jh.clrs;
		jwide /= p->is_raw;
		for (row=col=jrow=0; (int)jrow < jh.high; jrow++) {
			rp = dcr_ljpeg_row (p, jrow, &jh);
			for (jcol=0; jcol < jwide; jcol++) {
				dcr_adobe_copy_pixel (p,trow+row, tcol+col, &rp);
				if (++col >= p->tile_width || col >= p->raw_width)
					row += 1 + (col = 0);
			}
		}
		dcr_fseek(p->obj_, save+4, SEEK_SET);
		if ((tcol += p->tile_width) >= p->raw_width)
			trow += p->tile_length + (tcol = 0);
		free (jh.row);
	}
}

void DCR_CLASS dcr_adobe_dng_load_raw_nc(DCRAW* p)
{
	ushort *pixel, *rp;
	unsigned int row, col;

	pixel = (ushort *) calloc (p->raw_width * p->tiff_samples, sizeof *pixel);
	dcr_merror (p, pixel, "adobe_dng_load_raw_nc()");
	for (row=0; row < p->raw_height; row++) {
		if (p->tiff_bps == 16)
			dcr_read_shorts (p, pixel, p->raw_width * p->tiff_samples);
		else {
			dcr_getbits(p, -1);
			for (col=0; col < p->raw_width * p->tiff_samples; col++)
				pixel[col] = dcr_getbits(p, p->tiff_bps);
		}
		for (rp=pixel, col=0; col < p->raw_width; col++)
			dcr_adobe_copy_pixel (p,row, col, &rp);
	}
	free (pixel);
}

void DCR_CLASS dcr_pentax_k10_load_raw(DCRAW* p)
{
	static const uchar pentax_tree[] =
	{ 0,2,3,1,1,1,1,1,1,2,0,0,0,0,0,0,
	3,4,2,5,1,6,0,7,8,9,10,11,12 };
	int row, col, diff;
	ushort vpred[2][2] = {{0,0},{0,0}}, hpred[2];

	dcr_init_decoder(p);
	dcr_make_decoder (p, pentax_tree, 0);
	dcr_getbits(p, -1);
	for (row=0; row < p->height; row++)
		for (col=0; col < p->raw_width; col++) {
			diff = dcr_ljpeg_diff (p, p->first_decode);
			if (col < 2) hpred[col] = vpred[row & 1][col] += diff;
			else	   hpred[col & 1] += diff;
			if (col < p->width)
				BAYER(row,col) = hpred[col & 1];
			if (hpred[col & 1] >> 12) dcr_derror(p);
		}
}

void DCR_CLASS dcr_nikon_compressed_load_raw(DCRAW* p)
{
	static const uchar nikon_tree[][32] = {
		{ 0,1,5,1,1,1,1,1,1,2,0,0,0,0,0,0,	/* 12-bit lossy */
			5,4,3,6,2,7,1,0,8,9,11,10,12 },
		{ 0,1,5,1,1,1,1,1,1,2,0,0,0,0,0,0,	/* 12-bit lossy after split */
		0x39,0x5a,0x38,0x27,0x16,5,4,3,2,1,0,11,12,12 },
		{ 0,1,4,2,3,1,2,0,0,0,0,0,0,0,0,0,  /* 12-bit lossless */
		5,4,6,3,7,2,8,1,9,0,10,11,12 },
		{ 0,1,4,3,1,1,1,1,1,2,0,0,0,0,0,0,	/* 14-bit lossy */
		5,6,4,7,8,3,9,2,1,0,10,11,12,13,14 },
		{ 0,1,5,1,1,1,1,1,1,1,2,0,0,0,0,0,	/* 14-bit lossy after split */
		8,0x5c,0x4b,0x3a,0x29,7,6,5,4,3,2,1,0,13,14 },
		{ 0,1,4,2,2,3,1,2,0,0,0,0,0,0,0,0,	/* 14-bit lossless */
		7,6,8,5,9,4,10,3,11,12,2,0,1,13,14 } };
	struct dcr_decode *dindex;
	ushort ver0, ver1, vpred[2][2], hpred[2], csize;
	int i, min, max, step=0, huff=0, split=0, row, col, len, shl, diff;

	dcr_fseek(p->obj_, p->meta_offset, SEEK_SET);
	ver0 = dcr_fgetc(p->obj_);
	ver1 = dcr_fgetc(p->obj_);
	if (ver0 == 0x49 || ver1 == 0x58)
		dcr_fseek(p->obj_, 2110, SEEK_CUR);
	if (ver0 == 0x46) huff = 2;
	if (p->tiff_bps == 14) huff += 3;
	dcr_read_shorts (p, vpred[0], 4);
	max = 1 << p->tiff_bps & 0x7fff;
	if ((csize = dcr_get2(p)) > 1)
		step = max / (csize-1);
	if (ver0 == 0x44 && ver1 == 0x20 && step > 0) {
		for (i=0; i < csize; i++)
			p->curve[i*step] = dcr_get2(p);
		for (i=0; i < max; i++)
			p->curve[i] = ( p->curve[i-i%step]*(step-i%step) +
			p->curve[i-i%step+step]*(i%step) ) / step;
		dcr_fseek(p->obj_, p->meta_offset+562, SEEK_SET);
		split = dcr_get2(p);
	} else if (ver0 != 0x46 && csize <= 0x4001)
		dcr_read_shorts (p, p->curve, max=csize);
	while (p->curve[max-2] == p->curve[max-1]) max--;
	dcr_init_decoder(p);
	dcr_make_decoder (p, nikon_tree[huff], 0);
	dcr_fseek(p->obj_, p->data_offset, SEEK_SET);
	dcr_getbits(p, -1);
	for (min=row=0; row < p->height; row++) {
		if (split && row == split) {
			dcr_init_decoder(p);
			dcr_make_decoder (p, nikon_tree[huff+1], 0);
			max += (min = 16) << 1;
		}
		for (col=0; col < p->raw_width; col++) {
			for (dindex=p->first_decode; dindex->branch[0]; )
				dindex = dindex->branch[dcr_getbits(p, 1)];
			len = dindex->leaf & 15;
			shl = dindex->leaf >> 4;
			diff = ((dcr_getbits(p, len-shl) << 1) + 1) << shl >> 1;
			if ((diff & (1 << (len-1))) == 0)
				diff -= (1 << len) - !shl;
			if (col < 2) hpred[col] = vpred[row & 1][col] += diff;
			else	   hpred[col & 1] += diff;
			if ((ushort)(hpred[col & 1] + min) >= max) dcr_derror(p);
			if ((unsigned) (col-p->left_margin) < p->width)
				BAYER(row,col-p->left_margin) = p->curve[LIM((short)hpred[col & 1],0,0x3fff)];
		}
	}
}

/*
Figure out if a NEF file is compressed.  These fancy heuristics
are only needed for the D100, thanks to a bug in some cameras
that tags all images as "compressed".
*/
int DCR_CLASS dcr_nikon_is_compressed(DCRAW* p)
{
	uchar test[256];
	int i;

	dcr_fseek(p->obj_, p->data_offset, SEEK_SET);
	dcr_fread(p->obj_, test, 1, 256);
	for (i=15; i < 256; i+=16)
		if (test[i]) return 1;
		return 0;
}

/*
Returns 1 for a Coolpix 995, 0 for anything else.
*/
int DCR_CLASS dcr_nikon_e995(DCRAW* p)
{
	int i, histo[256];
	const uchar often[] = { 0x00, 0x55, 0xaa, 0xff };

	memset (histo, 0, sizeof histo);
	dcr_fseek(p->obj_, -2000, SEEK_END);
	for (i=0; i < 2000; i++)
		histo[dcr_fgetc(p->obj_)]++;
	for (i=0; i < 4; i++)
		if (histo[often[i]] < 200)
			return 0;
		return 1;
}

/*
Returns 1 for a Coolpix 2100, 0 for anything else.
*/
int DCR_CLASS dcr_nikon_e2100(DCRAW* p)
{
	uchar t[12];
	int i;

	dcr_fseek(p->obj_, 0, SEEK_SET);
	for (i=0; i < 1024; i++) {
		dcr_fread(p->obj_, t, 1, 12);
		if (((t[2] & t[4] & t[7] & t[9]) >> 4
			& t[1] & t[6] & t[8] & t[11] & 3) != 3)
			return 0;
	}
	return 1;
}

void DCR_CLASS dcr_nikon_3700(DCRAW* p)
{
	int bits, i;
	uchar dp[24];
	const struct {
		int bits;
		char make[12], model[15];
	} table[] = {
		{ 0x00, "PENTAX",  "Optio 33WR" },
		{ 0x03, "NIKON",   "E3200" },
		{ 0x32, "NIKON",   "E3700" },
		{ 0x33, "OLYMPUS", "C740UZ" } };

	dcr_fseek(p->obj_, 3072, SEEK_SET);
	dcr_fread(p->obj_, dp, 1, 24);
	bits = (dp[8] & 3) << 4 | (dp[20] & 3);
	for (i=0; i < sizeof table / sizeof *table; i++)
		if (bits == table[i].bits) {
			strcpy (p->make,  table[i].make );
			strcpy (p->model, table[i].model);
		}
}

/*
Separates a Minolta DiMAGE Z2 from a Nikon E4300.
*/
int DCR_CLASS dcr_minolta_z2(DCRAW* p)
{
	int i, nz;
	char tail[424];

	dcr_fseek(p->obj_, -(long)(sizeof tail), SEEK_END);
	dcr_fread(p->obj_, tail, 1, sizeof tail);
	for (nz=i=0; i < sizeof tail; i++)
		if (tail[i]) nz++;
	return nz > 20;
}

/* Here p->raw_width is in bytes, not pixels. */
void DCR_CLASS dcr_nikon_e900_load_raw(DCRAW* p)
{
	int offset=0, irow, row, col;

	for (irow=0; irow < p->height; irow++) {
		row = irow * 2 % p->height;
		if (row == 1)
			offset = - (-offset & -4096);
		dcr_fseek(p->obj_, offset, SEEK_SET);
		offset += p->raw_width;
		dcr_getbits(p, -1);
		for (col=0; col < p->width; col++)
			BAYER(row,col) = dcr_getbits(p, 10);
	}
}

/*
The Fuji Super CCD is just a Bayer grid rotated 45 degrees.
*/
void DCR_CLASS dcr_fuji_load_raw(DCRAW* p)
{
	ushort *pixel;
	int wide, row, col, r, c;

	dcr_fseek(p->obj_, (p->top_margin*p->raw_width + p->left_margin) * 2, SEEK_CUR);
	wide = p->fuji_width << !p->fuji_layout;
	pixel = (ushort *) calloc (wide, sizeof *pixel);
	dcr_merror (p, pixel, "fuji_load_raw()");
	for (row=0; row < p->raw_height; row++) {
		dcr_read_shorts (p, pixel, wide);
		dcr_fseek(p->obj_, 2*(p->raw_width - wide), SEEK_CUR);
		for (col=0; col < wide; col++) {
			if (p->fuji_layout) {
				r = p->fuji_width - 1 - col + (row >> 1);
				c = col + ((row+1) >> 1);
			} else {
				r = p->fuji_width - 1 + row - (col >> 1);
				c = row + ((col+1) >> 1);
			}
			BAYER(r,c) = pixel[col];
		}
	}
	free (pixel);
}

void DCR_CLASS dcr_jpeg_thumb (DCRAW* p, FILE *tfp);

void DCR_CLASS dcr_ppm_thumb (DCRAW* p, FILE *tfp)
{
	char *thumb;
	p->thumb_length = p->thumb_width*p->thumb_height*3;
	thumb = (char *) malloc (p->thumb_length);
	dcr_merror (p, thumb, "ppm_thumb()");
	fprintf (tfp, "P6\n%d %d\n255\n", p->thumb_width, p->thumb_height);
	dcr_fread(p->obj_, thumb, 1, p->thumb_length);
	fwrite (thumb, 1, p->thumb_length, tfp);
	free (thumb);
}

void DCR_CLASS dcr_layer_thumb (DCRAW* p, FILE *tfp)
{
	int i, c;
	char *thumb, map[][4] = { "012","102" };

	p->colors = p->thumb_misc >> 5 & 7;
	p->thumb_length = p->thumb_width*p->thumb_height;
	thumb = (char *) calloc (p->colors, p->thumb_length);
	dcr_merror (p, thumb, "layer_thumb()");
	fprintf (tfp, "P%d\n%d %d\n255\n",
		5 + (p->colors >> 1), p->thumb_width, p->thumb_height);
	dcr_fread(p->obj_, thumb, p->thumb_length, p->colors);
	for (i=0; i < (int)p->thumb_length; i++)
		FORCC(p) putc (thumb[i+p->thumb_length*(map[p->thumb_misc >> 8][c]-'0')], tfp);
	free (thumb);
}

void DCR_CLASS dcr_rollei_thumb (DCRAW* p, FILE *tfp)
{
	unsigned i;
	ushort *thumb;

	p->thumb_length = p->thumb_width * p->thumb_height;
	thumb = (ushort *) calloc (p->thumb_length, 2);
	dcr_merror (p, thumb, "rollei_thumb()");
	fprintf (tfp, "P6\n%d %d\n255\n", p->thumb_width, p->thumb_height);
	dcr_read_shorts (p, thumb, p->thumb_length);
	for (i=0; i < p->thumb_length; i++) {
		putc (thumb[i] << 3, tfp);
		putc (thumb[i] >> 5  << 2, tfp);
		putc (thumb[i] >> 11 << 3, tfp);
	}
	free (thumb);
}

void DCR_CLASS dcr_rollei_load_raw(DCRAW* p)
{
	uchar pixel[10];
	unsigned iten=0, isix, i, buffer=0, row, col, todo[16];

	isix = p->raw_width * p->raw_height * 5 / 8;
	while (dcr_fread(p->obj_, pixel, 1, 10) == 10) {
		for (i=0; i < 10; i+=2) {
			todo[i]   = iten++;
			todo[i+1] = pixel[i] << 8 | pixel[i+1];
			buffer    = pixel[i] >> 2 | buffer << 6;
		}
		for (   ; i < 16; i+=2) {
			todo[i]   = isix++;
			todo[i+1] = buffer >> (14-i)*5;
		}
		for (i=0; i < 16; i+=2) {
			row = todo[i] / p->raw_width - p->top_margin;
			col = todo[i] % p->raw_width - p->left_margin;
			if (row < p->height && col < p->width)
				BAYER(row,col) = (todo[i+1] & 0x3ff);
		}
	}
	p->maximum = 0x3ff;
}

int DCR_CLASS dcr_bayer (DCRAW* p, unsigned row, unsigned col)
{
	return (row < p->height && col < p->width) ? BAYER(row,col) : 0;
}

void DCR_CLASS dcr_phase_one_flat_field (DCRAW* p, int is_float, int nc)
{
	ushort head[8];
	unsigned wide, y, x, rend, cend, row, col;
	int c;
	float *mrow, num, mult[4];

	dcr_read_shorts (p, head, 8);
	wide = head[2] / head[4];
	mrow = (float *) calloc (nc*wide, sizeof *mrow);
	dcr_merror (p, mrow, "phase_one_flat_field()");
	for (y=0; y < (unsigned int)(head[3] / head[5]); y++) {
		for (x=0; x < wide; x++)
			for (c=0; c < nc; c+=2) {
				num = is_float ? (float)dcr_getreal(p, 11) : dcr_get2(p)/32768.0f;
				if (y==0) mrow[c*wide+x] = num;
				else mrow[(c+1)*wide+x] = (num - mrow[c*wide+x]) / head[5];
			}
		if (y==0) continue;
		rend = head[1]-p->top_margin + y*head[5];
		for (row = rend-head[5]; row < p->height && row < rend; row++) {
			for (x=1; x < wide; x++) {
				for (c=0; c < nc; c+=2) {
					mult[c] = mrow[c*wide+x-1];
					mult[c+1] = (mrow[c*wide+x] - mult[c]) / head[4];
				}
				cend = head[0]-p->left_margin + x*head[4];
				for (col = cend-head[4]; col < p->width && col < cend; col++) {
					c = nc > 2 ? FC(row,col) : 0;
					if (!(c & 1)) {
						c = (int)(BAYER(row,col) * mult[c]);
						BAYER(row,col) = LIM(c,0,65535);
					}
					for (c=0; c < nc; c+=2)
						mult[c] += mult[c+1];
				}
			}
			for (x=0; x < wide; x++)
				for (c=0; c < nc; c+=2)
					mrow[c*wide+x] += mrow[(c+1)*wide+x];
		}
	}
	free (mrow);
}

void DCR_CLASS dcr_phase_one_correct(DCRAW* p)
{
	unsigned entries, tag, data, save, col, row, type;
	int len, i, j, k, cip, val[4], dev[4], sum, max;
	int head[9], diff, mindiff=INT_MAX, off_412=0;
	static const signed char dir[12][2] =
	{ {-1,-1}, {-1,1}, {1,-1}, {1,1}, {-2,0}, {0,-2}, {0,2}, {2,0},
	{-2,-2}, {-2,2}, {2,-2}, {2,2} };
	float poly[8], num, cfrac, frac, mult[2], *yval[2];
	ushort curve[0x10000], *xval[2];

	if (p->opt.half_size || !p->meta_length) return;
	if (p->opt.verbose) fprintf (stderr,_("Phase One correction...\n"));
	dcr_fseek(p->obj_, p->meta_offset, SEEK_SET);
	p->order = dcr_get2(p);
	dcr_fseek(p->obj_, 6, SEEK_CUR);
	dcr_fseek(p->obj_, p->meta_offset+dcr_get4(p), SEEK_SET);
	entries = dcr_get4(p);  dcr_get4(p);
	while (entries--) {
		tag  = dcr_get4(p);
		len  = dcr_get4(p);
		data = dcr_get4(p);
		save = dcr_ftell(p->obj_);
		dcr_fseek(p->obj_, p->meta_offset+data, SEEK_SET);
		if (tag == 0x419) {				/* Polynomial curve */
			for (dcr_get4(p), i=0; i < 8; i++)
				poly[i] = (float)dcr_getreal(p, 11);
			poly[3] += (p->ph1.tag_210 - poly[7]) * poly[6] + 1;
			for (i=0; i < 0x10000; i++) {
				num = (poly[5]*i + poly[3])*i + poly[1];
				curve[i] = (unsigned short)LIM(num,0,65535);
			} goto apply;				/* apply to right half */
		} else if (tag == 0x41a) {			/* Polynomial curve */
			for (i=0; i < 4; i++)
				poly[i] = (float)dcr_getreal(p, 11);
			for (i=0; i < 0x10000; i++) {
				for (num=0, j=4; j--; )
					num = num * i + poly[j];
				curve[i] = (unsigned short)LIM(num+i,0,65535);
			} apply:					/* apply to whole image */
			for (row=0; row < p->height; row++)
				for (col = (tag & 1)*p->ph1.split_col; col < p->width; col++)
					BAYER(row,col) = curve[BAYER(row,col)];
		} else if (tag == 0x400) {			/* Sensor defects */
			while ((len -= 8) >= 0) {
				col  = dcr_get2(p) - p->left_margin;
				row  = dcr_get2(p) - p->top_margin;
				type = dcr_get2(p); dcr_get2(p);
				if (col >= p->width) continue;
				if (type == 131)			/* Bad column */
					for (row=0; row < p->height; row++)
						if (FC(row,col) == 1) {
							for (sum=i=0; i < 4; i++)
								sum += val[i] = dcr_bayer (p, row+dir[i][0], col+dir[i][1]);
							for (max=i=0; i < 4; i++) {
								dev[i] = abs((val[i] << 2) - sum);
								if (dev[max] < dev[i]) max = i;
							}
							BAYER(row,col) = (unsigned short)((sum - val[max])/3.0 + 0.5);
						} else {
							for (sum=0, i=8; i < 12; i++)
								sum += dcr_bayer (p, row+dir[i][0], col+dir[i][1]);
							BAYER(row,col) = (unsigned short)(0.5 + sum * 0.0732233 +
								(dcr_bayer(p, row,col-2) + dcr_bayer(p, row,col+2)) * 0.3535534);
						}
						else if (type == 129) {			/* Bad pixel */
							if (row >= p->height) continue;
							j = (FC(row,col) != 1) * 4;
							for (sum=0, i=j; i < j+8; i++)
								sum += dcr_bayer (p, row+dir[i][0], col+dir[i][1]);
							BAYER(row,col) = (sum + 4) >> 3;
						}
			}
		} else if (tag == 0x401) {			/* All-color flat fields */
			dcr_phase_one_flat_field (p, 1, 2);
		} else if (tag == 0x416 || tag == 0x410) {
			dcr_phase_one_flat_field (p, 0, 2);
		} else if (tag == 0x40b) {			/* Red+blue flat field */
			dcr_phase_one_flat_field (p, 0, 4);
		} else if (tag == 0x412) {
			dcr_fseek(p->obj_, 36, SEEK_CUR);
			diff = abs (dcr_get2(p) - p->ph1.tag_21a);
			if (mindiff > diff) {
				mindiff = diff;
				off_412 = dcr_ftell(p->obj_) - 38;
			}
		}
		dcr_fseek(p->obj_, save, SEEK_SET);
	}
	if (off_412) {
		dcr_fseek(p->obj_, off_412, SEEK_SET);
		for (i=0; i < 9; i++) head[i] = dcr_get4(p) & 0x7fff;
		yval[0] = (float *) calloc (head[1]*head[3] + head[2]*head[4], 6);
		dcr_merror (p, yval[0], "phase_one_correct()");
		yval[1] = (float  *) (yval[0] + head[1]*head[3]);
		xval[0] = (ushort *) (yval[1] + head[2]*head[4]);
		xval[1] = (ushort *) (xval[0] + head[1]*head[3]);
		dcr_get2(p);
		for (i=0; i < 2; i++)
			for (j=0; j < head[i+1]*head[i+3]; j++)
				yval[i][j] = (float)dcr_getreal(p, 11);
		for (i=0; i < 2; i++)
			for (j=0; j < head[i+1]*head[i+3]; j++)
				xval[i][j] = dcr_get2(p);
		for (row=0; row < p->height; row++)
			for (col=0; col < p->width; col++) {
				cfrac = (float) col * head[3] / p->raw_width;
				cfrac -= cip = (int)cfrac;
				num = (float)(BAYER(row,col) * 0.5);
				for (i=cip; i < cip+2; i++) {
					for (k=j=0; j < head[1]; j++)
						if (num < xval[0][k = head[1]*i+j]) break;
						frac = (j == 0 || j == head[1]) ? 0 :
						(xval[0][k] - num) / (xval[0][k] - xval[0][k-1]);
						mult[i-cip] = yval[0][k-1] * frac + yval[0][k] * (1-frac);
				}
				i = (int)(((mult[0] * (1-cfrac) + mult[1] * cfrac)
					* (row + p->top_margin) + num) * 2);
				BAYER(row,col) = LIM(i,0,65535);
			}
		free (yval[0]);
	}
}

void DCR_CLASS dcr_phase_one_load_raw(DCRAW* p)
{
	int row, col, a, b;
	ushort *pixel, akey, bkey, mask;

	dcr_fseek(p->obj_, p->ph1.key_off, SEEK_SET);
	akey = dcr_get2(p);
	bkey = dcr_get2(p);
	mask = p->ph1.format == 1 ? 0x5555:0x1354;
	dcr_fseek(p->obj_, p->data_offset + p->top_margin*p->raw_width*2, SEEK_SET);
	pixel = (ushort *) calloc (p->raw_width, sizeof *pixel);
	dcr_merror (p, pixel, "phase_one_load_raw()");
	for (row=0; row < p->height; row++) {
		dcr_read_shorts (p, pixel, p->raw_width);
		for (col=0; col < p->raw_width; col+=2) {
			a = pixel[col+0] ^ akey;
			b = pixel[col+1] ^ bkey;
			pixel[col+0] = (a & mask) | (b & ~mask);
			pixel[col+1] = (b & mask) | (a & ~mask);
		}
		for (col=0; col < p->width; col++)
			BAYER(row,col) = pixel[col+p->left_margin];
	}
	free (pixel);
	dcr_phase_one_correct(p);
}

unsigned DCR_CLASS dcr_ph1_bits (DCRAW* p,int nbits)
{
	if (nbits == -1)
		return (unsigned int)(p->ph1_bits_bitbuf = p->ph1_bits_vbits = 0);
	if (nbits == 0) return 0;
	if ((p->ph1_bits_vbits -= nbits) < 0) {
		p->ph1_bits_bitbuf = p->ph1_bits_bitbuf << 32 | dcr_get4(p);
		p->ph1_bits_vbits += 32;
	}
	return (unsigned int)(p->ph1_bits_bitbuf << (64-nbits-p->ph1_bits_vbits) >> (64-nbits));
}

void DCR_CLASS dcr_phase_one_load_raw_c(DCRAW* p)
{
	static const int length[] = { 8,7,6,9,11,10,5,12,14,13 };
	int *offset, len[2], pred[2], row, col, i, j;
	ushort *pixel;
	short (*black)[2];

	pixel = (ushort *) calloc (p->raw_width + p->raw_height*4, 2);
	dcr_merror (p, pixel, "phase_one_load_raw_c()");
	offset = (int *) (pixel + p->raw_width);
	dcr_fseek(p->obj_, p->strip_offset, SEEK_SET);
	for (row=0; row < p->raw_height; row++)
		offset[row] = dcr_get4(p);
	black = (short (*)[2]) offset + p->raw_height;
	dcr_fseek(p->obj_, p->ph1.black_off, SEEK_SET);
	if (p->ph1.black_off)
		dcr_read_shorts (p, (ushort *) black[0], p->raw_height*2);
	for (i=0; i < 256; i++)
		p->curve[i] = (unsigned short)(i*i / 3.969 + 0.5);
	for (row=0; row < p->raw_height; row++) {
		dcr_fseek(p->obj_, p->data_offset + offset[row], SEEK_SET);
		dcr_ph1_bits(p,-1);
		pred[0] = pred[1] = 0;
		for (col=0; col < p->raw_width; col++) {
			if (col >= (p->raw_width & -8))
				len[0] = len[1] = 14;
			else if ((col & 7) == 0)
				for (i=0; i < 2; i++) {
					for (j=0; j < 5 && !dcr_ph1_bits(p,1); j++);
					if (j--) len[i] = length[j*2 + dcr_ph1_bits(p,1)];
				}
				if ((i = len[col & 1]) == 14)
					pixel[col] = pred[col & 1] = dcr_ph1_bits(p,16);
				else
					pixel[col] = pred[col & 1] += dcr_ph1_bits(p,i) + 1 - (1 << (i - 1));
				if (pred[col & 1] >> 16) dcr_derror(p);
				if (p->ph1.format == 5 && pixel[col] < 256)
					pixel[col] = p->curve[pixel[col]];
		}
		if ((unsigned) (row-p->top_margin) < p->height)
			for (col=0; col < p->width; col++) {
				i = (pixel[col+p->left_margin] << 2)
					- p->ph1.black + black[row][col >= p->ph1.split_col];
				if (i > 0) BAYER(row-p->top_margin,col) = i;
			}
	}
	free (pixel);
	dcr_phase_one_correct(p);
	p->maximum = 0xfffc - p->ph1.black;
}

void DCR_CLASS dcr_hasselblad_load_raw(DCRAW* p)
{
	struct dcr_jhead jh;
	struct dcr_decode *dindex;
	int row, col, pred[2], len[2], diff, i;

	if (!dcr_ljpeg_start (p,&jh, 0)) return;
	free (jh.row);
	p->order = 0x4949;
	dcr_ph1_bits(p,-1);
	for (row=-p->top_margin; row < p->height; row++) {
		pred[0] = pred[1] = 0x8000;
		for (col=-p->left_margin; col < p->raw_width-p->left_margin; col+=2) {
			for (i=0; i < 2; i++) {
				for (dindex=jh.huff[0]; dindex->branch[0]; )
					dindex = dindex->branch[dcr_ph1_bits(p,1)];
				len[i] = dindex->leaf;
			}
			for (i=0; i < 2; i++) {
				diff = dcr_ph1_bits(p,len[i]);
				if ((diff & (1 << (len[i]-1))) == 0)
					diff -= (1 << len[i]) - 1;
				if (diff == 65535) diff = -32768;
				pred[i] += diff;
				if (row >= 0 && (unsigned)(col+i) < p->width)
					BAYER(row,col+i) = pred[i];
			}
		}
	}
	p->maximum = 0xffff;
}

void DCR_CLASS dcr_leaf_hdr_load_raw(DCRAW* p)
{
	ushort *pixel;
	unsigned tile=0, r, c, row, col;

	pixel = (ushort *) calloc (p->raw_width, sizeof *pixel);
	dcr_merror (p, pixel, "leaf_hdr_load_raw()");
	FORC(p->tiff_samples) {
		for (r=0; r < p->raw_height; r++) {
			if (r % p->tile_length == 0) {
				dcr_fseek(p->obj_, p->data_offset + 4*tile++, SEEK_SET);
				dcr_fseek(p->obj_, dcr_get4(p) + 2*p->left_margin, SEEK_SET);
			}
			if (p->filters && c != p->opt.shot_select) continue;
			dcr_read_shorts (p, pixel, p->raw_width);
			if ((row = r - p->top_margin) >= p->height) continue;
			for (col=0; col < p->width; col++)
				if (p->filters)  BAYER(row,col) = pixel[col];
				else p->image[row*p->width+col][c] = pixel[col];
		}
	}
	free (pixel);
	if (!p->filters) {
		p->maximum = 0xffff;
		p->raw_color = 1;
	}
}

void DCR_CLASS dcr_sinar_4shot_load_raw(DCRAW* p)
{
	ushort *pixel;
	unsigned shot, row, col, r, c;

	if ((shot = p->opt.shot_select) || p->opt.half_size) {
		if (shot) shot--;
		if (shot > 3) shot = 3;
		dcr_fseek(p->obj_, p->data_offset + shot*4, SEEK_SET);
		dcr_fseek(p->obj_, dcr_get4(p), SEEK_SET);
		dcr_unpacked_load_raw(p);
		return;
	}
	free (p->image);
	p->image = (ushort (*)[4])
		calloc ((p->iheight=p->height)*(p->iwidth=p->width), sizeof *p->image);
	dcr_merror (p, p->image, "sinar_4shot_load_raw()");
	pixel = (ushort *) calloc (p->raw_width, sizeof *pixel);
	dcr_merror (p, pixel, "sinar_4shot_load_raw()");
	for (shot=0; shot < 4; shot++) {
		dcr_fseek(p->obj_, p->data_offset + shot*4, SEEK_SET);
		dcr_fseek(p->obj_, dcr_get4(p), SEEK_SET);
		for (row=0; row < p->raw_height; row++) {
			dcr_read_shorts (p, pixel, p->raw_width);
			if ((r = row-p->top_margin - (shot >> 1 & 1)) >= p->height) continue;
			for (col=0; col < p->raw_width; col++) {
				if ((c = col-p->left_margin - (shot & 1)) >= p->width) continue;
				p->image[r*p->width+c][FC(row,col)] = pixel[col];
			}
		}
	}
	free (pixel);
	p->shrink = p->filters = 0;
}

void DCR_CLASS dcr_imacon_full_load_raw(DCRAW* p)
{
	int row, col;

	for (row=0; row < p->height; row++)
		for (col=0; col < p->width; col++)
			dcr_read_shorts (p, p->image[row*p->width+col], 3);
}

void DCR_CLASS dcr_packed_12_load_raw(DCRAW* p)
{
	int vbits=0, rbits=0, irow, row, col;
	UINT64 bitbuf=0;

	if (p->raw_width * 2 >= p->width * 3) {	/* If raw_width is in bytes, */
		rbits = p->raw_width * 8;
		p->raw_width = p->raw_width * 2 / 3;	/* convert it to pixels and  */
		rbits -= p->raw_width * 12;		/* save the remainder.       */
	}
	p->order = p->load_flags & 1 ? 0x4949 : 0x4d4d;
	for (irow=0; irow < p->height; irow++) {
		row = irow;
		if (p->load_flags & 2 &&
			(row = irow * 2 % p->height + irow / (p->height/2)) == 1 &&
			p->load_flags & 4) {
			if (vbits=0, p->tiff_compress)
				dcr_fseek(p->obj_, p->data_offset - (-p->width*p->height*3/4 & -2048), SEEK_SET);
			else {
				dcr_fseek(p->obj_, 0, SEEK_END);
				dcr_fseek(p->obj_, dcr_ftell(p->obj_)/2, SEEK_SET);
			}
		}
		for (col=0; col < p->raw_width; col++) {
			if ((vbits -= 12) < 0) {
				bitbuf = bitbuf << 32 | dcr_get4(p);
				vbits += 32;
			}
			if ((unsigned) (col-p->left_margin) < p->width)
				BAYER(row,col-p->left_margin) = (unsigned short)(bitbuf << (52-vbits) >> 52);
			if (p->load_flags & 8 && (col % 10) == 9)
				if (vbits=0, bitbuf & 255) dcr_derror(p);
		}
		vbits -= rbits;
	}
	if (!strcmp(p->make,"OLYMPUS")) p->black >>= 4;
}

void DCR_CLASS dcr_unpacked_load_raw(DCRAW* p)
{
	ushort *pixel;
	int row, col, bits=0;

	while (1 << ++bits < (int)p->maximum);
	dcr_fseek(p->obj_, (p->top_margin*p->raw_width + p->left_margin) * 2, SEEK_CUR);
	pixel = (ushort *) calloc (p->width, sizeof *pixel);
	dcr_merror (p, pixel, "unpacked_load_raw()");
	for (row=0; row < p->height; row++) {
		dcr_read_shorts (p, pixel, p->width);
		dcr_fseek(p->obj_, 2*(p->raw_width - p->width), SEEK_CUR);
		for (col=0; col < p->width; col++)
			if ((BAYER2(row,col) = pixel[col]) >> bits) dcr_derror(p);
	}
	free (pixel);
}

void DCR_CLASS nokia_load_raw(DCRAW* p)
{
  uchar  *data,  *dp;
  ushort *pixel, *pix;
  int dwide, row, c;

  dwide = p->raw_width * 5 / 4;
  data = (uchar *) malloc (dwide + p->raw_width*2);
  dcr_merror (p,data, "nokia_load_raw()");
  pixel = (ushort *) (data + dwide);
  for (row=0; row < p->raw_height; row++) {
    if (dcr_fread(p->obj_, data, 1, dwide) < dwide) dcr_derror(p);
    for (dp=data, pix=pixel; pix < pixel+p->raw_width; dp+=5, pix+=4)
      FORC4 pix[c] = (dp[c] << 2) | (dp[4] >> (c << 1) & 3);
    if (row < p->top_margin)
      FORC(p->width) p->black += pixel[c];
    else
      FORC(p->width) BAYER(row-p->top_margin,c) = pixel[c];
  }
  free (data);
  if (p->top_margin) p->black /= p->top_margin * p->width;
  p->maximum = 0x3ff;
}

unsigned DCR_CLASS dcr_pana_bits (DCRAW* p,int nbits)
{
  int byte;

  if (!nbits) return p->pana_bits_vbits=0;
  if (!p->pana_bits_vbits) {
    dcr_fread(p->obj_, p->pana_bits_buf+p->load_flags, 1, 0x4000-p->load_flags);
    dcr_fread(p->obj_, p->pana_bits_buf, 1, p->load_flags);
  }
  p->pana_bits_vbits = (p->pana_bits_vbits - nbits) & 0x1ffff;
  byte = p->pana_bits_vbits >> 3 ^ 0x3ff0;
  return (p->pana_bits_buf[byte] | p->pana_bits_buf[byte+1] << 8) >> (p->pana_bits_vbits & 7) & ~(-1 << nbits);
}

void DCR_CLASS dcr_panasonic_load_raw(DCRAW* p)
{
  int row, col, i, j, sh=0, pred[2], nonz[2];

  dcr_pana_bits(p,0);
  for (row=0; row < p->height; row++)
    for (col=0; col < p->raw_width; col++) {
      if ((i = col % 14) == 0)
	pred[0] = pred[1] = nonz[0] = nonz[1] = 0;
      if (i % 3 == 2) sh = 4 >> (3 - dcr_pana_bits(p,2));
      if (nonz[i & 1]) {
	if ((j = dcr_pana_bits(p,8))) {
	  if ((pred[i & 1] -= 0x80 << sh) < 0 || sh == 4)
	       pred[i & 1] &= ~(-1 << sh);
	  pred[i & 1] += j << sh;
	}
      } else if ((nonz[i & 1] = dcr_pana_bits(p,8)) || i > 11)
	pred[i & 1] = nonz[i & 1] << 4 | dcr_pana_bits(p,4);
      if (col < p->width)
	if ((BAYER(row,col) = pred[col & 1]) > 4098) dcr_derror(p);
    }
}

void DCR_CLASS dcr_olympus_e300_load_raw(DCRAW* p)
{
	uchar  *data,  *dp;
	ushort *pixel, *pix;
	int dwide, row, col;

	dwide = p->raw_width * 16 / 10;
	dcr_fseek(p->obj_, dwide*p->top_margin, SEEK_CUR);
	data = (uchar *) malloc (dwide + p->raw_width*2);
	dcr_merror (p, data, "olympus_e300_load_raw()");
	pixel = (ushort *) (data + dwide);
	for (row=0; row < p->height; row++) {
		if ((int)dcr_fread(p->obj_, data, 1, dwide) < dwide) dcr_derror(p);
		for (dp=data, pix=pixel; pix < pixel+p->raw_width; dp+=3, pix+=2) {
			if (((dp-data) & 15) == 15)
				if (*dp++ && pix < pixel+p->width+p->left_margin) dcr_derror(p);
				pix[0] = dp[1] << 8 | dp[0];
				pix[1] = dp[2] << 4 | dp[1] >> 4;
		}
		for (col=0; col < p->width; col++)
			BAYER(row,col) = (pixel[col+p->left_margin] & 0xfff);
	}
	free (data);
	p->maximum >>= 4;
	p->black >>= 4;
}

void DCR_CLASS dcr_olympus_e410_load_raw(DCRAW* p)
{
	int row, col, nbits, sign, low, high, i, w, n, nw;
	int acarry[2][3], *carry, pred, diff;

	dcr_fseek(p->obj_, 7, SEEK_CUR);
	dcr_getbits(p, -1);
	for (row=0; row < p->height; row++) {
		memset (acarry, 0, sizeof acarry);
		for (col=0; col < p->width; col++) {
			carry = acarry[col & 1];
			i = 2 * (carry[2] < 3);
			for (nbits=2+i; (ushort) carry[0] >> (nbits+i); nbits++);
			sign = dcr_getbits(p, 1) * -1;
			low  = dcr_getbits(p, 2);
			for (high=0; high < 12; high++)
				if (dcr_getbits(p, 1)) break;
				if (high == 12)
					high = dcr_getbits(p, 16-nbits) >> 1;
				carry[0] = (high << nbits) | dcr_getbits(p, nbits);
				diff = (carry[0] ^ sign) + carry[1];
				carry[1] = (diff*3 + carry[1]) >> 5;
				carry[2] = carry[0] > 16 ? 0 : carry[2]+1;
				if (row < 2 && col < 2) pred = 0;
				else if (row < 2) pred = BAYER(row,col-2);
				else if (col < 2) pred = BAYER(row-2,col);
				else {
					w  = BAYER(row,col-2);
					n  = BAYER(row-2,col);
					nw = BAYER(row-2,col-2);
					if ((w < nw && nw < n) || (n < nw && nw < w)) {
						if (ABS(w-nw) > 32 || ABS(n-nw) > 32)
							pred = w + n - nw;
						else pred = (w + n) >> 1;
					} else pred = ABS(w-nw) > ABS(n-nw) ? w : n;
				}
				if ((BAYER(row,col) = pred + ((diff << 2) | low)) >> 12) dcr_derror(p);
		}
	}
}

void DCR_CLASS dcr_minolta_rd175_load_raw(DCRAW* p)
{
	uchar pixel[768];
	unsigned irow, box, row, col;

	for (irow=0; irow < 1481; irow++) {
		if (dcr_fread(p->obj_, pixel, 1, 768) < 768) dcr_derror(p);
		box = irow / 82;
		row = irow % 82 * 12 + ((box < 12) ? box | 1 : (box-12)*2);
		switch (irow) {
		case 1477: case 1479: continue;
		case 1476: row = 984; break;
		case 1480: row = 985; break;
		case 1478: row = 985; box = 1;
		}
		if ((box < 12) && (box & 1)) {
			for (col=0; col < 1533; col++, row ^= 1)
				if (col != 1) BAYER(row,col) = (col+1) & 2 ?
					pixel[col/2-1] + pixel[col/2+1] : pixel[col/2] << 1;
				BAYER(row,1)    = pixel[1]   << 1;
				BAYER(row,1533) = pixel[765] << 1;
		} else
			for (col=row & 1; col < 1534; col+=2)
				BAYER(row,col) = pixel[col/2] << 1;
	}
	p->maximum = 0xff << 1;
}

void DCR_CLASS dcr_casio_qv5700_load_raw(DCRAW* p)
{
	uchar  data[3232],  *dp;
	ushort pixel[2576], *pix;
	int row, col;

	for (row=0; row < p->height; row++) {
		dcr_fread(p->obj_, data, 1, 3232);
		for (dp=data, pix=pixel; dp < data+3220; dp+=5, pix+=4) {
			pix[0] = (dp[0] << 2) + (dp[1] >> 6);
			pix[1] = (dp[1] << 4) + (dp[2] >> 4);
			pix[2] = (dp[2] << 6) + (dp[3] >> 2);
			pix[3] = (dp[3] << 8) + (dp[4]     );
		}
		for (col=0; col < p->width; col++)
			BAYER(row,col) = (pixel[col] & 0x3ff);
	}
	p->maximum = 0x3fc;
}

void DCR_CLASS dcr_quicktake_100_load_raw(DCRAW* p)
{
	uchar pixel[484][644];
	static const short gstep[16] =
	{ -89,-60,-44,-32,-22,-15,-8,-2,2,8,15,22,32,44,60,89 };
	static const short rstep[6][4] =
	{ {  -3,-1,1,3  }, {  -5,-1,1,5  }, {  -8,-2,2,8  },
    { -13,-3,3,13 }, { -19,-4,4,19 }, { -28,-6,6,28 } };
	static const short curve[256] =
	{ 0,1,2,3,4,5,6,7,8,9,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
    28,29,30,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,53,
    54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,74,75,76,77,78,
    79,80,81,82,83,84,86,88,90,92,94,97,99,101,103,105,107,110,112,114,116,
    118,120,123,125,127,129,131,134,136,138,140,142,144,147,149,151,153,155,
    158,160,162,164,166,168,171,173,175,177,179,181,184,186,188,190,192,195,
    197,199,201,203,205,208,210,212,214,216,218,221,223,226,230,235,239,244,
    248,252,257,261,265,270,274,278,283,287,291,296,300,305,309,313,318,322,
    326,331,335,339,344,348,352,357,361,365,370,374,379,383,387,392,396,400,
    405,409,413,418,422,426,431,435,440,444,448,453,457,461,466,470,474,479,
    483,487,492,496,500,508,519,531,542,553,564,575,587,598,609,620,631,643,
    654,665,676,687,698,710,721,732,743,754,766,777,788,799,810,822,833,844,
    855,866,878,889,900,911,922,933,945,956,967,978,989,1001,1012,1023 };
	int rb, row, col, sharp, val=0;

	dcr_getbits(p, -1);
	memset (pixel, 0x80, sizeof pixel);
	for (row=2; row < p->height+2; row++) {
		for (col=2+(row & 1); col < p->width+2; col+=2) {
			val = ((pixel[row-1][col-1] + 2*pixel[row-1][col+1] +
				pixel[row][col-2]) >> 2) + gstep[dcr_getbits(p, 4)];
			pixel[row][col] = val = LIM(val,0,255);
			if (col < 4)
				pixel[row][col-2] = pixel[row+1][~row & 1] = val;
			if (row == 2)
				pixel[row-1][col+1] = pixel[row-1][col+3] = val;
		}
		pixel[row][col] = val;
	}
	for (rb=0; rb < 2; rb++)
		for (row=2+rb; row < p->height+2; row+=2)
			for (col=3-(row & 1); col < p->width+2; col+=2) {
				if (row < 4 || col < 4) sharp = 2;
				else {
					val = ABS(pixel[row-2][col] - pixel[row][col-2])
						+ ABS(pixel[row-2][col] - pixel[row-2][col-2])
						+ ABS(pixel[row][col-2] - pixel[row-2][col-2]);
					sharp = val <  4 ? 0 : val <  8 ? 1 : val < 16 ? 2 :
					val < 32 ? 3 : val < 48 ? 4 : 5;
				}
				val = ((pixel[row-2][col] + pixel[row][col-2]) >> 1)
					+ rstep[sharp][dcr_getbits(p, 2)];
				pixel[row][col] = val = LIM(val,0,255);
				if (row < 4) pixel[row-2][col+2] = val;
				if (col < 4) pixel[row+2][col-2] = val;
			}
	for (row=2; row < p->height+2; row++)
		for (col=3-(row & 1); col < p->width+2; col+=2) {
			val = ((pixel[row][col-1] + (pixel[row][col] << 2) +
				pixel[row][col+1]) >> 1) - 0x100;
			pixel[row][col] = LIM(val,0,255);
		}
	for (row=0; row < p->height; row++)
		for (col=0; col < p->width; col++)
			BAYER(row,col) = curve[pixel[row+2][col+2]];
	p->maximum = 0x3ff;
}

const int * DCR_CLASS dcr_make_decoder_int (DCRAW* p, const int *source, int level)
{
	struct dcr_decode *cur;

	cur = p->free_decode++;
	if (level < source[0]) {
		cur->branch[0] = p->free_decode;
		source = dcr_make_decoder_int (p, source, level+1);
		cur->branch[1] = p->free_decode;
		source = dcr_make_decoder_int (p, source, level+1);
	} else {
		cur->leaf = source[1];
		source += 2;
	}
	return source;
}

int DCR_CLASS dcr_radc_token (DCRAW* p, int tree)
{
	int t;
	static const int *s, source[] = {
		1,1, 2,3, 3,4, 4,2, 5,7, 6,5, 7,6, 7,8,
		1,0, 2,1, 3,3, 4,4, 5,2, 6,7, 7,6, 8,5, 8,8,
		2,1, 2,3, 3,0, 3,2, 3,4, 4,6, 5,5, 6,7, 6,8,
		2,0, 2,1, 2,3, 3,2, 4,4, 5,6, 6,7, 7,5, 7,8,
		2,1, 2,4, 3,0, 3,2, 3,3, 4,7, 5,5, 6,6, 6,8,
		2,3, 3,1, 3,2, 3,4, 3,5, 3,6, 4,7, 5,0, 5,8,
		2,3, 2,6, 3,0, 3,1, 4,4, 4,5, 4,7, 5,2, 5,8,
		2,4, 2,7, 3,3, 3,6, 4,1, 4,2, 4,5, 5,0, 5,8,
		2,6, 3,1, 3,3, 3,5, 3,7, 3,8, 4,0, 5,2, 5,4,
		2,0, 2,1, 3,2, 3,3, 4,4, 4,5, 5,6, 5,7, 4,8,
		1,0, 2,2, 2,-2,
		1,-3, 1,3,
		2,-17, 2,-5, 2,5, 2,17,
		2,-7, 2,2, 2,9, 2,18,
		2,-18, 2,-9, 2,-2, 2,7,
		2,-28, 2,28, 3,-49, 3,-9, 3,9, 4,49, 5,-79, 5,79,
		2,-1, 2,13, 2,26, 3,39, 4,-16, 5,55, 6,-37, 6,76,
		2,-26, 2,-13, 2,1, 3,-39, 4,16, 5,-55, 6,-76, 6,37
	};

	if (p->free_decode == p->first_decode)
		for (s=source, t=0; t < 18; t++) {
			p->radc_token_dstart[t] = p->free_decode;
			s = dcr_make_decoder_int (p, s, 0);
		}
	if (tree == 18) {
		if (p->kodak_cbpp == 243)
			return (dcr_getbits(p, 6) << 2) + 2;	/* most DC50 photos */
		else
			return (dcr_getbits(p, 5) << 3) + 4;	/* DC40, Fotoman Pixtura */
	}
	for (p->radc_token_dindex = p->radc_token_dstart[tree]; p->radc_token_dindex->branch[0]; )
		p->radc_token_dindex = p->radc_token_dindex->branch[dcr_getbits(p, 1)];
	return p->radc_token_dindex->leaf;
}

#define FORYX for (y=1; y < 3; y++) for (x=col+1; x >= col; x--)

#define PREDICTOR (c ? (buf[c][y-1][x] + buf[c][y][x+1]) / 2 \
: (buf[c][y-1][x+1] + 2*buf[c][y-1][x] + buf[c][y][x+1]) / 4)

void DCR_CLASS dcr_kodak_radc_load_raw(DCRAW* p)
{
	int row, col, tree, nreps, rep, step, i, c, s, r, x, y, val;
	short last[3] = { 16,16,16 }, mul[3], buf[3][3][386];

	dcr_init_decoder(p);
	dcr_getbits(p, -1);
	for (i=0; i < sizeof(buf)/sizeof(short); i++)
		buf[0][0][i] = 2048;
	for (row=0; row < p->height; row+=4) {
		FORC3 mul[c] = dcr_getbits(p, 6);
		FORC3 {
			val = ((0x1000000/last[c] + 0x7ff) >> 12) * mul[c];
			s = val > 65564 ? 10:12;
			x = ~(-1 << (s-1));
			val <<= 12-s;
			for (i=0; i < sizeof(buf[0])/sizeof(short); i++)
				buf[c][0][i] = (buf[c][0][i] * val + x) >> s;
			last[c] = mul[c];
			for (r=0; r <= !c; r++) {
				buf[c][1][p->width/2] = buf[c][2][p->width/2] = mul[c] << 7;
				for (tree=1, col=p->width/2; col > 0; ) {
					if ((tree = dcr_radc_token(p,tree))) {
						col -= 2;
						if (tree == 8)
							FORYX buf[c][y][x] = dcr_radc_token(p,tree+10) * mul[c];
						else
							FORYX buf[c][y][x] = dcr_radc_token(p,tree+10) * 16 + PREDICTOR;
					} else
						do {
							nreps = (col > 2) ? dcr_radc_token(p,9) + 1 : 1;
							for (rep=0; rep < 8 && rep < nreps && col > 0; rep++) {
								col -= 2;
								FORYX buf[c][y][x] = PREDICTOR;
								if (rep & 1) {
									step = dcr_radc_token(p,10) << 4;
									FORYX buf[c][y][x] += step;
								}
							}
						} while (nreps == 9);
				}
				for (y=0; y < 2; y++)
					for (x=0; x < p->width/2; x++) {
						val = (buf[c][y+1][x] << 4) / mul[c];
						if (val < 0) val = 0;
						if (c) BAYER(row+y*2+c-1,x*2+2-c) = val;
						else   BAYER(row+r*2+y,x*2+y) = val;
					}
					memcpy (buf[c][0]+!c, buf[c][2], sizeof buf[c][0]-2*!c);
			}
		}
		for (y=row; y < row+4; y++)
			for (x=0; x < p->width; x++)
				if ((x+y) & 1) {
					r = x ? x-1 : x+1;
					s = x+1 < p->width ? x+1 : x-1;
					val = (BAYER(y,x)-2048)*2 + (BAYER(y,r)+BAYER(y,s))/2;
					if (val < 0) val = 0;
					BAYER(y,x) = val;
				}
	}
	p->maximum = 0xfff;
	p->use_gamma = 0;
}

#undef FORYX
#undef PREDICTOR

#ifdef NO_JPEG
void DCR_CLASS dcr_kodak_jpeg_load_raw(DCRAW* p) {}
#else

METHODDEF(boolean)
fill_input_buffer (j_decompress_ptr cinfo)
{
	static uchar jpeg_buffer[4096]; // NOTE: This static not used as NO_JPEG is defined
	size_t nbytes;

	//nbytes = dcr_fread(p->obj_, jpeg_buffer, 1, 4096);
	nbytes = fread (jpeg_buffer, 1, 4096, ifp);
	_swab (jpeg_buffer, jpeg_buffer, nbytes);
	cinfo->src->next_input_byte = jpeg_buffer;
	cinfo->src->bytes_in_buffer = nbytes;
	return TRUE;
}

void DCR_CLASS dcr_kodak_jpeg_load_raw(DCRAW* p)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPARRAY buf;
	JSAMPLE (*pixel)[3];
	int row, col;

	cinfo.err = jpeg_std_error (&jerr);
	jpeg_create_decompress (&cinfo);
	jpeg_stdio_src (&cinfo);
	cinfo.src->fill_input_buffer = fill_input_buffer;
	jpeg_read_header (&cinfo, TRUE);
	jpeg_start_decompress (&cinfo);
	if ((cinfo.output_width      != p->width  ) ||
		(cinfo.output_height*2   != p->height ) ||
		(cinfo.output_components != 3      )) {
		fprintf (stderr,_("%s: incorrect JPEG dimensions\n"), p->ifname);
		jpeg_destroy_decompress (&cinfo);
		longjmp (p->failure, 3);
	}
	buf = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, p->width*3, 1);

	while (cinfo.output_scanline < cinfo.output_height) {
		row = cinfo.output_scanline * 2;
		jpeg_read_scanlines (&cinfo, buf, 1);
		pixel = (JSAMPLE (*)[3]) buf[0];
		for (col=0; col < p->width; col+=2) {
			BAYER(row+0,col+0) = pixel[col+0][1] << 1;
			BAYER(row+1,col+1) = pixel[col+1][1] << 1;
			BAYER(row+0,col+1) = pixel[col][0] + pixel[col+1][0];
			BAYER(row+1,col+0) = pixel[col][2] + pixel[col+1][2];
		}
	}
	jpeg_finish_decompress (&cinfo);
	jpeg_destroy_decompress (&cinfo);
	p->maximum = 0xff << 1;
}
#endif

void DCR_CLASS dcr_kodak_dc120_load_raw(DCRAW* p)
{
	static const int mul[4] = { 162, 192, 187,  92 };
	static const int add[4] = {   0, 636, 424, 212 };
	uchar pixel[848];
	int row, shift, col;

	for (row=0; row < p->height; row++) {
		if (dcr_fread(p->obj_, pixel, 1, 848) < 848) dcr_derror(p);
		shift = row * mul[row & 3] + add[row & 3];
		for (col=0; col < p->width; col++)
			BAYER(row,col) = (ushort) pixel[(col + shift) % 848];
	}
	p->maximum = 0xff;
}

void DCR_CLASS dcr_eight_bit_load_raw(DCRAW* p)
{
	uchar *pixel;
	unsigned row, col, val, lblack=0;

	pixel = (uchar *) calloc (p->raw_width, sizeof *pixel);
	dcr_merror (p, pixel, "eight_bit_load_raw()");
	dcr_fseek(p->obj_, p->top_margin*p->raw_width, SEEK_CUR);
	for (row=0; row < p->height; row++) {
		if (dcr_fread(p->obj_, pixel, 1, p->raw_width) < p->raw_width) dcr_derror(p);
		for (col=0; col < p->raw_width; col++) {
			val = p->curve[pixel[col]];
			if ((unsigned) (col-p->left_margin) < p->width)
				BAYER(row,col-p->left_margin) = val;
			else lblack += val;
		}
	}
	free (pixel);
	if (p->raw_width > p->width+1)
		p->black = lblack / ((p->raw_width - p->width) * p->height);
	if (!strncmp(p->model,"DC2",3))
		p->black = 0;
	p->maximum = p->curve[0xff];
}

void DCR_CLASS dcr_kodak_yrgb_load_raw(DCRAW* p)
{
  uchar *pixel;
  int row, col, y, cb, cr, rgb[3], c;

  pixel = (uchar *) calloc (p->raw_width, 3*sizeof *pixel);
  dcr_merror (p, pixel, "kodak_yrgb_load_raw()");
  for (row=0; row < p->height; row++) {
    if (~row & 1)
      if (dcr_fread(p->obj_, pixel, p->raw_width, 3) < 3) dcr_derror(p);
    for (col=0; col < p->raw_width; col++) {
      y  = pixel[p->width*2*(row & 1) + col];
      cb = pixel[p->width + (col & -2)]   - 128;
      cr = pixel[p->width + (col & -2)+1] - 128;
      rgb[1] = y-((cb + cr + 2) >> 2);
      rgb[2] = rgb[1] + cb;
      rgb[0] = rgb[1] + cr;
      FORC3 p->image[row*p->width+col][c] = LIM(rgb[c],0,255);
    }
  }
  free (pixel);
  p->use_gamma = 0;
}

void DCR_CLASS dcr_kodak_262_load_raw(DCRAW* p)
{
	static const uchar kodak_tree[2][26] =
	{ { 0,1,5,1,1,2,0,0,0,0,0,0,0,0,0,0, 0,1,2,3,4,5,6,7,8,9 },
    { 0,3,1,1,1,1,1,2,0,0,0,0,0,0,0,0, 0,1,2,3,4,5,6,7,8,9 } };
	struct dcr_decode *decode[2];
	uchar *pixel;
	int *strip, ns, i, row, col, chess, pi=0, pi1, pi2, pred, val;

	dcr_init_decoder(p);
	for (i=0; i < 2; i++) {
		decode[i] = p->free_decode;
		dcr_make_decoder (p, kodak_tree[i], 0);
	}
	ns = (p->raw_height+63) >> 5;
	pixel = (uchar *) malloc (p->raw_width*32 + ns*4);
	dcr_merror (p, pixel, "kodak_262_load_raw()");
	strip = (int *) (pixel + p->raw_width*32);
	p->order = 0x4d4d;
	for (i=0; i < ns; i++)
		strip[i] = dcr_get4(p);
	for (row=0; row < p->raw_height; row++) {
		if ((row & 31) == 0) {
			dcr_fseek(p->obj_, strip[row >> 5], SEEK_SET);
			dcr_getbits(p, -1);
			pi = 0;
		}
		for (col=0; col < p->raw_width; col++) {
			chess = (row + col) & 1;
			pi1 = chess ? pi-2           : pi-p->raw_width-1;
			pi2 = chess ? pi-2*p->raw_width : pi-p->raw_width+1;
			if (col <= chess) pi1 = -1;
			if (pi1 < 0) pi1 = pi2;
			if (pi2 < 0) pi2 = pi1;
			if (pi1 < 0 && col > 1) pi1 = pi2 = pi-2;
			pred = (pi1 < 0) ? 0 : (pixel[pi1] + pixel[pi2]) >> 1;
			pixel[pi] = val = pred + dcr_ljpeg_diff (p, decode[chess]);
			if (val >> 8) dcr_derror(p);
			val = p->curve[pixel[pi++]];
			if ((unsigned) (col-p->left_margin) < p->width)
				BAYER(row,col-p->left_margin) = val;
			else p->black += val;
		}
	}
	free (pixel);
	if (p->raw_width > p->width)
		p->black /= (p->raw_width - p->width) * p->height;
}

int DCR_CLASS dcr_kodak_65000_decode (DCRAW* p, short *out, int bsize)
{
	uchar c, blen[768];
	ushort raw[6];
	INT64 bitbuf=0;
	int save, bits=0, i, j, len, diff;

	save = dcr_ftell(p->obj_);
	bsize = (bsize + 3) & -4;
	for (i=0; i < bsize; i+=2) {
		c = dcr_fgetc(p->obj_);
		if ((blen[i  ] = c & 15) > 12 ||
			(blen[i+1] = c >> 4) > 12 ) {
			dcr_fseek(p->obj_, save, SEEK_SET);
			for (i=0; i < bsize; i+=8) {
				dcr_read_shorts (p, raw, 6);
				out[i  ] = raw[0] >> 12 << 8 | raw[2] >> 12 << 4 | raw[4] >> 12;
				out[i+1] = raw[1] >> 12 << 8 | raw[3] >> 12 << 4 | raw[5] >> 12;
				for (j=0; j < 6; j++)
					out[i+2+j] = raw[j] & 0xfff;
			}
			return 1;
		}
	}
	if ((bsize & 7) == 4) {
		bitbuf  = dcr_fgetc(p->obj_) << 8;
		bitbuf += dcr_fgetc(p->obj_);
		bits = 16;
	}
	for (i=0; i < bsize; i++) {
		len = blen[i];
		if (bits < len) {
			for (j=0; j < 32; j+=8)
				bitbuf += (INT64) dcr_fgetc(p->obj_) << (bits+(j^8));
			bits += 32;
		}
		diff = (int)(bitbuf & (0xffff >> (16-len)));
		bitbuf >>= len;
		bits -= len;
		if ((diff & (1 << (len-1))) == 0)
			diff -= (1 << len) - 1;
		out[i] = diff;
	}
	return 0;
}

void DCR_CLASS dcr_kodak_65000_load_raw(DCRAW* p)
{
	short buf[256];
	int row, col, len, pred[2], ret, i;

	for (row=0; row < p->height; row++)
		for (col=0; col < p->width; col+=256) {
			pred[0] = pred[1] = 0;
			len = MIN (256, p->width-col);
			ret = dcr_kodak_65000_decode (p, buf, len);
			for (i=0; i < len; i++)
				if ((BAYER(row,col+i) =	p->curve[ret ? buf[i] :
				(pred[i & 1] += buf[i])]) >> 12) dcr_derror(p);
		}
}

void DCR_CLASS dcr_kodak_ycbcr_load_raw(DCRAW* p)
{
	short buf[384], *bp;
	int row, col, len, c, i, j, k, y[2][2], cb, cr, rgb[3];
	ushort *ip;

	for (row=0; row < p->height; row+=2)
		for (col=0; col < p->width; col+=128) {
			len = MIN (128, p->width-col);
			dcr_kodak_65000_decode (p, buf, len*3);
			y[0][1] = y[1][1] = cb = cr = 0;
			for (bp=buf, i=0; i < len; i+=2, bp+=2) {
				cb += bp[4];
				cr += bp[5];
				rgb[1] = -((cb + cr + 2) >> 2);
				rgb[2] = rgb[1] + cb;
				rgb[0] = rgb[1] + cr;
				for (j=0; j < 2; j++)
					for (k=0; k < 2; k++) {
						if ((y[j][k] = y[j][k^1] + *bp++) >> 10) dcr_derror(p);
						ip = p->image[(row+j)*p->width + col+i+k];
						FORC3 ip[c] = p->curve[LIM(y[j][k]+rgb[c], 0, 0xfff)];
					}
			}
		}
}

void DCR_CLASS dcr_kodak_rgb_load_raw(DCRAW* p)
{
	short buf[768], *bp;
	int row, col, len, c, i, rgb[3];
	ushort *ip=p->image[0];

	for (row=0; row < p->height; row++)
		for (col=0; col < p->width; col+=256) {
			len = MIN (256, p->width-col);
			dcr_kodak_65000_decode (p, buf, len*3);
			memset (rgb, 0, sizeof rgb);
			for (bp=buf, i=0; i < len; i++, ip+=4)
				FORC3 if ((ip[c] = rgb[c] += *bp++) >> 12) dcr_derror(p);
		}
}

void DCR_CLASS dcr_kodak_thumb_load_raw(DCRAW* p)
{
	int row, col;
	p->colors = p->thumb_misc >> 5;
	for (row=0; row < p->height; row++)
		for (col=0; col < p->width; col++)
			dcr_read_shorts (p, p->image[row*p->width+col], p->colors);
		p->maximum = (1 << (p->thumb_misc & 31)) - 1;
}

void DCR_CLASS dcr_sony_decrypt (DCRAW *p, unsigned *data, int len, int start, int key)
{
	if (start) {
		for (p->sony_decrypt_p=0; p->sony_decrypt_p < 4; p->sony_decrypt_p++)
			p->sony_decrypt_pad[p->sony_decrypt_p] = key = key * 48828125 + 1;
		p->sony_decrypt_pad[3] = p->sony_decrypt_pad[3] << 1 | (p->sony_decrypt_pad[0]^p->sony_decrypt_pad[2]) >> 31;
		for (p->sony_decrypt_p=4; p->sony_decrypt_p < 127; p->sony_decrypt_p++)
			p->sony_decrypt_pad[p->sony_decrypt_p] = (p->sony_decrypt_pad[p->sony_decrypt_p-4]^p->sony_decrypt_pad[p->sony_decrypt_p-2]) << 1 | (p->sony_decrypt_pad[p->sony_decrypt_p-3]^p->sony_decrypt_pad[p->sony_decrypt_p-1]) >> 31;
		for (p->sony_decrypt_p=0; p->sony_decrypt_p < 127; p->sony_decrypt_p++)
			p->sony_decrypt_pad[p->sony_decrypt_p] = htonl(p->sony_decrypt_pad[p->sony_decrypt_p]);
	}
	while (len--)
		*data++ ^= p->sony_decrypt_pad[p->sony_decrypt_p++ & 127] = p->sony_decrypt_pad[(p->sony_decrypt_p+1) & 127] ^ p->sony_decrypt_pad[(p->sony_decrypt_p+65) & 127];
}

void DCR_CLASS dcr_sony_load_raw(DCRAW* p)
{
	uchar head[40];
	ushort *pixel;
	unsigned i, key, row, col;

	dcr_fseek(p->obj_, 200896, SEEK_SET);
	dcr_fseek(p->obj_, (unsigned) dcr_fgetc(p->obj_)*4 - 1, SEEK_CUR);
	p->order = 0x4d4d;
	key = dcr_get4(p);
	dcr_fseek(p->obj_, 164600, SEEK_SET);
	dcr_fread(p->obj_, head, 1, 40);
	dcr_sony_decrypt (p, (unsigned int *) head, 10, 1, key);
	for (i=26; i-- > 22; )
		key = key << 8 | head[i];
	dcr_fseek(p->obj_, p->data_offset, SEEK_SET);
	pixel = (ushort *) calloc (p->raw_width, sizeof *pixel);
	dcr_merror (p, pixel, "sony_load_raw()");
	for (row=0; row < p->height; row++) {
		if (dcr_fread(p->obj_, pixel, 2, p->raw_width) < p->raw_width) dcr_derror(p);
		dcr_sony_decrypt (p, (unsigned int *) pixel, p->raw_width/2, !row, key);
		for (col=9; col < p->left_margin; col++)
			p->black += ntohs(pixel[col]);
		for (col=0; col < p->width; col++)
			if ((BAYER(row,col) = ntohs(pixel[col+p->left_margin])) >> 14)
				dcr_derror(p);
	}
	free (pixel);
	if (p->left_margin > 9)
		p->black /= (p->left_margin-9) * p->height;
	p->maximum = 0x3ff0;
}

void DCR_CLASS dcr_sony_arw_load_raw(DCRAW* p)
{
	int col, row, len, diff, sum=0;

	dcr_getbits(p, -1);
	for (col = p->raw_width; col--; )
		for (row=0; row < p->raw_height+1; row+=2) {
			if (row == p->raw_height) row = 1;
			len = 4 - dcr_getbits(p, 2);
			if (len == 3 && dcr_getbits(p, 1)) len = 0;
			if (len == 4)
				while (len < 17 && !dcr_getbits(p, 1)) len++;
				diff = dcr_getbits(p, len);
				if ((diff & (1 << (len-1))) == 0)
					diff -= (1 << len) - 1;
				if ((sum += diff) >> 12) dcr_derror(p);
				if (row < p->height) BAYER(row,col) = sum;
		}
}

void DCR_CLASS dcr_sony_arw2_load_raw(DCRAW* p)
{
	uchar *data, *dp;
	ushort pix[16];
	int row, col, val, max, min, imax, imin, sh, bit, i;

	data = (uchar *) malloc (p->raw_width*p->tiff_bps >> 3);
	dcr_merror (p, data, "sony_arw2_load_raw()");
	for (row=0; row < p->height; row++) {
		dcr_fread(p->obj_, data, 1, p->raw_width*p->tiff_bps >> 3);
		if (p->tiff_bps == 8) {
			for (dp=data, col=0; col < p->width-30; dp+=16) {
				max = 0x7ff & (val = dcr_sget4(p, dp));
				min = 0x7ff & val >> 11;
				imax = 0x0f & val >> 22;
				imin = 0x0f & val >> 26;
				for (sh=0; sh < 4 && 0x80 << sh <= max-min; sh++);
				for (bit=30, i=0; i < 16; i++)
					if      (i == imax) pix[i] = max;
					else if (i == imin) pix[i] = min;
					else {
						pix[i] = ((dcr_sget2(p, dp+(bit >> 3)) >> (bit & 7) & 0x7f) << sh) + min;
						if (pix[i] > 0x7ff) pix[i] = 0x7ff;
						bit += 7;
					}
					for (i=0; i < 16; i++, col+=2)
						BAYER(row,col) = p->curve[pix[i] << 1] >> 1;
					col -= col & 1 ? 1:31;
			}
		} else if (p->tiff_bps == 12)
			for (dp=data, col=0; col < p->width; dp+=3, col+=2) {
				BAYER(row,col)   = ((dp[1] << 8 | dp[0]) & 0xfff) << 1;
				BAYER(row,col+1) =  (dp[2] << 4 | dp[1] >> 4) << 1;
			}
	}
	free (data);
}

#define HOLE(row) ((holes >> (((row) - p->raw_height) & 7)) & 1)

/* Kudos to Rich Taylor for figuring out SMaL's compression algorithm. */
void DCR_CLASS dcr_smal_decode_segment (DCRAW* p, unsigned seg[2][2], int holes)
{
	uchar hist[3][13] = {
		{ 7, 7, 0, 0, 63, 55, 47, 39, 31, 23, 15, 7, 0 },
		{ 7, 7, 0, 0, 63, 55, 47, 39, 31, 23, 15, 7, 0 },
		{ 3, 3, 0, 0, 63,     47,     31,     15,    0 } };
	int low, high=0xff, carry=0, nbits=8;
	int s, count, bin, next, i, sym[3];
	uchar diff, pred[]={0,0};
	ushort data=0, range=0;
	unsigned pix, row, col;

	dcr_fseek(p->obj_, seg[0][1]+1, SEEK_SET);
	dcr_getbits(p, -1);
	for (pix=seg[0][0]; pix < seg[1][0]; pix++) {
		for (s=0; s < 3; s++) {
			data = data << nbits | dcr_getbits(p, nbits);
			if (carry < 0)
				carry = (nbits += carry+1) < 1 ? nbits-1 : 0;
			while (--nbits >= 0)
				if ((data >> nbits & 0xff) == 0xff) break;
				if (nbits > 0)
					data = ((data & ((1 << (nbits-1)) - 1)) << 1) |
					((data + (((data & (1 << (nbits-1)))) << 1)) & (-1 << nbits));
				if (nbits >= 0) {
					data += dcr_getbits(p, 1);
					carry = nbits - 8;
				}
				count = ((((data-range+1) & 0xffff) << 2) - 1) / (high >> 4);
				for (bin=0; hist[s][bin+5] > count; bin++);
				low = hist[s][bin+5] * (high >> 4) >> 2;
				if (bin) high = hist[s][bin+4] * (high >> 4) >> 2;
				high -= low;
				for (nbits=0; high << nbits < 128; nbits++);
				range = (range+low) << nbits;
				high <<= nbits;
				next = hist[s][1];
				if (++hist[s][2] > hist[s][3]) {
					next = (next+1) & hist[s][0];
					hist[s][3] = (hist[s][next+4] - hist[s][next+5]) >> 2;
					hist[s][2] = 1;
				}
				if (hist[s][hist[s][1]+4] - hist[s][hist[s][1]+5] > 1) {
					if (bin < hist[s][1])
						for (i=bin; i < hist[s][1]; i++) hist[s][i+5]--;
						else if (next <= bin)
							for (i=hist[s][1]; i < bin; i++) hist[s][i+5]++;
				}
				hist[s][1] = next;
				sym[s] = bin;
		}
		diff = sym[2] << 5 | sym[1] << 2 | (sym[0] & 3);
		if (sym[0] & 4)
			diff = diff ? -diff : 0x80;
		if (dcr_ftell(p->obj_) + 12 >= (long)seg[1][1])
			diff = 0;
		pred[pix & 1] += diff;
		row = pix / p->raw_width - p->top_margin;
		col = pix % p->raw_width - p->left_margin;
		if (row < p->height && col < p->width)
			BAYER(row,col) = pred[pix & 1];
		if (!(pix & 1) && HOLE(row)) pix += 2;
	}
	p->maximum = 0xff;
}

void DCR_CLASS dcr_smal_v6_load_raw(DCRAW* p)
{
	unsigned seg[2][2];

	dcr_fseek(p->obj_, 16, SEEK_SET);
	seg[0][0] = 0;
	seg[0][1] = dcr_get2(p);
	seg[1][0] = p->raw_width * p->raw_height;
	seg[1][1] = INT_MAX;
	dcr_smal_decode_segment (p,seg, 0);
	p->use_gamma = 0;
}

int DCR_CLASS dcr_median4 (int *p)
{
	int min, max, sum, i;

	min = max = sum = p[0];
	for (i=1; i < 4; i++) {
		sum += p[i];
		if (min > p[i]) min = p[i];
		if (max < p[i]) max = p[i];
	}
	return (sum - min - max) >> 1;
}

void DCR_CLASS dcr_fill_holes (DCRAW* p, int holes)
{
	int row, col, val[4];

	for (row=2; row < p->height-2; row++) {
		if (!HOLE(row)) continue;
		for (col=1; col < p->width-1; col+=4) {
			val[0] = BAYER(row-1,col-1);
			val[1] = BAYER(row-1,col+1);
			val[2] = BAYER(row+1,col-1);
			val[3] = BAYER(row+1,col+1);
			BAYER(row,col) = dcr_median4(val);
		}
		for (col=2; col < p->width-2; col+=4)
			if (HOLE(row-2) || HOLE(row+2))
				BAYER(row,col) = (BAYER(row,col-2) + BAYER(row,col+2)) >> 1;
			else {
				val[0] = BAYER(row,col-2);
				val[1] = BAYER(row,col+2);
				val[2] = BAYER(row-2,col);
				val[3] = BAYER(row+2,col);
				BAYER(row,col) = dcr_median4(val);
			}
	}
}

void DCR_CLASS dcr_smal_v9_load_raw(DCRAW* p)
{
	unsigned seg[256][2], offset, nseg, holes, i;

	dcr_fseek(p->obj_, 67, SEEK_SET);
	offset = dcr_get4(p);
	nseg = dcr_fgetc(p->obj_);
	dcr_fseek(p->obj_, offset, SEEK_SET);
	for (i=0; i < nseg*2; i++)
		seg[0][i] = dcr_get4(p) + p->data_offset*(i & 1);
	dcr_fseek(p->obj_, 78, SEEK_SET);
	holes = dcr_fgetc(p->obj_);
	dcr_fseek(p->obj_, 88, SEEK_SET);
	seg[nseg][0] = p->raw_height * p->raw_width;
	seg[nseg][1] = dcr_get4(p) + p->data_offset;
	for (i=0; i < nseg; i++)
		dcr_smal_decode_segment (p,seg+i, holes);
	if (holes) dcr_fill_holes (p,holes);
}

/* RESTRICTED code starts here */
#if RESTRICTED

void DCR_CLASS dcr_foveon_decoder (DCRAW* p, unsigned size, unsigned code)
{
	struct dcr_decode *cur;
	int i, len;

	if (!code) {
		for (i=0; i < (int)size; i++)
			p->foveon_decoder_huff[i] = dcr_get4(p);
		dcr_init_decoder(p);
	}
	cur = p->free_decode++;
	if (p->free_decode > p->first_decode+2048) {
		fprintf (stderr,_("%s: decoder table overflow\n"), p->ifname);
		longjmp (p->failure, 2);
	}
	if (code)
		for (i=0; i < (int)size; i++)
			if (p->foveon_decoder_huff[i] == code) {
				cur->leaf = i;
				return;
			}
	if ((len = code >> 27) > 26) return;
	code = (len+1) << 27 | (code & 0x3ffffff) << 1;

	cur->branch[0] = p->free_decode;
	dcr_foveon_decoder (p,size, code);
	cur->branch[1] = p->free_decode;
	dcr_foveon_decoder (p,size, code+1);
}

void DCR_CLASS dcr_foveon_thumb (DCRAW* p, FILE *tfp)
{
	unsigned bwide, row, col, bitbuf=0, bit=1, c, i;
	char *buf;
	struct dcr_decode *dindex;
	short pred[3];

	bwide = dcr_get4(p);
	fprintf (tfp, "P6\n%d %d\n255\n", p->thumb_width, p->thumb_height);
	if (bwide > 0) {
		if (bwide < (unsigned int)(p->thumb_width*3)) return;
		buf = (char *) malloc (bwide);
		dcr_merror (p, buf, "foveon_thumb()");
		for (row=0; row < p->thumb_height; row++) {
			dcr_fread(p->obj_, buf, 1, bwide);
			fwrite (buf, 3, p->thumb_width, tfp);
		}
		free (buf);
		return;
	}
	dcr_foveon_decoder (p,256, 0);

	for (row=0; row < p->thumb_height; row++) {
		memset (pred, 0, sizeof pred);
		if (!bit) dcr_get4(p);
		for (bit=col=0; col < p->thumb_width; col++)
			FORC3 {
			for (dindex=p->first_decode; dindex->branch[0]; ) {
				if ((bit = (bit-1) & 31) == 31)
					for (i=0; i < 4; i++)
						bitbuf = (bitbuf << 8) + dcr_fgetc(p->obj_);
					dindex = dindex->branch[bitbuf >> bit & 1];
			}
			pred[c] += dindex->leaf;
			fputc (pred[c], tfp);
		}
	}
}

void DCR_CLASS dcr_foveon_load_camf(DCRAW* p)
{
	unsigned key, i, val;

	dcr_fseek(p->obj_, p->meta_offset, SEEK_SET);
	key = dcr_get4(p);
	dcr_fread(p->obj_, p->meta_data, 1, p->meta_length);
	for (i=0; i < p->meta_length; i++) {
		key = (key * 1597 + 51749) % 244944;
		val = (unsigned int)(key * (INT64) 301593171 >> 24);
		p->meta_data[i] ^= ((((key << 8) - val) >> 1) + val) >> 17;
	}
}

void DCR_CLASS dcr_foveon_load_raw(DCRAW* p)
{
	struct dcr_decode *dindex;
	short diff[1024];
	unsigned bitbuf=0;
	int pred[3], fixed, row, col, bit=-1, c, i;

	fixed = dcr_get4(p);
	dcr_read_shorts (p, (ushort *) diff, 1024);
	if (!fixed) dcr_foveon_decoder (p, 1024, 0);

	for (row=0; row < p->height; row++) {
		memset (pred, 0, sizeof pred);
		if (!bit && !fixed && atoi(p->model+2) < 14) dcr_get4(p);
		for (col=bit=0; col < p->width; col++) {
			if (fixed) {
				bitbuf = dcr_get4(p);
				FORC3 pred[2-c] += diff[bitbuf >> c*10 & 0x3ff];
			}
			else FORC3 {
				for (dindex=p->first_decode; dindex->branch[0]; ) {
					if ((bit = (bit-1) & 31) == 31)
						for (i=0; i < 4; i++)
							bitbuf = (bitbuf << 8) + dcr_fgetc(p->obj_);
						dindex = dindex->branch[bitbuf >> bit & 1];
				}
				pred[c] += diff[dindex->leaf];
				if (pred[c] >> 16 && ~pred[c] >> 16) dcr_derror(p);
			}
			FORC3 p->image[row*p->width+col][c] = pred[c];
		}
	}
	if (p->opt.document_mode)
		for (i=0; i < p->height*p->width*4; i++)
			if ((short) p->image[0][i] < 0) p->image[0][i] = 0;
			dcr_foveon_load_camf(p);
}

const char * DCR_CLASS dcr_foveon_camf_param (DCRAW* p, const char *block, const char *param)
{
	unsigned idx, num;
	char *pos, *cp, *dp;

	for (idx=0; idx < p->meta_length; idx += dcr_sget4(p, pos+8)) {
		pos = p->meta_data + idx;
		if (strncmp (pos, "CMb", 3)) break;
		if (pos[3] != 'P') continue;
		if (strcmp (block, pos+dcr_sget4(p, pos+12))) continue;
		cp = pos + dcr_sget4(p, pos+16);
		num = dcr_sget4(p, cp);
		dp = pos + dcr_sget4(p, cp+4);
		while (num--) {
			cp += 8;
			if (!strcmp (param, dp+dcr_sget4(p, cp)))
				return dp+dcr_sget4(p, cp+4);
		}
	}
	return 0;
}

void * DCR_CLASS dcr_foveon_camf_matrix (DCRAW* p, unsigned dim[3], const char *name)
{
	unsigned i, idx, type, ndim, size, *mat;
	char *pos, *cp, *dp;
	double dsize;

	for (idx=0; idx < p->meta_length; idx += dcr_sget4(p, pos+8)) {
		pos = p->meta_data + idx;
		if (strncmp (pos, "CMb", 3)) break;
		if (pos[3] != 'M') continue;
		if (strcmp (name, pos+dcr_sget4(p, pos+12))) continue;
		dim[0] = dim[1] = dim[2] = 1;
		cp = pos + dcr_sget4(p, pos+16);
		type = dcr_sget4(p, cp);
		if ((ndim = dcr_sget4(p, cp+4)) > 3) break;
		dp = pos + dcr_sget4(p, cp+8);
		for (i=ndim; i--; ) {
			cp += 12;
			dim[i] = dcr_sget4(p, cp);
		}
		if ((dsize = (double) dim[0]*dim[1]*dim[2]) > p->meta_length/4) break;
		mat = (unsigned *) malloc ((size = (unsigned int)dsize) * 4);
		dcr_merror (p, mat, "dcr_foveon_camf_matrix()");
		for (i=0; i < size; i++)
			if (type && type != 6)
				mat[i] = dcr_sget4(p, dp + i*4);
			else
				mat[i] = dcr_sget4(p, dp + i*2) & 0xffff;
			return mat;
	}
	fprintf (stderr,_("%s: \"%s\" matrix not found!\n"), p->ifname, name);
	return 0;
}

int DCR_CLASS dcr_foveon_fixed (DCRAW* p, void *ptr, int size, const char *name)
{
	void *dp;
	unsigned dim[3];

	dp = dcr_foveon_camf_matrix (p,dim, name);
	if (!dp) return 0;
	memcpy (ptr, dp, size*4);
	free (dp);
	return 1;
}

float DCR_CLASS dcr_foveon_avg (short *pix, int range[2], float cfilt)
{
	int i;
	float val, min=FLT_MAX, max=-FLT_MAX, sum=0;

	for (i=range[0]; i <= range[1]; i++) {
		sum += val = pix[i*4] + (pix[i*4]-pix[(i-1)*4]) * cfilt;
		if (min > val) min = val;
		if (max < val) max = val;
	}
	if (range[1] - range[0] == 1) return sum/2;
	return (sum - min - max) / (range[1] - range[0] - 1);
}

short * DCR_CLASS dcr_foveon_make_curve (DCRAW* p, double max, double mul, double filt)
{
	short *curve;
	unsigned i, size;
	double x;

	if (!filt) filt = 0.8;
	size = (unsigned int)(4*M_PI*max / filt);
	if (size == UINT_MAX) size--;
	curve = (short *) calloc (size+1, sizeof *curve);
	dcr_merror (p, p->curve, "foveon_make_curve()");
	p->curve[0] = size;
	for (i=0; i < size; i++) {
		x = i*filt/max/4;
		p->curve[i+1] = (short)((cos(x)+1)/2 * tanh(i*filt/mul) * mul + 0.5);
	}
	return curve;
}

void DCR_CLASS dcr_foveon_make_curves
(DCRAW* p, short **curvep, float dq[3], float div[3], float filt)
{
	double mul[3], max=0;
	int c;

	FORC3 mul[c] = dq[c]/div[c];
	FORC3 if (max < mul[c]) max = mul[c];
	FORC3 curvep[c] = dcr_foveon_make_curve (p,max, mul[c], filt);
}

int DCR_CLASS dcr_foveon_apply_curve (short *curve, int i)
{
	if (abs(i) >= curve[0]) return 0;
	return i < 0 ? -curve[1-i] : curve[1+i];
}

#define image4 ((short (*)[4]) p->image)

void DCR_CLASS dcr_foveon_interpolate(DCRAW* p)
{
	static const short hood[] = { -1,-1, -1,0, -1,1, 0,-1, 0,1, 1,-1, 1,0, 1,1 };
	short *pix, prev[3], *curve[8], (*shrink)[3];
	float cfilt=0, ddft[3][3][2], ppm[3][3][3];
	float cam_xyz[3][3], correct[3][3], last[3][3], trans[3][3];
	float chroma_dq[3], color_dq[3], diag[3][3], div[3];
	float (*black)[3], (*sgain)[3], (*sgrow)[3];
	float fsum[3], val, frow, num;
	int row, col, c, i, j, diff, sgx, irow, sum, min, max, limit;
	int dscr[2][2], dstb[4], (*smrow[7])[3], total[4], ipix[3];
	int work[3][3], smlast, smred, smred_p=0, dev[3];
	int satlev[3], keep[4], active[4];
	unsigned dim[3], *badpix;
	double dsum=0, trsum[3];
	char str[128];
	const char* cp;

	if (p->opt.verbose)
		fprintf (stderr,_("Foveon interpolation...\n"));

	dcr_foveon_fixed (p, dscr, 4, "DarkShieldColRange");
	dcr_foveon_fixed (p, ppm[0][0], 27, "PostPolyMatrix");
	dcr_foveon_fixed (p, satlev, 3, "SaturationLevel");
	dcr_foveon_fixed (p, keep, 4, "KeepImageArea");
	dcr_foveon_fixed (p, active, 4, "ActiveImageArea");
	dcr_foveon_fixed (p, chroma_dq, 3, "ChromaDQ");
	dcr_foveon_fixed (p, color_dq, 3,
		dcr_foveon_camf_param (p, "IncludeBlocks", "ColorDQ") ?
		"ColorDQ" : "ColorDQCamRGB");
	if (dcr_foveon_camf_param (p, "IncludeBlocks", "ColumnFilter"))
		dcr_foveon_fixed (p, &cfilt, 1, "ColumnFilter");

	memset (ddft, 0, sizeof ddft);
	if (!dcr_foveon_camf_param (p, "IncludeBlocks", "DarkDrift")
		|| !dcr_foveon_fixed (p, ddft[1][0], 12, "DarkDrift"))
		for (i=0; i < 2; i++) {
			dcr_foveon_fixed (p, dstb, 4, i ? "DarkShieldBottom":"DarkShieldTop");
			for (row = dstb[1]; row <= dstb[3]; row++)
				for (col = dstb[0]; col <= dstb[2]; col++)
					FORC3 ddft[i+1][c][1] += (short) image4[row*p->width+col][c];
				FORC3 ddft[i+1][c][1] /= (dstb[3]-dstb[1]+1) * (dstb[2]-dstb[0]+1);
		}

	if (!(cp = dcr_foveon_camf_param (p, "WhiteBalanceIlluminants", p->model2)))
	{ fprintf (stderr,_("%s: Invalid p->white balance \"%s\"\n"), p->ifname, p->model2);
	return; }
	dcr_foveon_fixed (p, cam_xyz, 9, cp);
	dcr_foveon_fixed (p, correct, 9,
		dcr_foveon_camf_param (p, "WhiteBalanceCorrections", p->model2));
	memset (last, 0, sizeof last);
	for (i=0; i < 3; i++)
		for (j=0; j < 3; j++)
			FORC3 last[i][j] += correct[i][c] * cam_xyz[c][j];

#define LAST(x,y) last[(i+x)%3][(c+y)%3]
	for (i=0; i < 3; i++)
		FORC3 diag[c][i] = LAST(1,1)*LAST(2,2) - LAST(1,2)*LAST(2,1);
#undef LAST
	FORC3 div[c] = diag[c][0]*0.3127f + diag[c][1]*0.329f + diag[c][2]*0.3583f;
	sprintf (str, "%sRGBNeutral", p->model2);
	if (dcr_foveon_camf_param (p, "IncludeBlocks", str))
		dcr_foveon_fixed (p, div, 3, str);
	num = 0;
	FORC3 if (num < div[c]) num = div[c];
	FORC3 div[c] /= num;

	memset (trans, 0, sizeof trans);
	for (i=0; i < 3; i++)
		for (j=0; j < 3; j++)
			FORC3 trans[i][j] += p->rgb_cam[i][c] * last[c][j] * div[j];
	FORC3 trsum[c] = trans[c][0] + trans[c][1] + trans[c][2];
	dsum = (6*trsum[0] + 11*trsum[1] + 3*trsum[2]) / 20;
	for (i=0; i < 3; i++)
		FORC3 last[i][c] = (float)(trans[i][c] * dsum / trsum[i]);
	memset (trans, 0, sizeof trans);
	for (i=0; i < 3; i++)
		for (j=0; j < 3; j++)
			FORC3 trans[i][j] += (i==c ? 32 : -1) * last[c][j] / 30;

	dcr_foveon_make_curves (p, curve, color_dq, div, cfilt);
	FORC3 chroma_dq[c] /= 3;
	dcr_foveon_make_curves (p, curve+3, chroma_dq, div, cfilt);
	FORC3 dsum += chroma_dq[c] / div[c];
	curve[6] = dcr_foveon_make_curve (p, dsum, dsum, cfilt);
	curve[7] = dcr_foveon_make_curve (p, dsum*2, dsum*2, cfilt);

	sgain = (float (*)[3]) dcr_foveon_camf_matrix (p, dim, "SpatialGain");
	if (!sgain) return;
	sgrow = (float (*)[3]) calloc (dim[1], sizeof *sgrow);
	sgx = (p->width + dim[1]-2) / (dim[1]-1);

	black = (float (*)[3]) calloc (p->height, sizeof *black);
	for (row=0; row < p->height; row++) {
		for (i=0; i < 6; i++)
			ddft[0][0][i] = (float)(ddft[1][0][i] +
			row / (p->height-1.0) * (ddft[2][0][i] - ddft[1][0][i]));
		FORC3 black[row][c] =
			( dcr_foveon_avg (p->image[row*p->width]+c, dscr[0], cfilt) +
			dcr_foveon_avg (p->image[row*p->width]+c, dscr[1], cfilt) * 3
			- ddft[0][c][0] ) / 4 - ddft[0][c][1];
	}
	memcpy (black, black+8, sizeof *black*8);
	memcpy (black+p->height-11, black+p->height-22, 11*sizeof *black);
	memcpy (last, black, sizeof last);

	for (row=1; row < p->height-1; row++) {
		FORC3 if (last[1][c] > last[0][c]) {
			if (last[1][c] > last[2][c])
				black[row][c] = (last[0][c] > last[2][c]) ? last[0][c]:last[2][c];
		} else
			if (last[1][c] < last[2][c])
				black[row][c] = (last[0][c] < last[2][c]) ? last[0][c]:last[2][c];
			memmove (last, last+1, 2*sizeof last[0]);
			memcpy (last[2], black[row+1], sizeof last[2]);
	}
	FORC3 black[row][c] = (last[0][c] + last[1][c])/2;
	FORC3 black[0][c] = (black[1][c] + black[3][c])/2;

	val = (float)(1 - exp(-1/24.0));
	memcpy (fsum, black, sizeof fsum);
	for (row=1; row < p->height; row++)
		FORC3 fsum[c] += black[row][c] =
		(black[row][c] - black[row-1][c])*val + black[row-1][c];
	memcpy (last[0], black[p->height-1], sizeof last[0]);
	FORC3 fsum[c] /= p->height;
	for (row = p->height; row--; )
		FORC3 last[0][c] = black[row][c] =
		(black[row][c] - fsum[c] - last[0][c])*val + last[0][c];

	memset (total, 0, sizeof total);
	for (row=2; row < p->height; row+=4)
		for (col=2; col < p->width; col+=4) {
			FORC3 total[c] += (short) p->image[row*p->width+col][c];
			total[3]++;
		}
	for (row=0; row < p->height; row++)
		FORC3 black[row][c] += fsum[c]/2 + total[c]/(total[3]*100.0f);

	for (row=0; row < p->height; row++) {
		for (i=0; i < 6; i++)
			ddft[0][0][i] = (float)(ddft[1][0][i] +
			row / (p->height-1.0) * (ddft[2][0][i] - ddft[1][0][i]));
		pix = p->image[row*p->width];
		memcpy (prev, pix, sizeof prev);
		frow = (float)(row / (p->height-1.0) * (dim[2]-1));
		if ((irow = (int)frow) == (int)(dim[2]-1)) irow--;
		frow -= irow;
		for (i=0; i < (int)dim[1]; i++)
			FORC3 sgrow[i][c] = sgain[ irow   *dim[1]+i][c] * (1-frow) +
			sgain[(irow+1)*dim[1]+i][c] *    frow;
		for (col=0; col < p->width; col++) {
			FORC3 {
				diff = pix[c] - prev[c];
				prev[c] = pix[c];
				ipix[c] = (int)( pix[c] + floor ((diff + (diff*diff >> 14)) * cfilt
					- ddft[0][c][1] - ddft[0][c][0] * ((float) col/p->width - 0.5)
					- black[row][c] ));
			}
			FORC3 {
				work[0][c] = ipix[c] * ipix[c] >> 14;
				work[2][c] = ipix[c] * work[0][c] >> 14;
				work[1][2-c] = ipix[(c+1) % 3] * ipix[(c+2) % 3] >> 14;
			}
			FORC3 {
				for (val=0, i=0; i < 3; i++)
					for (  j=0; j < 3; j++)
						val += ppm[c][i][j] * work[i][j];
					ipix[c] = (int)(floor ((ipix[c] + floor(val)) *
						( sgrow[col/sgx  ][c] * (sgx - col%sgx) +
						sgrow[col/sgx+1][c] * (col%sgx) ) / sgx / div[c]));
					if (ipix[c] > 32000) ipix[c] = 32000;
					pix[c] = ipix[c];
			}
			pix += 4;
		}
	}
	free (black);
	free (sgrow);
	free (sgain);

	if ((badpix = (unsigned int *) dcr_foveon_camf_matrix (p, dim, "BadPixels"))) {
		for (i=0; i < (int)dim[0]; i++) {
			col = (badpix[i] >> 8 & 0xfff) - keep[0];
			row = (badpix[i] >> 20       ) - keep[1];
			if ((int)(row-1) > p->height-3 || (int)(col-1) > p->width-3)
				continue;
			memset (fsum, 0, sizeof fsum);
			for (sum=j=0; j < 8; j++)
				if (badpix[i] & (1 << j)) {
					FORC3 fsum[c] += (short)
						p->image[(row+hood[j*2])*p->width+col+hood[j*2+1]][c];
					sum++;
				}
				if (sum) FORC3 p->image[row*p->width+col][c] = (unsigned short)(fsum[c]/sum);
		}
		free (badpix);
	}

	/* Array for 5x5 Gaussian averaging of red values */
	smrow[6] = (int (*)[3]) calloc (p->width*5, sizeof **smrow);
	dcr_merror (p, smrow[6], "foveon_interpolate()");
	for (i=0; i < 5; i++)
		smrow[i] = smrow[6] + i*p->width;

	/* Sharpen the reds against these Gaussian averages */
	for (smlast=-1, row=2; row < p->height-2; row++) {
		while (smlast < row+2) {
			for (i=0; i < 6; i++)
				smrow[(i+5) % 6] = smrow[i];
			pix = p->image[++smlast*p->width+2];
			for (col=2; col < p->width-2; col++) {
				smrow[4][col][0] =
					(pix[0]*6 + (pix[-4]+pix[4])*4 + pix[-8]+pix[8] + 8) >> 4;
				pix += 4;
			}
		}
		pix = p->image[row*p->width+2];
		for (col=2; col < p->width-2; col++) {
			smred = ( 6 *  smrow[2][col][0]
				+ 4 * (smrow[1][col][0] + smrow[3][col][0])
				+      smrow[0][col][0] + smrow[4][col][0] + 8 ) >> 4;
			if (col == 2)
				smred_p = smred;
			i = pix[0] + ((pix[0] - ((smred*7 + smred_p) >> 3)) >> 3);
			if (i > 32000) i = 32000;
			pix[0] = i;
			smred_p = smred;
			pix += 4;
		}
	}

	/* Adjust the brighter pixels for better linearity */
	min = 0xffff;
	FORC3 {
		i = (int)(satlev[c] / div[c]);
		if (min > i) min = i;
	}
	limit = min * 9 >> 4;
	for (pix=p->image[0]; pix < p->image[p->height*p->width]; pix+=4) {
		if (pix[0] <= limit || pix[1] <= limit || pix[2] <= limit)
			continue;
		min = max = pix[0];
		for (c=1; c < 3; c++) {
			if (min > pix[c]) min = pix[c];
			if (max < pix[c]) max = pix[c];
		}
		if (min >= limit*2) {
			pix[0] = pix[1] = pix[2] = max;
		} else {
			i = 0x4000 - ((min - limit) << 14) / limit;
			i = 0x4000 - (i*i >> 14);
			i = i*i >> 14;
			FORC3 pix[c] += (max - pix[c]) * i >> 14;
		}
	}
	/*
	Because photons that miss one detector often hit another,
	the sum R+G+B is much less noisy than the individual p->colors.
	So smooth the hues without smoothing the total.
	*/
	for (smlast=-1, row=2; row < p->height-2; row++) {
		while (smlast < row+2) {
			for (i=0; i < 6; i++)
				smrow[(i+5) % 6] = smrow[i];
			pix = p->image[++smlast*p->width+2];
			for (col=2; col < p->width-2; col++) {
				FORC3 smrow[4][col][c] = (pix[c-4]+2*pix[c]+pix[c+4]+2) >> 2;
				pix += 4;
			}
		}
		pix = p->image[row*p->width+2];
		for (col=2; col < p->width-2; col++) {
			FORC3 dev[c] = -dcr_foveon_apply_curve (curve[7], pix[c] -
				((smrow[1][col][c] + 2*smrow[2][col][c] + smrow[3][col][c]) >> 2));
			sum = (dev[0] + dev[1] + dev[2]) >> 3;
			FORC3 pix[c] += dev[c] - sum;
			pix += 4;
		}
	}
	for (smlast=-1, row=2; row < p->height-2; row++) {
		while (smlast < row+2) {
			for (i=0; i < 6; i++)
				smrow[(i+5) % 6] = smrow[i];
			pix = p->image[++smlast*p->width+2];
			for (col=2; col < p->width-2; col++) {
				FORC3 smrow[4][col][c] =
					(pix[c-8]+pix[c-4]+pix[c]+pix[c+4]+pix[c+8]+2) >> 2;
				pix += 4;
			}
		}
		pix = p->image[row*p->width+2];
		for (col=2; col < p->width-2; col++) {
			for (total[3]=375, sum=60, c=0; c < 3; c++) {
				for (total[c]=i=0; i < 5; i++)
					total[c] += smrow[i][col][c];
				total[3] += total[c];
				sum += pix[c];
			}
			if (sum < 0) sum = 0;
			j = total[3] > 375 ? (sum << 16) / total[3] : sum * 174;
			FORC3 pix[c] += dcr_foveon_apply_curve (curve[6],
				((j*total[c] + 0x8000) >> 16) - pix[c]);
			pix += 4;
		}
	}

	/* Transform the image to a different colorspace */
	for (pix=p->image[0]; pix < p->image[p->height*p->width]; pix+=4) {
		FORC3 pix[c] -= dcr_foveon_apply_curve (curve[c], pix[c]);
		sum = (pix[0]+pix[1]+pix[1]+pix[2]) >> 2;
		FORC3 pix[c] -= dcr_foveon_apply_curve (curve[c], pix[c]-sum);
		FORC3 {
			for (dsum=i=0; i < 3; i++)
				dsum += trans[c][i] * pix[i];
			if (dsum < 0)  dsum = 0;
			if (dsum > 24000) dsum = 24000;
			ipix[c] = (int)(dsum + 0.5);
		}
		FORC3 pix[c] = ipix[c];
	}

	/* Smooth the image bottom-to-top and save at 1/4 scale */
	shrink = (short (*)[3]) calloc ((p->width/4) * (p->height/4), sizeof *shrink);
	dcr_merror (p, shrink, "foveon_interpolate()");
	for (row = p->height/4; row--; )
		for (col=0; col < p->width/4; col++) {
			ipix[0] = ipix[1] = ipix[2] = 0;
			for (i=0; i < 4; i++)
				for (j=0; j < 4; j++)
					FORC3 ipix[c] += p->image[(row*4+i)*p->width+col*4+j][c];
				FORC3
					if (row+2 > p->height/4)
						shrink[row*(p->width/4)+col][c] = ipix[c] >> 4;
					else
						shrink[row*(p->width/4)+col][c] =
						(shrink[(row+1)*(p->width/4)+col][c]*1840 + ipix[c]*141 + 2048) >> 12;
		}
	/* From the 1/4-scale image, smooth right-to-left */
	for (row=0; row < (p->height & ~3); row++) {
		ipix[0] = ipix[1] = ipix[2] = 0;
		if ((row & 3) == 0)
			for (col = p->width & ~3 ; col--; )
				FORC3 smrow[0][col][c] = ipix[c] =
				(shrink[(row/4)*(p->width/4)+col/4][c]*1485 + ipix[c]*6707 + 4096) >> 13;

		/* Then smooth left-to-right */
		ipix[0] = ipix[1] = ipix[2] = 0;
		for (col=0; col < (p->width & ~3); col++)
			FORC3 smrow[1][col][c] = ipix[c] =
			(smrow[0][col][c]*1485 + ipix[c]*6707 + 4096) >> 13;

		/* Smooth top-to-bottom */
		if (row == 0)
			memcpy (smrow[2], smrow[1], sizeof **smrow * p->width);
		else
			for (col=0; col < (p->width & ~3); col++)
				FORC3 smrow[2][col][c] =
				(smrow[2][col][c]*6707 + smrow[1][col][c]*1485 + 4096) >> 13;

			/* Adjust the chroma toward the smooth values */
			for (col=0; col < (p->width & ~3); col++) {
				for (i=j=30, c=0; c < 3; c++) {
					i += smrow[2][col][c];
					j += p->image[row*p->width+col][c];
				}
				j = (j << 16) / i;
				for (sum=c=0; c < 3; c++) {
					ipix[c] = dcr_foveon_apply_curve (curve[c+3],
						((smrow[2][col][c] * j + 0x8000) >> 16) - p->image[row*p->width+col][c]);
					sum += ipix[c];
				}
				sum >>= 3;
				FORC3 {
					i = p->image[row*p->width+col][c] + ipix[c] - sum;
					if (i < 0) i = 0;
					p->image[row*p->width+col][c] = i;
				}
			}
	}
	free (shrink);
	free (smrow[6]);
	for (i=0; i < 8; i++)
		free (curve[i]);

	/* Trim off the black border */
	active[1] -= keep[1];
	active[3] -= 2;
	i = active[2] - active[0];
	for (row=0; row < active[3]-active[1]; row++)
		memcpy (p->image[row*i], p->image[(row+active[1])*p->width+active[0]],
		i * sizeof *p->image);
	p->width = i;
	p->height = row;
}
//#undef image
#endif //RESTRICTED
/* RESTRICTED code ends here */

/*
Seach from the current directory up to the root looking for
a ".badpixels" file, and fix those pixels now.
*/
void DCR_CLASS dcr_bad_pixels(DCRAW* p, char *fname)
{
	FILE *fp=0;
	char *cp, line[128];
	int len, time, row, col, r, c, rad, tot, n, fixed=0;

	if (!p->filters) return;
	if (fname)
		fp = fopen (fname, "r");
	else {
		for (len=32 ; ; len *= 2) {
			fname = (char *) malloc (len);
			if (!fname) return;
			if (_getcwd (fname, len-16)) break;
			free (fname);
			if (errno != ERANGE) return;
		}
#if defined(WIN32) || defined(DJGPP)
		if (fname[1] == ':')
			memmove (fname, fname+2, len-2);
		for (cp=fname; *cp; cp++)
			if (*cp == '\\') *cp = '/';
#endif
		cp = fname + strlen(fname);
		if (cp[-1] == '/') cp--;
		while (*fname == '/') {
			strcpy (cp, "/.badpixels");
			if ((fp = fopen (fname, "r"))) break;
			if (cp == fname) break;
			while (*--cp != '/');
		}
		free (fname);
	}
	if (!fp) return;
	while (fgets (line, 128, fp)) {
		cp = strchr (line, '#');
		if (cp) *cp = 0;
		if (sscanf (line, "%d %d %d", &col, &row, &time) != 3) continue;
		if ((unsigned) col >= p->width || (unsigned) row >= p->height) continue;
		if (time > p->timestamp) continue;
		for (tot=n=0, rad=1; rad < 3 && n==0; rad++)
			for (r = row-rad; r <= row+rad; r++)
				for (c = col-rad; c <= col+rad; c++)
					if ((unsigned) r < p->height && (unsigned) c < p->width &&
						(r != row || c != col) && dcr_fc(p,r,c) == dcr_fc(p,row,col)) {
						tot += BAYER2(r,c);
						n++;
					}
					BAYER2(row,col) = tot/n;
					if (p->opt.verbose) {
						if (!fixed++)
							fprintf (stderr,_("Fixed dead pixels at:"));
						fprintf (stderr, " %d,%d", col, row);
					}
	}
	if (fixed) fputc ('\n', stderr);
	fclose (fp);
}

void DCR_CLASS dcr_subtract (DCRAW* p,char *fname)
{
	FILE *fp;
	int dim[3]={0,0,0}, comment=0, number=0, error=0, nd=0, c, row, col;
	ushort *pixel;

	if (!(fp = fopen (fname, "rb"))) {
		perror (fname);  return;
	}
	if (fgetc(fp) != 'P' || fgetc(fp) != '5') error = 1;
	while (!error && nd < 3 && (c = fgetc(fp)) != EOF) {
		if (c == '#')  comment = 1;
		if (c == '\n') comment = 0;
		if (comment) continue;
		if (isdigit(c)) number = 1;
		if (number) {
			if (isdigit(c)) dim[nd] = dim[nd]*10 + c -'0';
			else if (isspace(c)) {
				number = 0;  nd++;
			} else error = 1;
		}
	}
	if (error || nd < 3) {
		fprintf (stderr,_("%s is not a valid PGM file!\n"), fname);
		fclose (fp);  return;
	} else if (dim[0] != p->width || dim[1] != p->height || dim[2] != 65535) {
		fprintf (stderr,_("%s has the wrong dimensions!\n"), fname);
		fclose (fp);  return;
	}
	pixel = (ushort *) calloc (p->width, sizeof *pixel);
	dcr_merror (p, pixel, "subtract()");
	for (row=0; row < p->height; row++) {
		fread (pixel, 2, p->width, fp);
		for (col=0; col < p->width; col++)
			BAYER(row,col) = MAX (BAYER(row,col) - ntohs(pixel[col]), 0);
	}
	fclose (fp);
	free (pixel);
	p->black = 0;
}

void DCR_CLASS dcr_pseudoinverse (double (*in)[3], double (*out)[3], int size)
{
	double work[3][6], num;
	int i, j, k;

	for (i=0; i < 3; i++) {
		for (j=0; j < 6; j++)
			work[i][j] = j == i+3;
		for (j=0; j < 3; j++)
			for (k=0; k < size; k++)
				work[i][j] += in[k][i] * in[k][j];
	}
	for (i=0; i < 3; i++) {
		num = work[i][i];
		for (j=0; j < 6; j++)
			work[i][j] /= num;
		for (k=0; k < 3; k++) {
			if (k==i) continue;
			num = work[k][i];
			for (j=0; j < 6; j++)
				work[k][j] -= work[i][j] * num;
		}
	}
	for (i=0; i < size; i++)
		for (j=0; j < 3; j++)
			for (out[i][j]=k=0; k < 3; k++)
				out[i][j] += work[j][k+3] * in[i][k];
}

void DCR_CLASS dcr_cam_xyz_coeff (DCRAW* p, double cam_xyz[4][3])
{
	double cam_rgb[4][3], inverse[4][3], num;
	int i, j, k;

	for (i=0; i < p->colors; i++)		/* Multiply out XYZ colorspace */
		for (j=0; j < 3; j++)
			for (cam_rgb[i][j] = k=0; k < 3; k++)
				cam_rgb[i][j] += cam_xyz[i][k] * xyz_rgb[k][j];

	for (i=0; i < p->colors; i++) {		/* Normalize cam_rgb so that */
		for (num=j=0; j < 3; j++)		/* cam_rgb * (1,1,1) is (1,1,1,1) */
			num += cam_rgb[i][j];
		for (j=0; j < 3; j++)
			cam_rgb[i][j] /= num;
		p->pre_mul[i] = 1 / (float)num;
	}
	dcr_pseudoinverse (cam_rgb, inverse, p->colors);
	for (p->raw_color = i=0; i < 3; i++)
		for (j=0; j < p->colors; j++)
			p->rgb_cam[i][j] = (float)inverse[j][i];
}

#ifdef COLORCHECK
void DCR_CLASS dcr_colorcheck(DCRAW* p)
{
#define NSQ 24
	// Coordinates of the GretagMacbeth ColorChecker squares
	// p->width, p->height, 1st_column, 1st_row
	static const int cut[NSQ][4] = {
		{ 241, 231, 234, 274 },
		{ 251, 235, 534, 274 },
		{ 255, 239, 838, 272 },
		{ 255, 240, 1146, 274 },
		{ 251, 237, 1452, 278 },
		{ 243, 238, 1758, 288 },
		{ 253, 253, 218, 558 },
		{ 255, 249, 524, 562 },
		{ 261, 253, 830, 562 },
		{ 260, 255, 1144, 564 },
		{ 261, 255, 1450, 566 },
		{ 247, 247, 1764, 576 },
		{ 255, 251, 212, 862 },
		{ 259, 259, 518, 862 },
		{ 263, 261, 826, 864 },
		{ 265, 263, 1138, 866 },
		{ 265, 257, 1450, 872 },
		{ 257, 255, 1762, 874 },
		{ 257, 253, 212, 1164 },
		{ 262, 251, 516, 1172 },
		{ 263, 257, 826, 1172 },
		{ 263, 255, 1136, 1176 },
		{ 255, 252, 1452, 1182 },
		{ 257, 253, 1760, 1180 } };
	// ColorChecker Chart under 6500-kelvin illumination
	static const double gmb_xyY[NSQ][3] = {
		{ 0.400, 0.350, 10.1 },		// Dark Skin
		{ 0.377, 0.345, 35.8 },		// Light Skin
		{ 0.247, 0.251, 19.3 },		// Blue Sky
		{ 0.337, 0.422, 13.3 },		// Foliage
		{ 0.265, 0.240, 24.3 },		// Blue Flower
		{ 0.261, 0.343, 43.1 },		// Bluish Green
		{ 0.506, 0.407, 30.1 },		// Orange
		{ 0.211, 0.175, 12.0 },		// Purplish Blue
		{ 0.453, 0.306, 19.8 },		// Moderate Red
		{ 0.285, 0.202, 6.6 },		// Purple
		{ 0.380, 0.489, 44.3 },		// Yellow Green
		{ 0.473, 0.438, 43.1 },		// Orange Yellow
		{ 0.187, 0.129, 6.1 },		// Blue
		{ 0.305, 0.478, 23.4 },		// Green
		{ 0.539, 0.313, 12.0 },		// Red
		{ 0.448, 0.470, 59.1 },		// Yellow
		{ 0.364, 0.233, 19.8 },		// Magenta
		{ 0.196, 0.252, 19.8 },		// Cyan
		{ 0.310, 0.316, 90.0 },		// White
		{ 0.310, 0.316, 59.1 },		// Neutral 8
		{ 0.310, 0.316, 36.2 },		// Neutral 6.5
		{ 0.310, 0.316, 19.8 },		// Neutral 5
		{ 0.310, 0.316, 9.0 },		// Neutral 3.5
		{ 0.310, 0.316, 3.1 } };		// Black
	double gmb_cam[NSQ][4], gmb_xyz[NSQ][3];
	double inverse[NSQ][3], cam_xyz[4][3], num;
	int c, i, j, k, sq, row, col, count[4];

	memset (gmb_cam, 0, sizeof gmb_cam);
	for (sq=0; sq < NSQ; sq++) {
		FORCC(p) count[c] = 0;
		for   (row=cut[sq][3]; row < cut[sq][3]+cut[sq][1]; row++)
			for (col=cut[sq][2]; col < cut[sq][2]+cut[sq][0]; col++) {
				c = FC(row,col);
				if (c >= p->colors) c -= 2;
				gmb_cam[sq][c] += BAYER(row,col);
				count[c]++;
			}
			FORCC(p) gmb_cam[sq][c] = gmb_cam[sq][c]/count[c] - p->black;
			gmb_xyz[sq][0] = gmb_xyY[sq][2] * gmb_xyY[sq][0] / gmb_xyY[sq][1];
			gmb_xyz[sq][1] = gmb_xyY[sq][2];
			gmb_xyz[sq][2] = gmb_xyY[sq][2] *
				(1 - gmb_xyY[sq][0] - gmb_xyY[sq][1]) / gmb_xyY[sq][1];
	}
	dcr_pseudoinverse (gmb_xyz, inverse, NSQ);
	for (i=0; i < p->colors; i++)
		for (j=0; j < 3; j++)
			for (cam_xyz[i][j] = k=0; k < NSQ; k++)
				cam_xyz[i][j] += gmb_cam[k][i] * inverse[k][j];
	dcr_cam_xyz_coeff (p, cam_xyz);
	if (p->opt.verbose) {
		printf ("    { \"%s %s\", %d,\n\t{", p->make, p->model, p->black);
		num = 10000 / (cam_xyz[1][0] + cam_xyz[1][1] + cam_xyz[1][2]);
		FORCC(p) for (j=0; j < 3; j++)
			printf ("%c%d", (c | j) ? ',':' ', (int) (cam_xyz[c][j] * num + 0.5));
		puts (" } },");
	}
#undef NSQ
}
#endif

void DCR_CLASS dcr_hat_transform (float *temp, float *base, int st, int size, int sc)
{
	int i;
	for (i=0; i < sc; i++)
		temp[i] = 2*base[st*i] + base[st*(sc-i)] + base[st*(i+sc)];
	for (; i+sc < size; i++)
		temp[i] = 2*base[st*i] + base[st*(i-sc)] + base[st*(i+sc)];
	for (; i < size; i++)
		temp[i] = 2*base[st*i] + base[st*(i-sc)] + base[st*(2*size-2-(i+sc))];
}

void DCR_CLASS dcr_wavelet_denoise(DCRAW* p)
{
	float *fimg=0, *temp, thold, mul[2], avg, diff;
	int scale=1, size, lev, hpass, lpass, row, col, nc, c, i, wlast;
	ushort *window[4];
	static const float noise[] =
	{ 0.8002f,0.2735f,0.1202f,0.0585f,0.0291f,0.0152f,0.0080f,0.0044f };

	if (p->opt.verbose) fprintf (stderr,_("Wavelet denoising...\n"));

	while (p->maximum << scale < 0x10000) scale++;
	p->maximum <<= --scale;
	p->black <<= scale;
	if ((size = p->iheight*p->iwidth) < 0x15550000)
		fimg = (float *) malloc ((size*3 + p->iheight + p->iwidth) * sizeof *fimg);
	dcr_merror (p, fimg, "wavelet_denoise()");
	temp = fimg + size*3;
	if ((nc = p->colors) == 3 && p->filters) nc++;
	FORC(nc) {	/* denoise R,G1,B,G3 individually */
		for (i=0; i < size; i++)
			fimg[i] = 256 * (float)sqrt(p->image[i][c] << scale);
		for (hpass=lev=0; lev < 5; lev++) {
			lpass = size*((lev & 1)+1);
			for (row=0; row < p->iheight; row++) {
				dcr_hat_transform (temp, fimg+hpass+row*p->iwidth, 1, p->iwidth, 1 << lev);
				for (col=0; col < p->iwidth; col++)
					fimg[lpass + row*p->iwidth + col] = temp[col] * 0.25f;
			}
			for (col=0; col < p->iwidth; col++) {
				dcr_hat_transform (temp, fimg+lpass+col, p->iwidth, p->iheight, 1 << lev);
				for (row=0; row < p->iheight; row++)
					fimg[lpass + row*p->iwidth + col] = temp[row] * 0.25f;
			}
			thold = p->opt.threshold * noise[lev];
			for (i=0; i < size; i++) {
				fimg[hpass+i] -= fimg[lpass+i];
				if	(fimg[hpass+i] < -thold) fimg[hpass+i] += thold;
				else if (fimg[hpass+i] >  thold) fimg[hpass+i] -= thold;
				else	 fimg[hpass+i] = 0;
				if (hpass) fimg[i] += fimg[hpass+i];
			}
			hpass = lpass;
		}
		for (i=0; i < size; i++)
			p->image[i][c] = (unsigned short)(CLIP(SQR(fimg[i]+fimg[lpass+i])/0x10000));
	}
	if (p->filters && p->colors == 3) {  /* pull G1 and G3 closer together */
		for (row=0; row < 2; row++)
			mul[row] = 0.125f * p->pre_mul[FC(row+1,0) | 1] / p->pre_mul[FC(row,0) | 1];
		for (i=0; i < 4; i++)
			window[i] = (ushort *) fimg + p->width*i;
		for (wlast=-1, row=1; row < p->height-1; row++) {
			while (wlast < row+1) {
				for (wlast++, i=0; i < 4; i++)
					window[(i+3) & 3] = window[i];
				for (col = FC(wlast,1) & 1; col < p->width; col+=2)
					window[2][col] = BAYER(wlast,col);
			}
			thold = p->opt.threshold/512;
			for (col = (FC(row,0) & 1)+1; col < p->width-1; col+=2) {
				avg = ( window[0][col-1] + window[0][col+1] +
					window[2][col-1] + window[2][col+1] - p->black*4 )
					* mul[row & 1] + (window[1][col] - p->black) * 0.5f + p->black;
				avg = avg < 0 ? 0 : (float)sqrt(avg);
				diff = (float)sqrt(BAYER(row,col)) - avg;
				if      (diff < -thold) diff += thold;
				else if (diff >  thold) diff -= thold;
				else diff = 0;
				BAYER(row,col) = (unsigned short)(CLIP(SQR(avg+diff) + 0.5f));
			}
		}
	}
	free (fimg);
}

void DCR_CLASS dcr_scale_colors(DCRAW* p)
{
	unsigned bottom, right, size, row, col, ur, uc, i, x, y, c, sum[8];
	int val, dark, sat;
	double dsum[8], dmin, dmax;
	float scale_mul[4], fr, fc;
	ushort *img=0, *pix;

	if (p->opt.user_mul[0])
		memcpy (p->pre_mul, p->opt.user_mul, sizeof p->pre_mul);
	if (p->opt.use_auto_wb || (p->opt.use_camera_wb && p->cam_mul[0] == -1)) {
		memset (dsum, 0, sizeof dsum);
		bottom = MIN (p->opt.greybox[1]+p->opt.greybox[3], p->height);
		right  = MIN (p->opt.greybox[0]+p->opt.greybox[2], p->width);
		for (row=p->opt.greybox[1]; row < bottom; row += 8)
			for (col=p->opt.greybox[0]; col < right; col += 8) {
				memset (sum, 0, sizeof sum);
				for (y=row; y < row+8 && y < bottom; y++)
					for (x=col; x < col+8 && x < right; x++)
						FORC4 {
						if (p->filters) {
							c = FC(y,x);
							val = BAYER(y,x);
						} else
							val = p->image[y*p->width+x][c];
						if (val > (int)p->maximum-25) goto skip_block;
						if ((val -= p->black) < 0) val = 0;
						sum[c] += val;
						sum[c+4]++;
						if (p->filters) break;
					}
					FORC(8) dsum[c] += sum[c];
skip_block: ;
			}
			FORC4 if (dsum[c]) p->pre_mul[c] = (float)(dsum[c+4] / dsum[c]);
	}
	if (p->opt.use_camera_wb && p->cam_mul[0] != -1) {
		memset (sum, 0, sizeof sum);
		for (row=0; row < 8; row++)
			for (col=0; col < 8; col++) {
				c = FC(row,col);
				if ((val = p->white[row][col] - p->black) > 0)
					sum[c] += val;
				sum[c+4]++;
			}
			if (sum[0] && sum[1] && sum[2] && sum[3])
				FORC4 p->pre_mul[c] = (float) sum[c+4] / sum[c];
			else if (p->cam_mul[0] && p->cam_mul[2])
				memcpy (p->pre_mul, p->cam_mul, sizeof p->pre_mul);
			else
				fprintf (stderr,_("%s: Cannot use camera p->white balance.\n"), p->ifname);
	}
	if (p->pre_mul[3] == 0) p->pre_mul[3] = p->colors < 4 ? p->pre_mul[1] : 1;
	dark = p->black;
	sat = p->maximum;
	if (p->opt.threshold) dcr_wavelet_denoise(p);
	p->maximum -= p->black;
	for (dmin=DBL_MAX, dmax=c=0; c < 4; c++) {
		if (dmin > p->pre_mul[c])
			dmin = p->pre_mul[c];
		if (dmax < p->pre_mul[c])
			dmax = p->pre_mul[c];
	}
	if (!p->opt.highlight) dmax = dmin;
	FORC4 scale_mul[c] = (float)((p->pre_mul[c] /= (float)dmax) * 65535.0f / p->maximum);
	if (p->opt.verbose) {
		fprintf (stderr,_("Scaling with darkness %d, saturation %d, and\nmultipliers"), dark, sat);
		FORC4 fprintf (stderr, " %f", p->pre_mul[c]);
		fputc ('\n', stderr);
	}
	size = p->iheight*p->iwidth;
	for (i=0; i < size*4; i++) {
		val = p->image[0][i];
		if (!val) continue;
		val -= p->black;
		val = (int)(val * scale_mul[i & 3]);
		p->image[0][i] = CLIP(val);
	}
	if ((p->opt.aber[0] != 1 || p->opt.aber[2] != 1) && p->colors == 3) {
		if (p->opt.verbose)
			fprintf (stderr,_("Correcting chromatic aberration...\n"));
		for (c=0; c < 4; c+=2) {
			if (p->opt.aber[c] == 1) continue;
			img = (ushort *) malloc (size * sizeof *img);
			dcr_merror (p, img, "scale_colors()");
			for (i=0; i < size; i++)
				img[i] = p->image[i][c];
			for (row=0; row < p->iheight; row++) {
				fr = (float)((row - p->iheight*0.5) * p->opt.aber[c] + p->iheight*0.5);
				ur = (unsigned int)fr;
				if ((int)ur > p->iheight-2) continue;
				fr -= ur;
				for (col=0; col < p->iwidth; col++) {
					fc = (float)((col - p->iwidth*0.5) * p->opt.aber[c] + p->iwidth*0.5);
					uc = (unsigned int)fc;
					if ((int)uc > p->iwidth-2) continue;
					fc -= uc;
					pix = img + ur*p->iwidth + uc;
					p->image[row*p->iwidth+col][c] = (unsigned short)(
						(pix[     0]*(1-fc) + pix[       1]*fc) * (1-fr) +
						(pix[p->iwidth]*(1-fc) + pix[p->iwidth+1]*fc) * fr);
				}
			}
			free(img);
		}
	}
}

void DCR_CLASS dcr_pre_interpolate(DCRAW* p)
{
	ushort (*img)[4];
	int row, col, c;

	if (p->shrink) {
		if (p->opt.half_size) {
			p->height = p->iheight;
			p->width  = p->iwidth;
		} else {
			img = (ushort (*)[4]) calloc (p->height*p->width, sizeof *img);
			dcr_merror (p, img, "pre_interpolate()");
			for (row=0; row < p->height; row++)
				for (col=0; col < p->width; col++) {
					c = dcr_fc(p,row,col);
					img[row*p->width+col][c] = p->image[(row >> 1)*p->iwidth+(col >> 1)][c];
				}
				free (p->image);
				p->image = img;
				p->shrink = 0;
		}
	}
	if (p->filters && p->colors == 3) {
		if ((p->mix_green = p->opt.four_color_rgb)) p->colors++;
		else {
			for (row = FC(1,0) >> 1; row < p->height; row+=2)
				for (col = FC(row,1) & 1; col < p->width; col+=2)
					p->image[row*p->width+col][1] = p->image[row*p->width+col][3];
				p->filters &= ~((p->filters & 0x55555555) << 1);
		}
	}
	if (p->opt.half_size) p->filters = 0;
}

void DCR_CLASS dcr_border_interpolate (DCRAW* p, int border)
{
	unsigned row, col, y, x, f, sum[8];
	int c;

	for (row=0; row < p->height; row++)
		for (col=0; col < p->width; col++) {
			if (col==(unsigned int)border && row >= (unsigned int)border && row < p->height-(unsigned int)border)
				col = p->width-border;
			memset (sum, 0, sizeof sum);
			for (y=row-1; y != row+2; y++)
				for (x=col-1; x != col+2; x++)
					if (y < p->height && x < p->width) {
						f = dcr_fc(p,y,x);
						sum[f] += p->image[y*p->width+x][f];
						sum[f+4]++;
					}
			f = dcr_fc(p,row,col);
			FORCC(p) if ((unsigned int)c != f && sum[c+4])
				p->image[row*p->width+col][c] = sum[c] / sum[c+4];
		}
}

void DCR_CLASS dcr_lin_interpolate(DCRAW* p)
{
	int code[16][16][32], *ip, sum[4];
	int c, i, x, y, row, col, shift, color;
	ushort *pix;

	if (p->opt.verbose) fprintf (stderr,_("Bilinear interpolation...\n"));

	dcr_border_interpolate(p,1);
	for (row=0; row < 16; row++)
		for (col=0; col < 16; col++) {
			ip = code[row][col];
			memset (sum, 0, sizeof sum);
			for (y=-1; y <= 1; y++)
				for (x=-1; x <= 1; x++) {
					shift = (y==0) + (x==0);
					if (shift == 2) continue;
					color = dcr_fc(p,row+y,col+x);
					*ip++ = (p->width*y + x)*4 + color;
					*ip++ = shift;
					*ip++ = color;
					sum[color] += 1 << shift;
				}
				FORCC(p)
					if (c != dcr_fc(p,row,col)) {
						*ip++ = c;
						*ip++ = 256 / sum[c];
					}
		}
	for (row=1; row < p->height-1; row++)
		for (col=1; col < p->width-1; col++) {
			pix = p->image[row*p->width+col];
			ip = code[row & 15][col & 15];
			memset (sum, 0, sizeof sum);
			for (i=8; i--; ip+=3)
				sum[ip[2]] += pix[ip[0]] << ip[1];
			for (i=p->colors; --i; ip+=2)
				pix[ip[0]] = sum[ip[0]] * ip[1] >> 8;
		}
}

/*
This algorithm is officially called:

  "Interpolation using a Threshold-based variable number of gradients"

	described in http://scien.stanford.edu/class/psych221/projects/99/tingchen/algodep/vargra.html

	  I've extended the basic idea to work with non-Bayer filter arrays.
	  Gradients are numbered clockwise from NW=0 to W=7.
*/
void DCR_CLASS dcr_vng_interpolate(DCRAW* p)
{
	static const signed char *cp, terms[] = {
		-2,-2,+0,-1,0,0x01, -2,-2,+0,+0,1,0x01, -2,-1,-1,+0,0,0x01,
		-2,-1,+0,-1,0,0x02, -2,-1,+0,+0,0,0x03, -2,-1,+0,+1,1,0x01,
		-2,+0,+0,-1,0,0x06, -2,+0,+0,+0,1,0x02, -2,+0,+0,+1,0,0x03,
		-2,+1,-1,+0,0,0x04, -2,+1,+0,-1,1,0x04, -2,+1,+0,+0,0,0x06,
		-2,+1,+0,+1,0,0x02, -2,+2,+0,+0,1,0x04, -2,+2,+0,+1,0,0x04,
		-1,-2,-1,+0,0,(const signed char)0x80, -1,-2,+0,-1,0,0x01, -1,-2,+1,-1,0,0x01,
		-1,-2,+1,+0,1,0x01, -1,-1,-1,+1,0,(const signed char)0x88, -1,-1,+1,-2,0,0x40,
		-1,-1,+1,-1,0,0x22, -1,-1,+1,+0,0,0x33, -1,-1,+1,+1,1,0x11,
		-1,+0,-1,+2,0,0x08, -1,+0,+0,-1,0,0x44, -1,+0,+0,+1,0,0x11,
		-1,+0,+1,-2,1,0x40, -1,+0,+1,-1,0,0x66, -1,+0,+1,+0,1,0x22,
		-1,+0,+1,+1,0,0x33, -1,+0,+1,+2,1,0x10, -1,+1,+1,-1,1,0x44,
		-1,+1,+1,+0,0,0x66, -1,+1,+1,+1,0,0x22, -1,+1,+1,+2,0,0x10,
		-1,+2,+0,+1,0,0x04, -1,+2,+1,+0,1,0x04, -1,+2,+1,+1,0,0x04,
		+0,-2,+0,+0,1,(const signed char)0x80, +0,-1,+0,+1,1,(const signed char)0x88, +0,-1,+1,-2,0,0x40,
		+0,-1,+1,+0,0,0x11, +0,-1,+2,-2,0,0x40, +0,-1,+2,-1,0,0x20,
		+0,-1,+2,+0,0,0x30, +0,-1,+2,+1,1,0x10, +0,+0,+0,+2,1,0x08,
		+0,+0,+2,-2,1,0x40, +0,+0,+2,-1,0,0x60, +0,+0,+2,+0,1,0x20,
		+0,+0,+2,+1,0,0x30, +0,+0,+2,+2,1,0x10, +0,+1,+1,+0,0,0x44,
		+0,+1,+1,+2,0,0x10, +0,+1,+2,-1,1,0x40, +0,+1,+2,+0,0,0x60,
		+0,+1,+2,+1,0,0x20, +0,+1,+2,+2,0,0x10, +1,-2,+1,+0,0,(const signed char)0x80,
		+1,-1,+1,+1,0,(const signed char)0x88, +1,+0,+1,+2,0,0x08, +1,+0,+2,-1,0,0x40,
		+1,+0,+2,+1,0,0x10
	}, chood[] = { -1,-1, -1,0, -1,+1, 0,+1, +1,+1, +1,0, +1,-1, 0,-1 };
	ushort (*brow[5])[4], *pix;
	int prow=7, pcol=1, *ip, *code[16][16], gval[8], gmin, gmax, sum[4];
	int row, col, x, y, x1, x2, y1, y2, t, weight, grads, color, diag;
	int g, diff, thold, num, c;

	dcr_lin_interpolate(p);
	if (p->opt.verbose) fprintf (stderr,_("VNG interpolation...\n"));

	if (p->filters == 1) prow = pcol = 15;
	ip = (int *) calloc ((prow+1)*(pcol+1), 1280);
	dcr_merror (p, ip, "vng_interpolate()");
	for (row=0; row <= prow; row++)		/* Precalculate for VNG */
		for (col=0; col <= pcol; col++) {
			code[row][col] = ip;
			for (cp=terms, t=0; t < 64; t++) {
				y1 = *cp++;  x1 = *cp++;
				y2 = *cp++;  x2 = *cp++;
				weight = *cp++;
				grads = *cp++;
				color = dcr_fc(p, row+y1,col+x1);
				if (dcr_fc(p, row+y2,col+x2) != color) continue;
				diag = (dcr_fc(p, row,col+1) == color && dcr_fc(p, row+1,col) == color) ? 2:1;
				if (abs(y1-y2) == diag && abs(x1-x2) == diag) continue;
				*ip++ = (y1*p->width + x1)*4 + color;
				*ip++ = (y2*p->width + x2)*4 + color;
				*ip++ = weight;
				for (g=0; g < 8; g++)
					if (grads & 1<<g) *ip++ = g;
					*ip++ = -1;
			}
			*ip++ = INT_MAX;
			for (cp=chood, g=0; g < 8; g++) {
				y = *cp++;  x = *cp++;
				*ip++ = (y*p->width + x) * 4;
				color = dcr_fc(p, row,col);
				if (dcr_fc(p, row+y,col+x) != color && dcr_fc(p, row+y*2,col+x*2) == color)
					*ip++ = (y*p->width + x) * 8 + color;
				else
					*ip++ = 0;
			}
		}
	brow[4] = (ushort (*)[4]) calloc (p->width*3, sizeof **brow);
	dcr_merror (p, brow[4], "vng_interpolate()");
	for (row=0; row < 3; row++)
		brow[row] = brow[4] + row*p->width;
	for (row=2; row < p->height-2; row++) {		/* Do VNG interpolation */
		for (col=2; col < p->width-2; col++) {
			pix = p->image[row*p->width+col];
			ip = code[row & prow][col & pcol];
			memset (gval, 0, sizeof gval);
			while ((g = ip[0]) != INT_MAX) {		/* Calculate gradients */
				diff = ABS(pix[g] - pix[ip[1]]) << ip[2];
				gval[ip[3]] += diff;
				ip += 5;
				if ((g = ip[-1]) == -1) continue;
				gval[g] += diff;
				while ((g = *ip++) != -1)
					gval[g] += diff;
			}
			ip++;
			gmin = gmax = gval[0];			/* Choose a p->opt.threshold */
			for (g=1; g < 8; g++) {
				if (gmin > gval[g]) gmin = gval[g];
				if (gmax < gval[g]) gmax = gval[g];
			}
			if (gmax == 0) {
				memcpy (brow[2][col], pix, sizeof *p->image);
				continue;
			}
			thold = gmin + (gmax >> 1);
			memset (sum, 0, sizeof sum);
			color = dcr_fc(p, row,col);
			for (num=g=0; g < 8; g++,ip+=2) {		/* Average the neighbors */
				if (gval[g] <= thold) {
					FORCC(p)
						if (c == color && ip[1])
							sum[c] += (pix[c] + pix[ip[1]]) >> 1;
						else
							sum[c] += pix[ip[0] + c];
						num++;
				}
			}
			FORCC(p) {					/* Save to buffer */
				t = pix[color];
				if (c != color)
					t += (sum[c] - sum[color]) / num;
				brow[2][col][c] = CLIP(t);
			}
		}
		if (row > 3)				/* Write buffer to image */
			memcpy (p->image[(row-2)*p->width+2], brow[0]+2, (p->width-4)*sizeof *p->image);
		for (g=0; g < 4; g++)
			brow[(g-1) & 3] = brow[g];
	}
	memcpy (p->image[(row-2)*p->width+2], brow[0]+2, (p->width-4)*sizeof *p->image);
	memcpy (p->image[(row-1)*p->width+2], brow[1]+2, (p->width-4)*sizeof *p->image);
	free (brow[4]);
	free (code[0][0]);
}

/*
Patterned Pixel Grouping Interpolation by Alain Desbiolles
*/
void DCR_CLASS dcr_ppg_interpolate(DCRAW* p)
{
	int dir[5] = { 1, p->width, -1, -p->width, 1 };
	int row, col, diff[2], guess[2], c, d, i;
	ushort (*pix)[4];

	dcr_border_interpolate(p,3);
	if (p->opt.verbose) fprintf (stderr,_("PPG interpolation...\n"));

	/*  Fill in the green layer with gradients and pattern recognition: */
	for (row=3; row < p->height-3; row++)
		for (col=3+(FC(row,3) & 1), c=FC(row,col); col < p->width-3; col+=2) {
			pix = p->image + row*p->width+col;
			for (i=0; (d=dir[i]) > 0; i++) {
				guess[i] = (pix[-d][1] + pix[0][c] + pix[d][1]) * 2
					- pix[-2*d][c] - pix[2*d][c];
				diff[i] = ( ABS(pix[-2*d][c] - pix[ 0][c]) +
					ABS(pix[ 2*d][c] - pix[ 0][c]) +
					ABS(pix[  -d][1] - pix[ d][1]) ) * 3 +
					( ABS(pix[ 3*d][1] - pix[ d][1]) +
					ABS(pix[-3*d][1] - pix[-d][1]) ) * 2;
			}
			d = dir[i = diff[0] > diff[1]];
			pix[0][1] = ULIM(guess[i] >> 2, pix[d][1], pix[-d][1]);
		}
		/*  Calculate red and blue for each green pixel:		*/
		for (row=1; row < p->height-1; row++)
			for (col=1+(FC(row,2) & 1), c=FC(row,col+1); col < p->width-1; col+=2) {
				pix = p->image + row*p->width+col;
				for (i=0; (d=dir[i]) > 0; c=2-c, i++)
					pix[0][c] = CLIP((pix[-d][c] + pix[d][c] + 2*pix[0][1]
					- pix[-d][1] - pix[d][1]) >> 1);
			}
		/*  Calculate blue for red pixels and vice versa:		*/
		for (row=1; row < p->height-1; row++)
			for (col=1+(FC(row,1) & 1), c=2-FC(row,col); col < p->width-1; col+=2) {
				pix = p->image + row*p->width+col;
				for (i=0; (d=dir[i]+dir[i+1]) > 0; i++) {
					diff[i] = ABS(pix[-d][c] - pix[d][c]) +
						ABS(pix[-d][1] - pix[0][1]) +
						ABS(pix[ d][1] - pix[0][1]);
					guess[i] = pix[-d][c] + pix[d][c] + 2*pix[0][1]
						- pix[-d][1] - pix[d][1];
				}
				if (diff[0] != diff[1])
					pix[0][c] = CLIP(guess[diff[0] > diff[1]] >> 1);
				else
					pix[0][c] = CLIP((guess[0]+guess[1]) >> 2);
			}
}

/*
Adaptive Homogeneity-Directed interpolation is based on
the work of Keigo Hirakawa, Thomas Parks, and Paul Lee.
*/
#define TS 256		/* Tile Size */

void DCR_CLASS dcr_ahd_interpolate(DCRAW* p)
{
	int i, j, k, top, left, row, col, tr, tc, c, d, val, hm[2];
	ushort (*pix)[4], (*rix)[3];
	static const int dir[4] = { -1, 1, -TS, TS };
	unsigned ldiff[2][4], abdiff[2][4], leps, abeps;
	float r, cbrt[0x10000], xyz[3], xyz_cam[3][4];
	ushort (*rgb)[TS][TS][3];
	short (*lab)[TS][TS][3], (*lix)[3];
	char (*homo)[TS][TS], *buffer;

	if (p->opt.verbose) fprintf (stderr,_("AHD interpolation...\n"));

	for (i=0; i < 0x10000; i++) {
		r = i / 65535.0f;
		cbrt[i] = r > 0.008856f ? (float)pow(r,1/3.0f) : 7.787f*r + 16/116.0f;
	}
	for (i=0; i < 3; i++)
		for (j=0; j < p->colors; j++)
			for (xyz_cam[i][j] =0, k=0; k < 3; k++)
				xyz_cam[i][j] += (float)(xyz_rgb[i][k] * p->rgb_cam[k][j] / d65_white[i]);

		dcr_border_interpolate(p,5);
		buffer = (char *) malloc (26*TS*TS);		/* 1664 kB */
		dcr_merror (p, buffer, "ahd_interpolate()");
		rgb  = (ushort(*)[TS][TS][3]) buffer;
		lab  = (short (*)[TS][TS][3])(buffer + 12*TS*TS);
		homo = (char  (*)[TS][TS])   (buffer + 24*TS*TS);

		for (top=2; top < p->height-5; top += TS-6)
			for (left=2; left < p->width-5; left += TS-6) {

				/*  Interpolate green horizontally and vertically:		*/
				for (row = top; row < top+TS && row < p->height-2; row++) {
					col = left + (FC(row,left) & 1);
					for (c = FC(row,col); col < left+TS && col < p->width-2; col+=2) {
						pix = p->image + row*p->width+col;
						val = ((pix[-1][1] + pix[0][c] + pix[1][1]) * 2
							- pix[-2][c] - pix[2][c]) >> 2;
						rgb[0][row-top][col-left][1] = ULIM(val,pix[-1][1],pix[1][1]);
						val = ((pix[-p->width][1] + pix[0][c] + pix[p->width][1]) * 2
							- pix[-2*p->width][c] - pix[2*p->width][c]) >> 2;
						rgb[1][row-top][col-left][1] = ULIM(val,pix[-p->width][1],pix[p->width][1]);
					}
				}
				/*  Interpolate red and blue, and convert to CIELab:		*/
				for (d=0; d < 2; d++)
					for (row=top+1; row < top+TS-1 && row < p->height-3; row++)
						for (col=left+1; col < left+TS-1 && col < p->width-3; col++) {
							pix = p->image + row*p->width+col;
							rix = &rgb[d][row-top][col-left];
							lix = &lab[d][row-top][col-left];
							if ((c = 2 - FC(row,col)) == 1) {
								c = FC(row+1,col);
								val = pix[0][1] + (( pix[-1][2-c] + pix[1][2-c]
									- rix[-1][1] - rix[1][1] ) >> 1);
								rix[0][2-c] = CLIP(val);
								val = pix[0][1] + (( pix[-p->width][c] + pix[p->width][c]
									- rix[-TS][1] - rix[TS][1] ) >> 1);
							} else
								val = rix[0][1] + (( pix[-p->width-1][c] + pix[-p->width+1][c]
								+ pix[+p->width-1][c] + pix[+p->width+1][c]
								- rix[-TS-1][1] - rix[-TS+1][1]
								- rix[+TS-1][1] - rix[+TS+1][1] + 1) >> 2);
							rix[0][c] = CLIP(val);
							c = FC(row,col);
							rix[0][c] = pix[0][c];
							xyz[0] = xyz[1] = xyz[2] = 0.5;
							FORCC(p) {
								xyz[0] += xyz_cam[0][c] * rix[0][c];
								xyz[1] += xyz_cam[1][c] * rix[0][c];
								xyz[2] += xyz_cam[2][c] * rix[0][c];
							}
							xyz[0] = cbrt[CLIP((int) xyz[0])];
							xyz[1] = cbrt[CLIP((int) xyz[1])];
							xyz[2] = cbrt[CLIP((int) xyz[2])];
							lix[0][0] = (short)(64 * (116 * xyz[1] - 16));
							lix[0][1] = (short)(64 * 500 * (xyz[0] - xyz[1]));
							lix[0][2] = (short)(64 * 200 * (xyz[1] - xyz[2]));
						}
				/*  Build homogeneity maps from the CIELab images:		*/
				memset (homo, 0, 2*TS*TS);
				for (row=top+2; row < top+TS-2 && row < p->height-4; row++) {
					tr = row-top;
					for (col=left+2; col < left+TS-2 && col < p->width-4; col++) {
						tc = col-left;
						for (d=0; d < 2; d++) {
							lix = &lab[d][tr][tc];
							for (i=0; i < 4; i++) {
								ldiff[d][i] = ABS(lix[0][0]-lix[dir[i]][0]);
								abdiff[d][i] = SQR(lix[0][1]-lix[dir[i]][1])
									+ SQR(lix[0][2]-lix[dir[i]][2]);
							}
						}
						leps = MIN(MAX(ldiff[0][0],ldiff[0][1]),
							MAX(ldiff[1][2],ldiff[1][3]));
						abeps = MIN(MAX(abdiff[0][0],abdiff[0][1]),
							MAX(abdiff[1][2],abdiff[1][3]));
						for (d=0; d < 2; d++)
							for (i=0; i < 4; i++)
								if (ldiff[d][i] <= leps && abdiff[d][i] <= abeps)
									homo[d][tr][tc]++;
					}
				}
				/*  Combine the most homogenous pixels for the final result:	*/
				for (row=top+3; row < top+TS-3 && row < p->height-5; row++) {
					tr = row-top;
					for (col=left+3; col < left+TS-3 && col < p->width-5; col++) {
						tc = col-left;
						for (d=0; d < 2; d++)
							for (hm[d]=0, i=tr-1; i <= tr+1; i++)
								for (j=tc-1; j <= tc+1; j++)
									hm[d] += homo[d][i][j];
								if (hm[0] != hm[1])
									FORC3 p->image[row*p->width+col][c] = rgb[hm[1] > hm[0]][tr][tc][c];
								else
									FORC3 p->image[row*p->width+col][c] =
									(rgb[0][tr][tc][c] + rgb[1][tr][tc][c]) >> 1;
					}
				}
			}
			free (buffer);
}
#undef TS

void DCR_CLASS dcr_median_filter(DCRAW* p)
{
	ushort (*pix)[4];
	int pass, c, i, j, k, med[9];
	static const uchar opt[] =	/* Optimal 9-element median search */
	{ 1,2, 4,5, 7,8, 0,1, 3,4, 6,7, 1,2, 4,5, 7,8,
    0,3, 5,8, 4,7, 3,6, 1,4, 2,5, 4,7, 4,2, 6,4, 4,2 };

	for (pass=1; pass <= p->opt.med_passes; pass++) {
		if (p->opt.verbose)
			fprintf (stderr,_("Median filter pass %d...\n"), pass);
		for (c=0; c < 3; c+=2) {
			for (pix = p->image; pix < p->image+p->width*p->height; pix++)
				pix[0][3] = pix[0][c];
			for (pix = p->image+p->width; pix < p->image+p->width*(p->height-1); pix++) {
				if ((pix-p->image+1) % p->width < 2) continue;
				for (k=0, i = -p->width; i <= p->width; i += p->width)
					for (j = i-1; j <= i+1; j++)
						med[k++] = pix[j][3] - pix[j][1];
					for (i=0; i < sizeof opt; i+=2)
						if     (med[opt[i]] > med[opt[i+1]])
							SWAP (med[opt[i]] , med[opt[i+1]]);
						pix[0][c] = CLIP(med[4] + pix[0][1]);
			}
		}
	}
}

void DCR_CLASS dcr_blend_highlights(DCRAW* p)
{
	int clip=INT_MAX, row, col, c, i, j;
	static const float trans[2][4][4] =
	{ { { 1,1,1 }, { 1.7320508f,-1.7320508f,0 }, { -1,-1,2 } },
    { { 1,1,1,1 }, { 1,-1,1,-1 }, { 1,1,-1,-1 }, { 1,-1,-1,1 } } };
	static const float itrans[2][4][4] =
	{ { { 1,0.8660254f,-0.5f }, { 1,-0.8660254f,-0.5f }, { 1,0,1 } },
    { { 1,1,1,1 }, { 1,-1,1,-1 }, { 1,1,-1,-1 }, { 1,-1,-1,1 } } };
	float cam[2][4], lab[2][4], sum[2], chratio;

	if ((unsigned) (p->colors-3) > 1) return;
	if (p->opt.verbose) fprintf (stderr,_("Blending highlights...\n"));
	FORCC(p) if (clip > (i = (int)(65535*p->pre_mul[c]))) clip = i;
	for (row=0; row < p->height; row++)
		for (col=0; col < p->width; col++) {
			FORCC(p) if (p->image[row*p->width+col][c] > clip) break;
			if (c == p->colors) continue;
			FORCC(p) {
				cam[0][c] = p->image[row*p->width+col][c];
				cam[1][c] = MIN(cam[0][c],clip);
			}
			for (i=0; i < 2; i++) {
				FORCC(p) for (lab[i][c]=0, j=0; j < p->colors; j++)
					lab[i][c] += trans[p->colors-3][c][j] * cam[i][j];
				for (sum[i]=0,c=1; c < p->colors; c++)
					sum[i] += SQR(lab[i][c]);
			}
			chratio = (float)sqrt(sum[1]/sum[0]);
			for (c=1; c < p->colors; c++)
				lab[0][c] *= chratio;
			FORCC(p) for (cam[0][c]=0, j=0; j < p->colors; j++)
				cam[0][c] += itrans[p->colors-3][c][j] * lab[0][j];
			FORCC(p) p->image[row*p->width+col][c] = (unsigned short)(cam[0][c] / p->colors);
		}
}

#define SCALE (4 >> p->shrink)
void DCR_CLASS dcr_recover_highlights(DCRAW* p)
{
	float *map, sum, wgt, grow;
	int hsat[4], count, spread, change, val, i, c;
	unsigned high, wide, mrow, mcol, row, col, kc, d, y, x;
	ushort *pixel;
	static const signed char dir[8][2] =
    { {-1,-1}, {-1,0}, {-1,1}, {0,1}, {1,1}, {1,0}, {1,-1}, {0,-1} };

	if (p->opt.verbose) fprintf (stderr,_("Rebuilding highlights...\n"));

	grow = (float)pow (2, 4-p->opt.highlight);
	FORCC(p) hsat[c] = (int)(32000 * p->pre_mul[c]);
	for (kc=0, c=1; (int)c < p->colors; c++)
		if (p->pre_mul[kc] < p->pre_mul[c]) kc = c;
		high = p->height / SCALE;
		wide =  p->width / SCALE;
		map = (float *) calloc (high*wide, sizeof *map);
		dcr_merror (p, map, "recover_highlights()");
		FORCC(p) if ((unsigned int)c != kc) {
			memset (map, 0, high*wide*sizeof *map);
			for (mrow=0; mrow < high; mrow++)
				for (mcol=0; mcol < wide; mcol++) {
					sum = wgt = 0;
					count = 0;
					for (row = mrow*SCALE; row < (mrow+1)*SCALE; row++)
						for (col = mcol*SCALE; col < (mcol+1)*SCALE; col++) {
							pixel = p->image[row*p->width+col];
							if (pixel[c] / hsat[c] == 1 && pixel[kc] > 24000) {
								sum += pixel[c];
								wgt += pixel[kc];
								count++;
							}
						}
						if (count == SCALE*SCALE)
							map[mrow*wide+mcol] = sum / wgt;
				}
			for (spread = (int)(32/grow); spread--; ) {
				for (mrow=0; mrow < high; mrow++)
					for (mcol=0; mcol < wide; mcol++) {
						if (map[mrow*wide+mcol]) continue;
						sum = 0;
						count = 0;
						for (d=0; d < 8; d++) {
							y = mrow + dir[d][0];
							x = mcol + dir[d][1];
							if (y < high && x < wide && map[y*wide+x] > 0) {
								sum  += (1 + (d & 1)) * map[y*wide+x];
								count += 1 + (d & 1);
							}
						}
						if (count > 3)
							map[mrow*wide+mcol] = - (sum+grow) / (count+grow);
					}
					for (change=i=0; i < (int)(high*wide); i++)
						if (map[i] < 0) {
							map[i] = -map[i];
							change = 1;
						}
						if (!change) break;
			}
			for (i=0; i < (int)(high*wide); i++)
				if (map[i] == 0) map[i] = 1;
			for (mrow=0; mrow < high; mrow++)
				for (mcol=0; mcol < wide; mcol++) {
					for (row = mrow*SCALE; row < (mrow+1)*SCALE; row++)
						for (col = mcol*SCALE; col < (mcol+1)*SCALE; col++) {
							pixel = p->image[row*p->width+col];
							if (pixel[c] / hsat[c] > 1) {
								val = (int)(pixel[kc] * map[mrow*wide+mcol]);
								if (pixel[c] < val) pixel[c] = CLIP(val);
							}
						}
				}
		}
		free (map);
}
#undef SCALE

void DCR_CLASS dcr_tiff_get (DCRAW* p, unsigned base,
							 unsigned *tag, unsigned *type, unsigned *len, unsigned *save)
{
	*tag  = dcr_get2(p);
	*type = dcr_get2(p);
	*len  = dcr_get4(p);
	*save = dcr_ftell(p->obj_) + 4;
	if (*len * ("11124811248488"[*type < 14 ? *type:0]-'0') > 4)
		dcr_fseek(p->obj_, dcr_get4(p)+base, SEEK_SET);
}

void DCR_CLASS dcr_parse_thumb_note (DCRAW* p, int base, unsigned toff, unsigned tlen)
{
	unsigned entries, tag, type, len, save;

	entries = dcr_get2(p);
	while (entries--) {
		dcr_tiff_get (p, base, &tag, &type, &len, &save);
		if (tag == toff) p->thumb_offset = dcr_get4(p)+base;
		if (tag == tlen) p->thumb_length = dcr_get4(p);
		dcr_fseek(p->obj_, save, SEEK_SET);
	}
}

int DCR_CLASS dcr_parse_tiff_ifd (DCRAW* p, int base);

void DCR_CLASS dcr_parse_makernote (DCRAW* p, int base, int uptag)
{
	static const uchar xlat[2][256] = {
		{ 0xc1,0xbf,0x6d,0x0d,0x59,0xc5,0x13,0x9d,0x83,0x61,0x6b,0x4f,0xc7,0x7f,0x3d,0x3d,
		0x53,0x59,0xe3,0xc7,0xe9,0x2f,0x95,0xa7,0x95,0x1f,0xdf,0x7f,0x2b,0x29,0xc7,0x0d,
		0xdf,0x07,0xef,0x71,0x89,0x3d,0x13,0x3d,0x3b,0x13,0xfb,0x0d,0x89,0xc1,0x65,0x1f,
		0xb3,0x0d,0x6b,0x29,0xe3,0xfb,0xef,0xa3,0x6b,0x47,0x7f,0x95,0x35,0xa7,0x47,0x4f,
		0xc7,0xf1,0x59,0x95,0x35,0x11,0x29,0x61,0xf1,0x3d,0xb3,0x2b,0x0d,0x43,0x89,0xc1,
		0x9d,0x9d,0x89,0x65,0xf1,0xe9,0xdf,0xbf,0x3d,0x7f,0x53,0x97,0xe5,0xe9,0x95,0x17,
		0x1d,0x3d,0x8b,0xfb,0xc7,0xe3,0x67,0xa7,0x07,0xf1,0x71,0xa7,0x53,0xb5,0x29,0x89,
		0xe5,0x2b,0xa7,0x17,0x29,0xe9,0x4f,0xc5,0x65,0x6d,0x6b,0xef,0x0d,0x89,0x49,0x2f,
		0xb3,0x43,0x53,0x65,0x1d,0x49,0xa3,0x13,0x89,0x59,0xef,0x6b,0xef,0x65,0x1d,0x0b,
		0x59,0x13,0xe3,0x4f,0x9d,0xb3,0x29,0x43,0x2b,0x07,0x1d,0x95,0x59,0x59,0x47,0xfb,
		0xe5,0xe9,0x61,0x47,0x2f,0x35,0x7f,0x17,0x7f,0xef,0x7f,0x95,0x95,0x71,0xd3,0xa3,
		0x0b,0x71,0xa3,0xad,0x0b,0x3b,0xb5,0xfb,0xa3,0xbf,0x4f,0x83,0x1d,0xad,0xe9,0x2f,
		0x71,0x65,0xa3,0xe5,0x07,0x35,0x3d,0x0d,0xb5,0xe9,0xe5,0x47,0x3b,0x9d,0xef,0x35,
		0xa3,0xbf,0xb3,0xdf,0x53,0xd3,0x97,0x53,0x49,0x71,0x07,0x35,0x61,0x71,0x2f,0x43,
		0x2f,0x11,0xdf,0x17,0x97,0xfb,0x95,0x3b,0x7f,0x6b,0xd3,0x25,0xbf,0xad,0xc7,0xc5,
		0xc5,0xb5,0x8b,0xef,0x2f,0xd3,0x07,0x6b,0x25,0x49,0x95,0x25,0x49,0x6d,0x71,0xc7 },
		{ 0xa7,0xbc,0xc9,0xad,0x91,0xdf,0x85,0xe5,0xd4,0x78,0xd5,0x17,0x46,0x7c,0x29,0x4c,
		0x4d,0x03,0xe9,0x25,0x68,0x11,0x86,0xb3,0xbd,0xf7,0x6f,0x61,0x22,0xa2,0x26,0x34,
		0x2a,0xbe,0x1e,0x46,0x14,0x68,0x9d,0x44,0x18,0xc2,0x40,0xf4,0x7e,0x5f,0x1b,0xad,
		0x0b,0x94,0xb6,0x67,0xb4,0x0b,0xe1,0xea,0x95,0x9c,0x66,0xdc,0xe7,0x5d,0x6c,0x05,
		0xda,0xd5,0xdf,0x7a,0xef,0xf6,0xdb,0x1f,0x82,0x4c,0xc0,0x68,0x47,0xa1,0xbd,0xee,
		0x39,0x50,0x56,0x4a,0xdd,0xdf,0xa5,0xf8,0xc6,0xda,0xca,0x90,0xca,0x01,0x42,0x9d,
		0x8b,0x0c,0x73,0x43,0x75,0x05,0x94,0xde,0x24,0xb3,0x80,0x34,0xe5,0x2c,0xdc,0x9b,
		0x3f,0xca,0x33,0x45,0xd0,0xdb,0x5f,0xf5,0x52,0xc3,0x21,0xda,0xe2,0x22,0x72,0x6b,
		0x3e,0xd0,0x5b,0xa8,0x87,0x8c,0x06,0x5d,0x0f,0xdd,0x09,0x19,0x93,0xd0,0xb9,0xfc,
		0x8b,0x0f,0x84,0x60,0x33,0x1c,0x9b,0x45,0xf1,0xf0,0xa3,0x94,0x3a,0x12,0x77,0x33,
		0x4d,0x44,0x78,0x28,0x3c,0x9e,0xfd,0x65,0x57,0x16,0x94,0x6b,0xfb,0x59,0xd0,0xc8,
		0x22,0x36,0xdb,0xd2,0x63,0x98,0x43,0xa1,0x04,0x87,0x86,0xf7,0xa6,0x26,0xbb,0xd6,
		0x59,0x4d,0xbf,0x6a,0x2e,0xaa,0x2b,0xef,0xe6,0x78,0xb6,0x4e,0xe0,0x2f,0xdc,0x7c,
		0xbe,0x57,0x19,0x32,0x7e,0x2a,0xd0,0xb8,0xba,0x29,0x00,0x3c,0x52,0x7d,0xa8,0x49,
		0x3b,0x2d,0xeb,0x25,0x49,0xfa,0xa3,0xaa,0x39,0xa7,0xc5,0xa7,0x50,0x11,0x36,0xfb,
		0xc6,0x67,0x4a,0xf5,0xa5,0x12,0x65,0x7e,0xb0,0xdf,0xaf,0x4e,0xb3,0x61,0x7f,0x2f } };
	unsigned offset=0, entries, tag, type, len, save, c;
	unsigned ver97=0, serial=0, i, wbi=0, wb[4]={0,0,0,0};
	uchar buf97[324], ci, cj, ck;
	short sorder=p->order;
	char buf[10];
	/*
	The MakerNote might have its own TIFF header (possibly with
	its own byte-order!), or it might just be a table.
	*/
	dcr_fread(p->obj_, buf, 1, 10);
	if (!strncmp (buf,"KDK" ,3) ||	/* these aren't TIFF tables */
		!strncmp (buf,"VER" ,3) ||
		!strncmp (buf,"IIII",4) ||
		!strncmp (buf,"MMMM",4)) return;
	if (!strncmp (buf,"KC"  ,2) ||	/* Konica KD-400Z, KD-510Z */
		!strncmp (buf,"MLY" ,3)) {	/* Minolta DiMAGE G series */
		p->order = 0x4d4d;
		while ((long)(i=dcr_ftell(p->obj_)) < p->data_offset && i < 16384) {
			wb[0] = wb[2];  wb[2] = wb[1];  wb[1] = wb[3];
			wb[3] = dcr_get2(p);
			if (wb[1] == 256 && wb[3] == 256 &&
				wb[0] > 256 && wb[0] < 640 && wb[2] > 256 && wb[2] < 640)
				FORC4 p->cam_mul[c] = (float)wb[c];
		}
		goto quit;
	}
	if (!strcmp (buf,"Nikon")) {
		base = dcr_ftell(p->obj_);
		p->order = dcr_get2(p);
		if (dcr_get2(p) != 42) goto quit;
		offset = dcr_get4(p);
		dcr_fseek(p->obj_, offset-8, SEEK_CUR);
	} else if (!strcmp (buf,"OLYMPUS")) {
		base = dcr_ftell(p->obj_)-10;
		dcr_fseek(p->obj_, -2, SEEK_CUR);
		p->order = dcr_get2(p);  dcr_get2(p);
	} else if (!strncmp (buf,"FUJIFILM",8) ||
		!strncmp (buf,"SONY",4) ||
		!strcmp  (buf,"Panasonic")) {
		p->order = 0x4949;
		dcr_fseek(p->obj_,  2, SEEK_CUR);
	} else if (!strcmp (buf,"OLYMP") ||
		!strcmp (buf,"LEICA") ||
		!strcmp (buf,"Ricoh") ||
		!strcmp (buf,"EPSON"))
		dcr_fseek(p->obj_, -2, SEEK_CUR);
	else if (!strcmp (buf,"AOC") ||
		!strcmp (buf,"QVC"))
		dcr_fseek(p->obj_, -4, SEEK_CUR);
	else dcr_fseek(p->obj_, -10, SEEK_CUR);

	entries = dcr_get2(p);
	if (entries > 1000) return;
	while (entries--) {
		dcr_tiff_get (p, base, &tag, &type, &len, &save);
		tag |= uptag << 16;
		if (tag == 2 && strstr(p->make,"NIKON"))
			p->iso_speed = (dcr_get2(p),dcr_get2(p));
		if (tag == 4 && len > 26 && len < 35) {
			if ((i=(dcr_get4(p),dcr_get2(p))) != 0x7fff && !p->iso_speed)
				p->iso_speed = 50 * (float)pow (2, i/32.0 - 4);
			if ((i=(dcr_get2(p),dcr_get2(p))) != 0x7fff && !p->aperture)
				p->aperture = (float)pow (2, i/64.0);
			if ((i=dcr_get2(p)) != 0xffff && !p->shutter)
				p->shutter = (float)pow (2, (short) i/-32.0);
			wbi = (dcr_get2(p),dcr_get2(p));
			p->shot_order = (dcr_get2(p),dcr_get2(p));
		}
		if (tag == 8 && type == 4)
			p->shot_order = dcr_get4(p);
		if (tag == 9 && !strcmp(p->make,"Canon"))
			dcr_fread(p->obj_, p->artist, 64, 1);
		if (tag == 0xc && len == 4) {
			p->cam_mul[0] = (float)dcr_getreal(p,type);
			p->cam_mul[2] = (float)dcr_getreal(p,type);
		}
		if (tag == 0x10 && type == 4)
			p->unique_id = dcr_get4(p);
		if (tag == 0x11 && p->is_raw && !strncmp(p->make,"NIKON",5)) {
			dcr_fseek(p->obj_, dcr_get4(p)+base, SEEK_SET);
			dcr_parse_tiff_ifd (p, base);
		}
		if (tag == 0x14 && len == 2560 && type == 7) {
			dcr_fseek(p->obj_, 1248, SEEK_CUR);
			goto get2_256;
		}
		if (tag == 0x15 && type == 2 && p->is_raw)
			dcr_fread(p->obj_, p->model, 64, 1);
		if (strstr(p->make,"PENTAX")) {
			if (tag == 0x1b) tag = 0x1018;
			if (tag == 0x1c) tag = 0x1017;
		}
		if (tag == 0x1d)
			while ((c = dcr_fgetc(p->obj_)) && c != EOF)
				serial = serial*10 + (isdigit(c) ? c - '0' : c % 10);
		if (tag == 0x81 && type == 4) {
			p->data_offset = dcr_get4(p);
			dcr_fseek(p->obj_, p->data_offset + 41, SEEK_SET);
			p->raw_height = dcr_get2(p) * 2;
			p->raw_width  = dcr_get2(p);
			p->filters = 0x61616161;
		}
		if (tag == 0x29 && type == 1) {
			c = wbi < 18 ? "012347800000005896"[wbi]-'0' : 0;
			dcr_fseek(p->obj_, 8 + c*32, SEEK_CUR);
			FORC4 p->cam_mul[c ^ (c >> 1) ^ 1] = (float)dcr_get4(p);
		}
		if ((tag == 0x81  && type == 7) ||
			(tag == 0x100 && type == 7) ||
			(tag == 0x280 && type == 1)) {
			p->thumb_offset = dcr_ftell(p->obj_);
			p->thumb_length = len;
		}
		if (tag == 0x88 && type == 4 && (p->thumb_offset = dcr_get4(p)))
			p->thumb_offset += base;
		if (tag == 0x89 && type == 4)
			p->thumb_length = dcr_get4(p);
		if (tag == 0x8c || tag == 0x96)
			p->meta_offset = dcr_ftell(p->obj_);
		if (tag == 0x97) {
			for (i=0; i < 4; i++)
				ver97 = ver97 * 10 + dcr_fgetc(p->obj_)-'0';
			switch (ver97) {
			case 100:
				dcr_fseek(p->obj_, 68, SEEK_CUR);
				FORC4 p->cam_mul[(c >> 1) | ((c & 1) << 1)] = dcr_get2(p);
				break;
			case 102:
				dcr_fseek(p->obj_, 6, SEEK_CUR);
				goto get2_rggb;
			case 103:
				dcr_fseek(p->obj_, 16, SEEK_CUR);
				FORC4 p->cam_mul[c] = dcr_get2(p);
			}
			if (ver97 >= 200) {
				if (ver97 != 205) dcr_fseek(p->obj_, 280, SEEK_CUR);
				dcr_fread(p->obj_, buf97, 324, 1);
			}
		}
		if (tag == 0xa4 && type == 3) {
			dcr_fseek(p->obj_, wbi*48, SEEK_CUR);
			FORC3 p->cam_mul[c] = dcr_get2(p);
		}
		if (tag == 0xa7 && (unsigned) (ver97-200) < 12 && !p->cam_mul[0]) {
			ci = xlat[0][serial & 0xff];
			cj = xlat[1][dcr_fgetc(p->obj_)^dcr_fgetc(p->obj_)^dcr_fgetc(p->obj_)^dcr_fgetc(p->obj_)];
			ck = 0x60;
			for (i=0; i < 324; i++)
				buf97[i] ^= (cj += ci * ck++);
			i = "66666>666;6A"[ver97-200] - '0';
			FORC4 p->cam_mul[c ^ (c >> 1) ^ (i & 1)] =
				dcr_sget2 (p, buf97 + (i & -2) + c*2);
		}
		if (tag == 0x200 && len == 3)
			p->shot_order = (dcr_get4(p),dcr_get4(p));
		if (tag == 0x200 && len == 4)
			p->black = (dcr_get2(p)+dcr_get2(p)+dcr_get2(p)+dcr_get2(p))/4;
		if (tag == 0x201 && len == 4)
			goto get2_rggb;
		if (tag == 0x401 && len == 4) {
			p->black = (dcr_get4(p)+dcr_get4(p)+dcr_get4(p)+dcr_get4(p))/4;
		}
		if (tag == 0xe01) {		/* Nikon Capture Note */
			type = p->order;
			p->order = 0x4949;
			dcr_fseek(p->obj_, 22, SEEK_CUR);
			for (offset=22; offset+22 < len; offset += 22+i) {
				tag = dcr_get4(p);
				dcr_fseek(p->obj_, 14, SEEK_CUR);
				i = dcr_get4(p)-4;
				if (tag == 0x76a43207) p->flip = dcr_get2(p);
				else dcr_fseek(p->obj_, i, SEEK_CUR);
			}
			p->order = type;
		}
		if (tag == 0xe80 && len == 256 && type == 7) {
			dcr_fseek(p->obj_, 48, SEEK_CUR);
			p->cam_mul[0] = dcr_get2(p) * 508 * 1.078f / 0x10000;
			p->cam_mul[2] = dcr_get2(p) * 382 * 1.173f / 0x10000;
		}
		if (tag == 0xf00 && type == 7) {
			if (len == 614)
				dcr_fseek(p->obj_, 176, SEEK_CUR);
			else if (len == 734 || len == 1502)
				dcr_fseek(p->obj_, 148, SEEK_CUR);
			else goto next;
			goto get2_256;
		}
		if ((tag == 0x1011 && len == 9) || tag == 0x20400200)
			for (i=0; i < 3; i++)
				FORC3 p->cmatrix[i][c] = ((short) dcr_get2(p)) / 256.0f;
		if ((tag == 0x1012 || tag == 0x20400600) && len == 4)
			for (p->black = i=0; i < 4; i++)
				p->black += dcr_get2(p) << 2;
		if (tag == 0x1017 || tag == 0x20400100)
			p->cam_mul[0] = dcr_get2(p) / 256.0f;
		if (tag == 0x1018 || tag == 0x20400100)
			p->cam_mul[2] = dcr_get2(p) / 256.0f;
		if (tag == 0x2011 && len == 2) {
get2_256:
		p->order = 0x4d4d;
		p->cam_mul[0] = dcr_get2(p) / 256.0f;
		p->cam_mul[2] = dcr_get2(p) / 256.0f;
		}
		if ((tag | 0x70) == 0x2070 && type == 4)
			dcr_fseek(p->obj_, dcr_get4(p)+base, SEEK_SET);
		if (tag == 0x2010 && type != 7)
			p->load_raw = &DCR_CLASS dcr_olympus_e410_load_raw;
		if (tag == 0x2020)
			dcr_parse_thumb_note (p, base, 257, 258);
		if (tag == 0x2040)
			dcr_parse_makernote (p, base, 0x2040);
		if (tag == 0xb028) {
			dcr_fseek(p->obj_, dcr_get4(p), SEEK_SET);
			dcr_parse_thumb_note (p, base, 136, 137);
		}
		if (tag == 0x4001 && len > 500) {
			i = len == 582 ? 50 : len == 653 ? 68 : len == 5120 ? 142 : 126;
			dcr_fseek(p->obj_, i, SEEK_CUR);
get2_rggb:
			FORC4 p->cam_mul[c ^ (c >> 1)] = dcr_get2(p);
			dcr_fseek(p->obj_, 22, SEEK_CUR);
			FORC4 p->sraw_mul[c ^ (c >> 1)] = dcr_get2(p);
		}
next:
		dcr_fseek(p->obj_, save, SEEK_SET);
  }
quit:
  p->order = sorder;
}

/*
Since the TIFF DateTime string has no timezone information,
assume that the camera's clock was set to Universal Time.
*/
void DCR_CLASS dcr_get_timestamp (DCRAW* p, int reversed)
{
	struct tm t;
	char str[20];
	int i;

	str[19] = 0;
	if (reversed)
		for (i=19; i--; ) str[i] = dcr_fgetc(p->obj_);
	else
		dcr_fread(p->obj_, str, 19, 1);
	memset (&t, 0, sizeof t);
	if (sscanf (str, "%d:%d:%d %d:%d:%d", &t.tm_year, &t.tm_mon,
		&t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) != 6)
		return;
	t.tm_year -= 1900;
	t.tm_mon -= 1;
	if (mktime(&t) > 0)
		p->timestamp = mktime(&t);
}

void DCR_CLASS dcr_parse_exif (DCRAW* p, int base)
{
	unsigned kodak, entries, tag, type, len, save, c;
	double expo;

	kodak = !strncmp(p->make,"EASTMAN",7);
	entries = dcr_get2(p);
	while (entries--) {
		dcr_tiff_get (p, base, &tag, &type, &len, &save);
		switch (tag) {
		case 33434:  p->shutter = (float)dcr_getreal(p,type);			break;
		case 33437:  p->aperture = (float)dcr_getreal(p,type);			break;
		case 34855:  p->iso_speed = dcr_get2(p);			break;
		case 36867:
		case 36868:  dcr_get_timestamp(p,0);			break;
		case 37377:  if ((expo = -dcr_getreal(p,type)) < 128)
						 p->shutter = (float)pow (2, expo);		break;
		case 37378:  p->aperture = (float)pow (2, dcr_getreal(p,type)/2);	break;
		case 37386:  p->focal_len = (float)dcr_getreal(p,type);		break;
		case 37500:  dcr_parse_makernote (p,base, 0);		break;
		case 40962:  if (kodak) p->raw_width  = dcr_get4(p);	break;
		case 40963:  if (kodak) p->raw_height = dcr_get4(p);	break;
		case 41730:
			if (dcr_get4(p) == 0x20002)
				for (p->exif_cfa=c=0; c < 8; c+=2)
					p->exif_cfa |= dcr_fgetc(p->obj_) * 0x01010101 << c;
		}
		dcr_fseek(p->obj_, save, SEEK_SET);
	}
}

void DCR_CLASS dcr_parse_gps (DCRAW* p, int base)
{
  unsigned entries, tag, type, len, save, c;

  entries = dcr_get2(p);
  while (entries--) {
    dcr_tiff_get (p, base, &tag, &type, &len, &save);
    switch (tag) {
    case 1: case 3: case 5:
	  p->gpsdata[29+tag/2] = dcr_fgetc(p->obj_);	break;
    case 2: case 4: case 7:
	  FORC(6) p->gpsdata[tag/3*6+c] = dcr_get4(p);	break;
    case 6:
	  FORC(2) p->gpsdata[18+c] = dcr_get4(p);		break;
    case 18: case 29:
	  dcr_fgets(p->obj_, (char *) (p->gpsdata+14+tag/3), MIN(len,12));
    }
    dcr_fseek(p->obj_, save, SEEK_SET);
  }
}

void DCR_CLASS dcr_romm_coeff (DCRAW* p, float romm_cam[3][3])
{
	static const float rgb_romm[3][3] =	/* ROMM == Kodak ProPhoto */
	{ {  2.034193f, -0.727420f, -0.306766f },
    { -0.228811f,  1.231729f, -0.002922f },
    { -0.008565f, -0.153273f,  1.161839f } };
	int i, j, k;

	for (i=0; i < 3; i++)
		for (j=0; j < 3; j++)
			for (p->cmatrix[i][j]=0, k=0; k < 3; k++)
				p->cmatrix[i][j] += rgb_romm[i][k] * romm_cam[k][j];
}

void DCR_CLASS dcr_parse_mos (DCRAW* p, int offset)
{
	char data[40];
	int skip, from, i=0, j, c, neut[4], planes=0, frot=0;
	static const char *mod[] =
	{ "","DCB2","Volare","Cantare","CMost","Valeo 6","Valeo 11","Valeo 22",
    "Valeo 11p","Valeo 17","","Aptus 17","Aptus 22","Aptus 75","Aptus 65",
    "Aptus 54S","Aptus 65S","Aptus 75S","AFi 5","AFi 6","AFi 7" };
	float romm_cam[3][3];

	dcr_fseek(p->obj_, offset, SEEK_SET);
	while (1) {
		if (dcr_get4(p) != 0x504b5453) break;
		dcr_get4(p);
		dcr_fread(p->obj_, data, 1, 40);
		skip = dcr_get4(p);
		from = dcr_ftell(p->obj_);
		if (!strcmp(data,"JPEG_preview_data")) {
			p->thumb_offset = from;
			p->thumb_length = skip;
		}
		if (!strcmp(data,"icc_camera_profile")) {
			p->profile_offset = from;
			p->profile_length = skip;
		}
		if (!strcmp(data,"ShootObj_back_type")) {
			dcr_fscanf(p->obj_, "%d", &i);
			if ((unsigned) i < sizeof mod / sizeof (*mod))
				strcpy (p->model, mod[i]);
		}
		if (!strcmp(data,"icc_camera_to_tone_matrix")) {
			for (i=0; i < 3; i++)
				for (j=0; j < 3; j++)
					romm_cam[i][j] = dcr_int_to_float(dcr_get4(p));
			dcr_romm_coeff (p,romm_cam);
		}
		if (!strcmp(data,"CaptProf_color_matrix")) {
			for (i=0; i < 3; i++)
				for (j=0; j < 3; j++)
					dcr_fscanf(p->obj_, "%f", &romm_cam[i][j]);
			dcr_romm_coeff (p,romm_cam);
		}
		if (!strcmp(data,"CaptProf_number_of_planes"))
			dcr_fscanf(p->obj_, "%d", &planes);
		if (!strcmp(data,"CaptProf_raw_data_rotation"))
			dcr_fscanf(p->obj_, "%d", &p->flip);
		if (!strcmp(data,"CaptProf_mosaic_pattern"))
			FORC4 {
			dcr_fscanf(p->obj_, "%d", &i);
			if (i == 1) frot = c ^ (c >> 1);
		}
		if (!strcmp(data,"ImgProf_rotation_angle")) {
			dcr_fscanf(p->obj_, "%d", &i);
			p->flip = i - p->flip;
		}
		if (!strcmp(data,"NeutObj_neutrals") && !p->cam_mul[0]) {
			FORC4 dcr_fscanf(p->obj_, "%d", neut+c);
			FORC3 p->cam_mul[c] = (neut[c+1] ? (float) neut[0] / neut[c+1] : 0);
		}
		dcr_parse_mos (p,from);
		dcr_fseek(p->obj_, skip+from, SEEK_SET);
	}
	if (planes)
		p->filters = (planes == 1) * 0x01010101 *
		(uchar) "\x94\x61\x16\x49"[(p->flip/90 + frot) & 3];
}

void DCR_CLASS dcr_linear_table (DCRAW* p, unsigned len)
{
	int i;
	if (len > 0x1000) len = 0x1000;
	dcr_read_shorts (p, p->curve, len);
	for (i=len; i < 0x1000; i++)
		p->curve[i] = p->curve[i-1];
	p->maximum = p->curve[0xfff];
}

void DCR_CLASS dcr_parse_kodak_ifd (DCRAW* p, int base)
{
	unsigned entries, tag, type, len, save;
	int i, c, wbi=-2, wbtemp=6500;
	float mul[3], num;

	entries = dcr_get2(p);
	if (entries > 1024) return;
	while (entries--) {
		dcr_tiff_get (p, base, &tag, &type, &len, &save);
		if (tag == 1020) wbi = dcr_getint(p, type);
		if (tag == 1021 && len == 72) {		/* WB set in software */
			dcr_fseek(p->obj_, 40, SEEK_CUR);
			FORC3 p->cam_mul[c] = 2048.0f / dcr_get2(p);
			wbi = -2;
		}
		if (tag == (unsigned int)2118) wbtemp = dcr_getint(p, type);
		if (tag == (unsigned int)2130 + wbi)
			FORC3 mul[c] = (float)dcr_getreal(p, type);
		if (tag == (unsigned int)2140 + wbi && wbi >= 0)
			FORC3 {
			for (num=0.0f, i=0; i < 4; i++)
				num += (float)(dcr_getreal(p, type) * pow (wbtemp/100.0, i));
			p->cam_mul[c] = 2048 / (num * mul[c]);
		}
		if (tag == 2317) dcr_linear_table (p,len);
		if (tag == 6020) p->iso_speed = (float)dcr_getint(p, type);
		dcr_fseek(p->obj_, save, SEEK_SET);
	}
}

void DCR_CLASS dcr_parse_minolta (DCRAW* p, int base);

int DCR_CLASS dcr_parse_tiff_ifd (DCRAW* p, int base)
{
	unsigned entries, tag, type, len, plen=16, save;
	int ifd, use_cm=0, cfa, i, j, c, ima_len=0;
	char software[64], *cbuf, *cp;
	uchar cfa_pat[16], cfa_pc[] = { 0,1,2,3 }, tab[256];
	double dblack, cc[4][4], cm[4][3], cam_xyz[4][3], num;
	double ab[]={ 1,1,1,1 }, asn[] = { 0,0,0,0 }, xyz[] = { 1,1,1 };
	unsigned sony_curve[] = { 0,0,0,0,0,4095 };
	unsigned *buf, sony_offset=0, sony_length=0, sony_key=0;
	struct dcr_jhead jh;

	if (p->tiff_nifds >= sizeof p->tiff_ifd / sizeof p->tiff_ifd[0])
		return 1;
	ifd = p->tiff_nifds++;
	for (j=0; j < 4; j++)
		for (i=0; i < 4; i++)
			cc[j][i] = i == j;
	entries = dcr_get2(p);
	if (entries > 512) return 1;
	while (entries--) {
		dcr_tiff_get (p, base, &tag, &type, &len, &save);
		switch (tag) {
		case 17: case 18:
			if (type == 3 && len == 1)
				p->cam_mul[(tag-17)*2] = dcr_get2(p) / 256.0f;
			break;
		case 23:
			if (type == 3) p->iso_speed = dcr_get2(p);
			break;
		case 36: case 37: case 38:
			p->cam_mul[tag-0x24] = dcr_get2(p);
			break;
		case 39:
			if (len < 50 || p->cam_mul[0]) break;
			dcr_fseek(p->obj_, 12, SEEK_CUR);
			FORC3 p->cam_mul[c] = dcr_get2(p);
			break;
		case 46:
			if (type != 7 || dcr_fgetc(p->obj_) != 0xff || dcr_fgetc(p->obj_) != 0xd8) break;
			p->thumb_offset = dcr_ftell(p->obj_) - 2;
			p->thumb_length = len;
			break;
		case 2: case 256:			/* ImageWidth */
			p->tiff_ifd[ifd].width = dcr_getint(p, type);
			break;
		case 3: case 257:			/* ImageHeight */
			p->tiff_ifd[ifd].height = dcr_getint(p, type);
			break;
		case 258:				/* BitsPerSample */
			p->tiff_ifd[ifd].samples = len & 7;
			p->tiff_ifd[ifd].bps = dcr_get2(p);
			break;
		case 259:				/* Compression */
			p->tiff_ifd[ifd].comp = dcr_get2(p);
			break;
		case 262:				/* PhotometricInterpretation */
			p->tiff_ifd[ifd].phint = dcr_get2(p);
			break;
		case 270:				/* ImageDescription */
			dcr_fread(p->obj_, p->desc, 512, 1);
			break;
		case 271:				/* Make */
			dcr_fgets(p->obj_, p->make, 64);
			break;
		case 272:				/* Model */
			dcr_fgets(p->obj_, p->model, 64);
			break;
		case 280:				/* Panasonic RW2 offset */
			if (type != 4) break;
			p->load_raw = &DCR_CLASS dcr_panasonic_load_raw;
			p->load_flags = 0x2008;
		case 273:				/* StripOffset */
		case 513:
			p->tiff_ifd[ifd].offset = dcr_get4(p)+base;
			if (!p->tiff_ifd[ifd].bps) {
				dcr_fseek(p->obj_, p->tiff_ifd[ifd].offset, SEEK_SET);
				if (dcr_ljpeg_start (p,&jh, 1)) {
					p->tiff_ifd[ifd].comp    = 6;
					p->tiff_ifd[ifd].width   = jh.wide << (jh.clrs == 2);
					p->tiff_ifd[ifd].height  = jh.high;
					p->tiff_ifd[ifd].bps     = jh.bits;
					p->tiff_ifd[ifd].samples = jh.clrs;
				}
			}
			break;
		case 274:				/* Orientation */
			p->tiff_ifd[ifd].flip = "50132467"[dcr_get2(p) & 7]-'0';
			break;
		case 277:				/* SamplesPerPixel */
			p->tiff_ifd[ifd].samples = dcr_getint(p, type) & 7;
			break;
		case 279:				/* StripByteCounts */
		case 514:
			p->tiff_ifd[ifd].bytes = dcr_get4(p);
			break;
		case 305:  case 11:		/* Software */
			dcr_fgets(p->obj_, software, 64);
			if (!strncmp(software,"Adobe",5) ||
				!strncmp(software,"dcraw",5) ||
				!strncmp(software,"UFRaw",5) ||
				!strncmp(software,"Bibble",6) ||
				!strncmp(software,"Nikon Scan",10) ||
				!strcmp (software,"Digital Photo Professional"))
				p->is_raw = 0;
			break;
		case 306:				/* DateTime */
			dcr_get_timestamp(p,0);
			break;
		case 315:				/* Artist */
			dcr_fread(p->obj_, p->artist, 64, 1);
			break;
		case 322:				/* TileWidth */
			p->tile_width = dcr_getint(p, type);
			break;
		case 323:				/* TileLength */
			p->tile_length = dcr_getint(p, type);
			break;
		case 324:				/* TileOffsets */
			p->tiff_ifd[ifd].offset = len > 1 ? dcr_ftell(p->obj_) : dcr_get4(p);
			if (len == 4) {
				p->load_raw = &DCR_CLASS dcr_sinar_4shot_load_raw;
				p->is_raw = 5;
			}
			break;
		case 330:				/* SubIFDs */
			if (!strcmp(p->model,"DSLR-A100") && p->tiff_ifd[ifd].width == 3872) {
				p->load_raw = &DCR_CLASS dcr_sony_arw_load_raw;
				p->data_offset = dcr_get4(p)+base;
				ifd++;  break;
			}
			if(len > 1000) len=1000; /* 1000 SubIFDs is enough */
			while (len--) {
				i = dcr_ftell(p->obj_);
				dcr_fseek(p->obj_, dcr_get4(p)+base, SEEK_SET);
				if (dcr_parse_tiff_ifd (p, base)) break;
				dcr_fseek(p->obj_, i+4, SEEK_SET);
			}
			break;
		case 400:
			strcpy (p->make, "Sarnoff");
			p->maximum = 0xfff;
			break;
		case 28688:
			FORC4 sony_curve[c+1] = dcr_get2(p) >> 2 & 0xfff;
			for (i=0; i < 5; i++)
				for (j = sony_curve[i]+1; j <= (int)sony_curve[i+1]; j++)
					p->curve[j] = p->curve[j-1] + (1 << i);
				break;
		case 29184: sony_offset = dcr_get4(p);  break;
		case 29185: sony_length = dcr_get4(p);  break;
		case 29217: sony_key    = dcr_get4(p);  break;
		case 29264:
			dcr_parse_minolta (p, dcr_ftell(p->obj_));
			p->raw_width = 0;
			break;
		case 29443:
			FORC4 p->cam_mul[c ^ (c < 2)] = dcr_get2(p);
			break;
		case 29459:
			FORC4 p->cam_mul[c ^ (c >> 1)] = dcr_get2(p);
			break;
		case 33405:			/* Model2 */
			dcr_fgets(p->obj_, p->model2, 64);
			break;
		case 33422:			/* CFAPattern */
		case 64777:			/* Kodak P-series */
			if ((plen=len) > 16) plen = 16;
			dcr_fread(p->obj_, cfa_pat, 1, plen);
			for (p->colors=cfa=i=0; i < (int)plen; i++) {
				p->colors += !(cfa & (1 << cfa_pat[i]));
				cfa |= 1 << cfa_pat[i];
			}
			if (cfa == 070) memcpy (cfa_pc,"\003\004\005",3);	/* CMY */
			if (cfa == 072) memcpy (cfa_pc,"\005\003\004\001",4);	/* GMCY */
			goto guess_cfa_pc;
		case 33424:
			dcr_fseek(p->obj_, dcr_get4(p)+base, SEEK_SET);
			dcr_parse_kodak_ifd (p,base);
			break;
		case 33434:			/* ExposureTime */
			p->shutter = (float)dcr_getreal(p,type);
			break;
		case 33437:			/* FNumber */
			p->aperture = (float)dcr_getreal(p,type);
			break;
		case 34306:			/* Leaf p->white balance */
			FORC4 p->cam_mul[c ^ 1] = 4096.0f / dcr_get2(p);
			break;
		case 34307:			/* Leaf CatchLight color matrix */
			dcr_fread(p->obj_, software, 1, 7);
			if (strncmp(software,"MATRIX",6)) break;
			p->colors = 4;
			for (p->raw_color = i=0; i < 3; i++) {
				FORC4 dcr_fscanf(p->obj_, "%f", &p->rgb_cam[i][c^1]);
				if (!p->opt.use_camera_wb) continue;
				num = 0;
				FORC4 num += p->rgb_cam[i][c];
				FORC4 p->rgb_cam[i][c] /= (float)num;
			}
			break;
		case 34310:			/* Leaf metadata */
			dcr_parse_mos (p,dcr_ftell(p->obj_));
		case 34303:
			strcpy (p->make, "Leaf");
			break;
		case 34665:			/* EXIF tag */
			dcr_fseek(p->obj_, dcr_get4(p)+base, SEEK_SET);
			dcr_parse_exif (p,base);
			break;
		case 34853:			/* GPSInfo tag */
			dcr_fseek(p->obj_, dcr_get4(p)+base, SEEK_SET);
			dcr_parse_gps (p,base);
			break;
		case 34675:			/* InterColorProfile */
		case 50831:			/* AsShotICCProfile */
			p->profile_offset = dcr_ftell(p->obj_);
			p->profile_length = len;
			break;
		case 37122:			/* CompressedBitsPerPixel */
			p->kodak_cbpp = dcr_get4(p);
			break;
		case 37386:			/* FocalLength */
			p->focal_len = (float)dcr_getreal(p,type);
			break;
		case 37393:			/* ImageNumber */
			p->shot_order = dcr_getint(p, type);
			break;
		case 37400:			/* old Kodak KDC tag */
			for (p->raw_color = i=0; i < 3; i++) {
				dcr_getreal(p,type);
				FORC3 p->rgb_cam[i][c] = (float)dcr_getreal(p,type);
			}
			break;
		case 46275:			/* Imacon tags */
			strcpy (p->make, "Imacon");
			p->data_offset = dcr_ftell(p->obj_);
			ima_len = len;
			break;
		case 46279:
			if (!ima_len) break;
			dcr_fseek(p->obj_, 78, SEEK_CUR);
			p->raw_width  = dcr_get4(p);
			p->raw_height = dcr_get4(p);
			p->left_margin = dcr_get4(p) & 7;
			p->width = p->raw_width - p->left_margin - (dcr_get4(p) & 7);
			p->top_margin = dcr_get4(p) & 7;
			p->height = p->raw_height - p->top_margin - (dcr_get4(p) & 7);
			if (p->raw_width == 7262) {
				p->height = 5444;
				p->width  = 7244;
				p->left_margin = 7;
			}
			dcr_fseek(p->obj_, 52, SEEK_CUR);
			FORC3 p->cam_mul[c] = (float)dcr_getreal(p, 11);
			dcr_fseek(p->obj_, 114, SEEK_CUR);
			p->flip = (dcr_get2(p) >> 7) * 90;
			if (p->width * p->height * 6 == ima_len) {
				if (p->flip % 180 == 90) SWAP(p->width,p->height);
				p->filters = p->flip = 0;
			}
			sprintf (p->model, "Ixpress %d-Mp", p->height*p->width/1000000);
			p->load_raw = &DCR_CLASS dcr_imacon_full_load_raw;
			if (p->filters) {
				if (p->left_margin & 1) p->filters = 0x61616161;
				p->load_raw = &DCR_CLASS dcr_unpacked_load_raw;
			}
			p->maximum = 0xffff;
			break;
		case 50454:			/* Sinar tag */
		case 50455:
			if (!(cbuf = (char *) malloc(len))) break;
			dcr_fread(p->obj_, cbuf, 1, len);
			for (cp = cbuf-1; cp && cp < cbuf+len; cp = strchr(cp,'\n'))
				if (!strncmp (++cp,"Neutral ",8))
					sscanf (cp+8, "%f %f %f", p->cam_mul, p->cam_mul+1, p->cam_mul+2);
			free (cbuf);
			break;
		case 50458:
			if (!p->make[0]) strcpy (p->make, "Hasselblad");
			break;
		case 50459:			/* Hasselblad tag */
			i = p->order;
			j = dcr_ftell(p->obj_);
			c = p->tiff_nifds;
			p->order = dcr_get2(p);
			dcr_fseek(p->obj_, j+(dcr_get2(p),dcr_get4(p)), SEEK_SET);
			dcr_parse_tiff_ifd (p, j);
			p->maximum = 0xffff;
			p->tiff_nifds = c;
			p->order = i;
			break;
		case 50706:			/* DNGVersion */
			FORC4 p->dng_version = (p->dng_version << 8) + dcr_fgetc(p->obj_);
			if (!p->make[0]) strcpy (p->make, "DNG");
			p->is_raw = 1;
			break;
		case 50710:			/* CFAPlaneColor */
			if (len > 4) len = 4;
			p->colors = len;
			dcr_fread(p->obj_, cfa_pc, 1, p->colors);
guess_cfa_pc:
			FORCC(p) tab[cfa_pc[c]] = c;
			p->cdesc[c] = 0;
			for (i=16; i--; )
				p->filters = p->filters << 2 | tab[cfa_pat[i % plen]];
			break;
		case 50711:			/* CFALayout */
			if (dcr_get2(p) == 2) {
				p->fuji_width = 1;
				p->filters = 0x49494949;
			}
			break;
		case 291:
		case 50712:			/* LinearizationTable */
			dcr_linear_table (p,len);
			break;
		case 50714:			/* BlackLevel */
		case 50715:			/* BlackLevelDeltaH */
		case 50716:			/* BlackLevelDeltaV */
			for (dblack=i=0; i < (int)len && i < 65536; i++)
				dblack += dcr_getreal(p, type);
			p->black += (unsigned int)(dblack/len + 0.5);
			break;
		case 50717:			/* WhiteLevel */
			p->maximum = dcr_getint(p, type);
			break;
		case 50718:			/* DefaultScale */
			p->pixel_aspect  = dcr_getreal(p,type);
			p->pixel_aspect /= dcr_getreal(p,type);
			break;
		case 50721:			/* ColorMatrix1 */
		case 50722:			/* ColorMatrix2 */
			FORCC(p) for (j=0; j < 3; j++)
				cm[c][j] = dcr_getreal(p,type);
			use_cm = 1;
			break;
		case 50723:			/* CameraCalibration1 */
		case 50724:			/* CameraCalibration2 */
			for (i=0; i < p->colors; i++)
				FORCC(p) cc[i][c] = dcr_getreal(p,type);
		case 50727:			/* AnalogBalance */
			FORCC(p) ab[c] = dcr_getreal(p,type);
			break;
		case 50728:			/* AsShotNeutral */
			FORCC(p) asn[c] = dcr_getreal(p, type);
			break;
		case 50729:			/* AsShotWhiteXY */
			xyz[0] = dcr_getreal(p,type);
			xyz[1] = dcr_getreal(p,type);
			xyz[2] = 1 - xyz[0] - xyz[1];
			FORC3 xyz[c] /= d65_white[c];
			break;
		case 50740:			/* DNGPrivateData */
			if (p->dng_version) break;
			dcr_parse_minolta (p, j = dcr_get4(p)+base);
			dcr_fseek(p->obj_, j, SEEK_SET);
			dcr_parse_tiff_ifd (p, base);
			break;
		case 50752:
			dcr_read_shorts (p, p->cr2_slice, 3);
			break;
		case 50829:			/* ActiveArea */
			p->top_margin = dcr_getint(p, type);
			p->left_margin = dcr_getint(p, type);
			p->height = dcr_getint(p, type) - p->top_margin;
			p->width = dcr_getint(p, type) - p->left_margin;
			break;
		case 64772:			/* Kodak P-series */
			dcr_fseek(p->obj_, 16, SEEK_CUR);
			p->data_offset = dcr_get4(p);
			dcr_fseek(p->obj_, 28, SEEK_CUR);
			p->data_offset += dcr_get4(p);
			p->load_raw = &DCR_CLASS dcr_packed_12_load_raw;
    }
    dcr_fseek(p->obj_, save, SEEK_SET);
  }
  if (sony_length && (buf = (unsigned *) malloc(sony_length))) {
	  dcr_stream_ops *sops_;
	  dcr_stream_obj *sobj_;
	  dcr_fseek(p->obj_, sony_offset, SEEK_SET);
	  dcr_fread(p->obj_, buf, sony_length, 1);
	  dcr_sony_decrypt (p, buf, sony_length/4, 1, sony_key);

	  sops_ = p->ops_;
	  sobj_ = p->obj_;
  	  p->ops_ = &dcr_stream_fileops;
	  if ((p->obj_ = tmpfile())) {
		  dcr_fwrite(p->obj_, buf, sony_length, 1);
		  dcr_fseek(p->obj_, 0, SEEK_SET);
		  dcr_parse_tiff_ifd (p, -(int)sony_offset);
		  dcr_fclose(p->obj_);
	  }
	  p->ops_ = sops_;
	  p->obj_ = sobj_;

	  free (buf);
  }
  for (i=0; i < p->colors; i++)
	  FORCC(p) cc[i][c] *= ab[i];
  if (use_cm) {
	  FORCC(p) for (i=0; i < 3; i++)
		  for (cam_xyz[c][i]=j=0; j < p->colors; j++)
			  cam_xyz[c][i] += cc[c][j] * cm[j][i] * xyz[i];
		  dcr_cam_xyz_coeff (p, cam_xyz);
  }
  if (asn[0]) {
	  p->cam_mul[3] = 0;
	  FORCC(p) p->cam_mul[c] = 1.0f / (float)asn[c];
  }
  if (!use_cm)
	  FORCC(p) p->pre_mul[c] /= (float)cc[c][c];
  return 0;
}

void DCR_CLASS dcr_parse_tiff (DCRAW* p, int base)
{
	int doff, max_samp=0, raw=-1, thm=-1, i;
	struct dcr_jhead jh;

	dcr_fseek(p->obj_, base, SEEK_SET);
	p->order = dcr_get2(p);
	if (p->order != 0x4949 && p->order != 0x4d4d) return;
	dcr_get2(p);
	memset (p->tiff_ifd, 0, sizeof p->tiff_ifd);
	p->tiff_nifds = 0;
	while ((doff = dcr_get4(p))) {
		dcr_fseek(p->obj_, doff+base, SEEK_SET);
		if (dcr_parse_tiff_ifd (p, base)) break;
	}
	p->thumb_misc = 16;
	if (p->thumb_offset) {
		dcr_fseek(p->obj_, p->thumb_offset, SEEK_SET);
		if (dcr_ljpeg_start (p,&jh, 1)) {
			if ((unsigned)jh.bits<17 && (unsigned)jh.wide < 0x10000 && (unsigned)jh.high < 0x10000) {
				p->thumb_misc   = jh.bits;
				p->thumb_width  = jh.wide;
				p->thumb_height = jh.high;
			}
		}
	}
	for (i=0; i < (int)p->tiff_nifds; i++) {
		if (max_samp < p->tiff_ifd[i].samples)
			max_samp = p->tiff_ifd[i].samples;
		if (max_samp > 3) max_samp = 3;
		if ((p->tiff_ifd[i].comp != 6 || p->tiff_ifd[i].samples != 3) &&
		        (unsigned)(p->tiff_ifd[i].width | p->tiff_ifd[i].height) < 0x10000 &&
			(unsigned)p->tiff_ifd[i].bps < 33 && (unsigned)p->tiff_ifd[i].samples < 13 &&
			p->tiff_ifd[i].width*p->tiff_ifd[i].height > p->raw_width*p->raw_height) {
			p->raw_width     = p->tiff_ifd[i].width;
			p->raw_height    = p->tiff_ifd[i].height;
			p->tiff_bps      = p->tiff_ifd[i].bps;
			p->tiff_compress = p->tiff_ifd[i].comp;
			p->data_offset   = p->tiff_ifd[i].offset;
			p->tiff_flip     = p->tiff_ifd[i].flip;
			p->tiff_samples  = p->tiff_ifd[i].samples;
			raw = i;
		}
	}
	p->fuji_width *= (p->raw_width+1)/2;
	if (p->tiff_ifd[0].flip) p->tiff_flip = p->tiff_ifd[0].flip;
	if (raw >= 0 && !p->load_raw)
		switch (p->tiff_compress) {
	case 0:  case 1:
		switch (p->tiff_bps) {
		case  8: p->load_raw = &DCR_CLASS dcr_eight_bit_load_raw;	break;
		case 12: p->load_raw = &DCR_CLASS dcr_packed_12_load_raw;
			if (p->tiff_ifd[raw].phint == 2)
				p->load_flags = 6;
			if (strncmp(p->make,"PENTAX",6)) break;
		case 14:
		case 16: p->load_raw = &DCR_CLASS dcr_unpacked_load_raw;		break;
		}
		if (p->tiff_ifd[raw].bytes*5 == p->raw_width*p->raw_height*8)
			p->load_raw = &DCR_CLASS dcr_olympus_e300_load_raw;
		break;
		case 6:  case 7:  case 99:
			p->load_raw = &DCR_CLASS dcr_lossless_jpeg_load_raw;		break;
		case 262:
			p->load_raw = &DCR_CLASS dcr_kodak_262_load_raw;			break;
		case 32767:
			p->load_raw = &DCR_CLASS dcr_sony_arw2_load_raw;
			if (p->tiff_ifd[raw].bytes*8 == (int)(p->raw_width*p->raw_height*p->tiff_bps))
				break;
			p->raw_height += 8;
			p->load_raw = &DCR_CLASS dcr_sony_arw_load_raw;			break;
		case 32769:
			p->load_flags = 8;
		case 32773:
			p->load_raw = &DCR_CLASS dcr_packed_12_load_raw;			break;
		case 34713:
			p->load_raw = &DCR_CLASS dcr_nikon_compressed_load_raw;		break;
		case 65535:
			p->load_raw = &DCR_CLASS dcr_pentax_k10_load_raw;			break;
		case 65000:
			switch (p->tiff_ifd[raw].phint) {
			case 2: p->load_raw = &DCR_CLASS dcr_kodak_rgb_load_raw;   p->filters = 0;  break;
			case 6: p->load_raw = &DCR_CLASS dcr_kodak_ycbcr_load_raw; p->filters = 0;  break;
			case 32803: p->load_raw = &DCR_CLASS dcr_kodak_65000_load_raw;
			}
			case 32867: break;
			default: p->is_raw = 0;
    }
	if (!p->dng_version && p->tiff_samples == 3)
		if (p->tiff_ifd[raw].bytes && p->tiff_bps != 14 && p->tiff_bps != 2048)
			p->is_raw = 0;
	if (!p->dng_version && p->tiff_bps == 8 && p->tiff_compress == 1 &&
		p->tiff_ifd[raw].phint == 1) p->is_raw = 0;
	if (p->tiff_bps == 8 && p->tiff_samples == 4) p->is_raw = 0;
	for (i=0; i < (int)p->tiff_nifds; i++)
		if (i != raw && p->tiff_ifd[i].samples == max_samp &&
			p->tiff_ifd[i].bps>0 && p->tiff_ifd[i].bps < 33 &&
			(unsigned)(p->tiff_ifd[i].width | p->tiff_ifd[i].height) < 0x10000 &&
			p->tiff_ifd[i].width * p->tiff_ifd[i].height / SQR(p->tiff_ifd[i].bps+1) >
			(int)(p->thumb_width *       p->thumb_height / SQR(p->thumb_misc+1))) {
			p->thumb_width  = p->tiff_ifd[i].width;
			p->thumb_height = p->tiff_ifd[i].height;
			p->thumb_offset = p->tiff_ifd[i].offset;
			p->thumb_length = p->tiff_ifd[i].bytes;
			p->thumb_misc   = p->tiff_ifd[i].bps;
			thm = i;
		}
	if (thm >= 0) {
		p->thumb_misc |= p->tiff_ifd[thm].samples << 5;
		switch (p->tiff_ifd[thm].comp) {
		case 0:
			p->write_thumb = &DCR_CLASS dcr_layer_thumb;
			break;
		case 1:
			if (p->tiff_ifd[thm].bps > 8)
				p->thumb_load_raw = &DCR_CLASS dcr_kodak_thumb_load_raw;
			else
				p->write_thumb = &DCR_CLASS dcr_ppm_thumb;
			break;
		case 65000:
			p->thumb_load_raw = p->tiff_ifd[thm].phint == 6 ?
				&DCR_CLASS dcr_kodak_ycbcr_load_raw : &DCR_CLASS dcr_kodak_rgb_load_raw;
		}
	}
}

void DCR_CLASS dcr_parse_minolta (DCRAW* p, int base)
{
	int save, tag, len, offset, high=0, wide=0, i, c;
	short sorder=p->order;

	dcr_fseek(p->obj_, base, SEEK_SET);
	if (dcr_fgetc(p->obj_) || dcr_fgetc(p->obj_)-'M' || dcr_fgetc(p->obj_)-'R') return;
	p->order = dcr_fgetc(p->obj_) * 0x101;
	offset = base + dcr_get4(p) + 8;
	while ((save=dcr_ftell(p->obj_)) < offset) {
		for (tag=i=0; i < 4; i++)
			tag = tag << 8 | dcr_fgetc(p->obj_);
		len = dcr_get4(p);
		switch (tag) {
		case 0x505244:				/* PRD */
			dcr_fseek(p->obj_, 8, SEEK_CUR);
			high = dcr_get2(p);
			wide = dcr_get2(p);
			break;
		case 0x574247:				/* WBG */
			dcr_get4(p);
			i = strcmp(p->model,"DiMAGE A200") ? 0:3;
			FORC4 p->cam_mul[c ^ (c >> 1) ^ i] = dcr_get2(p);
			break;
		case 0x545457:				/* TTW */
			dcr_parse_tiff (p, dcr_ftell(p->obj_));
			p->data_offset = offset;
		}
		dcr_fseek(p->obj_, save+len+8, SEEK_SET);
	}
	p->raw_height = high;
	p->raw_width  = wide;
	p->order = sorder;
}

/*
Many cameras have a "debug mode" that writes JPEG and raw
at the same time.  The raw file has no header, so try to
to open the matching JPEG file and read its metadata.
*/
void DCR_CLASS dcr_parse_external_jpeg(DCRAW* p)
{
	char *file, *ext, *jname, *jfile, *jext;
	dcr_stream_ops *sops_;
	dcr_stream_obj *sobj_;

	ext  = strrchr (p->ifname, '.');
	file = strrchr (p->ifname, '/');
	if (!file) file = strrchr (p->ifname, '\\');
	if (!file) file = p->ifname-1;
	file++;
	if (!ext || strlen(ext) != 4 || ext-file != 8) return;
	jname = (char *) malloc (strlen(p->ifname) + 1);
	dcr_merror (p, jname, "parse_external_jpeg()");
	strcpy (jname, p->ifname);
	jfile = file - p->ifname + jname;
	jext  = ext  - p->ifname + jname;
	if (strcasecmp (ext, ".jpg")) {
		strcpy (jext, isupper(ext[1]) ? ".JPG":".jpg");
		if (isdigit(*file)) {
			memcpy (jfile, file+4, 4);
			memcpy (jfile+4, file, 4);
		}
	} else
		while (isdigit(*--jext)) {
			if (*jext != '9') {
				(*jext)++;
				break;
			}
			*jext = '0';
		}
	if (strcmp (jname, p->ifname)) {

		sops_ = p->ops_;
		sobj_ = p->obj_;
  		p->ops_ = &dcr_stream_fileops;

		if ((p->obj_ = fopen (jname, "rb"))) {
			if (p->opt.verbose)
				fprintf (stderr,_("Reading metadata from %s ...\n"), jname);
			dcr_parse_tiff (p, 12);
			p->thumb_offset = 0;
			p->is_raw = 1;
			dcr_fclose(p->obj_);
		}

		p->ops_ = sops_;
		p->obj_ = sobj_;

	}
	if (!p->timestamp)
		fprintf (stderr,_("Failed to read metadata from %s\n"), jname);
	free (jname);
}

/*
CIFF block 0x1030 contains an 8x8 p->white sample.
Load this into p->white[][] for use in scale_colors().
*/
void DCR_CLASS dcr_ciff_block_1030(DCRAW* p)
{
	static const ushort key[] = { 0x410, 0x45f3 };
	int i, bpp, row, col, vbits=0;
	unsigned long bitbuf=0;

	if ((dcr_get2(p),dcr_get4(p)) != 0x80008 || !dcr_get4(p)) return;
	bpp = dcr_get2(p);
	if (bpp != 10 && bpp != 12) return;
	for (i=row=0; row < 8; row++)
		for (col=0; col < 8; col++) {
			if (vbits < bpp) {
				bitbuf = bitbuf << 16 | (dcr_get2(p) ^ key[i++ & 1]);
				vbits += 16;
			}
			p->white[row][col] = (unsigned short)(
				bitbuf << (LONG_BIT - vbits) >> (LONG_BIT - bpp));
			vbits -= bpp;
		}
}

/*
Parse a CIFF file, better known as Canon CRW format.
*/
void DCR_CLASS dcr_parse_ciff (DCRAW* p, int offset, int length)
{
	int tboff, nrecs, c, type, len, save, wbi=-1;
	ushort key[] = { 0x410, 0x45f3 };

	dcr_fseek(p->obj_, offset+length-4, SEEK_SET);
	tboff = dcr_get4(p) + offset;
	dcr_fseek(p->obj_, tboff, SEEK_SET);
	nrecs = dcr_get2(p);
	if (nrecs > 100) return;
	while (nrecs--) {
		type = dcr_get2(p);
		len  = dcr_get4(p);
		save = dcr_ftell(p->obj_) + 4;
		dcr_fseek(p->obj_, offset+dcr_get4(p), SEEK_SET);
		if ((((type >> 8) + 8) | 8) == 0x38)
			dcr_parse_ciff (p,dcr_ftell(p->obj_), len);	/* Parse a sub-table */

		if (type == 0x0810)
			dcr_fread(p->obj_, p->artist, 64, 1);
		if (type == 0x080a) {
			dcr_fread(p->obj_, p->make, 64, 1);
			dcr_fseek(p->obj_, strlen(p->make) - 63, SEEK_CUR);
			dcr_fread(p->obj_, p->model, 64, 1);
		}
		if (type == 0x1810) {
			dcr_fseek(p->obj_, 12, SEEK_CUR);
			p->flip = dcr_get4(p);
		}
		if (type == 0x1835)			/* Get the decoder table */
			p->tiff_compress = dcr_get4(p);
		if (type == 0x2007) {
			p->thumb_offset = dcr_ftell(p->obj_);
			p->thumb_length = len;
		}
		if (type == 0x1818) {
			p->shutter = (float)pow (2, -dcr_int_to_float((dcr_get4(p),dcr_get4(p))));
			p->aperture = (float)pow (2, dcr_int_to_float(dcr_get4(p))/2);
		}
		if (type == 0x102a) {
			p->iso_speed = (float)pow (2, (dcr_get4(p),dcr_get2(p))/32.0f - 4) * 50;
			p->aperture  = (float)pow (2, (dcr_get2(p),(short)dcr_get2(p))/64.0f);
			p->shutter   = (float)pow (2,-((short)dcr_get2(p))/32.0f);
			wbi = (dcr_get2(p),dcr_get2(p));
			if (wbi > 17) wbi = 0;
			dcr_fseek(p->obj_, 32, SEEK_CUR);
			if (p->shutter > 1e6) p->shutter = dcr_get2(p)/10.0f;
		}
		if (type == 0x102c) {
			if (dcr_get2(p) > 512) {		/* Pro90, G1 */
				dcr_fseek(p->obj_, 118, SEEK_CUR);
				FORC4 p->cam_mul[c ^ 2] = dcr_get2(p);
			} else {				/* G2, S30, S40 */
				dcr_fseek(p->obj_, 98, SEEK_CUR);
				FORC4 p->cam_mul[c ^ (c >> 1) ^ 1] = dcr_get2(p);
			}
		}
		if (type == 0x0032) {
			if (len == 768) {			/* EOS D30 */
				dcr_fseek(p->obj_, 72, SEEK_CUR);
				FORC4 p->cam_mul[c ^ (c >> 1)] = 1024.0f / dcr_get2(p);
				if (!wbi) p->cam_mul[0] = -1;	/* use my auto p->white balance */
			} else if (!p->cam_mul[0]) {
				if (dcr_get2(p) == key[0])		/* Pro1, G6, S60, S70 */
					c = (strstr(p->model,"Pro1") ?
					"012346000000000000":"01345:000000006008")[wbi]-'0'+ 2;
				else {				/* G3, G5, S45, S50 */
					c = "023457000000006000"[wbi]-'0';
					key[0] = key[1] = 0;
				}
				dcr_fseek(p->obj_, 78 + c*8, SEEK_CUR);
				FORC4 p->cam_mul[c ^ (c >> 1) ^ 1] = (float)(dcr_get2(p) ^ key[c & 1]);
				if (!wbi) p->cam_mul[0] = -1;
			}
		}
		if (type == 0x10a9) {		/* D60, 10D, 300D, and clones */
			if (len > 66) wbi = "0134567028"[wbi]-'0';
			dcr_fseek(p->obj_, 2 + wbi*8, SEEK_CUR);
			FORC4 p->cam_mul[c ^ (c >> 1)] = dcr_get2(p);
		}
		if (type == 0x1030 && (0x18040 >> wbi & 1))
			dcr_ciff_block_1030(p);		/* all that don't have 0x10a9 */
		if (type == 0x1031) {
			p->raw_width = (dcr_get2(p),dcr_get2(p));
			p->raw_height = dcr_get2(p);
		}
		if (type == 0x5029) {
			p->focal_len = (float)(len >> 16);
			if ((len & 0xffff) == 2) p->focal_len /= 32;
		}
		if (type == 0x5813) p->flash_used = dcr_int_to_float(len);
		if (type == 0x5814) p->canon_ev   = dcr_int_to_float(len);
		if (type == 0x5817) p->shot_order = len;
		if (type == 0x5834) p->unique_id  = len;
		if (type == 0x580e) p->timestamp  = len;
		if (type == 0x180e) p->timestamp  = dcr_get4(p);
#ifdef LOCALTIME
		if ((type | 0x4000) == 0x580e)
			p->timestamp = mktime (gmtime (&p->timestamp));
#endif
		dcr_fseek(p->obj_, save, SEEK_SET);
	}
}

void DCR_CLASS dcr_parse_rollei(DCRAW* p)
{
	char line[128], *val;
	struct tm t;

	dcr_fseek(p->obj_, 0, SEEK_SET);
	memset (&t, 0, sizeof t);
	do {
		dcr_fgets(p->obj_, line, 128);
		if ((val = strchr(line,'=')))
			*val++ = 0;
		else
			val = line + strlen(line);
		if (!strcmp(line,"DAT"))
			sscanf (val, "%d.%d.%d", &t.tm_mday, &t.tm_mon, &t.tm_year);
		if (!strcmp(line,"TIM"))
			sscanf (val, "%d:%d:%d", &t.tm_hour, &t.tm_min, &t.tm_sec);
		if (!strcmp(line,"HDR"))
			p->thumb_offset = atoi(val);
		if (!strcmp(line,"X  "))
			p->raw_width = atoi(val);
		if (!strcmp(line,"Y  "))
			p->raw_height = atoi(val);
		if (!strcmp(line,"TX "))
			p->thumb_width = atoi(val);
		if (!strcmp(line,"TY "))
			p->thumb_height = atoi(val);
	} while (strncmp(line,"EOHD",4));
	p->data_offset = p->thumb_offset + p->thumb_width * p->thumb_height * 2;
	t.tm_year -= 1900;
	t.tm_mon -= 1;
	if (mktime(&t) > 0)
		p->timestamp = mktime(&t);
	strcpy (p->make, "Rollei");
	strcpy (p->model,"d530flex");
	p->write_thumb = &DCR_CLASS dcr_rollei_thumb;
}

void DCR_CLASS dcr_parse_sinar_ia(DCRAW* p)
{
	int entries, off;
	char str[8], *cp;

	p->order = 0x4949;
	dcr_fseek(p->obj_, 4, SEEK_SET);
	entries = dcr_get4(p);
	dcr_fseek(p->obj_, dcr_get4(p), SEEK_SET);
	while (entries--) {
		off = dcr_get4(p); dcr_get4(p);
		dcr_fread(p->obj_, str, 8, 1);
		if (!strcmp(str,"META"))   p->meta_offset = off;
		if (!strcmp(str,"THUMB")) p->thumb_offset = off;
		if (!strcmp(str,"RAW0"))   p->data_offset = off;
	}
	dcr_fseek(p->obj_, p->meta_offset+20, SEEK_SET);
	dcr_fread(p->obj_, p->make, 64, 1);
	p->make[63] = 0;
	if ((cp = strchr(p->make,' '))) {
		strcpy (p->model, cp+1);
		*cp = 0;
	}
	p->raw_width  = dcr_get2(p);
	p->raw_height = dcr_get2(p);
	p->load_raw = &DCR_CLASS dcr_unpacked_load_raw;
	p->thumb_width = (dcr_get4(p),dcr_get2(p));
	p->thumb_height = dcr_get2(p);
	p->write_thumb = &DCR_CLASS dcr_ppm_thumb;
	p->maximum = 0x3fff;
}

void DCR_CLASS dcr_parse_phase_one (DCRAW* p, int base)
{
	unsigned entries, tag, type, len, data, save, i, c;
	float romm_cam[3][3];
	char *cp;

	memset (&p->ph1, 0, sizeof p->ph1);
	dcr_fseek(p->obj_, base, SEEK_SET);
	p->order = dcr_get4(p) & 0xffff;
	if (dcr_get4(p) >> 8 != 0x526177) return;		/* "Raw" */
	dcr_fseek(p->obj_, dcr_get4(p)+base, SEEK_SET);
	entries = dcr_get4(p);
	dcr_get4(p);
	while (entries--) {
		tag  = dcr_get4(p);
		type = dcr_get4(p);
		len  = dcr_get4(p);
		data = dcr_get4(p);
		save = dcr_ftell(p->obj_);
		dcr_fseek(p->obj_, base+data, SEEK_SET);
		switch (tag) {
		case 0x100:  p->flip = "0653"[data & 3]-'0';  break;
		case 0x106:
			for (i=0; i < 9; i++)
				romm_cam[0][i] = (float)dcr_getreal(p, 11);
			dcr_romm_coeff (p,romm_cam);
			break;
		case 0x107:
			FORC3 p->cam_mul[c] = (float)dcr_getreal(p, 11);
			break;
		case 0x108:  p->raw_width     = data;	break;
		case 0x109:  p->raw_height    = data;	break;
		case 0x10a:  p->left_margin   = data;	break;
		case 0x10b:  p->top_margin    = data;	break;
		case 0x10c:  p->width         = data;	break;
		case 0x10d:  p->height        = data;	break;
		case 0x10e:  p->ph1.format    = data;	break;
		case 0x10f:  p->data_offset   = data+base;	break;
		case 0x110:  p->meta_offset   = data+base;
			p->meta_length   = len;			break;
		case 0x112:  p->ph1.key_off   = save - 4;		break;
		case 0x210:  p->ph1.tag_210   = dcr_int_to_float(data);	break;
		case 0x21a:  p->ph1.tag_21a   = data;		break;
		case 0x21c:  p->strip_offset  = data+base;		break;
		case 0x21d:  p->ph1.black     = data;		break;
		case 0x222:  p->ph1.split_col = data - p->left_margin;	break;
		case 0x223:  p->ph1.black_off = data+base;		break;
		case 0x301:
			p->model[63] = 0;
			dcr_fread(p->obj_, p->model, 1, 63);
			if ((cp = strstr(p->model," camera"))) *cp = 0;
		}
		dcr_fseek(p->obj_, save, SEEK_SET);
	}
	p->load_raw = p->ph1.format < 3 ?
		&DCR_CLASS dcr_phase_one_load_raw : &DCR_CLASS dcr_phase_one_load_raw_c;
	p->maximum = 0xffff;
	strcpy (p->make, "Phase One");
	if (p->model[0]) return;
	switch (p->raw_height) {
    case 2060: strcpy (p->model,"LightPhase");	break;
    case 2682: strcpy (p->model,"H 10");		break;
    case 4128: strcpy (p->model,"H 20");		break;
    case 5488: strcpy (p->model,"H 25");		break;
	}
}

void DCR_CLASS dcr_parse_fuji (DCRAW* p, int offset)
{
	unsigned entries, tag, len, save, c;

	dcr_fseek(p->obj_, offset, SEEK_SET);
	entries = dcr_get4(p);
	if (entries > 255) return;
	while (entries--) {
		tag = dcr_get2(p);
		len = dcr_get2(p);
		save = dcr_ftell(p->obj_);
		if (tag == 0x100) {
			p->raw_height = dcr_get2(p);
			p->raw_width  = dcr_get2(p);
		} else if (tag == 0x121) {
			p->height = dcr_get2(p);
			if ((p->width = dcr_get2(p)) == 4284) p->width += 3;
		} else if (tag == 0x130)
			p->fuji_layout = dcr_fgetc(p->obj_) >> 7;
		if (tag == 0x2ff0)
			FORC4 p->cam_mul[c ^ 1] = dcr_get2(p);
		dcr_fseek(p->obj_, save+len, SEEK_SET);
	}
	p->height <<= p->fuji_layout;
	p->width  >>= p->fuji_layout;
}

int DCR_CLASS dcr_parse_jpeg (DCRAW* p, int offset)
{
	int len, save, hlen, mark;

	dcr_fseek(p->obj_, offset, SEEK_SET);
	if (dcr_fgetc(p->obj_) != 0xff || dcr_fgetc(p->obj_) != 0xd8) return 0;

	while (dcr_fgetc(p->obj_) == 0xff && (mark = dcr_fgetc(p->obj_)) != 0xda) {
		p->order = 0x4d4d;
		len   = dcr_get2(p) - 2;
		save  = dcr_ftell(p->obj_);
		if (mark == 0xc0 || mark == 0xc3) {
			dcr_fgetc(p->obj_);
			p->raw_height = dcr_get2(p);
			p->raw_width  = dcr_get2(p);
		}
		p->order = dcr_get2(p);
		hlen  = dcr_get4(p);
		if (dcr_get4(p) == 0x48454150)		/* "HEAP" */
			dcr_parse_ciff (p,save+hlen, len-hlen);
		dcr_parse_tiff (p, save+6);
		dcr_fseek(p->obj_, save+len, SEEK_SET);
	}
	return 1;
}

void DCR_CLASS dcr_parse_riff(DCRAW* p)
{
	unsigned i, size, end;
	char tag[4], date[64], month[64];
	static const char mon[12][4] =
	{ "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };
	struct tm t;

	p->order = 0x4949;
	dcr_fread(p->obj_, tag, 4, 1);
	size = dcr_get4(p);
	end = dcr_ftell(p->obj_) + size;
	if (!memcmp(tag,"RIFF",4) || !memcmp(tag,"LIST",4)) {
		dcr_get4(p);
		while (dcr_ftell(p->obj_)+7 < (long)end)
			dcr_parse_riff(p);
	} else if (!memcmp(tag,"nctg",4)) {
		while (dcr_ftell(p->obj_)+7 < (long)end) {
			i = dcr_get2(p);
			size = dcr_get2(p);
			if ((i+1) >> 1 == 10 && size == 20)
				dcr_get_timestamp(p,0);
			else dcr_fseek(p->obj_, size, SEEK_CUR);
		}
	} else if (!memcmp(tag,"IDIT",4) && size < 64) {
		dcr_fread(p->obj_, date, 64, 1);
		date[size] = 0;
		memset (&t, 0, sizeof t);
		if (sscanf (date, "%*s %s %d %d:%d:%d %d", month, &t.tm_mday,
			&t.tm_hour, &t.tm_min, &t.tm_sec, &t.tm_year) == 6) {
			for (i=0; i < 12 && strcasecmp(mon[i],month); i++);
			t.tm_mon = i;
			t.tm_year -= 1900;
			if (mktime(&t) > 0)
				p->timestamp = mktime(&t);
		}
	} else
		dcr_fseek(p->obj_, size, SEEK_CUR);
}

void DCR_CLASS dcr_parse_smal (DCRAW* p, int offset, int fsize)
{
	int ver;

	dcr_fseek(p->obj_, offset+2, SEEK_SET);
	p->order = 0x4949;
	ver = dcr_fgetc(p->obj_);
	if (ver == 6)
		dcr_fseek(p->obj_, 5, SEEK_CUR);
	if ((int)dcr_get4(p) != fsize) return;
	if (ver > 6) p->data_offset = dcr_get4(p);
	p->raw_height = p->height = dcr_get2(p);
	p->raw_width  = p->width  = dcr_get2(p);
	strcpy (p->make, "SMaL");
	sprintf (p->model, "v%d %dx%d", ver, p->width, p->height);
	if (ver == 6) p->load_raw = &DCR_CLASS dcr_smal_v6_load_raw;
	if (ver == 9) p->load_raw = &DCR_CLASS dcr_smal_v9_load_raw;
}

void DCR_CLASS dcr_parse_cine(DCRAW* p)
{
	unsigned off_head, off_setup, off_image, i;

	p->order = 0x4949;
	dcr_fseek(p->obj_, 4, SEEK_SET);
	p->is_raw = dcr_get2(p) == 2;
	dcr_fseek(p->obj_, 14, SEEK_CUR);
	p->is_raw *= dcr_get4(p);
	off_head = dcr_get4(p);
	off_setup = dcr_get4(p);
	off_image = dcr_get4(p);
	p->timestamp = dcr_get4(p);
	if ((i = dcr_get4(p))) p->timestamp = i;
	dcr_fseek(p->obj_, off_head+4, SEEK_SET);
	p->raw_width = dcr_get4(p);
	p->raw_height = dcr_get4(p);
	switch (dcr_get2(p),dcr_get2(p)) {
    case  8:  p->load_raw = &DCR_CLASS dcr_eight_bit_load_raw;  break;
    case 16:  p->load_raw = &DCR_CLASS dcr_unpacked_load_raw;
	}
	dcr_fseek(p->obj_, off_setup+792, SEEK_SET);
	strcpy (p->make, "CINE");
	sprintf (p->model, "%d", dcr_get4(p));
	dcr_fseek(p->obj_, 12, SEEK_CUR);
	switch ((i=dcr_get4(p)) & 0xffffff) {
    case  3:  p->filters = 0x94949494;  break;
    case  4:  p->filters = 0x49494949;  break;
    default:  p->is_raw = 0;
	}
	dcr_fseek(p->obj_, 72, SEEK_CUR);
	switch ((dcr_get4(p)+3600) % 360) {
    case 270:  p->flip = 4;  break;
    case 180:  p->flip = 1;  break;
    case  90:  p->flip = 7;  break;
    case   0:  p->flip = 2;
	}
	p->cam_mul[0] = (float)dcr_getreal(p, 11);
	p->cam_mul[2] = (float)dcr_getreal(p, 11);
	p->maximum = ~(-1 << dcr_get4(p));
	dcr_fseek(p->obj_, 668, SEEK_CUR);
	p->shutter = dcr_get4(p)/1000000000.0f;
	dcr_fseek(p->obj_, off_image, SEEK_SET);
	if (p->opt.shot_select < p->is_raw)
		dcr_fseek(p->obj_, p->opt.shot_select*8, SEEK_CUR);
	p->data_offset  = (off_t) dcr_get4(p) + 8;
	p->data_offset += (off_t) ((__int64)dcr_get4(p) << 32);
}

char * DCR_CLASS dcr_foveon_gets (DCRAW* p, int offset, char *str, int len)
{
	int i;
	dcr_fseek(p->obj_, offset, SEEK_SET);
	for (i=0; i < len-1; i++)
		if ((str[i] = (char)dcr_get2(p)) == 0) break;
	str[i] = 0;
	return str;
}

void DCR_CLASS dcr_parse_foveon(DCRAW* p)
{
	int entries, img=0, off, len, tag, save, i, wide, high, pent, poff[256][2];
	char name[64], value[64];

	p->order = 0x4949;			/* Little-endian */
	dcr_fseek(p->obj_, 36, SEEK_SET);
	p->flip = dcr_get4(p);
	dcr_fseek(p->obj_, -4, SEEK_END);
	dcr_fseek(p->obj_, dcr_get4(p), SEEK_SET);
	if (dcr_get4(p) != 0x64434553) return;	/* SECd */
	entries = (dcr_get4(p),dcr_get4(p));
	while (entries--) {
		off = dcr_get4(p);
		len = dcr_get4(p);
		tag = dcr_get4(p);
		save = dcr_ftell(p->obj_);
		dcr_fseek(p->obj_, off, SEEK_SET);
		if (dcr_get4(p) != (unsigned int)(0x20434553 | (tag << 24))) return;
		switch (tag) {
		case 0x47414d49:			/* IMAG */
		case 0x32414d49:			/* IMA2 */
			dcr_fseek(p->obj_, 12, SEEK_CUR);
			wide = dcr_get4(p);
			high = dcr_get4(p);
			if (wide > p->raw_width && high > p->raw_height) {
				p->raw_width  = wide;
				p->raw_height = high;
				p->data_offset = off+24;
			}
			dcr_fseek(p->obj_, off+28, SEEK_SET);
			if (dcr_fgetc(p->obj_) == 0xff && dcr_fgetc(p->obj_) == 0xd8
				&& (int)p->thumb_length < len-28) {
				p->thumb_offset = off+28;
				p->thumb_length = len-28;
				p->write_thumb = &DCR_CLASS dcr_jpeg_thumb;
			}
			if (++img == 2 && !p->thumb_length) {
#if RESTRICTED
				p->thumb_offset = off+24;
				p->thumb_width = wide;
				p->thumb_height = high;
				p->write_thumb = &DCR_CLASS dcr_foveon_thumb;
#endif //RESTRICTED
			}
			break;
		case 0x464d4143:			/* CAMF */
			p->meta_offset = off+24;
			p->meta_length = len-28;
			if (p->meta_length > 0x20000)
				p->meta_length = 0x20000;
			break;
		case 0x504f5250:			/* PROP */
			pent = (dcr_get4(p),dcr_get4(p));
			dcr_fseek(p->obj_, 12, SEEK_CUR);
			off += pent*8 + 24;
			if ((unsigned) pent > 256) pent=256;
			for (i=0; i < pent*2; i++)
				poff[0][i] = off + dcr_get4(p)*2;
			for (i=0; i < pent; i++) {
				dcr_foveon_gets (p, poff[i][0], name, 64);
				dcr_foveon_gets (p, poff[i][1], value, 64);
				if (!strcmp (name, "ISO"))
					p->iso_speed = (float)atoi(value);
				if (!strcmp (name, "CAMMANUF"))
					strcpy (p->make, value);
				if (!strcmp (name, "CAMMODEL"))
					strcpy (p->model, value);
				if (!strcmp (name, "WB_DESC"))
					strcpy (p->model2, value);
				if (!strcmp (name, "TIME"))
					p->timestamp = atoi(value);
				if (!strcmp (name, "EXPTIME"))
					p->shutter = atoi(value) / 1000000.0f;
				if (!strcmp (name, "APERTURE"))
					p->aperture = (float)atof(value);
				if (!strcmp (name, "FLENGTH"))
					p->focal_len = (float)atof(value);
			}
#ifdef LOCALTIME
			p->timestamp = mktime (gmtime (&p->timestamp));
#endif
		}
		dcr_fseek(p->obj_, save, SEEK_SET);
	}
	p->is_foveon = 1;
}

/*
Thanks to Adobe for providing these excellent CAM -> XYZ matrices!
*/
void DCR_CLASS dcr_adobe_coeff (DCRAW* p, char *make, char *model)
{
	static const struct {
		const char *prefix;
		short black, maximum, trans[12];
	} table[] = {
    { "Apple QuickTake", 0, 0,		/* DJC */
	{ 17576,-3191,-3318,5210,6733,-1942,9031,1280,-124 } },
    { "Canon EOS D2000", 0, 0,
	{ 24542,-10860,-3401,-1490,11370,-297,2858,-605,3225 } },
    { "Canon EOS D6000", 0, 0,
	{ 20482,-7172,-3125,-1033,10410,-285,2542,226,3136 } },
    { "Canon EOS D30", 0, 0,
	{ 9805,-2689,-1312,-5803,13064,3068,-2438,3075,8775 } },
    { "Canon EOS D60", 0, 0xfa0,
	{ 6188,-1341,-890,-7168,14489,2937,-2640,3228,8483 } },
    { "Canon EOS 5D Mark II", 0, 0x3cf0,
	{ 4716,603,-830,-7798,15474,2480,-1496,1937,6651 } },
    { "Canon EOS 5D", 0, 0xe6c,
	{ 6347,-479,-972,-8297,15954,2480,-1968,2131,7649 } },
    { "Canon EOS 10D", 0, 0xfa0,
	{ 8197,-2000,-1118,-6714,14335,2592,-2536,3178,8266 } },
    { "Canon EOS 20Da", 0, 0,
	{ 14155,-5065,-1382,-6550,14633,2039,-1623,1824,6561 } },
    { "Canon EOS 20D", 0, 0xfff,
	{ 6599,-537,-891,-8071,15783,2424,-1983,2234,7462 } },
    { "Canon EOS 30D", 0, 0,
	{ 6257,-303,-1000,-7880,15621,2396,-1714,1904,7046 } },
    { "Canon EOS 40D", 0, 0x3f60,
	{ 6071,-747,-856,-7653,15365,2441,-2025,2553,7315 } },
    { "Canon EOS 50D", 0, 0x3d93,
	{ 4920,616,-593,-6493,13964,2784,-1774,3178,7005 } },
    { "Canon EOS 300D", 0, 0xfa0,
	{ 8197,-2000,-1118,-6714,14335,2592,-2536,3178,8266 } },
    { "Canon EOS 350D", 0, 0xfff,
	{ 6018,-617,-965,-8645,15881,2975,-1530,1719,7642 } },
    { "Canon EOS 400D", 0, 0xe8e,
	{ 7054,-1501,-990,-8156,15544,2812,-1278,1414,7796 } },
    { "Canon EOS 450D", 0, 0x390d,
	{ 5784,-262,-821,-7539,15064,2672,-1982,2681,7427 } },
    { "Canon EOS 1000D", 0, 0xe43,
	{ 6771,-1139,-977,-7818,15123,2928,-1244,1437,7533 } },
    { "Canon EOS-1Ds Mark III", 0, 0x3bb0,
	{ 5859,-211,-930,-8255,16017,2353,-1732,1887,7448 } },
    { "Canon EOS-1Ds Mark II", 0, 0xe80,
	{ 6517,-602,-867,-8180,15926,2378,-1618,1771,7633 } },
    { "Canon EOS-1D Mark II N", 0, 0xe80,
	{ 6240,-466,-822,-8180,15825,2500,-1801,1938,8042 } },
    { "Canon EOS-1D Mark III", 0, 0x3bb0,
	{ 6291,-540,-976,-8350,16145,2311,-1714,1858,7326 } },
    { "Canon EOS-1D Mark II", 0, 0xe80,
	{ 6264,-582,-724,-8312,15948,2504,-1744,1919,8664 } },
    { "Canon EOS-1DS", 0, 0xe20,
	{ 4374,3631,-1743,-7520,15212,2472,-2892,3632,8161 } },
    { "Canon EOS-1D", 0, 0xe20,
	{ 6806,-179,-1020,-8097,16415,1687,-3267,4236,7690 } },
    { "Canon EOS", 0, 0,
	{ 8197,-2000,-1118,-6714,14335,2592,-2536,3178,8266 } },
    { "Canon PowerShot A50", 0, 0,
	{ -5300,9846,1776,3436,684,3939,-5540,9879,6200,-1404,11175,217 } },
    { "Canon PowerShot A5", 0, 0,
	{ -4801,9475,1952,2926,1611,4094,-5259,10164,5947,-1554,10883,547 } },
    { "Canon PowerShot G10", 0, 0,
	{ 11093,-3906,-1028,-5047,12492,2879,-1003,1750,5561 } },
    { "Canon PowerShot G1", 0, 0,
	{ -4778,9467,2172,4743,-1141,4344,-5146,9908,6077,-1566,11051,557 } },
    { "Canon PowerShot G2", 0, 0,
	{ 9087,-2693,-1049,-6715,14382,2537,-2291,2819,7790 } },
    { "Canon PowerShot G3", 0, 0,
	{ 9212,-2781,-1073,-6573,14189,2605,-2300,2844,7664 } },
    { "Canon PowerShot G5", 0, 0,
	{ 9757,-2872,-933,-5972,13861,2301,-1622,2328,7212 } },
    { "Canon PowerShot G6", 0, 0,
	{ 9877,-3775,-871,-7613,14807,3072,-1448,1305,7485 } },
    { "Canon PowerShot G9", 0, 0,
	{ 7368,-2141,-598,-5621,13254,2625,-1418,1696,5743 } },
    { "Canon PowerShot Pro1", 0, 0,
	{ 10062,-3522,-999,-7643,15117,2730,-765,817,7323 } },
    { "Canon PowerShot Pro70", 34, 0,
	{ -4155,9818,1529,3939,-25,4522,-5521,9870,6610,-2238,10873,1342 } },
    { "Canon PowerShot Pro90", 0, 0,
	{ -4963,9896,2235,4642,-987,4294,-5162,10011,5859,-1770,11230,577 } },
    { "Canon PowerShot S30", 0, 0,
	{ 10566,-3652,-1129,-6552,14662,2006,-2197,2581,7670 } },
    { "Canon PowerShot S40", 0, 0,
	{ 8510,-2487,-940,-6869,14231,2900,-2318,2829,9013 } },
    { "Canon PowerShot S45", 0, 0,
	{ 8163,-2333,-955,-6682,14174,2751,-2077,2597,8041 } },
    { "Canon PowerShot S50", 0, 0,
	{ 8882,-2571,-863,-6348,14234,2288,-1516,2172,6569 } },
    { "Canon PowerShot S60", 0, 0,
	{ 8795,-2482,-797,-7804,15403,2573,-1422,1996,7082 } },
    { "Canon PowerShot S70", 0, 0,
	{ 9976,-3810,-832,-7115,14463,2906,-901,989,7889 } },
    { "Canon PowerShot A610", 0, 0,	/* DJC */
	{ 15591,-6402,-1592,-5365,13198,2168,-1300,1824,5075 } },
    { "Canon PowerShot A620", 0, 0,	/* DJC */
	{ 15265,-6193,-1558,-4125,12116,2010,-888,1639,5220 } },
    { "Canon PowerShot A630", 0, 0,	/* DJC */
	{ 14201,-5308,-1757,-6087,14472,1617,-2191,3105,5348 } },
    { "Canon PowerShot A640", 0, 0,	/* DJC */
	{ 13124,-5329,-1390,-3602,11658,1944,-1612,2863,4885 } },
    { "Canon PowerShot A650", 0, 0,	/* DJC */
	{ 9427,-3036,-959,-2581,10671,1911,-1039,1982,4430 } },
    { "Canon PowerShot A720", 0, 0,	/* DJC */
	{ 14573,-5482,-1546,-1266,9799,1468,-1040,1912,3810 } },
    { "Canon PowerShot S3 IS", 0, 0,	/* DJC */
	{ 14062,-5199,-1446,-4712,12470,2243,-1286,2028,4836 } },
    { "CINE 650", 0, 0,
	{ 3390,480,-500,-800,3610,340,-550,2336,1192 } },
    { "CINE 660", 0, 0,
	{ 3390,480,-500,-800,3610,340,-550,2336,1192 } },
    { "CINE", 0, 0,
	{ 20183,-4295,-423,-3940,15330,3985,-280,4870,9800 } },
    { "Contax N Digital", 0, 0xf1e,
	{ 7777,1285,-1053,-9280,16543,2916,-3677,5679,7060 } },
    { "EPSON R-D1", 0, 0,
	{ 6827,-1878,-732,-8429,16012,2564,-704,592,7145 } },
    { "FUJIFILM FinePix E550", 0, 0,
	{ 11044,-3888,-1120,-7248,15168,2208,-1531,2277,8069 } },
    { "FUJIFILM FinePix E900", 0, 0,
	{ 9183,-2526,-1078,-7461,15071,2574,-2022,2440,8639 } },
    { "FUJIFILM FinePix F8", 0, 0,
	{ 11044,-3888,-1120,-7248,15168,2208,-1531,2277,8069 } },
    { "FUJIFILM FinePix F7", 0, 0,
	{ 10004,-3219,-1201,-7036,15047,2107,-1863,2565,7736 } },
    { "FUJIFILM FinePix S100FS", 514, 0,
	{ 11521,-4355,-1065,-6524,13767,3058,-1466,1984,6045 } },
    { "FUJIFILM FinePix S20Pro", 0, 0,
	{ 10004,-3219,-1201,-7036,15047,2107,-1863,2565,7736 } },
    { "FUJIFILM FinePix S2Pro", 128, 0,
	{ 12492,-4690,-1402,-7033,15423,1647,-1507,2111,7697 } },
    { "FUJIFILM FinePix S3Pro", 0, 0,
	{ 11807,-4612,-1294,-8927,16968,1988,-2120,2741,8006 } },
    { "FUJIFILM FinePix S5Pro", 0, 0,
	{ 12300,-5110,-1304,-9117,17143,1998,-1947,2448,8100 } },
    { "FUJIFILM FinePix S5000", 0, 0,
	{ 8754,-2732,-1019,-7204,15069,2276,-1702,2334,6982 } },
    { "FUJIFILM FinePix S5100", 0, 0x3e00,
	{ 11940,-4431,-1255,-6766,14428,2542,-993,1165,7421 } },
    { "FUJIFILM FinePix S5500", 0, 0x3e00,
	{ 11940,-4431,-1255,-6766,14428,2542,-993,1165,7421 } },
    { "FUJIFILM FinePix S5200", 0, 0,
	{ 9636,-2804,-988,-7442,15040,2589,-1803,2311,8621 } },
    { "FUJIFILM FinePix S5600", 0, 0,
	{ 9636,-2804,-988,-7442,15040,2589,-1803,2311,8621 } },
    { "FUJIFILM FinePix S6", 0, 0,
	{ 12628,-4887,-1401,-6861,14996,1962,-2198,2782,7091 } },
    { "FUJIFILM FinePix S7000", 0, 0,
	{ 10190,-3506,-1312,-7153,15051,2238,-2003,2399,7505 } },
    { "FUJIFILM FinePix S9000", 0, 0,
	{ 10491,-3423,-1145,-7385,15027,2538,-1809,2275,8692 } },
    { "FUJIFILM FinePix S9500", 0, 0,
	{ 10491,-3423,-1145,-7385,15027,2538,-1809,2275,8692 } },
    { "FUJIFILM FinePix S9100", 0, 0,
	{ 12343,-4515,-1285,-7165,14899,2435,-1895,2496,8800 } },
    { "FUJIFILM FinePix S9600", 0, 0,
	{ 12343,-4515,-1285,-7165,14899,2435,-1895,2496,8800 } },
    { "FUJIFILM IS-1", 0, 0,
	{ 21461,-10807,-1441,-2332,10599,1999,289,875,7703 } },
    { "FUJIFILM IS Pro", 0, 0,
	{ 12300,-5110,-1304,-9117,17143,1998,-1947,2448,8100 } },
    { "Imacon Ixpress", 0, 0,		/* DJC */
	{ 7025,-1415,-704,-5188,13765,1424,-1248,2742,6038 } },
    { "KODAK NC2000", 0, 0,
	{ 13891,-6055,-803,-465,9919,642,2121,82,1291 } },
    { "Kodak DCS315C", 8, 0,
	{ 17523,-4827,-2510,756,8546,-137,6113,1649,2250 } },
    { "Kodak DCS330C", 8, 0,
	{ 20620,-7572,-2801,-103,10073,-396,3551,-233,2220 } },
    { "KODAK DCS420", 0, 0,
	{ 10868,-1852,-644,-1537,11083,484,2343,628,2216 } },
    { "KODAK DCS460", 0, 0,
	{ 10592,-2206,-967,-1944,11685,230,2206,670,1273 } },
    { "KODAK EOSDCS1", 0, 0,
	{ 10592,-2206,-967,-1944,11685,230,2206,670,1273 } },
    { "KODAK EOSDCS3B", 0, 0,
	{ 9898,-2700,-940,-2478,12219,206,1985,634,1031 } },
    { "Kodak DCS520C", 180, 0,
	{ 24542,-10860,-3401,-1490,11370,-297,2858,-605,3225 } },
    { "Kodak DCS560C", 188, 0,
	{ 20482,-7172,-3125,-1033,10410,-285,2542,226,3136 } },
    { "Kodak DCS620C", 180, 0,
	{ 23617,-10175,-3149,-2054,11749,-272,2586,-489,3453 } },
    { "Kodak DCS620X", 185, 0,
	{ 13095,-6231,154,12221,-21,-2137,895,4602,2258 } },
    { "Kodak DCS660C", 214, 0,
	{ 18244,-6351,-2739,-791,11193,-521,3711,-129,2802 } },
    { "Kodak DCS720X", 0, 0,
	{ 11775,-5884,950,9556,1846,-1286,-1019,6221,2728 } },
    { "Kodak DCS760C", 0, 0,
	{ 16623,-6309,-1411,-4344,13923,323,2285,274,2926 } },
    { "Kodak DCS Pro SLR", 0, 0,
	{ 5494,2393,-232,-6427,13850,2846,-1876,3997,5445 } },
    { "Kodak DCS Pro 14nx", 0, 0,
	{ 5494,2393,-232,-6427,13850,2846,-1876,3997,5445 } },
    { "Kodak DCS Pro 14", 0, 0,
	{ 7791,3128,-776,-8588,16458,2039,-2455,4006,6198 } },
    { "Kodak ProBack645", 0, 0,
	{ 16414,-6060,-1470,-3555,13037,473,2545,122,4948 } },
    { "Kodak ProBack", 0, 0,
	{ 21179,-8316,-2918,-915,11019,-165,3477,-180,4210 } },
    { "KODAK P712", 0, 0,
	{ 9658,-3314,-823,-5163,12695,2768,-1342,1843,6044 } },
    { "KODAK P850", 0, 0xf7c,
	{ 10511,-3836,-1102,-6946,14587,2558,-1481,1792,6246 } },
    { "KODAK P880", 0, 0xfff,
	{ 12805,-4662,-1376,-7480,15267,2360,-1626,2194,7904 } },
    { "Leaf CMost", 0, 0,
	{ 3952,2189,449,-6701,14585,2275,-4536,7349,6536 } },
    { "Leaf Valeo 6", 0, 0,
	{ 3952,2189,449,-6701,14585,2275,-4536,7349,6536 } },
    { "Leaf Aptus 54S", 0, 0,
	{ 8236,1746,-1314,-8251,15953,2428,-3673,5786,5771 } },
    { "Leaf Aptus 65", 0, 0,
	{ 7914,1414,-1190,-8777,16582,2280,-2811,4605,5562 } },
    { "Leaf Aptus 75", 0, 0,
	{ 7914,1414,-1190,-8777,16582,2280,-2811,4605,5562 } },
    { "Leaf", 0, 0,
	{ 8236,1746,-1314,-8251,15953,2428,-3673,5786,5771 } },
    { "Mamiya ZD", 0, 0,
	{ 7645,2579,-1363,-8689,16717,2015,-3712,5941,5961 } },
    { "Micron 2010", 110, 0,		/* DJC */
	{ 16695,-3761,-2151,155,9682,163,3433,951,4904 } },
    { "Minolta DiMAGE 5", 0, 0xf7d,
	{ 8983,-2942,-963,-6556,14476,2237,-2426,2887,8014 } },
    { "Minolta DiMAGE 7Hi", 0, 0xf7d,
	{ 11368,-3894,-1242,-6521,14358,2339,-2475,3056,7285 } },
    { "Minolta DiMAGE 7", 0, 0xf7d,
	{ 9144,-2777,-998,-6676,14556,2281,-2470,3019,7744 } },
    { "Minolta DiMAGE A1", 0, 0xf8b,
	{ 9274,-2547,-1167,-8220,16323,1943,-2273,2720,8340 } },
    { "MINOLTA DiMAGE A200", 0, 0,
	{ 8560,-2487,-986,-8112,15535,2771,-1209,1324,7743 } },
    { "Minolta DiMAGE A2", 0, 0xf8f,
	{ 9097,-2726,-1053,-8073,15506,2762,-966,981,7763 } },
    { "Minolta DiMAGE Z2", 0, 0,	/* DJC */
	{ 11280,-3564,-1370,-4655,12374,2282,-1423,2168,5396 } },
    { "MINOLTA DYNAX 5", 0, 0xffb,
	{ 10284,-3283,-1086,-7957,15762,2316,-829,882,6644 } },
    { "MINOLTA DYNAX 7", 0, 0xffb,
	{ 10239,-3104,-1099,-8037,15727,2451,-927,925,6871 } },
    { "NIKON D100", 0, 0,
	{ 5902,-933,-782,-8983,16719,2354,-1402,1455,6464 } },
    { "NIKON D1H", 0, 0,
	{ 7577,-2166,-926,-7454,15592,1934,-2377,2808,8606 } },
    { "NIKON D1X", 0, 0,
	{ 7702,-2245,-975,-9114,17242,1875,-2679,3055,8521 } },
    { "NIKON D1", 0, 0, /* multiplied by 2.218750, 1.0, 1.148438 */
	{ 16772,-4726,-2141,-7611,15713,1972,-2846,3494,9521 } },
    { "NIKON D2H", 0, 0,
	{ 5710,-901,-615,-8594,16617,2024,-2975,4120,6830 } },
    { "NIKON D2X", 0, 0,
	{ 10231,-2769,-1255,-8301,15900,2552,-797,680,7148 } },
    { "NIKON D40X", 0, 0,
	{ 8819,-2543,-911,-9025,16928,2151,-1329,1213,8449 } },
    { "NIKON D40", 0, 0,
	{ 6992,-1668,-806,-8138,15748,2543,-874,850,7897 } },
    { "NIKON D50", 0, 0,
	{ 7732,-2422,-789,-8238,15884,2498,-859,783,7330 } },
    { "NIKON D60", 0, 0,
	{ 8736,-2458,-935,-9075,16894,2251,-1354,1242,8263 } },
    { "NIKON D700", 0, 0,
	{ 8139,-2171,-663,-8747,16541,2295,-1925,2008,8093 } },
    { "NIKON D70", 0, 0,
	{ 7732,-2422,-789,-8238,15884,2498,-859,783,7330 } },
    { "NIKON D80", 0, 0,
	{ 8629,-2410,-883,-9055,16940,2171,-1490,1363,8520 } },
    { "NIKON D90", 0, 0xf00,
	{ 7309,-1403,-519,-8474,16008,2622,-2434,2826,8064 } },
    { "NIKON D200", 0, 0xfbc,
	{ 8367,-2248,-763,-8758,16447,2422,-1527,1550,8053 } },
    { "NIKON D300", 0, 0,
	{ 9030,-1992,-715,-8465,16302,2255,-2689,3217,8069 } },
    { "NIKON D3", 0, 0,
	{ 8139,-2171,-663,-8747,16541,2295,-1925,2008,8093 } },
    { "NIKON E950", 0, 0x3dd,		/* DJC */
	{ -3746,10611,1665,9621,-1734,2114,-2389,7082,3064,3406,6116,-244 } },
    { "NIKON E995", 0, 0,	/* copied from E5000 */
	{ -5547,11762,2189,5814,-558,3342,-4924,9840,5949,688,9083,96 } },
    { "NIKON E2100", 0, 0,	/* copied from Z2, new white balance */
	{ 13142,-4152,-1596,-4655,12374,2282,-1769,2696,6711} },
    { "NIKON E2500", 0, 0,
	{ -5547,11762,2189,5814,-558,3342,-4924,9840,5949,688,9083,96 } },
    { "NIKON E4300", 0, 0,	/* copied from Minolta DiMAGE Z2 */
	{ 11280,-3564,-1370,-4655,12374,2282,-1423,2168,5396 } },
    { "NIKON E4500", 0, 0,
	{ -5547,11762,2189,5814,-558,3342,-4924,9840,5949,688,9083,96 } },
    { "NIKON E5000", 0, 0,
	{ -5547,11762,2189,5814,-558,3342,-4924,9840,5949,688,9083,96 } },
    { "NIKON E5400", 0, 0,
	{ 9349,-2987,-1001,-7919,15766,2266,-2098,2680,6839 } },
    { "NIKON E5700", 0, 0,
	{ -5368,11478,2368,5537,-113,3148,-4969,10021,5782,778,9028,211 } },
    { "NIKON E8400", 0, 0,
	{ 7842,-2320,-992,-8154,15718,2599,-1098,1342,7560 } },
    { "NIKON E8700", 0, 0,
	{ 8489,-2583,-1036,-8051,15583,2643,-1307,1407,7354 } },
    { "NIKON E8800", 0, 0,
	{ 7971,-2314,-913,-8451,15762,2894,-1442,1520,7610 } },
    { "NIKON COOLPIX P6000", 0, 0,
	{ 9698,-3367,-914,-4706,12584,2368,-837,968,5801 } },
    { "OLYMPUS C5050", 0, 0,
	{ 10508,-3124,-1273,-6079,14294,1901,-1653,2306,6237 } },
    { "OLYMPUS C5060", 0, 0,
	{ 10445,-3362,-1307,-7662,15690,2058,-1135,1176,7602 } },
    { "OLYMPUS C7070", 0, 0,
	{ 10252,-3531,-1095,-7114,14850,2436,-1451,1723,6365 } },
    { "OLYMPUS C70", 0, 0,
	{ 10793,-3791,-1146,-7498,15177,2488,-1390,1577,7321 } },
    { "OLYMPUS C80", 0, 0,
	{ 8606,-2509,-1014,-8238,15714,2703,-942,979,7760 } },
    { "OLYMPUS E-10", 0, (short)0xffc0,
	{ 12745,-4500,-1416,-6062,14542,1580,-1934,2256,6603 } },
    { "OLYMPUS E-1", 0, (short)0xfff0,
	{ 11846,-4767,-945,-7027,15878,1089,-2699,4122,8311 } },
    { "OLYMPUS E-20", 0, (short)0xffc0,
	{ 13173,-4732,-1499,-5807,14036,1895,-2045,2452,7142 } },
    { "OLYMPUS E-300", 0, 0,
	{ 7828,-1761,-348,-5788,14071,1830,-2853,4518,6557 } },
    { "OLYMPUS E-330", 0, 0,
	{ 8961,-2473,-1084,-7979,15990,2067,-2319,3035,8249 } },
    { "OLYMPUS E-3", 0, 0xf99,
	{ 9487,-2875,-1115,-7533,15606,2010,-1618,2100,7389 } },
    { "OLYMPUS E-400", 0, (short)0xfff0,
	{ 6169,-1483,-21,-7107,14761,2536,-2904,3580,8568 } },
    { "OLYMPUS E-410", 0, 0xf6a,
	{ 8856,-2582,-1026,-7761,15766,2082,-2009,2575,7469 } },
    { "OLYMPUS E-420", 0, 0xfd7,
	{ 8746,-2425,-1095,-7594,15612,2073,-1780,2309,7416 } },
    { "OLYMPUS E-500", 0, 0,
	{ 8136,-1968,-299,-5481,13742,1871,-2556,4205,6630 } },
    { "OLYMPUS E-510", 0, 0xf6a,
	{ 8785,-2529,-1033,-7639,15624,2112,-1783,2300,7817 } },
    { "OLYMPUS E-520", 0, 0xfd2,
	{ 8344,-2322,-1020,-7596,15635,2048,-1748,2269,7287 } },
    { "OLYMPUS SP350", 0, 0,
	{ 12078,-4836,-1069,-6671,14306,2578,-786,939,7418 } },
    { "OLYMPUS SP3", 0, 0,
	{ 11766,-4445,-1067,-6901,14421,2707,-1029,1217,7572 } },
    { "OLYMPUS SP500UZ", 0, 0xfff,
	{ 9493,-3415,-666,-5211,12334,3260,-1548,2262,6482 } },
    { "OLYMPUS SP510UZ", 0, 0xffe,
	{ 10593,-3607,-1010,-5881,13127,3084,-1200,1805,6721 } },
    { "OLYMPUS SP550UZ", 0, 0xffe,
	{ 11597,-4006,-1049,-5432,12799,2957,-1029,1750,6516 } },
    { "OLYMPUS SP560UZ", 0, 0xff9,
	{ 10915,-3677,-982,-5587,12986,2911,-1168,1968,6223 } },
    { "OLYMPUS SP570UZ", 0, 0,
	{ 11522,-4044,-1146,-4736,12172,2904,-988,1829,6039 } },
    { "PENTAX *ist DL2", 0, 0,
	{ 10504,-2438,-1189,-8603,16207,2531,-1022,863,12242 } },
    { "PENTAX *ist DL", 0, 0,
	{ 10829,-2838,-1115,-8339,15817,2696,-837,680,11939 } },
    { "PENTAX *ist DS2", 0, 0,
	{ 10504,-2438,-1189,-8603,16207,2531,-1022,863,12242 } },
    { "PENTAX *ist DS", 0, 0,
	{ 10371,-2333,-1206,-8688,16231,2602,-1230,1116,11282 } },
    { "PENTAX *ist D", 0, 0,
	{ 9651,-2059,-1189,-8881,16512,2487,-1460,1345,10687 } },
    { "PENTAX K10D", 0, 0,
	{ 9566,-2863,-803,-7170,15172,2112,-818,803,9705 } },
    { "PENTAX K1", 0, 0,
	{ 11095,-3157,-1324,-8377,15834,2720,-1108,947,11688 } },
    { "PENTAX K20D", 0, 0,
	{ 9427,-2714,-868,-7493,16092,1373,-2199,3264,7180 } },
    { "PENTAX K200D", 0, 0,
	{ 9186,-2678,-907,-8693,16517,2260,-1129,1094,8524 } },
    { "PENTAX K2000", 0, 0,
	{ 11057,-3604,-1155,-5152,13046,2329,-282,375,8104 } },
    { "Panasonic DMC-FZ8", 0, (short)0xf7f0,
	{ 8986,-2755,-802,-6341,13575,3077,-1476,2144,6379 } },
    { "Panasonic DMC-FZ18", 0, 0,
	{ 9932,-3060,-935,-5809,13331,2753,-1267,2155,5575 } },
    { "Panasonic DMC-FZ28", 15, 0xfff,
	{ 10109,-3488,-993,-5412,12812,2916,-1305,2140,5543 } },
    { "Panasonic DMC-FZ30", 0, (short)0xf94c,
	{ 10976,-4029,-1141,-7918,15491,2600,-1670,2071,8246 } },
    { "Panasonic DMC-FZ50", 0, (short)0xfff0,	/* aka "LEICA V-LUX1" */
	{ 7906,-2709,-594,-6231,13351,3220,-1922,2631,6537 } },
    { "Panasonic DMC-L10", 15, 0xf96,
	{ 8025,-1942,-1050,-7920,15904,2100,-2456,3005,7039 } },
    { "Panasonic DMC-L1", 0, (short)0xf7fc,	/* aka "LEICA DIGILUX 3" */
	{ 8054,-1885,-1025,-8349,16367,2040,-2805,3542,7629 } },
    { "Panasonic DMC-LC1", 0, 0,	/* aka "LEICA DIGILUX 2" */
	{ 11340,-4069,-1275,-7555,15266,2448,-2960,3426,7685 } },
    { "Panasonic DMC-LX1", 0, (short)0xf7f0,	/* aka "LEICA D-LUX2" */
	{ 10704,-4187,-1230,-8314,15952,2501,-920,945,8927 } },
    { "Panasonic DMC-LX2", 0, 0,	/* aka "LEICA D-LUX3" */
	{ 8048,-2810,-623,-6450,13519,3272,-1700,2146,7049 } },
    { "Panasonic DMC-LX3", 15, 0xfff,	/* aka "LEICA D-LUX4" */
	{ 8128,-2668,-655,-6134,13307,3161,-1782,2568,6083 } },
    { "Panasonic DMC-FX150", 15, 0xfff,
	{ 9082,-2907,-925,-6119,13377,3058,-1797,2641,5609 } },
    { "Panasonic DMC-G1", 15, 0xfff,
	{ 8199,-2065,-1056,-8124,16156,2033,-2458,3022,7220 } },
    { "Phase One H 20", 0, 0,		/* DJC */
	{ 1313,1855,-109,-6715,15908,808,-327,1840,6020 } },
    { "Phase One P 2", 0, 0,
	{ 2905,732,-237,-8134,16626,1476,-3038,4253,7517 } },
    { "Phase One P 30", 0, 0,
	{ 4516,-245,-37,-7020,14976,2173,-3206,4671,7087 } },
    { "Phase One P 45", 0, 0,
	{ 5053,-24,-117,-5684,14076,1702,-2619,4492,5849 } },
    { "SAMSUNG GX-1", 0, 0,
	{ 10504,-2438,-1189,-8603,16207,2531,-1022,863,12242 } },
    { "Sinar", 0, 0,			/* DJC */
	{ 16442,-2956,-2422,-2877,12128,750,-1136,6066,4559 } },
    { "SONY DSC-F828", 491, 0,
	{ 7924,-1910,-777,-8226,15459,2998,-1517,2199,6818,-7242,11401,3481 } },
    { "SONY DSC-R1", 512, 0,
	{ 8512,-2641,-694,-8042,15670,2526,-1821,2117,7414 } },
    { "SONY DSC-V3", 0, 0,
	{ 7511,-2571,-692,-7894,15088,3060,-948,1111,8128 } },
    { "SONY DSLR-A100", 0, 0xfeb,
	{ 9437,-2811,-774,-8405,16215,2290,-710,596,7181 } },
    { "SONY DSLR-A200", 0, 0,
	{ 9847,-3091,-928,-8485,16345,2225,-715,595,7103 } },
    { "SONY DSLR-A300", 0, 0,
	{ 9847,-3091,-928,-8485,16345,2225,-715,595,7103 } },
    { "SONY DSLR-A350", 0, 0xffc,
	{ 6038,-1484,-578,-9146,16746,2513,-875,746,7217 } },
    { "SONY DSLR-A700", 254, 0x1ffe,
	{ 5775,-805,-359,-8574,16295,2391,-1943,2341,7249 } },
    { "SONY DSLR-A900", 254, 0x1ffe,
	{ 5209,-1072,-397,-8845,16120,2919,-1618,1803,8654 } }
  };
  double cam_xyz[4][3];
  char name[130];
  int i, j;

  sprintf (name, "%s %s", make, model);
  for (i=0; i < sizeof table / sizeof *table; i++)
	  if (!strncmp (name, table[i].prefix, strlen(table[i].prefix))) {
		  if (table[i].black)   p->black = (ushort) table[i].black;
		  if (table[i].maximum) p->maximum = (ushort) table[i].maximum;
		  for (j=0; j < 12; j++)
			  cam_xyz[0][j] = table[i].trans[j] / 10000.0;
		  dcr_cam_xyz_coeff (p, cam_xyz);
		  break;
	  }
}

void DCR_CLASS dcr_simple_coeff (DCRAW* p, int index)
{
	static const float table[][12] = {
		/* index 0 -- all Foveon cameras */
		{ 1.4032f,-0.2231f,-0.1016f,-0.5263f,1.4816f,0.017f,-0.0112f,0.0183f,0.9113f },
			/* index 1 -- Kodak DC20 and DC25 */
		{ 2.25f,0.75f,-1.75f,-0.25f,-0.25f,0.75f,0.75f,-0.25f,-0.25f,-1.75f,0.75f,2.25f },
		/* index 2 -- Logitech Fotoman Pixtura */
		{ 1.893f,-0.418f,-0.476f,-0.495f,1.773f,-0.278f,-1.017f,-0.655f,2.672f },
		/* index 3 -- Nikon E880, E900, and E990 */
		{ -1.936280f,  1.800443f, -1.448486f,  2.584324f,
		1.405365f, -0.524955f, -0.289090f,  0.408680f,
		-1.204965f,  1.082304f,  2.941367f, -1.818705f }
	};
	int i, c;

	for (p->raw_color = i=0; i < 3; i++)
		FORCC(p) p->rgb_cam[i][c] = table[index][i*p->colors+c];
}

short DCR_CLASS dcr_guess_byte_order (DCRAW* p, int words)
{
	uchar test[4][2];
	int t=2, msb;
	double diff, sum[2] = {0,0};

	dcr_fread(p->obj_, test[0], 2, 2);
	for (words-=2; words--; ) {
		dcr_fread(p->obj_, test[t], 2, 1);
		for (msb=0; msb < 2; msb++) {
			diff = (test[t^2][msb] << 8 | test[t^2][!msb])
				- (test[t  ][msb] << 8 | test[t  ][!msb]);
			sum[msb] += diff*diff;
		}
		t = (t+1) & 3;
	}
	return sum[0] < sum[1] ? 0x4d4d : 0x4949;
}

/*
Identify which camera created this file, and set global variables
accordingly.
*/
void DCR_CLASS dcr_identify(DCRAW* p)
{
	char head[32], *cp;
	unsigned hlen, fsize, i, c, is_canon;
	struct dcr_jhead jh;
	static const struct {
		int fsize;
		char make[12], model[19], withjpeg;
	} table[] = {
		{    62464, "Kodak",    "DC20"            ,0 },
		{   124928, "Kodak",    "DC20"            ,0 },
		{  1652736, "Kodak",    "DCS200"          ,0 },
		{  4159302, "Kodak",    "C330"            ,0 },
		{  4162462, "Kodak",    "C330"            ,0 },
		{   460800, "Kodak",    "C603v"           ,0 },
		{   614400, "Kodak",    "C603v"           ,0 },
		{  6163328, "Kodak",    "C603"            ,0 },
		{  6166488, "Kodak",    "C603"            ,0 },
		{  9116448, "Kodak",    "C603y"           ,0 },
		{   311696, "ST Micro", "STV680 VGA"      ,0 },  /* SPYz */
		{   614400, "Kodak",    "KAI-0340"        ,0 },
		{   787456, "Creative", "PC-CAM 600"      ,0 },
		{  1138688, "Minolta",  "RD175"           ,0 },
		{  3840000, "Foculus",  "531C"            ,0 },
		{   786432, "AVT",      "F-080C"          ,0 },
		{  1447680, "AVT",      "F-145C"          ,0 },
		{  1920000, "AVT",      "F-201C"          ,0 },
		{  5067304, "AVT",      "F-510C"          ,0 },
		{ 10134608, "AVT",      "F-510C"          ,0 },
		{ 16157136, "AVT",      "F-810C"          ,0 },
		{  1409024, "Sony",     "XCD-SX910CR"     ,0 },
		{  2818048, "Sony",     "XCD-SX910CR"     ,0 },
		{  3884928, "Micron",   "2010"            ,0 },
		{  6624000, "Pixelink", "A782"            ,0 },
		{ 13248000, "Pixelink", "A782"            ,0 },
		{  6291456, "RoverShot","3320AF"          ,0 },
		{  6553440, "Canon",    "PowerShot A460"  ,0 },
		{  6653280, "Canon",    "PowerShot A530"  ,0 },
		{  6573120, "Canon",    "PowerShot A610"  ,0 },
		{  9219600, "Canon",    "PowerShot A620"  ,0 },
		{ 10341600, "Canon",    "PowerShot A720"  ,0 },
		{ 10383120, "Canon",    "PowerShot A630"  ,0 },
		{ 12945240, "Canon",    "PowerShot A640"  ,0 },
		{ 15636240, "Canon",    "PowerShot A650"  ,0 },
		{  5298000, "Canon",    "PowerShot SD300" ,0 },
		{  7710960, "Canon",    "PowerShot S3 IS" ,0 },
		{  5939200, "OLYMPUS",  "C770UZ"          ,0 },
		{  1581060, "NIKON",    "E900"            ,1 },  /* or E900s,E910 */
		{  2465792, "NIKON",    "E950"            ,1 },  /* or E800,E700 */
		{  2940928, "NIKON",    "E2100"           ,1 },  /* or E2500 */
		{  4771840, "NIKON",    "E990"            ,1 },  /* or E995, Oly C3030Z */
		{  4775936, "NIKON",    "E3700"           ,1 },  /* or Optio 33WR */
		{  5869568, "NIKON",    "E4300"           ,1 },  /* or DiMAGE Z2 */
		{  5865472, "NIKON",    "E4500"           ,1 },
		{  7438336, "NIKON",    "E5000"           ,1 },  /* or E5700 */
		{  8998912, "NIKON",    "COOLPIX S6"      ,1 },
		{  1976352, "CASIO",    "QV-2000UX"       ,1 },
		{  3217760, "CASIO",    "QV-3*00EX"       ,1 },
		{  6218368, "CASIO",    "QV-5700"         ,1 },
		{  6054400, "CASIO",    "QV-R41"          ,1 },
		{  7530816, "CASIO",    "QV-R51"          ,1 },
		{  7684000, "CASIO",    "QV-4000"         ,1 },
		{  4948608, "CASIO",    "EX-S100"         ,1 },
		{  7542528, "CASIO",    "EX-Z50"          ,1 },
		{  7753344, "CASIO",    "EX-Z55"          ,1 },
		{  7426656, "CASIO",    "EX-P505"         ,1 },
		{  9313536, "CASIO",    "EX-P600"         ,1 },
		{ 10979200, "CASIO",    "EX-P700"         ,1 },
		{  3178560, "PENTAX",   "Optio S"         ,1 },
		{  4841984, "PENTAX",   "Optio S"         ,1 },
		{  6114240, "PENTAX",   "Optio S4"        ,1 },  /* or S4i, CASIO EX-Z4 */
		{ 10702848, "PENTAX",   "Optio 750Z"      ,1 },
		{ 16098048, "SAMSUNG",  "S85"             ,1 },
		{ 16215552, "SAMSUNG",  "S85"             ,1 },
		{ 12582980, "Sinar",    ""                ,0 },
		{ 33292868, "Sinar",    ""                ,0 },
		{ 44390468, "Sinar",    ""                ,0 } };
	static const char *corp[] =
		{ "Canon", "NIKON", "EPSON", "KODAK", "Kodak", "OLYMPUS", "PENTAX",
		"MINOLTA", "Minolta", "Konica", "CASIO", "Sinar", "Phase One",
		"SAMSUNG", "Mamiya" };

	p->tiff_flip = p->flip = p->filters = -1;	/* 0 is valid, so -1 is unknown */
	p->raw_height = p->raw_width = p->fuji_width = p->fuji_layout = p->cr2_slice[0] = 0;
	p->maximum = p->height = p->width = p->top_margin = p->left_margin = 0;
	p->cdesc[0] = p->desc[0] = p->artist[0] = p->make[0] = p->model[0] = p->model2[0] = 0;
	p->iso_speed = p->shutter = p->aperture = p->focal_len = 0.0f; p->unique_id = 0;
	memset (p->gpsdata, 0, sizeof p->gpsdata);
	memset (p->white, 0, sizeof p->white);
	p->thumb_offset = p->thumb_length = p->thumb_width = p->thumb_height = 0;
	p->load_raw = p->thumb_load_raw = 0;
	p->write_thumb = &DCR_CLASS dcr_jpeg_thumb;
	p->data_offset = p->meta_length = p->tiff_bps = p->tiff_compress = 0;
	p->kodak_cbpp = p->zero_after_ff = p->dng_version = p->load_flags = 0;
	p->timestamp = p->shot_order = p->tiff_samples = p->black = p->is_foveon = 0;
	p->mix_green = p->profile_length = p->data_error = p->zero_is_bad = 0;
	p->pixel_aspect = p->is_raw = p->raw_color = p->use_gamma = 1;
	p->tile_width = p->tile_length = INT_MAX;
	for (i=0; i < 4; i++) {
		p->cam_mul[i] = (float)(i == 1);
		p->pre_mul[i] = (float)(i < 3);
		FORC3 p->cmatrix[c][i] = 0.0f;
		FORC3 p->rgb_cam[c][i] = (float)(c == i);
	}
	p->colors = 3;
	p->tiff_bps = 12;
	for (i=0; i < 0x4000; i++) p->curve[i] = i;

	p->order = dcr_get2(p);
	hlen = dcr_get4(p);
	dcr_fseek(p->obj_, 0, SEEK_SET);
	dcr_fread(p->obj_, head, 1, 32);
	dcr_fseek(p->obj_, 0, SEEK_END);
	fsize = dcr_ftell(p->obj_);
	if ((cp = (char *) memmem (head, 32, "MMMM", 4)) ||
		(cp = (char *) memmem (head, 32, "IIII", 4))) {
		dcr_parse_phase_one (p,cp-head);
		if (cp-head) dcr_parse_tiff(p,0);
	} else if (p->order == 0x4949 || p->order == 0x4d4d) {
		if (!memcmp (head+6,"HEAPCCDR",8)) {
			p->data_offset = hlen;
			dcr_parse_ciff (p,hlen, fsize - hlen);
		} else {
			dcr_parse_tiff(p,0);
		}
	} else if (!memcmp (head,"\xff\xd8\xff\xe1",4) &&
		!memcmp (head+6,"Exif",4)) {
		dcr_fseek(p->obj_, 4, SEEK_SET);
		p->data_offset = 4 + dcr_get2(p);
		dcr_fseek(p->obj_, p->data_offset, SEEK_SET);
		if (dcr_fgetc(p->obj_) != 0xff)
			dcr_parse_tiff(p,12);
		p->thumb_offset = 0;
	} else if (!memcmp (head+25,"ARECOYK",7)) {
		strcpy (p->make, "Contax");
		strcpy (p->model,"N Digital");
		dcr_fseek(p->obj_, 33, SEEK_SET);
		dcr_get_timestamp(p,1);
		dcr_fseek(p->obj_, 60, SEEK_SET);
		FORC4 p->cam_mul[c ^ (c >> 1)] = (float)dcr_get4(p);
	} else if (!strcmp (head, "PXN")) {
		strcpy (p->make, "Logitech");
		strcpy (p->model,"Fotoman Pixtura");
	} else if (!strcmp (head, "qktk")) {
		strcpy (p->make, "Apple");
		strcpy (p->model,"QuickTake 100");
	} else if (!strcmp (head, "qktn")) {
		strcpy (p->make, "Apple");
		strcpy (p->model,"QuickTake 150");
	} else if (!memcmp (head,"FUJIFILM",8)) {
		dcr_fseek(p->obj_, 84, SEEK_SET);
		p->thumb_offset = dcr_get4(p);
		p->thumb_length = dcr_get4(p);
		dcr_fseek(p->obj_, 92, SEEK_SET);
		dcr_parse_fuji (p,dcr_get4(p));
		if (p->thumb_offset > 120) {
			dcr_fseek(p->obj_, 120, SEEK_SET);
			p->is_raw += (i = dcr_get4(p)) && 1;
			if (p->is_raw == 2 && p->opt.shot_select)
				dcr_parse_fuji (p,i);
		}
		dcr_fseek(p->obj_, 100, SEEK_SET);
		p->data_offset = dcr_get4(p);
		dcr_parse_tiff (p, p->thumb_offset+12);
	} else if (!memcmp (head,"RIFF",4)) {
		dcr_fseek(p->obj_, 0, SEEK_SET);
		dcr_parse_riff(p);
	} else if (!memcmp (head,"\0\001\0\001\0@",6)) {
		dcr_fseek(p->obj_, 6, SEEK_SET);
		dcr_fread(p->obj_, p->make, 1, 8);
		dcr_fread(p->obj_, p->model, 1, 8);
		dcr_fread(p->obj_, p->model2, 1, 16);
		p->data_offset = dcr_get2(p);
		dcr_get2(p);
		p->raw_width = dcr_get2(p);
		p->raw_height = dcr_get2(p);
		p->load_raw = &DCR_CLASS nokia_load_raw;
		p->filters = 0x61616161;
	} else if (!memcmp (head,"DSC-Image",9))
		dcr_parse_rollei(p);
	else if (!memcmp (head,"PWAD",4))
		dcr_parse_sinar_ia(p);
	else if (!memcmp (head,"\0MRM",4))
		dcr_parse_minolta(p, 0);
	else if (!memcmp (head,"FOVb",4))
		dcr_parse_foveon(p);
	else if (!memcmp (head,"CI",2))
		dcr_parse_cine(p);
	else
		for (i=0; i < sizeof table / sizeof *table; i++)
			if ((int)fsize == table[i].fsize) {
				strcpy (p->make,  table[i].make );
				strcpy (p->model, table[i].model);
				if (table[i].withjpeg)
					dcr_parse_external_jpeg(p);
			}
	if (p->make[0] == 0) dcr_parse_smal (p,0, fsize);
	if (p->make[0] == 0) dcr_parse_jpeg (p,p->is_raw = 0);

	for (i=0; i < sizeof corp / sizeof *corp; i++)
		if (strstr (p->make, corp[i]))		/* Simplify company names */
			strcpy (p->make, corp[i]);
	if (!strncmp (p->make,"KODAK",5))
		p->make[16] = p->model[16] = 0;
	cp = p->make + strlen(p->make);		/* Remove trailing spaces */
	while (*--cp == ' ') *cp = 0;
	cp = p->model + strlen(p->model);
	while (*--cp == ' ') *cp = 0;
	i = strlen(p->make);			/* Remove p->make from p->model */
	if (!strncasecmp (p->model, p->make, i) && p->model[i++] == ' ')
		memmove (p->model, p->model+i, 64-i);
	if (!strncmp (p->model,"Digital Camera ",15))
		strcpy (p->model, p->model+15);
	p->desc[511] = p->artist[63] = p->make[63] = p->model[63] = p->model2[63] = 0;
	if (!p->is_raw) goto notraw;

	if (!p->maximum) p->maximum = (1 << p->tiff_bps) - 1;
	if (!p->height) p->height = p->raw_height;
	if (!p->width)  p->width  = p->raw_width;
	if (p->fuji_width) {
		p->width = p->height + p->fuji_width;
		p->height = p->width - 1;
		p->pixel_aspect = 1;
	}
	if (p->height == 2624 && p->width == 3936)  /* Pentax K10D and Samsung GX10 */
	{   p->height  = 2616;   p->width  = 3896; }
	if (p->height == 3136 && p->width == 4864)	/* Pentax K20D */
	{   p->height  = 3124;   p->width  = 4688; }
	if (p->height == 3014 && p->width == 4096)	/* Ricoh GX200 */
		p->width  = 4014;
	if (p->dng_version) {
		if (p->filters == UINT_MAX) p->filters = 0;
		if (p->filters) p->is_raw = p->tiff_samples;
		else	 p->colors = p->tiff_samples;
		if (p->tiff_compress == 1)
			p->load_raw = &DCR_CLASS dcr_adobe_dng_load_raw_nc;
		if (p->tiff_compress == 7)
			p->load_raw = &DCR_CLASS dcr_adobe_dng_load_raw_lj;
		goto dng_skip;
	}
	if ((is_canon = !strcmp(p->make,"Canon")))
		p->load_raw = memcmp (head+6,"HEAPCCDR",8) ?
			&DCR_CLASS dcr_lossless_jpeg_load_raw : &DCR_CLASS dcr_canon_compressed_load_raw;
	if (!strcmp(p->make,"NIKON")) {
		if (!p->load_raw)
			p->load_raw = &DCR_CLASS dcr_packed_12_load_raw;
		if (p->model[0] == 'E')
			p->load_flags |= !p->data_offset << 2 | 2;
	}
	if (!strcmp(p->make,"CASIO")) {
		p->load_raw = &DCR_CLASS dcr_packed_12_load_raw;
		p->maximum = 0xf7f;
	}

	/* Set parameters based on camera name (for non-DNG files). */

	if (p->is_foveon) {
#if RESTRICTED
		if (p->height*2 < p->width) p->pixel_aspect = 0.5;
		if (p->height   > p->width) p->pixel_aspect = 2;
		p->filters = 0;
		p->load_raw = &DCR_CLASS dcr_foveon_load_raw;
		dcr_simple_coeff(p, 0);
#endif //RESTRICTED
	} else if (is_canon && p->tiff_bps == 15) {
		switch (p->width) {
		case 3344: p->width -= 66;
		case 3872: p->width -= 6;
		}
		p->filters = 0;
		p->load_raw = &DCR_CLASS dcr_canon_sraw_load_raw;
	} else if (!strcmp(p->model,"PowerShot 600")) {
		p->height = 613;
		p->width  = 854;
		p->raw_width = 896;
		p->pixel_aspect = 607/628.0;
		p->colors = 4;
		p->filters = 0xe1e4e1e4;
		p->load_raw = &DCR_CLASS dcr_canon_600_load_raw;
	} else if (!strcmp(p->model,"PowerShot A5") ||
		!strcmp(p->model,"PowerShot A5 Zoom")) {
		p->height = 773;
		p->width  = 960;
		p->raw_width = 992;
		p->pixel_aspect = 256/235.0;
		p->colors = 4;
		p->filters = 0x1e4e1e4e;
		p->load_raw = &DCR_CLASS dcr_canon_a5_load_raw;
	} else if (!strcmp(p->model,"PowerShot A50")) {
		p->height =  968;
		p->width  = 1290;
		p->raw_width = 1320;
		p->colors = 4;
		p->filters = 0x1b4e4b1e;
		p->load_raw = &DCR_CLASS dcr_canon_a5_load_raw;
	} else if (!strcmp(p->model,"PowerShot Pro70")) {
		p->height = 1024;
		p->width  = 1552;
		p->colors = 4;
		p->filters = 0x1e4b4e1b;
		p->load_raw = &DCR_CLASS dcr_canon_a5_load_raw;
	} else if (!strcmp(p->model,"PowerShot SD300")) {
		p->height = 1752;
		p->width  = 2344;
		p->raw_height = 1766;
		p->raw_width  = 2400;
		p->top_margin  = 12;
		p->left_margin = 12;
		p->load_raw = &DCR_CLASS dcr_canon_a5_load_raw;
	} else if (!strcmp(p->model,"PowerShot A460")) {
		p->height = 1960;
		p->width  = 2616;
		p->raw_height = 1968;
		p->raw_width  = 2664;
		p->top_margin  = 4;
		p->left_margin = 4;
		p->load_raw = &DCR_CLASS dcr_canon_a5_load_raw;
	} else if (!strcmp(p->model,"PowerShot A530")) {
		p->height = 1984;
		p->width  = 2620;
		p->raw_height = 1992;
		p->raw_width  = 2672;
		p->top_margin  = 6;
		p->left_margin = 10;
		p->load_raw = &DCR_CLASS dcr_canon_a5_load_raw;
		p->raw_color = 0;
	} else if (!strcmp(p->model,"PowerShot A610")) {
		if (dcr_canon_s2is(p)) strcpy (p->model+10, "S2 IS");
		p->height = 1960;
		p->width  = 2616;
		p->raw_height = 1968;
		p->raw_width  = 2672;
		p->top_margin  = 8;
		p->left_margin = 12;
		p->load_raw = &DCR_CLASS dcr_canon_a5_load_raw;
	} else if (!strcmp(p->model,"PowerShot A620")) {
		p->height = 2328;
		p->width  = 3112;
		p->raw_height = 2340;
		p->raw_width  = 3152;
		p->top_margin  = 12;
		p->left_margin = 36;
		p->load_raw = &DCR_CLASS dcr_canon_a5_load_raw;
	} else if (!strcmp(p->model,"PowerShot A720")) {
		p->height = 2472;
		p->width  = 3298;
		p->raw_height = 2480;
		p->raw_width  = 3336;
		p->top_margin  = 5;
		p->left_margin = 6;
		p->load_raw = &DCR_CLASS dcr_canon_a5_load_raw;
	} else if (!strcmp(p->model,"PowerShot A630")) {
		p->height = 2472;
		p->width  = 3288;
		p->raw_height = 2484;
		p->raw_width  = 3344;
		p->top_margin  = 6;
		p->left_margin = 12;
		p->load_raw = &DCR_CLASS dcr_canon_a5_load_raw;
	} else if (!strcmp(p->model,"PowerShot A640")) {
		p->height = 2760;
		p->width  = 3672;
		p->raw_height = 2772;
		p->raw_width  = 3736;
		p->top_margin  = 6;
		p->left_margin = 12;
		p->load_raw = &DCR_CLASS dcr_canon_a5_load_raw;
	} else if (!strcmp(p->model,"PowerShot A650")) {
		p->height = 3024;
		p->width  = 4032;
		p->raw_height = 3048;
		p->raw_width  = 4104;
		p->top_margin  = 12;
		p->left_margin = 48;
		p->load_raw = &DCR_CLASS dcr_canon_a5_load_raw;
	} else if (!strcmp(p->model,"PowerShot S3 IS")) {
		p->height = 2128;
		p->width  = 2840;
		p->raw_height = 2136;
		p->raw_width  = 2888;
		p->top_margin  = 8;
		p->left_margin = 44;
		p->load_raw = &DCR_CLASS dcr_canon_a5_load_raw;
	} else if (!strcmp(p->model,"PowerShot Pro90 IS")) {
		p->width  = 1896;
		p->colors = 4;
		p->filters = 0xb4b4b4b4;
	} else if (is_canon && p->raw_width == 2144) {
		p->height = 1550;
		p->width  = 2088;
		p->top_margin  = 8;
		p->left_margin = 4;
		if (!strcmp(p->model,"PowerShot G1")) {
			p->colors = 4;
			p->filters = 0xb4b4b4b4;
		}
	} else if (is_canon && p->raw_width == 2224) {
		p->height = 1448;
		p->width  = 2176;
		p->top_margin  = 6;
		p->left_margin = 48;
	} else if (is_canon && p->raw_width == 2376) {
		p->height = 1720;
		p->width  = 2312;
		p->top_margin  = 6;
		p->left_margin = 12;
	} else if (is_canon && p->raw_width == 2672) {
		p->height = 1960;
		p->width  = 2616;
		p->top_margin  = 6;
		p->left_margin = 12;
	} else if (is_canon && p->raw_width == 3152) {
		p->height = 2056;
		p->width  = 3088;
		p->top_margin  = 12;
		p->left_margin = 64;
		if (p->unique_id == 0x80000170)
			dcr_adobe_coeff (p, "Canon","EOS 300D");
	} else if (is_canon && p->raw_width == 3160) {
		p->height = 2328;
		p->width  = 3112;
		p->top_margin  = 12;
		p->left_margin = 44;
	} else if (is_canon && p->raw_width == 3344) {
		p->height = 2472;
		p->width  = 3288;
		p->top_margin  = 6;
		p->left_margin = 4;
	} else if (!strcmp(p->model,"EOS D2000C")) {
		p->filters = 0x61616161;
		p->black = p->curve[200];
	} else if (is_canon && p->raw_width == 3516) {
		p->top_margin  = 14;
		p->left_margin = 42;
		if (p->unique_id == 0x80000189)
			dcr_adobe_coeff (p, "Canon","EOS 350D");
		goto canon_cr2;
	} else if (is_canon && p->raw_width == 3596) {
		p->top_margin  = 12;
		p->left_margin = 74;
		goto canon_cr2;
	} else if (is_canon && p->raw_width == 3944) {
		p->height = 2602;
		p->width  = 3908;
		p->top_margin  = 18;
		p->left_margin = 30;
	} else if (is_canon && p->raw_width == 3948) {
		p->top_margin  = 18;
		p->left_margin = 42;
		p->height -= 2;
		if (p->unique_id == 0x80000236)
			dcr_adobe_coeff (p, "Canon","EOS 400D");
		if (p->unique_id == 0x80000254)
			dcr_adobe_coeff (p, "Canon","EOS 1000D");
		goto canon_cr2;
	} else if (is_canon && p->raw_width == 3984) {
		p->top_margin  = 20;
		p->left_margin = 76;
		p->height -= 2;
		goto canon_cr2;
	} else if (is_canon && p->raw_width == 4104) {
		p->height = 3024;
		p->width  = 4032;
		p->top_margin  = 12;
		p->left_margin = 48;
	} else if (is_canon && p->raw_width == 4312) {
		p->top_margin  = 18;
		p->left_margin = 22;
		p->height -= 2;
		if (p->unique_id == 0x80000176)
			dcr_adobe_coeff (p, "Canon","EOS 450D");
		goto canon_cr2;
	} else if (is_canon && p->raw_width == 4476) {
		p->top_margin  = 34;
		p->left_margin = 90;
		goto canon_cr2;
	} else if (is_canon && p->raw_width == 4480) {
		p->height = 3326;
		p->width  = 4432;
		p->top_margin  = 10;
		p->left_margin = 12;
		p->filters = 0x49494949;
	} else if (is_canon && p->raw_width == 1208) {
		p->top_margin  = 51;
		p->left_margin = 62;
		p->raw_width = p->width *= 4;
		goto canon_cr2;
	} else if (is_canon && p->raw_width == 1448) {
		p->top_margin  = 51;
		p->left_margin = 158;
		p->raw_width = p->width *= 4;
		goto canon_cr2;
	} else if (is_canon && p->raw_width == 5108) {
		p->top_margin  = 13;
		p->left_margin = 98;
canon_cr2:
		p->height -= p->top_margin;
		p->width  -= p->left_margin;
	} else if (is_canon && p->raw_width == 5712) {
		p->height = 3752;
		p->width  = 5640;
		p->top_margin  = 20;
		p->left_margin = 62;
	} else if (!strcmp(p->model,"D1")) {
		p->cam_mul[0] *= 256/527.0f;
		p->cam_mul[2] *= 256/317.0f;
	} else if (!strcmp(p->model,"D1X")) {
		p->width -= 4;
		p->pixel_aspect = 0.5;
	} else if (!strcmp(p->model,"D40X") ||
		!strcmp(p->model,"D60")  ||
		!strcmp(p->model,"D80")) {
		p->height -= 3;
		p->width  -= 4;
	} else if (!strcmp(p->model,"D3")   ||
		!strcmp(p->model,"D700")) {
		p->width -= 4;
		p->left_margin = 2;
	} else if (!strncmp(p->model,"D40",3) ||
		!strncmp(p->model,"D50",3) ||
		!strncmp(p->model,"D70",3)) {
		p->width--;
	} else if (!strcmp(p->model,"D90")) {
		p->width -= 42;
	} else if (!strcmp(p->model,"D100")) {
		if (p->tiff_compress == 34713 && !dcr_nikon_is_compressed(p)) {
			p->load_raw = &DCR_CLASS dcr_packed_12_load_raw;
			p->load_flags |= 8;
			p->raw_width = (p->width += 3) + 3;
		}
	} else if (!strcmp(p->model,"D200")) {
		p->left_margin = 1;
		p->width -= 4;
		p->filters = 0x94949494;
	} else if (!strncmp(p->model,"D2H",3)) {
		p->left_margin = 6;
		p->width -= 14;
	} else if (!strncmp(p->model,"D2X",3)) {
		if (p->width == 3264) p->width -= 32;
		else p->width -= 8;
	} else if (!strcmp(p->model,"D300")) {
		p->width -= 32;
	} else if (!strcmp(p->model,"COOLPIX P6000")) {
		p->load_flags = 1;
		p->filters = 0x94949494;
	} else if (fsize == 1581060) {
		p->height = 963;
		p->width = 1287;
		p->raw_width = 1632;
		p->load_raw = &DCR_CLASS dcr_nikon_e900_load_raw;
		p->maximum = 0x3f4;
		p->colors = 4;
		p->filters = 0x1e1e1e1e;
		dcr_simple_coeff(p, 3);
		p->pre_mul[0] = 1.2085f;
		p->pre_mul[1] = 1.0943f;
		p->pre_mul[3] = 1.1103f;
	} else if (fsize == 2465792) {
		p->height = 1203;
		p->width  = 1616;
		p->raw_width = 2048;
		p->load_raw = &DCR_CLASS dcr_nikon_e900_load_raw;
		p->colors = 4;
		p->filters = 0x4b4b4b4b;
		dcr_adobe_coeff (p, "NIKON","E950");
	} else if (fsize == 4771840) {
		p->height = 1540;
		p->width  = 2064;
		p->colors = 4;
		p->filters = 0xe1e1e1e1;
		p->load_raw = &DCR_CLASS dcr_packed_12_load_raw;
		p->load_flags = 6;
		if (!p->timestamp && dcr_nikon_e995(p))
			strcpy (p->model, "E995");
		if (strcmp(p->model,"E995")) {
			p->filters = 0xb4b4b4b4;
			dcr_simple_coeff(p, 3);
			p->pre_mul[0] = 1.196f;
			p->pre_mul[1] = 1.246f;
			p->pre_mul[2] = 1.018f;
		}
	} else if (!strcmp(p->model,"E2100")) {
		if (!p->timestamp && !dcr_nikon_e2100(p)) goto cp_e2500;
		p->height = 1206;
		p->width  = 1616;
		p->load_flags = 7;
	} else if (!strcmp(p->model,"E2500")) {
cp_e2500:
	strcpy (p->model, "E2500");
	p->height = 1204;
	p->width  = 1616;
	p->colors = 4;
	p->filters = 0x4b4b4b4b;
	} else if (fsize == 4775936) {
		p->height = 1542;
		p->width  = 2064;
		p->load_raw = &DCR_CLASS dcr_packed_12_load_raw;
		p->load_flags = 7;
		p->pre_mul[0] = 1.818f;
		p->pre_mul[2] = 1.618f;
		if (!p->timestamp) dcr_nikon_3700(p);
		if (p->model[0] == 'E' && atoi(p->model+1) < 3700)
			p->filters = 0x49494949;
		if (!strcmp(p->model,"Optio 33WR")) {
			p->flip = 1;
			p->filters = 0x16161616;
			p->pre_mul[0] = 1.331f;
			p->pre_mul[2] = 1.820f;
		}
	} else if (fsize == 5869568) {
		p->height = 1710;
		p->width  = 2288;
		p->filters = 0x16161616;
		if (!p->timestamp && dcr_minolta_z2(p)) {
			strcpy (p->make, "Minolta");
			strcpy (p->model,"DiMAGE Z2");
		}
		p->load_raw = &DCR_CLASS dcr_packed_12_load_raw;
		p->load_flags = 6 + (p->make[0] == 'M');
	} else if (!strcmp(p->model,"E4500")) {
		p->height = 1708;
		p->width  = 2288;
		p->colors = 4;
		p->filters = 0xb4b4b4b4;
	} else if (fsize == 7438336) {
		p->height = 1924;
		p->width  = 2576;
		p->colors = 4;
		p->filters = 0xb4b4b4b4;
	} else if (fsize == 8998912) {
		p->height = 2118;
		p->width  = 2832;
		p->maximum = 0xf83;
		p->load_raw = &DCR_CLASS dcr_packed_12_load_raw;
		p->load_flags = 7;
	} else if (!strcmp(p->model,"FinePix S5100") ||
		!strcmp(p->model,"FinePix S5500")) {
		p->load_raw = &DCR_CLASS dcr_unpacked_load_raw;
	} else if (!strcmp(p->make,"FUJIFILM")) {
		if (!strcmp(p->model+7,"S2Pro")) {
			strcpy (p->model+7," S2Pro");
			p->height = 2144;
			p->width  = 2880;
			p->flip = 6;
		} else
			p->maximum = 0x3e00;
		if (p->is_raw == 2 && p->opt.shot_select)
			p->maximum = 0x2f00;
		p->top_margin = (p->raw_height - p->height)/2;
		p->left_margin = (p->raw_width - p->width )/2;
		if (p->is_raw == 2)
			p->data_offset += (p->opt.shot_select > 0) * ( p->fuji_layout ?
			(p->raw_width *= 2) : p->raw_height*p->raw_width*2 );
		p->fuji_width = p->width >> !p->fuji_layout;
		p->width = (p->height >> p->fuji_layout) + p->fuji_width;
		p->raw_height = p->height;
		p->height = p->width - 1;
		p->load_raw = &DCR_CLASS dcr_fuji_load_raw;
		if (!(p->fuji_width & 1)) p->filters = 0x49494949;
	} else if (!strcmp(p->model,"RD175")) {
		p->height = 986;
		p->width = 1534;
		p->data_offset = 513;
		p->filters = 0x61616161;
		p->load_raw = &DCR_CLASS dcr_minolta_rd175_load_raw;
	} else if (!strcmp(p->model,"KD-400Z")) {
		p->height = 1712;
		p->width  = 2312;
		p->raw_width = 2336;
		goto konica_400z;
	} else if (!strcmp(p->model,"KD-510Z")) {
		goto konica_510z;
	} else if (!strcasecmp(p->make,"MINOLTA")) {
		p->load_raw = &DCR_CLASS dcr_unpacked_load_raw;
		if (!strncmp(p->model,"DiMAGE A",8)) {
			if (!strcmp(p->model,"DiMAGE A200"))
				p->filters = 0x49494949;
			p->load_raw = &DCR_CLASS dcr_packed_12_load_raw;
		} else if (!strncmp(p->model,"ALPHA",5) ||
			!strncmp(p->model,"DYNAX",5) ||
			!strncmp(p->model,"MAXXUM",6)) {
			sprintf (p->model+20, "DYNAX %-10s", p->model+6+(p->model[0]=='M'));
			dcr_adobe_coeff (p, p->make, p->model+20);
			p->load_raw = &DCR_CLASS dcr_packed_12_load_raw;
		} else if (!strncmp(p->model,"DiMAGE G",8)) {
			if (p->model[8] == '4') {
				p->height = 1716;
				p->width  = 2304;
			} else if (p->model[8] == '5') {
konica_510z:
			p->height = 1956;
			p->width  = 2607;
			p->raw_width = 2624;
			} else if (p->model[8] == '6') {
				p->height = 2136;
				p->width  = 2848;
			}
			p->data_offset += 14;
			p->filters = 0x61616161;
konica_400z:
			p->load_raw = &DCR_CLASS dcr_unpacked_load_raw;
			p->maximum = 0x3df;
			p->order = 0x4d4d;
		}
	} else if (!strcmp(p->model,"*ist DS")) {
		p->height -= 2;
	} else if (!strcmp(p->model,"K20D")) {
		p->filters = 0x16161616;
	} else if (!strcmp(p->model,"Optio S")) {
		if (fsize == 3178560) {
			p->height = 1540;
			p->width  = 2064;
			p->load_raw = &DCR_CLASS dcr_eight_bit_load_raw;
			p->cam_mul[0] *= 4;
			p->cam_mul[2] *= 4;
			p->pre_mul[0] = 1.391f;
			p->pre_mul[2] = 1.188f;
		} else {
			p->height = 1544;
			p->width  = 2068;
			p->raw_width = 3136;
			p->load_raw = &DCR_CLASS dcr_packed_12_load_raw;
			p->maximum = 0xf7c;
			p->pre_mul[0] = 1.137f;
			p->pre_mul[2] = 1.453f;
		}
	} else if (fsize == 6114240) {
		p->height = 1737;
		p->width  = 2324;
		p->raw_width = 3520;
		p->load_raw = &DCR_CLASS dcr_packed_12_load_raw;
		p->maximum = 0xf7a;
		p->pre_mul[0] = 1.980f;
		p->pre_mul[2] = 1.570f;
	} else if (!strcmp(p->model,"Optio 750Z")) {
		p->height = 2302;
		p->width  = 3072;
		p->load_raw = &DCR_CLASS dcr_packed_12_load_raw;
		p->load_flags = 7;
	} else if (!strcmp(p->model,"S85")) {
		p->height = 2448;
		p->width  = 3264;
		p->raw_width = fsize/p->height/2;
		p->order = 0x4d4d;
		p->load_raw = &DCR_CLASS dcr_unpacked_load_raw;
		p->maximum = 0xffff;
	} else if (!strcmp(p->model,"STV680 VGA")) {
		p->height = 484;
		p->width  = 644;
		p->load_raw = &DCR_CLASS dcr_eight_bit_load_raw;
		p->flip = 2;
		p->filters = 0x16161616;
		p->black = 16;
		p->pre_mul[0] = 1.097f;
		p->pre_mul[2] = 1.128f;
	} else if (!strcmp(p->model,"KAI-0340")) {
		p->height = 477;
		p->width  = 640;
		p->order = 0x4949;
		p->data_offset = 3840;
		p->load_raw = &DCR_CLASS dcr_unpacked_load_raw;
		p->pre_mul[0] = 1.561f;
		p->pre_mul[2] = 2.454f;
	} else if (!strcmp(p->model,"N95")) {
		p->height = p->raw_height - (p->top_margin = 2);
	} else if (!strcmp(p->model,"531C")) {
		p->height = 1200;
		p->width  = 1600;
		p->load_raw = &DCR_CLASS dcr_unpacked_load_raw;
		p->filters = 0x49494949;
		p->pre_mul[1] = 1.218f;
	} else if (!strcmp(p->model,"F-080C")) {
		p->height = 768;
		p->width  = 1024;
		p->load_raw = &DCR_CLASS dcr_eight_bit_load_raw;
	} else if (!strcmp(p->model,"F-145C")) {
		p->height = 1040;
		p->width  = 1392;
		p->load_raw = &DCR_CLASS dcr_eight_bit_load_raw;
	} else if (!strcmp(p->model,"F-201C")) {
		p->height = 1200;
		p->width  = 1600;
		p->load_raw = &DCR_CLASS dcr_eight_bit_load_raw;
	} else if (!strcmp(p->model,"F-510C")) {
		p->height = 1958;
		p->width  = 2588;
		p->load_raw = fsize < 7500000 ?
			&DCR_CLASS dcr_eight_bit_load_raw : &DCR_CLASS dcr_unpacked_load_raw;
		p->maximum = 0xfff0;
	} else if (!strcmp(p->model,"F-810C")) {
		p->height = 2469;
		p->width  = 3272;
		p->load_raw = &DCR_CLASS dcr_unpacked_load_raw;
		p->maximum = 0xfff0;
	} else if (!strcmp(p->model,"XCD-SX910CR")) {
		p->height = 1024;
		p->width  = 1375;
		p->raw_width = 1376;
		p->filters = 0x49494949;
		p->maximum = 0x3ff;
		p->load_raw = fsize < 2000000 ?
			&DCR_CLASS dcr_eight_bit_load_raw : &DCR_CLASS dcr_unpacked_load_raw;
	} else if (!strcmp(p->model,"2010")) {
		p->height = 1207;
		p->width  = 1608;
		p->order = 0x4949;
		p->filters = 0x16161616;
		p->data_offset = 3212;
		p->maximum = 0x3ff;
		p->load_raw = &DCR_CLASS dcr_unpacked_load_raw;
	} else if (!strcmp(p->model,"A782")) {
		p->height = 3000;
		p->width  = 2208;
		p->filters = 0x61616161;
		p->load_raw = fsize < 10000000 ?
			&DCR_CLASS dcr_eight_bit_load_raw : &DCR_CLASS dcr_unpacked_load_raw;
		p->maximum = 0xffc0;
	} else if (!strcmp(p->model,"3320AF")) {
		p->height = 1536;
		p->raw_width = p->width = 2048;
		p->filters = 0x61616161;
		p->load_raw = &DCR_CLASS dcr_unpacked_load_raw;
		p->maximum = 0x3ff;
		p->pre_mul[0] = 1.717f;
		p->pre_mul[2] = 1.138f;
		dcr_fseek(p->obj_, 0x300000, SEEK_SET);
		if ((p->order = dcr_guess_byte_order(p, 0x10000)) == 0x4d4d) {
			p->height -= (p->top_margin = 16);
			p->width -= (p->left_margin = 28);
			p->maximum = 0xf5c0;
			strcpy (p->make, "ISG");
			p->model[0] = 0;
		}
	} else if (!strcmp(p->make,"Hasselblad")) {
		if (p->load_raw == &DCR_CLASS dcr_lossless_jpeg_load_raw)
			p->load_raw = &DCR_CLASS dcr_hasselblad_load_raw;
		if (p->raw_width == 7262) {
			p->height = 5444;
			p->width  = 7248;
			p->top_margin  = 4;
			p->left_margin = 7;
			p->filters = 0x61616161;
		} else if (p->raw_width == 4090) {
			strcpy (p->model, "V96C");
			p->height -= (p->top_margin = 6);
			p->width -= (p->left_margin = 3) + 7;
			p->filters = 0x61616161;
		}
	} else if (!strcmp(p->make,"Sinar")) {
		if (!memcmp(head,"8BPS",4)) {
			dcr_fseek(p->obj_, 14, SEEK_SET);
			p->height = dcr_get4(p);
			p->width  = dcr_get4(p);
			p->filters = 0x61616161;
			p->data_offset = 68;
		}
		if (!p->load_raw) p->load_raw = &DCR_CLASS dcr_unpacked_load_raw;
		p->maximum = 0x3fff;
	} else if (!strcmp(p->make,"Leaf")) {
		p->maximum = 0x3fff;
		dcr_fseek(p->obj_, p->data_offset, SEEK_SET);
		if (dcr_ljpeg_start (p, &jh, 1) && jh.bits == 15)
			p->maximum = 0x1fff;
		if (p->tiff_samples > 1) p->filters = 0;
		if (p->tiff_samples > 1 || p->tile_length < p->raw_height)
			p->load_raw = &DCR_CLASS dcr_leaf_hdr_load_raw;
		if ((p->width | p->height) == 2048) {
			if (p->tiff_samples == 1) {
				p->filters = 1;
				strcpy (p->cdesc, "RBTG");
				strcpy (p->model, "CatchLight");
				p->top_margin =  8; p->left_margin = 18; p->height = 2032; p->width = 2016;
			} else {
				strcpy (p->model, "DCB2");
				p->top_margin = 10; p->left_margin = 16; p->height = 2028; p->width = 2022;
			}
		} else if (p->width+p->height == 3144+2060) {
			if (!p->model[0]) strcpy (p->model, "Cantare");
			if (p->width > p->height) {
				p->top_margin = 6; p->left_margin = 32; p->height = 2048;  p->width = 3072;
				p->filters = 0x61616161;
			} else {
				p->left_margin = 6;  p->top_margin = 32;  p->width = 2048; p->height = 3072;
				p->filters = 0x16161616;
			}
			if (!p->cam_mul[0] || p->model[0] == 'V') p->filters = 0;
			else p->is_raw = p->tiff_samples;
		} else if (p->width == 2116) {
			strcpy (p->model, "Valeo 6");
			p->height -= 2 * (p->top_margin = 30);
			p->width -= 2 * (p->left_margin = 55);
			p->filters = 0x49494949;
		} else if (p->width == 3171) {
			strcpy (p->model, "Valeo 6");
			p->height -= 2 * (p->top_margin = 24);
			p->width -= 2 * (p->left_margin = 24);
			p->filters = 0x16161616;
		}
	} else if (!strcmp(p->make,"LEICA") || !strcmp(p->make,"Panasonic")) {
		p->maximum = 0xfff0;
		if ((fsize-p->data_offset) / (p->width*8/7) == p->height)
			p->load_raw = &DCR_CLASS dcr_panasonic_load_raw;
		if (!p->load_raw) p->load_raw = &DCR_CLASS dcr_unpacked_load_raw;
		switch (p->width) {
		case 2568:
			dcr_adobe_coeff (p, "Panasonic","DMC-LC1");  break;
		case 3130:
			p->left_margin = -14;
		case 3170:
			p->left_margin += 18;
			p->width = 3096;
			if (p->height > 2326) {
				p->height = 2326;
				p->top_margin = 13;
				p->filters = 0x49494949;
			}
			p->zero_is_bad = 1;
			dcr_adobe_coeff (p, "Panasonic","DMC-FZ8");  break;
		case 3213:
			p->width -= 27;
		case 3177:
			p->width -= 10;
			p->filters = 0x49494949;
			p->zero_is_bad = 1;
			dcr_adobe_coeff (p, "Panasonic","DMC-L1");  break;
		case 3304:
			p->width -= 17;
			p->zero_is_bad = 1;
			dcr_adobe_coeff (p, "Panasonic","DMC-FZ30");  break;
		case 3330:
			p->width += 43;
			p->left_margin = -6;
			p->maximum = 0xf7f0;
		case 3370:
			p->width -= 82;
			p->left_margin += 15;
			if (p->height > 2480)
				p->height = 2480 - (p->top_margin = 10);
			p->filters = 0x49494949;
			p->zero_is_bad = 1;
			dcr_adobe_coeff (p, "Panasonic","DMC-FZ18");  break;
		case 3690:
			p->height -= 2;
			p->left_margin = -14;
			p->maximum = 0xf7f0;
		case 3770:
			p->width = 3672;
			if (--p->height == 2798 && (p->height = 2760))
				p->top_margin = 15;
			else p->filters = 0x49494949;
			p->left_margin += 17;
			p->zero_is_bad = 1;
			dcr_adobe_coeff (p, "Panasonic","DMC-FZ50");  break;
		case 3710:
			p->width = 3682;
			p->filters = 0x49494949;
			dcr_adobe_coeff (p, "Panasonic","DMC-L10");  break;
		case 3724:
			p->width -= 14;
		case 3836:
			p->width -= 42;
lx3:	p->filters = 0x16161616;
		if (p->make[0] != 'P')
			dcr_adobe_coeff (p, "Panasonic","DMC-LX3");
		break;
		case 3880:
			p->width -= 22;
			p->left_margin = 6;
			p->zero_is_bad = 1;
			dcr_adobe_coeff (p, "Panasonic","DMC-LX1");  break;
		case 4060:
			p->width = 3982;
			if (p->height == 2250) goto lx3;
			p->width = 4018;
			p->filters = 0x49494949;
			p->zero_is_bad = 1;
			dcr_adobe_coeff (p, "Panasonic","DMC-G1");  break;
		case 4290:
			p->height += 38;
			p->left_margin = -14;
			p->filters = 0x49494949;
		case 4330:
			p->width = 4248;
			if ((p->height -= 39) == 2400)
				p->top_margin = 15;
			p->left_margin += 17;
			dcr_adobe_coeff (p, "Panasonic","DMC-LX2");  break;
		case 4508:
			p->height -= 6;
			p->width = 4429;
			p->filters = 0x16161616;
			dcr_adobe_coeff (p, "Panasonic","DMC-FX150");  break;
		}
	} else if (!strcmp(p->model,"C770UZ")) {
		p->height = 1718;
		p->width  = 2304;
		p->filters = 0x16161616;
		p->load_raw = &DCR_CLASS dcr_packed_12_load_raw;
		p->load_flags = 7;
	} else if (!strcmp(p->make,"OLYMPUS")) {
		p->height += p->height & 1;
		p->filters = p->exif_cfa;
		if (p->load_raw == &DCR_CLASS dcr_olympus_e410_load_raw) {
			p->black >>= 4;
		} else if (!strcmp(p->model,"E-10") ||
			!strncmp(p->model,"E-20",4)) {
			p->black <<= 2;
		} else if (!strcmp(p->model,"E-300") ||
			!strcmp(p->model,"E-500")) {
			p->width -= 20;
			if (p->load_raw == &DCR_CLASS dcr_unpacked_load_raw) {
				p->maximum = 0xfc30;
				p->black = 0;
			}
		} else if (!strcmp(p->model,"E-330")) {
			p->width -= 30;
			if (p->load_raw == &DCR_CLASS dcr_unpacked_load_raw)
				p->maximum = 0xf790;
		} else if (!strcmp(p->model,"SP550UZ")) {
			p->thumb_length = fsize - (p->thumb_offset = 0xa39800);
			p->thumb_height = 480;
			p->thumb_width  = 640;
		}
	} else if (!strcmp(p->model,"N Digital")) {
		p->height = 2047;
		p->width  = 3072;
		p->filters = 0x61616161;
		p->data_offset = 0x1a00;
		p->load_raw = &DCR_CLASS dcr_packed_12_load_raw;
	} else if (!strcmp(p->model,"DSC-F828")) {
		p->width = 3288;
		p->left_margin = 5;
		p->data_offset = 862144;
		p->load_raw = &DCR_CLASS dcr_sony_load_raw;
		p->filters = 0x9c9c9c9c;
		p->colors = 4;
		strcpy (p->cdesc, "RGBE");
	} else if (!strcmp(p->model,"DSC-V3")) {
		p->width = 3109;
		p->left_margin = 59;
		p->data_offset = 787392;
		p->load_raw = &DCR_CLASS dcr_sony_load_raw;
	} else if (!strcmp(p->make,"SONY") && p->raw_width == 3984) {
		dcr_adobe_coeff (p, "SONY","DSC-R1");
		p->width = 3925;
		p->order = 0x4d4d;
	} else if (!strcmp(p->model,"DSLR-A100")) {
		p->height--;
	} else if (!strcmp(p->model,"DSLR-A350")) {
		p->height -= 4;
	} else if (!strcmp(p->model,"C603v")) {
		p->height = 480;
		p->width  = 640;
		goto c603v;
	} else if (!strcmp(p->model,"C603y")) {
		p->height = 2134;
		p->width  = 2848;
c603v:
		p->filters = 0;
		p->load_raw = &DCR_CLASS dcr_kodak_yrgb_load_raw;
	} else if (!strcmp(p->model,"C603")) {
		p->raw_height = p->height = 2152;
		p->raw_width  = p->width  = 2864;
		goto c603;
	} else if (!strcmp(p->model,"C330")) {
		p->height = 1744;
		p->width  = 2336;
		p->raw_height = 1779;
		p->raw_width  = 2338;
		p->top_margin = 33;
		p->left_margin = 1;
c603:
		p->order = 0x4949;
		if ((p->data_offset = fsize - p->raw_height*p->raw_width)) {
			dcr_fseek(p->obj_, 168, SEEK_SET);
			dcr_read_shorts (p, p->curve, 256);
		} else p->use_gamma = 0;
		p->load_raw = &DCR_CLASS dcr_eight_bit_load_raw;
	} else if (!strcasecmp(p->make,"KODAK")) {
		if (p->filters == UINT_MAX) p->filters = 0x61616161;
		if (!strncmp(p->model,"NC2000",6)) {
			p->width -= 4;
			p->left_margin = 2;
		} else if (!strcmp(p->model,"EOSDCS3B")) {
			p->width -= 4;
			p->left_margin = 2;
		} else if (!strcmp(p->model,"EOSDCS1")) {
			p->width -= 4;
			p->left_margin = 2;
		} else if (!strcmp(p->model,"DCS420")) {
			p->width -= 4;
			p->left_margin = 2;
		} else if (!strcmp(p->model,"DCS460")) {
			p->width -= 4;
			p->left_margin = 2;
		} else if (!strcmp(p->model,"DCS460A")) {
			p->width -= 4;
			p->left_margin = 2;
			p->colors = 1;
			p->filters = 0;
		} else if (!strcmp(p->model,"DCS660M")) {
			p->black = 214;
			p->colors = 1;
			p->filters = 0;
		} else if (!strcmp(p->model,"DCS760M")) {
			p->colors = 1;
			p->filters = 0;
		}
		if (strstr(p->model,"DC25")) {
			strcpy (p->model, "DC25");
			p->data_offset = 15424;
		}
		if (!strncmp(p->model,"DC2",3)) {
			p->height = 242;
			if (fsize < 100000) {
				p->raw_width = 256; p->width = 249;
				p->pixel_aspect = (4.0*p->height) / (3.0*p->width);
			} else {
				p->raw_width = 512; p->width = 501;
				p->pixel_aspect = (493.0*p->height) / (373.0*p->width);
			}
			p->data_offset += p->raw_width + 1;
			p->colors = 4;
			p->filters = 0x8d8d8d8d;
			dcr_simple_coeff(p, 1);
			p->pre_mul[1] = 1.179f;
			p->pre_mul[2] = 1.209f;
			p->pre_mul[3] = 1.036f;
			p->load_raw = &DCR_CLASS dcr_eight_bit_load_raw;
		} else if (!strcmp(p->model,"40")) {
			strcpy (p->model, "DC40");
			p->height = 512;
			p->width  = 768;
			p->data_offset = 1152;
			p->load_raw = &DCR_CLASS dcr_kodak_radc_load_raw;
		} else if (strstr(p->model,"DC50")) {
			strcpy (p->model, "DC50");
			p->height = 512;
			p->width  = 768;
			p->data_offset = 19712;
			p->load_raw = &DCR_CLASS dcr_kodak_radc_load_raw;
		} else if (strstr(p->model,"DC120")) {
			strcpy (p->model, "DC120");
			p->height = 976;
			p->width  = 848;
			p->pixel_aspect = p->height/0.75/p->width;
			p->load_raw = p->tiff_compress == 7 ?
				&DCR_CLASS dcr_kodak_jpeg_load_raw : &DCR_CLASS dcr_kodak_dc120_load_raw;
		} else if (!strcmp(p->model,"DCS200")) {
			p->thumb_height = 128;
			p->thumb_width  = 192;
			p->thumb_offset = 6144;
			p->thumb_misc   = 360;
			p->write_thumb = &DCR_CLASS dcr_layer_thumb;
			p->height = 1024;
			p->width  = 1536;
			p->data_offset = 79872;
			p->load_raw = &DCR_CLASS dcr_eight_bit_load_raw;
			p->black = 17;
		}
	} else if (!strcmp(p->model,"Fotoman Pixtura")) {
		p->height = 512;
		p->width  = 768;
		p->data_offset = 3632;
		p->load_raw = &DCR_CLASS dcr_kodak_radc_load_raw;
		p->filters = 0x61616161;
		dcr_simple_coeff(p, 2);
	} else if (!strcmp(p->model,"QuickTake 100")) {
		dcr_fseek(p->obj_, 544, SEEK_SET);
		p->height = dcr_get2(p);
		p->width  = dcr_get2(p);
		p->data_offset = (dcr_get4(p),dcr_get2(p)) == 30 ? 738:736;
		if (p->height > p->width) {
			SWAP(p->height,p->width);
			dcr_fseek(p->obj_, p->data_offset-6, SEEK_SET);
			p->flip = ~dcr_get2(p) & 3 ? 5:6;
		}
		p->load_raw = &DCR_CLASS dcr_quicktake_100_load_raw;
		p->filters = 0x61616161;
	} else if (!strcmp(p->model,"QuickTake 150")) {
		p->data_offset = 738 - head[5];
		if (head[5]) strcpy (p->model+10, "200");
		p->load_raw = &DCR_CLASS dcr_kodak_radc_load_raw;
		p->height = 480;
		p->width  = 640;
		p->filters = 0x61616161;
	} else if (!strcmp(p->make,"Rollei") && !p->load_raw) {
		switch (p->raw_width) {
		case 1316:
			p->height = 1030;
			p->width  = 1300;
			p->top_margin  = 1;
			p->left_margin = 6;
			break;
		case 2568:
			p->height = 1960;
			p->width  = 2560;
			p->top_margin  = 2;
			p->left_margin = 8;
		}
		p->filters = 0x16161616;
		p->load_raw = &DCR_CLASS dcr_rollei_load_raw;
		p->pre_mul[0] = 1.8f;
		p->pre_mul[2] = 1.3f;
	} else if (!strcmp(p->model,"PC-CAM 600")) {
		p->height = 768;
		p->data_offset = p->width = 1024;
		p->filters = 0x49494949;
		p->load_raw = &DCR_CLASS dcr_eight_bit_load_raw;
		p->pre_mul[0] = 1.14f;
		p->pre_mul[2] = 2.73f;
	} else if (!strcmp(p->model,"QV-2000UX")) {
		p->height = 1208;
		p->width  = 1632;
		p->data_offset = p->width * 2;
		p->load_raw = &DCR_CLASS dcr_eight_bit_load_raw;
	} else if (fsize == 3217760) {
		p->height = 1546;
		p->width  = 2070;
		p->raw_width = 2080;
		p->load_raw = &DCR_CLASS dcr_eight_bit_load_raw;
	} else if (!strcmp(p->model,"QV-4000")) {
		p->height = 1700;
		p->width  = 2260;
		p->load_raw = &DCR_CLASS dcr_unpacked_load_raw;
		p->maximum = 0xffff;
	} else if (!strcmp(p->model,"QV-5700")) {
		p->height = 1924;
		p->width  = 2576;
		p->load_raw = &DCR_CLASS dcr_casio_qv5700_load_raw;
	} else if (!strcmp(p->model,"QV-R41")) {
		p->height = 1720;
		p->width  = 2312;
		p->raw_width = 3520;
		p->left_margin = 2;
	} else if (!strcmp(p->model,"QV-R51")) {
		p->height = 1926;
		p->width  = 2580;
		p->raw_width = 3904;
		p->pre_mul[0] = 1.340f;
		p->pre_mul[2] = 1.672f;
	} else if (!strcmp(p->model,"EX-S100")) {
		p->height = 1544;
		p->width  = 2058;
		p->raw_width = 3136;
		p->pre_mul[0] = 1.631f;
		p->pre_mul[2] = 1.106f;
	} else if (!strcmp(p->model,"EX-Z50")) {
		p->height = 1931;
		p->width  = 2570;
		p->raw_width = 3904;
		p->pre_mul[0] = 2.529f;
		p->pre_mul[2] = 1.185f;
	} else if (!strcmp(p->model,"EX-Z55")) {
		p->height = 1960;
		p->width  = 2570;
		p->raw_width = 3904;
		p->pre_mul[0] = 1.520f;
		p->pre_mul[2] = 1.316f;
	} else if (!strcmp(p->model,"EX-P505")) {
		p->height = 1928;
		p->width  = 2568;
		p->raw_width = 3852;
		p->maximum = 0xfff;
		p->pre_mul[0] = 2.07f;
		p->pre_mul[2] = 1.88f;
	} else if (fsize == 9313536) {	/* EX-P600 or QV-R61 */
		p->height = 2142;
		p->width  = 2844;
		p->raw_width = 4288;
		p->pre_mul[0] = 1.797f;
		p->pre_mul[2] = 1.219f;
	} else if (!strcmp(p->model,"EX-P700")) {
		p->height = 2318;
		p->width  = 3082;
		p->raw_width = 4672;
		p->pre_mul[0] = 1.758f;
		p->pre_mul[2] = 1.504f;
	}
	if (!p->model[0])
		sprintf (p->model, "%dx%d", p->width, p->height);
	if (p->filters == UINT_MAX) p->filters = 0x94949494;
	if (p->raw_color) dcr_adobe_coeff (p, p->make, p->model);
	if (p->thumb_offset && !p->thumb_height) {
		dcr_fseek(p->obj_, p->thumb_offset, SEEK_SET);
		if (dcr_ljpeg_start (p,&jh, 1)) {
			p->thumb_width  = jh.wide;
			p->thumb_height = jh.high;
		}
	}
dng_skip:
	if (!p->load_raw || p->height < 22) p->is_raw = 0;
#ifdef NO_JPEG
	if (p->load_raw == &DCR_CLASS dcr_kodak_jpeg_load_raw) {
		fprintf (stderr,_("%s: You must link dcraw with libjpeg!!\n"), p->ifname);
		p->is_raw = 0;
	}
#endif
	if (!p->cdesc[0])
		strcpy (p->cdesc, p->colors == 3 ? "RGB":"GMCY");
	if (!p->raw_height) p->raw_height = p->height;
	if (!p->raw_width ) p->raw_width  = p->width;
	if (p->filters && p->colors == 3)
		for (i=0; i < 32; i+=4) {
			if ((p->filters >> i & 15) == 9)
				p->filters |= 2 << i;
			if ((p->filters >> i & 15) == 6)
				p->filters |= 8 << i;
		}
notraw:
		if (p->flip == -1) p->flip = p->tiff_flip;
		if (p->flip == -1) p->flip = 0;
}

#ifndef NO_LCMS
void DCR_CLASS dcr_apply_profile (DCRAW* p, char *input, char *output)
{
	char *prof;
	cmsHPROFILE hInProfile=0, hOutProfile=0;
	cmsHTRANSFORM hTransform;
	FILE *fp;
	unsigned size;

	cmsErrorAction (LCMS_ERROR_SHOW);
	if (strcmp (input, "embed"))
		hInProfile = cmsOpenProfileFromFile (input, "r");
	else if (p->profile_length) {
		prof = (char *) malloc (p->profile_length);
		dcr_merror (p, prof, "apply_profile()");
		dcr_fseek(p->obj_, p->profile_offset, SEEK_SET);
		dcr_fread(p->obj_, prof, 1, p->profile_length);
		hInProfile = cmsOpenProfileFromMem (prof, p->profile_length);
		free (prof);
	} else
		fprintf (stderr,_("%s has no embedded profile.\n"), p->ifname);
	if (!hInProfile) return;
	if (!output)
		hOutProfile = cmsCreate_sRGBProfile();
	else if ((fp = fopen (output, "rb"))) {
		dcr_fread(p->obj_, &size, 4, 1);
		fseek (fp, 0, SEEK_SET);
		p->oprof = (unsigned *) malloc (size = ntohl(size));
		dcr_merror (p, p->oprof, "apply_profile()");
		dcr_fread(p->obj_, p->oprof, 1, size);
		fclose (fp);
		if (!(hOutProfile = cmsOpenProfileFromMem (p->oprof, size))) {
			free (p->oprof);
			p->oprof = 0;
		}
	} else
		fprintf (stderr,_("Cannot open file %s!\n"), output);
	if (!hOutProfile) goto quit;
	if (p->opt.verbose)
		fprintf (stderr,_("Applying color profile...\n"));
	hTransform = cmsCreateTransform (hInProfile, TYPE_RGBA_16,
		hOutProfile, TYPE_RGBA_16, INTENT_PERCEPTUAL, 0);
	cmsDoTransform (hTransform, p->image, p->image, p->width*p->height);
	p->raw_color = 1;		/* Don't use rgb_cam with a profile */
	cmsDeleteTransform (hTransform);
	cmsCloseProfile (hOutProfile);
quit:
	cmsCloseProfile (hInProfile);
}
#endif

void DCR_CLASS dcr_convert_to_rgb(DCRAW* p)
{
	int row, col, c, i, j, k;
	ushort *img;
	float out[3], out_cam[3][4];
	double num, inverse[3][3];
	static const double xyzd50_srgb[3][3] =
	{ { 0.436083, 0.385083, 0.143055 },
    { 0.222507, 0.716888, 0.060608 },
    { 0.013930, 0.097097, 0.714022 } };
	static const double rgb_rgb[3][3] =
	{ { 1,0,0 }, { 0,1,0 }, { 0,0,1 } };
	static const double adobe_rgb[3][3] =
	{ { 0.715146, 0.284856, 0.000000 },
    { 0.000000, 1.000000, 0.000000 },
    { 0.000000, 0.041166, 0.958839 } };
	static const double wide_rgb[3][3] =
	{ { 0.593087, 0.404710, 0.002206 },
    { 0.095413, 0.843149, 0.061439 },
    { 0.011621, 0.069091, 0.919288 } };
	static const double prophoto_rgb[3][3] =
	{ { 0.529317, 0.330092, 0.140588 },
    { 0.098368, 0.873465, 0.028169 },
    { 0.016879, 0.117663, 0.865457 } };

	static const double (*out_rgb[])[3] =
	{ rgb_rgb, adobe_rgb, wide_rgb, prophoto_rgb, xyz_rgb };
	static const char *name[] =
	{ "sRGB", "Adobe RGB (1998)", "WideGamut D65", "ProPhoto D65", "XYZ" };
	static const unsigned phead[] =
	{ 1024, 0, 0x2100000, 0x6d6e7472, 0x52474220, 0x58595a20, 0, 0, 0,
    0x61637370, 0, 0, 0x6e6f6e65, 0, 0, 0, 0, 0xf6d6, 0x10000, 0xd32d };
	unsigned pbody[] =
	{ 10, 0x63707274, 0, 36,	/* cprt */
	0x64657363, 0, 40,	/* desc */
	0x77747074, 0, 20,	/* wtpt */
	0x626b7074, 0, 20,	/* bkpt */
	0x72545243, 0, 14,	/* rTRC */
	0x67545243, 0, 14,	/* gTRC */
	0x62545243, 0, 14,	/* bTRC */
	0x7258595a, 0, 20,	/* rXYZ */
	0x6758595a, 0, 20,	/* gXYZ */
	0x6258595a, 0, 20 };	/* bXYZ */
	static const unsigned pwhite[] = { 0xf351, 0x10000, 0x116cc };
	unsigned pcurve[] = { 0x63757276, 0, 1, 0x1000000 };

	memcpy (out_cam, p->rgb_cam, sizeof out_cam);
	p->raw_color |= p->colors == 1 || p->opt.document_mode ||
		p->opt.output_color < 1 || p->opt.output_color > 5;
	if (!p->raw_color) {
		p->oprof = (unsigned *) calloc (phead[0], 1);
		dcr_merror (p, p->oprof, "convert_to_rgb()");
		memcpy (p->oprof, phead, sizeof phead);
		if (p->opt.output_color == 5) p->oprof[4] = p->oprof[5];
		p->oprof[0] = 132 + 12*pbody[0];
		for (i=0; i < (int)pbody[0]; i++) {
			p->oprof[p->oprof[0]/4] = i ? (i > 1 ? 0x58595a20 : 0x64657363) : 0x74657874;
			pbody[i*3+2] = p->oprof[0];
			p->oprof[0] += (pbody[i*3+3] + 3) & -4;
		}
		memcpy (p->oprof+32, pbody, sizeof pbody);
		p->oprof[pbody[5]/4+2] = strlen(name[p->opt.output_color-1]) + 1;
		memcpy ((char *)p->oprof+pbody[8]+8, pwhite, sizeof pwhite);
		if (p->opt.output_bps == 8)
#ifdef SRGB_GAMMA
			pcurve[3] = 0x2330000;
#else
		pcurve[3] = 0x1f00000;
#endif
		for (i=4; i < 7; i++)
			memcpy ((char *)p->oprof+pbody[i*3+2], pcurve, sizeof pcurve);
		dcr_pseudoinverse ((double (*)[3]) out_rgb[p->opt.output_color-1], inverse, 3);
		for (i=0; i < 3; i++)
			for (j=0; j < 3; j++) {
				for (num = k=0; k < 3; k++)
					num += xyzd50_srgb[i][k] * inverse[j][k];
				p->oprof[pbody[j*3+23]/4+i+2] = (unsigned int)(num * 0x10000 + 0.5);
			}
			for (i=0; i < (int)(phead[0]/4); i++)
				p->oprof[i] = htonl(p->oprof[i]);
			strcpy ((char *)p->oprof+pbody[2]+8, "auto-generated by dcraw");
			strcpy ((char *)p->oprof+pbody[5]+12, name[p->opt.output_color-1]);
			for (i=0; i < 3; i++)
				for (j=0; j < p->colors; j++)
					for (out_cam[i][j] = (float)(k=0); k < 3; k++)
						out_cam[i][j] += (float)out_rgb[p->opt.output_color-1][i][k] * p->rgb_cam[k][j];
	}
	if (p->opt.verbose)
		fprintf (stderr, p->raw_color ? _("Building histograms...\n") :
	_("Converting to %s colorspace...\n"), name[p->opt.output_color-1]);

	memset (p->histogram, 0, sizeof p->histogram);
	for (img=p->image[0], row=0; row < p->height; row++)
		for (col=0; col < p->width; col++, img+=4) {
			if (!p->raw_color) {
				out[0] = out[1] = out[2] = 0;
				FORCC(p) {
					out[0] += out_cam[0][c] * img[c];
					out[1] += out_cam[1][c] * img[c];
					out[2] += out_cam[2][c] * img[c];
				}
				FORC3 img[c] = CLIP((int) out[c]);
			}
			else if (p->opt.document_mode)
				img[0] = img[FC(row,col)];
			FORCC(p) p->histogram[c][img[c] >> 3]++;
		}
	if (p->colors == 4 && p->opt.output_color) p->colors = 3;
	if (p->opt.document_mode && p->filters) p->colors = 1;
}

void DCR_CLASS dcr_fuji_rotate(DCRAW* p)
{
	int i, row, col;
	double step;
	float r, c, fr, fc;
	unsigned ur, uc;
	ushort wide, high, (*img)[4], (*pix)[4];

	if (!p->fuji_width) return;
	if (p->opt.verbose)
		fprintf (stderr,_("Rotating image 45 degrees...\n"));
	p->fuji_width = (p->fuji_width - 1 + p->shrink) >> p->shrink;
	step = sqrt(0.5);
	wide = (unsigned short)(p->fuji_width / step);
	high = (unsigned short)((p->height - p->fuji_width) / step);
	img = (ushort (*)[4]) calloc (wide*high, sizeof *img);
	dcr_merror (p, img, "fuji_rotate()");

	for (row=0; row < high; row++)
		for (col=0; col < wide; col++) {
			r = (float)(p->fuji_width + (row-col)*step);
			ur = (unsigned int)r;
			c = (float)((row+col)*step);
			uc = (unsigned int)c;
			if ((int)ur > (int)(p->height-2) || (int)uc > (int)(p->width-2)) continue;
			fr = r - ur;
			fc = c - uc;
			pix = p->image + ur*p->width + uc;
			for (i=0; i < p->colors; i++)
				img[row*wide+col][i] = (unsigned short)(
				(pix[    0][i]*(1-fc) + pix[      1][i]*fc) * (1-fr) +
				(pix[p->width][i]*(1-fc) + pix[p->width+1][i]*fc) * fr);
		}
	free (p->image);
	p->width  = wide;
	p->height = high;
	p->image  = img;
	p->fuji_width = 0;
}

void DCR_CLASS dcr_stretch(DCRAW* p)
{
	ushort newdim, (*img)[4], *pix0, *pix1;
	int row, col, c;
	double rc, frac;

	if (p->pixel_aspect == 1) return;
	if (p->opt.verbose) fprintf (stderr,_("Stretching the image...\n"));
	if (p->pixel_aspect < 1) {
		newdim = (unsigned short)(p->height / p->pixel_aspect + 0.5);
		img = (ushort (*)[4]) calloc (p->width*newdim, sizeof *img);
		dcr_merror (p, img, "stretch()");
		for (rc=row=0; row < newdim; row++, rc+=p->pixel_aspect) {
			frac = rc - (c = (int)rc);
			pix0 = pix1 = p->image[c*p->width];
			if (c+1 < p->height) pix1 += p->width*4;
			for (col=0; col < p->width; col++, pix0+=4, pix1+=4)
				FORCC(p) img[row*p->width+col][c] = (unsigned short)(pix0[c]*(1-frac) + pix1[c]*frac + 0.5);
		}
		p->height = newdim;
	} else {
		newdim = (unsigned short)(p->width * p->pixel_aspect + 0.5);
		img = (ushort (*)[4]) calloc (p->height*newdim, sizeof *img);
		dcr_merror (p, img, "stretch()");
		for (rc=col=0; col < newdim; col++, rc+=1/p->pixel_aspect) {
			frac = rc - (c = (int)rc);
			pix0 = pix1 = p->image[c];
			if (c+1 < p->width) pix1 += 4;
			for (row=0; row < p->height; row++, pix0+=p->width*4, pix1+=p->width*4)
				FORCC(p) img[row*newdim+col][c] = (unsigned short)(pix0[c]*(1-frac) + pix1[c]*frac + 0.5);
		}
		p->width = newdim;
	}
	free (p->image);
	p->image = img;
}

int DCR_CLASS dcr_flip_index (DCRAW* p, int row, int col)
{
	if (p->flip & 4) SWAP(row,col);
	if (p->flip & 2) row = p->iheight - 1 - row;
	if (p->flip & 1) col = p->iwidth  - 1 - col;
	return row * p->iwidth + col;
}

void DCR_CLASS dcr_gamma_lut (DCRAW* p, uchar lut[0x10000])
{
	int perc, c, val, total, i;
	float white=0, r;

	perc = (int)(p->width * p->height * 0.01);		/* 99th percentile white level */
	if (p->fuji_width) perc /= 2;
	if ((p->opt.highlight & ~2) || p->opt.no_auto_bright) perc = -1;
	FORCC(p) {
		for (val=0x2000, total=0; --val > 32; )
			if ((total += p->histogram[c][val]) > perc) break;
			if (white < val) white = (float)val;
	}
	white *= 8 / p->opt.bright;
	for (i=0; i < 0x10000; i++) {
		r = i / white;
		val = (int)(256 * ( !p->use_gamma ? r :
#ifdef SRGB_GAMMA
		r <= 0.00304 ? r*12.92 : pow(r,2.5/6)*1.055-0.055 ));
#else
		r <= 0.018 ? r*4.5 : pow(r,0.45)*1.099-0.099 ));
#endif
		if (val > 255) val = 255;
		lut[i] = val;
	}
}

void DCR_CLASS dcr_tiff_set (ushort *ntag,
							 ushort tag, ushort type, int count, int val)
{
	struct dcr_tiff_tag *tt;
	int c;

	tt = (struct dcr_tiff_tag *)(ntag+1) + (*ntag)++;
	tt->tag = tag;
	tt->type = type;
	tt->count = count;
	if (type < 3 && count <= 4)
		FORC(4) tt->val.c[c] = val >> (c << 3);
	else if (type == 3 && count <= 2)
		FORC(2) tt->val.s[c] = val >> (c << 4);
	else tt->val.i = val;
}

#define TOFF(ptr) ((char *)(&(ptr)) - (char *)th)

void DCR_CLASS dcr_tiff_head (DCRAW* p, struct dcr_tiff_hdr *th, int full)
{
	int c, psize=0;
	struct tm *t;

	memset (th, 0, sizeof *th);
	th->order = (unsigned short)(htonl(0x4d4d4949) >> 16);
	th->magic = 42;
	th->ifd = 10;
	if (full) {
		dcr_tiff_set (&th->ntag, 254, 4, 1, 0);
		dcr_tiff_set (&th->ntag, 256, 4, 1, p->width);
		dcr_tiff_set (&th->ntag, 257, 4, 1, p->height);
		dcr_tiff_set (&th->ntag, 258, 3, p->colors, p->opt.output_bps);
		if (p->colors > 2)
			th->tag[th->ntag-1].val.i = TOFF(th->bps);
		FORC4 th->bps[c] = p->opt.output_bps;
		dcr_tiff_set (&th->ntag, 259, 3, 1, 1);
		dcr_tiff_set (&th->ntag, 262, 3, 1, 1 + (p->colors > 1));
	}
	dcr_tiff_set (&th->ntag, 270, 2, 512, TOFF(th->desc));
	dcr_tiff_set (&th->ntag, 271, 2, 64, TOFF(th->make));
	dcr_tiff_set (&th->ntag, 272, 2, 64, TOFF(th->model));
	if (full) {
		if (p->oprof) psize = ntohl(p->oprof[0]);
		dcr_tiff_set (&th->ntag, 273, 4, 1, sizeof *th + psize);
		dcr_tiff_set (&th->ntag, 277, 3, 1, p->colors);
		dcr_tiff_set (&th->ntag, 278, 4, 1, p->height);
		dcr_tiff_set (&th->ntag, 279, 4, 1, p->height*p->width*p->colors*p->opt.output_bps/8);
	} else
		dcr_tiff_set (&th->ntag, 274, 3, 1, "12435867"[p->flip]-'0');
	dcr_tiff_set (&th->ntag, 282, 5, 1, TOFF(th->rat[0]));
	dcr_tiff_set (&th->ntag, 283, 5, 1, TOFF(th->rat[2]));
	dcr_tiff_set (&th->ntag, 284, 3, 1, 1);
	dcr_tiff_set (&th->ntag, 296, 3, 1, 2);
	dcr_tiff_set (&th->ntag, 305, 2, 32, TOFF(th->soft));
	dcr_tiff_set (&th->ntag, 306, 2, 20, TOFF(th->date));
	dcr_tiff_set (&th->ntag, 315, 2, 64, TOFF(th->artist));
	dcr_tiff_set (&th->ntag, 34665, 4, 1, TOFF(th->nexif));
	if (psize) dcr_tiff_set (&th->ntag, 34675, 7, psize, sizeof *th);
	dcr_tiff_set (&th->nexif, 33434, 5, 1, TOFF(th->rat[4]));
	dcr_tiff_set (&th->nexif, 33437, 5, 1, TOFF(th->rat[6]));
	dcr_tiff_set (&th->nexif, 34855, 3, 1, (int)p->iso_speed);
	dcr_tiff_set (&th->nexif, 37386, 5, 1, TOFF(th->rat[8]));
	if (p->gpsdata[1]) {
		dcr_tiff_set (&th->ntag, 34853, 4, 1, TOFF(th->ngps));
		dcr_tiff_set (&th->ngps,  0, 1,  4, 0x202);
		dcr_tiff_set (&th->ngps,  1, 2,  2, p->gpsdata[29]);
		dcr_tiff_set (&th->ngps,  2, 5,  3, TOFF(th->gps[0]));
		dcr_tiff_set (&th->ngps,  3, 2,  2, p->gpsdata[30]);
		dcr_tiff_set (&th->ngps,  4, 5,  3, TOFF(th->gps[6]));
		dcr_tiff_set (&th->ngps,  5, 1,  1, p->gpsdata[31]);
		dcr_tiff_set (&th->ngps,  6, 5,  1, TOFF(th->gps[18]));
		dcr_tiff_set (&th->ngps,  7, 5,  3, TOFF(th->gps[12]));
		dcr_tiff_set (&th->ngps, 18, 2, 12, TOFF(th->gps[20]));
		dcr_tiff_set (&th->ngps, 29, 2, 12, TOFF(th->gps[23]));
		memcpy (th->gps, p->gpsdata, sizeof th->gps);
	}
	th->rat[0] = th->rat[2] = 300;
	th->rat[1] = th->rat[3] = 1;
	FORC(6) th->rat[4+c] = 1000000;
	th->rat[4] = (int)(th->rat[4] * p->shutter);
	th->rat[6] = (int)(th->rat[6] * p->aperture);
	th->rat[8] = (int)(th->rat[8] * p->focal_len);
	strncpy (th->desc, p->desc, 512);
	strncpy (th->make, p->make, 64);
	strncpy (th->model, p->model, 64);
	strcpy (th->soft, "dcraw v"DCR_VERSION);
	t = gmtime (&p->timestamp);
	sprintf (th->date, "%04d:%02d:%02d %02d:%02d:%02d",
		t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
	strncpy (th->artist, p->artist, 64);
}

void DCR_CLASS dcr_jpeg_thumb (DCRAW* p, FILE *tfp)
{
	char *thumb;
	ushort exif[5];
	struct dcr_tiff_hdr th;

	thumb = (char *) malloc (p->thumb_length);
	dcr_merror (p, thumb, "jpeg_thumb()");
	dcr_fread(p->obj_, thumb, 1, p->thumb_length);
	fputc (0xff, tfp);
	fputc (0xd8, tfp);
	if (strcmp (thumb+6, "Exif")) {
		memcpy (exif, "\xff\xe1  Exif\0\0", 10);
		exif[1] = htons (8 + sizeof th);
		fwrite (exif, 1, sizeof exif, tfp);
		dcr_tiff_head (p,&th, 0);
		fwrite (&th, 1, sizeof th, tfp);
	}
	fwrite (thumb+2, 1, p->thumb_length-2, tfp);
	free (thumb);
}

void DCR_CLASS dcr_write_ppm_tiff (DCRAW* p, FILE *ofp)
{
	struct dcr_tiff_hdr th;
	uchar *ppm, lut[0x10000];
	ushort *ppm2;
	int c, row, col, soff, rstep, cstep;

	p->iheight = p->height;
	p->iwidth  = p->width;
	if (p->flip & 4) SWAP(p->height,p->width);
	ppm = (uchar *) calloc (p->width, p->colors*p->opt.output_bps/8);
	ppm2 = (ushort *) ppm;
	dcr_merror (p, ppm, "write_ppm_tiff()");
	if (p->opt.output_tiff) {
		dcr_tiff_head (p, &th, 1);
		fwrite (&th, sizeof th, 1, ofp);
		if (p->oprof)
			fwrite (p->oprof, ntohl(p->oprof[0]), 1, ofp);
	} else if (p->colors > 3)
		fprintf (ofp,
		"P7\nWIDTH %d\nHEIGHT %d\nDEPTH %d\nMAXVAL %d\nTUPLTYPE %s\nENDHDR\n",
		p->width, p->height, p->colors, (1 << p->opt.output_bps)-1, p->cdesc);
	else
		fprintf (ofp, "P%d\n%d %d\n%d\n",
		p->colors/2+5, p->width, p->height, (1 << p->opt.output_bps)-1);

	if (p->opt.output_bps == 8) dcr_gamma_lut (p, lut);
	soff  = dcr_flip_index (p, 0, 0);
	cstep = dcr_flip_index (p, 0, 1) - soff;
	rstep = dcr_flip_index (p, 1, 0) - dcr_flip_index (p, 0, p->width);
	for (row=0; row < p->height; row++, soff += rstep) {
		for (col=0; col < p->width; col++, soff += cstep){
			if (p->opt.output_bps == 8)
				FORCC(p) ppm [col*p->colors+c] = lut[p->image[soff][c]];
			else
				FORCC(p) ppm2[col*p->colors+c] =     p->image[soff][c];
		}
		if (p->opt.output_bps == 16 && !p->opt.output_tiff && htons(0x55aa) != 0x55aa)
			_swab ((char*)ppm2, (char*)ppm2, p->width*p->colors*2);
		fwrite (ppm, p->colors*p->opt.output_bps/8, p->width, ofp);
	}
	free (ppm);
}

///////////////////////////////////////////////////////////////////////////////




static int dcr_sfile_read(dcr_stream_obj *obj, void *buf, int size, int cnt)
{
	return fread(buf, size, cnt, (FILE *)obj);
}

static int dcr_sfile_write(dcr_stream_obj *obj, void *buf, int size, int cnt)
{
	return fwrite(buf, size, cnt, (FILE *)obj);
}

static long dcr_sfile_seek(dcr_stream_obj *obj, long offset, int origin)
{
	return fseek((FILE *)obj, offset, origin);
}

static int dcr_sfile_close(dcr_stream_obj *obj)
{
	return fclose((FILE *)obj);
}

static char* dcr_sfile_gets(dcr_stream_obj *obj, char *string, int n)
{
	return fgets(string,n,(FILE *)obj);
}

static int   dcr_sfile_eof(dcr_stream_obj *obj)
{
	return feof((FILE *)obj);
}

static long  dcr_sfile_tell(dcr_stream_obj *obj)
{
	return ftell((FILE *)obj);
}

static int   dcr_sfile_getc(dcr_stream_obj *obj)
{
	return fgetc((FILE *)obj);
}

static int   dcr_sfile_scanf(dcr_stream_obj *obj,const char *format, void* output)
{
	return fscanf((FILE *)obj, format, output);
}


///////////////////////////////////////////////////////////////////////////////


void DCR_CLASS dcr_init_dcraw(DCRAW* p)
{
	memset(p,0,sizeof(DCRAW));

	p->ops_ = &dcr_stream_fileops;

	p->opt.dark_frame = NULL;
	p->opt.bpfile     = NULL;
	p->opt.user_flip  = -1;
	p->opt.user_black = -1;
	p->opt.user_qual  = -1;
	p->opt.user_sat   = -1;
	p->opt.timestamp_only = 0;
	p->opt.thumbnail_only = 0;
	p->opt.identify_only  = 0;
	p->opt.use_fuji_rotate = 1;
	p->opt.write_to_stdout = 0;
	p->opt.bright = 1;
	p->opt.aber[0] = p->opt.aber[1] = p->opt.aber[2] = p->opt.aber[3] = 1;
	p->opt.output_color = 1;
	p->opt.output_bps =8;
	p->opt.greybox[0] = p->opt.greybox[1] = 0;
	p->opt.greybox[2] = p->opt.greybox[3] = UINT_MAX;
	p->opt.use_camera_matrix = -1;

}

void	DCR_CLASS dcr_cleanup_dcraw(DCRAW* p)
{
	if (p->meta_data) free (p->meta_data);
	if (p->oprof) free (p->oprof);
	if (p->image) free (p->image);
}

void DCR_CLASS dcr_print_manual(int argc, char **argv)
{
	printf(_("\nRaw photo decoder \"dcraw\" v%s"), DCR_VERSION);
    printf(_("\nby Dave Coffin, dcoffin a cybercom o net\n"));
    printf(_("\nUsage:  %s [OPTION]... [FILE]...\n\n"), argv[0]);
    puts(_("-v        Print verbose messages"));
    puts(_("-c        Write image data to standard output"));
    puts(_("-e        Extract embedded thumbnail image"));
    puts(_("-i        Identify files without decoding them"));
    puts(_("-i -v     Identify files and show metadata"));
    puts(_("-z        Change file dates to camera timestamp"));
    puts(_("-w        Use camera white balance, if possible"));
    puts(_("-a        Average the whole image for white balance"));
    puts(_("-A <x y w h> Average a grey box for white balance"));
    puts(_("-r <r g b g> Set custom white balance"));
    puts(_("+M/-M     Use/don't use an embedded color matrix"));
    puts(_("-C <r b>  Correct chromatic aberration"));
    puts(_("-P <file> Fix the dead pixels listed in this file"));
    puts(_("-K <file> Subtract dark frame (16-bit raw PGM)"));
    puts(_("-k <num>  Set the darkness level"));
    puts(_("-S <num>  Set the saturation level"));
    puts(_("-n <num>  Set threshold for wavelet denoising"));
    puts(_("-H [0-9]  Highlight mode (0=clip, 1=unclip, 2=blend, 3+=rebuild)"));
    puts(_("-t [0-7]  Flip image (0=none, 3=180, 5=90CCW, 6=90CW)"));
    puts(_("-o [0-5]  Output colorspace (raw,sRGB,Adobe,Wide,ProPhoto,XYZ)"));
#ifndef NO_LCMS
    puts(_("-o <file> Apply output ICC profile from file"));
    puts(_("-p <file> Apply camera ICC profile from file or \"embed\""));
#endif
    puts(_("-d        Document mode (no color, no interpolation)"));
    puts(_("-D        Document mode without scaling (totally raw)"));
    puts(_("-j        Don't stretch or rotate raw pixels"));
    puts(_("-W        Don't automatically brighten the image"));
    puts(_("-b <num>  Adjust brightness (default = 1.0)"));
    puts(_("-q [0-3]  Set the interpolation quality"));
    puts(_("-h        Half-size color image (twice as fast as \"-q 0\")"));
    puts(_("-f        Interpolate RGGB as four colors"));
    puts(_("-m <num>  Apply a 3x3 median filter to R-G and B-G"));
    puts(_("-s [0..N-1] Select one raw image or \"all\" from each file"));
    puts(_("-4        Write 16-bit linear instead of 8-bit with gamma"));
    puts(_("-T        Write TIFF instead of PPM"));
    puts("");
}

int DCR_CLASS dcr_parse_command_line_options(DCRAW* p, int argc, char **argv, int *arg)
{
	char opm, opt, *sp, *cp;
	int i, c;

	if (argv && arg) {
		argv[argc] = "";
		for (*arg=1; (((opm = argv[*arg][0]) - 2) | 2) == '+'; ) {

			opt = argv[(*arg)++][1];
			if ((cp = strchr (sp="nbrkStqmHAC", opt)))
				for (i=0;    i < "11411111142"[cp-sp]-'0'; i++)
					if (!isdigit(argv[(*arg)+i][0])) {
						fprintf (stderr,_("Non-numeric argument to \"-%c\"\n"), opt);
						return 1;
					}

			switch (opt) {
			case 'n':  p->opt.threshold   = (float)atof(argv[(*arg)++]);  break;
			case 'b':  p->opt.bright      = (float)atof(argv[(*arg)++]);  break;
			case 'r':  FORC4 p->opt.user_mul[c] = (float)atof(argv[(*arg)++]);  break;
			case 'C':  p->opt.aber[0] = 1 / atof(argv[(*arg)++]);
					   p->opt.aber[2] = 1 / atof(argv[(*arg)++]);  break;
			case 'k':  p->opt.user_black  = atoi(argv[(*arg)++]);  break;
			case 'S':  p->opt.user_sat    = atoi(argv[(*arg)++]);  break;
			case 't':  p->opt.user_flip   = atoi(argv[(*arg)++]);  break;
			case 'q':  p->opt.user_qual   = atoi(argv[(*arg)++]);  break;
			case 'm':  p->opt.med_passes  = atoi(argv[(*arg)++]);  break;
			case 'H':  p->opt.highlight   = atoi(argv[(*arg)++]);  break;
			case 's':
					   p->opt.shot_select = abs(atoi(argv[*arg]));
					   p->opt.multi_out = !strcmp(argv[(*arg)++],"all");
					   break;
			case 'o':
					   if (isdigit(argv[*arg][0]) && !argv[*arg][1])
						   p->opt.output_color = atoi(argv[(*arg)++]);
#ifndef NO_LCMS
						else p->opt.out_profile = argv[(*arg)++];
						break;
			case 'p':  p->opt.cam_profile = argv[(*arg)++];
#endif
					   break;
			case 'P':  p->opt.bpfile     = argv[(*arg)++];  break;
			case 'K':  p->opt.dark_frame = argv[(*arg)++];  break;
			case 'z':  p->opt.timestamp_only    = 1;  break;
			case 'e':  p->opt.thumbnail_only    = 1;  break;
			case 'i':  p->opt.identify_only     = 1;  break;
			case 'c':  p->opt.write_to_stdout   = 1;  break;
			case 'v':  p->opt.verbose           = 1;  break;
			case 'h':  p->opt.half_size         = 1;		/* "-h" implies "-f" */
			case 'f':  p->opt.four_color_rgb    = 1;  break;
			case 'A':  FORC4 p->opt.greybox[c]  = atoi(argv[(*arg)++]);
			case 'a':  p->opt.use_auto_wb       = 1;  break;
			case 'w':  p->opt.use_camera_wb     = 1;  break;
			case 'M':  p->opt.use_camera_matrix = (opm == '+');  break;
			case 'D':
			case 'd':  p->opt.document_mode = 1 + (opt == 'D');
			case 'j':  p->opt.use_fuji_rotate   = 0;  break;
			case 'W':  p->opt.no_auto_bright    = 1;  break;
			case 'T':  p->opt.output_tiff       = 1;  break;
			case '4':  p->opt.output_bps       = 16;  break;
			default:
				fprintf (stderr,_("Unknown option \"-%c\".\n"), opt);
				return 1;
			}
		}
	}

	if (p->opt.use_camera_matrix < 0)
		p->opt.use_camera_matrix = p->opt.use_camera_wb;

	return 0;
}
