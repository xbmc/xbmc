/* FreeType backend */

#if defined(XBOX_USE_FREETYPE)
#include "../linksboks.h"
extern LinksBoksExtFont *(*g_LinksBoksGetExtFontFunction) (unsigned char *fontname, int fonttype);

extern "C" {
#endif

#include "links.h"

#ifdef HAVE_FREETYPE


/* Pixmap size */
#define HEIGHT 200

/* Transformation coefficients */
#define TRANSFORM_X 1
#define TRANSFORM_Y 1

/* Additional smoothing */
#define BLUR_PIX 1
#define BLURS 0

FT_Library library;

int ft_get_char_metric(struct font *font, int code, int *x, int *y)
{
        FT_Face face;
        FT_Error err;
        if(!font)
                return 0;
        face=(FT_Face)(font->data);

        err=FT_Load_Char(face,code,FT_LOAD_DEFAULT); // no render!
        if(err)
                return 0;

        {
                FT_GlyphSlot  slot = face->glyph;  // a small shortcut
                *y=HEIGHT;
                *x=slot->advance.x >> 6;
                if(*x==0)
                        return 0;
        }
        return 1;
}

void blur_x(unsigned char *buffer,int width, int height)
{
        int x,y;
        unsigned char *buf=(unsigned char*)malloc(width*height);

        for(y=0;y<height;y++)
                for(x=0;x<width;x++){
                        int sum=0;
                        int xmin=(x-BLUR_PIX>0)?(x-BLUR_PIX):0;
                        int xmax=(x+BLUR_PIX<width)?(x+BLUR_PIX):width-1;
                        double k=1./(xmax-xmin+1);
                        int xxx;
                        for(xxx=xmin;xxx<=xmax;xxx++)
                                sum+=k*buffer[y*width+xxx];
                        buf[y*width+x]=sum;
                }

        memcpy(buffer,buf,width*height);
        free(buf);
}

void blur_y(unsigned char *buffer,int width, int height)
{
        int x,y;
        unsigned char *buf=(unsigned char*)malloc(width*height);

        for(x=0;x<width;x++)
                for(y=0;y<height;y++){
                        int sum=0;
                        int ymin=(y-BLUR_PIX>0)?(y-BLUR_PIX):0;
                        int ymax=(y+BLUR_PIX<height)?(y+BLUR_PIX):height-1;
                        double k=1./(ymax-ymin+1);
                        int yyy;
                        for(yyy=ymin;yyy<=ymax;yyy++)
                                sum+=k*buffer[yyy*width+x];
                        buf[y*width+x]=sum;
                }

        memcpy(buffer,buf,width*height);
        free(buf);
}


int ft_get_char(struct font *font, int code, unsigned char **dest, int *x, int *y)
{
        FT_Face face;
        FT_Error err;
        if(!font)
                return 0;
        face=(FT_Face)(font->data);
        err=FT_Load_Char(face,code,FT_LOAD_RENDER);//FT_LOAD_MONOCHROME);
        if(err)
                return 0;

        {
                FT_GlyphSlot  slot = face->glyph;  // a small shortcut
                FT_Bitmap bitmap=slot->bitmap;
                int rows=bitmap.rows;
                int pitch=bitmap.pitch;
                int cols=bitmap.width;
                int bitmap_top=slot->bitmap_top;
                int bitmap_left=slot->bitmap_left;
                int x0=bitmap_left;
                int y0=font->baseline-bitmap_top;
                int i,j;

                *x=slot->advance.x >> 6;
                *y=HEIGHT;

                *dest=(unsigned char *)mem_calloc(*x*(*y));
                for(i=0;i<cols;i++)
                for(j=0;j<rows;j++)
                        {
                                int xx=i+x0;
                                int yy=j+y0;
                                if(xx>=0 && xx<(*x) && yy>=0 && yy<(*y))
					/*(*dest)[yy*(*x)+xx]=bitmap.buffer[j*cols+i];*/
					if(bitmap.buffer[j*cols+i])
                                                (*dest)[yy*(*x)+xx]=255;
                        }
                for(i=0;i<BLURS;i++){
                        blur_x(*dest,*x,*y);
                        blur_y(*dest,*x,*y);
                }

        }
        return 1;
}

void ft_free_font(struct font *font)
{
#ifndef XBOX_USE_FREETYPE
        FT_Done_Face((FT_Face)(font->data));
#endif
}

void ft_index_font(struct font *font)
{
        FT_Face face=(FT_Face)(font->data);
        int length=face->num_glyphs;
        int current=0;
        int d;
        font->letter=(letter *)mem_alloc(length*sizeof(struct letter));

        for(d=0;d<65536;d++){ /* Lower Unicode only */
                int xw,yw;
                int glyph;
                if(glyph=FT_Get_Char_Index(face,d)){
                        if(ft_get_char_metric(font,d,&xw,&yw)) {
                                font->letter[current].xsize=xw;
                                font->letter[current].ysize=yw;
                                font->letter[current].code=d;
                                font->letter[current].list=NULL;
                                current++;
                        }
                }
        }
        font->n_letters=current;
}

struct font *create_ft_font(unsigned char *filename)
{
	struct font *font;
        int font_ascend;
        int font_descend;
        int font_height;
	int pixel_height;
	FT_Face face;
#ifdef XBOX_USE_FREETYPE
	LinksBoksExtFont *lxf;
#endif

	if(!filename) return NULL;

#ifndef XBOX_USE_FREETYPE /* function is redefined below if this is defined */

	if(FT_New_Face(library, filename,0,&face))
		return NULL;

        font=create_font(face->family_name,(face->style_flags&FT_STYLE_FLAG_BOLD)?"bold":"medium",(face->style_flags&FT_STYLE_FLAG_ITALIC)?"italic":"roman","serif",(face->face_flags&FT_FACE_FLAG_FIXED_WIDTH)?"mono":"vari");

#else /* XBOX_USE_FREETYPE */

	if(!g_LinksBoksGetExtFontFunction)
		return NULL;

	lxf = g_LinksBoksGetExtFontFunction(filename, LINKSBOKS_EXTFONT_TYPE_FREETYPE);
	if(!lxf || !lxf->fontdata)
		return NULL;

	font = create_font(lxf->family, lxf->weight, lxf->slant, lxf->adstyl, lxf->spacing);

	font->data = face = (FT_Face)lxf->fontdata;

#endif /* XBOX_USE_FREETYPE */

		font->data=face;

        font_ascend  = face->ascender;
        font_descend = face->descender;
        font_height  = face->height;

	pixel_height=(double)HEIGHT*(font_ascend-font_descend)/(double)font_height;
	font->baseline=(double)HEIGHT*(font_height+font_descend)/(double)font_height;

        font->get_char=ft_get_char;
        font->get_char_metric=ft_get_char_metric;
        font->free_font=ft_free_font;
        font->index_font=ft_index_font;
        font->font_type=FONT_TYPE_FREETYPE;
        font->filename=stracpy(filename);

        FT_Set_Pixel_Sizes(face,0,pixel_height);
        {
                FT_Matrix     matrix;              // transformation matrix
                FT_Vector     origin;

                matrix.xx=(FT_Fixed)(TRANSFORM_X*0x10000);
                matrix.xy=0;
                matrix.yx=0;
                matrix.yy=(FT_Fixed)(TRANSFORM_Y*0x10000);

                origin.x=0;
                origin.y=0;
                FT_Set_Transform(face,&matrix,&origin);
        }

        return font;
}

void init_freetype()
{
#ifndef XBOX_USE_FREETYPE
        struct font *font;
        if(FT_Init_FreeType(&library))
                exit(1);
#endif
}

void finalize_freetype()
{
#ifndef XBOX_USE_FREETYPE
        FT_Done_FreeType(library);
#endif
}

#ifdef XBOX_USE_FREETYPE
}	/* extern "C" */
#endif

#endif /* HAVE_FREETYPE */
