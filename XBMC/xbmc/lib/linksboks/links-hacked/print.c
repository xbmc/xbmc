#include "links.h"

#include <locale.h>

#ifdef G

struct ps_printer {
        FILE *file;

        double page_width;
        double page_height;

        double page_leftmargin;
        double page_rightmargin;
        double page_topmargin;
        double page_bottommargin;

        double coeff;
        double clip_y_min;
        double clip_y_max;
        int extent;
};

/*
int ps_print_char(struct ps_printer *printer, int x0, int y0, struct style *style, int code)
{
        struct font *font;
        struct bitmap bitmap;
	unsigned short *primary_data;
	unsigned short red, green, blue;
        int image;

        font=find_font_with_char(code, style->table,NULL,NULL);
        bitmap.y=style->height*printer->coeff;
        if(!font) {
                bitmap.x=bitmap.y*0.5;
                if(style->mono_space>0)
                        bitmap.x=compute_width(style->mono_space,style->mono_height,style->height);

                if(display_optimize) bitmap.x*=3;
                bitmap.data=mem_calloc(bitmap.x*bitmap.y);
                {
                        int x,y;
                        unsigned char *data=bitmap.data;
                        int xw=bitmap.x;
                        int x0=0.2*bitmap.x;
                        int x1=0.8*bitmap.x;
                        int yw=bitmap.y;
                        int y0=0.2*bitmap.y;
                        int y1=0.8*bitmap.y;

                        for(x=x0;x<x1;x++){
                                data[x+xw*y0]=255;
                                data[x+(y1-1)*xw]=255;
                        }
                        for(y=y0;y<y1;y++){
                                data[y*xw+x0]=255;
                                data[y*xw+x1]=255;
                        }
                }
        } else {
                struct letter *letter=find_char_in_font(font,code);
                load_scaled_char(code, &(bitmap.data), &(bitmap.x), bitmap.y, font, style);
        }
        bitmap.skip=bitmap.x;
        primary_data=mem_alloc(3*bitmap.x*bitmap.y*sizeof(*primary_data));
	round_color_sRGB_to_48(&red, &green, &blue,
		((style->r0)<<16)|((style->g0)<<8)|(style->b0));
	mix_two_colors(primary_data, bitmap.data,
		bitmap.x*bitmap.y,
		red,green,blue,
		apply_gamma_single_8_to_16(style->r1,user_gamma/sRGB_gamma),
		apply_gamma_single_8_to_16(style->g1,user_gamma/sRGB_gamma),
		apply_gamma_single_8_to_16(style->b1,user_gamma/sRGB_gamma)
	);
        if (display_optimize){
                bitmap.x/=3;
		decimate_3(&primary_data,bitmap.x,bitmap.y);
        }

        fprintf(printer->file,"/picstr %d string def\n",bitmap.x);
        fprintf(printer->file,"%g %g translate\n",x0,y0);
        fprintf(printer->file,"/picstr %d string def\n");
        mem_free(bitmap.data);
	mem_free(primary_data);
        return bitmap.x;
}

void ps_print_text(struct ps_printer *printer, int x0, int y0, struct style *style, unsigned char *text)
{
        int code;
        double x=printer->coeff*x0+printer->page_leftmargin;
        double y=printer->page_height-printer->page_topmargin-printer->coeff*(y0-printer->clip_y_min)-style->height;
        while(*text){
                GET_UTF_8(text,code);
                x+=ps_print_char(printer,x,y,style,code);
        }
}
*/

void print_g_object(struct g_object *obj,int x0,int y0, void *data)
{
        struct ps_printer *printer=(struct ps_printer*)data;

        struct g_object_text  *text =(struct g_object_text  *)obj;
        struct g_object_tag   *tag  =(struct g_object_tag   *)obj;
        struct g_object_image *image=(struct g_object_image *)obj;
        int xw=obj->xw;
        int yw=obj->yw;

        /* Translate our coords to page ones */
        double height=yw*printer->coeff;
        double width=xw*printer->coeff;
        double x=printer->coeff*x0+printer->page_leftmargin;
        double y=printer->page_height-printer->page_topmargin-printer->coeff*(y0-printer->clip_y_min)-height;

        /* Filter out-of-page objects */
        /* Determine next page extent */

        if(y0<printer->clip_y_min) return;
        if(y0+yw>printer->clip_y_max){
                if(y0<=printer->clip_y_max && obj->type==G_OBJECT_TEXT)
                        if(printer->clip_y_max-y0 >= printer->extent){
                                printer->extent=printer->clip_y_max-y0+1;
                        }
                return;
        }

        switch(obj->type){
        /*
        case(G_OBJECT_TEXT):
                {
                        ps_print_text(printer,x0,y0,text->style,text->text);
                        return;
                }
        */
        default:
                if(xw>0 && yw>0){
                        //PDF_rect(printer->p,x,y,width,height);
                        //PDF_stroke(printer->p);
                        fprintf(printer->file,"newpath\n");
                        fprintf(printer->file,"%g %g moveto\n",x,y);
                        fprintf(printer->file,"%g %g lineto\n",x+width,y);
                        fprintf(printer->file,"%g %g lineto\n",x+width,y+height);
                        fprintf(printer->file,"%g %g lineto\n",x,y+height);
                        fprintf(printer->file,"closepath\nstroke\n");
                }
        }
}

struct ps_printer *init_printer(unsigned char *filename, int xw, int yw)
{
        struct ps_printer *printer=mem_alloc(sizeof(struct ps_printer));
        struct rect r;

        /* open new file */
        printer->file=fopen(filename,"w");
        if(!printer->file)
                return NULL;

        /* A4 ? */
        printer->page_width=595;
        printer->page_height=840;

        printer->page_leftmargin=printer->page_width*0.05;
        printer->page_rightmargin=printer->page_width*0.05;
        printer->page_topmargin=printer->page_height*0.05;
        printer->page_bottommargin=printer->page_height*0.05;


        printer->coeff=(printer->page_width-printer->page_leftmargin-printer->page_rightmargin)/xw;

        return printer;
}

void shutdown_printer(struct ps_printer *printer)
{
        if(!printer) return;
        fclose(printer->file);
        mem_free(printer);
}

void print_graphical_doc(struct f_data_c *fd, unsigned char *filename)
{
        struct session *ses=fd->ses;
        struct f_data *f=fd->f_data;
        struct g_object_area *root=(struct g_object_area*)f->root;
        int n_line=0;
        int xw=root->xw;
        int yw=root->yw;
        struct ps_printer *printer;

        printer=init_printer(filename,xw,yw);

        if(!printer) return;

        printer->clip_y_min=0;
        printer->clip_y_max=(printer->page_height-printer->page_topmargin-printer->page_bottommargin)/printer->coeff;

        while(printer->clip_y_min<yw){
                printer->extent=0;
                g_object_do_recursive((struct g_object*)root,0,0,print_g_object,(void*)printer);

                fprintf(printer->file,"showpage\n");
                printer->clip_y_min+=(printer->page_height-printer->page_topmargin-printer->page_bottommargin)/printer->coeff-printer->extent;
                printer->clip_y_max+=(printer->page_height-printer->page_topmargin-printer->page_bottommargin)/printer->coeff-printer->extent;
        }

        shutdown_printer(printer);
}

#endif

void print_to_file(struct session *ses, unsigned char *file)
{
        /* FIXME: We need this for correct (xy.z) floats formatting */
        setlocale(LC_NUMERIC,"C");

        if(!F)
                msg_box(ses->term, NULL,
                        TXT(T_ERROR), AL_CENTER,
                        "Can't print in text mode.",
                        NULL, 1,
                        TXT(T_CANCEL), NULL, B_ENTER | B_ESC);
#ifdef G
        else
                print_graphical_doc(ses->screen,file);
#endif
}

