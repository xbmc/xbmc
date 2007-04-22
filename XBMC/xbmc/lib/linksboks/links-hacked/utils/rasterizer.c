/*

 Simple Unicode TTF rasterizer

 (part of Hacked Links project)

Usage: ./rasterizer <options> font_file.ttf
Where options are:
	height=120
	transform_x=1
	transform_y=1
	blur_pix=1
	blurs=1
	dump_all=0
	dump_first=0
	dump_last=65535

 it will create directory tmp/ and place char pngs to it

 */

#include "../config.h"

#ifdef HAVE_FREETYPE
#ifdef HAVE_GD_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

/* You need libGD to compile this! */

#include <gd.h>
#include <stdio.h>
#include <stdlib.h>

/* Dump to text files instead of PNG */
#undef TEXT


/* Pixmap size */
#define HEIGHT 120

/* Transformation coefficients */
#define TRANSFORM_X 1
#define TRANSFORM_Y 1

/* Additional smoothing */
#define BLUR_PIX 1
#define BLURS 1

/* Whether to dump all lower Unicode */
#define DUMP_ALL 0

int height=HEIGHT;
double transform_x=TRANSFORM_X;
double transform_y=TRANSFORM_Y;
int blur_pix=BLUR_PIX;
int blurs=BLURS;
int dump_all=DUMP_ALL;
int dump_first=0;
int dump_last=65535;


int pixel_height;
int baseline;

struct chars_to_dump {
        int n1;
        int n2;
};

/* UNICODE sets to rasterize, just comment out or add what you want */
struct chars_to_dump chars[]={
        { 0x0000,0x00ff },        /* Latin-1 */
        { 0x0100,0x024f },        /* Latin Extended */
        { 0x0250,0x02af },        /* IPA Extensions */
        { 0x02b0,0x036f },        /* Spacings and Diacritics */
        { 0x0370,0x0400 },        /* Greek */
        { 0x0401,0x052f },        /* Cyrillic */
        { 0x0590,0x05ff },        /* Hebrew */
        { 0x0600,0x06ff },        /* Arabic */
        { 0x1e00,0x06ff },        /* Latin extended additional */
        { 0x1f00,0x1fff },        /* Greek extended */
        { 0x2000,0x20ff },        /* Punctuations, sub/supscripts, currencies */
        { 0x2100,0x21ff },        /* misc. */
        { 0,0 },    /* Must be the last!!! */
};

void blur_x(int *buffer,int width, int height)
{
        int x,y;
        int *buf=(int*)malloc(width*height*sizeof(int));

        for(y=0;y<height;y++)
                for(x=0;x<width;x++){
                        int sum=0;
                        int xmin=(x-blur_pix>0)?(x-blur_pix):0;
                        int xmax=(x+blur_pix<width)?(x+blur_pix):width-1;
                        double k=1./(xmax-xmin+1);
                        int xxx;
                        for(xxx=xmin;xxx<=xmax;xxx++)
                                sum+=k*buffer[y*width+xxx];
                        buf[y*width+x]=sum;
                }

        memcpy(buffer,buf,sizeof(int)*width*height);
        free(buf);
}

void blur_y(int *buffer,int width, int height)
{
        int x,y;
        int *buf=(int*)malloc(width*height*sizeof(int));

        for(x=0;x<width;x++)
                for(y=0;y<height;y++){
                        int sum=0;
                        int ymin=(y-blur_pix>0)?(y-blur_pix):0;
                        int ymax=(y+blur_pix<height)?(y+blur_pix):height-1;
                        double k=1./(ymax-ymin+1);
                        int yyy;
                        for(yyy=ymin;yyy<=ymax;yyy++)
                                sum+=k*buffer[yyy*width+x];
                        buf[y*width+x]=sum;
                }

        memcpy(buffer,buf,sizeof(int)*width*height);
        free(buf);
}

void dump_bitmap(int ch,unsigned char *buffer, int width, int height, int x0, int y0, int cols, int rows)
{
        char *filename=(char*)malloc(50);
        int x,y,i;
        int *bitmap=(int*)calloc(height*width,sizeof(int));

        snprintf(filename,49,"tmp/%04x.png",ch);
        printf(" -> %s\n",filename);
        
        if(width>0 && !(ch!=32 && rows==0)) {
                for(y=0;y<rows;y++){
                        for(x=0;x<cols;x++){
                                int l=y*cols+x;
                                int d=(y+y0)*width+x+x0;
                                if(buffer[l] && x+x0>0 && x+x0<width && y+y0>0 && y+y0<height)
                                        bitmap[d]=buffer[l];
                        }
                }
                for(i=0;i<BLURS;i++){
                        blur_x(bitmap,width,height);
                        blur_y(bitmap,width,height);
                }

#ifdef TEXT
                {
                        FILE *f=fopen(filename,"w");
                        for(y=0;y<height;y++){
                                for(x=0;x<width;x++){
                                        fprintf(f,"%d ",bitmap[y*width+x]);
                                }
                                fprintf(f,"\n");
                        }
                        fclose(f);
                }
#else
                {
                        gdImagePtr im=gdImageCreate(width,height);
                        FILE *f;
                        int *color=(int*)malloc(256*sizeof(int));
                        int d;
                        int x,y;

                        for(d=0;d<256;d++)
                                color[d]=gdImageColorAllocate(im,d,d,d);

                        for(y=0;y<height;y++)
                                for(x=0;x<width;x++)
                                        gdImageSetPixel(im,x,y,color[bitmap[y*width+x]]);

                        f=fopen(filename,"wb");
                        gdImagePng(im,f);
                        fclose(f);

                        gdImageDestroy(im);
                        free(color);
                }
#endif
        }

        free(filename);
        free(bitmap);
}

void look_at_char(FT_Face face,int ch)
{
        FT_GlyphSlot  slot = face->glyph;  // a small shortcut
        int glyph = FT_Get_Char_Index( face, ch );
        FT_Vector  origin;
        FT_Bitmap  bitmap;
        int width;

        origin.x = 0;
        origin.y = 0;

        if(glyph==0) return;
        printf("char %d -> glyph %d  ",ch,glyph);

        FT_Load_Glyph(face,glyph,0);

        FT_Render_Glyph(face->glyph,0);

        FT_Glyph_To_Bitmap(&slot,0,&origin,1);
    
        width=slot->advance.x >> 6;

        {
                FT_Bitmap bitmap=slot->bitmap;
                int rows=bitmap.rows;
                int pitch=bitmap.pitch;
                int cols=bitmap.width;
                int bitmap_top=slot->bitmap_top;
                int bitmap_left=slot->bitmap_left;
                int x0=bitmap_left;
                int y0=baseline-bitmap_top;
                int ngrays=bitmap.num_grays;

                printf("%d %d, %d %d, %d",width, height, x0, y0, ngrays);
                dump_bitmap(ch,bitmap.buffer,width,height,x0,y0,cols,rows);
        }
}

int main(int argc, char** argv)
{
        FT_Library  library;
        FT_Face face;
        char *filename=NULL;
        int d;
        int font_ascend;
        int font_descend;
        int font_height;
        if(FT_Init_FreeType( &library ))
                return 1;

        if(argc<2){
                printf("Usage: %s <options> font_file\n",argv[0]);
                printf("Where options are:\n");
                printf("\theight=%d\n",height);
                printf("\ttransform_x=%g\n",transform_x);
                printf("\ttransform_y=%g\n",transform_y);
                printf("\tblur_pix=%d\n",blur_pix);
                printf("\tblurs=%d\n",blurs);
                printf("\tdump_all=%d\n",dump_all);
                printf("\tdump_first=%d\n",dump_first);
                printf("\tdump_last=%d\n",dump_last);
                return 1;
        }

        for(d=1;d<argc;d++)
                if(!strncmp(argv[d],"height=",7))
                        sscanf(argv[d],"height=%d",&height);
                else if(!strncmp(argv[d],"transform_x=",12))
                        sscanf(argv[d],"transform_x=%lf",&transform_x);
                else if(!strncmp(argv[d],"transform_y=",12))
                        sscanf(argv[d],"transform_y=%lf",&transform_x);
                else if(!strncmp(argv[d],"blur_pix=",9))
                        sscanf(argv[d],"blur_pix=%d",&blur_pix);
                else if(!strncmp(argv[d],"blurs=",6))
                        sscanf(argv[d],"blurs=%d",&blurs);
                else if(!strncmp(argv[d],"dump_all=",9))
                        sscanf(argv[d],"dump_all=%d",&dump_all);
                else if(!strncmp(argv[d],"dump_first=",11))
                        sscanf(argv[d],"dump_first=%d",&dump_first);
                else if(!strncmp(argv[d],"dump_last=",10))
                        sscanf(argv[d],"dump_last=%d",&dump_last);
                else
                        filename=argv[d];

        if(!filename)
                return 1;

        printf("Trying %s\n",filename);
        if(FT_New_Face(library, filename,0,&face))
                return;
        printf("Ok, %s, %s\n",face->family_name,face->style_name);

        font_ascend  = face->ascender;
        font_descend = face->descender;
        font_height  = face->height;
        printf("Ascend %d, descend %d, height %d\n",font_ascend,font_descend,font_height);
        printf("Vertical BBox is %d - %d\n",face->bbox.yMin,face->bbox.yMax);

        /*
        ascend  = face->bbox.yMax;
        descend = face->bbox.yMin;
        height  = ascend-descend;
        */

        pixel_height=(double)height*(font_ascend-font_descend)/(double)font_height;

        baseline=(double)height*(font_height+font_descend)/(double)font_height;

        printf("Our baseline will be at %d\n",baseline);

        FT_Set_Pixel_Sizes(face,0,pixel_height);

        {
                FT_Matrix     matrix;              // transformation matrix
                FT_Vector     origin;

                matrix.xx=(FT_Fixed)(transform_x*0x10000);
                matrix.xy=0;
                matrix.yx=0;
                matrix.yy=(FT_Fixed)(transform_y*0x10000);

                origin.x=0;
                origin.y=0;
                FT_Set_Transform(face,&matrix,&origin);
        }

        system("rm -rf tmp");
        system("mkdir tmp");

        if(dump_all){
                /* Dumping all lower Unicode */
                printf("Dumping all available UNICODE range...\n");
                for(d=dump_first;d<=dump_last;d++)
                        look_at_char(face,d);
        }
        else {
                d=0;
                while(chars[d].n2){
                        int ch;
                        int n1=chars[d].n1;
                        int n2=chars[d].n2;
                        printf("Processing UNICODE chars from %d till %d...\n",n1,n2);
                        for(ch=n1;ch<=n2;ch++)
                                look_at_char(face,ch);
                        d++;
                }
        }
        return 1;
}
#endif
#endif