/* bookmarks.c
 * (c) 2002 Petr 'Brain' Kulhavy, Karel 'Clock' Kulhavy
 * This file is a part of the Links program, released under GPL.
 */

#include <stdio.h>
#include <string.h>

#include "links.h"

int can_write_bookmarks=0;	/* global flag if we can write bookmarks */

unsigned char *current_bookmarks_file; /* Name of currently open bookmarks file */
time_t         current_bookmarks_time; /* Time of its last modification */

void *bookmark_new_item(void *);
unsigned char *bookmark_type_item(struct terminal *, void *, int);
void bookmark_delete_item(void *);
void bookmark_edit_item(struct dialog_data *,void *,void (*)(struct dialog_data *,void *,void *,struct list_description *),void *, unsigned char);
void bookmark_copy_item(void *, void *);
void bookmark_goto_item(struct session *, void *);
void *bookmark_default_value(struct session*, unsigned char);
void bookmark_close_dialog(struct list_description *);
void save_bookmarks(void);

struct list bookmarks={&bookmarks,&bookmarks,0,-1,NULL};
/* when you change anything, don't forget to change it in reinit_bookmarks too !*/

struct bookmark_ok_struct{
	void (*fn)(struct dialog_data *,void *,void *,struct list_description *);
	void *data;	
	struct dialog_data *dlg;
};

struct bookmark_list{
	/* common for all lists */
	struct bookmark_list *next;
	struct bookmark_list *prev;
	unsigned char type;  
	int depth;
	void *fotr;

	/* bookmark specific */
	unsigned char *title;
	unsigned char *url;
};

struct list_description bookmark_ld=
{
	1,  /* 0= flat; 1=tree */
	&bookmarks,  /* list */
	bookmark_new_item,	/* no codepage translations */
	bookmark_edit_item,	/* translate when create dialog and translate back when ok is pressed */
	bookmark_default_value,	/* codepage translation from current_page_encoding to UTF8 */
	bookmark_delete_item,	/* no codepage translations */
	bookmark_copy_item,	/* no codepage translations */
	bookmark_type_item,	/* no codepage translations (bookmarks are internally in UTF8) */

        1,1,1,1,                  /* User can do anything with items */

	0,		/* this is set in init_bookmarks function */
	60,  /* width of main window */
        15,  /* # of items in main window */
	T_BOOKMARK,
	T_BOOKMARKS_ALREADY_IN_USE,
	T_BOOKMARK_MANAGER,
	T_DELETE_BOOKMARK,
	T_GOTO,
	bookmark_goto_item,	/* FIXME: should work (URL in UTF8), but who knows? */

        bookmark_close_dialog,

	0,0,0,0,  /* internal vars */
	0 /* modified */
};

struct kawasaki
{
	unsigned char *title;
	unsigned char *url;
};



/* clears the bookmark list */
void free_bookmarks(void) 
{
	struct bookmark_list *bm;

	foreach(bm, bookmarks) {
		mem_free(bm->title);
		mem_free(bm->url);
	}

	free_list(bookmarks);
}


/* called before exiting the links */
void finalize_bookmarks(void)
{
	free_bookmarks();

        if(current_bookmarks_file)
                mem_free(current_bookmarks_file);
}



/* allocates struct kawasaki and puts current page title and url */
/* type: 0=item, 1=directory */
/* on error returns NULL */
void *bookmark_default_value(struct session *ses, unsigned char type)
{
	struct kawasaki *zelena;
	unsigned char *txt;

	txt=mem_alloc(MAX_STR_LEN*sizeof(unsigned char));
	if (!txt)return NULL;
	
	zelena=mem_alloc(sizeof(struct kawasaki));
	if (!zelena){mem_free(txt);return NULL;}

	zelena->url=NULL;
	zelena->title=NULL;
	if (get_current_url(ses,txt,MAX_STR_LEN))
	{
		if (ses->screen->f_data)
		{
			struct conv_table* ct;
			
			ct=get_translation_table(ses->term->spec->charset,bookmark_ld.codepage);
			zelena->url=convert_string(ct,txt,strlen(txt),NULL);
		}
		else
			zelena->url=stracpy(txt);
	}
	if (get_current_title(ses,txt,MAX_STR_LEN))  /* ses->screen->f_data must exist here */
	{
		struct conv_table* ct;
		
		ct=get_translation_table(ses->term->spec->charset,bookmark_ld.codepage);
		zelena->title=convert_string(ct,txt,strlen(txt),NULL);
	}

	mem_free(txt);

	return zelena;
}


void bookmark_copy_item(void *in, void *out)
{
	struct bookmark_list *item_in=(struct bookmark_list*)in;
	struct bookmark_list *item_out=(struct bookmark_list*)out;

	item_out->type=item_in->type;
	item_out->depth=item_in->depth;

	if (item_out->title)
	{
		mem_free(item_out->title);
		item_out->title=stracpy(item_in->title);
	}
	else internal("Bookmarks inconsistency.\n");
	if (item_out->url)
	{
		mem_free(item_out->url);
		item_out->url=stracpy(item_in->url);
	}
	else internal("Bookmarks inconsistency.\n");
	return;
}


unsigned char *bm_add_msg[] = {
	TXT(T_NNAME),
	TXT(T_URL),
};


/* Called to setup the add bookmark dialog */
void bookmark_edit_item_fn(struct dialog_data *dlg)
{
	int max = 0, min = 0;
	int w, rw;
	int y = gf_val(-1, -1*G_BFU_FONT_SIZE);
	struct terminal *term;
	int a;

	term = dlg->win->term;
	
	for (a=0;a<dlg->n-2;a++)
	{
		max_text_width(term, bm_add_msg[a], &max, AL_LEFT);
		min_text_width(term, bm_add_msg[a], &min, AL_LEFT);
	}
	max_buttons_width(term, dlg->items + dlg->n-2, 2, &max);
	min_buttons_width(term, dlg->items + dlg->n-2, 2, &min);
	if (max < dlg->dlg->items->dlen) max = dlg->dlg->items->dlen;
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
	
	if (w > max) w = max;
	if (w < min) w = min;
        /* if (w > term->x - 2 * DIALOG_LB) w = term->x - 2 * DIALOG_LB; */
	if (w < 1) w = 1;

        rw = w;
        /* w = rw = gf_val(50,30*G_BFU_FONT_SIZE); */
	
	for (a=0;a<dlg->n-2;a++)
	{
		dlg_format_text(dlg, NULL, bm_add_msg[a], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
		y += gf_val(2,2*G_BFU_FONT_SIZE);
	}
	dlg_format_buttons(dlg, NULL, dlg->items+dlg->n-2, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = w + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB;
	for (a=0;a<dlg->n-2;a++)
	{
		dlg_format_text(dlg, term, bm_add_msg[a], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
		dlg_format_field(dlg, term, &dlg->items[a], dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
		y+=gf_val(1,G_BFU_FONT_SIZE);
	}
	dlg_format_buttons(dlg, term, &dlg->items[dlg->n-2], 2, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
}


/* Puts url and title into the bookmark item */
void bookmark_edit_done(void *data)
{
	struct dialog *d=(struct dialog*)data;
	struct bookmark_list *item=(struct bookmark_list *)d->udata;
	unsigned char *title, *url;
	struct bookmark_ok_struct* s=(struct bookmark_ok_struct*)d->udata2;
	int a;

	if ((item->type)&1)a=4; /* folder */
	else a=5;
	title = (unsigned char *)&d->items[a];
	url = title + MAX_STR_LEN;

	if (item->title)
	{
		struct conv_table* ct;
		
		mem_free(item->title);
		ct=get_translation_table(s->dlg->win->term->spec->charset,bookmark_ld.codepage);
		item->title=convert_string(ct,title,strlen(title),NULL);
	}

	if (item->url)
	{
		struct conv_table* ct;
		
		mem_free(item->url);
		ct=get_translation_table(s->dlg->win->term->spec->charset,bookmark_ld.codepage);
		item->url=convert_string(ct,url,strlen(url),NULL);
	}

	s->fn(s->dlg,s->data,item,&bookmark_ld);
	d->udata=0;  /* for abort function */
}


/* destroys an item, this function is called when edit window is aborted */
void bookmark_edit_abort(struct dialog_data *data)
{
	struct bookmark_list *item=(struct bookmark_list*)data->dlg->udata;
	struct dialog *dlg=data->dlg;

	mem_free(dlg->udata2);
	if (item)bookmark_delete_item(item);
}


/* dlg_title is TITLE_EDIT or TITLE_ADD */
/* edit item function */
void bookmark_edit_item(struct dialog_data *dlg,void *data,void (*ok_fn)(struct dialog_data *,void * ,void *, struct list_description *),void * ok_arg, unsigned char dlg_title)
{
	struct bookmark_list *item=(struct bookmark_list *)data;
	unsigned char *title, *url;
	struct dialog *d;
	struct bookmark_ok_struct *s;
	int a;
	
	/* Create the dialog */
	if (!(s=mem_alloc(sizeof(struct bookmark_ok_struct))))return;
	s->fn=ok_fn;
	s->data=ok_arg;
	s->dlg=dlg;
		

	if ((item->type)&1)a=4; /* folder */
	else a=5;
	if (!(d = mem_alloc(sizeof(struct dialog) + a * sizeof(struct dialog_item) + 2 * MAX_STR_LEN))){mem_free(s);return;}
	memset(d, 0, sizeof(struct dialog) + a * sizeof(struct dialog_item) + 2 * MAX_STR_LEN);


	title = (unsigned char *)&d->items[a];
	url = title + MAX_STR_LEN;

	{
		unsigned char *txt;
		struct conv_table* ct;
		
		ct=get_translation_table(bookmark_ld.codepage,dlg->win->term->spec->charset);
		txt=convert_string(ct,item->title,strlen(item->title),NULL);
		safe_strncpy(title,txt,MAX_STR_LEN);
		mem_free(txt);

		txt=convert_string(ct,item->url,strlen(item->url),NULL);
		safe_strncpy(url,txt,MAX_STR_LEN);
		mem_free(txt);
	}

	switch (dlg_title)
	{
		case TITLE_EDIT:
		if ((item->type)&1)d->title=TXT(T_EDIT_FOLDER);
		else d->title=TXT(T_EDIT_BOOKMARK);
		break;

		case TITLE_ADD:
		if ((item->type)&1)d->title=TXT(T_ADD_FOLDER);
		else d->title=TXT(T_ADD_BOOKMARK);
		break;

		default:
		internal("Unsupported dialog title.\n");
	}
	d->fn = bookmark_edit_item_fn;
	d->udata=data;  /* item */
	d->udata2=s;
	d->refresh = bookmark_edit_done;
	d->abort = bookmark_edit_abort;
	d->refresh_data = d;

	d->items[0].type = D_FIELD;
	d->items[0].dlen = MAX_STR_LEN;
	d->items[0].data = title;
	d->items[0].fn = check_nonempty;

	a=0;
	if (!((item->type)&1))  /* item */
	{
		d->items[1].type = D_FIELD;
		d->items[1].dlen = MAX_STR_LEN;
		d->items[1].data = url;
		d->items[1].fn = check_nonempty;
		a++;
	}

	d->items[a+1].type = D_BUTTON;
	d->items[a+1].gid = B_ENTER;
	d->items[a+1].fn = ok_dialog;
	d->items[a+1].text = TXT(T_OK);
	
	d->items[a+2].type = D_BUTTON;
	d->items[a+2].gid = B_ESC;
	d->items[a+2].text = TXT(T_CANCEL);
	d->items[a+2].fn = cancel_dialog;
	
	d->items[a+3].type = D_END;
	
	do_dialog(dlg->win->term, d, getml(d, NULL));
}


/* create new bookmark item and returns pointer to it, on error returns 0*/
/* bookmark is filled with given data, data are deallocated afterwards */
void *bookmark_new_item(void * data)
{
	struct bookmark_list *b;
	struct kawasaki *zelena=(struct kawasaki *)data;

	b=mem_alloc(sizeof(struct bookmark_list));
	if (!b)return 0;
	
	b->url=mem_alloc(sizeof(unsigned char));
	if (!(b->url)){mem_free(b);return 0;}
	b->title=mem_alloc(sizeof(unsigned char));
	if (!(b->title)){mem_free(b->url);mem_free(b);return 0;}
	
	*(b->url)=0;  /* empty strings */
	*(b->title)=0;

	if (!zelena) return b;
	
	if (zelena->title)
	{
		add_to_strn(&(b->title),zelena->title);
		mem_free(zelena->title);
	}
	if (zelena->url)
	{
		add_to_strn(&(b->url),zelena->url);
		mem_free(zelena->url);
	}

	mem_free(zelena);

	return b;
}


/* allocate string and print bookmark into it */
/* x: 0=type all, 1=type title only */
unsigned char *bookmark_type_item(struct terminal *term, void *data, int x)
{
	unsigned char *txt, *txt1;
	struct bookmark_list* item=(struct bookmark_list*)data;
	struct conv_table *table;

	if (item==(struct bookmark_list*)(&bookmarks))   /* head */
		return stracpy(_(TXT(T_BOOKMARKS),term));

	txt=stracpy(item->title);
	if (!x&&!((item->type)&1))
	{
		add_to_strn(&txt," (");
		if (item->url)add_to_strn(&txt,item->url);
		add_to_strn(&txt,")");
	}

	table=get_translation_table(bookmark_ld.codepage,term->spec->charset);
	txt1=convert_string(table,txt,strlen(txt),NULL);
	mem_free(txt);
	return txt1;
}


/* goto bookmark (called when goto button is pressed) */
void bookmark_goto_item(struct session *ses, void *i)
{
	struct bookmark_list *item=(struct bookmark_list*)i;

    goto_url(ses,item->url);
}


/* delete bookmark from list */
void bookmark_delete_item(void *data)
{
	struct bookmark_list* item=(struct bookmark_list*)data;
	struct bookmark_list *prev=item->prev;
	struct bookmark_list *next=item->next;

	if (list_empty(*item)||((struct list*)data==&bookmarks))return;  /* empty list or head */

	if (item->url)mem_free(item->url);
	if (item->title)mem_free(item->title);
        if (next)
                next->prev=item->prev;
        if (prev)
                prev->next=item->next;
	mem_free(item);
        bookmark_ld.modified=1;
}



/* returns previous item in the same folder and with same the depth, or father if there's no previous item */
/* we suppose that previous items have correct pointer fotr */
struct bookmark_list *previous_on_this_level(struct bookmark_list *item)
{
	struct bookmark_list *p;

	for (p=item->prev;p->depth>item->depth;p=p->fotr);
	return p;
}


/* create new bookmark at the end of the list */
/* if url is NULL, create folder */
/* both strings are null terminated */
void add_bookmark(unsigned char *title, unsigned char *url, int depth)
{
	struct bookmark_list *b,*p;
        struct conv_table *ct = get_translation_table(
                                                      get_cp_index(options_get("bookmarks_codepage")),
                                                      bookmark_ld.codepage);
        struct document_options *dop;

        if (!title) return;
	
	b=mem_alloc(sizeof(struct bookmark_list));
        if (!b) return;

	dop = mem_calloc(sizeof(struct document_options));
        if (!dop){
		mem_free(b);
		return;
        }
	dop->cp = get_cp_index(options_get("bookmarks_codepage"));
	
        b->title=convert_string(ct,title,strlen(title),dop);

	if (url){
                dop->plain = 1;
		b->url=convert_string(ct,url,strlen(url),dop);
                b->type=0;
        }
	else {
		b->url=mem_alloc(sizeof(unsigned char));
                if (!(b->url)){
                        mem_free(b->title);
			mem_free(b);
                        mem_free(dop);
                        return;
                }
		*(b->url)=0;
		b->type=1;
        }
	
	b->depth=depth;

	p=bookmarks.prev;
	b->prev=p;
	b->next=(struct bookmark_list *)(&bookmarks);
	p->next=b;
	bookmarks.prev=b;

	
	p=previous_on_this_level(b);

	if (p->depth<b->depth)
		b->fotr=p;   /* directory b belongs into */
	else
		b->fotr=p->fotr;

	mem_free(dop);
}

/* Created pre-cooked bookmarks */
void create_initial_bookmarks(void)
{
        bookmark_ld.codepage=get_cp_index(options_get("bookmarks_codepage"));
        add_bookmark("Links",NULL,0);
	add_bookmark("LinksBoks Homepage","http://ysbox.online.fr/",1);
	add_bookmark("Calibration Procedure","http://atrey.karlin.mff.cuni.cz/~clock/twibright/links/calibration.html",1);
	add_bookmark("Links Homepage","http://atrey.karlin.mff.cuni.cz/~clock/twibright/links/",1);
	add_bookmark("Links Manual","http://links.sourceforge.net/docs/manual-0.90-en/",1);
	add_bookmark("Google","http://www.google.com/",1);
}

void load_bookmarks(void)
{
	FILE *f;
	unsigned char *buf;
	int len;
	
	unsigned char *p, *end;
	unsigned char *name, *attr;
	int namelen;
	int status;
	unsigned char *title=0;
	unsigned char *url=0;
	int depth;

	struct document_options dop;
	
        bookmark_ld.codepage=get_cp_index(options_get("bookmarks_codepage"));

        memset(&dop, 0, sizeof(dop));
	dop.plain=1;

	/* status:	
	 *		0 = find <dt> or </dl> element
	 *		1 = find <a> or <h3> element
	 *		2 = reading bookmark, find </a> element, title is pointer 
	 *		    behind the leading <a> element
	 *		3 = reading folder name, find </h3> element, title is
	 *		pointer behind leading <h3> element
	 */

	f=fopen(options_get("bookmarks_file"),"r");
	can_write_bookmarks=1;
	if (!f){
		can_write_bookmarks=(errno==ENOENT); /* if open failed and the file exists, don't write initial bookmarks when finishing */
		create_initial_bookmarks();
		bookmark_ld.modified=1;
		return;
        }

	fseek(f,0,SEEK_END);
	len=ftell(f);
	fseek(f,0,SEEK_SET);
	if (!(buf=mem_alloc(len))){fclose(f);return;}
	fread(buf,len,1,f);
	fclose(f);

	p=buf;
	end=buf+len;
	
	status=0;  /* find bookmark */
	depth=0;
	
	d_opt=&dop;
	while (1){
		unsigned char *s;
		
                while (p<end&&*p!='<')
                        p++;  /* find start of html tag */

                if (p>=end)
                        break;   /* parse end */
                s=p;

                if (p+2<=end &&
                    (p[1]=='!'
                     ||p[1]=='?')) {
                        p=skip_comment(p,end);
                        continue;
                }

                if (parse_element(p, end, &name, &namelen, &attr, &p)) {
                        p++;
                        continue;
                }
		
		switch (status) {
                case 0:  /* <dt> or </dl> */
                        if (namelen==2&&
                            !casecmp(name,"dt",2))
                                status=1;
                        else if (namelen==3 &&
                                 !casecmp(name,"/dl",3)) {
                                depth--;
                                if (depth==-1)goto smitec;
			}
                        continue;

                case 1:   /* find "a" element */
                        if (namelen==1 &&
                            !casecmp(name,"a",1)){
                                url=get_attr_val(attr,"href");
                                if (!url)
                                        continue;
				status=2;
				title=p;
			}
                        if (namelen==2 &&
                            !casecmp(name,"h3",1)) {
                                status=3;
                                title=p;
			}
			continue;

                case 2:   /* find "/a" element */
                        if (namelen!=2||
                            casecmp(name,"/a",2))
                                continue;   /* ignore all other elements */
                        *s=0;
                        add_bookmark(title,url,depth);
			mem_free(url);
                        status=0;
                        continue;

                case 3:   /* find "/h3" element */
                        if (namelen!=3||
                            casecmp(name,"/h3",2))
                                continue;   /* ignore all other elements */
                        *s=0;
                        add_bookmark(title,NULL,depth);
                        status=0;
			depth++;
                        continue;
                }

	}
        if (status==2)
                mem_free(url);
smitec:
	mem_free(buf);
	d_opt=&dd_opt;
	bookmark_ld.modified=0;

}

void init_bookmarks(void)
{
        current_bookmarks_file = stracpy(options_get("bookmarks_file"));
        current_bookmarks_time = get_modification_time(options_get("bookmarks_file"));
        load_bookmarks();
}

void reinit_bookmarks(void)
{
        int need_reread = 0;

        /* we need to reread if bookmarks filename has been changed */
        if(strcmp(current_bookmarks_file, options_get("bookmarks_file"))){
                mem_free(current_bookmarks_file);
                current_bookmarks_file = stracpy(options_get("bookmarks_file"));
                need_reread=1;
        }

        /* we need to reread if bookmarks encoding has been changed */
        if(bookmark_ld.codepage != get_cp_index(options_get("bookmarks_codepage"))){
                bookmark_ld.codepage=get_cp_index(options_get("bookmarks_codepage"));
                need_reread=1;
        }

        /* we need to reread if bookmarks file has been changed on disk */
        if(current_bookmarks_time !=
           get_modification_time(options_get("bookmarks_file"))){
                current_bookmarks_time = get_modification_time(options_get("bookmarks_file"));
                need_reread=1;
        }

        if(need_reread){
                free_bookmarks();
                bookmarks.next=&bookmarks;
                bookmarks.prev=&bookmarks;
                bookmarks.type=0;
                bookmarks.depth=-1;
                bookmarks.fotr=NULL;
                load_bookmarks();
                reinit_list_window(&bookmark_ld);
        }
}

/* gets str, converts all < = > & to appropriate entity 
 * returns allocated string with result
 */
static unsigned char *convert_to_entity_string(unsigned char *str)
{
	unsigned char *dst, *p, *q;
	int size;
	
	for (size=1,p=str;*p;size+=*p=='&'?5:*p=='<'||*p=='>'||*p=='='?4:1,p++);

	dst=mem_alloc(size*sizeof(unsigned char));
	if (!dst) internal("Cannot allocate memory.\n");
	
	for (p=str,q=dst;*p;p++,q++)
	{
		switch(*p)
		{
			case '<':
			case '>':
			q[0]='&',q[1]=*p=='<'?'l':'g',q[2]='t',q[3]=';',q+=3;
			break;

			case '=':
			q[0]='&',q[1]='e',q[2]='q',q[3]=';',q+=3;
			break;

			case '&':
			q[0]='&',q[1]='a',q[2]='m',q[3]='p',q[4]=';',q+=4;
			break;

			default:
			*q=*p;
			break;
		}
	}
	*q=0;
	return dst;
}

/* writes bookmarks to disk */
void save_bookmarks(void)
{
	FILE *f;
	struct bookmark_list *b;
	int depth;
	int a;
	struct conv_table* ct;

        if (!bookmark_ld.modified ||
            !can_write_bookmarks)
                return;

        ct=get_translation_table(
                                 bookmark_ld.codepage,
                                 get_cp_index(options_get("bookmarks_codepage")));

        f=fopen(options_get("bookmarks_file"),"w");

        if (!f)
                goto save_failure;

        fputs(
	"<HTML>\n"
	"<HEAD>\n"
	"<!-- This is an automatically generated file.\n"
	"It will be read and overwritten.\n"
	"Do Not Edit! -->\n"
	"<TITLE>Links bookmarks</TITLE>\n"
	"</HEAD>\n"
	"<H1>Links bookmarks</H1>\n\n"
	"<DL><P>\n"
	,f);

        depth=0;

        foreach(b,bookmarks) {
                for (a=b->depth;a<depth;a++)
                        fprintf(f,"</DL>\n");

                depth=b->depth;
	
		if ((b->type)&1)
		{
			unsigned char *txt, *txt1;
                        txt  = convert_string(ct,b->title,strlen(b->title),NULL);
                        txt1 = convert_to_entity_string(txt);
			fprintf(f,"    <DT><H3>%s</H3>\n<DL>\n",txt1);
			mem_free(txt);
			mem_free(txt1);
			depth++;
		}
		else
		{
			unsigned char *txt1, *txt2, *txt3;
			txt1 = convert_string(ct,b->title,strlen(b->title),NULL);
			txt2 = convert_string(ct,b->url,strlen(b->url),NULL);
                        txt3 = convert_to_entity_string(txt1);
			fprintf(f,"    <DT><A HREF=\"%s\">%s</A>\n",txt2,txt3);
			mem_free(txt1);
			mem_free(txt2);
			mem_free(txt3);
		}
        }

        for (a=0;a<depth;a++)
                fprintf(f,"</DL>\n");

        fputs(
	"</DL><P>\n"
	"</HTML>\n"
	,f);

        fclose(f);

        bookmark_ld.modified=0;

        current_bookmarks_time = get_modification_time(options_get("bookmarks_file"));

        return;

save_failure:
        internal("Can't save bookmarks!\n");
}

void menu_bookmark_manager(struct terminal *term,void *fcp,struct session *ses)
{
        /* We need to reread bookmarks if they are changed on disk */
        reinit_bookmarks();

        create_list_window(&bookmark_ld,&bookmarks,term,ses);
}

void bookmark_close_dialog(struct list_description *ld)
{
        if(ld->modified){
                save_bookmarks();
        }
}
