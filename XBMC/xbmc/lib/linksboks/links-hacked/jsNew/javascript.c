/*
 *	(C)2003 by Technotrend - martin.zwickel@technotrend.de
 */
#include <jsdbgapi.h>
#include "js.h"

#ifdef JS

static JSRuntime *js_rt=NULL;

int js_memory_limit=10; // 10k enough for much
int js_fun_depth=10;

static int js_use_count=0;

struct s_schedule {
	struct f_data_c *fd;
	unsigned char *script;
	int scriptlen;
	int timeri;
};

struct s_timers{
	signed int timerid;
	struct s_schedule *timersched;
};

struct js_state {
	JSContext *context;
        JSObject  *globalObj;
	

//	struct javascript_context *ctx;	/* kontext beziciho javascriptu??? */
//	struct list_head queue;		/* struct js_request - list of javascripts to run */
//	struct js_request *active;	/* request is running */
	unsigned char *src;		/* zdrojak beziciho javascriptu??? */	/* mikulas: ne. to je zdrojak stranky */
	int running;
	int srclen;
	int wrote;
	int write_pos;
	struct s_timers timers[MAXTIMERS];
};

static void js_error(const char*a,void *v)
{
	unsigned char *txt;
	struct f_data_c *fd=(struct f_data_c*)v;
	struct terminal *term;
	char *b=mem_alloc(strlen(a)+1);
	if(!fd)
		return;
	term=fd->ses->term;
	memcpy(b,a,strlen(a));
	b[strlen(a)]=0;
	msg_box(
			term,   /* terminal */
			getml(txt,NULL),
			TEXT(T_JAVASCRIPT_ERROR),   /* title */
			AL_CENTER,   /* alignment */
			b,   /* message */
			NULL,   /* data for button functions */
			1,   /* # of buttons */
			TEXT(T_DISMISS),NULL,B_ENTER|B_ESC  /* first button */
		   );
}

static void redraw_document(struct f_data_c *f)
{
	draw_fd(f);
}


#define DEF_PROP_FUNCTION(a) static JSBool a(JSContext *context,JSObject *obj,jsval id,jsval *vp)
#define PROP_DUMP	fprintf(stderr,"PROP: %s=%s\n",JS_GetStringBytes(JS_ValueToString(context, id)),JS_GetStringBytes(JS_ValueToString(context, *vp)))

#define GET_FD(context) ((struct f_data_c*)JS_GetPrivate(context,JS_GetGlobalObject(context)))

DEF_PROP_FUNCTION(Dlocation_get)
{
	TRACE;
	PROP_DUMP;
}
DEF_PROP_FUNCTION(Dimages_get)
{
	TRACE;
	PROP_DUMP;
}
DEF_PROP_FUNCTION(Dall_get)
{
	TRACE;
	PROP_DUMP;
}
DEF_PROP_FUNCTION(Danchors_get)
{
	TRACE;
	PROP_DUMP;
}
DEF_PROP_FUNCTION(Dcookie_get)
{
	TRACE;
	PROP_DUMP;
}
DEF_PROP_FUNCTION(Dforms_get)
{
	TRACE;
	PROP_DUMP;
}
DEF_PROP_FUNCTION(DlastModified_get)
{
	TRACE;
	PROP_DUMP;
	struct f_data_c *fd=GET_FD(context);
	if (!fd->rq||!fd->rq->ce)
		return JS_FALSE;
	fprintf(stderr,"last-modified:%s\n",fd->rq->ce->last_modified);
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(context,fd->rq->ce->last_modified)); // XXX memleak?
	return JS_TRUE;
}
DEF_PROP_FUNCTION(Dlinks_get)
{
	TRACE;
	PROP_DUMP;
}
DEF_PROP_FUNCTION(Dreferer_get)
{
	TRACE;
	PROP_DUMP;
}
DEF_PROP_FUNCTION(Dtitle_get)
{
	TRACE;
	PROP_DUMP;
	unsigned char *title,*t;
	struct conv_table* ct;
	struct f_data_c *fd=GET_FD(context);

	title=mem_alloc(MAX_STR_LEN*sizeof(unsigned char));
	if (!(get_current_title(fd->ses,title,MAX_STR_LEN)))
	{
		mem_free(title);
		return JS_FALSE;
	}
	if (fd->f_data)
	{
		ct=get_translation_table(fd->f_data->opt.cp,fd->f_data->cp);
		t = convert_string(ct, title, strlen(title), NULL);
		mem_free(title);
		title=t;
	}
	fprintf(stderr,"current title: %s\n",title);
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(context,title)); // XXX memleak?
	mem_free(title);
	return JS_TRUE;
}
DEF_PROP_FUNCTION(Dtitle_set)
{
	TRACE;
	PROP_DUMP;
	unsigned char *title,*t;
	struct conv_table* ct;
	struct f_data_c *fd=GET_FD(context);
	int l=0;

	title=JS_GetStringBytes(JS_ValueToString(context, *vp));
	
	if (!title)
		return JS_FALSE;

	if (!(fd->f_data))
		return JS_FALSE;

	if (fd->f_data->title)
		mem_free(fd->f_data->title);
	fd->f_data->title=init_str();
	fd->f_data->uncacheable=1;
	ct=get_translation_table(fd->f_data->cp,fd->f_data->opt.cp);
	t=convert_string(ct,title,strlen(title),NULL);
	add_to_str(&(fd->f_data->title),&l,t);
	mem_free(t);

	redraw_document(fd);
	return JS_TRUE;
}

static JSPropertySpec doc_property[]=
{
	{"location",		1, JSPROP_PERMANENT|JSPROP_READONLY	, Dlocation_get		, 0},
	{"images",		2, JSPROP_PERMANENT|JSPROP_READONLY	, Dimages_get		, 0},
	{"all",			3, JSPROP_PERMANENT|JSPROP_READONLY	, Dall_get		, 0},
	{"anchors",		4, JSPROP_PERMANENT|JSPROP_READONLY	, Danchors_get		, 0},
	{"cookie",		5, JSPROP_PERMANENT|JSPROP_READONLY	, Dcookie_get		, 0},
	{"forms",		6, JSPROP_PERMANENT|JSPROP_READONLY	, Dforms_get		, 0},
	{"lastModified",	7, JSPROP_PERMANENT|JSPROP_READONLY	, DlastModified_get	, 0},
	{"links",		8, JSPROP_PERMANENT|JSPROP_READONLY	, Dlinks_get		, 0},
	{"referer",		9, JSPROP_PERMANENT|JSPROP_READONLY	, Dreferer_get		, 0},
        {"title",		10, JSPROP_PERMANENT			, Dtitle_get		, Dtitle_set},
	
	{0,0,0,0,0}	
};

static JSClass global_class = {
	"global",JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,
	JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub
};

static JSClass doc_class = {
	"document",0,
	JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,
	JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub
};

#define DEF_FUNCTION(a) static JSBool a(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)

DEF_FUNCTION(FGwrite)
{
	TRACE;
	struct f_data_c *fd=GET_FD(context);
	int len;
	int pos;
	unsigned char *s;
	unsigned char *str;
	
	fprintf(stderr, "documeng.write called...\n");
	if (!fd->js)
		return;
	if (fd->js->write_pos == -1)
		return;
	if (fd->js->write_pos < 0)
		internal("js_upcall_document_write: js->write_pos trashed");

	str = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));
	len = JS_GetStringLength(JSVAL_TO_STRING(argv[0]));
	
	if (!fd->js->src) {
		unsigned char *s, *eof;
		struct fragment *f;
		if (!fd->rq || !fd->rq->ce) return;
		defrag_entry(fd->rq->ce);
		f = fd->rq->ce->frag.next;
		if ((void *)f == &fd->rq->ce->frag || f->offset) return;
		s = f->data, eof = f->data + f->length;
		if (!(fd->js->src = memacpy(s, eof - s))) return;
		fd->js->srclen = eof - s;
	}
	fprintf(stderr, "%p, %d bytes, adding %d bytes.\n", fd->js->src, fd->js->srclen, len);
	if (!(s = mem_realloc(fd->js->src, fd->js->srclen + len))) return;
	fd->js->src = s;
	if ((pos = fd->js->srclen - fd->js->write_pos) < 0)
		pos = 0;
	memmove(s + pos + len, s + pos, fd->js->srclen - pos);
	memcpy(s + pos, str, len);
	fd->js->srclen += len;
	fd->js->wrote = 1;

	return JS_TRUE;
}

DEF_FUNCTION(FGalert)
{
	TRACE;
	unsigned char *txt;
	const char *a;
	struct f_data_c *fd=GET_FD(context);
	struct terminal *term=fd->ses->term;
	char *b;
	a=JS_GetStringBytes(JS_ValueToString(context,argv[0]));
//	JS_SuspendRequest(context);

	if(!a)
		return JS_FALSE;
	b=mem_alloc(strlen(a)+1);
	memcpy(b,a,strlen(a));
	b[strlen(a)]=0;
	msg_box(
			term,   /* terminal */
			getml(b,NULL),
			"JS-Alert",   /* title */
			AL_CENTER,   /* alignment */
			b,   /* message */
			NULL,   /* data for button functions */
			1,   /* # of buttons */
			TEXT(T_OK),NULL,B_ENTER|B_ESC  /* first button */
		   );
	return JS_TRUE;
}

DEF_FUNCTION(FGconfirm)
{
	TRACE;
	unsigned char *txt;
	const char *a;
	struct f_data_c *fd=GET_FD(context);
	struct terminal *term=fd->ses->term;
	char *b;
	a=JS_GetStringBytes(JS_ValueToString(context,argv[0]));
	if(!a)
		return JS_FALSE;
	b=mem_alloc(strlen(a)+1);
	memcpy(b,a,strlen(a));
	b[strlen(a)]=0;
	msg_box(
			term,   /* terminal */
			getml(b,NULL),
			"JS-Confirm",   /* title */
			AL_CENTER,   /* alignment */
			b,   /* message */
			NULL,   /* data for button functions */
			2,   /* # of buttons */
			TEXT(T_YES),NULL,B_ENTER,  /* first button */
			TEXT(T_NO),NULL,B_ESC  /* first button */
		   );

	JS_NewDoubleValue(context, 1, rval);
	return JS_TRUE;
}

DEF_FUNCTION(FGclose)
{
	TRACE;
	fprintf(stderr,"JS: close unsupported!\n");
	return JS_FALSE;
}

DEF_FUNCTION(FGopen)
{
	TRACE;
	fprintf(stderr,"JS: open unsupported!\n");
	return JS_FALSE;
}

static void schedule_me(void*data)
{
	TRACE;
	struct s_schedule *s=data;
	s->fd->js->timers[s->timeri].timerid=-1;
    jsint_execute_code(s->fd,s->script,s->scriptlen,-1,-1,-1);
    mem_free(s->script);
    mem_free(s);
	s=NULL;
}   

DEF_FUNCTION(FGsetTimeout)
{
	TRACE;
	struct s_schedule *s;
	const char *a;
	struct f_data_c *fd=GET_FD(context);
	int timeri;
	jsdouble x;
	a=JS_GetStringBytes(JS_ValueToString(context,argv[0]));
	if(!a)
		return JS_FALSE;

	if (!JS_ValueToNumber(context,argv[1],&x))
        return JS_FALSE;

	fprintf(stderr,"Scheduling JS-Code(): %s\n",a);

	s=(struct s_schedule*)mem_alloc(sizeof(struct s_schedule));
	if(!s)
	{
		fprintf(stderr,"s: OUT OF MEM!\n");
		return JS_FALSE;
	}
	s->fd=fd;
	s->scriptlen=strlen(a);
	s->script=mem_alloc(s->scriptlen+1);
	if(!s->script)
	{
		fprintf(stderr,"script: OUT OF MEM!\n");
		free(s);
		return JS_FALSE;
	}
	memcpy(s->script,a,s->scriptlen);
	s->script[s->scriptlen]=0;
	for(timeri=0;timeri<MAXTIMERS;timeri++)
	{
		if(fd->js->timers[timeri].timerid==-1)
		{
			fprintf(stderr,"Installing timer: %d=",timeri);
			s->timeri=timeri;
			fd->js->timers[timeri].timersched=s;
			fd->js->timers[timeri].timerid=install_timer(x,(void(*)(void*))schedule_me,s); // saving to kill the timer ...
			fprintf(stderr,"%d\n",fd->js->timers[timeri].timerid);
			goto nofree;
		}
	}
	mem_free(s->script);
	mem_free(s);
nofree:
	return JS_TRUE;
}

DEF_FUNCTION(FGstop)
{
	TRACE;
	struct f_data_c *fd=GET_FD(context);
	stop_button_pressed(fd->ses);
}

DEF_FUNCTION(FGback)
{
	TRACE;
	struct f_data_c *fd=GET_FD(context);
	int a=0;
	struct location *loc;

        go_back(fd->ses,NULL);
/*	foreach(loc,fd->ses->history)
	{
		fprintf(stderr,"HIST(%d): %s\n",a,loc->url);
		if (a==-1)
		{
			break;
		}
		a++;
	}
	
	go_backwards(fd->ses->term,(void*)(jsid->n),fd->ses);
*/
        return JS_TRUE;
}

static JSFunctionSpec global_functions[]=
{
	{"alert",         FGalert,         1},
	{"confirm",       FGconfirm,       1},
	{"write",         FGwrite,         1},
	{"open",          FGopen,          1},
	{"close",         FGclose,         1},
	{"setTimeout",    FGsetTimeout,    1},
	{"stop",          FGstop,          1},
	{"back",          FGback,          1},
	{0}
};

//#define DUMMYFUNCTION_MINARGS 2

/*static JSFunctionSpec my_functions[] = {
//	{"dummyFunction", dummyFunction, DUMMYFUNCTION_MINARGS, 0, 0},
	{0,0,0,0,0}
};*/


static void errorreporter(JSContext *context, const char *message, JSErrorReport *report)
{
	TRACE;
	int pos=0;
	struct f_data_c *fd=NULL;
	JSObject *obj;
	fprintf(stderr,"*** JS-Error: '%s'",message);
	if(report&&report->filename)
		fprintf(stderr," in file \"%s\" at line %d\n    Code:  %s\n    Exact: %s",report->filename,report->lineno?report->lineno:-1,report->linebuf?report->linebuf:"NULL",report->tokenptr?report->tokenptr:"NULL");
	fprintf(stderr,"\n");
	obj=JS_GetGlobalObject(context);
	if(obj)
		fd=(struct f_data_c*)JS_GetPrivate(context,obj);
	js_error(message,(void*)fd);
	if(!strcmp(message,"out of memory"))
	{
		fprintf(stderr,"OUTOFMEM: stopping JS-Runtime!\n");
		if(fd&&fd->js)
			fd->js->running=0;
	}
}


int jsint_create(struct f_data_c *fd)
{
	TRACE;
	struct js_state *js;
	JSBool ok;
	int i=0;
	js = fd->js;

	if(!js_rt)
	{
		fprintf(stderr,"Creating new JS-Runtime with %d bytes of memory\n",js_memory_limit*1024);
		if(js_memory_limit<5)
		{
			fprintf(stderr,"MemoryLimit should be >= 5k!\n");
		}
		js_rt=JS_NewRuntime(js_memory_limit*1024);
		if(!js_rt)
		{
			fprintf(stderr,"ERR: Can't start JS-Runtime!\n");
			return 1;
		}
	}

	if(fd->js) internal("javascript state present");
	if(!(js = mem_calloc(sizeof(struct js_state))))
		return 1;
	js->running=1;
	for(i=0;i<MAXTIMERS;i++)
		js->timers[i].timerid=-1;

	fprintf(stderr,"Creating new Context\n");
	if(!(js->context=JS_NewContext(js_rt, STACK_CHUNK_SIZE)))
	{
		fprintf(stderr,"ERR: Can't allocate JS_Context!\n");
		mem_free(js);
		return 1;
	}
	js_use_count++;

	JS_SetErrorReporter(js->context,errorreporter);

	fprintf(stderr,"Creating new GlobalObject\n");
	js->globalObj = JS_NewObject(js->context, &global_class, 0, 0);
	if (!js->globalObj) {
		fprintf(stderr,"ERR: Can't create global object\n");
		mem_free(js);
		return 1;
	}

	ok = JS_InitStandardClasses(js->context, js->globalObj);
	if(!ok)
	{	
		fprintf(stderr,"ERR: Can't init standard classes!\n");
		mem_free(js);
		return 1;
	}

	JS_SetPrivate(js->context,JS_GetGlobalObject(js->context),(void*)fd);

	JS_InitClass(js->context,js->globalObj,NULL,&doc_class,NULL,0,doc_property,NULL,NULL,NULL);

	
	JS_DefineFunctions(js->context, js->globalObj, global_functions);

	fd->js=js;

	return 0;
}

void jsint_done_execution(struct f_data_c *fd)
{
	struct js_request *r;
	struct js_state *js = fd->js;
	if (!js) {
		internal("no js in frame");
		return;
	}

	/* accept all cookies set by the script */
	// jsint_set_cookies(fd,1);

	if (js->write_pos != -1) 
	{
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
	}
}

/* executes or queues javascript code in frame:
 *  write_pos is number of bytes from the position where document.write should write to the end of document
 *  write_pos == -1 if it is not from <SCRIPT> statement and cannot use document.write
 */
void jsint_execute_code(struct f_data_c *fd, unsigned char *code, int len, int write_pos, int onclick_submit, int onsubmit)
{
	TRACE;
	jsval rval;
	JSBool ok;
	char *filename;
	uintN lineno;
	char *rcode;

	if(!len)
	{
		fprintf(stderr,"No code ?!\n");
		return;
	}
/*	rcode=mem_alloc(len+1);
	if(!rcode)
	{
		fprintf(stderr,"Out of mem\n");
		return;
	}
	memcpy(rcode,code,len);
	rcode[len]=0;*/
	fprintf(stderr,"(%d)%s\n",len,code);

	if (!js_enable)return;

	if (!fd->js && jsint_create(fd))
	{
		fprintf(stderr,"Cannot create JS-Runtime!\n");
		return;
	}

	if(!fd->js->running)
	{
		fprintf(stderr,"JS disabled!\n");
		return;
	}


	fd->js->write_pos = write_pos;
	fprintf(stderr, "Write at offset %d...\n", fd->js->write_pos);

	fprintf(stderr,"Evaluating ...\n");
	ok = JS_EvaluateScript(fd->js->context, fd->js->globalObj, code, len, fd->f_data->script_href_base, 1, &rval);

	if(ok)
	{
		fprintf(stderr,"Code OK!\n");
//		ok=JS_ValueToNumber(js->context, rval, &d);
	}
	else
		fprintf(stderr,"Code NOT OK!\n");

//	mem_free(rcode);

	jsint_done_execution(fd);
}

void jsint_destroy(struct f_data_c *fd)
{
	TRACE;
	struct js_state *js = fd->js;
	int i;
	
	fd->script_t = 0;
	if (!js)
		return;
	
	for(i=0;i<MAXTIMERS;i++)
	{
		if(js->timers[i].timerid!=-1)
		{
			fprintf(stderr,"Killing timer: %d=%d\n",i,js->timers[i].timerid);
			kill_timer(js->timers[i].timerid);
			if(js->timers[i].timersched)
			{
				fprintf(stderr,"  Freeing up memory\n");
				free(js->timers[i].timersched->script);
				free(js->timers[i].timersched);
			}
		}
	}
	fd->js = NULL;

	fprintf(stderr,"Destroying context\n");
	JS_DestroyContext(js->context);

	if (js->src)
		mem_free(js->src);
/*	if (js->active)
		mem_free(js->active);*/
	mem_free(js);

	js_use_count--;
/*	if(js_use_count==0)
	{
		fprintf(stderr,"Killing JS-Runtime\n");
		JS_DestroyRuntime(js_rt);
		js_rt=NULL;
	}*/

	return;

/*	struct js_state *js = fd->js;
	fd->script_t = 0;
	if (!js) return;
	fd->js = NULL;
	pr(js_destroy_context(js->ctx)) return;
	if (js->src) mem_free(js->src);
	if (js->active) mem_free(js->active);
	free_list(js->queue);
	mem_free(js);
*/
}

void jsint_shutdown()
{
	if (!js_enable||!js_rt)
		return;

	if(js_use_count)
		fprintf(stderr,"JS-Runtime still in use! Force kill ...\n");

	fprintf(stderr,"Killing JS-Runtime\n");
	JS_DestroyRuntime(js_rt);
	js_rt=NULL;
}


void jsint_scan_script_tags(struct f_data_c *fd)
{
	TRACE;
	unsigned char *name, *attr;
	int namelen;
	unsigned char *val, *e, *ee;
	unsigned char *s, *ss, *eof;
	unsigned char *start, *end;
	unsigned char *onload_code=NULL;
	int uv;
	int bs;

	if (!js_enable || !fd->rq || !fd->rq->ce || !fd->f_data)
		return;
	if (!jsint_get_source(fd, &ss, &eof))
	{
		if (get_file(fd->rq, &ss, &eof))
			return;
/*		fd->js->src=ss;
		fd->js->srclen=(int)(eof-ss);*/
	}

	d_opt = &fd->f_data->opt;

	s = ss;
	/* search for <... */
se:
	while (s < eof && *s != '<')
		sp:s++;
	if (s >= eof || fd->script_t < 0)
	{
		if (onload_code && fd->script_t != -1)
		{
			jsint_execute_code(fd,onload_code,strlen(onload_code),-1,-1,-1);
		}
		fd->script_t = -1;
		goto ret;
	}
	/* skip comments <! <? */
	if (s + 2 <= eof && (s[1] == '!' || s[1] == '?')) {
		s = skip_comment(s, eof);
		goto se;
	}
	if (parse_element(s, eof, &name, &namelen, &attr, &s))
		goto sp;
	if (!onload_code&&namelen==4&&!casecmp(name,"BODY",4))
	{
		/* if the element doesn't have onload attribute get_attr_val returns NULL */
		onload_code=get_attr_val(attr,"onload");
		goto se;
	}

	if (!onload_code&&namelen==3&&!casecmp(name,"IMG",3))
	{
		/* if the element doesn't have onload attribute get_attr_val returns NULL */
		onload_code=get_attr_val(attr,"onload");
		goto se;
	}

	if (namelen != 6 || casecmp(name, "SCRIPT", 6) || s - ss < fd->script_t)
		goto se;
	start = end = NULL;
	if ((val = get_attr_val(attr, "src")))
	{
		unsigned char *url;
		if (fd->f_data->script_href_base && ((url = join_urls(fd->f_data->script_href_base, val))))
		{
			struct additional_file *af = request_additional_file(fd->f_data, url);
			mem_free(url);
			mem_free(val);
			if (!af || !af->rq)
				goto se;
			if (af->rq->state >= 0)
				goto ret;
			get_file(af->rq, &start, &end);
			if (start == end)
				goto se;
		}
		else
		{
			mem_free(val);
			goto se;
		}
	}
	e = s;
	uv = 0;
	bs = 0;
	while (e < eof && *e != '<')
	{
es:
		if (!uv && (*e == '"' || *e == '\'')) uv = *e, bs = 0;
		else if (*e == '\\' && uv) bs = 1;
		else if (*e == uv && !bs) uv = 0;
		else bs = 0;
		e++;
	}
	if (e + 8 <= eof)
	{
		if(casecmp(e,"</SCRIPT",8))
			goto es;
	}
	else
		e = eof;
	ee = e;
	while (ee < eof && *ee != '>')
		*ee++;
	if (ee < eof)
		ee++;
	fd->script_t = ee - ss;
	if (!start || !end)
		jsint_execute_code(fd, s, e - s, eof - ee,-1,-1);
	else
	if (start && end)
		jsint_execute_code(fd, start, end - start, eof - ee,-1,-1);
ret:
	if (onload_code)
		mem_free(onload_code);

	d_opt = &dd_opt;
}

void jsint_destroy_document_description(struct f_data *f)
{
	TRACE;
}

int jsint_get_source(struct f_data_c *fd, unsigned char **start, unsigned char **end)
{
	struct js_state *js = fd->js;
	TRACE;

	if (!js || !js->src) return 0;
	if (start) *start = js->src;
	if (end) *end = js->src + js->srclen;

    return 1;
}

/* for <a href="javascript:..."> */
void javascript_func(struct session *ses, unsigned char *code)
{
	TRACE;
	jsint_execute_code(current_frame(ses),code,strlen(code),-1,-1,-1);
}
#else
int jsint_get_source(struct f_data_c *fd, unsigned char **start, unsigned char **end)
{
}

#endif
