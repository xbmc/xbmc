/* jpeg.c
 * JPEG decoding
 * (c) 2002 Karel 'Clock' Kulhavy
 * This file is a part of the Links program, released under GPL.
 */

#include "cfg.h"

#ifdef G
#include "links.h"

#ifdef HAVE_JPEG
#include "../jpeg-6b/jpeglib.h"

#if BITS_IN_JSAMPLE != 8
#error "You have a weird jpeglib compiled for 12 bits per sample that is not able to read ordinary JPEG's. \
See INSTALL for description how to compile Links with jpeglib statically to supply your own \"good\" version \
of jpeglib or reinstall your system's jpeglib to be a normal one."
#endif /* #if BITS_IN_JSAMPLE != 8 */

struct jerr_struct{
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

struct jerr_struct *global_jerr;
struct jpeg_decompress_struct *global_cinfo;

METHODDEF(void) my_error_exit(j_common_ptr cinfo)
{
	longjmp(global_jerr->setjmp_buffer,2);
}

METHODDEF(void) /* Only for the sake of libjpeg */
nop(j_decompress_ptr cinfo)
{
}

METHODDEF(void)
my_output_message(j_common_ptr cinfo)
{
}

METHODDEF(boolean) my_fill_input_buffer(j_decompress_ptr cinfo)
{
 return FALSE; /* We utilize I/O suspension (or emulsion? ;-) ) */
}

METHODDEF(void) my_skip_input_data(j_decompress_ptr cinfo,long num_bytes)
{
	if (num_bytes>cinfo->src->bytes_in_buffer)
	{
	 	/* We have to enter skipping state */
	 	cinfo->src->next_input_byte+=cinfo->src->bytes_in_buffer;
		((struct jpg_decoder *)(global_cimg->decoder))->skip_bytes
  			=num_bytes-cinfo->src->bytes_in_buffer;
  		cinfo->src->bytes_in_buffer=0;
	}
	else
	{
		/* We only pull out some bytes from buffer. */
		cinfo->src->next_input_byte+=num_bytes;
		cinfo->src->bytes_in_buffer-=num_bytes;
	}
}

void jpeg_start(struct cached_image *cimg)
{
	struct jpg_decoder *jd;

	global_cinfo=mem_alloc(sizeof(*global_cinfo));
	global_jerr=mem_alloc(sizeof(*global_jerr));
	global_cinfo->err = jpeg_std_error(&(global_jerr->pub));
	global_jerr->pub.error_exit=my_error_exit;
	global_jerr->pub.output_message=my_output_message;
	if (setjmp(global_jerr->setjmp_buffer)){
g19_2000:
		mem_free(global_cinfo);
		mem_free(global_jerr);
		img_end(cimg);
		return;
	}
	jpeg_create_decompress(global_cinfo);
	if (setjmp(global_jerr->setjmp_buffer)){
		jpeg_destroy_decompress(global_cinfo);
		goto g19_2000;
		return;
	}
	jpeg_stdio_src(global_cinfo,stdin);
	global_cinfo->src->init_source=&nop;
	global_cinfo->src->fill_input_buffer=&my_fill_input_buffer;
	global_cinfo->src->skip_input_data=&my_skip_input_data;
	global_cinfo->src->resync_to_restart=&jpeg_resync_to_restart;
	global_cinfo->src->term_source=nop;
	global_cinfo->src->bytes_in_buffer=0;
	global_cinfo->src->next_input_byte=NULL;
	cimg->decoder=mem_alloc(sizeof(struct jpg_decoder));
	jd=(struct jpg_decoder *)cimg->decoder;
	jd->cinfo=global_cinfo;
	jd->jerr=global_jerr;
	jd->state=0;
	jd->skip_bytes=0;
	jd->jdata=NULL;
	jd->scanlines[0]=NULL; /* This flags emptiness */
}

void jpeg_restart(struct cached_image *cimg, unsigned char *data, int length)
{
	struct jpg_decoder *deco;

	deco=(struct jpg_decoder *)(cimg->decoder);
#ifdef DEBUG
	if (!deco) internal("NULL decoder in jpeg_restart");
#endif /* #ifdef DEBUG */
	global_cinfo=((struct jpg_decoder *)(cimg->decoder))->cinfo;
	global_jerr=((struct jpg_decoder *)(cimg->decoder))->jerr;

	if (deco->skip_bytes>=length){
		deco->skip_bytes-=length;
		return;
	}else{
		data+=deco->skip_bytes;
		length-=deco->skip_bytes;
		deco->skip_bytes=0;
	}
	if (deco->jdata){
		memcpy(deco->jdata,global_cinfo->src->next_input_byte,
			global_cinfo->src->bytes_in_buffer);
		deco->jdata=mem_realloc(
			deco->jdata, global_cinfo->src->bytes_in_buffer+length);
	}else{
		deco->jdata=mem_alloc(global_cinfo->src->bytes_in_buffer+length);
	}
	memcpy(deco->jdata+global_cinfo->src->bytes_in_buffer
		,data,length);
	global_cinfo->src->next_input_byte=deco->jdata;
	/* ...:::...:..:.:::.:.::::.::.:.:.:.::..::::.::::.:...: */
	global_cinfo->src->bytes_in_buffer+=length;
	if (setjmp(global_jerr->setjmp_buffer)) goto decoder_ended;
	switch(deco->state){
		case 0:
		/* jpeg_read_header */
	   	if (JPEG_SUSPENDED==jpeg_read_header(global_cinfo,TRUE))
			break;
		global_cinfo->buffered_image=TRUE;
		deco->state=1;

		case 1:
		/* If the scaling is sufficiently brutal we can leave out
		 * some DCT coefficients...: */
		/* jpeg_start_decompress */
		if (jpeg_start_decompress(global_cinfo)==FALSE)
			break;
		cimg->width=global_cinfo->output_width;

		/* If the image is grayscale then as soon as the width
		 * is known, the scanlines are allocated to the space
	         * needed for grayscale data.
	         */	 
		if (global_cinfo->output_components==1){
			int a;

			/* It means grayscale data */
#ifdef DEBUG
			if (deco->scanlines[0])
				internal("Prebouchavam deco->scanlines[0]");
#endif /* #ifdef DEBUG */
			deco->scanlines[0]=mem_alloc(16*cimg->width);	
			for (a=1;a<16;a++){
				deco->scanlines[a]=deco->scanlines[0]
					+a*cimg->width;
			}
		}
		   
		cimg->height=global_cinfo->output_height;
		cimg->buffer_bytes_per_pixel=3;
		cimg->red_gamma=sRGB_gamma;
		cimg->green_gamma=sRGB_gamma;
		cimg->blue_gamma=sRGB_gamma;
		cimg->strip_optimized=0;
		header_dimensions_known(cimg);
new_scan:
		deco->state=2;

		case 2:
		/* jpeg_start_output */
		if (FALSE==jpeg_start_output(global_cinfo,global_cinfo->input_scan_number)){
susp0:
			/* Suspended */
			break;
		}
		deco->state=3;

		case 3:
		/* jpeg_read_scanlines */
		if (global_cinfo->output_components==1){
			/* grayscale */

			int lines,pixels;
			unsigned char *dest, *src;
			
                	while (global_cinfo->output_scanline
				<global_cinfo->output_height){
                 		if ((lines=jpeg_read_scanlines(global_cinfo,deco->scanlines,16)))
                 		{
					cimg->rows_added=1;
					pixels=lines*global_cinfo->output_width;
					dest=cimg->buffer+cimg->buffer_bytes_per_pixel*cimg->width
							 *(global_cinfo->output_scanline-lines);
					src=deco->scanlines[0];
					for(;pixels;pixels--){
						/* Transform GRAY -> R G B triplet */
						dest[0]=*src;
						dest[1]=*src;
						dest[2]=*src;
						dest+=3;
						src++;
					}
                 		}
                 		else
                 		{
	                		 /* We are suspended and we want more data */
                 	 		goto susp0; /* Break the switch statement */
                 		}
			}
		}else{
			/* color */
                	while (global_cinfo->output_scanline
				<global_cinfo->output_height){
				int a;

#ifdef DEBUG
				if (deco->scanlines[0])
					internal("Prebouchavam dec->scanlines v barevne sekci");
#endif /* #ifdef DEBUG */
				for (a=0;a<16;a++) deco->scanlines[a]=cimg->buffer
					+(global_cinfo
					->output_scanline+a)
					*global_cinfo->output_width*3;
			
                 		if (jpeg_read_scanlines(global_cinfo,deco->scanlines,1)){
					 cimg->rows_added=1;
					 deco->scanlines[0]=NULL;
				}else{
					 deco->scanlines[0]=NULL;
	                	 	/* We are suspended and we want more data */
                 	 		goto susp0; /* Break the switch statement */
				}
			}
		}
		deco->state=4;

		case 4:
		/* jpeg_finish_output */
		if (FALSE==jpeg_finish_output(global_cinfo))
		{
			/* Suspended */
			break;
		}
		if (!jpeg_input_complete(global_cinfo))
		{
			/* Some more scans awaited... */
			goto new_scan;
		}
		deco->state=5;

		case 5:
		/* jpeg_finish_decompress */
		if (FALSE==jpeg_finish_decompress(global_cinfo))
			break;
decoder_ended:
		img_end(cimg);
	}
}
#endif /* #ifdef HAVE_JPEG */

#endif /* #ifdef G */

