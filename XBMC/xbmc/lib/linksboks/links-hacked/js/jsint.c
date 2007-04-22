/* jsint.c
 * (c) 2002 Mikulas Patocka (Vrxni Ideolog), Petr 'Brain' Kulhavy
 * This file is a part of the Links program, relased under GPL.
 */

/* 
 * Ve vsech upcallech plati, ze pokud dostanu ID nejakeho objektu, tak
 * javascript ma k tomu objektu pristupova prava. Jinymi slovy pristupova prava
 * se testuji v upcallech jen, aby se neco neproneslo vratnici ven. Dovnitr se
 * muze donaset vsechno, co si javascript donese, na to ma prava.
 * 
 * Navic vsechny upcally dostanou pointer na f_data_c, kde bezi javascript,
 * takze se bude moci testovat, zda javascript nesaha na f_data_c, ke kteremu
 * nema pristupova prava.
 *
 *   Brain
 */

/* Uctovani pameti:
 * js_mem_alloc/js_mem_free se bude pouzivat na struktury fax_me_tender
 * dale se bude pouzivat take ve funkcich pro praci s cookies, protoze string
 * cookies v javascript_context se tez alokuje pomoci js_mem_alloc/js_mem_free.
 */

#include "../links.h"

#ifdef JS

#include "struct.h"
#include "ipret.h"

extern int js_durchfall;
/*
vypisuje to: jaky kod byl zarazen do fronty. jaky kod byl predan interpretu do js_execute_code. jaky kod byl vykonan a ukoncen intepretem jsint_done_execution
#define TRACE_EXECUTE
*/

struct js_request {
	struct js_request *next;
	struct js_request *prev;
	int onclick_submit;	/* >=0 (znamena cislo formulare) pokud tohle je request onclick handleru u submit tlacitka nebo onsubmit handleru */
	int onsubmit;		/* dtto pro submit handler */
	int write_pos;	/* document.write position from END of document. -1 if document.write cannot be used */
	int wrote;	/* this request called document.write */
	int len;
	unsigned char code[1];
};


/* set_cookies bude parsovat takhle:
 * +....=............;...............................+   <- tohle je strinzik
 *      ^            ^
 * najdu 1. rovnase a za nim 1. strednik, najdu nasledujici rovnase a
 * nasledujici strednik, pokud to bude platne (&'='<&';') a 2. jmeno bude
 * "expires", tak to je furt jedna cookie -> poslu Mikulasovi.
 *
 * kdyz ne, tak je to jina susenka a poslu to 1. Mikulasovi
 *
 * pokud najdu ';' a za nim whitespace, za kterym neni rovnase, tak od toho
 * stredniku je to garbaz, kterou vratim do strinziku (fd->js->ctx->cookies)
 */

/* sets all cookies in fd>js->ctx->cookies */
/* final_flush means, that set_cookies was called from jsint_done_execution */






/* JESTLI V TYHLE FUNKCI BUDE NEJAKA BUGA, tak za to muze PerM, Clock a pan GNU, 
 * ktery tady kolem rusili, ze jsem se nemohl soustredit. Takze se s
 * pripadnejma reklamacema obratte na ne! 
 *
 *  Brain
 */




void jsint_set_cookies(struct f_data_c *fd, int final_flush)
{
	unsigned char *str;
	unsigned char *next;
	unsigned char *eq1, *semic1, *eq2, *semic2;

	if(!(fd->js)||!(fd->js->ctx))internal("jsint_set_cookies called with NULL context.\n");
	if (!(fd->js->ctx->cookies)||!(fd->rq))return;  /* cookies string is empty, nothing to set */
	str=fd->js->ctx->cookies;
	
a_znova:
	eq1=strchr(str,'=');
	semic1=strchr(str,';');
	
	if (!*str||(!final_flush&&!semic1))  /* na konci neni strednik a skript jeste bezi, takze to musime vratit do stringu a vypadnout */
	{
		unsigned char *bla=NULL;
		if (*str)bla=stracpy1(str);
		js_mem_free(fd->js->ctx->cookies);
		fd->js->ctx->cookies=bla;
		return;
	}

	/* ted se v str bud vyskytuje strednik, nebo skript uz skoncil */

	if (semic1&&eq1>semic1)	/* '=' je za ';' takze to pred strednikem a strednik skipnem */
	{
		str=semic1+1;
		goto a_znova;
	}

	next=semic1?semic1+1:str+strlen(str);
	if (!eq1)	/* neni tam '=', takze to preskocime */
	{
		str=next;
		goto a_znova;
	}

	/* ted by to mela bejt regulerni susenka */
	
	eq2=NULL,semic2=NULL;
	if (semic1!=NULL)
	{
		eq2=strchr(semic1+1,'=');
		semic2=strchr(semic1+1,';');
	}
	
	if (eq2&&semic1&&(final_flush||semic2))
	{
		unsigned char *p=strstr(semic1+1,"expires");
		if (!p)p=strstr(semic1+1,"EXPIRES");
		if (p&&p>semic1&&p<eq2)  /* za 1. prirazenim nasleduje "expires=", takze to je porad jedna susenka */
			next=semic2?semic2+1:str+strlen(str);
	}

	if (*next)next[-1]=0;
	
	for (;*str&&WHITECHAR(*str);str++); /* skip whitechars */

	set_cookie(fd->ses->term, fd->rq->url, str);

	str=next;
	goto a_znova;
}


int jsint_object_type(long to_je_on_Padre)
{
	return to_je_on_Padre&JS_OBJ_MASK;
}


int jsint_create(struct f_data_c *fd)
{
	struct js_state *js;

	if (fd->js) internal("javascript state present");
	if (!(js = mem_calloc(sizeof(struct js_state)))) return 1;
	if (!(js->ctx = js_create_context(fd, ((fd->id)<<JS_OBJ_MASK_SIZE)|JS_OBJ_T_DOCUMENT))) {
		mem_free(js);
		return 1;
	}
	init_list(js->queue);
	fd->js = js;
	return 0;
}

void jsint_destroy(struct f_data_c *fd)
{
	struct js_state *js = fd->js;
	fd->script_t = 0;
	if (!js) return;
	fd->js = NULL;
	pr(js_destroy_context(js->ctx)) return;
	if (js->src) mem_free(js->src);
	if (js->active) mem_free(js->active);
	free_list(js->queue);
	mem_free(js);
}

/* for <a href="javascript:..."> */
void javascript_func(struct session *ses, unsigned char *hlavne_ze_je_vecirek)
{
	unsigned char *code=get_url_data(hlavne_ze_je_vecirek);

	jsint_execute_code(current_frame(ses),code,strlen(code),-1,-1,-1);
}

/* executes or queues javascript code in frame:
	write_pos is number of bytes from the position where document.write should write to the end of document
	write_pos == -1 if it is not from <SCRIPT> statement and cannot use document.write
*/
/* data je cislo formulare pro onclick submit handler, jinak se nepouziva */
void jsint_execute_code(struct f_data_c *fd, unsigned char *code, int len, int write_pos, int onclick_submit, int onsubmit)
{
	struct js_request *r, *q;

	for (;code&&len&&*code&&((*code)==' '||(*code)==9||(*code)==13||(*code)==10);code++,len--);
	
	/*
	FUJ !!!!
	if (!strncasecmp(code,"javascript:",strlen("javascript:")))code+=strlen("javascript:");
	*/

	if (len >= 11 && !casecmp(code, "javascript:", 11)) code += 11, len -= 11;
	
        if (!options_get_bool("js_enable"))return;

#ifdef TRACE_EXECUTE
	fprintf(stderr, "Submitted: ^^%.*s^^\n", len, code);
#endif

	if (!fd->js && jsint_create(fd)) return;
	if (!(r = mem_calloc(sizeof(struct js_request) + len))) return;
	r->write_pos = write_pos;
	r->len = len;
	r->onclick_submit=onclick_submit;
	r->onsubmit=onsubmit;
	memcpy(r->code, code, len);
	if (write_pos == -1) {
		add_to_list(*(struct list_head *)fd->js->queue.prev, r);
	} else {
		/* add it beyond all <SCRIPT> requests but before non-<SCRIPT> ones */
		foreach(q, fd->js->queue) if (q->write_pos == -1) break;
		q = q->prev;
		add_at_pos(q, r);
	}
	jsint_run_queue(fd);
}

void jsint_done_execution(struct f_data_c *fd)
{
	struct js_request *r;
	struct js_state *js = fd->js;
	if (!js) {
		internal("no js in frame");
		return;
	}
	if (!js->active) {
		internal("jsint_done_execution: completion function called on inactive js");
		return;
	}

#ifdef TRACE_EXECUTE
	fprintf(stderr, "Done: ^^%.*s^^\n", js->active->len, js->active->code);
#endif

	/* accept all cookies set by the script */
	jsint_set_cookies(fd,1);

	/* js->active obsahuje request prave dobehnuteho skriptu */

	/* dobehl onclick_handler a nezaplatil (vratil false), budou se dit veci */
	if ((js->active->onclick_submit)>=0&&!js->ctx->zaplatim)
	{
		/* pokud je handler od stejneho formulare, jako je defered, tak odlozeny skok znicime a zlikvidujem prislusny onsubmit handler z fronty */
		if (js->active->onclick_submit==fd->ses->defered_data)
		{
			foreach (r,js->queue)
				/* to je onsubmit od naseho formulare, tak ho smazem */
				if (r->onsubmit==js->active->onclick_submit)
				{
					del_from_list(r);
					mem_free(r);
					break;	/* zadny dalsi onsubmit tohoto formulare uz nebude */
				}
			ses_destroy_defered_jump(fd->ses);
		}
	}

	if (js->active->write_pos == -1) mem_free(js->active), js->active = NULL;
	else {
		r = js->active; js->active = NULL;
		if (r->wrote) js->wrote = 1;
		jsint_scan_script_tags(fd);
		if (!f_is_finished(fd->f_data)) {
			fd->done = 0;
			fd->parsed_done = 0;
		}
		if (js->wrote && fd->script_t == -1) {
			fd->done = 0;
			fd->parsed_done = 0;
			fd_loaded(NULL, fd);
			js->wrote = 0;
		}
		mem_free(r);
	}
	
	/* no script to run, call defered goto-url's */
	if (!js->active&&list_empty(fd->js->queue)&&(fd->ses)&&(fd->ses->defered_url))
	{
		unsigned char *url, *target;
		
		url=stracpy(fd->ses->defered_url);
		target=stracpy(fd->ses->defered_url);
		
		goto_url_f(fd->ses,NULL,url,target,fd->ses->defered_target_base,fd->ses->defered_data,0,0,0);
		mem_free(url);
		mem_free(target);
	}
	else
		jsint_run_queue(fd);
}

void jsint_run_queue(struct f_data_c *fd)
{
	struct js_request *r;
	struct js_state *js = fd->js;

	if ((!fd->done && fd->f_data) || !js || js->active || list_empty(js->queue)) return;

	r = js->queue.next;
	del_from_list(r);
	js->active = r;
#ifdef TRACE_EXECUTE
	fprintf(stderr, "Executing: ^^%.*s^^\n", r->len, r->code);

#endif
	pr(js_execute_code(js->ctx, r->code, r->len, (void (*)(void *))jsint_done_execution));
}

/* returns: 1 - source is modified by document.write
	    0 - original source
*/

int jsint_get_source(struct f_data_c *fd, unsigned char **start, unsigned char **end)
{
	struct js_state *js = fd->js;

	if (!js || !js->src) return 0;
	if (start) *start = js->src;
	if (end) *end = js->src + js->srclen;
	return 1;
}

/*
 * tests if script running in frame "running" can access document in frame "accessed"
 * 0=no permission, 1=permission OK
 */
int jsint_can_access(struct f_data_c *running, struct f_data_c *accessed)
{
	int a;
	unsigned char *h1, *h2;
	if (!running || !accessed || !running->rq || !accessed->rq) return 0;

	h1 = get_host_name(running->rq->url);
	h2 = get_host_name(accessed->rq->url);
	a = !strcasecmp(h1, h2);
	mem_free(h1);
	mem_free(h2);
	return a;
}


/* doc_id is real document id, whithout any type */
/* fd must be a valid pointer */
struct f_data_c *jsint_find_recursive(struct f_data_c *fd, long doc_id)
{
	struct f_data_c *sub, *fdd;
	if (fd->id == doc_id) return fd;
	foreach(sub, fd->subframes) {
		if ((fdd = jsint_find_recursive(sub, doc_id))) return fdd;
	}
	return NULL;
}

/*
 *	This function finds document that has given ID
 */
struct f_data_c *jsint_find_document(long doc_id)
{
	struct f_data_c *fd;
	struct session *ses;
	int type=jsint_object_type(doc_id);
	
	if (type!=JS_OBJ_T_DOCUMENT&&type!=JS_OBJ_T_FRAME)
		{unsigned char txt[256]; snprintf(txt,256,"jsint_find_document called with type=%d\n",type);internal(txt);}
	doc_id>>=JS_OBJ_MASK_SIZE;
	foreach(ses, sessions) if ((fd = jsint_find_recursive(ses->screen, doc_id))) return fd;
	return NULL;
}

void jsint_destroy_document_description(struct f_data *f)
{
	struct js_document_description *jsd;
	if (!f)return;
	
	jsd= f->js_doc;
	if (!jsd) return;
	f->js_doc = NULL;
	/* Pro Martina: vsecky polozky vyrobene vyse se tady zase musi uvolnit (jak kurtizana v rimskejch laznich) */
	/* -------------- */
	mem_free(jsd);
}

/* Document has just loaded. Scan for <SCRIPT> tags and execute each of them */

void jsint_scan_script_tags(struct f_data_c *fd)
{
	unsigned char *name, *attr;
	int namelen;
	unsigned char *val, *e, *ee;
	unsigned char *s, *ss, *eof;
	unsigned char *start, *end;
	unsigned char *onload_code=NULL;
	int uv;
	int bs;

	if (!options_get_bool("js_enable"))return;
	if (!fd->rq || !fd->rq->ce || !fd->f_data) return;
	if (!jsint_get_source(fd, &ss, &eof)) {
		if (get_file(fd->rq, &ss, &eof)) return;
	}

	d_opt = &fd->f_data->opt;

	s = ss;
	se:
	while (s < eof && *s != '<') sp:s++;
	if (s >= eof || fd->script_t < 0)
	{
		if (onload_code && fd->script_t != -1)
		{
			jsint_execute_code(fd,onload_code,strlen(onload_code),-1,-1,-1);
		}
		fd->script_t = -1;
		goto ret;
	}
	if (s + 2 <= eof && (s[1] == '!' || s[1] == '?')) {
		s = skip_comment(s, eof);
		goto se;
	}
	if (parse_element(s, eof, &name, &namelen, &attr, &s)) goto sp;
	if (!onload_code&&namelen==4&&!casecmp(name,"BODY",4))
	{
		onload_code=get_attr_val(attr,"onload");	/* if the element doesn't have onload attribute get_attr_val returns NULL */
		goto se;
	}
		
	if (!onload_code&&namelen==3&&!casecmp(name,"IMG",3))
	{
		onload_code=get_attr_val(attr,"onload");	/* if the element doesn't have onload attribute get_attr_val returns NULL */
		goto se;
	}
		
	if (namelen != 6 || casecmp(name, "SCRIPT", 6) || s - ss < fd->script_t) goto se;
	start = end = NULL;
	if ((val = get_attr_val(attr, "src"))) {
		unsigned char *url;
		if (fd->f_data->script_href_base && ((url = join_urls(fd->f_data->script_href_base, val)))) {
			struct additional_file *af = request_additional_file(fd->f_data, url);
			mem_free(url);
			mem_free(val);
			if (!af || !af->rq) goto se;
			if (af->rq->state >= 0) goto ret;
			get_file(af->rq, &start, &end);
			if (start == end) goto se;
		} else {
			mem_free(val);
			goto se;
		}
	}
	e = s;
	uv = 0;
	bs = 0;
	while (e < eof && *e != '<') {
		es:
		if (!uv && (*e == '"' || *e == '\'')) uv = *e, bs = 0;
		else if (*e == '\\' && uv) bs = 1;
		else if (*e == uv && !bs) uv = 0;
		else bs = 0;
		e++;
	}
	if (e + 8 <= eof) {
		if (/*uv ||*/ casecmp(e, "</SCRIPT", 8)) goto es;
	} else e = eof;
	ee = e;
	while (ee < eof && *ee != '>') ee++;
	if (ee < eof) ee++;
	fd->script_t = ee - ss;
	if (!start || !end) jsint_execute_code(fd, s, e - s, eof - ee,-1,-1);
	else jsint_execute_code(fd, start, end - start, eof - ee,-1,-1);
	ret:
	if (onload_code)mem_free(onload_code);

	d_opt = &dd_opt;
}


struct hopla_mladej
{
	struct form_control *fc;
	struct form_state *fs;
};


/* Returns pointer to the object with given ID in the document, or NULL when
 * there's no such object. Document must be a valid pointer.
 *
 * Pointer type depends on type of object, caller must know the type and
 * interpret the pointer in the right way.
 */

void *jsint_find_object(struct f_data_c *document, long obj_id)
{
	int type=obj_id&JS_OBJ_MASK;
	int orig_obj_id=obj_id;
	obj_id>>=JS_OBJ_MASK_SIZE;

	switch (type)
	{
 		/* form element
		 * obj_id can be from 0 to (form_info_len-1) 
		 * returns allocated struct hopla_mladej, you must free it after use
		 */
		case JS_OBJ_T_TEXT:
		case JS_OBJ_T_PASSWORD:
		case JS_OBJ_T_TEXTAREA:
		case JS_OBJ_T_CHECKBOX:
		case JS_OBJ_T_RADIO:
		case JS_OBJ_T_SELECT:
		case JS_OBJ_T_SUBMIT:
		case JS_OBJ_T_RESET:
		case JS_OBJ_T_HIDDEN:
		case JS_OBJ_T_BUTTON:
		{
			struct hopla_mladej *hopla;

			struct form_control *fc;
			int n=document->vs->form_info_len;
			int a=0;
	
			if (obj_id<0||obj_id>=n)return NULL;
			hopla=mem_alloc(sizeof(struct hopla_mladej));
			if (!hopla)return NULL;

			if (!(document->f_data)){mem_free(hopla);return NULL;};

			foreachback(fc,document->f_data->forms)
				if (fc->g_ctrl_num==obj_id){a=1;break;}
			if (!a){mem_free(hopla);return NULL;}

			if (!(hopla->fs=find_form_state(document, fc))){mem_free(hopla);return NULL;}
			hopla->fc=fc;
			return hopla;
		}
		
		/* link 
		 * obj_id can be from 0 to (nlinks-1)
		 */
		case JS_OBJ_T_LINK:
		{
			struct link*l;
			int n;

			if (!(document->f_data))return NULL;

			l=document->f_data->links;
			n=document->f_data->nlinks;
	
			if (obj_id<0||obj_id>=n)return NULL;
			return l+obj_id;
		}
		
		/* form
		 * obj_id is form_num in struct form_control (f_data->forms)
		 */
		case JS_OBJ_T_FORM:
		{
			struct form_control *f;

			if (!(document->f_data))return NULL;
			foreachback(f, document->f_data->forms) if ((f->form_num)==obj_id)return f;
			return NULL;
		}
		
		/* anchors
		 * obj_id is position in list of all tags
		 */
		case JS_OBJ_T_ANCHOR:
		{
			struct tag *t;
			int a=0;

			if (!(document->f_data))return NULL;
			foreach(t,document->f_data->tags)
			{
				if (obj_id==a)return t;
				a++;
			}
			return NULL;
		}
		break;

		/* this is document
		 * call jsint_find_document
		 * returned value is struct f_data_c
		 */
		case JS_OBJ_T_FRAME:
		case JS_OBJ_T_DOCUMENT:
		return jsint_find_document(orig_obj_id);

		/* image
		 * returned value is struct g_object_image *
		 */
		case JS_OBJ_T_IMAGE:
#ifdef G
		if (F)
		{
			struct xlist_head *fi;

			if (!document->f_data)return NULL;
			foreach(fi,document->f_data->images)
			{
				struct g_object_image *gi;
				struct g_object_image goi;

				gi = (struct g_object_image *)((char *)fi + ((char *)&goi - (char *)&(goi.image_list)));
				if (gi->id==obj_id)return gi;
			}
			return NULL;
		}else 
#endif
		return NULL;

		default:
		internal("jsint_find_object: unknown type %d.",type);
		return NULL;  /* never called, but GCC likes it ;-) */
	}
}


long *__add_id(long *field,int *len,long id)
{
	long *p;
	int a;
	for (a=0;a<(*len);a++)	/* this object is already on the list */
		if (field[a]==id)return field;
	
	(*len)++;
	p=mem_realloc(field,(*len)*sizeof(long));
	if (!p){mem_free(field);return NULL;}

	p[(*len)-1]=id;
	return p;
}

long *__add_fd_id(long *field,int *len,long fd,long id, unsigned char *name)
{
	long *p;
	int a;
	for (a=0;a<(*len);a+=3)	/* this object is already on the list */
		if (field[a]==fd&&field[a+1]==id)return field;
	
	
	(*len)+=3;
	p=mem_realloc(field,(*len)*sizeof(long));
	if (!p){mem_free(field);return NULL;}

	p[(*len)-3]=fd;
	p[(*len)-2]=id;
	p[(*len)-1]=(name&&(*name))?(long)stracpy(name):(long)NULL;
	return p;
}

static long js_upcall_get_frame_id(void *data);

/* finds all objects with name takhle_tomu_u_nas_nadavame
 * in fd and all it's subframes with rq==NULL
 * js_ctx is f_data_c of the accessing script
 */
static long *__find_in_subframes(struct f_data_c *js_ctx, struct f_data_c *fd, long *pole_vole, int *n_items, unsigned char *takhle_tomu_u_nas_nadavame)
{
	struct f_data_c *ff;
	struct form_control *f;
#ifdef G
	struct xlist_head *fi;
#endif

	/* search frame */
	foreach(ff,fd->subframes)
		if (ff->loc&&ff->loc->name&&!strcmp(ff->loc->name,takhle_tomu_u_nas_nadavame)&&jsint_can_access(js_ctx,ff))	/* to je on! */
			if (!(pole_vole=__add_id(pole_vole,n_items,js_upcall_get_frame_id(ff))))return NULL;

	if (!(fd->f_data))goto a_je_po_ptakach;

#ifdef G
	if (F)
	/* search images */
	foreach(fi,fd->f_data->images)
	{
		struct g_object_image *gi;
		struct g_object_image goi;

		gi = (struct g_object_image *)((char *)fi + ((char *)(&goi) - (char *)(&(goi.image_list))));
		if (gi->name&&!strcmp(gi->name, takhle_tomu_u_nas_nadavame))
			if (!(pole_vole=__add_id(pole_vole,n_items,JS_OBJ_T_IMAGE+((gi->id)<<JS_OBJ_MASK_SIZE))))return NULL;
	}
#endif
	/* search forms */
	foreachback(f,fd->f_data->forms)
		if (f->form_name&&!strcmp(f->form_name,takhle_tomu_u_nas_nadavame))   /* tak tohle JE Jim Beam */
			if (!(pole_vole=__add_id(pole_vole,n_items,((f->form_num)<<JS_OBJ_MASK_SIZE)+JS_OBJ_T_FORM)))return NULL;

	/* search form elements */
	foreachback(f,fd->f_data->forms)
		if (f->name&&!strcmp(f->name,takhle_tomu_u_nas_nadavame))   /* tak tohle JE Jim Beam */
		{
			long tak_mu_to_ukaz=0;
			tak_mu_to_ukaz=(f->g_ctrl_num)<<JS_OBJ_MASK_SIZE;
			switch (f->type)
			{
				case FC_TEXT:		tak_mu_to_ukaz|=JS_OBJ_T_TEXT; break;
				case FC_PASSWORD:	tak_mu_to_ukaz|=JS_OBJ_T_PASSWORD; break;
				case FC_TEXTAREA:	tak_mu_to_ukaz|=JS_OBJ_T_TEXTAREA; break;
				case FC_CHECKBOX:	tak_mu_to_ukaz|=JS_OBJ_T_CHECKBOX; break;
				case FC_RADIO:		tak_mu_to_ukaz|=JS_OBJ_T_RADIO; break;
				case FC_IMAGE:
				case FC_SELECT:		tak_mu_to_ukaz|=JS_OBJ_T_SELECT; break;
				case FC_SUBMIT:		tak_mu_to_ukaz|=JS_OBJ_T_SUBMIT ; break;
				case FC_RESET:		tak_mu_to_ukaz|=JS_OBJ_T_RESET ; break;
				case FC_HIDDEN:		tak_mu_to_ukaz|=JS_OBJ_T_HIDDEN ; break;
				case FC_BUTTON:		tak_mu_to_ukaz|=JS_OBJ_T_BUTTON ; break;
				default: /* internal("Invalid form element type.\n"); */
				tak_mu_to_ukaz=0;break;
			}
			if (tak_mu_to_ukaz&&!(pole_vole=__add_id(pole_vole,n_items,tak_mu_to_ukaz)))return NULL;
		}
		
a_je_po_ptakach:
	/* find in all rq==NULL */
	foreach(ff,fd->subframes)
		if (!(ff->rq)) pole_vole=__find_in_subframes(js_ctx,ff,pole_vole,n_items,takhle_tomu_u_nas_nadavame);

	
	return pole_vole;
}

/* resolves name of an object, returns field of all ID's with the name
 * obj_id is object in which we're searching
 * takhle_tomu_u_nas_nadavame is the searched name
 * context is identifier of the javascript context
 * n_items is number of returned items
 *
 * on error returns NULL
 */
long *jsint_resolve(void *context, long obj_id, char *takhle_tomu_u_nas_nadavame,int *n_items)
{
	struct f_data_c *fd;
	struct f_data_c *js_ctx=(struct f_data_c*)context;
	long *pole_vole;
	*n_items=0;

	if (!takhle_tomu_u_nas_nadavame||!(*takhle_tomu_u_nas_nadavame))return NULL;
	pole_vole=mem_alloc(sizeof(long));
	if (!pole_vole)return NULL;
	switch(jsint_object_type(obj_id))
	{
		/* searched object can be a frame, image, form or a form element */
		case JS_OBJ_T_DOCUMENT:
		case JS_OBJ_T_FRAME:
		fd=jsint_find_document(obj_id);
		if (!fd||!(jsint_can_access(js_ctx,fd)))break;
		
		pole_vole=__find_in_subframes(js_ctx, fd, pole_vole, n_items, takhle_tomu_u_nas_nadavame);
		break;	

		/* searched name can be a form element */
		case JS_OBJ_T_FORM:	
		{
			struct form_control *fc=jsint_find_object(js_ctx,obj_id);
			struct form_control *f;
			if (!fc){mem_free(pole_vole);return NULL;}

			if (!(js_ctx->f_data)){mem_free(pole_vole);return NULL;}
			foreachback(f,js_ctx->f_data->forms)
			{
				if (f->form_num==fc->form_num)	/* this form */
					if (f->name&&!strcmp(f->name,takhle_tomu_u_nas_nadavame))   /* this IS Jim Beam */
					{
						long tak_mu_to_ukaz=0;
						tak_mu_to_ukaz=(f->g_ctrl_num)<<JS_OBJ_MASK_SIZE;
						switch (f->type)
						{
							case FC_TEXT:		tak_mu_to_ukaz|=JS_OBJ_T_TEXT; break;
							case FC_PASSWORD:	tak_mu_to_ukaz|=JS_OBJ_T_PASSWORD; break;
							case FC_TEXTAREA:	tak_mu_to_ukaz|=JS_OBJ_T_TEXTAREA; break;
							case FC_CHECKBOX:	tak_mu_to_ukaz|=JS_OBJ_T_CHECKBOX; break;
							case FC_RADIO:		tak_mu_to_ukaz|=JS_OBJ_T_RADIO; break;
							case FC_IMAGE:
							case FC_SELECT:		tak_mu_to_ukaz|=JS_OBJ_T_SELECT; break;
							case FC_SUBMIT:		tak_mu_to_ukaz|=JS_OBJ_T_SUBMIT ; break;
							case FC_RESET:		tak_mu_to_ukaz|=JS_OBJ_T_RESET ; break;
							case FC_HIDDEN:		tak_mu_to_ukaz|=JS_OBJ_T_HIDDEN ; break;
							case FC_BUTTON:		tak_mu_to_ukaz|=JS_OBJ_T_BUTTON ; break;
							default: tak_mu_to_ukaz=0;break;
							/* internal("Invalid form element type.\n"); */
						}
						if ((tak_mu_to_ukaz&JS_OBJ_MASK)&&!(pole_vole=__add_id(pole_vole,n_items,tak_mu_to_ukaz)))return NULL;
					}
			}
		}
		break;
	}
	if (!pole_vole)return NULL;
	if (!(*n_items)){mem_free(pole_vole);pole_vole=NULL;}
	return pole_vole;
}

/*------------------------>>>>>>>> UPCALLS <<<<<<<<-------------------------*/


/* tyhle upcally se volaji ze select smycky:

	void js_upcall_confirm(void *data)
	void js_upcall_alert(void * data)
	void js_upcall_close_window(void *data)
	void js_upcall_get_string(void *data)
	void js_upcall_goto_url(void * data)
	void js_upcall_goto_history(void * data)
	void js_upcall_set_image_src(void* data)

V nich se musi volat js_spec_vykill_timer, aby se znicil timer, ktery upcall
zavolal.

Tyto upcally MUZOU dostavat f_data_c pointer primo, protoze kdyz ten f_data_c
umre a s nim i ten JS, tak se timery znicej --- tudiz se nic nestane.
*/


static void redraw_document(struct f_data_c *f)
{
	/*
	if (F) {
		f->xl = -1;
		f->yl = -1;
		draw_to_window(f->ses->win, (void (*)(struct terminal *, void *))draw_doc, f);
	}
	*/
	draw_fd(f);
}

struct js_document_description *js_upcall_get_document_description(void *p, long doc_id)
{
	struct js_document_description *js_doc;
	struct f_data *f;
	struct f_data_c *pfd = p;
	struct f_data_c *fd;
	fd = jsint_find_document(doc_id);
	if (!fd || !fd->f_data || !jsint_can_access(pfd, fd)) return NULL;
	f = fd->f_data;
	if (f->js_doc) return f->js_doc;
	if (!(js_doc = mem_calloc(sizeof(struct js_document_description)))) return NULL;
	/* Pro Martina: pridat sem prohlizeni f_data a vytvoreni struktury */
	/* -------------- */
	return f->js_doc = js_doc;
}


/* returns ID of a document with the javascript */
long js_upcall_get_document_id(void *data)
{
	struct f_data_c *fd;
	if (!data)internal("js_upcall_get_document_id called with NULL pointer!");

	fd=(struct f_data_c*)data;
	return (((fd->id)<<JS_OBJ_MASK_SIZE)|JS_OBJ_T_DOCUMENT);
}


/* same as get_document_id, but returned type is FRAME */
static long js_upcall_get_frame_id(void *data)
{
	struct f_data_c *fd;
	if (!data)internal("js_upcall_get_document_id called with NULL pointer!");

	fd=(struct f_data_c*)data;
	return (((fd->id)<<JS_OBJ_MASK_SIZE)|JS_OBJ_T_FRAME);
}


/* writes "len" bytes starting at "str" to document */
void js_upcall_document_write(void *p, unsigned char *str, int len)
{
	int pos;
	unsigned char *s;
	struct f_data_c *fd = p;
	struct js_state *js = fd->js;
	if (!js)return;
	if (!js->active) internal("js_upcall_document_write: no request active");
	if (js->active->write_pos == -1) return;
	if (js->active->write_pos < 0) internal("js_upcall_document_write: js->active trashed");
	if (!js->src) {
		unsigned char *s, *eof;
		struct fragment *f;
		if (!fd->rq || !fd->rq->ce) return;
		defrag_entry(fd->rq->ce);
		f = fd->rq->ce->frag.next;
		if ((void *)f == &fd->rq->ce->frag || f->offset) return;
		s = f->data, eof = f->data + f->length;
		if (!(js->src = memacpy(s, eof - s))) return;
		js->srclen = eof - s;
	}
	if (!(s = mem_realloc(js->src, js->srclen + len))) return;
	js->src = s;
	if ((pos = js->srclen - js->active->write_pos) < 0) pos = 0;
	memmove(s + pos + len, s + pos, js->srclen - pos);
	memcpy(s + pos, str, len);
	js->srclen += len;
	js->active->wrote = 1;
}


/* returns title of actual document (=document in the script context) */
/* when an error occurs, returns NULL */
/* returned string should be deallocated after use */
unsigned char *js_upcall_get_title(void *data)
{
	struct f_data_c *fd;
	unsigned char *title, *t;
	struct conv_table* ct;

	if (!data)internal("js_upcall_get_title called with NULL pointer!");
	fd=(struct f_data_c *)data;

	title=mem_alloc(MAX_STR_LEN*sizeof(unsigned char));
	if (!title)internal("Cannot allocate memory in js_upcall_get_title");
	
	if (!(get_current_title(fd->ses,title,MAX_STR_LEN))){mem_free(title);return NULL;}
	if (fd->f_data)
	{
		ct=get_translation_table(fd->f_data->opt.cp,fd->f_data->cp);
		t = convert_string(ct, title, strlen(title), NULL);
		mem_free(title);
		title=t;
	}
	return title;
}


/* sets title of actual document (=document in the script context) */
/* string title will be deallocated after use */
void js_upcall_set_title(void *data, unsigned char *title)
{
	unsigned char *t;
	struct conv_table* ct;
	struct f_data_c *fd;
	int l=0;

	if (!data)internal("js_upcall_get_title called with NULL pointer!");
	fd=(struct f_data_c *)data;

	if (!title)return;
	
	if (!(fd->f_data)){mem_free(title);return;}
	if (fd->f_data->title)mem_free(fd->f_data->title);
	fd->f_data->title=init_str();
	fd->f_data->uncacheable=1;
	ct=get_translation_table(fd->f_data->cp,fd->f_data->opt.cp);
	t = convert_string(ct, title, strlen(title), NULL);
	add_to_str(&(fd->f_data->title),&l,t);
	mem_free(t);

	mem_free(title);
	redraw_document(fd);
}


/* returns URL of actual document (=document in the script context) */
/* when an error occurs, returns NULL */
/* returned string should be deallocated after use */
unsigned char *js_upcall_get_location(void *data)
{
	struct f_data_c *fd;
	unsigned char *loc;

	if (!data)internal("js_upcall_get_location called with NULL pointer!");
	fd=(struct f_data_c *)data;

	loc=mem_alloc(MAX_STR_LEN*sizeof(unsigned char));
	if (!loc)internal("Cannot allocate memory in js_upcall_get_location.\n");
	
	if (!(get_current_url(fd->ses,loc,MAX_STR_LEN))){mem_free(loc);return NULL;}
	return loc;	
}


/* returns string containing last modification date */
/* or NULL when the date is not known or when an error occurs */
unsigned char *js_upcall_document_last_modified(void *data, long document_id)
{
	struct f_data_c *fd;
	struct f_data_c *document;
	unsigned char *retval;
	
	document=jsint_find_document(document_id);
	if (!data)internal("js_upcall_document_last_modified called with NULL pointer!");
	fd=(struct f_data_c *)data;

	if (!document)return NULL;  /* document not found */
	if (!jsint_can_access(fd, document))return NULL; /* you have no permissions to look at the document */
	
	if (!fd->rq||!fd->rq->ce)return NULL;
	retval=stracpy(fd->rq->ce->last_modified);

	return retval;
}


/* returns allocated string with user-agent */
unsigned char *js_upcall_get_useragent(void *data)
{
	struct f_data_c *fd;
	unsigned char *retval=init_str();
	int l=0;
	
	if (!data)internal("js_upcall_get_useragent called with NULL pointer!");
	fd=(struct f_data_c *)data;

        if (!options_get("http_fake_useragent")) {
		add_to_str(&retval, &l, "Links (" LINKSBOKS_VERSION_STRING "; ");
		add_to_str(&retval, &l, system_name);
		add_to_str(&retval, &l, ")");
	}
	else {
		add_to_str(&retval, &l, options_get("http_fake_useragent"));
	}

	return retval;
}


/* returns allocated string with browser name */
unsigned char *js_upcall_get_appname(void)
{
        if (!options_get("http_fake_useragent"))
                return stracpy("Links");
        else
                return stracpy(options_get("http_fake_useragent"));
}


/* returns allocated string with browser name */
unsigned char *js_upcall_get_appcodename(void)
{
        return js_upcall_get_appname();
}


/* returns allocated string with browser version: "version_number (system_name)" */
unsigned char *js_upcall_get_appversion(void)
{
	unsigned char *str;
	int l=0;

        if (options_get("http_fake_useragent"))
                return stracpy(options_get("http_fake_useragent"));

	str=init_str();
	add_to_str(&str,&l,LINKSBOKS_VERSION_STRING);
	add_to_str(&str,&l," (");
	add_to_str(&str,&l,system_name);
	add_to_str(&str,&l,")");
	return str;
}


/* returns allocated string with referrer */
unsigned char *js_upcall_get_referrer(void *data)
{
	struct f_data_c *fd;
	unsigned char *retval=init_str();
	unsigned char *loc;
	int l=0;
	
	if (!data)internal("js_upcall_get_referrer called with NULL pointer!");
	fd=(struct f_data_c *)data;

        switch (options_get_int("http_referer"))
	{
		case REFERER_FAKE:
		add_to_str(&retval, &l, options_get("http_referer_fake_referer"));
		break;
		
		case REFERER_SAME_URL:
		loc=mem_alloc(MAX_STR_LEN*sizeof(unsigned char));
		if (!loc)break;
		if (!(get_current_url(fd->ses,loc,MAX_STR_LEN))){mem_free(loc);break;}
		add_to_str(&retval, &l, loc);
		mem_free(loc);
		break;

		case REFERER_REAL:
		{
			unsigned char *post;

			if (!fd->rq||!(fd->rq->prev_url))break;   /* no referrer */
			post=strchr(fd->rq->prev_url, POST_CHAR);
			if (!post)add_to_str(&retval, &l, fd->rq->prev_url);
			else add_bytes_to_str(&retval, &l, fd->rq->prev_url, post - fd->rq->prev_url);
		}
		break;
	}

	return retval;
}

/* This beast will kill all currently running scripts. */
void jsint_kill_recursively(struct f_data_c *fd)
{
	struct f_data_c *f;

        if (fd->js)
            js_downcall_game_over(fd->js->ctx);

        foreach(f,fd->subframes)
            jsint_kill_recursively(f);
}

struct gimme_js_id
{
	long id; /* id of f_data_c */
	long js_id; /* unique id of javascript */
};

/* tady se netestuje js_id, protoze BFU to chce killnout, tak to proste killne */
/* aux function for all dialog upcalls */
static void __js_kill_script_pressed(void *data)
{
	struct f_data_c *fd;
	struct gimme_js_id *jsid=(struct gimme_js_id*)data;

	fd=jsint_find_document(jsid->id);
	if (!fd)return;  /* context no longer exists */

	if (!(fd->js))return;
	js_downcall_game_over(fd->js->ctx);   /* call downcall */
}



/* aux function for js_upcall_confirm */
static void __js_upcall_confirm_ok_pressed(void *data)
{
	struct f_data_c *fd;
	struct gimme_js_id *jsid=(struct gimme_js_id*)data;

	fd=jsint_find_document(jsid->id);
	if (!fd)return;  /* context no longer exists */

	if (!(fd->js)||jsid->js_id!=fd->js->ctx->js_id)return;
	js_downcall_vezmi_true(fd->js->ctx);   /* call downcall */
}


/* aux function for js_upcall_confirm */
static void __js_upcall_confirm_cancel_pressed(void *data)
{
	struct f_data_c *fd;
	struct gimme_js_id *jsid=(struct gimme_js_id*)data;

	fd=jsint_find_document(jsid->id);
	if (!fd)return;  /* context no longer exists */

	if (!(fd->js)||jsid->js_id!=fd->js->ctx->js_id)return;
	js_downcall_vezmi_false(fd->js->ctx);   /* call downcall */
}


/* creates dialog with text s->string and buttons OK/Cancel */
/* s->string will be dealocated */
/* s will be dealocated too */
/* must be called from select loop */
void js_upcall_confirm(void *data)
{
	struct fax_me_tender_string *s=(struct fax_me_tender_string*)data;
	struct gimme_js_id* jsid;
	struct f_data_c *fd;
	struct terminal *term;
	unsigned char *txt;

	if (!s)internal("js_upcall_confirm called with NULL pointer\n");  /* to jenom kdyby na mne PerM zkousel naky oplzlosti... */

	/* context must be a valid pointer ! */
	fd=(struct f_data_c*)(s->ident);
	term=fd->ses->term;

	if (!fd->js)return;
	jsid=mem_alloc(sizeof(struct gimme_js_id));
	if (!jsid)internal("Cannot allocate memory in js_upcall_confirm.\n");
	
	/* kill timer, that called me */
	js_spec_vykill_timer(fd->js->ctx,0);

	/* fill in jsid */
	jsid->id=((fd->id)<<JS_OBJ_MASK_SIZE)|JS_OBJ_T_DOCUMENT;
	jsid->js_id=fd->js->ctx->js_id;
	
	skip_nonprintable(s->string);
	if (fd->f_data)
	{
		struct conv_table* ct;
		
		ct=get_translation_table(fd->f_data->cp,fd->f_data->opt.cp);
		txt=convert_string(ct,s->string,strlen(s->string),NULL);
	}
	else
		txt=stracpy(s->string);
	js_mem_free(s->string);
	msg_box(
		term,   /* terminal */
		getml(txt,jsid,NULL),   /* memory blocks to free */
		TXT(T_QUESTION),   /* title */
		AL_CENTER,   /* alignment */
		txt,   /* message */
		jsid,   /* data for button functions */
		3,   /* # of buttons */
		TXT(T_OK),__js_upcall_confirm_ok_pressed,B_ENTER,  /* first button */
		TXT(T_CANCEL),__js_upcall_confirm_cancel_pressed,B_ESC,  /* second button */
		TXT(T_KILL_SCRIPT), __js_kill_script_pressed,NULL
	);

	js_mem_free(s);
}


/* aux function for js_upcall_alert */
static void __js_upcall_alert_ok_pressed(void *data)
{
	struct f_data_c *fd;
	struct gimme_js_id *jsid=(struct gimme_js_id*)data;

	fd=jsint_find_document(jsid->id);
	if (!fd)return;  /* context no longer exists */

	if (!(fd->js)||jsid->js_id!=fd->js->ctx->js_id)return;
	js_downcall_vezmi_null(fd->js->ctx);   /* call downcall */
}


/* gets struct fax_me_tender_string* */
/* creates dialog with title "Alert" and message got from struct fax_me_tender_string */
/* structure and the text are both deallocated */
/* must be called from select loop */
void js_upcall_alert(void * data)
{
	struct fax_me_tender_string *s=(struct fax_me_tender_string*)data;
	struct gimme_js_id* jsid;
	struct f_data_c *fd;
	struct terminal *term;
	unsigned char *txt;

	if (!s)internal("Alert called with NULL pointer.\n"); /* to jenom kdyby na mne PerM zkousel naky oplzlosti... */

	/* context must be a valid pointer ! */
	fd=(struct f_data_c*)(s->ident);
	term=fd->ses->term;

	if (!fd->js) return;
	jsid=mem_alloc(sizeof(struct gimme_js_id));
	if (!jsid)internal("Cannot allocate memory in js_upcall_alert.\n");
	
	/* kill timer, that called me */
	js_spec_vykill_timer(fd->js->ctx,0);

	/* fill in jsid */
	jsid->id=((fd->id)<<JS_OBJ_MASK_SIZE)|JS_OBJ_T_DOCUMENT;
	jsid->js_id=fd->js->ctx->js_id;
	
	skip_nonprintable(s->string);
	if (fd->f_data)
	{
		struct conv_table* ct;
		
		ct=get_translation_table(fd->f_data->cp,fd->f_data->opt.cp);
		txt=convert_string(ct,s->string,strlen(s->string),NULL);
	}
	else
		txt=stracpy(s->string);
	js_mem_free(s->string);
	msg_box(
		term,   /* terminal */
		getml(txt,jsid,NULL),   /* memory blocks to free */
		TXT(T_ALERT),   /* title */
		AL_CENTER,   /* alignment */
		txt,   /* message */
		jsid,   /* data for button functions */
		2,   /* # of buttons */
		TXT(T_OK),__js_upcall_alert_ok_pressed,B_ENTER|B_ESC,
		TXT(T_KILL_SCRIPT), __js_kill_script_pressed,NULL
	);

	js_mem_free(s);
}


/* aux function for js_upcall_close_window */
/* tady se netestuje js_id, protoze BFU zmacklo, ze chce zavrit okno a v
 * nekterych pripadech by ho to nezavrelo (kdyby se testovalo) a to by vypadalo
 * blbe */
static void __js_upcall_close_window_yes_pressed(void *data)
{
	struct f_data_c *fd;
	struct gimme_js_id *jsid=(struct gimme_js_id*)data;

	fd=jsint_find_document(jsid->id);
	if (!fd)return;  /* context no longer exists */

	really_exit_prog(fd->ses);
}


/* asks user if he really wants to close the window and calls really_exit_prog */
/* argument is struct fax_me_tender_nothing* */
/* must be called from select loop */
void js_upcall_close_window(void *data)
{
	struct fax_me_tender_nothing *s=(struct fax_me_tender_nothing*)data;
	struct gimme_js_id* jsid;
	struct f_data_c *fd;
	struct terminal *term;

	if (!s)internal("js_upcall_close_window called with NULL pointer\n");  /* to jenom kdyby na mne PerM zkousel naky oplzlosti... */

	/* context must be a valid pointer ! */
	fd=(struct f_data_c*)(s->ident);
	term=fd->ses->term;

	if (!fd->js) return;
	jsid=mem_alloc(sizeof(struct gimme_js_id));
	if (!jsid)internal("Cannot allocate memory in js_upcall_close_window\n");
	
	/* kill timer, that called me */
	js_spec_vykill_timer(fd->js->ctx,0);

	/* fill in jsid */
	jsid->id=((fd->id)<<JS_OBJ_MASK_SIZE)|JS_OBJ_T_DOCUMENT;
	jsid->js_id=fd->js->ctx->js_id;
	
	msg_box(
		term,   /* terminal */
		getml(jsid,NULL),   /* memory blocks to free */
		TXT(T_EXIT_LINKS),   /* title */
		AL_CENTER,   /* alignment */
		TXT(T_SCRIPT_TRYING_TO_CLOSE_WINDOW),   /* message */
		jsid,   /* data for button functions */
		2,   /* # of buttons */
		TXT(T_YES),__js_upcall_close_window_yes_pressed,NULL,
		TXT(T_KILL_SCRIPT), __js_kill_script_pressed,NULL
	);

	js_mem_free(s);
}


/* returns parent window ID of the script */
long js_upcall_get_window_id(void *data)
{
	struct f_data_c *fd;
	if (!data)internal("js_upcall_get_window_id called with NULL pointer!");

	fd=(struct f_data_c*)data;
	return ((fd->id)<<JS_OBJ_MASK_SIZE)|JS_OBJ_T_FRAME;
}



/* aux function for js_upcall_get_string */
static void __js_upcall_get_string_ok_pressed(void *data, unsigned char *str)
{
	struct f_data_c *fd;
	struct gimme_js_id *jsid=(struct gimme_js_id*)data;

	fd=jsint_find_document(jsid->id);
	if (!fd)return;  /* context no longer exists */

	if (!(fd->js)||jsid->js_id!=fd->js->ctx->js_id)return;
	js_downcall_vezmi_string(fd->js->ctx, stracpy(str));   /* call downcall */
}


struct history js_get_string_history={0, {&js_get_string_history.items, &js_get_string_history.items}};


/* creates input field for string, with text s->string1, default response
 * s->string2 and buttons OK/Kill Script
 * s->string1 and s->string2 will be dealocated 
 * s will be dealocated too 
 * must be called from select loop */

void js_upcall_get_string(void *data)
{
	struct fax_me_tender_2_stringy *s=(struct fax_me_tender_2_stringy*)data;
	struct gimme_js_id* jsid;
	struct f_data_c *fd;
	struct terminal *term;
	unsigned char *str1,*str2;

	if (!s)internal("js_upcall_get_string called with NULL pointer\n");  /* to jenom kdyby na mne PerM zkousel naky oplzlosti... */

	/* context must be a valid pointer ! */
	fd=(struct f_data_c*)(s->ident);
	term=fd->ses->term;

	if (!fd->js) return;
	jsid=mem_alloc(sizeof(struct gimme_js_id));
	if (!jsid)internal("Cannot allocate memory in js_upcall_get_string\n");
	
	/* kill timer, that called me */
	js_spec_vykill_timer(fd->js->ctx,0);

	/* fill in jsid */
	jsid->id=((fd->id)<<JS_OBJ_MASK_SIZE)|JS_OBJ_T_DOCUMENT;
	jsid->js_id=fd->js->ctx->js_id;
	
	str1=stracpy(s->string1);
	str2=stracpy(s->string2);
	js_mem_free(s->string1);
	js_mem_free(s->string2);

	input_field(
		term,   /* terminal */
		getml(str1, str2,jsid,NULL),   /* mem to free */
		TXT(T_ENTER_STRING),  /* title */
		str1,   /* question */
		TXT(T_OK),   /* ok button */
		TXT(T_KILL_SCRIPT),  /* cancel button */
		jsid,   /* data for functions */
		&js_get_string_history,   /* history */
		MAX_INPUT_URL_LEN,   /* string len */
		str2,  /* string to fill the dialog with */
		0,  /* min value */
		0,  /* max value */
		NULL,  /* check fn */
		__js_upcall_get_string_ok_pressed,
		__js_kill_script_pressed
	);
	js_mem_free(s);
}


/* clears window with javascript */
/* must be called from select loop */
/* javascript must halt before calling this upcall */
void js_upcall_clear_window(void *data)
{
	/* context must be a valid pointer ! */
	struct f_data_c *fd=(struct f_data_c*)data;
	/* no jsint_destroy context or so here, it's called automatically from reinit_f_data_c */
	reinit_f_data_c(fd);
}


/* returns allocated string with window name */
unsigned char *js_upcall_get_window_name(void *data)
{
	/* context must be a valid pointer ! */
	struct f_data_c *fd=(struct f_data_c*)data;
	
	return fd->loc?stracpy(fd->loc->name):NULL;
}


/* returns allocated field of ID's of links in JS document
 * number of links is stored in len
 * if number of links is 0, returns NULL 
 * on error returns NULL too
 */
long *js_upcall_get_links(void *data, long document_id, int *len)
{
	struct f_data_c *js_ctx=(struct f_data_c*)data;
	struct f_data_c *fd;
	struct link *l;
	int a;
	long *to_je_Ono;

	fd=jsint_find_document(document_id);
	if (!js_ctx)internal("js_upcall_get_links called with NULL context pointer\n");
	if (!fd||!jsint_can_access(js_ctx,fd))return NULL;
	if (!(fd->f_data))return NULL;
	*len=fd->f_data->nlinks;
	if (!(*len))return NULL;
	l=fd->f_data->links;
	to_je_Ono=mem_alloc((*len)*sizeof(long));
	if (!to_je_Ono)internal("Cannot allocate memory in js_upcall_get_links\n");

	for (a=0;a<(*len);a++)
		/*to_je_Ono[a]=JS_OBJ_T_LINK+(((l+a)->num)<<JS_OBJ_MASK_SIZE);*/
		to_je_Ono[a]=JS_OBJ_T_LINK+(a<<JS_OBJ_MASK_SIZE);
	
	return to_je_Ono;
}


/* returns allocated string with TARGET of the link
 * if the link doesn't exist in the document, returns NULL
 */
unsigned char *js_upcall_get_link_target(void *data, long document_id, long link_id)
{
	struct f_data_c *js_ctx=(struct f_data_c*)data;
	struct f_data_c *fd;
	struct link *l;

	if (!js_ctx)internal("js_upcall_get_link_target called with NULL context pointer\n");
	if ((link_id&JS_OBJ_MASK)!=JS_OBJ_T_LINK)return NULL;   /* this isn't link */

	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return NULL;

	l=jsint_find_object(fd,link_id);
	if (!l)return NULL;

	return stracpy((l->target)?(l->target):(unsigned char *)(""));
}


/* returns allocated field of ID's of forms in JS document
 * number of forms is stored in len
 * if number of forms is 0, returns NULL 
 * on error returns NULL too
 */
long *js_upcall_get_forms(void *data, long document_id, int *len)
{
	struct f_data_c *js_ctx=(struct f_data_c *)data;
	struct f_data_c *fd;
	struct form_control *fc;
	long *to_je_Ono;
	long last=0;

	if (!js_ctx)internal("js_upcall_get_forms called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return NULL;

	if (!(fd->f_data))return NULL;

	to_je_Ono=mem_alloc(sizeof(long));
	if (!to_je_Ono)internal("Cannot allocate memory in js_upcall_get_forms\n");

	*len=0;

	foreachback(fc, fd->f_data->forms)
	{
		long *p;
		int a;

		if ((*len)&&(fc->form_num)==last)continue;
		for (a=0;a<(*len);a++)
			if ((to_je_Ono[a]>>JS_OBJ_MASK_SIZE)==(fc->form_num))goto already_have;  /* we already have this number */
		
		(*len)++;
		p=mem_realloc(to_je_Ono,(*len)*sizeof(long));
		if (!p)internal("Cannot reallocate memoru in js_upcall_get_forms\n");
		to_je_Ono=p;
		to_je_Ono[(*len)-1]=JS_OBJ_T_FORM|((fc->form_num)<<JS_OBJ_MASK_SIZE);
		last=fc->form_num;
already_have:;
	}

	if (!(*len)){mem_free(to_je_Ono);to_je_Ono=NULL;}
	
	return to_je_Ono;
}


/* returns allocated string with the form action
 * when an error occurs, returns NULL
 */
unsigned char *js_upcall_get_form_action(void *data, long document_id, long form_id)
{
	struct f_data_c *js_ctx=(struct f_data_c *)data;
	struct f_data_c *fd;
	struct form_control *fc;

	if (!js_ctx)internal("js_upcall_get_form_action called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return NULL;

	if ((form_id&JS_OBJ_MASK)!=JS_OBJ_T_FORM)return NULL;   /* this isn't form */

	fc=jsint_find_object(fd,form_id);
	if (!fc)return NULL;
	
	return stracpy(fc->action);
}



/* returns allocated string with the form target
 * when an error occurs, returns NULL
 */
unsigned char *js_upcall_get_form_target(void *data, long document_id, long form_id)
{
	struct f_data_c *js_ctx=(struct f_data_c *)data;
	struct f_data_c *fd;
	struct form_control *fc;

	if (!js_ctx)internal("js_upcall_get_form_target called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return NULL;

	if ((form_id&JS_OBJ_MASK)!=JS_OBJ_T_FORM)return NULL;   /* this isn't form */

	fc=jsint_find_object(fd,form_id);
	if (!fc)return NULL;
	
	return stracpy(fc->target);
}



/* returns allocated string with the form method
 * when an error occurs, returns NULL
 */
unsigned char *js_upcall_get_form_method(void *data, long document_id, long form_id)
{
	struct f_data_c *js_ctx=(struct f_data_c *)data;
	struct f_data_c *fd;
	struct form_control *fc;

	if (!js_ctx)internal("js_upcall_get_form_method called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return NULL;

	if ((form_id&JS_OBJ_MASK)!=JS_OBJ_T_FORM)return NULL;   /* this isn't form */

	fc=jsint_find_object(fd,form_id);
	if (!fc)return NULL;
	
	switch (fc->method)
	{
		case FM_GET:
		return stracpy("GET");

		case FM_POST:
		case FM_POST_MP:
		return stracpy("POST");

		default:
		internal("Invalid form method!\n");
		return NULL;  /* never called, but GCC likes it */
	}
}



/* returns allocated string with the form encoding (value of attribute enctype)
 * when an error occurs, returns NULL
 */
unsigned char *js_upcall_get_form_encoding(void *data, long document_id, long form_id)
{
	struct f_data_c *js_ctx=(struct f_data_c *)data;
	struct f_data_c *fd;
	struct form_control *fc;

	if (!js_ctx)internal("js_upcall_get_form_encoding called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return NULL;

	if ((form_id&JS_OBJ_MASK)!=JS_OBJ_T_FORM)return NULL;   /* this isn't form */

	fc=jsint_find_object(fd,form_id);
	if (!fc)return NULL;
	
	switch (fc->method)
	{
		case FM_GET:
		case FM_POST:
		return stracpy("application/x-www-form-urlencoded");

		case FM_POST_MP:
		return stracpy("multipart/form-data");

		default:
		internal("Invalid form method!\n");
		return NULL;  /* never called, but GCC likes it */
	}
}


/* returns allocated string containing protocol from current URL in the script context
 * on error (or there's no protocol) NULL is returned
 */
unsigned char *js_upcall_get_location_protocol(void *data)
{
	struct f_data_c *fd;
	unsigned char *loc;
	unsigned char *p;
	int l;

	if (!data)internal("js_upcall_get_location called with NULL pointer!");
	fd=(struct f_data_c *)data;

	loc=mem_alloc(MAX_STR_LEN*sizeof(unsigned char));
	if (!loc)internal("Cannot allocate memory in js_upcall_get_location_protocol\n");
	
	if (!(get_current_url(fd->ses,loc,MAX_STR_LEN))){mem_free(loc);return NULL;}

	if (parse_url(loc, &l, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL)){mem_free(loc);return NULL;}
	p=memacpy(loc,l+1);   /* l is pointing to the colon, but we want protocol with colon */
	mem_free(loc);
	return p;
}


/* returns allocated string containing port of current URL in the script context
 * on error (or there's no protocol) NULL is returned
 */
unsigned char *js_upcall_get_location_port(void *data)
{
	struct f_data_c *fd;
	unsigned char *loc;
	unsigned char *p;

	if (!data)internal("js_upcall_get_location called with NULL pointer!");
	fd=(struct f_data_c *)data;

	loc=mem_alloc(MAX_STR_LEN*sizeof(unsigned char));
	if (!loc)internal("Cannot allocate memory in js_upcall_get_location_port\n");
	
	if (!(get_current_url(fd->ses,loc,MAX_STR_LEN))){mem_free(loc);return NULL;}

	p=get_port_str(loc);
	mem_free(loc);
	return p;
}


/* returns allocated string containing hostname of current URL in the script context
 * on error (or there's no protocol) NULL is returned
 */
unsigned char *js_upcall_get_location_hostname(void *data)
{
	struct f_data_c *fd;
	unsigned char *loc;
	unsigned char *p;

	if (!data)internal("js_upcall_get_location called with NULL pointer!");
	fd=(struct f_data_c *)data;

	loc=mem_alloc(MAX_STR_LEN*sizeof(unsigned char));
	if (!loc)internal("Cannot allocate memory in js_upcall_get_location_hostname\n");
	
	if (!(get_current_url(fd->ses,loc,MAX_STR_LEN))){mem_free(loc);return NULL;}

	p=get_host_name(loc);
	mem_free(loc);
	return p;
}


/* returns allocated string containing hostname and port of current URL in the script context
 * on error (or there's no protocol) NULL is returned
 */
unsigned char *js_upcall_get_location_host(void *data)
{
	struct f_data_c *fd;
	unsigned char *loc;
	unsigned char *p, *h;
	int l1,l2;

	if (!data)internal("js_upcall_get_location called with NULL pointer!");
	fd=(struct f_data_c *)data;

	loc=mem_alloc(MAX_STR_LEN*sizeof(unsigned char));
	if (!loc)internal("Cannot allocate memory in js_upcall_get_location_host\n");
	
	if (!(get_current_url(fd->ses,loc,MAX_STR_LEN))){mem_free(loc);return NULL;}

	if (parse_url(loc, NULL, NULL, NULL, NULL, NULL, &h, &l1, NULL, &l2, NULL, NULL, NULL)){mem_free(loc);return NULL;}
	p=memacpy(h,l1+l2);
	mem_free(loc);
	return p;
}


/* returns allocated string containing pathname of current URL in the script context
 * on error (or there's no protocol) NULL is returned
 */
unsigned char *js_upcall_get_location_pathname(void *data)
{
	struct f_data_c *fd;
	unsigned char *loc;
	unsigned char *d, *p;

	if (!data)internal("js_upcall_get_location called with NULL pointer!");
	fd=(struct f_data_c *)data;

	loc=mem_alloc(MAX_STR_LEN*sizeof(unsigned char));
	if (!loc)internal("Cannot allocate memory in js_upcall_get_location_pathname\n");
	
	if (!(get_current_url(fd->ses,loc,MAX_STR_LEN))){mem_free(loc);return NULL;}

	if (parse_url(loc, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &d, NULL, NULL)){mem_free(loc);return NULL;}
	if (!d){mem_free(loc);return NULL;}
	p=memacpy(d,strcspn(d,"?"));
	mem_free(loc);
	return p;
}


/* returns allocated string containing everything after ? in current URL in the script context
 * on error (or there's no protocol) NULL is returned
 */
unsigned char *js_upcall_get_location_search(void *data)
{
	struct f_data_c *fd;
	unsigned char *loc;
	unsigned char *d, *p;

	if (!data)internal("js_upcall_get_location called with NULL pointer!");
	fd=(struct f_data_c *)data;

	loc=mem_alloc(MAX_STR_LEN*sizeof(unsigned char));
	if (!loc)internal("Canot allocate memory in js_upcall_get_location_search\n");
	
	if (!(get_current_url(fd->ses,loc,MAX_STR_LEN))){mem_free(loc);return NULL;}

	if (parse_url(loc, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &d, NULL, NULL)){mem_free(loc);return NULL;}
	if (!d){mem_free(loc);return NULL;}
	p=stracpy(strchr(d,'?'));
	mem_free(loc);
	return p;
}


/* returns allocated string containing everything between # and ? in current URL in the script context
 * on error (or there's no protocol) NULL is returned
 */
unsigned char *js_upcall_get_location_hash(void *data)
{
	struct f_data_c *fd;
	unsigned char *loc;
	unsigned char *d, *p;

	if (!data)internal("js_upcall_get_location called with NULL pointer!");
	fd=(struct f_data_c *)data;

	loc=mem_alloc(MAX_STR_LEN*sizeof(unsigned char));
	if (!loc)internal("Cannot allocate memory in js_upcall_get_location_hash.");
	
	if (!(get_current_url(fd->ses,loc,MAX_STR_LEN))){mem_free(loc);return NULL;}

	if (parse_url(loc, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &d, NULL, NULL)){mem_free(loc);return NULL;}
	if (!d){mem_free(loc);return NULL;}
	d=strchr(d,'#');
	if (!d){mem_free(loc);return NULL;}
	d++;
	p=memacpy(d,strcspn(d,"?"));
	mem_free(loc);
	return p;
}


/* returns allocated field of all form elements
 * size of the field will be stored in len
 * when an error occurs, returns NULL
 */
long *js_upcall_get_form_elements(void *data, long document_id, long form_id, int *len)
{
	struct f_data_c *js_ctx=(struct f_data_c *)data;
	struct f_data_c *fd;
	struct form_control *fc, *fc2;
	long *pole_Premysla_Zavorace;
	int b;

	if (!js_ctx)internal("js_upcall_get_form_elements called with NULL context pointer\n");
	if ((form_id&JS_OBJ_MASK)!=JS_OBJ_T_FORM)return NULL;   /* this isn't form */
	fd=jsint_find_document(document_id);
	if (!fd||!fd->f_data||!jsint_can_access(js_ctx,fd))return NULL;

	fc=jsint_find_object(fd,form_id);
	if (!fc)return NULL;

	*len=0;
	
	foreach (fc2, fd->f_data->forms)
		if (fc2->form_num==fc->form_num)(*len)++;

	if (!(*len))return NULL;
	
	pole_Premysla_Zavorace=mem_alloc((*len)*sizeof(long));
	if (!pole_Premysla_Zavorace)internal("Cannot allocate memory in js_upcall_get_form_elements.");
	
	b=0;
	foreach (fc2, fd->f_data->forms)
		if (fc2->form_num==fc->form_num)
		{
			switch (fc2->type)
			{
				case FC_TEXT:		pole_Premysla_Zavorace[b]=JS_OBJ_T_TEXT; break;
				case FC_PASSWORD:	pole_Premysla_Zavorace[b]=JS_OBJ_T_PASSWORD; break;
				case FC_TEXTAREA:	pole_Premysla_Zavorace[b]=JS_OBJ_T_TEXTAREA; break;
				case FC_CHECKBOX:	pole_Premysla_Zavorace[b]=JS_OBJ_T_CHECKBOX; break;
				case FC_RADIO:		pole_Premysla_Zavorace[b]=JS_OBJ_T_RADIO; break;
				case FC_IMAGE:
				case FC_SELECT:		pole_Premysla_Zavorace[b]=JS_OBJ_T_SELECT; break;
				case FC_SUBMIT:		pole_Premysla_Zavorace[b]=JS_OBJ_T_SUBMIT ; break;
				case FC_RESET:		pole_Premysla_Zavorace[b]=JS_OBJ_T_RESET ; break;
				case FC_HIDDEN:		pole_Premysla_Zavorace[b]=JS_OBJ_T_HIDDEN ; break;
				case FC_BUTTON:		pole_Premysla_Zavorace[b]=JS_OBJ_T_BUTTON ; break;
				default: /* internal("Invalid form element type.\n"); */
				(*len)--;
				continue;
			}
			pole_Premysla_Zavorace[b]|=((fc2->g_ctrl_num)<<JS_OBJ_MASK_SIZE);
			b++;
		}
	return pole_Premysla_Zavorace;
}


/* returns allocated field with anchors
 * size of the field is stored in len
 * when there're no anchors, *len is 0 and NULL is returned
 * on error NULL is returned
 */
long *js_upcall_get_anchors(void *hej_Hombre, long document_id, int *len)
{
	struct f_data_c *js_ctx=(struct f_data_c*)hej_Hombre;
	struct f_data_c *fd;
	struct tag *t;
	int a;
	long *to_je_Ono;
	*len=0;

	if (!js_ctx)internal("js_upcall_get_anchors called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return NULL;

	if (!(fd->f_data))return NULL;
	foreach(t,fd->f_data->tags)(*len)++;
	if (!(*len))return NULL;
	to_je_Ono=mem_alloc((*len)*sizeof(long));
	if (!to_je_Ono)internal("Cannot allocate memory in js_upcall_get_anchors\n");

	a=0;
	foreach(t,fd->f_data->tags)
	{
		to_je_Ono[a]=JS_OBJ_T_ANCHOR+(a<<JS_OBJ_MASK_SIZE);
		a++;
	}
	return to_je_Ono;
	
}


/* returns whether radio or checkbox is checked
 * return value: 0=not checked
 *               1=checked
 *              -1=error
 */
int js_upcall_get_checkbox_radio_checked(void *smirak, long document_id, long radio_tv_id)
{
	struct f_data_c *js_ctx=(struct f_data_c*)smirak;
	struct f_data_c *fd;
	struct hopla_mladej *hopla;
	int state;

	if (!js_ctx)internal("js_upcall_get_checkbox_radio_checked called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return -1;

	if ((radio_tv_id&JS_OBJ_MASK)!=JS_OBJ_T_RADIO&&(radio_tv_id&JS_OBJ_MASK)!=JS_OBJ_T_CHECKBOX)return -1;   /* this isn't radio nor TV */

	hopla=jsint_find_object(fd,radio_tv_id);
	if (!hopla)return -1;
	
	state=hopla->fs->state;
	mem_free(hopla);
	return state;
}


/* checks/unchecks radio or checkbox 
 */
void js_upcall_set_checkbox_radio_checked(void *smirak, long document_id, long radio_tv_id, int value)
{
	struct f_data_c *js_ctx=(struct f_data_c*)smirak;
	struct f_data_c *fd;
	struct hopla_mladej *hopla;

	if (!js_ctx)internal("js_upcall_set_checkbox_radio_checked called with NULL context pointer\n");
	if ((radio_tv_id&JS_OBJ_MASK)!=JS_OBJ_T_RADIO&&(radio_tv_id&JS_OBJ_MASK)!=JS_OBJ_T_CHECKBOX)return;   /* this isn't radio nor TV */
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return;

	hopla=jsint_find_object(fd,radio_tv_id);
	if (!hopla)return;
	
	hopla->fs->state=!!value;
	mem_free(hopla);
	redraw_document(fd);
}


/* returns whether radio or checkbox is checked
 * return value: 0=default not checked
 *		 1=default checked
 *		-1=error
 */
int js_upcall_get_checkbox_radio_default_checked(void *bidak_smirak, long document_id, long radio_tv_id)
{
	struct f_data_c *js_ctx=(struct f_data_c*)bidak_smirak;
	struct f_data_c *fd;
	struct hopla_mladej *hopla;
	int default_checked;

	if (!js_ctx)internal("js_upcall_get_checkbox_radio_default_checked called with NULL context pointer\n");
	if ((radio_tv_id&JS_OBJ_MASK)!=JS_OBJ_T_RADIO&&(radio_tv_id&JS_OBJ_MASK)!=JS_OBJ_T_CHECKBOX)return -1;   /* this isn't radio nor TV */
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return -1;

	hopla=jsint_find_object(fd,radio_tv_id);
	if (!hopla)return -1;
	
	default_checked=hopla->fc->default_state;
	mem_free(hopla);
	return default_checked;
}


/* sets radio/checkbox default_checked in the form
 */
void js_upcall_set_checkbox_radio_default_checked(void *bidak_smirak, long document_id, long radio_tv_id, int value)
{
	struct f_data_c *js_ctx=(struct f_data_c*)bidak_smirak;
	struct f_data_c *fd;
	struct hopla_mladej *hopla;
	int something_changed;
	value=!!value;

	if (!js_ctx)internal("js_upcall_set_checkbox_radio_default_checked called with NULL context pointer\n");
	if ((radio_tv_id&JS_OBJ_MASK)!=JS_OBJ_T_RADIO&&(radio_tv_id&JS_OBJ_MASK)!=JS_OBJ_T_CHECKBOX)return;   /* this isn't radio nor TV */
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return;

	hopla=jsint_find_object(fd,radio_tv_id);
	if (!hopla)return;
	
	something_changed=(hopla->fc->default_state)^value;
	hopla->fc->default_state=value;
	fd->f_data->uncacheable|=something_changed;
	mem_free(hopla);
}


/* returns allocated string with name of the form element
 * don't forget to free the string after use
 * on error returns NULL
 */
unsigned char *js_upcall_get_form_element_name(void *bidak, long document_id, long ksunt_id)
{
	struct f_data_c *js_ctx=(struct f_data_c*)bidak;
	struct f_data_c *fd;
	struct hopla_mladej *hopla;
	unsigned char *hele_ho_bidaka;

	if (!js_ctx)internal("js_upcall_get_form_element_name called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return NULL;

	switch (ksunt_id&JS_OBJ_MASK)
	{
		case JS_OBJ_T_RADIO:
		case JS_OBJ_T_TEXT:
		case JS_OBJ_T_PASSWORD:
		case JS_OBJ_T_TEXTAREA:
		case JS_OBJ_T_CHECKBOX:
		case JS_OBJ_T_SELECT:
		case JS_OBJ_T_SUBMIT:
		case JS_OBJ_T_RESET:
		case JS_OBJ_T_HIDDEN:
		case JS_OBJ_T_BUTTON:
		break;

		default:
		return NULL;   /* To neni Jim Beam! */
	}

	hopla=jsint_find_object(fd,ksunt_id);
	if (!hopla)return NULL;
	
	hele_ho_bidaka=stracpy(hopla->fc->name);
	mem_free(hopla);
	return hele_ho_bidaka;
}


/* sets name of the form element
 * name is allocated string, this function deallocates it
 */
void js_upcall_set_form_element_name(void *bidak, long document_id, long ksunt_id, unsigned char *name)
{
	struct f_data_c *js_ctx=(struct f_data_c*)bidak;
	struct f_data_c *fd;
	struct hopla_mladej *hopla;

	if (!js_ctx)internal("js_upcall_set_form_element_name called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd)){if (name)mem_free(name);return;}

	switch (ksunt_id&JS_OBJ_MASK)
	{
		case JS_OBJ_T_RADIO:
		case JS_OBJ_T_TEXT:
		case JS_OBJ_T_PASSWORD:
		case JS_OBJ_T_TEXTAREA:
		case JS_OBJ_T_CHECKBOX:
		case JS_OBJ_T_SELECT:
		case JS_OBJ_T_SUBMIT:
		case JS_OBJ_T_RESET:
		case JS_OBJ_T_HIDDEN:
		case JS_OBJ_T_BUTTON:
		break;

		default:
		if(name) mem_free(name);
		return;   /* To neni Jim Beam! */
	}

	hopla=jsint_find_object(fd,ksunt_id);
	if (!hopla){if (name)mem_free(name);return;}
	
	if ((name||(hopla->fc->name))&&strcmp(name,hopla->fc->name))
	{
		mem_free(hopla->fc->name);
		hopla->fc->name=stracpy(name);
		fd->f_data->uncacheable=1;
	}
	mem_free(hopla);
	if(name) mem_free(name);
}


/* returns allocated string with value of VALUE attribute of the form element
 * on error returns NULL
 * don't forget to free the string after use
 */
unsigned char *js_upcall_get_form_element_default_value(void *bidak, long document_id, long ksunt_id)
{
	struct f_data_c *js_ctx=(struct f_data_c*)bidak;
	struct f_data_c *fd;
	struct hopla_mladej *hopla;
	unsigned char *hele_ho_bidaka;

	if (!js_ctx)internal("js_upcall_get_form_element_default_value called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return NULL;

	switch (ksunt_id&JS_OBJ_MASK)
	{
		case JS_OBJ_T_RADIO:
		case JS_OBJ_T_TEXT:
		case JS_OBJ_T_PASSWORD:
		case JS_OBJ_T_TEXTAREA:
		case JS_OBJ_T_CHECKBOX:
		case JS_OBJ_T_SELECT:
		case JS_OBJ_T_SUBMIT:
		case JS_OBJ_T_RESET:
		case JS_OBJ_T_HIDDEN:
		break;

		default:
		return NULL;   /* To neni Jim Beam! */
	}

	hopla=jsint_find_object(fd,ksunt_id);
	if (!hopla)return NULL;
	
	hele_ho_bidaka=stracpy(hopla->fc->default_value);
	mem_free(hopla);
	return hele_ho_bidaka;
}


/* sets attribute VALUE of the form element
 * name is allocated string that, this function frees it
 * when name is NULL default value will be empty
 */
void js_upcall_set_form_element_default_value(void *bidak, long document_id, long ksunt_id, unsigned char *name)
{
	struct f_data_c *js_ctx=(struct f_data_c*)bidak;
	struct f_data_c *fd;
	struct hopla_mladej *hopla;

	if (!js_ctx)internal("js_upcall_set_form_element_default_value called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd)){if (name)mem_free(name);return;}

	switch (ksunt_id&JS_OBJ_MASK)
	{
		case JS_OBJ_T_RADIO:
		case JS_OBJ_T_TEXT:
		case JS_OBJ_T_PASSWORD:
		case JS_OBJ_T_TEXTAREA:
		case JS_OBJ_T_CHECKBOX:
		case JS_OBJ_T_SELECT:
		case JS_OBJ_T_SUBMIT:
		case JS_OBJ_T_RESET:
		case JS_OBJ_T_HIDDEN:
		break;

		default:
		if (name)mem_free(name);
		return;   /* To neni Jim Beam! */
	}

	hopla=jsint_find_object(fd,ksunt_id);
	if (!hopla){if (name)mem_free(name);return;}
	
	if ((name||(hopla->fc->default_value))&&strcmp(name,hopla->fc->default_value))
	{
		mem_free(hopla->fc->default_value);
		hopla->fc->default_value=stracpy(name);
		fd->f_data->uncacheable=1;
	}
	mem_free(hopla);
	if (name)mem_free(name);
}


/* returns allocated string with actual value of password, text or textarea element
 * on error returns NULL
 * don't forget to free the string after use
 */
unsigned char *js_upcall_get_form_element_value(void *bidak, long document_id, long ksunt_id)
{
	struct f_data_c *js_ctx=(struct f_data_c*)bidak;
	struct f_data_c *fd;
	struct hopla_mladej *hopla;
	unsigned char *hele_ho_bidaka;

	if (!js_ctx)internal("js_upcall_get_form_element_value called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return NULL;

	switch (ksunt_id&JS_OBJ_MASK)
	{
		case JS_OBJ_T_TEXT:
		case JS_OBJ_T_PASSWORD:
		case JS_OBJ_T_TEXTAREA:
		break;

		default:
		return NULL;   /* To neni Jim Beam! */
	}

	hopla=jsint_find_object(fd,ksunt_id);
	if (!hopla)return NULL;
	
	hele_ho_bidaka=stracpy(hopla->fs->value);
	mem_free(hopla);
	return hele_ho_bidaka;
}


/* sets actual value of password, text or textarea element
 * name is allocated string that, this function frees it
 * when name is NULL default value will be empty
 */
void js_upcall_set_form_element_value(void *bidak, long document_id, long ksunt_id, unsigned char *name)
{
	struct f_data_c *js_ctx=(struct f_data_c*)bidak;
	struct f_data_c *fd;
	struct hopla_mladej *hopla;

	if (!js_ctx)internal("js_upcall_set_form_element_value called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd)){if (name)mem_free(name);return;}

	fd=jsint_find_document(document_id);

	switch (ksunt_id&JS_OBJ_MASK)
	{
		case JS_OBJ_T_TEXT:
		case JS_OBJ_T_PASSWORD:
		case JS_OBJ_T_TEXTAREA:
		break;

		default:
		if (name)mem_free(name);
		return;   /* To neni Jim Beam! */
	}

	hopla=jsint_find_object(fd,ksunt_id);
	if (!hopla){if (name)mem_free(name);return;}
	
	mem_free(hopla->fs->value);
	hopla->fs->value=stracpy(name);
	if (hopla->fs->state > strlen(hopla->fs->value))
		hopla->fs->state = strlen(hopla->fs->value);
	if ((ksunt_id&JS_OBJ_MASK) != JS_OBJ_T_TEXTAREA) {
		if (hopla->fs->vpos > strlen(hopla->fs->value))
			hopla->fs->vpos = strlen(hopla->fs->value);
	}
	mem_free(hopla);
	if (name)mem_free(name);
	redraw_document(fd);
}


/* emulates click on everything */
void js_upcall_click(void *bidak, long document_id, long elem_id)
{
	struct f_data_c *js_ctx=(struct f_data_c*)bidak;
	struct f_data_c *fd;

	if (!js_ctx)internal("js_upcall_click called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return;

	switch (elem_id&JS_OBJ_MASK)
	{
		case JS_OBJ_T_CHECKBOX:
		case JS_OBJ_T_RADIO:
		case JS_OBJ_T_SUBMIT:
		case JS_OBJ_T_RESET:
		case JS_OBJ_T_BUTTON:
		{
			struct hopla_mladej *hopla;
			int a;
			struct link *l;

			if (!fd->f_data)return;
			hopla=jsint_find_object(fd,elem_id);
			if (!hopla)return;
	
			for (a=0;a<fd->f_data->nlinks;a++)
			{
				l=&(fd->f_data->links[a]);
				if (l->form&&l->form==hopla->fc)	/* to je on! */
				{
					int old_link=fd->vs->current_link;
					fd->vs->current_link=a;
					enter(fd->ses,fd,0,0);
					draw_fd(fd);
					fd->vs->current_link=old_link;
					change_screen_status(fd->ses);
					print_screen_status(fd->ses);
					break;
				}
			}
			mem_free(hopla);
		}
		break;
	}
}

#ifdef G
static int __find_go_link_num;
static struct g_object *__to_je_on_bidak;
static void __find_go(struct g_object *p, struct g_object *c)
{
	if (c->draw==(void (*)(struct f_data_c *, struct g_object *, int, int))g_text_draw)
		if (((struct g_object_text*)c)->link_num==__find_go_link_num){__to_je_on_bidak=c;return;}
	if (c->get_list)c->get_list(c,__find_go);
}
#endif

/* emulates focus on password, text and textarea */
void js_upcall_focus(void *bidak, long document_id, long elem_id)
{
	struct f_data_c *js_ctx=(struct f_data_c*)bidak;
	struct f_data_c *fd;

	if (!js_ctx)internal("js_upcall_focus called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return;

	switch (elem_id&JS_OBJ_MASK)
	{
		case JS_OBJ_T_TEXT:
		case JS_OBJ_T_PASSWORD:
		case JS_OBJ_T_TEXTAREA:
		{
			struct hopla_mladej *hopla;
			int a;
			struct link *l;

			if (!fd->f_data)return;
			hopla=jsint_find_object(fd,elem_id);
			if (!hopla)return;
	
			for (a=0;a<fd->f_data->nlinks;a++)
			{
				l=&(fd->f_data->links[a]);
				if (l->form&&l->form==hopla->fc)	/* to je on! */
				{
					fd->vs->current_link=a;
#ifdef G
					if (F)
					{
						fd->ses->locked_link=1;
						__to_je_on_bidak=NULL;
						__find_go_link_num=a;

						/* tak tedka tu budu carovat g_object_text, kterej patri k tomuhle linku */
						if (fd->f_data->root->get_list)fd->f_data->root->get_list(fd->f_data->root,__find_go);
						fd->f_data->locked_on=__to_je_on_bidak;
					}
#endif
					if (l->js_event&&l->js_event->focus_code)
						jsint_execute_code(fd,l->js_event->focus_code,strlen(l->js_event->focus_code),-1,-1,-1);

					draw_fd(fd);
					change_screen_status(fd->ses);
					print_screen_status(fd->ses);
					break;
				}
			}
			mem_free(hopla);
		}
		break;
	}
}

/* emulates focus on password, text and textarea */
void js_upcall_blur(void *bidak, long document_id, long elem_id)
{
	struct f_data_c *js_ctx=(struct f_data_c*)bidak;
	struct f_data_c *fd;

	if (!js_ctx)internal("js_upcall_blur called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return;

	/* in text mode do nothing, because we don't know where to go with cursor */
#ifdef G
	if (F)
		switch (elem_id&JS_OBJ_MASK)
		{
			case JS_OBJ_T_TEXT:
			case JS_OBJ_T_PASSWORD:
			case JS_OBJ_T_TEXTAREA:
			{
				struct hopla_mladej *hopla;
				int a;
				struct link *l;
	
				if (!fd->f_data)return;
				hopla=jsint_find_object(fd,elem_id);
				if (!hopla)return;
		
				for (a=0;a<fd->f_data->nlinks;a++)
				{
					l=&(fd->f_data->links[a]);
					if (l->form&&l->form==hopla->fc)	/* to je on! */
					{
						fd->ses->locked_link=0;
						if (l->js_event&&l->js_event->blur_code)
							jsint_execute_code(fd,l->js_event->blur_code,strlen(l->js_event->blur_code),-1,-1,-1);

						/* pro jistotu */
						draw_fd(fd);
						change_screen_status(fd->ses);
						print_screen_status(fd->ses);
						break;
					}
				}
				mem_free(hopla);
			}
			break;
		}
#endif
}

/* emulates submit of a form */
void js_upcall_submit(void *bidak, long document_id, long form_id)
{
	struct f_data_c *js_ctx=(struct f_data_c*)bidak;
	struct f_data_c *fd;
	struct form_control *form;
	int has_onsubmit;
	unsigned char *u;

	if (!js_ctx)internal("js_upcall_submit called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return;

	if ((form_id&JS_OBJ_MASK)!=JS_OBJ_T_FORM)return;
	form=jsint_find_object(fd,form_id);
	if (!form)return;

	u=get_form_url(fd->ses,fd,form,&has_onsubmit);
	goto_url_f(fd->ses,NULL,u,NULL,fd,form->form_num, has_onsubmit,0,0);
	mem_free(u);
	draw_fd(fd);
	change_screen_status(fd->ses);
	print_screen_status(fd->ses);
}


/* emulates reset of a form */
void js_upcall_reset(void *bidak, long document_id, long form_id)
{
	struct f_data_c *js_ctx=(struct f_data_c*)bidak;
	struct f_data_c *fd;

	if (!js_ctx)internal("js_upcall_reset called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return;
	if ((form_id&JS_OBJ_MASK)!=JS_OBJ_T_FORM)return;
	if (!fd->f_data)return;
		
	reset_form(fd,form_id>>JS_OBJ_MASK_SIZE);
	draw_fd(fd);
	change_screen_status(fd->ses);
	print_screen_status(fd->ses);
}

/* returns length (number of radio buttons) of a radio
 * on error returns -1
 */
int js_upcall_get_radio_length(void *p, long document_id, long radio_id)
{
	struct f_data_c *js_ctx=(struct f_data_c*)p;
	struct f_data_c *fd;
	struct form_control *f;
	struct hopla_mladej *hopla;
	struct form_control *radio;
	int count=0;

	if (!js_ctx)internal("js_upcall_get_radio_length called with NULL context pointer\n");
	if ((radio_id&JS_OBJ_MASK)!=JS_OBJ_T_RADIO) return -1;
	fd=jsint_find_document(document_id);
	if (!fd||!fd->f_data||!jsint_can_access(js_ctx,fd))return -1;

	hopla=jsint_find_object(fd,radio_id);
	if (!hopla)return -1;
	radio=hopla->fc;

	/* find form elements with the same type, form_num (belonging to the same form) and name */
	foreachback(f,fd->f_data->forms)
		if (f->type==radio->type&&f->form_num==radio->form_num&&!strcmp(radio->name,f->name))count++;
	mem_free(hopla);
	return count;
}

/* returns number of items in a select form element 
 * on error returns -1
 */
int js_upcall_get_select_length(void *p, long document_id, long select_id)
{
	int l;
	struct f_data_c *js_ctx=(struct f_data_c*)p;
	struct f_data_c *fd;
	struct hopla_mladej *hopla;

	if (!js_ctx)internal("js_upcall_get_select_length called with NULL context pointer\n");
	if ((select_id&JS_OBJ_MASK)!=JS_OBJ_T_SELECT) return -1;
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return -1;

	hopla=jsint_find_object(fd,select_id);
	if (!hopla)return -1;

	l = hopla->fc->nvalues;
	mem_free(hopla);
	return l;
}


/* returns allocated field of select items
 * don't forget to free: text and value of each item and the field
 * on error returns NULL
 * n is number of items in the field
 */
struct js_select_item* js_upcall_get_select_options(void *p, long document_id, long select_id, int *n)
{
	struct f_data_c *js_ctx=(struct f_data_c*)p;
	struct f_data_c *fd;
	struct hopla_mladej *hopla;
	struct js_select_item* elektricke_pole;
	int ukazme_si_na_nej;

	*n=0;
	if (!js_ctx)internal("js_upcall_get_select_length called with NULL context pointer\n");
	if ((select_id&JS_OBJ_MASK)!=JS_OBJ_T_SELECT) return NULL;
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return NULL;

	hopla=jsint_find_object(fd,select_id);
	if (!hopla)return NULL;

	*n=hopla->fc->nvalues;
	elektricke_pole=mem_alloc((*n)*sizeof(struct js_select_item));
	if (!elektricke_pole)internal("Cannot allocate memory in js_upcall_get_select_options\n");

	for (ukazme_si_na_nej=0;ukazme_si_na_nej<(*n);ukazme_si_na_nej++)
	{
		elektricke_pole[ukazme_si_na_nej].text=stracpy((hopla->fc->labels)[ukazme_si_na_nej]);
		elektricke_pole[ukazme_si_na_nej].value=stracpy((hopla->fc->values)[ukazme_si_na_nej]);
		elektricke_pole[ukazme_si_na_nej].selected=(ukazme_si_na_nej==(hopla->fs->state));
		elektricke_pole[ukazme_si_na_nej].default_selected=(ukazme_si_na_nej==(hopla->fc->default_state));
	}
	mem_free(hopla);
	return elektricke_pole;
}

/* returns index of just selected item in a select form element
 * on error returns -1
 */
int js_upcall_get_select_index(void *p, long document_id, long select_id)
{
	struct f_data_c *js_ctx=(struct f_data_c*)p;
	struct f_data_c *fd;
	struct hopla_mladej *hopla;
	int l;

	if (!js_ctx)internal("js_upcall_get_select_length called with NULL context pointer\n");
	if ((select_id&JS_OBJ_MASK)!=JS_OBJ_T_SELECT) return -1;
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return -1;

	hopla=jsint_find_object(fd,select_id);
	if (!hopla)return -1;

	l = hopla->fs->state;
	mem_free(hopla);
	return l;
}


struct gimme_js_id_string
{
	long id;
	long js_id;
	unsigned char *string;
	signed int n;
};

/* open a link in a new xterm */
void send_vodevri_v_novym_vokne(struct terminal *term, void (*open_window)(struct terminal *term, unsigned char *, unsigned char *), struct session *ses)
{
        if (ses->dn_url) {
		unsigned char *enc_url = encode_url(ses->dn_url);
		open_window(term, path_to_exe, enc_url);
		mem_free(enc_url);
	}
}

/* aux function for js_upcall_goto_url */
static void __js_upcall_goto_url_ok_pressed(void *data)
{
	struct f_data_c *fd;
	struct gimme_js_id_string *jsid=(struct gimme_js_id_string*)data;

	fd=jsint_find_document(jsid->id);
	if (!fd)return;  /* context no longer exists */

	/* it doesn't matter, that fd->js is NULL */
	if (jsid->n&&can_open_in_new(fd->ses->term)) /* open in new window */
        {
		if (fd->ses->dn_url) mem_free(fd->ses->dn_url);
		fd->ses->dn_url=stracpy(jsid->string);
                /* open_in_new_window(fd->ses->term, send_vodevri_v_novym_vokne, fd->ses); */
                /* Temporary (unofficial) fix - just use first possible way */
                {
                        struct open_in_new *oin=get_open_in_new(fd->ses->term->environment);
                        if (oin && oin[0].text) {
                                send_vodevri_v_novym_vokne(fd->ses->term, oin[0].fn, fd->ses);
                                mem_free(oin);
                                return;
                        }
                }
        }
	else 
		goto_url(fd->ses,jsid->string);

	if (!(fd->js)||jsid->js_id!=fd->js->ctx->js_id)return;
	js_downcall_vezmi_null(fd->js->ctx);   /* call downcall */
}


/* aux function for js_upcall_goto_url */
static void __js_upcall_goto_url_cancel_pressed(void *data)
{
	struct f_data_c *fd;
	struct gimme_js_id *jsid=(struct gimme_js_id*)data;

	fd=jsint_find_document(jsid->id);
	if (!fd)return;  /* context no longer exists */

	if (!(fd->js)||jsid->js_id!=fd->js->ctx->js_id)return;
	js_downcall_vezmi_null(fd->js->ctx);   /* call downcall */
}


/* gets struct fax_me_tender_int_string */
/* asks user whether to go to the url or not */
/* structure and the text are both deallocated */
/* must be called from select loop */
/* if num in fax_me_tender_int_string is not null, open in a new window */
void js_upcall_goto_url(void * data)
{
	struct fax_me_tender_int_string *s=(struct fax_me_tender_int_string*)data;
	struct gimme_js_id_string* jsid;
	struct f_data_c *fd;
	struct terminal *term;

	fd=(struct f_data_c*)(s->ident);
	term=fd->ses->term;

	if (!fd->js) return;

	/* kill timer, that called me */
	js_spec_vykill_timer(fd->js->ctx,0);

	if (!s)internal("js_upcall_goto_url called with NULL pointer\n");

	if (!s->string){js_mem_free(data);goto goto_url_failed;}
	jsid=mem_alloc(sizeof(struct gimme_js_id_string));
	if (!jsid)internal("Cannot allocate memory in js_upcall_goto_url.\n");
	
	/* context must be a valid pointer ! */
	/* fill in jsid */
	jsid->id=((fd->id)<<JS_OBJ_MASK_SIZE)|JS_OBJ_T_DOCUMENT;
	jsid->js_id=fd->js->ctx->js_id;
	if (fd->loc&&fd->loc->url) jsid->string=join_urls(fd->loc->url,s->string);
	else jsid->string=stracpy(s->string);
	if (!(jsid->string)){mem_free(jsid);js_mem_free(s->string);js_mem_free(data);goto goto_url_failed;}
	/* goto the same url */
	{
		unsigned char txt[MAX_STR_LEN];
		void *p;

		p=get_current_url(fd->ses,txt,MAX_STR_LEN);
		if (p&&fd->loc&&fd->loc->url&&!strcmp(txt,jsid->string)){mem_free(jsid->string);mem_free(jsid);js_mem_free(s->string);js_mem_free(data);goto goto_url_failed;}
	}
	js_mem_free(s->string);
	jsid->n=s->num;
	
	msg_box(
		term,   /* terminal */
		getml(jsid->string,jsid,NULL),   /* memory blocks to free */
		TXT(T_GOTO_URL),   /* title */
		AL_CENTER|AL_EXTD_TEXT,   /* alignment */
		jsid->n?TXT(T_JS_IS_ATTEMPTING_TO_OPEN_NEW_WINDOW_WITH_URL):TXT(T_JS_IS_ATTEMPTING_TO_GO_TO_URL), " \"",jsid->string,"\".",NULL,   /* message */
		jsid,   /* data for button functions */
		3,   /* # of buttons */
		TXT(T_ALLOW),__js_upcall_goto_url_ok_pressed,B_ENTER,
		TXT(T_REJECT),__js_upcall_goto_url_cancel_pressed,B_ESC,
		TXT(T_KILL_SCRIPT), __js_kill_script_pressed,NULL  /* dirty trick: gimme_js_id_string and gimme_js_id begins with the same long */
	);

	js_mem_free(s);
	return;
goto_url_failed:
	js_downcall_vezmi_null(fd->js->ctx);   /* call downcall */
	return;
}


/* returns number of items in history */
int js_upcall_get_history_length(void *context)
{
	struct f_data_c *fd=(struct f_data_c*)context;
	struct location *l;
	int len=0;

	if (!fd)internal("PerMe, PerMe, ja si te podam!\n");

	foreach(l,fd->ses->history)len++;

	return len;
}


/* aux function for js_upcall_goto_history */
static void __js_upcall_goto_history_ok_pressed(void *data)
{
	struct f_data_c *fd;
	struct gimme_js_id_string *jsid=(struct gimme_js_id_string*)data;
	struct location *loc;
	int a;

	fd=jsint_find_document(jsid->id);
	if (!fd)return;  /* context no longer exists */
	
        a = jsid->n;
        if (a < 0) a = -a;

        if (!list_empty(fd->ses->history))
		for (loc = cur_loc(fd->ses); loc != (void *) &fd->ses->history && a > 0;
					loc = (jsid->n < 0 ? loc->next : loc->prev)) a--;

        if (a > 0 &&
            (fd->js) &&
            jsid->js_id==fd->js->ctx->js_id) {
		/* call downcall */
		js_downcall_vezmi_null(fd->js->ctx);
		return;
	}

        if (a == 0 &&
            loc != (void *) &fd->ses->history)
		go_backwards(fd->ses->term, loc, fd->ses);
}


/* gets struct fax_me_tender_int_string 
 * either num or string is set, but not both, the other must be NULL
 * asks user whether to go to the url or not
 * structure and the text are both deallocated
 * must be called from select loop
 * number can be:
 * 		>0	go forward in history (not supported)
 *		 0	do nothing (means use string)
 * 		<0	go backward in history (supported :) )
 * if string is defined - find appropriate history item and go to the url, when
 * the URL doesn't exist do nothing
 * 
 * JAK TO FUNGUJE:
 * string se prekonvertuje na cislo (projde se historie)
 * po zmacknuti OK se spocita delka historie a pokud je dostatecna, n-krat
 * zavola go_back. Pokud neni, tak se chovame jako pri cancelu.
 */

void js_upcall_goto_history(void * data)
{
	struct fax_me_tender_int_string *s=(struct fax_me_tender_int_string*)data;
	struct gimme_js_id_string* jsid;
	struct f_data_c *fd;
	struct terminal *term;
	unsigned char *url=NULL;
	unsigned char txt[16];

	/* context must be a valid pointer ! */
	fd=(struct f_data_c*)(s->ident);

	if (!fd->js) return;

	/* kill timer, that called me */
	js_spec_vykill_timer(fd->js->ctx,0);

	if (!s)internal("Hele, tyhle prasarny si zkousej na nekoho jinyho, jo?!\n");

	if (!(s->num)&&!(s->string))internal("Tak tohle na mne nezkousej, bidaku!\n");
	if ((s->num)&&(s->string))internal("Ta sedla!\n");
	jsid=mem_alloc(sizeof(struct gimme_js_id_string));
	if (!jsid)internal("Can't allocate memory in js_upcall_goto_history\n");
	
	/* find the history item */
	if (s->num)	/* goto n-th item */
	{
		struct location *loc;
		int a = s->num;

                jsid->n = a;

		if (a < 0) a = -a;
		if (! list_empty(fd->ses->history)) {
			for (loc = cur_loc(fd->ses);
			     loc != (void *) &fd->ses->history;
			     loc = (s->num < 0 ? loc->next : loc->prev)) {
				if (a == 0) {
					url = stracpy(loc->url);
					break;
				}
				a--;
			}
		}
	}
	else	/* goto given url */
	{
		struct location *loc;
		int a=0;

		foreach(loc,fd->ses->history)
		{
			if (!strcmp(s->string,loc->url)){url=stracpy(s->string);jsid->n=a;break;}
			a++;
		}
	}

	if (s->string)js_mem_free(s->string);
	if (!url){js_mem_free(data);mem_free(jsid);goto goto_history_failed;}

	term=fd->ses->term;

	/* fill in jsid */
	jsid->id=((fd->id)<<JS_OBJ_MASK_SIZE)|JS_OBJ_T_DOCUMENT;
	jsid->js_id=fd->js->ctx->js_id;
	jsid->string=url;
	
	snprintf(txt,16," (%d) ",jsid->n);
	msg_box(
		term,   /* terminal */
		getml(url,jsid,NULL),   /* memory blocks to free */
		TXT(T_GOTO_HISTORY),   /* title */
		AL_CENTER|AL_EXTD_TEXT,   /* alignment */
		TXT(T_JS_IS_ATTEMPTING_TO_GO_INTO_HISTORY), txt, TXT(T_TO_URL), " \"",url,"\".",NULL,   /* message */
		jsid,   /* data for button functions */
		3,   /* # of buttons */
		TXT(T_ALLOW),__js_upcall_goto_history_ok_pressed,B_ENTER,
		TXT(T_REJECT),__js_upcall_goto_url_cancel_pressed,B_ESC,
		TXT(T_KILL_SCRIPT), __js_kill_script_pressed,NULL  /* dirty trick: gimme_js_id_string and gimme_js_id begins with the same long */
	);

	js_mem_free(s);
	return;
goto_history_failed:
	js_downcall_vezmi_null(fd->js->ctx);
	return;
}


/* set default status-line text
 * tak_se_ukaz_Kolbene is allocated string or NULL
 */
void js_upcall_set_default_status(void *context, unsigned char *tak_se_ukaz_Kolbene)
{
	struct f_data_c *fd=(struct f_data_c*)context;
	unsigned char *trouba;

	if (!fd)internal("Tak tohle teda ne, bobanku!\n");

	if (!(*tak_se_ukaz_Kolbene)){mem_free(tak_se_ukaz_Kolbene);tak_se_ukaz_Kolbene=NULL;} /* Ale to hlavni jsme se nedozvedeli - s tim chrapanim jste mi neporadil... */

	if (fd->ses->default_status)mem_free(fd->ses->default_status);
	skip_nonprintable(tak_se_ukaz_Kolbene);
	if (fd->f_data&&tak_se_ukaz_Kolbene)
	{
		struct conv_table* ct; /* ... a ted ty pochybne reci o majetku ... */
		
		ct=get_translation_table(fd->f_data->cp,fd->f_data->opt.cp);
		trouba=convert_string(ct,tak_se_ukaz_Kolbene,strlen(tak_se_ukaz_Kolbene),NULL); /* Taky to mate levnejsi - jinak by to stalo deset! */
		mem_free(tak_se_ukaz_Kolbene);
		/* a je to v troube... */
	}
	else
	{
		trouba=tak_se_ukaz_Kolbene;
	}

	fd->ses->default_status=trouba;
	change_screen_status(fd->ses);
	print_screen_status(fd->ses);
}


/* returns allocated string with default status-line value or NULL when default value is empty
 */
unsigned char* js_upcall_get_default_status(void *context)
{
	struct f_data_c *fd=(struct f_data_c *)context;
	unsigned char *tak_se_ukaz_Danku=NULL;
	unsigned char *trouba;

	if (!fd)internal("Ale hovno!\n");
	
	if (fd->ses->default_status&&(*fd->ses->default_status))tak_se_ukaz_Danku=stracpy(fd->ses->default_status);
	skip_nonprintable(tak_se_ukaz_Danku);
	if (fd->f_data&&tak_se_ukaz_Danku)
	{
		struct conv_table* ct;
		
		ct=get_translation_table(fd->f_data->opt.cp,fd->f_data->cp);
		trouba=convert_string(ct,tak_se_ukaz_Danku,strlen(tak_se_ukaz_Danku),NULL);
		mem_free(tak_se_ukaz_Danku);
	}
	else
	{
		trouba=tak_se_ukaz_Danku;
	}

	/* Tak to mame Kolben a Danek po peti korunach... */
	
	return trouba; /* No jo, je to v troube! */
}


/* set status-line text
 * tak_se_ukaz_Kolbene is allocated string or NULL
 */
void js_upcall_set_status(void *context, unsigned char *tak_se_ukaz_Kolbene)
{
	struct f_data_c *fd=(struct f_data_c*)context;
	unsigned char *trouba;

	if (!fd)internal("To leda tak -PRd!\n");

	if (!(*tak_se_ukaz_Kolbene)){mem_free(tak_se_ukaz_Kolbene);tak_se_ukaz_Kolbene=NULL;}

	if (fd->ses->st)mem_free(fd->ses->st);
	skip_nonprintable(tak_se_ukaz_Kolbene);
	if (fd->f_data&&tak_se_ukaz_Kolbene)
	{
		struct conv_table* ct;
		
		ct=get_translation_table(fd->f_data->cp,fd->f_data->opt.cp);
		trouba=convert_string(ct,tak_se_ukaz_Kolbene,strlen(tak_se_ukaz_Kolbene),NULL);
		mem_free(tak_se_ukaz_Kolbene);
		/* a je to v troube... */
	}
	else
	{
		trouba=tak_se_ukaz_Kolbene;
	}

	fd->ses->st=trouba;
	print_screen_status(fd->ses);
}


/* returns allocated string with default status-line value or NULL when default value is empty
 */
unsigned char* js_upcall_get_status(void *context)
{
	struct f_data_c *fd=(struct f_data_c *)context;
	unsigned char *tak_se_ukaz_Danku=NULL;
	unsigned char *trouba;

	if (!fd)internal("To leda tak hovno!\n");
	
	if (fd->ses->st&&(*fd->ses->st))tak_se_ukaz_Danku=stracpy(fd->ses->st);
	skip_nonprintable(tak_se_ukaz_Danku);
	if (fd->f_data&&tak_se_ukaz_Danku)
	{
		struct conv_table* ct;
		
		ct=get_translation_table(fd->f_data->opt.cp,fd->f_data->cp);
		trouba=convert_string(ct,tak_se_ukaz_Danku,strlen(tak_se_ukaz_Danku),NULL);
		mem_free(tak_se_ukaz_Danku);
	}
	else
	{
		trouba=tak_se_ukaz_Danku;
	}

	/* Kolben a Danek, to mame po peti korunach... */
	
	return trouba;
}

/* returns allocated string with cookies, or NULL on error */
unsigned char * js_upcall_get_cookies(void *context)
{
	struct f_data_c *fd=(struct f_data_c *)context;
	unsigned char *s=init_str();
	int l=0;
	int nc=0;
	struct cookie *c, *d;
	unsigned char *server, *data;
	struct c_domain *cd;

	if (!fd)internal("Tak tomu rikam selhani komunikace...\n");

	/* zavolame set_cookies, ten zparsuje fd->js->ctx->cookies a necha tam nezparsovatelnej zbytek */

	if (!fd->js||!fd->js->ctx) {mem_free(s);return NULL;}
	if (!fd->rq) goto ty_uz_se_nevratis;
	
	jsint_set_cookies(fd,0);
	
	server = get_host_name(fd->rq->url);
	data = get_url_data(fd->rq->url);

	if (data > fd->rq->url) data--;
	foreach (cd, c_domains) if (is_in_domain(cd->domain, server)) goto ok;
	mem_free(server);
ty_uz_se_nevratis:
	if (fd->js->ctx->cookies)add_to_str(&s,&l,fd->js->ctx->cookies);
	else {mem_free(s);s=NULL;}
	return s;
	ok:
	foreach (c, cookies) if (is_in_domain(c->domain, server)) if (is_path_prefix(c->path, data)) {
		if (cookie_expired(c)) {
			d = c;
			c = c->prev;
			del_from_list(d);
			free_cookie(d);
			mem_free(d);
			continue;
		}
		if (c->secure) continue;
		if (!nc) nc = 1;
		else add_to_str(&s, &l, "; ");
		add_to_str(&s, &l, c->name);
		add_to_str(&s, &l, "=");
		add_to_str(&s, &l, c->value);
	}
	
	if (!nc) {mem_free(s);s=NULL;}
	mem_free(server);

	/* za strinzik sestaveny z vnitrni reprezentace susenek jeste prilepime nezparsovatelnej zbytek */
	if (fd->js->ctx->cookies)
	{
		if (!s)s=stracpy(fd->js->ctx->cookies);
		else {add_to_str(&s,&l,"; ");add_to_str(&s,&l,fd->js->ctx->cookies);}
	}
	return s;
}

/* FIXME: document.all nechodi, musi se prepsat, aby vracel dvojice frame:idcko */


/* adds all in given f_data_c, the f_data_c must be accessible by the javascript */
void __add_all_recursive_in_fd(long **field, int *len, struct f_data_c *fd, struct f_data_c *js_ctx)
{
	struct f_data_c *ff;
	struct form_control *fc;
	
#ifdef G
	struct xlist_head *fi;
#endif

       	/* add all accessible frames */
       	foreach(ff,fd->subframes)
       		if (jsint_can_access(js_ctx,ff))
       			if (!((*field)=__add_fd_id(*field,len,js_upcall_get_frame_id(fd),js_upcall_get_frame_id(ff),ff->f_data?ff->f_data->opt.framename:NULL)))return;

	if (!(fd->f_data))goto tady_uz_nic_peknyho_nebude;

#ifdef G
	/* add all images */
	if (F)
		foreach(fi,fd->f_data->images)
		{
			struct g_object_image *gi;
			struct g_object_image goi;
	
			gi = (struct g_object_image *)((char *)fi + ((char *)(&goi) - (char *)(&(goi.image_list))));
			if (!((*field)=__add_fd_id(*field,len,js_upcall_get_frame_id(fd),JS_OBJ_T_IMAGE+((gi->id)<<JS_OBJ_MASK_SIZE),gi->name)))return;
		}
#endif
	/* add all forms */
	foreachback(fc,fd->f_data->forms)
		if (!((*field)=__add_fd_id(*field,len,js_upcall_get_frame_id(fd),((fc->form_num)<<JS_OBJ_MASK_SIZE)+JS_OBJ_T_FORM,fc->form_name)))return;

	/* add all form elements */
	foreachback(fc,fd->f_data->forms)
		{
			long tak_mu_to_ukaz=0;
			tak_mu_to_ukaz=(fc->g_ctrl_num)<<JS_OBJ_MASK_SIZE;
			switch (fc->type)
			{
				case FC_TEXT:		tak_mu_to_ukaz|=JS_OBJ_T_TEXT; break;
				case FC_PASSWORD:	tak_mu_to_ukaz|=JS_OBJ_T_PASSWORD; break;
				case FC_TEXTAREA:	tak_mu_to_ukaz|=JS_OBJ_T_TEXTAREA; break;
				case FC_CHECKBOX:	tak_mu_to_ukaz|=JS_OBJ_T_CHECKBOX; break;
				case FC_RADIO:		tak_mu_to_ukaz|=JS_OBJ_T_RADIO; break;
				case FC_IMAGE:
				case FC_SELECT:		tak_mu_to_ukaz|=JS_OBJ_T_SELECT; break;
				case FC_SUBMIT:		tak_mu_to_ukaz|=JS_OBJ_T_SUBMIT ; break;
				case FC_RESET:		tak_mu_to_ukaz|=JS_OBJ_T_RESET ; break;
				case FC_HIDDEN:		tak_mu_to_ukaz|=JS_OBJ_T_HIDDEN ; break;
				case FC_BUTTON:		tak_mu_to_ukaz|=JS_OBJ_T_BUTTON ; break;
				default:/* internal("Invalid form element type.\n"); */
				tak_mu_to_ukaz=0;break;
			}
			if (tak_mu_to_ukaz&&!((*field)=__add_fd_id(*field,len,js_upcall_get_frame_id(fd),tak_mu_to_ukaz,fc->name)))return;
		}
	
tady_uz_nic_peknyho_nebude:

	foreach(ff,fd->subframes)
		if (jsint_can_access(js_ctx,ff)) __add_all_recursive_in_fd(field,len,ff,js_ctx);
}

/* returns allocated field of all objects in the document (document.all)
 * size of the field will be stored in len
 * the field has 3x more items than the number of objects
 * field[x+0]==id of frame
 * field[x+1]==id of the object
 * field[x+2]==allocated unsigned char* with name of the object or NULL (when there's no name)
 *
 * when an error occurs, returns NULL
 */
long * js_upcall_get_all(void *chuligane, long document_id, int *len)
{
	struct f_data_c *js_ctx=(struct f_data_c*)chuligane;
	long *pole_neorane; /* Premysle Zavoraci, kde se flakas? Zase forbesis, co? */
	struct f_data_c *fd;

	if (!js_ctx)internal("js_upcall_get_all called with NULL context pointer\n");
	fd=jsint_find_document(document_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return NULL;

	*len=0;

	pole_neorane=mem_alloc(sizeof(long));
	if (!pole_neorane)return NULL;

	__add_all_recursive_in_fd(&pole_neorane,len,fd,js_ctx);

	/* nothing was found */
	if (!pole_neorane)return NULL;
	if (!(*len))mem_free(pole_neorane),pole_neorane=NULL;
	
	return pole_neorane;
}


/* returns allocated field of all images
 * size of the field will be stored in len
 * when an error occurs, returns NULL
 */

long *js_upcall_get_images(void *chuligane, long document_id, int *len)
{
#ifdef G
	struct f_data_c *fd;
	struct f_data_c *js_ctx=(struct f_data_c*)chuligane;
	long *pole_Premysla_Zavorace;
	struct xlist_head *fi;
	int a;

	if (F)
	{
		if (!js_ctx)internal("js_upcall_get_images called with NULL context pointer\n");
		fd=jsint_find_document(document_id);
		if (!fd||!fd->f_data||!jsint_can_access(js_ctx,fd))return NULL;
	
		*len=0;
		
		foreach(fi,fd->f_data->images)(*len)++;
	
		if (!(*len))return NULL;
		
		pole_Premysla_Zavorace=mem_alloc((*len)*sizeof(long));
		if (!pole_Premysla_Zavorace)internal("Cannot allocate memory in js_upcall_get_images.");
		
		a=0;
		foreachback(fi,fd->f_data->images)
		{
			unsigned id;
			struct g_object_image gi;
	
			id=((struct g_object_image *)((char *)fi + ((char *)&gi - (char *)&(gi.image_list))))->id;
	
			pole_Premysla_Zavorace[a]=JS_OBJ_T_IMAGE+(id<<JS_OBJ_MASK_SIZE);
			a++;
		}
		return pole_Premysla_Zavorace;
	}else
#endif
	{
		document_id=document_id;
		*len=0;
		return NULL;
	}
}

/* returns width of given image or -1 on error */
int js_upcall_get_image_width(void *chuligane, long document_id, long image_id)
{
#ifdef G
	struct f_data_c *js_ctx=(struct f_data_c*)chuligane;
	struct f_data_c *fd;
	struct g_object_image *gi;

	if (F)
	{
		if (!js_ctx)internal("js_upcall_get_image_width called with NULL context pointer\n");
		if ((image_id&JS_OBJ_MASK)!=JS_OBJ_T_IMAGE)return -1;
		fd=jsint_find_document(document_id);
		if (!fd||!jsint_can_access(js_ctx,fd))return -1;
	
		gi=jsint_find_object(fd,image_id);
	
		if (!gi)return -1;
		
		return gi->cimg->width;
	}else
#endif
	{
		document_id=document_id;
		image_id=image_id;
		return -1;
	}
}


/* returns height of given image or -1 on error */
int js_upcall_get_image_height(void *chuligane, long document_id, long image_id)
{
#ifdef G
	struct f_data_c *js_ctx=(struct f_data_c*)chuligane;
	struct f_data_c *fd;
	struct g_object_image *gi;

	if (F)
	{
		if (!js_ctx)internal("js_upcall_get_image_height called with NULL context pointer\n");
		if ((image_id&JS_OBJ_MASK)!=JS_OBJ_T_IMAGE)return -1;
		fd=jsint_find_document(document_id);
		if (!fd||!jsint_can_access(js_ctx,fd))return -1;
	
		gi=jsint_find_object(fd,image_id);
	
		if (!gi)return -1;
		
		return gi->cimg->height;
	}else
#endif
	{
		document_id=document_id;
		image_id=image_id;
		return -1;
	}
}


/* returns border of given image or -1 on error */
int js_upcall_get_image_border(void *chuligane, long document_id, long image_id)
{
#ifdef G
	struct f_data_c *fd;
	struct f_data_c *js_ctx=(struct f_data_c*)chuligane;
	struct g_object_image *gi;

	if (F)
	{
		if (!js_ctx)internal("js_upcall_get_image_border called with NULL context pointer\n");
		if ((image_id&JS_OBJ_MASK)!=JS_OBJ_T_IMAGE)return -1;
		fd=jsint_find_document(document_id);
		if (!fd||!jsint_can_access(js_ctx,fd))return -1;
	
		gi=jsint_find_object(fd,image_id);
	
		if (!gi)return -1;
		
		return gi->border;
	}else
#endif
	{
		document_id=document_id;
		image_id=image_id;
		return -1;
	}
}


/* returns vspace of given image or -1 on error */
int js_upcall_get_image_vspace(void *chuligane, long document_id, long image_id)
{
#ifdef G
	struct f_data_c *fd;
	struct f_data_c *js_ctx=(struct f_data_c*)chuligane;
	struct g_object_image *gi;

	if (F)
	{
		if (!js_ctx)internal("js_upcall_get_image_vspace called with NULL context pointer\n");
		if ((image_id&JS_OBJ_MASK)!=JS_OBJ_T_IMAGE)return -1;
		fd=jsint_find_document(document_id);
		if (!fd||!jsint_can_access(js_ctx,fd))return -1;
	
		gi=jsint_find_object(fd,image_id);
	
		if (!gi)return -1;
		
		return gi->vspace;
	}else
#endif
	{
		document_id=document_id;
		image_id=image_id;
		return -1;
	}
}


/* returns hspace of given image or -1 on error */
int js_upcall_get_image_hspace(void *chuligane, long document_id, long image_id)
{
#ifdef G
	struct f_data_c *fd;
	struct f_data_c *js_ctx=(struct f_data_c*)chuligane;
	struct g_object_image *gi;

	if (F)
	{
		if (!js_ctx)internal("js_upcall_get_image_hspace called with NULL context pointer\n");
		if ((image_id&JS_OBJ_MASK)!=JS_OBJ_T_IMAGE)return -1;
		fd=jsint_find_document(document_id);
		if (!fd||!jsint_can_access(js_ctx,fd))return -1;
	
		gi=jsint_find_object(fd,image_id);
	
		if (!gi)return -1;
		
		return gi->hspace;
	}else
#endif
	{
		document_id=document_id;
		image_id=image_id;
		return -1;
	}
}


/* returns allocated string with name of given image or NULL on error */
unsigned char * js_upcall_get_image_name(void *chuligane, long document_id, long image_id)
{
#ifdef G
	struct f_data_c *fd;
	struct f_data_c *js_ctx=(struct f_data_c*)chuligane;
	struct g_object_image *gi;

	if (F)
	{
		if (!js_ctx)internal("js_upcall_get_image_name called with NULL context pointer\n");
		if ((image_id&JS_OBJ_MASK)!=JS_OBJ_T_IMAGE)return NULL;
		fd=jsint_find_document(document_id);
		if (!fd||!jsint_can_access(js_ctx,fd))return NULL;
		
		gi=jsint_find_object(fd,image_id);
	
		if (!gi)return NULL;
		
		return stracpy(gi->name);
	}else
#endif
	{
		document_id=document_id;
		image_id=image_id;
		return NULL;
	}
}


/* returns allocated string with name of given image or NULL on error */
unsigned char * js_upcall_get_image_alt(void *chuligane, long document_id, long image_id)
{
#ifdef G
	struct f_data_c *fd;
	struct f_data_c *js_ctx=(struct f_data_c*)chuligane;
	struct g_object_image *gi;

	if (F)
	{
		if (!js_ctx)internal("js_upcall_get_image_alt called with NULL context pointer\n");
		if ((image_id&JS_OBJ_MASK)!=JS_OBJ_T_IMAGE)return NULL;
		fd=jsint_find_document(document_id);
		if (!fd||!jsint_can_access(js_ctx,fd))return NULL;
	
		gi=jsint_find_object(fd,image_id);
	
		if (!gi)return NULL;
		
		return stracpy(gi->alt);
	}else
#endif
	{
		chuligane=chuligane;
		document_id=document_id;
		image_id=image_id;
		return NULL;
	}
}


/* sets image name to given value */
/* name is deallocated after use with mem_free */
void js_upcall_set_image_name(void *chuligane, long document_id, long image_id, unsigned char *name)
{
#ifdef G
	struct f_data_c *fd;
	struct f_data_c *js_ctx=(struct f_data_c*)chuligane;
	struct g_object_image *gi;

	if (F)
	{
		if (!js_ctx)internal("js_upcall_set_image_name called with NULL context pointer\n");
		if ((image_id&JS_OBJ_MASK)!=JS_OBJ_T_IMAGE)return;
		fd=jsint_find_document(document_id);
		if (!fd||!jsint_can_access(js_ctx,fd))return;
	
		gi=jsint_find_object(fd,image_id);
	
		if (!gi)return;
		
		if (gi->name)mem_free(gi->name);
		gi->name=stracpy(name);	/* radeji takhle, protoze to je bezpecnejsi: az PerM zase do neceho slapne, tak se to pozna hned tady a ne buhvikde */
		if (name)mem_free(name);
		return;
	}else
#endif
	{
		chuligane=chuligane;
		document_id=document_id;
		image_id=image_id;
		if (name)mem_free(name);
		return;
	}
}


/* sets image alt to given value */
/* alt is deallocated after use with mem_free */
void js_upcall_set_image_alt(void *chuligane, long document_id, long image_id, unsigned char *alt)
{
#ifdef G
	struct f_data_c *fd;
	struct f_data_c *js_ctx=(struct f_data_c*)chuligane;
	struct g_object_image *gi;

	if (F)
	{
		if (!js_ctx)internal("js_upcall_set_image_alt called with NULL context pointer\n");
		if ((image_id&JS_OBJ_MASK)!=JS_OBJ_T_IMAGE)return;
		fd=jsint_find_document(document_id);
		if (!fd||!fd->f_data||!jsint_can_access(js_ctx,fd))return;
	
		gi=jsint_find_object(fd,image_id);
	
		if (!gi)return;
		
		if (gi->alt)mem_free(gi->alt);
		gi->alt=stracpy(alt);	/* radeji takhle, protoze to je bezpecnejsi: az PerM zase do neceho slapne, tak se to pozna hned tady a ne buhvikde */
		if (fd->f_data&&gi->link_num>=0&&gi->link_num<fd->f_data->nlinks)
		{
			struct link *l=&fd->f_data->links[gi->link_num];
	
			if (l->img_alt)mem_free(l->img_alt);
			l->img_alt=stracpy(alt);
		}
		if (alt)mem_free(alt);
		change_screen_status(fd->ses);
		print_screen_status(fd->ses);
		return;
	}else
#endif
	{
		chuligane=chuligane;
		document_id=document_id;
		image_id=image_id;
		if (alt)mem_free(alt);
		return;
	}
}


/* returns allocated string with source URL of given image or NULL on error */
unsigned char * js_upcall_get_image_src(void *chuligane, long document_id, long image_id)
{
#ifdef G
	struct f_data_c *fd;
	struct f_data_c *js_ctx=(struct f_data_c*)chuligane;
	struct g_object_image *gi;
	
	if (F)
	{
		if (!js_ctx)internal("js_upcall_get_image_src called with NULL context pointer\n");
		if ((image_id&JS_OBJ_MASK)!=JS_OBJ_T_IMAGE)return NULL;
		fd=jsint_find_document(document_id);
		if (!fd||!jsint_can_access(js_ctx,fd))return NULL;
	
		gi=jsint_find_object(fd,image_id);
	
		if (!gi)return NULL;
		
		return stracpy(gi->orig_src);
	}else
#endif
	{
		document_id=document_id;
		image_id=image_id;
		return NULL;
	}
}


/* changes image URL
 * gets struct fax_me_tender_string_2_longy
 * 	num1 = document_id, num2 = image_id, string = url
 *
 * frees the string and the fax_me_tender struct with js_mem_free function
 */
void js_upcall_set_image_src(void *chuligane)
{
	unsigned char *zvrat;
	struct fax_me_tender_string_2_longy *fax=(struct fax_me_tender_string_2_longy*)chuligane;
	struct f_data_c *js_ctx;
#ifdef G
	struct f_data_c *fd;
	struct g_object_image *gi;
	long image_id,document_id;
	unsigned char *vecirek;
	if (F)
	{
		js_ctx=(struct f_data_c*)fax->ident;
		js_spec_vykill_timer(js_ctx->js->ctx,0);
		if (!chuligane)internal("js_upcall_set_image_src called with NULL argument\n");
		if (!js_ctx)internal("js_upcall_set_image_src called with NULL context pointer\n");
		image_id=fax->obj_id;
		document_id=fax->doc_id;
		if ((image_id&JS_OBJ_MASK)!=JS_OBJ_T_IMAGE){js_mem_free(fax->string);fax->string=NULL;goto abych_tu_nepovecerel;}
		fd=jsint_find_document(document_id);
		if (!fd||!jsint_can_access(js_ctx,fd)){js_mem_free(fax->string);fax->string=NULL;goto abych_tu_nepovecerel;}
	
		gi=jsint_find_object(fd,image_id);
	
		if (!gi||!fd->f_data){js_mem_free(fax->string);fax->string=NULL;goto abych_tu_nepovecerel;}
	
		/* string joinnem s url */
		if (fd->loc&&fd->loc->url) vecirek=join_urls(fd->loc->url,fax->string);
		else vecirek=stracpy(fax->string);
		/* a mame to kompatidebilni s verzi pred jointem */
	
		change_image(gi,vecirek,fax->string,fd->f_data);
		if (vecirek) mem_free(vecirek);
		fd->f_data->uncacheable = 1;
		abych_tu_nepovecerel:;
	}else
#endif
	{
		js_ctx=(struct f_data_c*)fax->ident;
		if (!js_ctx)internal("js_upcall_set_image_src called with NULL context pointer\n");
		js_spec_vykill_timer(js_ctx->js->ctx,0);
		if (!chuligane)internal("js_upcall_set_image_src called with NULL argument\n");
	}
	zvrat=stracpy(fax->string);
	js_mem_free(fax->string);
	js_mem_free(fax);
	js_downcall_vezmi_string(js_ctx->js->ctx,zvrat);
}


/* returns 1 if image has completed loading, 0 when not, -1 on error */
int js_upcall_image_complete(void *chuligane, long document_id, long image_id)
{
#ifdef G
	struct f_data_c *fd;
	struct f_data_c *js_ctx=(struct f_data_c*)chuligane;
	struct g_object_image *gi;

	if (F)
	{
		if (!js_ctx)internal("js_upcall_image_complete called with NULL context pointer\n");
		if ((image_id&JS_OBJ_MASK)!=JS_OBJ_T_IMAGE)return -1;
		fd=jsint_find_document(document_id);
		if (!fd||!jsint_can_access(js_ctx,fd))return -1;

		gi=jsint_find_object(fd,image_id);
	
		if (!gi)return -1;
		
		if (!gi->af||!gi->af->rq||!gi->af->rq->state)return -1;
		return gi->af->rq->state==O_OK;
	}else
#endif
	{
		document_id=document_id;
		image_id=image_id;
		return -1;
	}
}


/* returns parent of given frame (or document), or -1 on error or no permissions */
/* if frame_id is already top frame returns the same frame */
long js_upcall_get_parent(void *chuligane, long frame_id)
{
	struct f_data_c *fd, *ff;
	struct f_data_c *js_ctx=(struct f_data_c*)chuligane;

	if (!js_ctx)internal("js_upcall_get_parent called with NULL context pointer\n");
	if ((frame_id&JS_OBJ_MASK)!=JS_OBJ_T_FRAME&&(frame_id&JS_OBJ_MASK)!=JS_OBJ_T_DOCUMENT)return -1;
	fd=jsint_find_document(frame_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return -1;

	for (ff=fd->parent;ff&&!ff->rq;ff=ff->parent);
	
	if (!ff)ff=fd->ses->screen;
	return jsint_can_access(fd,ff)?js_upcall_get_frame_id(ff):-1;
}


/* returns top of given frame (or document), or -1 on error */
/* returns highest grandparent accessible from given frame */
long js_upcall_get_frame_top(void *chuligane, long frame_id)
{
	struct f_data_c *fd, *ff;
	struct f_data_c *js_ctx=(struct f_data_c*)chuligane;

	if (!js_ctx)internal("js_upcall_get_frame_top called with NULL context pointer\n");
	if ((frame_id&JS_OBJ_MASK)!=JS_OBJ_T_FRAME&&(frame_id&JS_OBJ_MASK)!=JS_OBJ_T_DOCUMENT)return -1;
	fd=jsint_find_document(frame_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return -1;
	for (ff=fd->parent;ff;ff=ff->parent)
	{
		if (ff->rq)
		{
			if (!jsint_can_access(fd,ff))break;
			fd=ff;
		}
	}
	return js_upcall_get_frame_id(fd);
}


/* returns allocated field of subframes or NULL on error */
/* count cointains length of the field */
/* don't forget to free the field after use */
long * js_upcall_get_subframes(void *chuligane, long frame_id, int *count)
{
	struct f_data_c *js_ctx=(struct f_data_c*)chuligane;
	struct f_data_c *fd;
	struct f_data_c *f;
	int a;
	long *pole;
	*count=0;

	if (!js_ctx)internal("js_upcall_get_subframes called with NULL context pointer\n");
	if ((frame_id&JS_OBJ_MASK)!=JS_OBJ_T_FRAME&&(frame_id&JS_OBJ_MASK)!=JS_OBJ_T_DOCUMENT)return NULL;
	fd=jsint_find_document(frame_id);
	if (!fd||!jsint_can_access(js_ctx,fd))return NULL;

	foreach(f,fd->subframes)
		if (jsint_can_access(fd,f))(*count)++;

	if (!*count)return NULL;
	pole=mem_alloc((*count)*sizeof(long));
	if (!pole)return NULL;
	
	a=0;
	foreach(f,fd->subframes)
		if (jsint_can_access(fd,f))
			{pole[a]=js_upcall_get_frame_id(f);a++;}
	return pole;
}


/*---------------------             DOWNCALLS          ---------------------------*/

void js_downcall_game_over(void *context)
{
	struct f_data_c *fd=(struct f_data_c*)(((js_context*)(context))->ptr);
	
	js_error(_(TXT(T_SCRIPT_KILLED_BY_USER),fd->ses->term),context);
	if (fd->ses->default_status)mem_free(fd->ses->default_status),fd->ses->default_status=NULL; /* pekne uklidime bordylek, ktery nam BFU nacintalo do status lajny */
	js_durchfall=0;
	if(((js_context*)context)->running)
		js_volej_kolbena(context);
	/* Kolben - ale nespi mi - co s tim budeme delat? */
	((js_context*)context)->running=0;
}



void js_downcall_vezmi_int(void *context, int i)
{
}


void js_downcall_vezmi_float(void *context, float f)
{
}

#else

void jsint_execute_code(struct f_data_c *fd, unsigned char *code, int len, int write_pos, int onclick_submit, int onsubmit)
{
}

void jsint_destroy(struct f_data_c *fd)
{
}

void jsint_scan_script_tags(struct f_data_c *fd)
{
}

void jsint_destroy_document_description(struct f_data *f)
{
}

int jsint_get_source(struct f_data_c *fd, unsigned char **start, unsigned char **end)
{
	return 0;
}

#endif
