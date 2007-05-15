/* Font manager code */

#include "links.h"

void fontlist_edit_item(struct dialog_data *,void *,void (*)(struct dialog_data *,void *,void *,struct list_description *),void *, unsigned char);
void fontlist_copy_item(struct font *in, struct font *out);
void *fontlist_default_value(struct session*, unsigned char);
void *fontlist_new_item(void *);
void fontlist_delete_item(void *);
unsigned char *fontlist_type_item(struct terminal *, void *, int);

struct list_description fontlist_ld={
	0,  /* 0= flat; 1=tree */
	&fontlist,  /* list */
	fontlist_new_item,
	fontlist_edit_item,
        fontlist_default_value,
	fontlist_delete_item,
	fontlist_copy_item,
        fontlist_type_item,

        0,1,1,1, /* User can move, delete and add items */

        0,   /* this is set in init_fontlist function */
	60,  /* width of main window */
	20,  /* # of items in main window */
	T_FONTLIST_ITEM,
	T_FONTLIST_ALREADY_IN_USE,
	T_FONTLIST_MANAGER,
	T_FONTLIST_DELETE,
	0,	/* no button */
	NULL,	/* no button */

        NULL,   /* no close hook */

	0,0,0,0,  /* internal vars */
	0 /* modified */
};

struct fontlist_ok_struct{
	void (*fn)(struct dialog_data *,void *,void *,struct list_description *);
	void *data;	
	struct dialog_data *dlg;
};

void fontlist_copy_item(struct font *in, struct font *out)
{
        if(out->family) mem_free(out->family);
        out->family=stracpy(in->family);

        if(out->weight) mem_free(out->weight);
        out->weight=stracpy(in->weight);

        if(out->slant) mem_free(out->slant);
        out->slant=stracpy(in->slant);

        if(out->adstyl) mem_free(out->adstyl);
        out->adstyl=stracpy(in->adstyl);

        if(out->filename) mem_free(out->filename);
        out->filename=stracpy(in->filename);

        if(out->spacing) mem_free(out->spacing);
        out->spacing=stracpy(in->spacing);

        out->font_type=in->font_type;
        out->n_letters=in->n_letters;
        out->letter=in->letter;
        out->data=in->data;

        out->get_char_metric=in->get_char_metric;
        out->get_char=in->get_char;
        out->free_font=in->free_font;
        out->index_font=in->index_font;
}

void *fontlist_default_value(struct session *ses, unsigned char type)
{
        return NULL;
}

void *fontlist_new_item(void *data)
{
        return create_font(NULL,NULL,NULL,NULL,NULL);
}

void fontlist_delete_item(void *data)
{
        struct font *del=(struct font *)data;
	struct font *next=del->next;
	struct font *prev=del->prev;

        if(del->family) mem_free(del->family);
        if(del->weight) mem_free(del->weight);
        if(del->slant) mem_free(del->slant);
        if(del->adstyl) mem_free(del->adstyl);
        if(del->filename) mem_free(del->filename);
        if(del->spacing) mem_free(del->spacing);

        if (next) next->prev=del->prev;
	if (prev) prev->next=del->next;
	mem_free(del);
        recreate_style_tables();
}

unsigned char *fontlist_type_item(struct terminal *term, void *data, int x)
{
        unsigned char *txt;
        struct font *font=(struct font*)data;
        int l=0;

        if ((struct list*)data==(&fontlist))return stracpy(_(TXT(T_FONTLIST),term));

        txt=init_str();
        switch(font->font_type){
        case FONT_TYPE_BUILTIN:
                add_to_str(&txt,&l,"builtin");
                break;
        case FONT_TYPE_FREETYPE:
                add_to_str(&txt,&l,"freetype");
                break;
#ifdef XBOX_USE_XFONT
		case FONT_TYPE_XFONT:
				add_to_str(&txt,&l,"xfont");
				break;
#endif
        default:
                add_to_str(&txt,&l,"unknown");
        }
        add_to_str(&txt,&l,": ");

        if(font->family) add_to_str(&txt,&l,font->family);
        add_to_str(&txt,&l,"-");
        if(font->weight) add_to_str(&txt,&l,font->weight);
        add_to_str(&txt,&l,"-");
        if(font->slant) add_to_str(&txt,&l,font->slant);
        add_to_str(&txt,&l,"-");
        if(font->adstyl) add_to_str(&txt,&l,font->adstyl);
        add_to_str(&txt,&l,"-");
        if(font->spacing) add_to_str(&txt,&l,font->spacing);
        add_to_str(&txt,&l,", ");
        add_num_to_str(&txt,&l,font->n_letters);
        add_to_str(&txt,&l," letters");
        return txt;
}

void menu_fontlist_manager(struct terminal *term,void *fcp,struct session *ses)
{
        create_list_window(&fontlist_ld,&fontlist,term,ses);
}


void fontlist_edit_item_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
        struct dialog *d=dlg->dlg;
        int max = 0, min = 0;
	int w, rw;
	int y = gf_val(-1, -G_BFU_FONT_SIZE);
	max_text_width(term, d->title, &max, AL_LEFT);
	min_text_width(term, d->title, &min, AL_LEFT);
	max_buttons_width(term, dlg->items + 1, 2, &max);
	min_buttons_width(term, dlg->items + 1, 2, &min);
	if (max < dlg->dlg->items->dlen) max = dlg->dlg->items->dlen;
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
        if (w > max) w = max;
	if (w < min) w = min;
	if (w > term->x - 2 * DIALOG_LB) w = term->x - 2 * DIALOG_LB;
	if (w < 1) w = 1;
	rw = 0;
	y += gf_val(2, G_BFU_FONT_SIZE * 2);
	dlg_format_buttons(dlg, NULL, dlg->items + 1, 2, 0, &y, w, &rw, AL_CENTER);
	if(rw>w) w = rw;
	dlg->xw = w + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB;
	dlg_format_field(dlg, term, &dlg->items[0], dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, term, &dlg->items[1], 2, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
}

void fontlist_edit_done(void *data)
{
	struct dialog *d=(struct dialog*)data;
	struct font *item=(struct font *)d->udata;
        struct fontlist_ok_struct *s=(struct fontlist_ok_struct*)d->udata2;
	unsigned char *value=(unsigned char *)&d->items[4];
        struct font *font=NULL;

        fontlist_delete_item(item);

#ifdef HAVE_FREETYPE
        if(!font)
                font=create_ft_font(value);
#endif
#ifdef XBOX_USE_XFONT
        if(!font)
                font=create_xfont_font(value);
#endif
        if(font){
                font->index_font(font);
                s->fn(s->dlg,s->data,font,&fontlist_ld);
                n_fonts=compute_n_fonts();
        }
        d->udata=0;  /* for abort function */
}

/* destroys an item, this function is called when edit window is aborted */
void fontlist_edit_abort(struct dialog_data *data)
{
	struct font *item=(struct font*)data->dlg->udata;
	struct dialog *dlg=data->dlg;

        if (dlg->udata2) mem_free(dlg->udata2);
        if (item)
                fontlist_delete_item(item);
}

void fontlist_edit_item(struct dialog_data *dlg, void *data, void (*ok_fn)(struct dialog_data *, void *, void *, struct list_description *), void *ok_arg, unsigned char dlg_title)
{
	struct font *new=(struct font*)data;
	struct terminal *term=dlg->win->term;
	struct fontlist_ok_struct *s;
	struct dialog *d;
	unsigned char *value;

        if(new->type || dlg_title!=TITLE_ADD)
                return;

        if (!(d = mem_alloc(sizeof(struct dialog) + 4 * sizeof(struct dialog_item) + MAX_STR_LEN))) return;
        memset(d, 0, sizeof(struct dialog) + 4 * sizeof(struct dialog_item) + MAX_STR_LEN);


        value=(unsigned char *)&d->items[4];
        /*
        if (new->value) strncpy(value, new->value, MAX_STR_LEN);
        */
        snprintf(value,MAX_STR_LEN,"");

        /* Create the dialog */
        if (!(s=mem_alloc(sizeof(struct fontlist_ok_struct))))return;
        s->fn=ok_fn;
        s->data=ok_arg;
        s->dlg=dlg;

        /* FIXME */
        d->title=stracpy("Enter font file path");

        d->udata=data;
        d->udata2=s;
        d->abort=fontlist_edit_abort;
        d->refresh=fontlist_edit_done;
        d->refresh_data = d;
        d->fn = fontlist_edit_item_fn;
        d->items[0].type = D_FIELD;
        d->items[0].dlen = MAX_STR_LEN;
        d->items[0].data = value;
        d->items[0].fn = check_nonempty;
        d->items[1].type = D_BUTTON;
        d->items[1].gid = B_ENTER;
        d->items[1].fn = ok_dialog;
        d->items[1].text = TXT(T_OK);
        d->items[2].type = D_BUTTON;
        d->items[2].gid = B_ESC;
        d->items[2].text = TXT(T_CANCEL);
        d->items[2].fn = cancel_dialog;
        d->items[3].type = D_END;

        do_dialog(term, d, getml(d, NULL));
}
