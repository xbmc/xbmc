/* Builtin fonts handling */

#include "links.h"

#ifdef G

/* Points to the next unread byte from png data block */
#ifdef __XBOX__
#ifdef XBOX_BFONTS_SECTION
unsigned char *builtin_font_data;
#else
FILE *fontfd;
#endif
#else
extern unsigned char builtin_font_data[];
#endif
extern struct builtin_letter letter_data[];
extern struct builtin_font builtin_font_table[];


/* Number of fonts. font number 0 is system_font (it's
 * images are in system_font/ directory) and is used
 * for special purpose.
 */
extern int n_builtin_fonts;


#ifdef __XBOX__
#ifndef XBOX_BFONTS_SECTION

void xbox_read_stored_data(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
	FILE *work;

	work=png_get_io_ptr(png_ptr);
	fread(data, 1, length, fontfd);
}

#endif
#endif

void read_stored_data(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
	struct read_work *work;

	work=png_get_io_ptr(png_ptr);
	if (length>work->length) png_error(png_ptr,"Ran out of input data");
	memcpy(data,work->pointer,length);
	work->length-=length;
	work->pointer+=length;
}

void my_png_warning(png_structp a, png_const_charp b)
{
}

void my_png_error(png_structp a, png_const_charp error_string)
{
	error("Error when loading compiled-in font: %s.\n",error_string);
}

/* The data that fall out of this function express this: 0 is paper. 255 is ink. 34
 * is 34/255ink+(255-34)paper. No gamma is involved in this formula, as you can see.
 * The multiplications and additions take place in photon space.
 */
static void load_char(unsigned char **dest, int *x, int *y, 
unsigned char *png_data, int png_length)
{
	png_structp png_ptr;
	png_infop info_ptr;
	double gamma;
	int y1,number_of_passes;
	unsigned char **ptrs;
	struct read_work work;

	work.pointer = png_data;
	work.length = png_length;
	
	png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING,
			NULL, my_png_error, my_png_warning);
	info_ptr=png_create_info_struct(png_ptr);
#if defined(__XBOX__) && !defined(XBOX_BFONTS_SECTION)
	png_set_read_fn(png_ptr,&fontfd,(png_rw_ptr)&xbox_read_stored_data);
#else
	png_set_read_fn(png_ptr,&work,(png_rw_ptr)&read_stored_data);
#endif
	png_read_info(png_ptr, info_ptr);
	*x=png_get_image_width(png_ptr,info_ptr);
	*y=png_get_image_height(png_ptr,info_ptr);
	if (png_get_gAMA(png_ptr,info_ptr, &gamma))
		png_set_gamma(png_ptr, 1.0, gamma);
	else
		png_set_gamma(png_ptr, 1.0, sRGB_gamma);
	{
		int bit_depth;
		int color_type;

		color_type=png_get_color_type(png_ptr, info_ptr);
		bit_depth=png_get_bit_depth(png_ptr, info_ptr);
		if (color_type==PNG_COLOR_TYPE_GRAY){
			if (bit_depth<8){
				 png_set_expand(png_ptr);
			}
			if (bit_depth==16){
				 png_set_strip_16(png_ptr);
			}
		}
		if (color_type==PNG_COLOR_TYPE_PALETTE){
			png_set_expand(png_ptr);
#ifdef HAVE_PNG_SET_RGB_TO_GRAY
			png_set_rgb_to_gray(png_ptr,1,54.0/256,183.0/256);
#else
			goto end;
#endif
		}
		if (color_type & PNG_COLOR_MASK_ALPHA){
			png_set_strip_alpha(png_ptr);
		}
		if (color_type==PNG_COLOR_TYPE_RGB ||
			color_type==PNG_COLOR_TYPE_RGB_ALPHA){
#ifdef HAVE_PNG_SET_RGB_TO_GRAY
			png_set_rgb_to_gray(png_ptr, 1, 54.0/256, 183.0/256);
#else
			goto end;
#endif
		}
		
	}
	/* If the depth is different from 8 bits/gray, make the libpng expand
	 * it to 8 bit gray.
	 */
	number_of_passes=png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr,info_ptr);
	*dest=mem_alloc(*x*(*y));
	ptrs=mem_alloc(*y*sizeof(*ptrs));
	for (y1=0;y1<*y;y1++) ptrs[y1]=*dest+*x*y1;
	for (;number_of_passes;number_of_passes--){
		png_read_rows(png_ptr, ptrs, NULL, *y);
	}
	png_read_end(png_ptr, NULL);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	mem_free(ptrs);
	return;
#ifndef HAVE_PNG_SET_RGB_TO_GRAY
	end:
	*dest=mem_calloc(*x*(*y));
	return;
#endif
}

/* Returns a pointer to a structure describing the letter found or NULL
 * if the letter is not found. Tries all possibilities in the style table
 * before returning NULL.
 */
struct builtin_letter *find_stored_letter(struct builtin_font *bfont, int letter_number)
{
	int first, last, half, diff, font_index, font_number;

        first=bfont->begin;
        last=bfont->length+first-1;

        while(first<=last){
                half=(first+last)>>1;
			diff=letter_data[half].code-letter_number;
                        if (diff>=0){
                                if (diff==0){
                                        return letter_data+half;
                                }else{
					/* Value in table is bigger */
                                        last=half-1;
                                }
                        }else{
                                /* Value in the table is smaller */
                                first=half+1;
			}
        }
        return NULL;
}


int get_builtin_char_metric(struct font *font, int code, int *x, int *y)
{
	struct builtin_letter *l;

        l=find_stored_letter((struct builtin_font *)(font->data),code);
	if (!l){
		*x=0;
                *y=0;
                return 0;
        }

        *x=l->xsize;
        *y=l->ysize;

        return 1;
}

#if defined(__XBOX__) && !defined(XBOX_BFONTS_SECTION)
int get_builtin_char(struct font *font, int code, unsigned char **dest, int *x, int *y)
{
	struct builtin_letter *letter=find_stored_letter((struct builtin_font *)(font->data),code);
	fseek(fontfd, letter->begin, SEEK_SET);
	//rewind(fontfd);

	load_char(dest, x, y, NULL, letter->length);
    return 1;
}

#else

int get_builtin_char(struct font *font, int code, unsigned char **dest, int *x, int *y)
{
	struct builtin_letter *letter=find_stored_letter((struct builtin_font *)(font->data),code);
        unsigned char *png_begin=builtin_font_data+letter->begin;
        int png_length=letter->length;

        load_char(dest, x, y, png_begin, png_length);
        return 1;
}

#endif

void index_builtin_font(struct font *font)
{
        int current=0;
        int d;
        struct builtin_font *bfont=(struct builtin_font*)font->data;
        int length=bfont->length;

        font->letter=mem_alloc(length*sizeof(struct letter));

        for(d=bfont->begin;d<bfont->begin+length;d++){
                int xw,yw;
                struct builtin_letter *letter=&(letter_data[d]);
                font->letter[current].xsize=letter->xsize;
                font->letter[current].ysize=letter->ysize;
                font->letter[current].code=letter->code;
                font->letter[current].list=NULL;
                current++;
        }
        font->n_letters=current;
}

struct font *builtin_font_create(struct builtin_font *bfont)
{
        struct font *font=create_font(bfont->family,bfont->weight,bfont->slant,bfont->adstyl,bfont->spacing);

        font->data=bfont;
        font->font_type=FONT_TYPE_BUILTIN;

        font->get_char=get_builtin_char;
        font->get_char_metric=get_builtin_char_metric;
        font->free_font=NULL;
        font->index_font=index_builtin_font;

        return font;
}

void init_builtin_fonts()
{
        int i;
		char hey[8];

#ifdef __XBOX__

#ifdef XBOX_BFONTS_SECTION
		/* Load font data from XBE section */
		builtin_font_data = XLoadSection( "BFONTS" );
#else
        unsigned char *fonts_file = stracpy(links_home);

        add_to_strn(&fonts_file, "font_include.inc");
        if (!fonts_file)
			return;

		fontfd = fopen(fonts_file, "rb");
        mem_free(fonts_file);

        if (!fontfd)
			return;

		fseek(fontfd, 0x00157562, SEEK_SET);
		fread(hey, 1, 8, fontfd);

#endif

#endif

        for(i=0;i<n_builtin_fonts;i++){
                struct builtin_font *bfont=&(builtin_font_table[i]);
                struct font *font=builtin_font_create(bfont);
                register_font(font);
                font->index_font(font);
        }
}

void finalize_builtin_fonts()
{
#ifdef __XBOX__
#ifndef XBOX_BFONTS_SECTION
	fclose(fontfd);
#endif
#endif
}
#endif
