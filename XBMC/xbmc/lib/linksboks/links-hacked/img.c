/* img.c
 * Generic image decoding and PNG and JPG decoders.
 * (c) 2002 Karel 'Clock' Kulhavy
 * This is a part of the Links program, released under GPL.
 
 * Used in graphics mode of Links only
 TODO: odstranit zbytecne ditherovani z strip_optimized header_dimensions_known,
       protoze pozadi obrazku musi byt stejne jako pozadi stranky, a to se nikdy
       neditheruje, protoze je to vzdy jednolita barva. Kdyz uz to nepujde
       odstranit tak tam aspon dat fixne zaokrouhlovani.
 TODO: pouzit get_filled_bitmap az bude napsany k optimalizaci zadavani
       grafickych dat do X serveru z hlediska zaalokovane pameti.
 TODO: dodelat stripy do jpegu a png a tiff.
 */
#include "cfg.h"
#include "links.h"

#ifdef G

#ifdef HAVE_ENDIAN_H
/* Max von Sydow */
#include <endian.h>
#endif

#ifdef HAVE_JPEG
#include "../jpeg-6b/jpeglib.h"
#endif

struct decoded_image {
	int bla;
};

int gamma_stamp; /* stamp counter for gamma changes */

#define RESTART_SIZE 8192
/* Size of biggest chunk of compressed data that is processed in one run */


/* End of decoder structs */

struct g_object_image *global_goi;
struct cached_image *global_cimg;
int end_callback_hit;
#endif /* #ifdef G */
int dither_images;
#ifdef G

/* This is a dummy */
void img_draw_decoded_image(struct graphics_device *dev, struct decoded_image *d, int x, int y, int xw, int yw, int xo, int yo)
{
}

/* This is a dummy */
void img_release_decoded_image(struct decoded_image *d)
{
	mem_free(d);
}

/* mem_free(cimg->decoder) */
void destroy_decoder (struct cached_image *cimg)
{
#ifdef HAVE_JPEG
	struct jpg_decoder *jd;
#endif /* #ifdef HAVE_JPEG */
	struct png_decoder *pd;
#ifdef HAVE_TIFF
	struct tiff_decoder *td;
#endif /* #ifdef HAVE_TIFF */

        if (cimg->decoder){
		switch(cimg->image_type){
		case IM_PNG:
			pd=(struct png_decoder *)cimg->decoder;
			png_destroy_read_struct(
				&pd->png_ptr,
				&pd->info_ptr,
				NULL);
			break;
#ifdef HAVE_JPEG
		case IM_JPG:
			jd=(struct jpg_decoder *)cimg->decoder;

			jpeg_destroy_decompress(jd->cinfo);
			mem_free(jd->cinfo);
			mem_free(jd->jerr);
			if (jd->jdata) mem_free(jd->jdata);
			if (jd->scanlines[0])
				mem_free(jd->scanlines[0]);
			break;
#endif /* #ifdef HAVE_JPEG */
		case IM_GIF:
			gif_destroy_decoder(cimg);
			break;
		case IM_XBM:
			/* do nothing */
			break;
#ifdef HAVE_TIFF
		case IM_TIFF:
			td=(struct tiff_decoder *)cimg->decoder;
			if (td->tiff_open)
			{
				if (td->tiff_data)mem_free(td->tiff_data);
				td->tiff_open=0;
			}
			break;
#endif
		}
		mem_free(cimg->decoder);
                cimg->decoder = NULL;
        }
}

void img_destruct_image(struct g_object *object)
{
	struct g_object_image *goi=(struct g_object_image *)object;
	
	if (goi->orig_src)mem_free(goi->orig_src);
	if (goi->alt)mem_free(goi->alt);
	if (goi->name)mem_free(goi->name);
	if (goi->src)mem_free(goi->src);
	release_image_map(goi->map);
	if (goi->image_list.next)del_from_list(&goi->image_list);
	if (goi->xw&&goi->yw){
                goi->cimg->refcount--;
	}
	mem_free(goi);
}

/* Frees all data allocated by cached_image including cached image itself */
void img_destruct_cached_image(struct cached_image *cimg)
{
	switch (cimg->state){
		case 0:
		case 1:
		case 2:
		case 3:
		case 9:
		case 11:
		break;
		
		case 12:
		case 14:
		if (cimg->gamma_table) mem_free(cimg->gamma_table);
		if (cimg->bmp.user){
			drv->unregister_bitmap(&(cimg->bmp));
		}
		if (cimg->strip_optimized){
			if (cimg->dregs) mem_free(cimg->dregs);
		}else{
			mem_free(cimg->buffer);
		}
		case 8:
		case 10:
		destroy_decoder(cimg);
		break;

		case 13:
		case 15:
		drv->unregister_bitmap(&(cimg->bmp));
		break;
		
#ifdef DEBUG
		default:
		fprintf(stderr,"img_destruct_cached_image: state=%d\n",cimg->state);
		internal("Invalid state in struct cached_image");
#endif /* #ifdef DEBUG */
	}
	mem_free(cimg->url);
	mem_free(cimg);
}

/* You thorow in a vertical dimension of image and it returns
 * new dimension according to the aspect ratio and user-set image
 * scaling factor. When scaling factor is 100% and screen pixels
 * are non-square, the pixture will be always in one dimension
 * untouched and in the second _ENLARGED_. So that no information
 * in the picture will be lost.
 * Input may be <0. In this case output=input
 * Input may be 0. In this case output=0.
 * If input is >0 the output is also >0.
 */
int img_scale_h(unsigned scale, int in){
	int out;
	/* We assume unsigned long holds at least 32 bits */
	unsigned long pre;

	if (in<=0) return in;
	pre=((unsigned long)(aspect<65536UL?65536UL:aspect)*scale+128)>>8;
	out=((unsigned long)in*pre+12800UL)/25600UL;
	if (out<1) out=1;
	return out;
}

int img_scale_v(unsigned scale, int in){
	int out;
	unsigned long divisor;

	if (in<=0) return in;
	divisor=(100*(aspect>=65536UL?65536UL:aspect)+128)>>8;
	out=((unsigned long)in*(scale*256)+(divisor>>1))/divisor;
	if (out<1) out=1;
	return out;
}

/* Compute 8-bit background for filling buffer with cimg->*_gamma
 * (performs rounding) */
void compute_background_8(unsigned char *rgb, struct cached_image *cimg)
{
	unsigned short red, green, blue;

	round_color_sRGB_to_48(&red, &green, &blue
		, cimg->background_color);
	rgb[0]=apply_gamma_single_16_to_8(red
		,cimg->red_gamma/user_gamma);
	rgb[1]=apply_gamma_single_16_to_8(green
		,cimg->green_gamma/user_gamma);
	rgb[2]=apply_gamma_single_16_to_8(blue
		,cimg->blue_gamma/user_gamma);
}

/* updates cimg state when header dimensions are know. Only allowed to be called
 * in state 8 and 10.
 * Allocates right amount of memory into buffer, formats it (with background or
 * zeroes, depens on buffer_bytes_per_pixel). Updates dimensions (xww and yww)
 * according to newly known header dimensions. Fills in gamma_stamp, bmp.user
 * (NULL because we not bother with generating bitmap here)
 * and rows_added.
 * Resets strip_optimized if image will be scaled or 
 * Allocates dregs if on exit strip_optimized is nonzero.
 * Allocates and computes gamma_table, otherwise
 * 	sets gamma_table to NULL. Also doesn't make gamma table if image contains less
 * 	than 1024 pixels (it would be probably a waste of time).
 * Output state is always 12 (from input state 8) or 14 (from input state 10).
 *
 * The caller must have set the following elements of cimg:
 *	width
 *	height
 *	buffer_bytes_per_pixel
 *	red_gamma
 *	green_gamma
 *	blue_gamma
 *	strip_optimized
 */
void header_dimensions_known(struct cached_image *cimg)
{
	unsigned short red, green, blue;

#ifdef DEBUG
	if ((cimg->state^8)&13){
		fprintf(stderr,"cimg->state=%d\n",cimg->state);
		internal("Invalid state in header_dimensions_known");
	}
	if (cimg->width<1||cimg->height<1){
		fprintf(stderr,"width=%d height=%d\n",cimg->width, cimg->height);
		internal("Zero dimensions in header_dimensions_known");
	}
#endif /* #ifdef DEBUG */
	if (cimg->wanted_xw<0){
		/* Unspecified width */
		if (cimg->wanted_yw<0){
			/* Unspecified width and height */
			cimg->xww=img_scale_h(cimg->scale, cimg->width);
			cimg->yww=img_scale_v(cimg->scale, cimg->height);
		}else{
			/* Unspecified width specified height */
			cimg->xww=(cimg->yww
				*cimg->width+(cimg->height>>1))
				/cimg->height;
			if (cimg->xww<=0) cimg->xww=1;
			
		}
	}else{
		if (cimg->wanted_yw<0){
			/* Specified width unspecified height */
			cimg->yww=(cimg->xww
				*cimg->height+(cimg->width>>1))
				/cimg->width;
			if (cimg->yww<=0) cimg->yww=1;
		}
	}
	if (cimg->width!=cimg->xww||cimg->height!=cimg->yww) cimg->strip_optimized=0;
	cimg->gamma_stamp=gamma_stamp;
	if (cimg->strip_optimized){
		struct bitmap tmpbmp;
		unsigned short *buf_16;
		int i;

		tmpbmp.x=cimg->width;
		tmpbmp.y=1;
		/* No buffer, bitmap is valid from the very beginning */
		cimg->bmp.x=cimg->width;
		cimg->bmp.y=cimg->height;
		drv->get_empty_bitmap(&(cimg->bmp));
		buf_16=mem_alloc(sizeof(*buf_16)*3*cimg->width);
		round_color_sRGB_to_48(&red, &green, &blue
			, cimg->background_color);
		mix_one_color_48(buf_16,cimg->width, red, green, blue);
#ifdef DEBUG
		if (cimg->height<=0){
			fprintf(stderr,"cimg->height=%d\n",cimg->height);
			internal("Invalid cimg->height in strip_optimized section of\
 header_dimensions_known");
		}
#endif /* #ifdef DEBUG */
		/* The skip is uninitialized here and is read by dither_start
		 * but is not used in any malicious way so it doesn't matter
		 */
		tmpbmp.data=cimg->bmp.data;
		cimg->dregs=dither_images?dither_start(buf_16,&tmpbmp):NULL;
		tmpbmp.data=(unsigned char *)tmpbmp.data+cimg->bmp.skip;
		if (cimg->dregs)
			for (i=cimg->height-1;i;i--){
				dither_restart(buf_16,&tmpbmp,cimg->dregs);
				tmpbmp.data=(unsigned char *)tmpbmp.data+cimg->bmp.skip;
			}
		else
			for (i=cimg->height-1;i;i--){
				(*round_fn)(buf_16,&tmpbmp);
				tmpbmp.data=(unsigned char *)tmpbmp.data+cimg->bmp.skip;
			}
		mem_free(buf_16);
		drv->register_bitmap(&(cimg->bmp));
		if(cimg->dregs) memset(cimg->dregs,0,cimg->width*sizeof(*cimg->dregs)*3);
		cimg->bmp.user=(void *)&end_callback_hit; /* Nonzero value */
		/* This ensures the dregs are none and because strip
		 * optimization is unusable in interlaced pictures,
		 * this saves the zeroing out at the beginning of the
		 * decoder itself.
		 */
	}else {
		cimg->rows_added=1;
		cimg->bmp.user=NULL;
		cimg->buffer=mem_alloc(cimg->width*cimg->height
			*cimg->buffer_bytes_per_pixel);
		if (cimg->buffer_bytes_per_pixel==4
			 	||cimg->buffer_bytes_per_pixel==4
				*sizeof(unsigned short))
			{	
			/* Make the buffer contain full transparency */
			memset(cimg->buffer,0,cimg->width*cimg->height
				*cimg->buffer_bytes_per_pixel);
		}else{
			/* Fill the buffer with background color */
			if (cimg->buffer_bytes_per_pixel>4){
				/* 16-bit */
				unsigned short red, green, blue;

				round_color_sRGB_to_48(&red, &green, &blue
					, cimg->background_color);

				red=apply_gamma_single_16_to_16(red
					,cimg->red_gamma/user_gamma);
				green=apply_gamma_single_16_to_16(green
					,cimg->green_gamma/user_gamma);
				blue=apply_gamma_single_16_to_16(blue
					,cimg->blue_gamma / user_gamma);
				mix_one_color_48((unsigned short *)cimg->buffer
					,cimg->width*cimg->height,red
					,green, blue);
			}else{
				unsigned char rgb[3];
				
				/* 8-bit */
				compute_background_8(rgb,cimg);
				mix_one_color_24(cimg->buffer
					,cimg->width*cimg->height
					,rgb[0],rgb[1],rgb[2]);
			}
		}
	}
	if (cimg->buffer_bytes_per_pixel<=4&&cimg->width*cimg->height>=1024){
		make_gamma_table(cimg);
	}else if (cimg->buffer_bytes_per_pixel>=6&&cimg->width*cimg->height>=262144){
		make_gamma_table(cimg);
	}else cimg->gamma_table=NULL;
	cimg->state|=4; /* Update state */
}

/* Fills "tmp" buffer with the resulting data and does not free the input
 * buffer. May be called only in states 12 and 14 of cimg
 */
unsigned short *buffer_to_16(unsigned short *tmp, struct cached_image *cimg
	,unsigned char *buffer, int height)
{
	unsigned short red, green,blue;

#ifdef DEBUG
	if (cimg->state!=12&&cimg->state!=14){
		fprintf(stderr,"cimg->state=%d\n",cimg->state);
		internal("invalid state in buffer_to_16");
	}
#endif /* #ifdef DEBUG */
	switch (cimg->buffer_bytes_per_pixel){
		case 3:
			if (cimg->gamma_table){
				apply_gamma_exponent_24_to_48_table(tmp, buffer,
					cimg->width*height
					,cimg->gamma_table);
			}
			else{
				apply_gamma_exponent_24_to_48(tmp,buffer,cimg->width
					*height
					,user_gamma/cimg->red_gamma
					,user_gamma/cimg->green_gamma
					,user_gamma/cimg->blue_gamma);
			}
		break;

		case 3*sizeof(unsigned short):
			if (cimg->gamma_table){
				apply_gamma_exponent_48_to_48_table(tmp
					,(unsigned short *)buffer
					,cimg->width*height, cimg->gamma_table);
			}else{
				apply_gamma_exponent_48_to_48(tmp,(unsigned short *)buffer
					,cimg->width*height,
					user_gamma/cimg->red_gamma,
					user_gamma/cimg->green_gamma,
					user_gamma/cimg->blue_gamma);
			}
		break;

		/* Alpha's: */
		case 4: 
		{

			round_color_sRGB_to_48(&red,&green,&blue,cimg->background_color);
			if (cimg->gamma_table){
				apply_gamma_exponent_and_undercolor_32_to_48_table(
						tmp, buffer, cimg->width *height,
						cimg->gamma_table, red, green, blue);
			}else{
				
				apply_gamma_exponent_and_undercolor_32_to_48(tmp,buffer
					,cimg->width*height,
					user_gamma/cimg->red_gamma,
					user_gamma/cimg->green_gamma,
					user_gamma/cimg->blue_gamma,
					red, green, blue);
			}
		}
		break;

		case 4*sizeof(unsigned short):
		{
			round_color_sRGB_to_48(&red, &green, &blue,
				cimg->background_color);
			if (cimg->gamma_table){
				apply_gamma_exponent_and_undercolor_64_to_48_table
					(tmp, (unsigned short *)buffer, cimg->width*height
					,cimg->gamma_table, red, green, blue);
			}else{
				apply_gamma_exponent_and_undercolor_64_to_48(tmp
					,(unsigned short*)buffer,cimg->width*height,
					user_gamma/cimg->red_gamma,
					user_gamma/cimg->green_gamma,
					user_gamma/cimg->blue_gamma,
					red,green,blue);
			}
		}
		break;
		
#ifdef DEBUG
		default:
		internal("buffer_to_16: unknown mem organization");
#endif /* #ifdef DEBUG */

	}
	return tmp;
}

/* Returns allocated buffer with the resulting data and does not free the input
 * buffer. May be called only in states 12 and 14 of cimg
 * use_strip: 1 if the image is already registered and prepare_strip and
 * commit_strip is to be used
 * 0: if the image is not yet registered and instead one big register_bitmap
 * will be used eventually
 * dregs must be externally allocated and contain required value or must be
 * NULL.
 * if !dregs then rounding is performed instead of dithering.
 * dregs are not freed. 
 * bottom dregs are placed back into dregs.
 * Before return the bitmap will be in registered state and changes will be
 * commited.
 * height must be >=1 !!!
 */
void buffer_to_bitmap_incremental(struct cached_image *cimg
	,unsigned char *buffer, int height, int yoff, int *dregs, int use_strip)
{
#define max_height 16
/* max_height must be at least 1 */
	unsigned short *tmp;
	struct bitmap tmpbmp;
	int add1=0, add2;

#ifdef DEBUG
	if (cimg->state!=12&&cimg->state!=14){
		fprintf(stderr,"cimg->state=%d\n",cimg->state);
		internal("Invalid state in buffer_to_bitmap_incremental\n");
	}
	if (height<1){
		fprintf(stderr,"height=%d\n",height);
		internal("Invalid height in buffer_to_bitmap_incremental\n");
	}
	if (cimg->width<1||cimg->height<1){
		fprintf(stderr,"cimg->width=%d, cimg->height=%d\n",cimg->width,
				cimg->height);
		internal("Invalid cimg->width x cimg->height in\
buffer_to_bitmap_incremental");
	}
#endif /* #ifdef DEBUG */
	tmp=mem_alloc(cimg->width*(height<max_height?height:max_height)*3*sizeof(*tmp));
	/* Prepare a fake bitmap for dithering */
	tmpbmp.x=cimg->width;
	if (!use_strip){
	       tmpbmp.data=(unsigned char *)cimg->bmp.data+cimg->bmp.skip*yoff;
	       add1=cimg->bmp.skip*max_height;
	}
	add2=cimg->buffer_bytes_per_pixel*cimg->width*max_height;
not_enough:
	tmpbmp.y=height<max_height?height:max_height;
	if (use_strip) tmpbmp.data=drv->prepare_strip(&(cimg->bmp),yoff,tmpbmp.y);
	tmpbmp.skip=cimg->bmp.skip;
	buffer_to_16(tmp, cimg, buffer, tmpbmp.y);
	if (dregs){
	       	dither_restart(tmp, &tmpbmp, dregs);
	}
	else {
		
		(*round_fn)(tmp, &tmpbmp);
	}
	if (use_strip) drv->commit_strip(&(cimg->bmp),yoff,tmpbmp.y);
	height-=tmpbmp.y;
	if (!height) goto end;
	buffer+=add2;
	yoff+=tmpbmp.y;
	tmpbmp.data=(unsigned char *)tmpbmp.data+add1; 
	/* This has no effect if use_strip but it's faster
	 * to add to bogus value than to play with
	 * conditional jumps.
	 */
	goto not_enough;
end:
	mem_free(tmp);
	if (!use_strip) drv->register_bitmap(&(cimg->bmp));
}

/* Takes the buffer and resamples the data into the bitmap. Automatically
 * destroys the previous bitmap. Must be called only when cimg->buffer is valid.
 * Sets bmp->user to non-NULL
 * If gamma_table is used, it must be still allocated here (take care if you
 * want to destroy gamma table and call buffer_to_bitmap, first call buffer_to_bitmap
 * and then destroy gamma_table).
 */
void buffer_to_bitmap(struct cached_image *cimg)
{
	unsigned short *tmp, *tmp1;
	int ix, iy, ox, oy, gonna_be_smart;
	int *dregs;

#ifdef DEBUG
	if(cimg->state!=12&&cimg->state!=14){
		fprintf(stderr,"cimg->state=%d\n",cimg->state);
		internal("buffer_to_bitmap called in invalid state");
	}
	if (cimg->strip_optimized) internal("strip_optimized in buffer_to_bitmap");
	if (cimg->width<1||cimg->height<1){
		fprintf(stderr,"cimg->width=%d, cimg->height=%d\n",cimg->width,
				cimg->height);
		internal("Invalid cimg->width x cimg->height in\
buffer_to_bitmap");
	}
#endif /* #ifdef DEBUG */


	if (!cimg->rows_added) return;

	/* Here of course width and height must be already filled */
	cimg->rows_added=0;
	ix=cimg->width;
	iy=cimg->height;
	ox=cimg->xww;
	oy=cimg->yww;
	if (ix==ox&&iy==oy) gonna_be_smart=1;
	else{
		gonna_be_smart=0;
		tmp=mem_alloc(ix*iy*3*sizeof(*tmp));
		buffer_to_16(tmp,cimg,cimg->buffer,iy);
		if (!cimg->decoder){
			mem_free(cimg->buffer);
			cimg->buffer=NULL;
		}
	
		/* Scale the image to said size */
#ifdef DEBUG
		if (ox<=0||oy<=0){
			internal("ox or oy <=0 before resampling image");
		}
#endif /* #ifdef DEBUG */
		if (ix!=ox||iy!=oy){
			/* We must really scale */
			tmp1=tmp;
			scale_color(tmp1,ix,iy,&tmp,ox,oy);
		}
	}
	if (cimg->bmp.user) drv->unregister_bitmap(&cimg->bmp);
	cimg->bmp.x=ox;
	cimg->bmp.y=oy;
	drv->get_empty_bitmap(&(cimg->bmp));
	if (gonna_be_smart){
		dregs=dither_images?mem_calloc(sizeof(*dregs)*3*cimg->width):NULL;
		buffer_to_bitmap_incremental(cimg, cimg->buffer, cimg->height,
			0, dregs, 0);
		if (dregs) mem_free(dregs);
	}else{
		if (dither_images)
			dither(tmp,&(cimg->bmp));
		else
			(*round_fn)(tmp,&(cimg->bmp));
		mem_free(tmp);
		drv->register_bitmap(&(cimg->bmp));
	}
	cimg->bmp.user=(void *)&end_callback_hit;
	/* Indicate that the bitmap is valid. The value is just any
	   nonzero value */
	cimg->rows_added=0;
	/* Indicate the bitmap is up-to-date */
}

/* Performs state transition for end of stream or error in image or
 * end of image */
void img_end(struct cached_image *cimg)
{
	switch(cimg->state){
		case 12:
		case 14:
		if (cimg->strip_optimized){
		       if (cimg->dregs)	mem_free(cimg->dregs);
		}
		else{
			buffer_to_bitmap(cimg);
			mem_free(cimg->buffer);
		}
		if (cimg->gamma_table) mem_free(cimg->gamma_table);
		case 8:
		case 10:
		destroy_decoder(cimg);
		case 0:
		case 1:
		case 2:	
		case 3:
		case 9:
		case 11:
		case 13:
		case 15:
		break;	
#ifdef DEBUG
		default:
		fprintf(stderr,"state=%d\n",cimg->state);
		internal("Invalid state encountered in end");
#endif /* #ifdef DEBUG */
	}
	cimg->state|=1;
}

void r3l0ad(struct cached_image *cimg, struct g_object_image *goi)
{
	cimg->last_count2=goi->af->rq->ce->count2;
	cimg->gamma_stamp=gamma_stamp;
	switch(cimg->state){
		case 8:
		case 10:
		destroy_decoder(cimg);
		case 1:
		case 3:
		case 9:
		case 11:
		case 0:
		case 2:
		break;

		case 12:
		if (cimg->gamma_table) mem_free(cimg->gamma_table);
		destroy_decoder(cimg);
		if (cimg->strip_optimized){
			if (cimg->dregs) mem_free(cimg->dregs);
		}else{
			mem_free(cimg->buffer);
		}
		if (cimg->bmp.user){
			case 13:
			drv->unregister_bitmap(&cimg->bmp);
		}
                cimg->xww=img_scale_h(cimg->scale, cimg->wanted_xw<0?32:cimg->wanted_xw);
                cimg->yww=img_scale_v(cimg->scale, cimg->wanted_yw<0?32:cimg->wanted_yw);
		break;

		case 14:
		if (cimg->gamma_table) mem_free(cimg->gamma_table);
		destroy_decoder(cimg);
		if (cimg->strip_optimized){
			if (cimg->dregs) mem_free(cimg->dregs);
		}else{
			mem_free(cimg->buffer);
		}
		if (cimg->bmp.user){
			case 15:
			drv->unregister_bitmap(&cimg->bmp);
		}
		break;

#ifdef DEBUG
		default:
		fprintf(stderr,"cimg->state=%d\n",cimg->state);
		internal("Invalid state in r3l0ad()");
#endif /* #ifdef DEBUG */
	}
	cimg->state&=2;
}

/* Returns 1 if match. If returns 1 then test is mem_free'd.
 * If doesn't return 1 then returns 0
 * dtest - Destructive TEST
 */
static inline int dtest(unsigned char *template, unsigned char *test)
{
	if (strcmp(template,test)) return 0;
	else{
		mem_free(test);
		return 1;
	}
}

/* content_type will be mem_free'd before return from this function.
 * This may be called only in state 0 or 2 */
void type(struct cached_image *cimg, unsigned char *content_type)
{
#ifdef DEBUG
	if (cimg->state!=0&&cimg->state!=2){
		fprintf(stderr,"cimg->state=%d\n",cimg->state);
		internal("Invalid state encountered in type()");
	}
#endif /* #ifdef DEBUG */
#ifdef HAVE_JPEG
	if (dtest("image/jpeg",content_type)){
		cimg->image_type=IM_JPG;
		jpeg_start(cimg);
	}else if (dtest("image/jpg",content_type)){
		cimg->image_type=IM_JPG;
		jpeg_start(cimg);
	}else if (dtest("image/jpe",content_type)){
		cimg->image_type=IM_JPG;
		jpeg_start(cimg);
	}else if (dtest("image/pjpe",content_type)){
		cimg->image_type=IM_JPG;
		jpeg_start(cimg);
	}else if (dtest("image/pjpeg",content_type)){
		cimg->image_type=IM_JPG;
		jpeg_start(cimg);
	}else if (dtest("image/pjpg",content_type)){
		cimg->image_type=IM_JPG;
		jpeg_start(cimg);
	}else 
#endif /* #ifdef HAVE_JPEG */
		if (dtest("image/png",content_type)){
		cimg->image_type=IM_PNG;
		png_start(cimg);
	}else if (dtest("image/gif",content_type)){
		cimg->image_type=IM_GIF;
		gif_start(cimg);
	}else if (dtest("image/x-xbitmap",content_type)){
		cimg->image_type=IM_XBM;
		xbm_start(cimg);
#ifdef HAVE_TIFF
	}else if (dtest("image/tiff",content_type)){
		cimg->image_type=IM_TIFF;
		tiff_start(cimg);
	}else if (dtest("image/tif",content_type)){
		cimg->image_type=IM_TIFF;
		tiff_start(cimg);
#endif /* #ifdef HAVE_TIFF */
	}else{
		/* Error */
		mem_free(content_type);
		img_end(cimg);
		return;
	}
	cimg->state|=8; /* Advance the state according to the table in
			   links-doc.html */
	cimg->last_length=0;
}

/* Doesn't print anything. Downloads more data if available.
 * Sets up cimg->reparse and cimg->xww and cimg->yww accordingly to
 * the state of the decoder. When changing xww and yww also changes xw and yw
 * in g_object_image.
 *      return value 1 means the data were chopped and the caller shall not redraw
 *      	(because it would be too slow and because we are probably choked
 *      	up with the data)
 */
int img_process_download(struct g_object_image *goi, struct f_data_c *fdatac)
{
	unsigned char *data, *ctype;
	int length;
	struct cached_image *cimg = goi->cimg;
	int chopped=0;

#ifdef DEBUG
	if (!goi->af) internal("NULL goi->af in process_download\n");
	if (cimg->state>=16){ /* Negative don't occur becaus it's unsigned char */
		fprintf(stderr,"cimg->state=%d\n",cimg->state);
		internal("Invalid cimg->state in img_process_download\n");
	}
#endif /* #ifdef DEBUG */
	if (!goi->af->rq) return 0;
	if (!goi->af->rq->ce) goto end;
	if (goi->af->rq->ce->count2!=cimg->last_count2||
		(cimg->state>=12&&gamma_stamp!=cimg->gamma_stamp)){
		/* Reload */
		r3l0ad(cimg,goi);
	}
	/*if (!goi->af->rq->ce->head) goto end;*/ /* Mikulas: head muze byt NULL*/ /* Mikulas: tak se to zpracuje a nebude se skakat na konec, kdyz je to NULL */

	if (cimg->state==0||cimg->state==2){
		/* Type still unknown */
		ctype=get_content_type(goi->af->rq->ce->head,
			goi->af->rq->url);
#ifdef DEBUG
		if (!ctype)
			internal("NULL ctype in process_download()");
#endif /* #ifdef DEBUG */
		type(cimg,ctype);
	}
		
	/* Now, if we are in state where decoder is running (8, 10, 12, 14), we may feed
	 * some data into it.
	 */

	if (!((cimg->state^8)&9)){
		if (goi->af->rq->state==O_LOADING
			||goi->af->rq->state==O_OK
			||goi->af->rq->state==O_INCOMPLETE){
			/* Cache is valid */
			defrag_entry(goi->af->rq->ce);
			/* Now it is defragmented and we can suck on the resulting
			 * single fragment
			 */
		
		}else goto end;
		if ((goi->af->rq->ce->frag.next)==&(goi->af->rq->ce->frag)) goto end;
			/* No fragments */
		data=(*(struct fragment *)(goi->af->rq->ce->frag.next)).data;
		length=(*(struct fragment *)(goi->af->rq->ce->frag.next)).length;
		if (length==cimg->last_length) goto end; /* No new data */

		data+=cimg->last_length;
		length-=cimg->last_length;
		if (length>RESTART_SIZE){
			length=RESTART_SIZE;
			chopped=1;
			if (fdatac){
				refresh_image(fdatac,(struct g_object *)goi,0);
			}
		}
		/* Decoder has been already started */
		switch(cimg->image_type){
		case IM_PNG:
			png_restart(cimg,data,length);
			break;
#ifdef HAVE_JPEG
		case IM_JPG:
			jpeg_restart(cimg,data,length);
			break;
#endif /* #ifdef HAVE_JPEG */
		case IM_XBM:
			xbm_restart(cimg,data,length);
			break;
		case IM_GIF:
			gif_restart(data,length);
			break;
#ifdef HAVE_TIFF
		case IM_TIFF:
			tiff_restart(cimg,data,length);
			break;
#endif /* #ifdef HAVE_TIFF */
#ifdef DEBUG
		default:
			fprintf(stderr,"cimg->image_type=%d\n",cimg->state);
			internal("Invalid image_type encountered when processing data in\
img_process_download.\n");
#endif /* #ifdef DEBUG */
		}
		cimg->last_length+=length;
	}
	end:
	
	/* Test end */
	if (!chopped){
		/* We must not perform end with chopped because some
		 * unprocessed data still wait for us :)
		 */
		if (goi->af->rq->state==O_FAILED
			||goi->af->rq->state==O_OK
			||goi->af->rq->state==O_INCOMPLETE
			||(goi->af->rq->ce&&goi->af->rq->stat.state<0)){
#ifdef HAVE_TIFF
			if (!((cimg->state^8)&9)&&cimg->image_type==IM_TIFF)
				tiff_finish(cimg);
#endif
			img_end(cimg);
		}
	}
	return chopped;
}

/* Input: rgb (sRGB) triplet (0...255)
 * Returns a color that is very contrasty on that background sRGB color
 */
int get_foreground(int rgb)
{
	int r,g,b;

	r=(rgb>>16)&255;
	g=(rgb>>8)&255;
	b=rgb&255;

	r=r<128?255:0;
	g=g<128?255:0;
	b=b<128?255:0;

	return (r<<16)|(g<<8)|b;
}

void draw_frame_mark (struct graphics_driver *drv, struct 
	graphics_device *dev, int x, int y, int xw, int yw
	, int bg, int fg, int broken)
{
#ifdef DEBUG
	if (xw<1||yw<1) internal("zero dimension in draw_frame_mark");
#endif /* #ifdef DEBUG */
	if (broken == 1){
		/* Draw between ( 0 and 1/4 ) and ( 3/4 and 1 ) of each
		 * side (0-1)
		 */
		 int xl, xh, yl, yh;
		 
		 xh=xw-(xl=xw>>2);
		 yh=yw-(yl=yw>>2);
		/* Draw full sides and the box inside */
		drv->draw_hline(dev,x,y,x+xl,fg);
		drv->draw_hline(dev,x+xl,y,x+xh,bg);
		drv->draw_hline(dev,x+xh,y,x+xw,fg);
		if (yw>=1){
			if (yw>=2){
				drv->draw_vline(dev,x,y+1,y+yl,fg);
				drv->draw_vline(dev,x,y+yl,y+yh,bg);
				drv->draw_vline(dev,x,y+yh,y+yw-1,fg);
				if (xw>=1){
					if (xw>=2){
						drv->fill_area(dev,
							x+1,y+1,x+xw-1,y+yw-1,
							bg);
					}
					drv->draw_vline(dev,x+xw-1,y+1,y+yl,fg);
					drv->draw_vline(dev,x+xw-1,y+yl,y+yh,bg);
					drv->draw_vline(dev,x+xw-1,y+yh,y+yw-1,fg);
				}
			}
			drv->draw_hline(dev,x,y+yw-1,x+xl,fg);
			drv->draw_hline(dev,x+xl,y+yw-1,x+xh,bg);
			drv->draw_hline(dev,x+xh,y+yw-1,x+xw,fg);
		}
	}else {
		/* Draw full sides and the box inside */
		drv->draw_hline(dev,x,y,x+xw,fg);
		if (yw>=1){
			if (yw>=2){
				drv->draw_vline(dev,x,y+1,y+yw-1,fg);
				if (xw>=1){
					if (xw>=2){
						if (broken < 2) drv->fill_area(dev,
							x+1,y+1,x+xw-1,y+yw-1,
							bg);
					}
					drv->draw_vline(dev,x+xw-1,y+1,
						y+yw-1,fg);
				}
			}
			drv->draw_hline(dev,x,y+yw-1,x+xw,fg);
		}
		if (broken == 2 && xw > 2 && yw > 2) {
			draw_frame_mark(drv, dev, x + 1, y + 1, xw - 2, yw - 2, bg, fg, 3);
		}
	}
}

/* Entry is allowed only in states 12, 13, 14, 15
 * Draws the picture from bitmap.
 * Before doing so, ensures that bitmap is present and if not, converts it from
 * the buffer.
 */
static void draw_picture(struct f_data_c *fdatac, struct g_object_image *goi,
		int x, int y, int bg)
{
	struct graphics_device *dev=fdatac->ses->term->dev;
	struct cached_image *cimg=goi->cimg;
	struct rect saved;

#ifdef DEBUG
	if (goi->cimg->state<12||goi->cimg->state>=16){
		fprintf(stderr,"cimg->state=%d\n",cimg->state);
		internal("Invalid cimg->state in draw_picture");
	}
#endif /* #ifdef DEBUG */
	if (!(cimg->state&1)){
		if (!cimg->bmp.user)
			buffer_to_bitmap(cimg);
	}
#ifdef DEBUG
	else if (!cimg->bmp.user){
		fprintf(stderr,"cimg->state=%d\n",cimg->state);
		internal("Nonexistent bitmap in said cimg->state in draw_picture");
	}
#endif /* #ifdef DEBUG */
	restrict_clip_area(dev, &saved, x, y, x+goi->xw, y+goi->yw);
	drv->draw_bitmap(dev,&cimg->bmp,x,y);
	drv->fill_area(dev, x+cimg->bmp.x, y, x+goi->xw,y+cimg->bmp.y, bg);
	drv->fill_area(dev, x,y+cimg->bmp.y,x+goi->xw, y+goi->yw,bg);
	drv->set_clip_area(dev,&saved);
}

/* Ensures in buffer there is not newer picture than in bitmap. Allowed to be
 * called only in state 12, 13, 14, 15.
 */
void update_bitmap(struct cached_image *cimg)
{
#ifdef DEBUG
	if (cimg->state<12||cimg->state>=16){
		fprintf(stderr,"cimg->state=%d\n",cimg->state);
		internal("Invalid state in update_bitmap");
	}
#endif /* #ifdef DEBUG */
	if (!(cimg->state&1)&&
		!cimg->strip_optimized
		&&cimg->rows_added) buffer_to_bitmap(cimg);
}

/* Draws the image at x,y. Is called from other C sources. */
void img_draw_image (struct f_data_c *fdatac, struct g_object_image *goi,
	int x, int y)
{
	long color_bg, color_fg;
	struct cached_image *cimg = goi->cimg;
	/* refresh_image(fdatac, goi, 1000); To sem asi napsal mikulas jako
	 * navod, jak se vola to refresh_image.  Nicmene ja jsem milostive
	 * usoudil, ze zadnejch 1000, ale 0.
	 */
	
	if (!(goi->xw&&goi->yw)) return; /* At least one dimension is zero */
	global_goi=goi;
	global_cimg=goi->cimg;
	if (img_process_download(goi, fdatac)) return; /* Choked with data, will not
							* draw. */
	/* Now we will only draw... */
	color_bg=dip_get_color_sRGB(cimg->background_color);
        color_fg=dip_get_color_sRGB(get_foreground(cimg->background_color));
	if (cimg->state<12){
		draw_frame_mark(drv, fdatac->ses->term->dev,x,y,goi->xw, 
			goi->yw,color_bg,color_fg,cimg->state&1);
	}else
#ifdef DEBUG
	if (cimg->state<16){
#else
	{
#endif /* #ifdef DEBUG */
		update_bitmap(cimg);
		draw_picture(fdatac,goi,x,y,color_bg);
	}
#ifdef DEBUG
	else{
		fprintf(stderr,"cimg->state=%d\n",cimg->state);
		internal("Invalid state in img_draw_image");
	}
#endif /* #ifdef DEBUG */
	if (fdatac->active && fdatac->vs->current_link != -1 && fdatac->vs->current_link == goi->link_num
    	&& fdatac->f_data->links[goi->link_num].where) {
                draw_frame_mark(drv, fdatac->ses->term->dev,x,y,goi->xw,
			goi->yw,color_bg,color_fg,2);
	}
}

/* Prior to calling this function you have to fill out
 * image -> xw
 * image -> yw
 * image -> background
 * 
 * The URL will not be freed.
 */
void find_or_make_cached_image(struct g_object_image *image, unsigned char *url,
	int scale)
{
	struct cached_image *cimg;

	if (!(cimg = find_cached_image(image->background, url, image->xw,
		image->yw, scale, aspect))){
		/* We have to make a new image cache entry */
		cimg = mem_alloc(sizeof(*cimg));
		cimg->refcount = 1;
		cimg->background_color=image->background;
#ifdef DEBUG
		if (!url)
			internal ("NULL url as argument of\
find_or_make_cached_image");
#endif /* #ifdef DEBUG */
		cimg->scale = scale;
		cimg->aspect = aspect;
		cimg->url = stracpy(url);
		cimg->wanted_xw = image->xw;
		cimg->wanted_yw = image->yw;
		cimg->xww = image->xw<0?img_scale_h(cimg->scale, 32):cimg->wanted_xw;
		cimg->yww = image->yw<0?img_scale_v(cimg->scale, 32):cimg->wanted_yw;
		cimg->state=0;
		/* width, height, image_type, buffer, buffer_bytes_per_pixel, red_gamma,
		 * green_gamma, blue_gamma, gamma_stamp, bitmap, last_length, rows_added,
		 * and decoder is invalid in both state 0 and state 2. Thus is need no to
		 * be filled in.
		 */

		/* last_count2 is unitialized */
		cimg->last_count2=-1;
		if (cimg->wanted_xw>=0&&cimg->wanted_yw>=0) cimg->state|=2;
		add_image_to_cache(cimg);
	}
	global_cimg=image->cimg=cimg;
}

struct g_object_image *insert_image(struct g_part *p, struct image_description *im)
{
	struct g_object_image *image;
	struct cached_image *cimg;
	int retval;

	image=mem_alloc(sizeof(*image));
	global_goi=image;
        image->type=G_OBJECT_IMAGE;
        image->mouse_event=&g_text_mouse;
	image->draw=&img_draw_image;
	image->destruct=&img_destruct_image;
	image->get_list=NULL;
	image->link_num = im->link_num;
	image->link_order = im->link_order;
	image->map = NULL;
	/*
	image->x is already filled
	image->y is already filled
	*/
	image->xw=img_scale_h(d_opt->image_scale, im->xsize);
	image->yw=img_scale_v(d_opt->image_scale, im->ysize);

	/* Put the data for javascript inside */
	image->id=(current_f_data->n_images)++;
	image->name=stracpy(im->name);
	image->alt=stracpy(im->alt);
	image->orig_src=stracpy(im->src);
	image->border=im->border;
	image->vspace=im->vspace;
	image->hspace=im->hspace;
	image->src=stracpy(im->url);

        if (!(image->xw&&image->yw)){
		/* At least one is zero */
		if (image->xw<0) image->xw=0;
		if (image->yw<0) image->yw=0;
		if (im->insert_flag)add_to_list(current_f_data->images,&image->image_list);
		else image->image_list.prev=NULL,image->image_list.next=NULL;
		return image;
	}
	/*
	image->parent is already filled
	*/
	image->af=request_additional_file(current_f_data,im->url);
	image->background=p->root->bg->u.sRGB;

	/* This supplies the result into image->cimg and global_cimg */
	find_or_make_cached_image(image, im->url, d_opt->image_scale);
	cimg=global_cimg;

next_chunk:
	retval=img_process_download(image,NULL);
	if (retval&&!(cimg->state&4)) goto next_chunk;
	image->xw=image->cimg->xww;
	image->yw=image->cimg->yww;
	if (cimg->state==0||cimg->state==8) image->af->need_reparse=1;
	if (im->insert_flag)add_to_list(current_f_data->images,&image->image_list);
	else image->image_list.prev=NULL,image->image_list.next=NULL;
	return image;
}

void change_image (struct g_object_image *goi, unsigned char *url, unsigned char *src, struct f_data
		*fdata)
{
	/*struct cached_image *cimg;*/

	global_goi=goi;
	mem_free(goi->src);
	goi->src=stracpy(url);
	if (goi->orig_src)mem_free(goi->orig_src);
	goi->orig_src=stracpy(src);
	if (!(goi->xw&&goi->yw)) return;
	goi->cimg->refcount--;
	goi->af=request_additional_file(fdata,url);

	find_or_make_cached_image(goi, url, fdata->opt.image_scale);
	/* Automatically sets up global_cimg */

	refresh_image(fdata->fd,(struct g_object*)goi,1);
}

#endif
