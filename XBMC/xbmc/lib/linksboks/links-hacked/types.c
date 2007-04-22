/* types.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"


/*------------------------ ASSOCIATIONS -----------------------*/

/* DECLARATIONS */

void assoc_edit_item(struct dialog_data *,void *,void (*)(struct dialog_data *,void *,void *,struct list_description *),void *, unsigned char);
void assoc_copy_item(void *, void *);
void *assoc_default_value(struct session*, unsigned char);
void *assoc_new_item(void *);
void assoc_delete_item(void *);
unsigned char *assoc_type_item(struct terminal *, void *, int);

struct list assoc={&assoc,&assoc,0,-1,NULL};


struct assoc_ok_struct{
	void (*fn)(struct dialog_data *,void *,void *,struct list_description *);
	void *data;	
	struct dialog_data *dlg;
};


struct list_description assoc_ld={
	0,  /* 0= flat; 1=tree */
	&assoc,  /* list */
	assoc_new_item,
	assoc_edit_item,
	assoc_default_value,
	assoc_delete_item,
	assoc_copy_item,
	assoc_type_item,

        1,1,1,1,                  /* User can do anything with items */

	0,		/* this is set in init_assoc function */
	50,  /* width of main window */
	10,  /* # of items in main window */
	T_ASSOCIATION,
	T_ASSOCIATIONS_ALREADY_IN_USE,
	T_ASSOCIATIONS_MANAGER,
	T_DELETE_ASSOCIATION,
	0,	/* no button */
	NULL,	/* no button */

        NULL,   /* no close hook */

	0,0,0,0,  /* internal vars */
	0 /* modified */
};





void *assoc_default_value(struct session* ses, unsigned char type)
{
	type=type;
	ses=ses;
	return NULL;
}


void *assoc_new_item(void *ignore)
{
	struct assoc *new;

	ignore=ignore;
	if (!(new = mem_alloc(sizeof(struct assoc)))) return NULL;
	new->label = stracpy("");
	new->ct = stracpy("");
	new->prog = stracpy("");
	new->block = new->xwin = new->cons = 1;
	new->ask = 1;
	new->type=0;
	new->system = SYSTEM_ID;
	return new;
}


void assoc_delete_item(void *data)
{
	struct assoc *del=(struct assoc *)data;
	struct assoc *next=del->next;
	struct assoc *prev=del->prev;

	if (del->label)mem_free(del->label);
	if (del->ct)mem_free(del->ct);
	if (del->prog)mem_free(del->prog);
	if (next)next->prev=del->prev;
	if (prev)prev->next=del->next;
	mem_free(del);
}


void assoc_copy_item(void *in, void *out)
{
	struct assoc *item_in=(struct assoc *)in;
	struct assoc *item_out=(struct assoc *)out;

	item_out->cons=item_in->cons;
	item_out->xwin=item_in->xwin;
	item_out->block=item_in->block;
	item_out->ask=item_in->ask;
	item_out->system=item_in->system;

	if (item_out->label)mem_free(item_out->label);
	if (item_out->ct)mem_free(item_out->ct);
	if (item_out->prog)mem_free(item_out->prog);

	item_out->label=stracpy(item_in->label);
	item_out->ct=stracpy(item_in->ct);
	item_out->prog=stracpy(item_in->prog);
}


/* allocate string and print association into it */
/* x: 0=type all, 1=type title only */
unsigned char *assoc_type_item(struct terminal *term, void *data, int x)
{
	unsigned char *txt, *txt1;
	struct conv_table *table;
	struct assoc* item=(struct assoc*)data;

	if ((struct list*)item==(&assoc))return stracpy(_(TXT(T_ASSOCIATIONS),term));
	txt=stracpy("");
	if (item->system != SYSTEM_ID) add_to_strn(&txt, "XX ");
	add_to_strn(&txt, item->label);
	if (item->ct){add_to_strn(&txt,": ");add_to_strn(&txt,item->ct);}
	if (!x)
	{
		add_to_strn(&txt," -> ");
		if (item->prog)add_to_strn(&txt,item->prog);
	}
	table=get_translation_table(assoc_ld.codepage,term->spec->charset);
	txt1=convert_string(table,txt,strlen(txt),NULL);
	mem_free(txt);
			
	return txt1;
}


void menu_assoc_manager(struct terminal *term,void *fcp,struct session *ses)
{
	create_list_window(&assoc_ld,&assoc,term,ses);
}

unsigned char *ct_msg[] = {
	TXT(T_LABEL),
	TXT(T_CONTENT_TYPES),
	TXT(T_PROGRAM__IS_REPLACED_WITH_FILE_NAME),
#ifdef ASSOC_BLOCK
	TXT(T_BLOCK_TERMINAL_WHILE_PROGRAM_RUNNING),
#endif
#ifdef ASSOC_CONS_XWIN
	TXT(T_RUN_ON_TERMINAL),
	TXT(T_RUN_IN_XWINDOW),
#endif
	TXT(T_ASK_BEFORE_OPENING),
};

void assoc_edit_item_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, rw;
	int y = gf_val(-1, -G_BFU_FONT_SIZE);
	int p = 1;
#ifdef ASSOC_BLOCK
	p++;
#endif
#ifdef ASSOC_CONS_XWIN
	p += 2;
#endif
	max_text_width(term, ct_msg[0], &max, AL_LEFT);
	min_text_width(term, ct_msg[0], &min, AL_LEFT);
	max_text_width(term, ct_msg[1], &max, AL_LEFT);
	min_text_width(term, ct_msg[1], &min, AL_LEFT);
	max_text_width(term, ct_msg[2], &max, AL_LEFT);
	min_text_width(term, ct_msg[2], &min, AL_LEFT);
	max_group_width(term, ct_msg + 3, dlg->items + 3, p, &max);
	min_group_width(term, ct_msg + 3, dlg->items + 3, p, &min);
	max_buttons_width(term, dlg->items + 3 + p, 2, &max);
	min_buttons_width(term, dlg->items + 3 + p, 2, &min);
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > term->x - 2 * DIALOG_LB) w = term->x - 2 * DIALOG_LB;
	if (w < 1) w = 1;
	rw = 0;
	dlg_format_text(dlg, NULL, _(ct_msg[0], term), 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(2, G_BFU_FONT_SIZE * 2);
	dlg_format_text(dlg, NULL, _(ct_msg[1], term), 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(2, G_BFU_FONT_SIZE * 2);
	dlg_format_text(dlg, NULL, _(ct_msg[2], term), 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(2, G_BFU_FONT_SIZE * 2);
	dlg_format_group(dlg, NULL, ct_msg + 3, dlg->items + 3, p, 0, &y, w, &rw);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, NULL, dlg->items + 3 + p, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = w + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB;
	dlg_format_text(dlg, term, ct_msg[0], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_field(dlg, term, &dlg->items[0], dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text(dlg, term, ct_msg[1], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_field(dlg, term, &dlg->items[1], dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text(dlg, term, ct_msg[2], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_field(dlg, term, &dlg->items[2], dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_group(dlg, term, ct_msg + 3, &dlg->items[3], p, dlg->x + DIALOG_LB, &y, w, NULL);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, term, &dlg->items[3 + p], 2, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
}

/* Puts url and title into the bookmark item */
void assoc_edit_done(void *data)
{
	struct dialog *d=(struct dialog*)data;
	struct assoc *item=(struct assoc *)d->udata;
	struct assoc_ok_struct* s=(struct assoc_ok_struct*)d->udata2;
	unsigned char *txt;
	struct conv_table *table;
	unsigned char *label, *ct, *prog;

	label=(unsigned char *)&d->items[10];
	ct=label+MAX_STR_LEN;
	prog=ct+MAX_STR_LEN;

	table=get_translation_table(s->dlg->win->term->spec->charset,assoc_ld.codepage);
	txt=convert_string(table,label,strlen(label),NULL);
	mem_free(item->label); item->label=txt;

	txt=convert_string(table,ct,strlen(ct),NULL);
	mem_free(item->ct); item->ct=txt;

	txt=convert_string(table,prog,strlen(prog),NULL);
	mem_free(item->prog); item->prog=txt;

	s->fn(s->dlg,s->data,item,&assoc_ld);
	d->udata=0;  /* for abort function */
}


/* destroys an item, this function is called when edit window is aborted */
void assoc_edit_abort(struct dialog_data *data)
{
	struct assoc *item=(struct assoc*)data->dlg->udata;
	struct dialog *dlg=data->dlg;

	mem_free(dlg->udata2);
	if (item)assoc_delete_item(item);
}


void assoc_edit_item(struct dialog_data *dlg, void *data, void (*ok_fn)(struct dialog_data *, void *, void *, struct list_description *), void *ok_arg, unsigned char dlg_title)
{
	int p;
	struct assoc *new=(struct assoc*)data;
	struct terminal *term=dlg->win->term;
	struct dialog *d;
	struct assoc_ok_struct *s;
	unsigned char *ct, *prog, *label;

	if (!(d = mem_alloc(sizeof(struct dialog) + 10 * sizeof(struct dialog_item) + 3 * MAX_STR_LEN))) return;
	memset(d, 0, sizeof(struct dialog) + 10 * sizeof(struct dialog_item) + 3 * MAX_STR_LEN);

	label=(unsigned char *)&d->items[10];
	ct=label+MAX_STR_LEN;
	prog=ct+MAX_STR_LEN;

	if (new->label)strncpy(label,new->label,MAX_STR_LEN);
	if (new->ct)strncpy(ct,new->ct,MAX_STR_LEN);
	if (new->prog)strncpy(prog,new->prog,MAX_STR_LEN);
	
	/* Create the dialog */
	if (!(s=mem_alloc(sizeof(struct assoc_ok_struct))))return;
	s->fn=ok_fn;
	s->data=ok_arg;
	s->dlg=dlg;
		
	switch (dlg_title)
	{
		case TITLE_EDIT:
		d->title=TXT(T_EDIT_ASSOCIATION);
		break;

		case TITLE_ADD:
		d->title=TXT(T_ADD_ASSOCIATION);
		break;

		default:
		internal("Unsupported dialog title.\n");
	}

	d->udata=data;
	d->udata2=s;
	d->fn = assoc_edit_item_fn;
	d->abort=assoc_edit_abort;
	d->refresh=assoc_edit_done;
	d->refresh_data = d;
	d->items[0].type = D_FIELD;
	d->items[0].dlen = MAX_STR_LEN;
	d->items[0].data = label;
	d->items[0].fn = check_nonempty;
	d->items[1].type = D_FIELD;
	d->items[1].dlen = MAX_STR_LEN;
	d->items[1].data = ct;
	d->items[1].fn = check_nonempty;
	d->items[2].type = D_FIELD;
	d->items[2].dlen = MAX_STR_LEN;
	d->items[2].data = prog;
	d->items[2].fn = check_nonempty;
	p = 3;
#ifdef ASSOC_BLOCK
	d->items[p].type = D_CHECKBOX;
	d->items[p].data = (unsigned char *)&new->block;
	d->items[p++].dlen = sizeof(int);
#endif
#ifdef ASSOC_CONS_XWIN
	d->items[p].type = D_CHECKBOX;
	d->items[p].data = (unsigned char *)&new->cons;
	d->items[p++].dlen = sizeof(int);
	d->items[p].type = D_CHECKBOX;
	d->items[p].data = (unsigned char *)&new->xwin;
	d->items[p++].dlen = sizeof(int);
#endif
	d->items[p].type = D_CHECKBOX;
	d->items[p].data = (unsigned char *)&new->ask;
	d->items[p++].dlen = sizeof(int);
	d->items[p].type = D_BUTTON;
	d->items[p].gid = B_ENTER;
	d->items[p].fn = ok_dialog;
	d->items[p++].text = TXT(T_OK);
	d->items[p].type = D_BUTTON;
	d->items[p].gid = B_ESC;
	d->items[p].text = TXT(T_CANCEL);
	d->items[p++].fn = cancel_dialog;
	d->items[p++].type = D_END;
	do_dialog(term, d, getml(d, NULL));
}

void update_assoc(struct assoc *new)
{
	struct assoc *repl;
	if (!new->label[0] || !new->ct[0] || !new->prog[0]) return;
	if (!(repl = mem_alloc(sizeof(struct assoc)))) return;
	add_to_list(assoc, repl);
	repl->label = stracpy(new->label);
	repl->ct = stracpy(new->ct);
	repl->prog = stracpy(new->prog);
	repl->block = new->block;
	repl->cons = new->cons;
	repl->xwin = new->xwin;
	repl->ask = new->ask;
	repl->system = new->system;
	repl->type=0;
	new->system = new->system;
}

/*------------------------ EXTENSIONS -----------------------*/

/* DECLARATIONS */
void ext_edit_item(struct dialog_data *,void *,void (*)(struct dialog_data *,void *,void *,struct list_description *),void *, unsigned char);
void ext_copy_item(void *, void *);
void *ext_default_value(struct session*, unsigned char);
void *ext_new_item(void *);
void ext_delete_item(void *);
unsigned char *ext_type_item(struct terminal *, void *, int);

struct list extensions = { &extensions, &extensions, 0, -1, NULL };


struct list_description ext_ld={
	0,  /* 0= flat; 1=tree */
	&extensions,  /* list */
	ext_new_item,
	ext_edit_item,
	assoc_default_value,
	ext_delete_item,
	ext_copy_item,
	ext_type_item,

        1,1,1,1,                  /* User can do anything with items */

	0,		/* this is set in init_assoc function */
	40,  /* width of main window */
	8,  /* # of items in main window */
	T_eXTENSION,
	T_EXTENSIONS_ALREADY_IN_USE,
	T_EXTENSIONS_MANAGER,
	T_DELETE_EXTENSION,
	0,	/* no button */
	NULL,	/* no button */

        NULL,   /* no close hook */

	0,0,0,0,  /* internal vars */
	0 /* modified */
};





void *ext_new_item(void *ignore)
{
	struct extension *new;

	ignore=ignore;
	if (!(new = mem_alloc(sizeof(struct extension)))) return NULL;
	new->ext = stracpy("");
	new->ct = stracpy("");
	new->type=0;
	return new;
}


void ext_delete_item(void *data)
{
	struct extension *del=(struct extension *)data;
	struct extension *next=del->next;
	struct extension *prev=del->prev;

	if (del->ext)mem_free(del->ext);
	if (del->ct)mem_free(del->ct);
	if (next)next->prev=del->prev;
	if (prev)prev->next=del->next;
	mem_free(del);
}


void ext_copy_item(void *in, void *out)
{
	struct extension *item_in=(struct extension *)in;
	struct extension *item_out=(struct extension *)out;

	if (item_out->ext)mem_free(item_out->ext);
	if (item_out->ct)mem_free(item_out->ct);

	item_out->ext=stracpy(item_in->ext);
	item_out->ct=stracpy(item_in->ct);
}


/* allocate string and print extension into it */
/* x: 0=type all, 1=type title only */
unsigned char *ext_type_item(struct terminal *term, void *data, int x)
{
	unsigned char *txt, *txt1;
	struct conv_table *table;
	struct extension* item=(struct extension*)data;

	if ((struct list*)item==(&extensions))return stracpy(_(TXT(T_FILE_EXTENSIONS),term));
	txt=stracpy(item->ext);
	if (item->ct){add_to_strn(&txt,": ");add_to_strn(&txt,item->ct);}
	table=get_translation_table(assoc_ld.codepage,term->spec->charset);
	txt1=convert_string(table,txt,strlen(txt),NULL);
	mem_free(txt);
			
	return txt1;
}


void menu_ext_manager(struct terminal *term,void *fcp,struct session *ses)
{
	create_list_window(&ext_ld,&extensions,term,ses);
}

unsigned char *ext_msg[] = {
	TXT(T_EXTENSION_S),
	TXT(T_CONTENT_TYPE),
};

void ext_edit_item_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, rw;
	int y = gf_val(-1, -G_BFU_FONT_SIZE);
	max_text_width(term, ext_msg[0], &max, AL_LEFT);
	min_text_width(term, ext_msg[0], &min, AL_LEFT);
	max_text_width(term, ext_msg[1], &max, AL_LEFT);
	min_text_width(term, ext_msg[1], &min, AL_LEFT);
	max_buttons_width(term, dlg->items + 2, 2, &max);
	min_buttons_width(term, dlg->items + 2, 2, &min);
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > term->x - 2 * DIALOG_LB) w = term->x - 2 * DIALOG_LB;
	if (w < 1) w = 1;
	rw = 0;
	dlg_format_text(dlg, NULL, ext_msg[0], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(2, G_BFU_FONT_SIZE * 2);
	dlg_format_text(dlg, NULL, ext_msg[1], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(2, G_BFU_FONT_SIZE * 2);
	dlg_format_buttons(dlg, NULL, dlg->items + 2, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = w + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB;
	dlg_format_text(dlg, term, ext_msg[0], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_field(dlg, term, &dlg->items[0], dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text(dlg, term, ext_msg[1], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_field(dlg, term, &dlg->items[1], dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, term, &dlg->items[2], 2, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
}


/* Puts url and title into the bookmark item */
void ext_edit_done(void *data)
{
	struct dialog *d=(struct dialog*)data;
	struct extension *item=(struct extension *)d->udata;
	struct assoc_ok_struct* s=(struct assoc_ok_struct*)d->udata2;
	unsigned char *txt;
	struct conv_table *table;
	unsigned char *ext, *ct;

	ext=(unsigned char *)&d->items[5];
	ct=ext+MAX_STR_LEN;

	table=get_translation_table(s->dlg->win->term->spec->charset,ext_ld.codepage);
	txt=convert_string(table,ext,strlen(ext),NULL);
	mem_free(item->ext); item->ext=txt;

	txt=convert_string(table,ct,strlen(ct),NULL);
	mem_free(item->ct); item->ct=txt;

	s->fn(s->dlg,s->data,item,&ext_ld);
	d->udata=0;  /* for abort function */
}


/* destroys an item, this function is called when edit window is aborted */
void ext_edit_abort(struct dialog_data *data)
{
	struct extension *item=(struct extension*)data->dlg->udata;
	struct dialog *dlg=data->dlg;

	mem_free(dlg->udata2);
	if (item)ext_delete_item(item);
}


void ext_edit_item(struct dialog_data *dlg, void *data, void (*ok_fn)(struct dialog_data *, void *, void *, struct list_description *), void *ok_arg, unsigned char dlg_title)
{
	struct extension *new=(struct extension*)data;
	struct terminal *term=dlg->win->term;
	struct dialog *d;
	struct assoc_ok_struct *s;
	unsigned char *ext;
	unsigned char *ct;

	if (!(d = mem_alloc(sizeof(struct dialog) + 5 * sizeof(struct dialog_item) + 2 * MAX_STR_LEN))) return;
	memset(d, 0, sizeof(struct dialog) + 5 * sizeof(struct dialog_item) + 2 * MAX_STR_LEN);

	ext=(unsigned char *)&d->items[5];
	ct = ext + MAX_STR_LEN;
	if (new->ext)strncpy(ext, new->ext, MAX_STR_LEN);
	if (new->ct)strncpy(ct, new->ct, MAX_STR_LEN);

	/* Create the dialog */
	if (!(s=mem_alloc(sizeof(struct assoc_ok_struct))))return;
	s->fn=ok_fn;
	s->data=ok_arg;
	s->dlg=dlg;
		
	switch (dlg_title)
	{
		case TITLE_EDIT:
		d->title=TXT(T_EDIT_EXTENSION);
		break;

		case TITLE_ADD:
		d->title=TXT(T_ADD_EXTENSION);
		break;

		default:
		internal("Unsupported dialog title.\n");
	}

	d->udata=data;
	d->udata2=s;
	d->abort=ext_edit_abort;
	d->refresh=ext_edit_done;
	d->refresh_data = d;
	d->title = TXT(T_EXTENSION);
	d->fn = ext_edit_item_fn;
	d->items[0].type = D_FIELD;
	d->items[0].dlen = MAX_STR_LEN;
	d->items[0].data = ext;
	d->items[0].fn = check_nonempty;
	d->items[1].type = D_FIELD;
	d->items[1].dlen = MAX_STR_LEN;
	d->items[1].data = ct;
	d->items[1].fn = check_nonempty;
	d->items[2].type = D_BUTTON;
	d->items[2].gid = B_ENTER;
	d->items[2].fn = ok_dialog;
	d->items[2].text = TXT(T_OK);
	d->items[3].type = D_BUTTON;
	d->items[3].gid = B_ESC;
	d->items[3].text = TXT(T_CANCEL);
	d->items[3].fn = cancel_dialog;
	d->items[4].type = D_END;
	do_dialog(term, d, getml(d, NULL));
}

void update_ext(struct extension *new)
{
	struct extension *repl;
	if (!new->ext[0] || !new->ct[0]) return;
	if (!(repl = mem_alloc(sizeof(struct extension)))) return;
	add_to_list(extensions, repl);
	repl->ext = stracpy(new->ext);
	repl->ct = stracpy(new->ct);
	repl->type=0;
}


/* creates default extensions if extension list is empty */
void create_initial_extensions(void)
{
	struct extension ext;

	if (!list_empty(extensions))return;

	/* here you can add any default extension you want */
	ext.ext="aif,aiff,aifc",ext.ct="audio/x-aiff",update_ext(&ext);
	ext.ext="au,snd",ext.ct="audio/basic",update_ext(&ext);
	ext.ext="avi",ext.ct="video/x-msvideo",update_ext(&ext);
	ext.ext="deb",ext.ct="application/x-debian-package",update_ext(&ext);
	ext.ext="dl",ext.ct="video/dl",update_ext(&ext);
	ext.ext="dvi",ext.ct="application/x-dvi",update_ext(&ext);
	ext.ext="fli",ext.ct="video/fli",update_ext(&ext);
	ext.ext="gif",ext.ct="image/gif",update_ext(&ext);
	ext.ext="gl",ext.ct="video/gl",update_ext(&ext);
	ext.ext="jpg,jpeg,jpe",ext.ct="image/jpeg",update_ext(&ext);
	ext.ext="mid,midi",ext.ct="audio/midi",update_ext(&ext);
	ext.ext="mpeg,mpg,mpe",ext.ct="video/mpeg",update_ext(&ext);
	ext.ext="pbm",ext.ct="image/x-portable-bitmap",update_ext(&ext);
	ext.ext="pdf",ext.ct="application/pdf",update_ext(&ext);
	ext.ext="pgm",ext.ct="image/x-portable-graymap",update_ext(&ext);
	ext.ext="pgp",ext.ct="application/pgp-signature",update_ext(&ext);
	ext.ext="png",ext.ct="image/png",update_ext(&ext);
	ext.ext="pnm",ext.ct="image/x-portable-anymap",update_ext(&ext);
	ext.ext="ppm",ext.ct="image/x-portable-pixmap",update_ext(&ext);
	ext.ext="ppt",ext.ct="application/powerpoint",update_ext(&ext);
	ext.ext="ps,eps,ai",ext.ct="application/postscript",update_ext(&ext);
	ext.ext="qt,mov",ext.ct="video/quicktime",update_ext(&ext);
	ext.ext="ra,rm,ram",ext.ct="audio/x-pn-realaudio",update_ext(&ext);
	ext.ext="rtf",ext.ct="application/rtf",update_ext(&ext);
	ext.ext="swf",ext.ct="application/x-shockwave-flash",update_ext(&ext);
	ext.ext="tga",ext.ct="image/targa",update_ext(&ext);
	ext.ext="tiff,tif",ext.ct="image/tiff",update_ext(&ext);
	ext.ext="wav",ext.ct="audio/x-wav",update_ext(&ext);
	ext.ext="xbm",ext.ct="image/x-xbitmap",update_ext(&ext);
	ext.ext="xls",ext.ct="application/excel",update_ext(&ext);
	ext.ext="xpm",ext.ct="image/x-xpixmap",update_ext(&ext);
}

/* --------------------------- PROG -----------------------------*/


int is_in_list(unsigned char *list, unsigned char *str, int l)
{
	unsigned char *l2, *l3;
	if (!l) return 0;
	rep:
	while (*list && *list <= ' ') list++;
	if (!*list) return 0;
	for (l2 = list; *l2 && *l2 != ','; l2++) ;
	for (l3 = l2 - 1; l3 >= list && *l3 <= ' '; l3--) ;
	l3++;
	if (l3 - list == l && !casecmp(str, list, l)) return 1;
	list = l2;
	if (*list == ',') list++;
	goto rep;
}

unsigned char *get_content_type(unsigned char *head, unsigned char *url)
{
	struct extension *e;
	struct assoc *a;
	unsigned char *ct, *ext, *exxt;
	int extl, el;
	/* ysbox: the header check is now below. That's because of stupid servers
	which return text/plain for wmv videos! */
	ext = NULL, extl = 0;
	for (ct = url; *ct && !end_of_dir(*ct); ct++)
		if (*ct == '.') ext = ct + 1;
		else if (dir_sep(*ct)) ext = NULL;
	if (ext) while (ext[extl] && !dir_sep(ext[extl]) && !end_of_dir(ext[extl])) extl++;
	if ((extl == 3 && !casecmp(ext, "htm", 3))   ||
            (extl == 4 && !casecmp(ext, "html", 4)) ||
            (extl == 5 && !casecmp(ext, "shtml", 5)))
            return stracpy("text/html");
	foreach(e, extensions) if (is_in_list(e->ext, ext, extl)) return stracpy(e->ct);

	if ((extl == 3 && !casecmp(ext, "jpg", 3)) ||
	    (extl == 4 && !casecmp(ext, "pjpg", 4))||
	    (extl == 4 && !casecmp(ext, "jpeg", 4))||
	    (extl == 5 && !casecmp(ext, "pjpeg", 5))) return stracpy("image/jpeg");
	if ((extl == 3 && !casecmp(ext, "png", 3))) return stracpy("image/png");
	if ((extl == 3 && !casecmp(ext, "gif", 3))) return stracpy("image/gif");
	if ((extl == 3 && !casecmp(ext, "xbm", 3))) return stracpy("image/x-xbitmap");
	if ((extl == 3 && !casecmp(ext, "tif", 3)) ||
	    (extl == 4 && !casecmp(ext, "tiff", 4))) return stracpy("image/tiff");
	if ((extl == 3 && !casecmp(ext, "wmv", 4))) return stracpy("video/x-ms-wmv");
	if ((extl == 3 && !casecmp(ext, "asf", 4))) return stracpy("video/x-ms-asf");
	if ((extl == 3 && !casecmp(ext, "wma", 4))) return stracpy("video/x-ms-wma");

	if (head && (ct = parse_http_header(head, "Content-Type", NULL))) {
		unsigned char *s;
		if ((s = strchr(ct, ';'))) *s = 0;
		while (*ct && ct[strlen(ct) - 1] <= ' ') ct[strlen(ct) - 1] = 0;
		return ct;
	}
	exxt = init_str(); el = 0;
	add_to_str(&exxt, &el, "application/x-");
	add_bytes_to_str(&exxt, &el, ext, extl);
	foreach(a, assoc) if (is_in_list(a->ct, exxt, el)) return exxt;
	mem_free(exxt);
	return !force_html ? stracpy("text/plain") : stracpy("text/html");
}

/* returns field with associations */
struct assoc *get_type_assoc(struct terminal *term, unsigned char *type, int *n)
{
	struct assoc *vecirek;
	struct assoc *a;
	int count=0;
        foreach(a, assoc)
                if (a->system == SYSTEM_ID &&
                    ((term->environment & ENV_XWIN) ? a->xwin : a->cons) &&
                    is_in_list(a->ct, type, strlen(type)))
                        count ++;
        *n=count;
        if (!count)return NULL;
	vecirek=mem_alloc(count*sizeof(struct assoc));
	if (!vecirek)internal("Cannot allocate memory.\n");
	count=0;
	foreach(a, assoc) 
		if (a->system == SYSTEM_ID && (term->environment & ENV_XWIN ? a->xwin : a->cons) && is_in_list(a->ct, type, strlen(type))) 
			vecirek[count++]=*a;
	return vecirek;
}

void free_types()
{
	struct assoc *a;
	struct extension *e;
	struct protocol_program *p;
	foreach(a, assoc) {
		mem_free(a->ct);
		mem_free(a->prog);
		mem_free(a->label);
	}
	free_list(assoc);
	foreach(e, extensions) {
		mem_free(e->ext);
		mem_free(e->ct);
	}
	free_list(extensions);
}

void update_prog(struct list_head *l, unsigned char *p, int s)
{
	struct protocol_program *repl;
	foreach(repl, *l) if (repl->system == s) {
		mem_free(repl->prog);
		goto ss;
	}
	if (!(repl = mem_alloc(sizeof(struct protocol_program)))) return;
	add_to_list(*l, repl);
	repl->system = s;
	ss:
	if ((repl->prog = mem_alloc(MAX_STR_LEN))) {
		strncpy(repl->prog, p, MAX_STR_LEN);
		repl->prog[MAX_STR_LEN - 1] = 0;
	}
}

unsigned char *get_prog(struct list_head *l)
{
	struct protocol_program *repl;
	foreach(repl, *l) if (repl->system == SYSTEM_ID) return repl->prog;
	update_prog(l, "", SYSTEM_ID);
	foreach(repl, *l) if (repl->system == SYSTEM_ID) return repl->prog;
	return NULL;
}
