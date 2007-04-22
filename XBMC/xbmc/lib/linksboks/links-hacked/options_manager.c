/* Options manager stuff */

#include "links.h"

void *options_new_item(void *);
void  options_edit_item(struct dialog_data *,void *,void (*)(struct dialog_data *,void *,void *,struct list_description *),void *, unsigned char);
void  options_delete_item(void *);
void  options_copy_item(void *,void *);
unsigned char *options_type_item(struct terminal *, void *, int);
void *options_default_value(struct session*, unsigned char);

struct list options = { &options, &options, 0, -1, NULL };

struct list_description options_ld={
	1,  /* 0= flat; 1=tree */
	&options,  /* list */
	options_new_item,
	options_edit_item,
        0,
	options_delete_item,
	options_copy_item,
        options_type_item,

        1,0,0,0, /* User can only edit items, but not add, delete or move */

        0,   /* this is set in init_options function */
	60,  /* width of main window */
	15,  /* # of items in main window */
	T_OPTIONS_ITEM,
	T_OPTIONS_ALREADY_IN_USE,
	T_OPTIONS_MANAGER,
	0,
	0,	/* no button */
	NULL,	/* no button */

        NULL,   /* no close hook */

	0,0,0,0,  /* internal vars */
	0 /* modified */
};

struct select_str {
        struct terminal *term;
        struct dialog_data *dlg;
        void (*ok_fn)(struct dialog_data *, void *, void *, struct list_description *);
        void *ok_arg;
        struct options *item;
};

struct optionS_OKAY_struct{
	void (*fn)(struct dialog_data *,void *,void *,struct list_description *);
	void *data;	
	struct dialog_data *dlg;
};


void *options_new_item(void *ignore)
{
        struct options *new;

        ignore = ignore;

        new=mem_alloc(sizeof(struct options));
        if(!new)
                return NULL;
        new->name = stracpy("");
        new->title = NULL;
        new->value = stracpy("");
        new->type = 0;
        new->change_hook = NULL;

        return new;
}

void options_copy_item(void *in, void *out)
{
        struct options *item_in = (struct options*)in;
        struct options *item_out = (struct options*)out;
                 
	if(!item_in) return;
	if(!item_out) return;

        if(item_out->name)
                mem_free(item_out->name);
        item_out->name = stracpy(item_in->name);

        item_out->title = item_in->title;

        if(item_out->value)
                mem_free(item_out->value);
        item_out->value = stracpy(item_in->value);

        item_out->type = item_in->type;
        item_out->opt_type = item_in->opt_type;
        item_out->change_hook = item_in->change_hook;
}

/* Doesn't delete from list */
void options_free_item(struct options *item)
{
        if (item){
                if(item->name) mem_free(item->name);
                if(item->value) mem_free(item->value);
                mem_free(item);
        }
}

void options_delete_item(void *data)
{
	struct options* item=(struct options*)data;
	struct options *prev=item->prev;
	struct options *next=item->next;

        if ((struct list*)data==&options)  /* empty list or head */
                return;
        if(!list_empty(*item)){
                if (next)
                        next->prev=item->prev;
                if (prev)
                        prev->next=item->next;
        }
	options_free_item(item);
}


/* allocate string and print item into it */
/* x: 0=type all, 1=type title only */
unsigned char *options_type_item(struct terminal *term, void *data, int x)
{
	unsigned char *txt;
	struct conv_table *table;
	struct options* item=(struct options*)data;
        int l=0;

        /* First item */
        if ((struct list*)item==(&options))
                return stracpy(_(TXT(T_OPTIONS),term));

        txt=init_str();
        if(item->title) add_to_str(&txt,&l,_(item->title,term));
        if(item->type==0){
                switch(item->opt_type){
                case OPT_TYPE_CHAR:
                        add_to_str(&txt,&l," (char) ");
                        break;
                case OPT_TYPE_INT:
                        add_to_str(&txt,&l," (int) ");
                        break;
                case OPT_TYPE_BOOL:
                        add_to_str(&txt,&l," : ");
                        add_to_str(&txt,&l,(*item->value=='1')
                                   ? _(TXT(T_YES),term)
                                   : _(TXT(T_NO),term));
                        goto already_drawn;
                case OPT_TYPE_DOUBLE:
                        add_to_str(&txt,&l," (double) ");
                        break;
                case OPT_TYPE_RGB:
                        add_to_str(&txt,&l," (rgb) ");
                        break;
                case OPT_TYPE_FONT:
                        add_to_str(&txt,&l," (font) ");
                        break;
                }
                add_to_str(&txt,&l,": ");
                if(item->value) add_to_str(&txt,&l,item->value);
        already_drawn:
				l=l;
        }
        return txt;
}

void options_edit_item_fn(struct dialog_data *dlg)
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

        /* if (w > term->x - 2 * DIALOG_LB) w = term->x - 2 * DIALOG_LB; */
	if (w < 1) w = 1;
        rw = w;
        /* w = rw = gf_val(50,30*G_BFU_FONT_SIZE); */

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

int options_change_hook(struct session *ses,
                        struct options *current,
                        struct options *changed)
{
	/* Let's try user-provided hook */
	if(!current->change_hook ||
	   current->change_hook(ses, current, changed))
		return 1;
	else
		return 0;
}

void options_edit_done(void *data)
{
	struct dialog *d = (struct dialog*)data;
        struct optionS_OKAY_struct* s = (struct optionS_OKAY_struct*)d->udata2;
	unsigned char *value = (unsigned char *)&d->items[4];
	struct options *changed = (struct options *)d->udata;
        struct options *current = (struct options *)s->data;

        if(changed->value)
                mem_free(changed->value);
        changed->value = stracpy(value);

	if(options_change_hook(s->dlg->dlg->udata, current, changed))
                s->fn(s->dlg, current, changed, &options_ld);

        d->udata=0;  /* for abort function */
}

/* destroys an item, this function is called when edit window is aborted */
void options_edit_abort(struct dialog_data *data)
{
	struct options *item=(struct options*)data->dlg->udata;
	struct dialog *dlg=data->dlg;

        if(dlg->udata2) mem_free(dlg->udata2);
        options_free_item(item);
}


/* Font selection callbacks */
void font_sel(struct terminal *term, struct font *font, struct select_str *sel)
{
        if(sel->item->value)
                mem_free(sel->item->value);
        sel->item->value=stracpy(font->family);
        sel->ok_fn(sel->dlg,sel->ok_arg,sel->item,&options_ld);
}

void font_sel_list(struct select_str *sel)
{
	int i, selected;
	unsigned char *n;
	struct menu_item *mi;
        struct font *font;

        mi = new_menu(1);

        if (!mi)
                return;

        selected=0;
        i=0;

        foreach(font,fontlist){
                int d;
                int found=0;
                for(d=0;mi[d].text;d++)
                        if(!strcmp(mi[d].text,font->family))
                                found=1;
                if(!strcmp(font->family,"system"))
                        found=1; /* Exclude our SYSTEM font */
                if(!found){
                        add_to_menu(&mi, font->family, font->spacing, "", MENU_FUNC font_sel, (void *)font, 0);
                        if(!strcmp(sel->item->value,font->family))
                                selected = i;
                        i++;
                }
        }

        do_menu_selected(sel->term, mi, sel, selected);
}

void options_edit_item(struct dialog_data *dlg, void *data, void (*ok_fn)(struct dialog_data *, void *, void *, struct list_description *), void *ok_arg, unsigned char dlg_title)
{
	struct options *new=(struct options*)data;
	struct terminal *term=dlg->win->term;
	struct optionS_OKAY_struct *s;
	struct dialog *d;
	unsigned char *value;

        if(new->type)
		{
			struct event ev = { EV_KBD, ' ', 0, 0 };
			list_event_handler(dlg, &ev);
            return;
		}

        switch(new->opt_type){
        /* Toggle boolean */
        case OPT_TYPE_BOOL:{
                int val=options_get_bool(new->name);

		if(new->value)
                        mem_free(new->value);
		new->value=stracpy(val?"0":"1");
                if(options_change_hook(dlg->dlg->udata,ok_arg,new))
			ok_fn(dlg,ok_arg,new,&options_ld);
		return;
	}
        /* Choose font family */
        case OPT_TYPE_FONT:{
                struct select_str *sel=mem_alloc(sizeof(struct select_str));

                sel->term=term;
                sel->dlg=dlg;
                sel->ok_fn=ok_fn;
                sel->ok_arg=ok_arg;
                sel->item=new;
                add_to_ml(&dlg->ml, sel, NULL);
                font_sel_list(sel);
                return;
        }

        /* Generic option */
        default:

                if (!(d = mem_alloc(sizeof(struct dialog) + 4 * sizeof(struct dialog_item) + MAX_STR_LEN))) return;
                memset(d, 0, sizeof(struct dialog) + 4 * sizeof(struct dialog_item) + MAX_STR_LEN);

                value=(unsigned char *)&d->items[4];
                if (new->value)strncpy(value, new->value, MAX_STR_LEN);

                /* Create the dialog */
                if (!(s=mem_alloc(sizeof(struct optionS_OKAY_struct))))return;
                s->fn=ok_fn;
                s->data=ok_arg;
                s->dlg=dlg;

                switch (dlg_title){
                case TITLE_EDIT:
                        if(new->title)
                                d->title=_(new->title,term);
                        else
                                d->title="";
                        break;

                default:
                        internal("Unsupported dialog title.\n");
                }

                d->udata=data;
                d->udata2=s;
                d->abort=options_edit_abort;
                d->refresh=options_edit_done;
                d->refresh_data = d;
                d->title = _(new->title,term);/* TXT(T_OPTIONS_ITEM); */
                d->fn = options_edit_item_fn;
                d->items[0].type = D_FIELD;
                d->items[0].dlen = MAX_STR_LEN;
                d->items[0].data = value;
                d->items[0].fn = NULL; /* check_nonempty; */
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
}

void menu_options_manager(struct terminal *term,void *fcp,struct session *ses)
{
        create_list_window(&options_ld,&options,term,ses);
}

