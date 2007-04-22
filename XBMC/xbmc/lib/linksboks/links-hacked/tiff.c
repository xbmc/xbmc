/* tiff.c
 * TIFF image decoding
 * (c) 2002 Petr 'Brain' Kulhavy
 * This file is a part of the Links program, released under GPL.
 *
 * Compiles in graphics mode only and only when HAVE_TIFF.
 */
#include "cfg.h"

#ifdef G
#include "links.h"

#ifdef HAVE_TIFF
#include <tiffio.h>
#include "bits.h"

void tiff_start(struct cached_image *cimg)
{
	struct tiff_decoder * deco;

	deco=mem_alloc(sizeof(struct tiff_decoder));
	if (!deco){img_end(cimg);return;}

	cimg->decoder=deco;
	deco->tiff_size=0;
	deco->tiff_data=NULL;
	deco->tiff_open=0;
	deco->tiff_pos=0;
}


void tiff_restart(struct cached_image *cimg, unsigned char *data, int length)
{
	struct tiff_decoder * deco=(struct tiff_decoder*)cimg->decoder;
	unsigned char *p;

	if (!deco->tiff_data)p=mem_alloc(length);
	else p=mem_realloc(deco->tiff_data,deco->tiff_size+length);
	if (!p){img_end(cimg);return;}
	deco->tiff_data=p;
	memcpy(deco->tiff_data+deco->tiff_size,data,length);
	deco->tiff_size+=length;
}

static toff_t __tiff_size(thandle_t data)
{
	struct cached_image *cimg=(struct cached_image *)data;
	struct tiff_decoder *deco=(struct tiff_decoder*)cimg->decoder;

	if (!deco->tiff_open)internal("BUG IN LIBTIFF: sizeproc called on closed file. Contact the libtiff authors.\n");
	
	return deco->tiff_size;
}


static tsize_t __tiff_read(thandle_t data, tdata_t dest, tsize_t count)
{
	struct cached_image *cimg=(struct cached_image *)data;
	struct tiff_decoder *deco=(struct tiff_decoder*)cimg->decoder;
	int n;
	
	if (!deco->tiff_open)internal("BUG IN LIBTIFF: readproc called on closed file. Contact the libtiff authors.\n");

	n=(deco->tiff_pos+count>deco->tiff_size)?deco->tiff_size-deco->tiff_pos:count;
	memcpy(dest,deco->tiff_data+deco->tiff_pos,n);
	deco->tiff_pos+=n;
	return n;
}


static tsize_t __tiff_write(thandle_t data, tdata_t dest, tsize_t count)
{
	internal("BUG IN LIBTIFF: writeproc called on read-only file. Contact the libtiff authors.\n");
	return 0;
}


static toff_t __tiff_seek(thandle_t data, toff_t offset, int whence)
{
	struct cached_image *cimg=(struct cached_image *)data;
	struct tiff_decoder *deco=(struct tiff_decoder*)cimg->decoder;
	
	if (!deco->tiff_open)internal("BUG IN LIBTIFF: seekproc called on closed file. Contact the libtiff authors.\n");

	switch(whence)
	{
		case SEEK_SET:
		deco->tiff_pos=(offset>deco->tiff_size)?deco->tiff_size:offset;
		break;
		case SEEK_CUR:
		deco->tiff_pos+=(deco->tiff_pos+offset>deco->tiff_size)?deco->tiff_size-deco->tiff_pos:offset;
		break;
		case SEEK_END:
		deco->tiff_pos=(offset>deco->tiff_size)?0:deco->tiff_size-offset;
		break;
	}
	return deco->tiff_pos;
}


static int __tiff_close(void *data)
{
	struct cached_image *cimg=(struct cached_image *)data;
	struct tiff_decoder *deco=(struct tiff_decoder*)cimg->decoder;
	
	if (!deco->tiff_open)internal("BUG IN LIBTIFF: closeproc called on closed file. Contact the libtiff authors.\n");
	if (deco->tiff_data)mem_free(deco->tiff_data),deco->tiff_data=NULL;
	deco->tiff_open=0;
	return 0;
}


static int __tiff_mmap(thandle_t data, tdata_t *dest, toff_t *len)
{
	struct cached_image *cimg=(struct cached_image *)data;
	struct tiff_decoder *deco=(struct tiff_decoder*)cimg->decoder;
	
	if (!deco->tiff_open)internal("BUG IN LIBTIFF: mapproc called on closed file. Contact the libtiff authors.\n");
	*dest=deco->tiff_data;
	*len=deco->tiff_size;
	return 0;
}


static void __tiff_munmap(thandle_t data, tdata_t dest, toff_t len)
{
	struct cached_image *cimg=(struct cached_image *)data;
	struct tiff_decoder *deco=(struct tiff_decoder*)cimg->decoder;
	
	if (!deco->tiff_open)internal("BUG IN LIBTIFF: unmapproc called on closed file. Contact the libtiff authors.\n");
}


static void __tiff_error_handler(const char* module, const char* fmt, va_list ap)
{
}

static void flip_buffer(void *buf,int width,int height)
{
	if (htonl(0x12345678L)!=0x12345678L)	/* little endian --- ja to chci na intelu rychly!!! */
	{
#ifdef t4c
		t4c* buffer=(t4c*)buf;
		register t4c a,b;
		t4c *p,*q;
		int i,l;
		
		for (l=0,p=buffer,q=buffer+width*(height-1);l<(height>>1);l++,q-=(width<<1))
			for (i=0;i<width;a=*p,b=*q,*p++=b,*q++=a,i++);
#else
		unsigned char* buffer=(unsigned char*)buf;
		unsigned char *p,*q;
		int l;
		unsigned char *tmp;
		int w=4*width;
		
		tmp=mem_alloc(w*sizeof(unsigned char));
		if (!tmp)internal("Cannot allocate memory.\n");
		
		/* tohle je pomalejsi, protoze se kopiruje pamet->pamet, pamet->pamet */
		/* kdyz mame 4B typek, tak se kopiruje pamet->reg, reg->pamet */
		for (l=0,p=buffer,q=buffer+w*(height-1);l<(height>>1);l++,q-=w,p+=w)
			memcpy(tmp,p,w),memcpy(p,q,w),memcpy(q,tmp,w);
		mem_free(tmp);
#endif
	}
	else	/* big endian */
	{
		unsigned char zakazany_uvolneni[4];
		unsigned char* buffer=(unsigned char*)buf;
		int w=width<<2; /* 4 bytes per pixel */
		unsigned char *p,*q;
		int i,l;
		
		for (l=0,p=buffer,q=buffer+w*(height-1);l<(height>>1);l++,q-=(w<<1))
			for (i=0;i<width;i++,p+=4,q+=4)
			{
				memcpy(zakazany_uvolneni,p,4);
				p[0]=q[3];
				p[1]=q[2];
				p[2]=q[1];
				p[3]=q[0];
				q[0]=zakazany_uvolneni[3];
				q[1]=zakazany_uvolneni[2];
				q[2]=zakazany_uvolneni[1];
				q[3]=zakazany_uvolneni[0];
			}
		if (height&1) /* flip endianity of line in the middle (if the height is odd) */
			for (i=0;i<width;i++,p+=4)
			{
				memcpy(zakazany_uvolneni,p,4);
				p[0]=zakazany_uvolneni[3];
				p[1]=zakazany_uvolneni[2];
				p[2]=zakazany_uvolneni[1];
				p[3]=zakazany_uvolneni[0];
			}
	}
}

void tiff_finish(struct cached_image *cimg)
{
	struct tiff_decoder *deco=(struct tiff_decoder*)cimg->decoder;
	int bla;
	TIFF *t;

	if (!deco->tiff_size){img_end(cimg);return;}
	deco->tiff_open=1;
	TIFFSetErrorHandler(__tiff_error_handler);
	TIFFSetWarningHandler(__tiff_error_handler);
	t=TIFFClientOpen(
			"Prave si rek' svy posledni slova. A vybral sis k tomu prihodny misto.",
			"r",
			cimg,
			(TIFFReadWriteProc)__tiff_read,
			(TIFFReadWriteProc)__tiff_write,
			(TIFFSeekProc)__tiff_seek,
			(TIFFCloseProc)__tiff_close,
			(TIFFSizeProc)__tiff_size,
			(TIFFMapFileProc)__tiff_mmap,
			(TIFFUnmapFileProc)__tiff_munmap
		);
	if (!t){img_end(cimg);return;}
	bla=TIFFGetField(t, TIFFTAG_IMAGEWIDTH, &(cimg->width));
	if (!bla){TIFFClose(t);img_end(cimg);return;}
	bla=TIFFGetField(t, TIFFTAG_IMAGELENGTH, &(cimg->height));
	if (!bla){TIFFClose(t);img_end(cimg);return;}
	cimg->buffer_bytes_per_pixel=4;
	cimg->red_gamma=cimg->green_gamma=cimg->blue_gamma=sRGB_gamma;
	cimg->strip_optimized=0;
	header_dimensions_known(cimg);
	TIFFReadRGBAImage(t,cimg->width,cimg->height,(unsigned long*)(cimg->buffer),1);
	TIFFClose(t);

	/* For some reason the TIFFReadRGBAImage() function chooses the lower
	 * left corner as the origin.  Vertically mirror scanlines. 
	 */
	flip_buffer((void*)(cimg->buffer),cimg->width,cimg->height);
	
	img_end(cimg);
}
#endif /* #ifdef HAVE_TIFF */

#endif /* #ifdef G */
