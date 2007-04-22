#include "links.h"

/*------------------------ BLOCKLIST -----------------------*/

/* DECLARATIONS */
void blocklist_edit_item(struct dialog_data *,void *,void (*)(struct dialog_data *,void *,void *,struct list_description *),void *, unsigned char);
void blocklist_copy_item(void *, void *);
void *blocklist_default_value(struct session*, unsigned char);
void *blocklist_new_item(void *);
void blocklist_delete_item(void *);
void save_blocklist();
void load_blocklist();
unsigned char *blocklist_type_item(struct terminal *, void *, int);

struct list blocklist = { &blocklist, &blocklist, 0, -1, NULL };

struct blocklist_ok_struct{
	void (*fn)(struct dialog_data *,void *,void *,struct list_description *);
	void *data;	
	struct dialog_data *dlg;
};

struct list_description blocklist_ld={
	0,  /* 0= flat; 1=tree */
	&blocklist,  /* list */
	blocklist_new_item,
	blocklist_edit_item,
	blocklist_default_value,
	blocklist_delete_item,
	blocklist_copy_item,
	blocklist_type_item,

        1,0,1,1,                  /* User can edit, delete and add, but not move items */

	0,		/* this is set in init_blocklist function */
	40,  /* width of main window */
	8,  /* # of items in main window */
	T_BLOCKLIST_ITEM,
	T_BLOCKLIST_ALREADY_IN_USE,
	T_BLOCKLIST_MANAGER,
	T_DELETE_BLOCKLIST_ITEM,
	0,	/* no button */
	NULL,	/* no button */

        NULL,   /* no close hook */

	0,0,0,0,  /* internal vars */
	0 /* modified */
};

void *blocklist_default_value(struct session* ses, unsigned char type)
{
    return NULL;
}

void *blocklist_new_item(void *ignore)
{
        struct blocklist *new;

	ignore=ignore;
        if(!(new=mem_alloc(sizeof(struct blocklist)))) return NULL;
        new->pattern = stracpy("");
        new->type=0;
        return new;
}

void blocklist_delete_item(void *data)
{
        struct blocklist *del=(struct blocklist *)data;
	struct blocklist *next=del->next;
	struct blocklist *prev=del->prev;

        if (del->pattern)mem_free(del->pattern);
	if (next)next->prev=del->prev;
	if (prev)prev->next=del->next;
	mem_free(del);
        save_blocklist();
}


void blocklist_copy_item(void *in, void *out)
{
	struct blocklist *item_in=(struct blocklist *)in;
	struct blocklist *item_out=(struct blocklist *)out;

	if (item_out->pattern)mem_free(item_out->pattern);

	item_out->pattern=stracpy(item_in->pattern);
        save_blocklist();
}


/* allocate string and print item into it */
/* x: 0=type all, 1=type title only */
unsigned char *blocklist_type_item(struct terminal *term, void *data, int x)
{
	unsigned char *txt, *txt1;
	struct conv_table *table;
	struct blocklist* item=(struct blocklist*)data;

	if ((struct list*)item==(&blocklist))return stracpy(_(TXT(T_BLOCKLIST),term));
        txt=stracpy(item->pattern);
        table=get_translation_table(blocklist_ld.codepage,term->spec->charset);
	txt1=convert_string(table,txt,strlen(txt),NULL);
	mem_free(txt);

        return txt1;
}

unsigned char *blocklist_msg =  TXT(T_BLOCKLIST_ITEM);

void blocklist_edit_item_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, rw;
	int y = gf_val(-1, -G_BFU_FONT_SIZE);
	max_text_width(term, blocklist_msg, &max, AL_LEFT);
	min_text_width(term, blocklist_msg, &min, AL_LEFT);
	max_buttons_width(term, dlg->items + 1, 2, &max);
	min_buttons_width(term, dlg->items + 1, 2, &min);
	if (max < dlg->dlg->items->dlen) max = dlg->dlg->items->dlen;
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
        /* if (w > term->x - 2 * DIALOG_LB) w = term->x - 2 * DIALOG_LB; */
	if (w < 1) w = 1;
	rw = w;
	y += gf_val(2, G_BFU_FONT_SIZE * 2);
	dlg_format_buttons(dlg, NULL, dlg->items + 1, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = w + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB;
	dlg_format_field(dlg, term, &dlg->items[0], dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, term, &dlg->items[1], 2, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
}

void blocklist_edit_done(void *data)
{
	struct dialog *d=(struct dialog*)data;
	struct blocklist *item=(struct blocklist *)d->udata;
        struct blocklist_ok_struct* s=(struct blocklist_ok_struct*)d->udata2;
	unsigned char *txt;
	struct conv_table *table;
	unsigned char *pattern;

	pattern=(unsigned char *)&d->items[4];

	table=get_translation_table(s->dlg->win->term->spec->charset,blocklist_ld.codepage);
	txt=convert_string(table,pattern,strlen(pattern),NULL);
        if(item->pattern)mem_free(item->pattern);
        item->pattern=txt;

        s->fn(s->dlg,s->data,item,&blocklist_ld);
	d->udata=0;  /* for abort function */
        save_blocklist();
}

/* destroys an item, this function is called when edit window is aborted */
void blocklist_edit_abort(struct dialog_data *data)
{
	struct blocklist *item=(struct blocklist*)data->dlg->udata;
	struct dialog *dlg=data->dlg;

        mem_free(dlg->udata2);
        if (item)blocklist_delete_item(item);
}


void blocklist_edit_item(struct dialog_data *dlg, void *data, void (*ok_fn)(struct dialog_data *, void *, void *, struct list_description *), void *ok_arg, unsigned char dlg_title)
{
	struct blocklist *new=(struct blocklist*)data;
	struct terminal *term=dlg->win->term;
	struct blocklist_ok_struct *s;
	struct dialog *d;
	unsigned char *pattern;

        if (!(d = mem_alloc(sizeof(struct dialog) + 4 * sizeof(struct dialog_item) + MAX_STR_LEN))) return;
        memset(d, 0, sizeof(struct dialog) + 4 * sizeof(struct dialog_item) + MAX_STR_LEN);

	pattern=(unsigned char *)&d->items[4];
	if (new->pattern)strncpy(pattern, new->pattern, MAX_STR_LEN);

	/* Create the dialog */
	if (!(s=mem_alloc(sizeof(struct blocklist_ok_struct))))return;
        s->fn=ok_fn;
	s->data=ok_arg;
	s->dlg=dlg;
		
	switch (dlg_title)
	{
		case TITLE_EDIT:
                        d->title=TXT(T_EDIT_BLOCKLIST_ITEM);
		break;

		case TITLE_ADD:
                        d->title=TXT(T_ADD_BLOCKLIST_ITEM);
		break;

		default:
		internal("Unsupported dialog title.\n");
	}

	d->udata=data;
	d->udata2=s;
	d->abort=blocklist_edit_abort;
	d->refresh=blocklist_edit_done;
	d->refresh_data = d;
        d->title = TXT(T_BLOCKLIST_ITEM);
	d->fn = blocklist_edit_item_fn;
	d->items[0].type = D_FIELD;
	d->items[0].dlen = MAX_STR_LEN;
	d->items[0].data = pattern;
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

void add_to_blocklist(unsigned char *pattern)
{
        struct blocklist *repl;

	if (!pattern ||
	    !*pattern)
		return;

        repl = mem_alloc(sizeof(struct blocklist));
	if (!repl)
		return;

	add_to_list(blocklist, repl);
	repl->pattern = stracpy(pattern);
	repl->type=0;
}

int is_in_blocklist(unsigned char *url)
{
    int result=0;
    int d=0;
    struct blocklist *item;

    if(!url ||
       list_empty(blocklist)) return 0;

    foreach(item,blocklist)
            if(strstr(url,item->pattern))
                    result=1;

    return result;
}

void free_blocklist()
{
        struct blocklist *b;

        if(list_empty(blocklist))
                return;

        foreach(b, blocklist)
		if(b->pattern)
			mem_free(b->pattern);

        free_list(blocklist);
}

void save_blocklist()
{
	FILE *file;
	struct blocklist *item;
        unsigned char *blocklist_file;

        if(list_empty(blocklist))
                return;

        blocklist_file = stracpy (links_home) ;

        if (!blocklist_file)
		return ;

        add_to_strn(&blocklist_file, "blocklist");

        file = fopen(blocklist_file, "w");
        mem_free(blocklist_file);

        if (!file){
		mem_free(blocklist_file) ;
		return;
	}

	foreach(item, blocklist)
                if(item->pattern)
			fprintf(file, "%s\n", item->pattern);

	fclose(file);
}

void load_blocklist()
{
	FILE *file;
        unsigned char *blocklist_file;

	blocklist_file = stracpy (links_home) ;

        if (!blocklist_file)
		return ;

        add_to_strn(&blocklist_file, "blocklist");

        file = fopen(blocklist_file, "r");
	if (!file){
		mem_free(blocklist_file) ;
		return;
	}

        init_list(blocklist);

	while(!feof(file)){
                unsigned char *pattern = mem_alloc(MAX_STR_LEN);

                fscanf(file, "%s\n", pattern);

		add_to_blocklist(pattern);
		mem_free(pattern);
	}
        mem_free(blocklist_file);
	fclose(file);

	blocklist.type=0;
	blocklist.depth=-1;
	blocklist.fotr=NULL;

        reinit_list_window(&blocklist_ld);
}

void reload_blocklist()
{
        free_blocklist();
        load_blocklist();
}

void menu_blocklist_manager(struct terminal *term,void *fcp,struct session *ses)
{
        reload_blocklist();
        create_list_window(&blocklist_ld,&blocklist,term,ses);
}

void init_blocklist()
{
        load_blocklist();
}

void finalize_blocklist()
{
        save_blocklist();
        free_blocklist();
}
