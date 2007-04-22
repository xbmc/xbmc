/* png.c
 * PNG decoding
 * (c) 2002 Karel 'Clock' Kulhavy
 * This is a part of the Links program, released under GPL.
 */
#include "cfg.h"

#ifdef G
#include "links.h"

#ifdef HAVE_ENDIAN_H
#include <endian.h>
#endif /* #ifdef HAVE_ENDIAN_H */

#ifdef REPACK_16
#undef REPACK_16
#endif /* #ifdef REPACK_16 */

#if SIZEOF_UNSIGNED_SHORT != 2
#define REPACK_16
#endif /* #if SIZEOF_UNSIGNED_SHORT != 2 */

#ifndef REPACK_16
#ifndef AC_LITTLE_ENDIAN
#ifndef AC_BIG_ENDIAN
#define REPACK_16
#endif /* #ifndef AC_BIG_ENDIAN */
#endif /* #ifndef AC_LITTLE_ENDIAN */
#endif /* #ifndef REPACK_16 */
	  
/* Decoder structs */

/* Warning for from-web PNG images */
void img_my_png_warning(png_structp a, png_const_charp b)
{
}

/* Error for from-web PNG images. */
void img_my_png_error(png_structp png_ptr, png_const_charp error_string)
{
	longjmp(png_ptr->jmpbuf,1);
}

void png_info_callback(png_structp png_ptr, png_infop info_ptr)
{
	int bit_depth, color_type, intent;
	double gamma;
	int bytes_per_pixel=3;
	struct cached_image *cimg;

	cimg=global_cimg;

	bit_depth=png_get_bit_depth(png_ptr, info_ptr);
	color_type=png_get_color_type(png_ptr, info_ptr);
	if (color_type == PNG_COLOR_TYPE_PALETTE)
        	png_set_expand(png_ptr);
    	if (color_type == PNG_COLOR_TYPE_GRAY &&
        	bit_depth < 8) png_set_expand(png_ptr);
	if (png_get_valid(png_ptr, info_ptr,
        	PNG_INFO_tRNS)){
		png_set_expand(png_ptr); /* Legacy version of
		png_set_tRNS_to_alpha(png_ptr); */
		bytes_per_pixel++;
	}
	if (color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	if (bit_depth==16){
#ifndef REPACK_16
#ifdef AC_LITTLE_ENDIAN
		/* We use native endianity only if unsigned short is 2-byte
		 * because otherwise we have to reassemble the buffer so we
		 * will leave in the libpng-native big endian.
		 */
		png_set_swap(png_ptr);
#endif /* #ifdef AC_LITTLE_ENDIAN */
#endif /* #ifndef REPACK_16 */
		bytes_per_pixel*=sizeof(unsigned short);
	}
	png_set_interlace_handling(png_ptr);
	if (color_type==PNG_COLOR_TYPE_RGB_ALPHA
		||color_type==PNG_COLOR_TYPE_GRAY_ALPHA){
		if (bytes_per_pixel==3
			||bytes_per_pixel==3*sizeof(unsigned short))
			bytes_per_pixel=4*bytes_per_pixel/3;
	}
	cimg->width=png_get_image_width(png_ptr,info_ptr);
	cimg->height=png_get_image_height(png_ptr,info_ptr);
	cimg->buffer_bytes_per_pixel=bytes_per_pixel;
	if (png_get_sRGB(png_ptr, info_ptr, &intent)){
		gamma=sRGB_gamma;
	}
	else
 	{              
  		if (!png_get_gAMA(png_ptr, info_ptr, &gamma)){
			gamma=sRGB_gamma;
		}
	}
	cimg->red_gamma=gamma;
	cimg->green_gamma=gamma;
	cimg->blue_gamma=gamma;
	png_read_update_info(png_ptr,info_ptr);                 
	cimg->strip_optimized=0;
	header_dimensions_known(cimg);
}

/* Converts unsigned shorts to doublechars (in big endian) */
void a2char_from_unsigned_short(unsigned char *chr, unsigned short *shrt, int len)
{
	unsigned short s;

	for (;len;len--,shrt++,chr+=2){
		s=*shrt;
		*chr=s>>8;
		chr[1]=s;
	}
}

/* Converts doublechars (in big endian) to unsigned shorts */
void unsigned_short_from_2char(unsigned short *shrt, unsigned char *chr, int len)
{
	unsigned short s;
	
	for (;len;len--,shrt++,chr+=2){
		s=((*chr)<<8)|chr[1];
		*shrt=s;
	}
}

void png_row_callback(png_structp png_ptr, png_bytep new_row, png_uint_32
	row_num, int pass)
{
	struct cached_image *cimg;
#ifdef REPACK_16
	unsigned char *tmp;
	int channels;
#endif /* #ifdef REPACK_16 */

	cimg=global_cimg;
#ifdef REPACK_16
	if (cimg->buffer_bytes_per_pixel>4)
	{
		channels=cimg->buffer_bytes_per_pixel/sizeof(unsigned
			short);
		if (PNG_INTERLACE_NONE==png_get_interlace_type(png_ptr,
			((struct png_decoder *)cimg->decoder)->info_ptr))
		{
			unsigned_short_from_2char((unsigned short *)(cimg->buffer+cimg
				->buffer_bytes_per_pixel *cimg->width
				*row_num), new_row, cimg->width
				*channels);
		}else{
			tmp=mem_alloc(cimg->width*2*channels);
			a2char_from_unsigned_short(tmp, (unsigned short *)(cimg->buffer
				+cimg->buffer_bytes_per_pixel
				*cimg->width*row_num), cimg->width*channels);
			png_progressive_combine_row(png_ptr, tmp, new_row);
			unsigned_short_from_2char((unsigned short *)(cimg->buffer
				+cimg->buffer_bytes_per_pixel
				*cimg->width*row_num), tmp, cimg->width*channels);
			mem_free(tmp);
		}
	}else
#endif /* #ifdef REPACK_16 */
	{
		png_progressive_combine_row(png_ptr,
			cimg->buffer+cimg->buffer_bytes_per_pixel
			*cimg->width*row_num, new_row);
	}
	cimg->rows_added=1;
}

void png_end_callback(png_structp png_ptr, png_infop info)
{
	end_callback_hit=1;
}

/* Decoder structs */

void png_start(struct cached_image *cimg)
{
	png_structp png_ptr;
	png_infop info_ptr;
	struct png_decoder *decoder;

	png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING,
			NULL, img_my_png_error, img_my_png_warning);
#ifdef DEBUG
	if (!png_ptr) internal("png_create_read_struct failed\n");
#endif /* #ifdef DEBUG */
	info_ptr=png_create_info_struct(png_ptr);
#ifdef DEBUG
	if (!info_ptr) internal ("png_create_info_struct failed\n");
#endif /* #ifdef DEBUG */
	if (setjmp(png_ptr->jmpbuf)){
error:
		png_destroy_read_struct(&png_ptr, &info_ptr,
			(png_infopp)NULL);
		img_end(cimg);
		return;
	}
	png_set_progressive_read_fn(png_ptr, NULL,
				    png_info_callback, png_row_callback,
				    png_end_callback);
   	if (setjmp(png_ptr->jmpbuf)) goto error;
	decoder=mem_alloc(sizeof(*decoder));
	decoder->png_ptr=png_ptr;
	decoder->info_ptr=info_ptr;
	cimg->decoder=decoder;
}

void png_restart(struct cached_image *cimg, unsigned char *data, int length)
{
	png_structp png_ptr;
	png_infop info_ptr;

#ifdef DEBUG
	if (!cimg->decoder)
		internal("decoder NULL in png_restart\n");
#endif /* #ifdef DEBUG */
	png_ptr=((struct png_decoder *)(cimg->decoder))->png_ptr;
	info_ptr=((struct png_decoder *)(cimg->decoder))->info_ptr;
	end_callback_hit=0;
	if (setjmp(png_ptr->jmpbuf)){
		img_end(cimg);
		return;
	}
	png_process_data(png_ptr, info_ptr, data, length);
	if (end_callback_hit) img_end(cimg);
}


#endif /* #ifdef G */
