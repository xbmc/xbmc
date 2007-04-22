#include "links.h"

#ifdef G

void add_to_selection(struct selection *sel,unsigned char *text)
{
        if(!text || !sel)
                return;

        if(!sel->text){
                sel->text=init_str();
                sel->length=0;

        }

        {
                unsigned char *txt=convert_string(sel->conv, text, strlen(text), NULL);
                add_to_str(&sel->text,&sel->length,txt);
                mem_free(txt);
        }
}

void selection_clear(struct g_object *obj, int x0, int y0, void *data)
{
        struct g_object_text *text=(struct g_object_text *)obj;
        struct f_data_c *fd=(struct f_data_c *)data;
        struct selection *sel=fd?fd->selection:NULL;

        if(obj->type!=G_OBJECT_TEXT)
                return;
        text->selected=0;
        text->selection_start=NULL;
        text->selection_end=NULL;
        if(sel)
                draw_one_object(sel->fd,obj);
}

void selection_start(struct f_data_c *fd, long x, long y)
{
        fd->selection = mem_alloc(sizeof(struct selection));
        fd->selection->fd=fd;
        fd->selection->x1=x;
        fd->selection->y1=y;
        fd->selection->text=NULL;
        fd->selection->prev=NULL;
        fd->selection->conv = get_translation_table(get_cp_index("UTF-8"), /* Internal charset is utf-8 */
                                                    options_get_cp("text_selection_clipboard_charset"));
        fd->selection->rectangular_mode = options_get_int("text_selection_rectangular_mode");
        fd->selection->length=0;
}

void selection_put_to_clipboard(struct f_data_c *fd)
{
        struct selection *sel=fd->selection;

        if(!sel)
                return;

        if(sel->text){
                fd->ses->term->dev->drv->put_to_clipboard(fd->ses->term->dev,sel->text,sel->length);
                mem_free(sel->text);
        }
        mem_free(sel);
        fd->selection=NULL;
}

void sort(long *x1,long *x2)
{
        if(*x1>*x2){
                long t=*x1;
                *x1=*x2;
                *x2=t;
        }
}

void exchange(long *x1, long *x2)
{
        long t=*x1;
        *x1=*x2;
        *x2=t;
}

void selection_end(struct g_object *obj, int x0, int y0, void *data)
{
        struct g_object_text *text=(struct g_object_text *)obj;
        struct f_data_c *fd=(struct f_data_c *)data;
        struct selection *sel=fd->selection;

        if(obj->type!=G_OBJECT_TEXT || !sel)
                return;

        if(text->selected){
                unsigned char *cur=text->selection_start;
                unsigned char *ch=init_str();
                int ll=0;

                if(sel->prev && sel->prev->parent!=obj->parent && cur<text->selection_end)
                        add_chr_to_str(&ch,&ll,'\n');

                while(cur<text->selection_end)
                        add_chr_to_str(&ch,&ll,*(cur++));

                add_to_selection(sel,ch);
                if(ch) mem_free(ch);
                sel->prev=obj;
        }
}

int g_pos_in_str(unsigned char *text, struct style *style, int x0)
{
        unsigned char *text_orig=text;
        int x=0;

        while(*text && x<x0){
                int ch;
                GET_UTF_8(text,ch);
                x+=g_char_width(style,ch);
        }
        return text-text_orig;
}

/* x0, y0 - coordinates of text element */
void selection_show(struct g_object *obj, int x0, int y0, void *data)
{
        struct g_object_text *text = (struct g_object_text *)obj;
        struct f_data_c *fd = (struct f_data_c *)data;
        struct selection *sel = fd->selection;
        long x1,y1,x2,y2;
        int inside=0;

        if(obj->type != G_OBJECT_TEXT || !sel)
                return;

        /* Form element? */

        if(text->link_num>0){
                struct link *link=fd->f_data->links+text->link_num;
                if (link && link->form && link->form->type )
                        return;
        }

        x1=sel->x1;
        y1=sel->y1;
        x2=sel->x;
        y2=sel->y;

        if(y1 > y2){
                exchange(&x1,&x2);
                exchange(&y1,&y2);
        }

        inside=((y0 <= y2) && (y0+obj->yw >= y1));
        if(sel->rectangular_mode){
                long xx1=x1;
                long xx2=x2;
                sort(&xx1,&xx2);
                if((x0 >= xx2) || (x0+obj->xw <= xx1))
                        inside=0;
        }
        if(inside){
                text->selected=1;
                text->selection_start = text->text;
                text->selection_end = text->text + strlen(text->text);

                if(y0+obj->yw > y2){ /* Selection ends in the middle of the word */
                        text->selection_end = text->text+g_pos_in_str(text->text,text->style,x2-x0);
                }
                if(y0 <= y1){         /* Selection begins in the middle of the word */
                        int pos = g_pos_in_str(text->text, text->style, x1-x0);
                        text->selection_start = text->text+pos;
                        if(text->selection_start > text->selection_end){
                                unsigned char *t = text->selection_start;
                                text->selection_start = text->selection_end;
                                text->selection_end = t;
                        }
                        if(*text->selection_start)
                                BACK_UTF_8(text->selection_start,text->text);
                        if(*text->selection_end)
                                BACK_UTF_8(text->selection_end,text->text);
                }
                draw_one_object(sel->fd,obj);
        }
        else if(text->selected){
                text->selected=0;
                draw_one_object(sel->fd,obj);
        }
}

/* mode=0 - start, mode=1 - show, mode=2 - end */

void mouse_selection(struct f_data_c *fd, int x, int y, int mode)
{
        struct selection *sel=fd->selection;
	struct rect clip;
        struct terminal *term=fd->ses->term;

        switch(mode){
        case 0: /* Start selection */
                selection_start(fd,x,y);
                g_object_do_recursive(fd->f_data->root,0,0,selection_clear,fd);
                return;
        case 1: /* Show selection */
                sel->x=x;
                sel->y=y;
                sel->working=1;
                restrict_clip_area(term->dev, &clip, fd->xp, fd->yp, fd->xp + fd->xw - fd->f_data->vsb * G_SCROLL_BAR_WIDTH, fd->yp + fd->yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH);
                g_object_do_recursive(fd->f_data->root,0,0,selection_show,fd);
                term->dev->drv->set_clip_area(term->dev, &clip);
                return;
        case 2: /* Finish selection */
                sel->x2=x;
                sel->y2=y;
                g_object_do_recursive(fd->f_data->root,0,0,selection_end,fd);
                selection_put_to_clipboard(fd);
        }

}

#endif
