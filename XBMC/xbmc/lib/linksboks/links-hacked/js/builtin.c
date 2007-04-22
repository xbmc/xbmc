/* builtin.c
 * (c) 2002 Martin 'PerM' Pergel
 * This file is a part of the Links program, released under GPL.
 */

#include "../cfg.h"

#ifdef JS

#ifdef HAVE_MATH_H
#include <math.h>
#endif
#define __USE_XOPEN
#define _XOPEN_SOURCE
#include <time.h>
#undef _XOPEN_SOURCE
#undef __USE_XOPEN
#include <ctype.h>

#define PREFIX 0x40
#define DELKAJMENATIMERU 20
#define DELKACASU 100

#include "struct.h"
#include "typy.h"
#include "builtin_keys.h"
#include "builtin.h"
#include "ipret.h"
#include "ns.h"
#include "typy.h"

#undef MD5_CTX
/*#undef MD5_CTX*/
#include "md5.h"

/* PerM is pig and he uses rint. rint can crash when the values it too large */
#ifdef rint
#undef rint
#endif

static inline int my_rint(float f)
{
	if (f < -MAXINT+1) return -MAXINT;
	if (f > MAXINT-1) return MAXINT;
	if (f >= 0) return (int)(f + 0.5);
	return -(int)(-f + 0.5);
}

#define rint my_rint


lns fotr_je_lotr;
long js_lengthid/*=0*/;
long CStoString,CSvalueOf,CSMIN_VALUE,CSMAX_VALUE,CSNaN,CSlength,
	CSindexOf,CSlastIndexOf,CSsubstring,CScharAt,CStoLowerCase,CSsubstr,
	CStoUpperCase,CSsplit,CSparse,CSUTC;

#define INTFUNC(a)	pna=buildin(a,context->namespace,context->lnamespace,context);\
			pna->type=FUNKINT; \
			pna->value=i++;

#define INTMATVAR(a,b)	pna=buildin(a,context->namespace,context->lnamespace,context);\
			pna->type=INTVAR; \
			pna->value=0; \
			pna->mid=0; \
			flo=js_mem_alloc(sizeof(float)); \
			*flo=b; \
			pna->handler=(long)flo;

#define COLORVAR(a,b)	pna=buildin(a,context->namespace,context->lnamespace,context);\
			pna->type=INTVAR; \
			pna->value=Cbarvicka; \
			pna->mid=0; \
			pna->handler=b;

#define BIVAR(a,b,c,d)	pna=buildin(a,context->namespace,context->lnamespace,context);\
			pna->type=INTVAR; \
			pna->value=b; \
			pna->mid=c; \
			pna->handler=d;

#define BIVAR1(a,b,c,d)	pomvar=buildin(a,context->namespace,context->lnamespace,context);\
			pomvar->type=INTVAR; \
			pomvar->value=b; \
			pomvar->mid=c; \
			pomvar->handler=d;			

#define CBIVAR1(a,b,c,d)	if(a){	BIVAR1(a,b,c,d); \
					mem_free(a); \
			}

#define BUILDVAR(a,b) 	pomvar=buildin(a,context->namespace,context->lnamespace,context);\
			pomvar->type=INTVAR; \
			pomvar->value=b; \
			pomvar->mid=0; \
			pomvar->handler=0;

#define BUILDFCE(a,b)	pomvar=buildin(a,context->namespace,context->lnamespace,context);\
			pomvar->type=FUNKINT; \
			pomvar->value=b;

#define BUILDSFCE(a,b,c)	BUILDFCE(a,b);\
				pomvar->mid=c;

#define BAE(a,b)	pomvar=buildin(a,context->namespace,context->lnamespace,context);\
			pomvar->mid=0;\
			pomvar->handler=0;\
			switch(pomvar->type=b->typ)\
			{	case UNDEFINED: \
				case NULLOVY: \
				case BOOLEAN: \
				case INTEGER: \
				case FLOAT: \
				case STRING: \
				case FUNKCE: \
					pomvar->value=b->argument; \
				break; \
				case ADDRSPACE: \
					pomvar->type=ADDRSPACEP; \
				case ADDRSPACEP: \
				case ARRAY: \
					pomvar->value=b->argument; \
					pomvar1=lookup(MIN1KEY,(plns*)b->argument,context); \
					if(pomvar1->type!=PARLIST) \
						my_internal("Parent list corrupted!\n",context); \
					add_to_parlist(pomvar,pomvar1); \
				break; \
				default: \
					my_internal("Internal: Strange array-element assignment!\n",context); \
				break; \
			} \
			js_mem_free(b); 
		
#define idebug(a) fprintf(stderr,a);
#undef idebug
#define idebug(a)
			
long MIN1KEY;

extern int js_durchfall;

void js_volej_danka(js_bordylek*bordylek)
{	js_context*context=(js_context*)bordylek->context;
	*bordylek->mytimer=-1;
	js_mem_free(bordylek);
	context->callback(context->ptr);
}

void js_volej_kolbena(js_context*context)
{	int timerno=0;
	js_bordylek*bordylek;
	while(timerno<TIMERNO &&(context->t[timerno]!=-1))timerno++;
        if(timerno>=TIMERNO)
        {	js_error("Too many timers",context);
		timerno=0; /* Timery by mely byt po erroru vytlucene, ale pokud interpretace konci uspesne a zustal viset timer,
			    * tak se nesmi zahodit!! */
        }
	bordylek=js_mem_alloc(sizeof(js_bordylek));
	bordylek->context=context;
	bordylek->mytimer=&context->t[timerno];
	context->bordely[timerno]=bordylek;
	context->t[timerno]=install_timer(1,(void(*)(void*))js_volej_danka,bordylek);
}

static void schedule_me(void*data)
{	struct fax_me_tender_2_stringy *faxoff = data;
	js_context*context=faxoff->ident;
	borlist* na_bordel=context->bordel,*pom;
	char* kod=faxoff->string1;
	void*fdatac=context->ptr;
	*(int*)faxoff->string2=-1;
	js_mem_free(data);
	jsint_execute_code(fdatac,kod,strlen(kod),-1,-1,-1);
	if(!na_bordel)internal("Ztraceny text pri interpretovani!\n");
	if(na_bordel->binec==kod)
	{	pom=na_bordel->next;
		js_mem_free(na_bordel->binec);
		js_mem_free(na_bordel);
		context->bordel=pom;
	} else{	while(na_bordel->next && na_bordel->next->binec!=kod)
			na_bordel=na_bordel->next;
		if(!na_bordel->next)
			internal("Ztraceny text pri interpretovani!\n");
		pom=na_bordel->next;
		na_bordel->next=pom->next;
		js_mem_free(pom->binec);
		js_mem_free(pom);
	}
}

/* Dvojice funkci call_setTimeout a schedule_me slouzi k ovladani uzivatelskych
 * timeru v javascriptu. Takovychto timeru lze najednou udelat 127 (pak dojde
 * k js_erroru). Celkovy pocet timeru neni omezen. Dle ch56 od Netscapu mame
 * pri zavolani setTimeout vratit string, ktery jednoznacne popisuje nastaveny
 * timer, takze tady se to udela tak, ze se integer (zde nutne 32bitovy,
 * menebitovost nevadi pokud kompilator bude ochoten andovat s cislem vetsim,
 * nez je MAXINT, vicebitovost nebude vadit dokud cisla timeru nezacnou
 * prekracovat 2^32. Je to hloupe omezeni, ale zato je odolne vuci pokusum
 * o hackovani delanim timeru s divnym jmenem. Tech 32 bitu se natvrdo
 * rozseka do 6 bytoveho slova obsahujiciho znaky 64 - 127. Po zavolani
 * clearTimeout se z tohoto slova slozi jednoznacne cislo od 0 do 2^32 - 1.
 */

static char* call_setTimeout(char*string,int time,js_context*context)
{
	struct fax_me_tender_2_stringy *faxoff =js_mem_alloc(sizeof(struct fax_me_tender_2_stringy));
	int timerno=0;
	char* jmeno_timeru=js_mem_alloc(DELKAJMENATIMERU);
	borlist*novy_bordel=js_mem_alloc(sizeof(borlist));
	novy_bordel->next=context->bordel;
	context->bordel=novy_bordel;
	novy_bordel->binec=string;
	jmeno_timeru[0]='\0';
	while(timerno<TIMERNO &&(context->t[timerno]!=-1))timerno++;
	if(timerno>=TIMERNO)
	{	js_error("Too many timers",context);
		js_mem_free(faxoff);
		return jmeno_timeru;
	}
	faxoff->string2=(char*)&context->t[timerno];
	context->bordely[timerno]=(js_bordylek*)faxoff;
	faxoff->ident=context;
	faxoff->string1=string;
	
	context->t[timerno]=install_timer(time,(void(*)(void*))schedule_me,faxoff);
	timerno=context->t[timerno];
	jmeno_timeru[13]=(char)(PREFIX|(timerno&31));
	timerno>>=5;
	jmeno_timeru[12]=(char)(PREFIX|(timerno&31));
        timerno>>=5;
	jmeno_timeru[11]=(char)(PREFIX|(timerno&31));
        timerno>>=5;
	jmeno_timeru[10]=(char)(PREFIX|(timerno&31));
        timerno>>=5;
	jmeno_timeru[9]=(char)(PREFIX|(timerno&31));
        timerno>>=5;
	jmeno_timeru[8]=(char)(PREFIX|(timerno&31));
        timerno>>=5;
	jmeno_timeru[7]=(char)(PREFIX|(timerno&31));
        timerno>>=5;
	jmeno_timeru[6]=(char)(PREFIX|(timerno&31));
        timerno>>=5;
	jmeno_timeru[5]=(char)(PREFIX|(timerno&31));
        timerno>>=5;
	jmeno_timeru[4]=(char)(PREFIX|(timerno&31));
        timerno>>=5;
	jmeno_timeru[3]=(char)(PREFIX|(timerno&31));
        timerno>>=5;
	jmeno_timeru[2]=(char)(PREFIX|(timerno&31));
        timerno>>=5;
	jmeno_timeru[1]=(char)(PREFIX|(timerno&31));
        timerno>>=5;
	jmeno_timeru[0]=(char)(PREFIX|(timerno&31));
	jmeno_timeru[14]='\0';
/*	printf("Cislo: %d ",timerno);*/
/*	timerno&=0xffffffff;
	jmeno_timeru[0]=(char)(PREFIX|(timerno>>27));
	timerno&=0x07ffffff;
	jmeno_timeru[1]=(char)(PREFIX|(timerno>>22));
	timerno&=0x003fffff;
	jmeno_timeru[2]=(char)(PREFIX|(timerno>>17));
	timerno&=0x0001ffff;
	jmeno_timeru[3]=(char)(PREFIX|(timerno>>12));
	timerno&=0x00000fff;
	jmeno_timeru[4]=(char)(PREFIX|(timerno>>7));
	timerno&=0x0000007f;
	jmeno_timeru[5]=(char)(PREFIX|(timerno>>2));
	timerno&=0x00000003;
	jmeno_timeru[6]=(char)(PREFIX|timerno);
	jmeno_timeru[7]='\0';*/
	idebug(jmeno_timeru);
	return jmeno_timeru;
}

static void call_close(js_context*context)
{
	struct fax_me_tender_nothing *faxoff=js_mem_alloc(sizeof(struct fax_me_tender_nothing));
	faxoff->ident=context->ptr;
	context->upcall_data =faxoff;
	context->upcall_typek=TYPEK_NIC;
	if(context->upcall_timer!=-1)
		js_warning("TWO UPCALL TIMERS",context->current->lineno,context);
	context->upcall_timer= install_timer(1,(void(*)(void*))js_upcall_close_window,faxoff);
}

static void call_goto(js_context*context,char*string,long integer)
{	struct fax_me_tender_int_string *faxoff=js_mem_alloc(sizeof(struct fax_me_tender_int_string));
	faxoff->ident=context->ptr;
	if(!(faxoff->num=integer))
		faxoff->string=string;
	else
		faxoff->string=0;
	context->upcall_data=faxoff;
	context->upcall_typek=TYPEK_INT_STRING;
	if(context->upcall_timer!=-1)
		js_warning("TWO UPCALL TIMERS",context->current->lineno,context);
	context->upcall_timer=install_timer(1,(void(*)(void*))js_upcall_goto_history,faxoff);

}

static void call_setsrc(js_context*context,char*string,lns*pna)
{	struct fax_me_tender_string_2_longy * to_je_ale_woprusz=js_mem_alloc(sizeof(struct fax_me_tender_string_2_longy));
	to_je_ale_woprusz->ident=context->ptr;
	to_je_ale_woprusz->string=stracpy1(string);
	mem_free(string);
	to_je_ale_woprusz->doc_id=pna->handler;
	to_je_ale_woprusz->obj_id=pna->mid;
	context->upcall_data=to_je_ale_woprusz;
	context->upcall_typek=TYPEK_STRING_2_LONGY;
	js_durchfall=1;
	if(context->upcall_timer!=-1)
		js_warning("TWO UPCALL TIMERS",context->current->lineno,context);
	context->upcall_timer=install_timer(1,(void(*)(void*))js_upcall_set_image_src,to_je_ale_woprusz);
}

static void call_alert(js_context*context,char*string)
{
	struct fax_me_tender_string * faxoff=js_mem_alloc(sizeof(struct fax_me_tender_string));
	faxoff->ident=context->ptr;
	faxoff->string=string;
	context->upcall_data=faxoff;
	context->upcall_typek=TYPEK_STRING;
	if(context->upcall_timer!=-1)
		js_warning("TWO UPCALL TIMERS",context->current->lineno,context);
	context->upcall_timer= install_timer(1,(void(*)(void*))js_upcall_alert,faxoff);
}

static void call_open(js_context*context,char*string,int a)
{
	struct fax_me_tender_int_string*faxoff=js_mem_alloc(sizeof(struct fax_me_tender_int_string));
	faxoff->ident=context->ptr;
	faxoff->string=string;
	faxoff->num=a; /* Fixme! Open nema vzdycky udelat nove okno! */
	context->upcall_data=faxoff;
	context->upcall_typek=TYPEK_INT_STRING;
	if(context->upcall_timer!=-1)
		js_warning("TWO UPCALL TIMERS",context->current->lineno,context);
	context->upcall_timer= install_timer(1,(void(*)(void*))js_upcall_goto_url,faxoff);
}

static void call_confirm(js_context*context,char*string)
{
	struct fax_me_tender_string *faxoff=js_mem_alloc(sizeof(struct fax_me_tender_string));
	faxoff->ident=context->ptr;
	faxoff->string=string;
	context->upcall_data=faxoff;
	context->upcall_typek=TYPEK_STRING;
	if(context->upcall_timer!=-1)
		js_warning("TWO UPCALL TIMERS",context->current->lineno,context);
	context->upcall_timer= install_timer(1,(void(*)(void*))js_upcall_confirm,faxoff);
}

static void call_prompt(js_context*context,char*string1,char*string2)
{
	struct fax_me_tender_2_stringy *faxoff=js_mem_alloc(sizeof(struct fax_me_tender_2_stringy));
	faxoff->ident=context->ptr;
	faxoff->string1=string1;
	faxoff->string2=string2;
	context->upcall_data=faxoff;
	context->upcall_typek=TYPEK_2_STRINGY;
	if(context->upcall_timer!=-1)
		js_warning("TWO UPCALL TIMERS",context->current->lineno,context);
	context->upcall_timer= install_timer(1,(void(*)(void*))js_upcall_get_string,faxoff);
}

/* Funkce add_builtin pridava pri vzniku javascriptoveho kontextu vsechno, co
 * javascript umi "z fabriky", tj. vestavene metody (alert, confirm...),
 * vestavene objekty (document, frames, forms), vestavene vlastnosti 
 * (cudlik.jmeno, form.method...). Hledani pojmenovanych entit (elementu
 * formulare, formularu samotnych...) dle jmena se odehrava v souboru
 * ns.c, ktery handluje s namespacem jeste "pod" funkci add_builtin, tj.
 * tato funkce uz teoreticky (i prakticky) muze zpozorovat objekty vznikle
 * pojmenovanim html-objektu v dokumentu.
 */


//#include <unistd.h>
static unsigned int I=1,J=1,K=1;

static void randseed(unsigned int a,unsigned int b,unsigned int c)
{	I=a;J=b;K=c; return;
}

static unsigned int rnd(void)
{	I=69069*I + 23606797*I;
	J^=(J<<15);
	J^=(J>>18);
	K^=(K<<13);
	K^=(K>>17);
	K&=0x8fffffff;
	return I+J+K;
}

void buildin_document(js_context*context,long id)
{	lns*pna,*p1,*pomvar;
	plns*nsp;
	long j=0;
	p1=pna=buildin("document",context->namespace,context->lnamespace,context);
	idebug("Vstavam dokumentarni vlastnosti");
	pna->type=ADDRSPACEP;
	pna->value=(long)(nsp=js_mem_alloc(sizeof(plns)));
	nsp->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
	pna->handler=pna->mid=nsp->handler=nsp->mid=id;
	while(j<HASHNUM)nsp->ns[j++]=0;
	nsp->next=context->lnamespace;
	context->lnamespace=nsp;
	pna=create(MIN1KEY,context->lnamespace,context);
	pna->type=PARLIST;
	pna->value=0;
	add_to_parlist(p1,pna); /* Byrokracie kvuli garbage col. */
	BIVAR("location",Clocation,id,0);
	BUILDFCE("toString",CtoString);
	pomvar->handler=C_OBJ_document;
	BIVAR("images",Cimages,id,id);

	BUILDFCE("clear",Cclear);
	BUILDFCE("close",Cdclose);
	BUILDFCE("open",Cdopen);
	BUILDFCE("write",Cwrite);
	BUILDFCE("writeln",Cwriteln); 
	COLORVAR("alinkColor",0xff00);
	BIVAR("all",Call,id,id);
	BIVAR("anchors",Canchors,id,id);
	COLORVAR("bgColor",0xffffff);
	BIVAR("cookie",Ccookie,id,0);
	COLORVAR("fgColor",0); /* Nejsme zadny becka! */
	BIVAR("forms",Cforms,id,id);
	BIVAR("lastModified",ClastModified,id,0);
	COLORVAR("linkColor",0xff);
	BIVAR("links",Clinks,id,id);
	BIVAR("referer",Creferer,id,0);
	BIVAR("title",Ctitle,id,0);
	COLORVAR("vlinkColor",0xff0000);
	/* NetChcip este umi:
	 * applets, domain, embeds, height, images, width, URL */
	context->lnamespace=context->lnamespace->next;
	idebug("Dokument vstavan!\n");
}

void add_builtin(js_context*context)
{	lns*pna,*p1,*pomvar;
	plns* nsp;
	float* flo;
	long j=0;
	srand(time(0));
	randseed(rand(),rand(),rand());
	BUILDFCE("parseInt",CparseInt); /* Macro - syntax: INITFUNC(name); Je prirazena
			      konstanta, kterou si musime zapamatovat, zjistime
			      ji podle poradi. parseInt ma cislo 0, dalsi 1...
			      Az budeme funkci volat, rozhodneme podle cisla 
			      switchem (bude to rychle)*/
	BUILDFCE("parseFloat",CparseFloat); /* 1 */
	BUILDFCE("escape",Cescape);/* 2 */
	BUILDFCE("unescape",Cunescape);/* 3 */
	CStoString=buildin("toString",context->namespace,context->lnamespace,context)->identifier;
	CSvalueOf=buildin("valueOf",context->namespace,context->lnamespace,context)->identifier;
	CSMIN_VALUE=buildin("MIN_VALUE",context->namespace,context->lnamespace,context)->identifier;
	CSMAX_VALUE=buildin("MAX_VALUE",context->namespace,context->lnamespace,context)->identifier;
	CSNaN=buildin("NaN",context->namespace,context->lnamespace,context)->identifier;
	CSlength=buildin("length",context->namespace,context->lnamespace,context)->identifier;
	js_lengthid=CSlength; /* Fixme! Black magic! */
	CSindexOf=buildin("indexOf",context->namespace,context->lnamespace,context)->identifier;
	CSlastIndexOf=buildin("lastIndexOf",context->namespace,context->lnamespace,context)->identifier;
	CSsubstring=buildin("substring",context->namespace,context->lnamespace,context)->identifier;
	CSsubstr=buildin("substr",context->namespace,context->lnamespace,context)->identifier;
	CScharAt=buildin("charAt",context->namespace,context->lnamespace,context)->identifier;
	CStoLowerCase=buildin("toLowerCase",context->namespace,context->lnamespace,context)->identifier;
	CStoUpperCase=buildin("toUpperCase",context->namespace,context->lnamespace,context)->identifier;
	CSsplit=buildin("split",context->namespace,context->lnamespace,context)->identifier;
	CSparse=buildin("parse",context->namespace,context->lnamespace,context)->identifier;
	CSUTC=buildin("UTC",context->namespace,context->lnamespace,context)->identifier;

	pna=buildin("-1",context->namespace,context->lnamespace,context);
	MIN1KEY=pna->identifier;
	pna->type=PARLIST;
	pna->value=0;
	add_to_parlist(&fotr_je_lotr,pna);

	p1=pna=buildin("Math",context->namespace,context->lnamespace,context);
	idebug("Vstavam matiku\n");
	pna->type=ADDRSPACEP;
	pna->value=(long)(nsp=js_mem_alloc(sizeof(plns)));
	nsp->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
	nsp->mid=0; /* To je moje! Do toho at Mikulas nestoura! B;-) */
	nsp->handler=0;
	while(j<HASHNUM)nsp->ns[j++]=0; /* j se inicializuje na 0 */
	nsp->next=context->lnamespace;
	context->lnamespace=nsp;

	pna=create(MIN1KEY,context->lnamespace,context);
	pna->type=PARLIST;
	pna->value=0;
	add_to_parlist(p1,pna); /* objekt si musi ukazovat na sveho drzitele */

	BUILDFCE("toString",CtoString);
	pomvar->handler=C_OBJ_Math;	
	BUILDFCE("abs",Cabs); /* 4 */
	BUILDFCE("acos",Cacos); /* 5 */
	BUILDFCE("asin",Casin); /* 6 */
	BUILDFCE("atan",Catan); /* 7 */
	BUILDFCE("atan2",Catan2); /* 8 */
	BUILDFCE("ceil",Cceil); /* 9 */
	BUILDFCE("cos",Ccos); /* 10 */
	BUILDFCE("exp",Cexp); /* 11 */
	BUILDFCE("log",Clog); /* 12 */
	BUILDFCE("max",Cmax); /* 13 */
	BUILDFCE("min",Cmin); /* 14 */
	BUILDFCE("pow",Cpow); /* 15 */
	BUILDFCE("random",Crandom); /* 16 */
	BUILDFCE("round",Cround); /* 17 */
	BUILDFCE("sin",Csin); /* 18 */
	BUILDFCE("sqrt",Csqrt); /* 19 */
	BUILDFCE("tan",Ctan); /* 20 */
	BUILDFCE("floor",Cfloor);
	BUILDFCE("md5",Cmd5);

	INTMATVAR("E",2.718281828459045);
	INTMATVAR("LN2",0.6931471805599453);
	INTMATVAR("LN10",2.302585092994046);
	INTMATVAR("LOG2E",1.4426950408889634);
	INTMATVAR("LOG10E",0.4342944819032518);
	INTMATVAR("PI",3.141592653589793);
	INTMATVAR("SQRT1_2",0.7071067811865476);
	INTMATVAR("SQRT2",1.4142135623730951);
/*	INTVAR("",);
	INTVAR("",);
	INTVAR("",);
	INTVAR("",); Tohle je rezerva... */
	
	context->lnamespace=context->lnamespace->next;
	idebug("Matika vstavana\n");

	BUILDFCE("toString",CtoString);
	pomvar->handler=C_OBJ_window;

	BUILDFCE("Array",CArray); /* 21 */
	BUILDFCE("Object",CArray);
	BUILDFCE("Boolean",CBoolean); /* 22 */
	BUILDFCE("Date",CDate); /* 23 */
	BUILDFCE("Number",CNumber); /* 24 */
	BUILDFCE("String",CString); /* 25 */
	BUILDFCE("alert",Calert); /* 26 */

	p1=pna=buildin("screen",context->namespace,context->lnamespace,context);
	idebug("Vstavam skrin!\n");
	pna->type=ADDRSPACEP;
	pna->value=(long)(nsp=js_mem_alloc(sizeof(plns)));
	nsp->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
	nsp->mid=0;
	nsp->handler=0;
	j=0;
	while(j<HASHNUM)nsp->ns[j++]=0;
	nsp->next=context->lnamespace;
	pna=create(MIN1KEY,nsp,context);
	pna->type=PARLIST;
	pna->value=0;
	add_to_parlist(p1,pna);
	idebug("Skrin stoji!\n");

/*	p1=pna=buildin("document",context->namespace,context->lnamespace,context);
	idebug("Vstavam dokumentarni vlastnosti!\n");
	pna->type=ADDRSPACEP;
	pna->value=(long)(nsp=js_mem_alloc(sizeof(plns)));
	nsp->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
	pna->handler=pna->mid=nsp->handler=nsp->mid=js_upcall_get_document_id(context->ptr);
	j=0;
	while(j<HASHNUM)nsp->ns[j++]=0;
	nsp->next=context->lnamespace;
	context->lnamespace=nsp;
	pna=create(MIN1KEY,context->lnamespace,context);
	pna->type=PARLIST;
	pna->value=0;
	add_to_parlist(p1,pna); //Vyplnili jsme byrokracii kvuli garbage col.
	
	BIVAR("location",Clocation,js_upcall_get_document_id(context->ptr),0);

	BUILDFCE("toString",CtoString);
	pomvar->handler=C_OBJ_document;

	j=js_upcall_get_document_id(context->ptr);
	BIVAR("images",Cimages,j,j);
	INTFUNC("clear"); // 27 
	INTFUNC("close"); // 28 
	INTFUNC("open"); // 29 
	INTFUNC("write"); // 30 
	INTFUNC("writeln"); // 31 
	j=context->lnamespace->mid; // Zakompostujem id dokumentu
	COLORVAR("alinkColor",0xff00);
	BIVAR("anchors",Canchors,j,j);
	COLORVAR("bgColor",0xffffff);
	BIVAR("cookie",Ccookie,j,0);
	COLORVAR("fgColor",0); // Nejsme zadny becka!
	BIVAR("forms",Cforms,j,j);
	BIVAR("lastModified",ClastModified,j,0);
	COLORVAR("linkColor",0xff);
	BIVAR("links",Clinks,j,j);
	BIVAR("referer",Creferer,j,0);
	BIVAR("title",Ctitle,j,0);
	COLORVAR("vlinkColor",0xff0000);
	// NetChcip este umi:
	// applets, domain, embeds, height, images, width, URL
	context->lnamespace=context->lnamespace->next;
	idebug("Dokument vstavan!\n");*/

	buildin_document(context,js_upcall_get_document_id(context->ptr));

	BIVAR("location",Clocation,js_upcall_get_document_id(context->ptr),j);

	p1=pna=buildin("navigator",context->namespace,context->lnamespace,context);
	idebug("Vstavam smiracke schopnosti\n");
	pna->type=ADDRSPACEP;
	pna->value=(long)(nsp=js_mem_alloc(sizeof(plns)));
	nsp->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
	nsp->handler=nsp->mid=0;
	j=0;
	while(j<HASHNUM)nsp->ns[j++]=0;
	nsp->next=context->lnamespace;
	context->lnamespace=nsp;
	pna=create(MIN1KEY,context->lnamespace,context);
	pna->type=PARLIST;
	pna->value=0;
	add_to_parlist(p1,pna);

	BUILDFCE("toString",CtoString);
	pomvar->handler=C_OBJ_navigator;

	BIVAR("appCodeName",CappCodeName,0,0);
	BIVAR("appName",CappName,0,0);
	BIVAR("appVersion",CappVersion,0,0);
	BIVAR("userAgent",CuserAgent,0,0);
	BUILDFCE("javaEnabled",CjavaEnabled);

	context->lnamespace=context->lnamespace->next;
	idebug("Navigator vstavan\n");
	
	BUILDFCE("confirm",Cconfirm); /* 32 */
	BUILDFCE("close",Cwclose); /* 33 */ /* tohle je window close, document close je blbe! */
	BUILDFCE("open",Cwopen); /* 34 */
	BUILDFCE("prompt",Cprompt); /* 35 */
	BUILDFCE("setTimeout",CsetTimeout); /* 36 */
	BUILDFCE("clearTimeout",CclearTimeout); /* 37 */


	p1=pna=buildin("history",context->namespace,context->lnamespace,context);
        idebug("Vstavam historii\n");
        pna->type=ADDRSPACEP;
        pna->value=(long)(nsp=js_mem_alloc(sizeof(plns)));
        nsp->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
        nsp->handler=nsp->mid=js_upcall_get_document_id(context->ptr);
        j=0;
        while(j<HASHNUM)nsp->ns[j++]=0;
        nsp->next=context->lnamespace;
        context->lnamespace=nsp;
        pna=create(MIN1KEY,context->lnamespace,context);
        pna->type=PARLIST;
        pna->value=0;
        add_to_parlist(p1,pna);
        j=context->lnamespace->mid;

        BIVAR("length",Chistorylength,j,0);
	BUILDFCE("back",Cback); /* 38 */
	BUILDFCE("forward",Cforward); /* 39 */
	BUILDFCE("go",Cgo); /* 40 */

        context->lnamespace=context->lnamespace->next;
        idebug("Historie vstavana\n");

	BUILDFCE("Image",CImage); /* 41 */
	BUILDFCE("eval",Ceval); /* 42 */
	BUILDFCE("isNaN",CisNaN);

/*	printf("Co se ma dat funkci js_get_window_id???\b\b\b\n");*/
	j=js_upcall_get_window_id(context->ptr);
	BIVAR("defaultStatus",CdefaultStatus,j,0);
	BIVAR("frames",Cframes,j,j);
	BIVAR("length",Cwlength,j,0);
	BIVAR("name",Cname,j,0);
	BIVAR("status",Cstatus,j,0);
	BIVAR("parent",Cparent,j,j);
	pna->index=(long)context->lnamespace;
/*	pna->type=ADDRSPACEP;
	p1=lookup(MIN1KEY,context->lnamespace,context);
	if(p1->type!=PARLIST)
		my_internal("Parentlist corrupted!\n",context);
	add_to_parlist(pna,p1); */
	BIVAR("self",Cself,j,j);
	pna->index=(long)context->lnamespace;
/*	pna->type=ADDRSPACEP;
	add_to_parlist(pna,p1); */
	BIVAR("top",Ctop,j,j);
	pna->index=(long)context->lnamespace;
/*	pna->type=ADDRSPACEP;
	add_to_parlist(pna,p1); */
	BIVAR("window",Cself,j,j);
	pna->index=(long)context->lnamespace;
/*	pna->type=ADDRSPACEP;
	add_to_parlist(pna,p1); */
	
/*	Tady jeste pribude document, window, navigator,
 *	Math - konstanty - jsou RO. Asi bude potreba typ VARINTRO, VARINTRW*/
	
}

abuf* force_getarg(abuf** odkud)
{	abuf* vysl=getarg(odkud);
	if(!vysl){vysl=js_mem_alloc(sizeof(abuf));
		vysl->typ=UNDEFINED;
		vysl->argument=0;
	}
	return vysl;
}

/* Stycne misto javascriptu a vnitrnich funkci. Zavola-li javascript nejakou
 * vestavenou funkci (alert, parseInt, escape, ...), dojde k obslouzeni
 * tohoto zavolani prave zde. Stejne tak se tady odehravaji podstatne akce
 * pri volani internich konstruktoru (vyroba pole, boolskeho objektu, ciselneho
 * objektu,...).
 */

static struct tm * casek;

int vartoint(lns*pna,js_context*context);

void sezer_zizalu(char*argv,js_context*context)
{	char*naargy=js_mem_alloc(strlen(argv)+2);
	time_t*cas=js_mem_alloc(sizeof(time_t));
	strcpy(naargy,argv);
	strcat(naargy,"~");
	time(cas);
	casek=localtime(cas);
	js_mem_free(cas);
#ifdef HAVE_STRPTIME
	if(naargy[0]>57)/*Je to mon,day,year...*/
	{	if(!strptime(naargy,"%h %d, %Y, %R:%S~",casek))
			if(!strptime(naargy,"%h %d, %y, %R:%S~",casek))
			if(!strptime(naargy,"%h %d, %Y, %R~",casek))
			if(!strptime(naargy,"%h %d, %y, %R~",casek))
			if(!strptime(naargy,"%h %d, %Y, %H~",casek))
			if(!strptime(naargy,"%h %d, %y, %H~",casek))
			if(!strptime(naargy,"%h %d, %Y~",casek))
			if(!strptime(naargy,"%h %d, %y~",casek))
			if(!strptime(naargy,"%h/%d/%y~",casek))
			if(!strptime(naargy,"%h/%d/%Y~",casek))
				js_warning("Set strange time!",context->current->lineno,context);
	} else
	{	if(!strptime(naargy,"%d %h, %Y %R:%S~",casek))
		if(!strptime(naargy,"%d %h, %y %R:%S~",casek))
		if(!strptime(naargy,"%d %h, %Y %R~",casek))
		if(!strptime(naargy,"%d %h, %y %R~",casek))
		if(!strptime(naargy,"%d %h, %Y %H~",casek))
		if(!strptime(naargy,"%d %h, %y %H~",casek))
		if(!strptime(naargy,"%d %h, %Y~",casek))
		if(!strptime(naargy,"%d %h, %y~",casek))
			js_warning("Set strange time!",context->current->lineno,context);
	}
#endif
	js_mem_free(naargy);
	js_mem_free(argv);
}

void js_intern_fupcall(js_context*context,long klic,lns*variable)	
{	/* Tady se pretypuji argumenty na seznam stringu a probublaji se
	   prislusnym handlerum. Pak se zase vysledek musi pretypovat
	   na abuf* a poslat zpatky dolu. Na vrcholu zasobniku mame
	   argumenty, po nasem zkonceni tam musime dat vysledek (i kdyby
	   mel byt void */
	abuf*argy=pulla(context),*vysl=js_mem_alloc(sizeof(abuf));
	abuf*pomarg,*pomarg1;
	char*pomstr,*pomstr1,*pomstr2,*cislo;
	long pomint, pomint1=0, pomint2;
	long*pompointer;
	int i=0,j=0;
	time_t*cas;
	char pomchar;

	plns*pomns;
	lns* pomvar,*pomvar1,*pomvar2;
	float * pomfloat,f1,f2;
	long rettype=UNDEFINED; /* Typek vraceneho vysledku */
	long retval=0; /* Hodnota vraceneho vysledku */
	switch(klic)
	{	case CparseInt:
			idebug("ParseInt called ");
			pomstr=tostring(force_getarg(&argy),context);
			pomarg=force_getarg(&argy);
			if(pomarg->typ==UNDEFINED)
				pomarg->typ=NULLOVY;
			pomint=to32int(pomarg,context);
			if(!strcmp(pomstr,"undefined"))
			{	zrusargy(argy,context);
				if(!options_get_bool("js_all_conversions"))
					js_error("Parsing undefined value!\n",context);
				else
				{	pomstr[0]='0';
					pomstr[1]='\0';
				}
			}
			if(!strcmp(pomstr,"null")|| !strcmp(pomstr,"false"))
			{	pomstr[0]='0';
				pomstr[1]='\0';
			}
			if(!strcmp(pomstr,"true"))
			{	pomstr[0]='1';
				pomstr[1]='\0';
			}
			retval=strtol(pomstr,(char**)0,pomint);
			rettype=INTEGER;
			js_mem_free(pomstr);
		break;
		case CparseFloat:
			idebug("ParseFloat called ");
			pomstr=tostring(force_getarg(&argy),context);
			if(!strcmp(pomstr,"undefined"))
			{	zrusargy(argy,context);
				js_mem_free(vysl);
				js_mem_free(pomstr); /* snad konsolidovano */
				if(!options_get_bool("js_all_conversions"))
					js_error("Parsing undefined value!\n",context);
				else{	pomstr[0]='0';
					pomstr[1]='\0';
				}
			}
			if(!strcmp(pomstr,"null")|| !strcmp(pomstr,"false"))
			{	pomstr[0]='0';
				pomstr[1]='\0';
			}
			if(!strcmp(pomstr,"true"))
			{	pomstr[0]='1';
				pomstr[1]='\0';
			}
			pomfloat=js_mem_alloc(sizeof(float));
			*pomfloat=atof(pomstr);
			if(*pomfloat!=MY_NAN)
			{	if(*pomfloat>MY_MAXDOUBLE)
					*pomfloat=MY_INFINITY;
				if(*pomfloat<MY_MINDOUBLE)
					*pomfloat=MY_MININFINITY;
			}
			js_mem_free(pomstr);
			rettype=FLOAT;
			retval=(long)pomfloat;
		break;
		case CisNaN:
			idebug("isNaN called\n ");
			f1=tofloat(force_getarg(&argy),context);
			rettype=BOOLEAN;
			if(f1==MY_NAN)
				retval=TRUE;
			else	retval=FALSE;
			idebug("and exited\n");
		break;
		case Cescape:
			idebug("Escape called\n ");
			pomstr=tostring(force_getarg(&argy),context);
			pomstr1=js_mem_alloc((1+strlen(pomstr))*3);
			i=j=0;
			while(pomstr[i])
				if(pomstr[i]>64 && pomstr[i]<123)pomstr1[j++]=pomstr[i++];
				else
				{	pomchar=pomstr[i++];
					pomstr1[j++]='%';
					if(pomchar/16<10)pomstr1[j++]=pomchar/16+48;
					else	pomstr1[j++]=87+pomchar/16;
					if((pomchar=pomchar%16)<10)pomstr1[j++]=pomchar+48;
					else	pomstr1[j++]=87+pomchar;
				}
			pomstr1[j]='\0';
			js_mem_free(pomstr);
			rettype=STRING;
			retval=(long)pomstr1;
			idebug("and exited!\n");
					
		break;
		case Cunescape:
			idebug("Unescape called ");
			pomstr=tostring(force_getarg(&argy),context);
			pomstr1=js_mem_alloc(1+strlen(pomstr));
			i=j=0;
			while(pomstr[i])
				if(pomstr[i]!='%')pomstr1[j++]=pomstr[i++];
				else
				{	pomint=1;
					if(pomstr[++i]<48) js_warning("Invalid escaped sequence",context->current->lineno,context),pomint=0;
					else
						if(pomstr[i]<58)
							pomint1=pomstr[i]-48;
						else	if(pomstr[i]<65)
								js_warning("Invalid escaped sequence",context->current->lineno,context),pomint=0;
							else {  pomint1=toupper(pomstr[i])-55;
								if(pomint1<10 || pomint1>15)    js_warning("Invalid escaped sequence",context->current->lineno,context), pomint=0;
							}
					if(!pomint)
					{	pomint1=2;
						pomint2=0;
						i++;
					} else{ if(pomstr[++i]<48) js_warning("Invalid escaped sequence",context->current->lineno,context),pomint2=0;
						else	if(pomstr[i]<58)
								pomint2=pomstr[i]-48;
							else {  pomint2=toupper(pomstr[i])-55;
								if(pomint2<10 || pomint2>15) js_warning("Invalid escaped sequence",context->current->lineno,context), pomint2=0;
							}
					}
					i++;
					pomstr1[j++]=(char)16*pomint1+pomint2;
				}
			pomstr1[j]='\0';
			
			rettype=STRING;
			retval=(long)pomstr1;
			js_mem_free(pomstr);
			idebug("and exited!\n");
		break;
		case Cabs:
			idebug("Abs called ");
			f1=tofloat(force_getarg(&argy),context);
			pomfloat=js_mem_alloc(sizeof(float));
			if((f1!=MY_MAXDOUBLE)&&(f1!=MY_MINDOUBLE)&&(f1!=MY_NAN)&&(f1!=MY_INFINITY)&&(f1!=MY_MININFINITY))
			{	*pomfloat=fabs(f1);
				if(*pomfloat!=MY_NAN)
				{	if(*pomfloat<MY_MINDOUBLE)
						*pomfloat=MY_MININFINITY;
					if(*pomfloat>MY_MAXDOUBLE)
						*pomfloat=MY_INFINITY;
				}
			} else{	if(f1==MY_NAN)
					*pomfloat=MY_NAN;
				if(f1==MY_MAXDOUBLE || f1==MY_MINDOUBLE)
					*pomfloat=MY_MAXDOUBLE;
				if(f1==MY_INFINITY ||f1==MY_MININFINITY)
					*pomfloat=MY_INFINITY;
			}
			rettype=FLOAT;
			retval=(long)pomfloat;
		break;
		case Cacos:
			idebug("Acos called ");
			f1=tofloat(force_getarg(&argy),context);
			pomfloat=js_mem_alloc(sizeof(float));
			if(f1<=1 && f1>=-1)
				*pomfloat=acos(f1);
			else
				*pomfloat=MY_NAN;
			rettype=FLOAT;
			retval=(long)pomfloat;
		break;
		case Casin:
			idebug("Asin called ");
			f1=tofloat(force_getarg(&argy),context);
			pomfloat=js_mem_alloc(sizeof(float));
			if(f1<=1 && f1>=-1)
				*pomfloat=asin(f1);
			else
				*pomfloat=MY_NAN;
			rettype=FLOAT;
			retval=(long)pomfloat;
		break;
		case Catan:
			idebug("Atan called ");
			f1=tofloat(force_getarg(&argy),context);
			pomfloat=js_mem_alloc(sizeof(float));
			*pomfloat=atan(f1);
			rettype=FLOAT;
			retval=(long)pomfloat;
		break;
		case Catan2:
			idebug("Atan2 called ");
			f1=tofloat(force_getarg(&argy),context);
			f2=tofloat(force_getarg(&argy),context);
			pomfloat=js_mem_alloc(sizeof(float));
			*pomfloat=atan2(f2,f1);
			rettype=FLOAT;
			retval=(long)pomfloat;
		break;
		case Cceil:
			idebug("Ceil called ");
			f1=tofloat(force_getarg(&argy),context);
			pomfloat=js_mem_alloc(sizeof(float));
			if((f1!=MY_MAXDOUBLE)&&(f1!=MY_MINDOUBLE)&&(f1!=MY_NAN)&&(f1!=MY_INFINITY)&&(f1!=MY_MININFINITY))
			{	
				if (f1 <= -MAXINT+1 || f1 >= MAXINT-1) *pomfloat = f1;
				else {
					*pomfloat=ceil(f1);
					if(*pomfloat!=MY_NAN)
					{	if(*pomfloat<MY_MINDOUBLE)
							*pomfloat=MY_MININFINITY;
						if(*pomfloat>MY_MAXDOUBLE)
							*pomfloat=MY_INFINITY;
					}
				}
			} else{	*pomfloat=f1;}
			
			retval=(long)pomfloat;
			rettype=FLOAT;
		break;
		case Ccos:
			idebug("Cos called ");
			f1=tofloat(force_getarg(&argy),context);
			pomfloat=js_mem_alloc(sizeof(float));
			*pomfloat=cos(f1);
			rettype=FLOAT;
			retval=(long)pomfloat;
			idebug("and exited!\n");
		break;
		case Clog:
			idebug("Log called ");
			f1=tofloat(force_getarg(&argy),context);
			pomfloat=js_mem_alloc(sizeof(float));
			if(f1<0)
				*pomfloat=MY_NAN;
			else
			{	if(f1<exp(MY_MINDOUBLE))
					*pomfloat=MY_MININFINITY;
				else {	*pomfloat=log(f1);
					if(*pomfloat>MY_MAXDOUBLE) /* Tohle je jen tak mezi nama trochu zbytecne B-) */
						*pomfloat=MY_INFINITY;
					if(*pomfloat<MY_MINDOUBLE)
						*pomfloat=MY_MININFINITY;
				}
			}
			rettype=FLOAT;
			retval=(long)pomfloat;
		break;
		case Cexp:
			idebug("Exp called ");
			f1=tofloat(force_getarg(&argy),context);
			pomfloat=js_mem_alloc(sizeof(float));
			if(f1>log(MY_MAXDOUBLE))
				*pomfloat=MY_INFINITY;
			else {	*pomfloat=exp(f1);
				if(*pomfloat>MY_MAXDOUBLE)
					*pomfloat=MY_INFINITY;
				if(*pomfloat<MY_MINDOUBLE) /* Tohle je jen tak mezi nama trochu zbytecne B-) */
					*pomfloat=MY_MININFINITY;
			}
			rettype=FLOAT;
			retval=(long)pomfloat;
		break;
		case Cmax:
			idebug("Max called ");
			f1=tofloat(force_getarg(&argy),context);
			f2=tofloat(force_getarg(&argy),context);
			pomfloat=js_mem_alloc(sizeof(float));
			*pomfloat=((f1>f2)?f1:f2);
			rettype=FLOAT;
			retval=(long)pomfloat;
		break;
		case Cmin:
			idebug("Min called ");
			f1=tofloat(force_getarg(&argy),context);
			f2=tofloat(force_getarg(&argy),context);
			pomfloat=js_mem_alloc(sizeof(float));
			*pomfloat=((f1<f2)?f1:f2);
			rettype=FLOAT;
			retval=(long)pomfloat;
		break;
		case Cpow:
			idebug("Pow called ");
			f1=tofloat(force_getarg(&argy),context);
			f2=tofloat(force_getarg(&argy),context);
			pomfloat=js_mem_alloc(sizeof(float));
			/* FIXME! Tady to muze vyzit! */
			*pomfloat=pow(f1,f2);
			rettype=FLOAT;
			retval=(long)pomfloat;
		break;
		case Crandom:
			idebug("Random called ");
			rettype=FLOAT;
			pomfloat=js_mem_alloc(sizeof(float));
			*pomfloat=((float)rnd())/UINT_MAX; /* Vracime uniforme mezi 0 a 1 */
			retval=(long)pomfloat;
		break;
		case Cround:
			rettype=FLOAT;
                        retval=(long)js_mem_alloc(sizeof(float));
			f1=tofloat(force_getarg(&argy),context);
			if(f1!=MY_NAN)
			{	
				if (f1 <= -MAXINT+1 || f1 >= MAXINT-1) {
					*(float*)retval=f1;
				} else {
					if (f1 >= 0) *(float*)retval=(int)(f1+0.5);
					else *(float*)retval=-(int)((-f1)+0.5);
					if((*(float*)retval)>MY_MAXDOUBLE)
						*(float*)retval=MY_INFINITY;
					if((*(float*)retval)<MY_MINDOUBLE)
						*(float*)retval=MY_MININFINITY;
					if(((float)(pomint=*(float*)retval))==*(float*)retval)
					{       rettype=INTEGER;
						js_mem_free((float*)retval);
						retval=pomint;
					}
				}
				idebug("je to cele!\n");
			} else	*(float*)retval=MY_NAN;
		break;
		case Csin:
			idebug("Sin called ");
			f1=tofloat(force_getarg(&argy),context);
			pomfloat=js_mem_alloc(sizeof(float));
			*pomfloat=sin(f1);
			rettype=FLOAT;
			retval=(long)pomfloat;
		break;
		case Csqrt:
			idebug("Sqrt called ");
			f1=tofloat(force_getarg(&argy),context);
			pomfloat=js_mem_alloc(sizeof(float));
			if(f1<0)
				*pomfloat=MY_NAN;
			else
				*pomfloat=sqrt(f1);
			rettype=FLOAT;
			retval=(long)pomfloat;
		break;
		case Ctan:
			idebug("Tan called ");
			f1=tofloat(force_getarg(&argy),context);
			pomfloat=js_mem_alloc(sizeof(float));
			/* FIXME! Tohle je potreba osetrit proti SIGFPE! */
			*pomfloat=tan(f1);
			rettype=FLOAT;
			retval=(long)pomfloat;
                break;
		case CArray:
			idebug("Array called ");
			pomarg=getarg(&argy);
			pomarg1=getarg(&argy);
			rettype=ARRAY;
			retval=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomns->next=context->lnamespace;
			pomns->mid=0;
			pomns->handler=0;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0; /* Bezprizorny namespace */
			pomint=0;
			if(pomarg)
			{	RESOLV(pomarg);
				if(pomarg1)
				{	/* Je to seznam (udelat)*/
					BAE("0",pomarg);
					RESOLV(pomarg1);
					BAE("1",pomarg1);
					pomstr=js_mem_alloc(DELKACISLA);
					pomint=2;
					while((pomarg=getarg(&argy)))
					{	snprintf(pomstr,DELKACISLA,"%d",(int)(pomint++));
						pomstr[DELKACISLA-1]='\0';
						RESOLV(pomarg);
						BAE(pomstr,pomarg);
					}
					js_mem_free(pomstr);
					
				} else 
				{	if(pomarg->typ!= INTEGER)
					{	BAE("0",pomarg);
					} else
					{	pomint=pomarg->argument;
						if(pomint<0)js_error("Invalid length of array!\n",context);
						/* Je to delka */
						js_mem_free(pomarg);
					}
				}
			}

			BUILDVAR("length",Clength);
			js_lengthid=pomvar->identifier; 
			pomvar->handler=pomint; /* Nastavim inicialni delku */
			BUILDFCE("join",Cjoin);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("reverse",Creverse);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("sort",Csort);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("shift",Cshift);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("unshift",Cunshift);
			pomvar->handler=(long)context->lnamespace;
/*			BUILDFCE("slice",Cslice);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("splice",Csplice);
			pomvar->handler=(long)context->lnamespace;*/
			BUILDFCE("toString",CtoString);
			pomvar->handler=C_OBJ_Objekt;
			/*zahashovavame argumenty 0,1,2,3,..., jinak*/
			context->lnamespace=context->lnamespace->next;
			idebug("and exited\n");
		break;
		case CImage:
/*			debug("CImage is only fake!!");*/
			rettype=ADDRSPACE;
			retval=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomns->next=context->lnamespace;
			pomns->mid=0;
			pomns->handler=0;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0;
			context->lnamespace=context->lnamespace->next;
		break;
		case CBoolean:
			idebug("CBoolean called ");
			pomarg=getarg(&argy);
			rettype=BOOLEAN;
			if(pomarg)
				retval=tobool(pomarg,context);
			else	retval=FALSE;
			idebug("and exited\n");
		break;
		case CNumber:
			idebug("CNumber called ");
			pomarg=getarg(&argy);
			rettype=FLOAT;
			if(pomarg)
			{	pomfloat=js_mem_alloc(sizeof(float));
				*pomfloat=tofloat(pomarg,context);
				retval=(long)pomfloat;
			} else{ retval=0;
				rettype=INTEGER;
			}
			idebug("and exited\n");
		break;
		case CDate:
			idebug("CDate called ");
			rettype=ADDRSPACE;
			retval=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomns->next=context->lnamespace;
			pomns->mid=0;
			pomns->handler=0;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0; /* Bezprizorny namespace */
			pomarg=getarg(&argy);
			
			if(pomarg && pomarg->typ!=UNDEFINED)
			{	if(pomarg->typ == STRING)
				{	sezer_zizalu((char*)pomarg->argument,context);
					js_mem_free(pomarg);
				}
				else{	pomint1=to32int(pomarg,context);
					if(!(pomarg=getarg(&argy)))
					{/*	pomint=to32int(pomarg,context);*/
						time_t t = (time_t)pomint1;
						casek=localtime(&t);
						/* Je to doba od usvitu dejin a aby toho nebylo malo tak v sekundach :-( */
					}
					else
					{	cas=js_mem_alloc(sizeof(time_t));
						time(cas);
						casek=localtime(cas);
						js_mem_free(cas);
						casek->tm_year=pomint1;
						casek->tm_mon=to32int(pomarg,context);
						if((pomarg=getarg(&argy)))
						{	casek->tm_mday=to32int(pomarg,context);
							if((pomarg=getarg(&argy)))
								casek->tm_hour=to32int(pomarg,context);
							if((pomarg=getarg(&argy)))
								casek->tm_min=to32int(pomarg,context);
							if((pomarg=getarg(&argy)))
								casek->tm_sec=to32int(pomarg,context);
						} else	
							js_error("Missing day!",context);
					}
				}
			} else{	cas=js_mem_alloc(sizeof(time_t));
				*cas=time(0);
				casek=localtime(cas);
				js_mem_free(cas);
				if(pomarg) delarg(pomarg,context);
			}
			/* Hopla mizero - a mame casek */
			BUILDFCE("parse",Cparse);
			BUILDFCE("setDate",CsetDate);
			pomvar->handler=casek->tm_mday;
			pompointer=&pomvar->handler;
			BUILDFCE("getDate",CgetDate);
			pomvar->handler=(long)pompointer;
			BUILDFCE("setHours",CsetHours);
			pomvar->handler=casek->tm_hour;
			pompointer=&pomvar->handler;
			BUILDFCE("getHours",CgetHours);
			pomvar->handler=(long)pompointer;
			BUILDFCE("setMinutes",CsetMinutes);
			pomvar->handler=casek->tm_min;
			pompointer=&pomvar->handler;
			BUILDFCE("getMinutes",CgetMinutes);
			pomvar->handler=(long)pompointer;
			BUILDFCE("setMonth",CsetMonth);
			pomvar->handler=casek->tm_mon;
			pompointer=&pomvar->handler;
			BUILDFCE("getMonth",CgetMonth);
			pomvar->handler=(long)pompointer;
			BUILDFCE("setSeconds",CsetSeconds);
			pomvar->handler=casek->tm_sec;
			pompointer=&pomvar->handler;
			BUILDFCE("getSeconds",CgetSeconds);
			pomvar->handler=(long)pompointer;
			BUILDFCE("setTime",CsetTime);
			pomvar->handler=casek->tm_sec;
			pompointer=&pomvar->handler;
			BUILDFCE("getTime",CgetTime);
			pomvar->handler=(long)pomns;
			BUILDFCE("setYear",CsetYear);
			pomvar->handler=casek->tm_year;
			pompointer=&(pomvar->handler);
			BUILDFCE("getYear",CgetYear);
			pomvar->handler=(long)pompointer;
			BUILDFCE("toGMTString",CtoGMTString);
			pomvar->handler=(long)pomns;
			BUILDFCE("toLocaleString",CtoLocaleString);
			pomvar->handler=(long)pomns;
			BUILDFCE("UTC",CUTC);
			pomvar->handler=(long)pomns;
			context->lnamespace=context->lnamespace->next;
			idebug("and exited!\n");
		break;
		case CString:
			idebug("CString called ");
			pomarg=getarg(&argy);
			rettype=STRING;
			if(pomarg)
				pomstr=tostring(pomarg,context);
			else {	pomstr=js_mem_alloc(1);
				pomstr[0]='\0';
			}
			retval=(long)pomstr;
			idebug("and exited!\n");
		break;
		case Calert:
			idebug("Calert called...\n");
			pomstr=tostring(force_getarg(&argy),context);
			call_alert(context,pomstr);
			js_durchfall=1;
			idebug("Calert exited!\n");
		break;
		case CjavaEnabled:
			idebug("javaEnabled is only fake!\n");
			rettype=BOOLEAN;
			retval=FALSE;
			idebug("CjavaEnabled called and exiting!\n");
		break;
		case Csubmitform:
			idebug("Csubmitform called ");
			rettype=UNDEFINED;
			retval=0;
			js_upcall_submit(context->ptr,variable->handler,variable->mid);
			idebug("and exited!\n");
		break;
		case Cresetform:
			idebug("Cresetform called ");
                        rettype=UNDEFINED;
                        retval=0;
                        js_upcall_reset(context->ptr,variable->handler,variable->mid);
                        idebug("and exited!\n");
		break;
		case Cclear:
			idebug("Cclear called ");
			js_mem_free(vysl);
			js_upcall_clear_window(context->ptr);
			js_durchfall=1;
			idebug("and exited!\n");
		break;
		case Cwclose:
			idebug("Cclose called...\n");
			call_close(context);
			js_durchfall=1;
			idebug("Cclose exited!\n");
		break;
		case Cwopen: /* To se te tyka!! B-) */
			idebug("Window open called... ");
			pomstr=tostring(force_getarg(&argy),context);
			call_open(context,pomstr,1);
			js_durchfall=1;
			idebug("and exited!\n");
		break;
		case Clocationreplace:
			idebug("Clocationreplace closed... ");
			pomstr=tostring(force_getarg(&argy),context);
			call_open(context,pomstr,0);
			js_durchfall=1;
			idebug("and exited!\n");
		break;
		case Cconfirm:  /* hotovo */
			idebug("Confirm called...\n");
			pomstr=tostring(force_getarg(&argy),context);
			call_confirm(context,pomstr);
			js_durchfall=1;
			idebug("Confirm exited!\n");
		break;
		case Cdclose:
			idebug("Document.close called... ");
			idebug("but not supported!!!\n");
		break;
		case Cdopen: /* /To se te tyka B;-) !! */
			idebug("Copen called ");
			idebug("but not written!!!\n");
		break;
		case Cwrite:
			idebug("Cwrite called...\n");
			pomstr=tostring(force_getarg(&argy),context);
			js_upcall_document_write(context->ptr,pomstr,strlen(pomstr));
			js_mem_free(pomstr);
			idebug("Cwrite exited!\n");
		break;
		case Cwriteln: 
			idebug("Cwriteln called ");
			pomstr=tostring(force_getarg(&argy),context);
			pomstr1=js_mem_alloc(strlen(pomstr)+2);
			strcpy(pomstr1,pomstr);
			pomstr1[strlen(pomstr)]='\13';
			pomstr1[strlen(pomstr)+1]='\0';
			js_upcall_document_write(context->ptr,pomstr1,strlen(pomstr1));
			js_mem_free(pomstr);
			js_mem_free(pomstr1);
			idebug("writeln is to debug!\n");
		break;
		case Cjoin:
			idebug("Cjoin called ");
			rettype=STRING;
			pomint=0;
			pomstr=js_mem_alloc(1);
			pomstr[0]='\0';
			pomarg=getarg(&argy);
			if(pomarg && (pomarg->typ!=UNDEFINED))
				pomstr2=tostring(pomarg,context);
			else {	if(pomarg)delarg(pomarg,context);
				pomstr2=js_mem_alloc(2);
				pomstr2[0]=',';
				pomstr2[1]='\0';
			}
			pomvar=lookup(js_lengthid,(plns*)variable->handler,context);
			if(pomvar->type!=INTVAR)my_internal("Internal:Strange type of \"length\" property!\n",context);
			pomint1=pomvar->handler;
			cislo=js_mem_alloc(DELKACISLA);
			while(pomint<pomint1)
			{	snprintf(cislo,DELKACISLA,"%d",(int)pomint++);
				pomvar=llookup(cislo,context->namespace,(plns*)variable->handler,context);
				pomarg=js_mem_alloc(sizeof(abuf));
				vartoarg(pomvar,pomarg,context);
				pomstr1=tostring(pomarg,context);
				retval=(long)js_mem_alloc(strlen(pomstr)+strlen(pomstr1)+strlen(pomstr2)+1);
				strcpy((char*)retval,pomstr);
				strcat((char*)retval,pomstr2);
				strcat((char*)retval,pomstr1);
				js_mem_free(pomstr);
				js_mem_free(pomstr1);
				pomstr=(char*)retval;
			}
			if(strlen(pomstr))
			{	retval=(long)js_mem_alloc(strlen(pomstr)+1);
				strcpy((char*)retval,(char*)pomstr+strlen(pomstr2));
			} else	retval=(long)stracpy1("");
			js_mem_free(pomstr);
			js_mem_free(pomstr2);
			js_mem_free(cislo);
			idebug("and exited!\n");
		break;
/*		case Cshit:
			idebug("Cshit called ");
			pomvar=lookup(js_lengthid,(plns*)variable->handler,context);
			if(pomvar->type!=INTVAR)internal("Internal: Strange type of \"length\" property!\n");
			if(pomvar->handler<=0)
				js_error("Shifting from empty array ",context);
			else{	cislo=js_mem_alloc(DELKACISLA);
				snprintf(cislo,DELKACISLA,"%d",(int)(--pomvar->handler));
				cislo[DELKACISLA-1]='\0';
				pomvar=llookup(cislo,context->namespace,(plns*)variable->handler,context);
				pomarg=js_mem_alloc(sizeof(abuf));
				pomarg->typ=VARIABLE;
				pomarg->argument=(long)pomvar;
				RESOLV(pomarg);
				clearvar(pomvar,context);
				rettype=pomarg->typ;
				retval=pomarg->argument;
				js_mem_free(pomarg);
				js_mem_free(cislo);
			}
			idebug("and exited!\n");
		break;
		case Cunshit:
			idebug("Cunshit called ");
			pomvar=lookup(js_lengthid,(plns*)variable->handler,context);
			if(pomvar->type!=INTVAR)internal("Internal: Strange type of \"length\" property!\n");
			cislo=js_mem_alloc(DELKACISLA);
			snprintf(cislo,DELKACISLA,"%d",(int)(pomvar->handler++));
			cislo[DELKACISLA-1]='\0';
			pomvar=llookup(cislo,context->namespace,(plns*)variable->handler,context);
			pomarg=getarg(&argy);
			if(!pomarg)
			{	js_error("Shifting nothing ",context);
				js_mem_free(cislo);
				return;
			}
			clearvar(pomvar,context);
			RESOLV(pomarg);
			switch((pomvar->type=pomarg->typ))
			{	case UNDEFINED:
					if(!options_get_bool("js_all_conversions"))
						js_error("You assigned UNDEFINED value!\n",context);
				case NULLOVY:
				case BOOLEAN:
				case INTEGER:
				case FUNKCE:
				case FLOAT:
				case STRING:
					pomvar->value=pomarg->argument;
				break;
				case ADDRSPACE:
					pomvar->type=ADDRSPACEP;
				case ARRAY:
				case ADDRSPACEP:
					pomvar->value=pomarg->argument;
					pomvar1=lookup(MIN1KEY,(plns*)pomarg->argument,context);
					if(pomvar1->type!=PARLIST)
					internal("Parent list corrupted!!!!\n");
					add_to_parlist(pomvar,pomvar1);
				break;
				default:
					my_internal("Internal: Unknown types assigned!\n",context);
				break;
			}
			js_mem_free(cislo);
			js_mem_free(pomarg);
			idebug("and exited!\n");
		break;*/
		case Cshift:
			pomvar=lookup(js_lengthid,(plns*)variable->handler,context);
			if(pomvar->type!=INTVAR)
				internal("Internal: Strange type of \"length\" property!\n");
			if(pomvar->handler<=0)
				js_error("Slicing from empty array ",context);
			else{	cislo=js_mem_alloc(DELKACISLA);
				cislo[DELKACISLA-1]='\0';
				snprintf(cislo,DELKACISLA-1,"%d",(int)(pomint=--pomvar->handler));
				pomvar=llookup(cislo,context->namespace,(plns*)variable->handler,context);
				pomvar2=js_mem_alloc(sizeof(lns));
				while(pomint--)
				{	snprintf(cislo,DELKACISLA-1,"%d",(int)pomint);
					pomvar1=llookup(cislo,context->namespace,(plns*)variable->handler,context);
					pomvar2->type=pomvar1->type;
					pomvar2->value=pomvar1->value;
					pomvar2->mid=pomvar1->mid;
					pomvar2->handler=pomvar1->handler;
					pomvar1->type=pomvar->type;
					pomvar1->value=pomvar->value;
					pomvar1->mid=pomvar->mid;
					pomvar1->handler=pomvar->handler;
					pomvar->type=pomvar2->type;
					pomvar->value=pomvar2->value;
					pomvar->mid=pomvar2->mid;
					pomvar->handler=pomvar2->handler;
					/* Mel jsem to delat takovym tim
					 * Krylovo trikem s dvojitym odecitanim
					 * :-) */
				}
				js_mem_free(pomvar2);
				/* Tady je potreba uprasknout pnou pomvar */
				pomarg=js_mem_alloc(sizeof(abuf));
				pomarg->typ=VARIABLE;
				pomarg->argument=(long)pomvar;
				RESOLV(pomarg);
				clearvar(pomvar,context);
				rettype=pomarg->typ;
				retval=pomarg->argument;
				js_mem_free(pomarg);
				js_mem_free(cislo);
			}
			idebug("and exited!\n");
		break;
		case Cunshift:
			idebug("Cunshit called ");
			pomvar=lookup(js_lengthid,(plns*)variable->handler,context);
			if(pomvar->type!=INTVAR)internal("Internal: Strange type of \"length\" property!\n");
			cislo=js_mem_alloc(DELKACISLA);
			cislo[DELKACISLA-1]='\0';
			snprintf(cislo,DELKACISLA-1,"%d",(int)(pomint=pomvar->handler++));
			pomvar=llookup(cislo,context->namespace,(plns*)variable->handler,context);
			pomarg=getarg(&argy);
			if(!pomarg)
			{	js_error("Shifting nothing ",context);
				js_mem_free(cislo);
				return; /* Tohle by mel byt internal */
			}
			clearvar(pomvar,context);
			/* Tady posuneme cele pole */
			while(pomint--)
			{	snprintf(cislo,DELKACISLA-1,"%ld",pomint);
				pomvar1=llookup(cislo,context->namespace,(plns*)variable->handler,context);
				pomvar->type=pomvar1->type;
				pomvar->value=pomvar1->value;
				pomvar->mid=pomvar1->mid;
				pomvar->handler=pomvar1->handler;
				pomvar=pomvar1;
			}/*posuneme o jedna */
			RESOLV(pomarg);
			switch((pomvar->type=pomarg->typ))
			{	case UNDEFINED:
					if(!options_get_bool("js_all_conversions"))
						js_error("You assigned UNDEFINED value!\n",context);
				case NULLOVY:
				case BOOLEAN:
				case INTEGER:
				case FUNKCE:
				case FLOAT:
				case STRING:
					pomvar->value=pomarg->argument;
				break;
				case ADDRSPACE:
					pomvar->type=ADDRSPACEP;
				case ARRAY:
				case ADDRSPACEP:
					pomvar->value=pomarg->argument;
					pomvar1=lookup(MIN1KEY,(plns*)pomarg->argument,context);
					if(pomvar1->type!=PARLIST)
					internal("Parent list corrupted!!!!\n");
					add_to_parlist(pomvar,pomvar1);
				break;
				default:
					internal("Internal: Unknown types assigned!\n");
				break;
			}
			js_mem_free(cislo);
			js_mem_free(pomarg);
			idebug("and exited!\n");
		break;
		case Cprompt:
			idebug("Cprompt called ");
			pomstr=tostring(force_getarg(&argy),context);
			pomstr1=tostring(force_getarg(&argy),context);
			if(!strcmp(pomstr1,"undefined"))strcpy(pomstr1,"");
			call_prompt(context,pomstr,pomstr1);
			js_durchfall=1;
			idebug("and exitted!\n");
		break;
		case CsetTimeout:
			idebug("CsetTimeout called ");
			pomstr=tostring(force_getarg(&argy),context);
			pomint=to32int(force_getarg(&argy),context);
			retval=(long)call_setTimeout(pomstr,pomint,context);
			rettype=STRING;
			idebug("and exitted...\n");
		break;
		case CclearTimeout:
			idebug("CclearTimeout called ");
			pomstr=tostring(force_getarg(&argy),context);
			if(strlen(pomstr)==14)
			{	pomint2=0;
				pomint2=pomstr[0]&0x1f;
				pomint2=(pomstr[1]&0x1f)+(pomint2<<5);
				pomint2=(pomstr[2]&0x1f)+(pomint2<<5);
				pomint2=(pomstr[3]&0x1f)+(pomint2<<5);
				pomint2=(pomstr[4]&0x1f)+(pomint2<<5);
				pomint2=(pomstr[5]&0x1f)+(pomint2<<5);
				pomint2=(pomstr[6]&0x1f)+(pomint2<<2);
				pomint2=(pomstr[7]&0x1f)+(pomint2<<2);
				pomint2=(pomstr[8]&0x1f)+(pomint2<<2);
				pomint2=(pomstr[9]&0x1f)+(pomint2<<2);
				pomint2=(pomstr[10]&0x1f)+(pomint2<<2);
				pomint2=(pomstr[11]&0x1f)+(pomint2<<2);
				pomint2=(pomstr[12]&0x1f)+(pomint2<<2);
				pomint2=(pomstr[13]&0x1f)+(pomint2<<2);
/*				printf("Killim timer c. %d ",(int)pomint2);*/
				pomint=0;
				while(pomint<TIMERNO)
				{	if(context->t[pomint]==pomint2)
					{	kill_timer(context->t[pomint]);
						context->t[pomint]=-1;
						idebug("Killim timer ");
						js_mem_free(context->bordely[pomint]);
						/* a je po timeru jako po vejprasku :-) */
					}
					pomint++;
				}
			}
			else idebug("Wrong name length!! ");
			js_mem_free(pomstr);	
			idebug("and exited\n");
		break;
		case Cclick:
			idebug("Cclick called ");
			js_upcall_click(context->ptr,variable->handler,variable->mid);
			idebug("and exited\n");
		break;
		case Creverse:
			idebug("Creverse called ");
			pomint=0;
/*			pomarg=getarg(&argy);
			if(pomarg && pomarg->typ!=UNDEFINED)
				pomstr=tostring(pomarg,context);
			else {	if(pomarg)delarg(pomarg,context);
				pomstr2=js_mem_alloc(2);
				pomstr2[0]=',';
				pomstr2[1]='\0';
			}*/
			pomvar=lookup(js_lengthid,(plns*)variable->handler,context);
			if(pomvar->type!=INTVAR)my_internal("Internal:Strange type of \"length\" property!\n",context);
			pomint1=pomvar->handler;
			cislo=js_mem_alloc(DELKACISLA);
			while(pomint<pomint1)
			{
				snprintf(cislo,DELKACISLA,"%d",(int)pomint++);
				pomvar=llookup(cislo,context->namespace,(plns*)variable->handler,context);
				snprintf(cislo,DELKACISLA,"%d",(int)pomint1--);
				pomvar1=llookup(cislo,context->namespace,(plns*)variable->handler,context);
				pomint2=pomvar->type;
				pomvar->type=pomvar1->type;
				pomvar1->type=pomint2;
				pomint2=pomvar->value;
				pomvar->value=pomvar1->value;
				pomvar1->value=pomint2;
				pomint2=pomvar->handler;
				pomvar->handler=pomvar1->handler;
				pomvar1->handler=pomint2;
			}
/*			js_mem_free(pomstr2);*/
			js_mem_free(cislo);
			rettype=UNDEFINED;
			retval=0;
			idebug("and exited!\n");
		break;
		case Csort:
			idebug("Csort called ");
			idebug("but not written!!!\n");
		break;
		case CIntMETFUN: /* but not written */
			idebug("CIntMETVAR called ");
			pomarg=(abuf*)variable->handler;
			RESOLV(pomarg);
			if(variable->identifier==CStoString)
			{	if(pomarg->typ!=INTEGER && pomarg->typ!=FLOAT && pomarg->typ!=BOOLEAN && pomarg->typ!=FUNKCE && pomarg->typ!=FUNKINT)
					if(!options_get_bool("js_all_conversions"))
						js_error("Calling toString of strange typed variable ",context);
				pomstr=tostring(pomarg,context);
				rettype=STRING;
				retval=(long)pomstr;
				
			}
			else

				
			if(variable->identifier==CSvalueOf)
			{	rettype=pomarg->typ;
				retval=pomarg->argument;
				js_mem_free(pomarg);
			} else
			
				
			if(variable->identifier==CSindexOf)
			{	
				pomstr1=tostring(force_getarg(&argy),context);
				pomstr=tostring(pomarg,context);
				if((pomarg=getarg(&argy)))
					pomint=to32int(pomarg,context);
				else	pomint=0;
				if((pomint>=strlen(pomstr))||(pomint<0))
				{	if(options_get_bool("js_all_conversions"))
					{	if(pomint<0)
							pomint=0;
						else
							pomint=strlen(pomstr)-1;
					} else	
					{	if(!options_get_bool("js_all_conversions"))
							js_error("Index out of range ",context);
						pomint=0;
					}
				}
				rettype=INTEGER;
				if(!pomstr||!pomstr1)
					retval=-1;
				else {	retval=(long)strstr(pomstr+pomint,pomstr1);
					if(retval)retval-=(long)pomstr;
					else	retval=-1;
				}
				if(pomstr)js_mem_free(pomstr);
				if(pomstr1)js_mem_free(pomstr1);
			} else

				
			if(variable->identifier==CSlastIndexOf)
			{	
				pomstr1=tostring(force_getarg(&argy),context);
				pomstr=tostring(pomarg,context);
				rettype=INTEGER;
				retval=0;
				pomint=(long)pomstr;
				if(pomstr && pomstr1)
				{	pomint1=((long)pomstr)+strlen(pomstr);
					while((pomint<pomint1)&&(pomint=(long)strstr((char*)pomint,pomstr1)))
						retval=(pomint++)-(long)pomstr;
				}
				if(pomstr)js_mem_free(pomstr);
				if(pomstr1)js_mem_free(pomstr1);
			} else

				
			if(variable->identifier==CSsubstring)
			{	
				pomstr1=tostring(pomarg,context);
				pomstr=js_mem_alloc(strlen(pomstr1)+1);
				pomarg=getarg(&argy);
				if(!pomarg) pomint=0;
				else {	pomint=to32int(pomarg,context);
					if((pomint>strlen(pomstr1))||(pomint<0))
					{	if(!options_get_bool("js_all_conversions"))
							js_error("Index out of range!",context);
						if(pomint<0)
							pomint=0;
						else	pomint=strlen(pomstr1);
					}
				}
				pomarg=getarg(&argy);
				if(!pomarg) pomint1=0;
				else {	pomint1=to32int(pomarg,context);
					if((pomint1>strlen(pomstr1))||(pomint<0))
					{	if(!options_get_bool("js_all_conversions"))
							js_error("Index out of range!",context);
						if(pomint1<0)
							pomint1=0;
						else	pomint1=strlen(pomstr1);
					}
				}
				if(pomint>pomint1)/* Elegantni prohazovani */
				{	pomint+=pomint1;
					pomint1=pomint-pomint1;
					pomint=pomint-pomint1;
				}/* Je pakarna, ze tu mam jenom dva pominty! */
				/* Pul roku v pakarne a rekli, ze je zdravej. Tejden na to zastrelil svyho dohlizitele */
				/* 								10 to midnight */
				strcpy(pomstr,pomstr1+pomint);
				pomstr[pomint1-pomint]='\0';
				js_mem_free(pomstr1);
				rettype=STRING;
				retval=(long)pomstr;
			} else
			
			if(variable->identifier==CSsubstr)
			{	pomstr1=tostring(pomarg,context);
				pomstr=js_mem_alloc(strlen(pomstr1)+1);
				pomarg=getarg(&argy);
				if(!pomarg) pomint=0;
				else{	pomint=to32int(pomarg,context);
					if(pomint>strlen(pomstr1))
						pomint=pomint%strlen(pomstr1);
					else while(pomint<0)
						pomint+=strlen(pomstr1);
				}
				pomarg=getarg(&argy);
				if(!pomarg) pomint1=strlen(pomstr1);
				else{	pomint1=to32int(pomarg,context);
					if(pomint1<=0)
					{	if(!options_get_bool("js_all_conversions"))
							js_error("Index out of range!",context); /* Uplne mimo - jako fyzici B-( */
						pomint1=0;
					}else{	if(pomint1>strlen(pomstr1))
						{	if(!options_get_bool("js_all_conversions"))
								js_error("Index out of range!",context);
							pomint1=strlen(pomstr1);
						}
					}
				}
				strcpy(pomstr,pomstr1+pomint);
				pomstr[pomint1]='\0';
				js_mem_free(pomstr1);
				rettype=STRING;
				retval=(long)pomstr;
			} else
			
			if(variable->identifier==CScharAt)
			{	
				pomstr1=tostring(pomarg,context);
				pomstr=js_mem_alloc(2);
				pomarg=getarg(&argy);
				pomstr[1]='\0';
				if(!pomarg)
					js_error("Argument required by method charAt ",context);
				else {	pomint=to32int(pomarg,context);
					if((pomint>=strlen(pomstr1))||(pomint<0))
					{	if(!options_get_bool("js_all_conversions"))
							js_error("Argument out of range ",context);
						pomint=0;
					}
					pomstr[0]=pomstr1[pomint];
				}
				js_mem_free(pomstr1);
				rettype=STRING;
				retval=(long)pomstr;
			} else

				
			if(variable->identifier==CStoLowerCase)
			{	pomstr=tostring(pomarg,context);
				rettype=STRING;
				retval=(long)js_mem_alloc(strlen(pomstr)+1);
				pomint=0;
				while((((char*)retval)[pomint]=tolower(pomstr[pomint])))pomint++;
				js_mem_free(pomstr);
			}else

				
			if(variable->identifier==CStoUpperCase)
			{	pomstr=tostring(pomarg,context);
				rettype=STRING;
				retval=(long)js_mem_alloc(strlen(pomstr)+1);
				pomint=0;
				while((((char*)retval)[pomint]=toupper(pomstr[pomint])))pomint++;
				js_mem_free(pomstr);
			} else

				
			if(variable->identifier==CSsplit)
			{	rettype=ARRAY;
				retval=(long)(pomns=js_mem_alloc(sizeof(plns)));
				pomns->next=context->lnamespace;
				pomns->handler=pomns->mid=0;
				pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
				pomint=0;
				while(pomint<HASHNUM)pomns->ns[pomint++]=0;
				context->lnamespace=pomns;
				pomvar=create(MIN1KEY,context->lnamespace,context);
				pomvar->type=PARLIST;
				pomvar->value=0; /* Bezprizorny namespace */
				pomint=0;
				pomstr2=pomstr=tostring(pomarg,context);
				pomstr1=tostring(force_getarg(&argy),context);
				pomint2=0;
				cislo=js_mem_alloc(DELKACISLA);
				if(pomstr1[0]=='\0')
				{	while(pomstr[0])
					{	snprintf(cislo,DELKACISLA,"%d",(int)(pomint2++));
						cislo[DELKACISLA-1]='\0';
						pomvar=buildin(cislo,context->namespace,context->lnamespace,context);
						pomvar->type=STRING;
						pomvar->value=(long)js_mem_alloc(2);/* Podle normy ma char velikost 1 */
						((char*)pomvar->value)[0]=pomstr[0];
						pomstr++;
						((char*)pomvar->value)[1]='\0';
					}
				}
				else
				{	while((pomint1=(long)strstr(pomstr,pomstr1)))
					{	snprintf(cislo,DELKACISLA,"%d",(int)(pomint2++));
						cislo[DELKACISLA-1]='\0';
						pomvar=buildin(cislo,context->namespace,context->lnamespace,context);
						pomvar->type=STRING;
						pomvar->value=(long)js_mem_alloc(pomint1-(long)pomstr+1);
						strncpy((char*)pomvar->value,pomstr,pomint1-(long)pomstr);
						((char*)pomvar->value)[pomint1-(long)pomstr]='\0';
						pomstr=strlen(pomstr1)+(char*)pomint1;/* Tohle by slo zoptimalizovat tak, ze a=strlen(pomstr1); */
						if(!*(pomstr-1))pomstr--;
					}
					snprintf(cislo,DELKACISLA,"%d",(int)(pomint2++));
					cislo[DELKACISLA-1]='\0';
					pomvar=buildin(cislo,context->namespace,context->lnamespace,context);
					pomvar->type=STRING;
					pomvar->value=(long)stracpy1(pomstr);
				}
				js_mem_free(pomstr2);
				js_mem_free(pomstr1);
				js_mem_free(cislo);
				BUILDVAR("length",Clength);
				js_lengthid=pomvar->identifier;
				pomvar->handler=pomint2; /* Nastavim inicialni delku */
				BUILDFCE("join",Cjoin);
				pomvar->handler=(long)context->lnamespace;
				BUILDFCE("reverse",Creverse);
				pomvar->handler=(long)context->lnamespace;
				BUILDFCE("sort",Csort);
				pomvar->handler=(long)context->lnamespace;
				BUILDFCE("toString",CtoString);
				pomvar->handler=C_OBJ_Objekt;
				context->lnamespace=context->lnamespace->next;
				idebug("and exited\n");	
			} else

			if(variable->identifier==CSparse)
			{	delarg(pomarg,context);
				if(!options_get_bool("js_all_conversions"))
					js_error("Neimplementovana featura! Nevim co to ma udelat\n",context);
			} else

			if(variable->identifier==CSUTC)
			{	delarg(pomarg,context);
				if(!options_get_bool("js_all_conversions"))
					js_error("Neimplementovana featura! Nevim co to ma udelat\n",context);
			} else

			my_internal("Blby operator vestavene metody!!\n",context);
			js_mem_free(variable);
			idebug("but not written!!!\n");
		break;
		case Cfocus:
			idebug("Cfocus called ");
			js_upcall_focus(context->ptr,variable->handler,variable->mid);
			idebug("but not written!!!\n");
		break;
		case Cblur:
			idebug("Cblur called ");
			js_upcall_blur(context->ptr,variable->handler,variable->mid);
			idebug("but not written!!!\n");
		break;
		case Cselect:
			idebug("Cselect called ");
			idebug("but not written!!!\n");
		break;
		case Cback:
			idebug("Cback called ");
			call_goto(context,"",-1);
			js_durchfall=1;
			idebug("and exited!\n");
		break;
		case Cforward:
			idebug("Cforward called ");
			call_goto(context,"",1);
			js_durchfall=1;
			idebug("and exited!\n");
		break;
		case Cgo:
			idebug("Cgo called ");
			pomstr=tostring(force_getarg(&argy),context);
			pomint=1;
			while(pomstr[pomint]&&((pomstr[pomint]>='0' && pomstr[pomint]<='9')||pomstr[pomint]=='-'))pomint++;
			if(pomstr[pomint])
				call_goto(context,pomstr,0);
			else
				call_goto(context,"",atol(pomstr)),js_mem_free(pomstr);
			js_durchfall=1;
			idebug("and exited!\n");
		break;
		case CtoString:
			rettype=STRING;
			switch(variable->handler)
			{	case C_OBJ_Objekt:
					retval=(long)js_mem_alloc(strlen("[object Object]")+1);
					strcpy((char*)retval,"[object Object]");
				break;
				case C_OBJ_document:
					retval=(long)js_mem_alloc(strlen("[object document]")+1);
					strcpy((char*)retval,"[object Document]");
				break;
				case C_OBJ_Math:
					retval=(long)js_mem_alloc(strlen("[object Math]")+1);
					strcpy((char*)retval,"[object Math]");
				break;
				case C_OBJ_window:
					retval=(long)js_mem_alloc(strlen("[object window]")+1);
					strcpy((char*)retval,"[object window]");
				break;
				case C_OBJ_location:
					pomstr=js_upcall_get_location(context->ptr);
					retval=(long)stracpy1(pomstr);
					if(pomstr)mem_free(pomstr);
				break;
				case C_OBJ_navigator:
					retval=(long)js_mem_alloc(strlen("[object navigator]")+1);
					strcpy((char*)retval,"[object navigator]");
				break;
				case C_OBJ_link:
				case C_OBJ_form:
				case C_OBJ_text:
				case C_OBJ_passwd:
				case C_OBJ_submit:
				case C_OBJ_reset:
				case C_OBJ_hidden:
				case C_OBJ_checkbox:
				case C_OBJ_radio:
				case C_OBJ_select:
				case C_OBJ_image:
					retval=(long)stracpy1("undefined");
				break;
				case C_OBJ_links:
				case C_OBJ_forms:
				case C_OBJ_anchors:
				case C_OBJ_elements:
				case C_OBJ_images:
					retval=(long)stracpy1("Objekt neceho");
					/* Asi */
				break;
				case C_OBJ_textarea:
				case C_OBJ_radioa:
					retval=(long)stracpy1("I don't know");
				break;
				default:
					internal("Strange type of object asked for toString method!\n");
				break;
			}
		break;
		case Cfloor:
			rettype=FLOAT;
			retval=(long)js_mem_alloc(sizeof(float));
			f1=tofloat(force_getarg(&argy),context);
			if(f1!=MY_NAN)
			{	
				if (f1 <= -MAXINT+1 || f1 >= MAXINT-1) *(float *)retval=f1;
				else {
					*(float*)retval=floor(f1);
					if((*(double*)retval)>MY_MAXDOUBLE)
						*(double*)retval=MY_INFINITY;
					if((*(double*)retval)<MY_MINDOUBLE)
						*(double*)retval=MY_MININFINITY;
					if(((float)(pomint=(int)rint(*(float*)retval)))==*(double*)retval)
					{	rettype=INTEGER;
						js_mem_free((float*)retval);
						retval=pomint;
						idebug("je to cele!\n");
					}
				}
			} else	*(double*)retval=MY_NAN;
		break;
		case Cmd5:
			rettype=STRING;
			pomstr=tostring(force_getarg(&argy),context);
			retval=(long)MD5Data(pomstr,strlen(pomstr),0);
			if(pomstr)js_mem_free(pomstr);
		break;
		case Ceval:
			idebug("Ceval is only fake!");
			rettype=STRING; /* 0; */
#ifdef PRO_DEBILY
			context->zlomeny_ramecek_jako_u_netchcipu=1;
#endif
			retval=(long)tostring(force_getarg(&argy),context); /*0; */
		break;
		case Cparse:
			idebug("Cparse called ");
			rettype=INTEGER;
			sezer_zizalu(pomstr=tostring(force_getarg(&argy),context),context);
			if(pomstr)js_mem_free(pomstr);
			retval=mktime(casek);
			idebug("and exited!\n");
		break;
		case CsetDate:
			idebug("CsetDate called ");
			if(!(pomarg=getarg(&argy)))
				js_error("setDate function requires argument!",context);
			else	variable->handler=to32int(pomarg,context);
				idebug("and exited!\n");
		break;
		case CgetDate:
			idebug("CgetDate called ");
			rettype=INTEGER;
			retval=*(long*)variable->handler;
			idebug("and exited!\n");
		break;
		case CsetHours:
			idebug("CsetHours called ");
			if(!(pomarg=getarg(&argy)))
				js_error("setHours function requires argument!",context);
			else	variable->handler=to32int(pomarg,context);
			idebug("and exited!\n");
		break;
		case CgetHours:
			idebug("CgetHours called ");
			rettype=INTEGER;
			retval=*(long*)variable->handler;
			idebug("and exited!\n");
		break;
		case CsetMinutes:
			idebug("CsetMinutes called ");
			if(!(pomarg=getarg(&argy)))
				js_error("setMinutes function requires argument!",context);
			else	variable->handler=to32int(pomarg,context);
			idebug("and exited!\n");
		break;
		case CgetMinutes:
			idebug("CgetMinutes called ");
			rettype=INTEGER;
			retval=*(long*)variable->handler;
			idebug("and exited!\n");
		break;
		case CsetMonth:
			idebug("CsetMonth called ");
			if(!(pomarg=getarg(&argy)))
				js_error("setMonth function requires argument!",context);
			else	variable->handler=to32int(pomarg,context);
			idebug("and exited!\n");
		break;
		case CgetMonth:
			idebug("CgetMonth called ");
			rettype=INTEGER;
			retval=*(long*)variable->handler;
			idebug("and exited!\n");
		break;
		case CsetSeconds:
			idebug("CsetSeconds called ");
			if(!(pomarg=getarg(&argy)))
				js_error("setSeconds function requires argument!",context);
			else	variable->handler=to32int(pomarg,context);
			idebug("and exited!\n");
		break;
		case CgetSeconds:
			idebug("CgetSeconds called ");
			rettype=INTEGER;
			retval=*(long*)variable->handler;
			idebug("and exited!\n");
		break;
		case CsetTime:
			idebug("CsetTime called ");
			if(!(pomarg=getarg(&argy)))
				js_error("setTime function requires argument!",context);
			else	variable->handler=to32int(pomarg,context);
			idebug("and exited!\n");
		break;
		case CgetTime:
			idebug("CgetTime called ");
			rettype=INTEGER;
			retval=*(long*)variable->handler;
			idebug("and exited!\n");
		break;
		case CsetYear:
			idebug("CsetYear called ");
			if(!(pomarg=getarg(&argy)))
				js_error("setYear function requires argument!",context);
			else	variable->handler=to32int(pomarg,context);
			idebug("and exited!\n");
		break;
		case CgetYear:
			idebug("CgetYear called ");
			rettype=INTEGER;
			retval=*(long*)variable->handler;
			idebug("and exited!\n");
		break;
		case CtoGMTString:
			idebug("CtoGMTString called ");
			pomint=vartoint(llookup("getTime",context->namespace,(plns*)variable->handler,context),context);
			{
				time_t t = (time_t)pomint;
				casek=gmtime(&t);
			}
			rettype=STRING;
			retval=(long)js_mem_alloc(DELKACASU);
			strftime((char*)retval,DELKACASU,"%D %T %Z",casek);
		break;
		case CtoLocaleString:
			idebug("CtoLocaleString called ");
			pomint=vartoint(llookup("getTime",context->namespace,(plns*)variable->handler,context),context);
			{
				time_t t = (time_t) pomint;
				casek=localtime(&t);
			}
			rettype=STRING;
			retval=(long)js_mem_alloc(DELKACASU);
			strftime((char*)retval,DELKACASU,"%D %T",casek);
			idebug("but not written\n");
		break;
		case CUTC:
			idebug("CUCT called ");
			idebug("but not written\n");
		break;
		default:
			zrusargy(argy,context);
			js_mem_free(vysl);
			internal("Internal: Strange internal function number!!\n");
		break;	
	}
	zrusargy(argy,context);
	vysl->typ=rettype;
	vysl->argument=retval;
	pusha(vysl,context);
	context->current=pullp(context); /* Takhle monstrosne zkonci upcall */
}

void get_var_value(lns*pna,long* typ, long*value,js_context*context)
{	long* policko;
	int pomint,i,j;
	char*pomstr;
	struct js_select_item * ale_te_lipy_se_nevzdam;
	plns*pomns;
	abuf*pomarg;
	lns*pomvar,*p1;
	switch(pna->value)
	{	case Cmatika:
			*typ=FLOAT;
			*value=(long)js_mem_alloc(sizeof(float));
			*(float*)*value=*(float*)pna->handler;
		break;
		case Cbarvicka:
			*typ=INTEGER;
			*value=pna->handler;
		break;
		case Clocation:
			*typ=ADDRSPACE;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomns->mid=pna->mid;
			pomns->handler=pna->handler;
			j=0;
			while(j<HASHNUM)pomns->ns[j++]=0;
			pomns->next=context->lnamespace;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0;
			j=context->lnamespace->mid;
			
			BUILDFCE("toString",CtoString);
			pomvar->mid=j;
			pomvar->handler=C_OBJ_location;
			
			BIVAR1("hash",Chash,j,0);
			BIVAR1("host",Chost,j,0);
			BIVAR1("hostname",Chostname,j,0);
			BIVAR1("href",Chref,j,0);
			BIVAR1("pathname",Cpathname,j,0);
			BIVAR1("port",Cport,j,0);
			BIVAR1("protocol",Cprotocol,j,0);
			BIVAR1("search",Csearch,j,0);
			BUILDFCE("replace",Clocationreplace);
			context->lnamespace=context->lnamespace->next;
		break;
		case Ctitle:
			*typ=STRING;
			pomstr=js_upcall_get_title(context->ptr);
			*value=(long)stracpy1(pomstr);
			if(pomstr)mem_free(pomstr);
		break;
		case ClastModified:
			*typ=STRING;
			pomstr=js_upcall_document_last_modified(context->ptr,pna->mid);
			*value=(long)stracpy1(pomstr);
			if(pomstr)mem_free(pomstr);
		break;
		case CuserAgent:
			*typ=STRING;
			pomstr=js_upcall_get_useragent(context->ptr);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
		break;
		case Creferer:
			*typ=STRING;
			pomstr=js_upcall_get_referrer(context->ptr);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
		break;
		case CappCodeName:
			*typ=STRING;
			pomstr=js_upcall_get_appcodename();
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
		break;
		case CappName:
			*typ=STRING;
			pomstr=js_upcall_get_appname();
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
		break;
		case CappVersion:
			*typ=STRING;
			pomstr=js_upcall_get_appversion();
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
		break;
		case CIntMETFUN:
			pomarg=(abuf*)pna->handler;
			RESOLV(pomarg);
			if(pna->identifier==CSlength)
			{	if(pomarg->typ!=STRING)
					if(!options_get_bool("js_all_conversions"))
						js_error("Length as internal is callable only by array or string ",context);
				pomstr=tostring(pomarg,context);
				*typ=INTEGER;
				*value=strlen(pomstr);
				js_mem_free(pomstr);
				js_mem_free(pna);
			}
			else 
			if(pna->identifier==CSMIN_VALUE)
			{	*typ=FLOAT;
				*value=(long)js_mem_alloc(sizeof(float));
				*(float*)(*value)=MY_MINDOUBLE;
				delarg(pomarg,context);
				js_mem_free(pna);
			} else
			if(pna->identifier==CSMAX_VALUE)
			{	*typ=FLOAT;
                               *value=(long)js_mem_alloc(sizeof(float));
                               *(float*)(*value)=MY_MAXDOUBLE;
                               delarg(pomarg,context);
                               js_mem_free(pna);
                       } else
                       if(pna->identifier==CSNaN)
                       {       *typ=FLOAT;
                               *value=(long)js_mem_alloc(sizeof(float));
                               *(float*)(*value)=MY_NAN; 
                               delarg(pomarg,context);
                               js_mem_free(pna);
                       } else  
                               internal("Strange internal property name!!\n");
                                        
                break;	
		case Chash:
			*typ=STRING;
			pomstr=js_upcall_get_location_hash(context->ptr);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
		break;
		case Chost:
			*typ=STRING;
			pomstr=js_upcall_get_location_host(context->ptr);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
		break;
		case Chostname:
			*typ=STRING;
			pomstr=js_upcall_get_location_hostname(context->ptr);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
		break;
		case Chref:
			*typ=STRING;
			pomstr=js_upcall_get_location(context->ptr);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
		break;
		case Cpathname:
			*typ=STRING;
			pomstr=js_upcall_get_location_pathname(context->ptr);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
		break;
		case Cport:
			*typ=STRING;
			pomstr=js_upcall_get_location_port(context->ptr);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
		break;
		case Cprotocol:
			*typ=STRING;
			pomstr=js_upcall_get_location_protocol(context->ptr);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
		break;
		case Csearch:
			*typ=STRING;
			pomstr=js_upcall_get_location_search(context->ptr);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
		break;
		case Cname:
			*typ=STRING;
			pomstr=js_upcall_get_window_name(context->ptr);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
		break;
		case Clinks:
			idebug("Clinks called ");
			*typ=ARRAY;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomstr=js_mem_alloc(DELKACISLA+1);
			pomns->next=context->lnamespace;
			pomns->handler=pna->handler;
			pomns->mid=0;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0; /* Bezprizorny namespace */
			i=0;
			pomint=0; /* Brain je ... */
			policko=js_upcall_get_links(context->ptr,pna->mid,&pomint);
			while(i<pomint)
			{	snprintf(pomstr,DELKACISLA,"%d",i);
				pomstr[DELKACISLA-1]='\0';
				BIVAR1(pomstr,Clinkptr,policko[i++],pna->handler);
			}
			js_mem_free(pomstr);
			if(policko)mem_free(policko);
			BUILDVAR("length",Clength);
			js_lengthid=pomvar->identifier;
			pomvar->handler=pomint; /* Nastavim inicialni delku */
			BUILDFCE("join",Cjoin);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("reverse",Creverse);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("sort",Csort);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("toString",CtoString);
			pomvar->handler=C_OBJ_links;
			context->lnamespace=context->lnamespace->next;
			idebug("and exited\n");
		break;
		case Cself:
			idebug("Cself called\n");
			*typ=ADDRSPACE;
			*value=(long)pna->index;
		break;

		case Ctop:
		
		case Cframeparent:
		case Cparent:
			idebug("Ctop called\n");
			*typ=ADDRSPACE;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomns->next=context->lnamespace;
			if(pna->value==Ctop)
				j=pomns->handler=pomns->mid=js_upcall_get_frame_top(context->ptr,pna->mid);
			else	j=pomns->handler=pomns->mid=js_upcall_get_parent(context->ptr,pna->mid);
			if(j==-1)
				j=pomns->handler=pomns->mid=pna->mid;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;

			buildin_document(context,j);
			BIVAR("location",Clocation,j,0);
			BIVAR("defaultStatus",CdefaultStatus,j,0);
			BIVAR("frames",Cframes,j,j);
			BIVAR("length",Cwlength,j,0);
			BIVAR("name",Cname,j,0);
			BIVAR("status",Cstatus,j,0);
			BIVAR("parent",Cparent,j,j);
			pna->index=(long)context->lnamespace;
/*			pna->type=ADDRSPACEP; */
			p1=create(MIN1KEY,context->lnamespace,context);
			p1->type=PARLIST;
			p1->value=0;
/*			add_to_parlist(pna,p1); */
			BIVAR("self",Cself,j,j);
			pna->index=(long)context->lnamespace;
/*			pna->type=ADDRSPACEP;
			add_to_parlist(pna,p1); */
			BIVAR("top",Ctop,j,j);
			BIVAR("window",Cself,j,j);
			pna->index=(long)context->lnamespace;
/*			pna->type=ADDRSPACEP;
			add_to_parlist(pna,p1);*/
			context->lnamespace=context->lnamespace->next;
			idebug("and exited\n");

		break;
		case Cframes:
		case Csubframes:
			idebug("Cframesy called ");
                        *typ=ARRAY;
                        *value=(long)(pomns=js_mem_alloc(sizeof(plns)));
                        pomstr=js_mem_alloc(DELKACISLA+1);
                        pomns->next=context->lnamespace;
                        pomns->mid=pna->mid;
			pomns->handler=pna->handler;
                        pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
                        pomint=0;
                        while(pomint<HASHNUM)pomns->ns[pomint++]=0;
                        context->lnamespace=pomns;
                        pomvar=create(MIN1KEY,context->lnamespace,context);
                        pomvar->type=PARLIST;
                        pomvar->value=0; /* Bezprizorny namespace */
                        i=0;
                        pomint=0; /* Brain je ... */
                        policko=js_upcall_get_subframes(context->ptr,pna->mid,&pomint);
                        if(policko)
				while(i<pomint)
	                        {       snprintf(pomstr,DELKACISLA,"%d",i);
        	                        pomstr[DELKACISLA-1]='\0';
					BIVAR1(pomstr,Cframeptr,policko[i],policko[i]);
					i++;
                        	}
                        js_mem_free(pomstr);
                        if(policko)mem_free(policko);
                        BUILDVAR("length",Clength);
                        js_lengthid=pomvar->identifier;
                        pomvar->handler=pomint; /* Nastavim inicialni delku */
                        BUILDFCE("join",Cjoin);
                        pomvar->handler=(long)context->lnamespace;
                        BUILDFCE("reverse",Creverse);
                        pomvar->handler=(long)context->lnamespace;
                        BUILDFCE("sort",Csort);
                        pomvar->handler=(long)context->lnamespace;
                        BUILDFCE("toString",CtoString);
                        pomvar->handler=C_OBJ_frames;
                        context->lnamespace=context->lnamespace->next;
                        idebug("and exited\n");
		break;
		case Clength:
                       idebug("Clength called ");
                       *typ=INTEGER;
                       *value=pna->handler;
                       idebug("and exited\n");
               break;
		case Cforms:
			idebug("Cforms called\n");
			*typ=ARRAY;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomstr=js_mem_alloc(DELKACISLA+1);
			pomns->next=context->lnamespace;
			pomns->mid=pna->mid;
			pomns->handler=pna->handler;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0; /* Bezprizorny namespace */
			i=0;
			pomint=0;
			policko=js_upcall_get_forms(context->ptr,pna->mid,&pomint);
			while(i<pomint)
			{	snprintf(pomstr,DELKACISLA,"%d",i);
				pomstr[DELKACISLA-1]='\0';
				BIVAR1(pomstr,Cformptr,policko[i++],pna->mid);
			}
			js_mem_free(pomstr);
			if(policko)mem_free(policko);
			BUILDVAR("length",Clength);
			js_lengthid=pomvar->identifier;
			pomvar->handler=pomint; /* Nastavim inicialni delku */
			BUILDFCE("join",Cjoin);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("reverse",Creverse);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("sort",Csort);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("toString",CtoString);
			pomvar->handler=C_OBJ_forms;
			context->lnamespace=context->lnamespace->next;
			idebug("Cforms exited!\n");
		break;
		case Canchors:
			idebug("Canchors called\n");
			*typ=ARRAY;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomstr=js_mem_alloc(DELKACISLA+1);
			pomns->next=context->lnamespace;
			pomns->mid=pna->mid;
			pomns->handler=pna->handler;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0; /* Bezprizorny namespace */
			i=0;
			pomint=0;
			policko=js_upcall_get_anchors(context->ptr,pna->mid,&pomint);
			while(i<pomint)
			{	snprintf(pomstr,DELKACISLA,"%d",i);
				pomstr[DELKACISLA-1]='\0';
				BIVAR1(pomstr,Canchorptr,policko[i++],pna->handler);
			}
			js_mem_free(pomstr);
			if(policko)mem_free(policko);
			BUILDVAR("length",Clength);
			js_lengthid=pomvar->identifier;
			pomvar->handler=pomint; /* Nastavim inicialni delku */
			BUILDFCE("join",Cjoin);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("reverse",Creverse);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("sort",Csort);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("toString",CtoString);
			pomvar->handler=C_OBJ_anchors;
			context->lnamespace=context->lnamespace->next;
			idebug("Canchors exited!\n");
		break;
		case Clinkptr:
			idebug("Clinkptr called");
			*typ=ADDRSPACE;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomns->next=context->lnamespace;
			pomns->handler=pomns->mid=0;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0; /* Bezprizorny namespace */
			BIVAR1("target",Ctarget,pna->mid,pna->handler);
			BUILDFCE("toString",CtoString);
			pomvar->handler=C_OBJ_form;

			context->lnamespace=context->lnamespace->next;
			idebug("and exited\n");
		break;
		case Cformptr:
			idebug("Cformptr called ");
			*typ=ADDRSPACE;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomns->next=context->lnamespace;
			pomns->mid=pna->mid; /* Nevim ale snad to bude fungovat */
			pomns->handler=pna->handler;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0; /* Bezprizorny namespace */
			BIVAR1("action",Caction,pna->mid,pna->handler);
			BIVAR1("elements",Celements,pna->mid,pna->handler);
			BIVAR1("encoding",Cencoding,pna->mid,pna->handler);
			BIVAR1("method",Cmethod,pna->mid,pna->handler);
			BIVAR1("target",Cftarget,pna->mid,pna->handler);
			BUILDSFCE("submit",Csubmitform,pna->mid);
			pomvar->handler=pna->handler;
			BUILDSFCE("reset",Cresetform,pna->mid);
			pomvar->handler=pna->handler;
			context->lnamespace=context->lnamespace->next;

			idebug("and exited\n");
		break;
		case Ctextptr:
			idebug("Ctextptr called ");
			*typ=ADDRSPACE;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomns->next=context->lnamespace;
			pomns->mid=0;
			pomns->handler=0;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0;
			BIVAR1("defaultValue",CdefaultValue,pna->mid,pna->handler);
			BIVAR1("name",Cfename,pna->mid,pna->handler);
			BIVAR1("value",Cvalue,pna->mid,pna->handler);
			BUILDSFCE("focus",Cfocus,pna->mid);
			pomvar->handler=pna->handler;
			BUILDSFCE("blur",Cblur,pna->mid);
			pomvar->handler=pna->handler;
			BUILDSFCE("select",Cselect,pna->mid);
			pomvar->handler=pna->handler;
			BUILDFCE("toString",CtoString);
			pomvar->handler=C_OBJ_text;
			context->lnamespace=context->lnamespace->next;
			idebug("and exited\n");
		break;
		case Cpasswdptr:
			idebug("Cpasswdptr called ");
			*typ=ADDRSPACE;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomns->next=context->lnamespace;
			pomns->mid=0;
			pomns->handler=0;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0;
			BIVAR1("defaultValue",CdefaultValue,pna->mid,pna->handler);
			BIVAR1("name",Cfename,pna->mid,pna->handler);
			BIVAR1("value",Cvalue,pna->mid,pna->handler);
			BUILDSFCE("focus",Cfocus,pna->mid);
			pomvar->handler=pna->handler;
			BUILDSFCE("blur",Cblur,pna->mid);
			pomvar->handler=pna->handler;
			BUILDSFCE("select",Cselect,pna->mid);
			pomvar->handler=pna->handler;
			BUILDFCE("toString",CtoString);
			pomvar->handler=C_OBJ_passwd;
			context->lnamespace=context->lnamespace->next;
			idebug("and exited\n");
		break;
		case Ctextarptr:
			idebug("Ctextarptr called ");
			*typ=ADDRSPACE;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomns->next=context->lnamespace;
			pomns->mid=0;
			pomns->handler=0;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0;
			BIVAR1("defaultValue",CdefaultValue,pna->mid,pna->handler);
			BIVAR1("name",Cfename,pna->mid,pna->handler);
			BIVAR1("value",Cvalue,pna->mid,pna->handler);
			BUILDSFCE("focus",Cfocus,pna->mid);
			pomvar->handler=pna->handler;
			BUILDSFCE("blur",Cblur,pna->mid);
			pomvar->handler=pna->handler;
			BUILDSFCE("select",Cselect,pna->mid);
			pomvar->handler=pna->handler;
			BUILDFCE("toString",CtoString);
			pomvar->handler=C_OBJ_textarea;

			context->lnamespace=context->lnamespace->next;
			idebug("and exited\n");
		break;
		case Csubmitptr:
			idebug("Csubmitptr called ");
			*typ=ADDRSPACE;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomns->next=context->lnamespace;
			pomns->mid=0;
			pomns->handler=0;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0;
			BIVAR1("name",Cfename,pna->mid,pna->handler);
			BIVAR1("value",CdefaultValue,pna->mid,pna->handler);
			BUILDSFCE("click",Cclick,pna->mid);
			pomvar->handler=pna->handler;
			BUILDFCE("toString",CtoString);
			pomvar->handler=C_OBJ_submit;

			context->lnamespace=context->lnamespace->next;
			idebug("and exited\n");
		break;
		case Cresetptr:
			idebug("Cresetptr called ");
			*typ=ADDRSPACE;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomns->next=context->lnamespace;
			pomns->mid=0;
			pomns->handler=0;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0;
			BIVAR1("name",Cfename,pna->mid,pna->handler);
			BIVAR1("value",CdefaultValue,pna->mid,pna->handler);
			BUILDSFCE("click",Cclick,pna->mid);
			pomvar->handler=pna->handler;
			BUILDFCE("toString",CtoString);
			pomvar->handler=C_OBJ_reset;

			context->lnamespace=context->lnamespace->next;
			idebug("and exited\n");
		break;
		case Chiddenptr:
			idebug("Chiddenptr called ");
			*typ=ADDRSPACE;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomns->next=context->lnamespace;
			pomns->mid=0;
			pomns->handler=0;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0;
			BIVAR1("name",Cfename,pna->mid,pna->handler);
			BIVAR1("value",CdefaultValue,pna->mid,pna->handler);
			BUILDFCE("toString",CtoString);
			pomvar->handler=C_OBJ_hidden;

			context->lnamespace=context->lnamespace->next;
			idebug("and exited\n");
		break;
		case Cchkboxptr:
			idebug("Cchkboxptr called ");
			*typ=ADDRSPACE;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomns->next=context->lnamespace;
			pomns->mid=0;
			pomns->handler=0;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0;
			BIVAR1("checked",Cchecked,pna->mid,pna->handler);
			BIVAR1("defaultChecked",CdefaultChecked,pna->mid,pna->handler);
			BIVAR1("name",Cfename,pna->mid,pna->handler);
			BIVAR1("value",CdefaultValue,pna->mid,pna->handler);
			BUILDSFCE("click",Cclick,pna->mid);
			pomvar->handler=pna->handler;
			BUILDFCE("toString",CtoString);
			pomvar->handler=C_OBJ_checkbox;

			context->lnamespace=context->lnamespace->next;


			idebug("and exited\n");
		break;
		case Cradioptr:
			idebug("Cradioptr called ");
			*typ=ADDRSPACE;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomns->next=context->lnamespace;
			pomns->mid=0;
			pomns->handler=0;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0;
			BIVAR1("checked",Cchecked,pna->mid,pna->handler);
			BIVAR1("defaultChecked",CdefaultChecked,pna->mid,pna->handler);
			BIVAR1("length",Cradiolength,pna->mid,pna->handler);
			BIVAR1("name",Cfename,pna->mid,pna->handler);
			BIVAR1("value",CdefaultValue,pna->mid,pna->handler);
			BUILDSFCE("click",Cclick,pna->mid);
			pomvar->handler=pna->handler;
			BUILDFCE("toString",CtoString);
			pomvar->handler=C_OBJ_radio;

			context->lnamespace=context->lnamespace->next;
			idebug("and exited\n");
		
		break;
		case Cselectptr:
			idebug("Cselectptr called ");
			*typ=ADDRSPACE;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomns->next=context->lnamespace;
			pomns->mid=0;
			pomns->handler=0;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0;
			BIVAR1("length",Cselectlength,pna->mid,pna->handler);
			BIVAR1("name",Cselectname,pna->mid,pna->handler);
			BIVAR1("options",Cselectoptions,pna->mid,pna->handler);
			BIVAR1("index",Cselectindex,pna->mid,pna->handler);
/*			BIVAR1("selected",Cselectselected,pna->mid,pna->handler);*/
			BIVAR1("selectedIndex",CselectselectedIndex,pna->mid,pna->handler);
/*			BIVAR1("text",Cselecttext,pna->mid,pna->handler);
			BIVAR1("value",Cselectvalue,pna->mid,pna->handler);*/
			BUILDFCE("toString",CtoString);
			pomvar->handler=C_OBJ_select;

			context->lnamespace=context->lnamespace->next;
		break;
		case Cselectname:
			idebug("Cselectname called ");
			*typ=STRING;
			pomstr=js_upcall_get_form_element_name(context->ptr,pna->handler,pna->mid);
			*value=(long)stracpy1(pomstr);
			mem_free(pomstr);
			idebug("and exited!\n");
		break;
		case Cselectoptions:
			 idebug("Cselectoptions called\n");
                        *typ=ARRAY;
                        *value=(long)(pomns=js_mem_alloc(sizeof(plns)));
                        pomstr=js_mem_alloc(DELKACISLA+1);
                        pomns->next=context->lnamespace;
                        pomns->mid=pna->mid;
                        pomns->handler=pna->handler;
                        pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
                        pomint=0;
                        while(pomint<HASHNUM)pomns->ns[pomint++]=0;
                        context->lnamespace=pomns;
                        pomvar=create(MIN1KEY,context->lnamespace,context);
                        pomvar->type=PARLIST;
                        pomvar->value=0; /* Bezprizorny namespace */
			ale_te_lipy_se_nevzdam=js_upcall_get_select_options(context->ptr,pna->handler,pna->mid,&pomint);
			i=pomint;
			while(pomint--){
				if(ale_te_lipy_se_nevzdam[pomint].text)mem_free(ale_te_lipy_se_nevzdam[pomint].text);
				if(ale_te_lipy_se_nevzdam[pomint].value)mem_free(ale_te_lipy_se_nevzdam[pomint].value);
			}
			if(ale_te_lipy_se_nevzdam) /* Treba ten dul neprodate... Treba vam ho vezmou */
				mem_free(ale_te_lipy_se_nevzdam); /* Mlady pane, videl jste nekdy dul? */
                        pomint=i;
			i=0;
                        while(i<pomint) /* ... to jsou chodby sem tam... */
                        {       snprintf(pomstr,DELKACISLA,"%d",i);
                                pomstr[DELKACISLA-1]='\0';
                                BIVAR1(pomstr,Cselectmrcha,pna->mid,pna->handler);
				pomvar->index=i++;
                        }
                        js_mem_free(pomstr); /* Penezenku vam muzou vzit B;-) */
                        BUILDVAR("length",Clength);
                        js_lengthid=pomvar->identifier;
                        pomvar->handler=pomint; /* Nastavim inicialni delku */
                        BUILDFCE("join",Cjoin);
                        pomvar->handler=(long)context->lnamespace;
                        BUILDFCE("reverse",Creverse);
                        pomvar->handler=(long)context->lnamespace;
                        BUILDFCE("sort",Csort);
                        pomvar->handler=(long)context->lnamespace;
                        BUILDFCE("toString",CtoString);
                        pomvar->handler=C_OBJ_forms;
                        context->lnamespace=context->lnamespace->next;
                        idebug("Cselectoptions exited!\n");
		break;
		case Cselectmrcha:
			idebug("Cselectmrcha called...\n");
                        *typ=ADDRSPACE;
                        *value=(long)(pomns=js_mem_alloc(sizeof(plns))); /* Dul se muze zaplavit, zasypat, ale nikdo vam ho nemuze vzit. B;-) */
                        pomns->next=context->lnamespace;
                        pomns->mid=0;
                        pomns->handler=0;
                        pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
                        pomint=0;
                        while(pomint<HASHNUM)pomns->ns[pomint++]=0;
                        context->lnamespace=pomns;
                        pomvar=create(MIN1KEY,context->lnamespace,context);
                        pomvar->type=PARLIST;
                        pomvar->value=0;
                        BIVAR1("defaultSelected",CselectmrchadefaultSelected,pna->mid,pna->handler);
			pomvar->index=pna->index;
			BIVAR1("selected",Cselectmrchaselected,pna->mid,pna->handler);
			pomvar->index=pna->index;
			BIVAR1("text",Cselectmrchatext,pna->mid,pna->handler);
			pomvar->index=pna->index;
			BIVAR1("value",Cselectmrchavalue,pna->mid,pna->handler);
			pomvar->index=pna->index;
			
                        BUILDFCE("toString",CtoString);
                        pomvar->handler=C_OBJ_select;
                        context->lnamespace=context->lnamespace->next;
			idebug("Cselectmrcha exited!\n");
		break;
		case CselectmrchadefaultSelected:
			idebug("CselectdefaultSelected called ");
			*typ=BOOLEAN;
			ale_te_lipy_se_nevzdam=js_upcall_get_select_options(context->ptr,pna->handler,pna->mid,&pomint);
			*value=ale_te_lipy_se_nevzdam[pna->index].default_selected;
			while(pomint--){
				if(ale_te_lipy_se_nevzdam[pomint].text)mem_free(ale_te_lipy_se_nevzdam[pomint].text);
				if(ale_te_lipy_se_nevzdam[pomint].value)mem_free(ale_te_lipy_se_nevzdam[pomint].value);
			}
			if(ale_te_lipy_se_nevzdam)
				mem_free(ale_te_lipy_se_nevzdam);
			idebug("and exited!\n");
		break;
		case Cselectmrchaselected:
			idebug("Cselectselected called ");
			*typ=BOOLEAN; /* Lepsi by bylo boobean na pocest pana GNU - be-e-e-e! */
			ale_te_lipy_se_nevzdam=js_upcall_get_select_options(context->ptr,pna->handler,pna->mid,&pomint);
			*value=ale_te_lipy_se_nevzdam[pna->index].selected;
			while(pomint--){
				if(ale_te_lipy_se_nevzdam[pomint].text)mem_free(ale_te_lipy_se_nevzdam[pomint].text);
				if(ale_te_lipy_se_nevzdam[pomint].value)mem_free(ale_te_lipy_se_nevzdam[pomint].value);
			}
			if(ale_te_lipy_se_nevzdam)
				mem_free(ale_te_lipy_se_nevzdam);
			idebug("and exited!\n");
		break;
		case CselectselectedIndex:
			idebug("Cselectindex called ");
			*typ=INTEGER;
			*value=js_upcall_get_select_index(context->ptr,pna->handler,pna->mid);
			if(*value==-1) js_error("Invalid select ",context);
			idebug("and exited\n");
		break;
		case Cselectmrchatext:
			idebug("Cselecttext called ");
			*typ=STRING; /* Lepsi by bylo boobean na pocest pana GNU - be-e-e-e! */
			ale_te_lipy_se_nevzdam=js_upcall_get_select_options(context->ptr,pna->handler,pna->mid,&pomint);
			*value=(long)stracpy1(ale_te_lipy_se_nevzdam[pna->index].text);
			while(pomint--){
				if(ale_te_lipy_se_nevzdam[pomint].text)mem_free(ale_te_lipy_se_nevzdam[pomint].text);
				if(ale_te_lipy_se_nevzdam[pomint].value)mem_free(ale_te_lipy_se_nevzdam[pomint].value);
			}
			if(ale_te_lipy_se_nevzdam)
				mem_free(ale_te_lipy_se_nevzdam);
			idebug("and exited!\n");
		break;
		case Cselectmrchavalue:
			idebug("Cselectvalue called ");
			*typ=STRING; /* Lepsi by bylo boobean na pocest pana GNU - be-e-e-e! */
			ale_te_lipy_se_nevzdam=js_upcall_get_select_options(context->ptr,pna->handler,pna->mid,&pomint);
			*value=(long)stracpy1(ale_te_lipy_se_nevzdam[pna->index].value);
			while(pomint--){
				if(ale_te_lipy_se_nevzdam[pomint].text)mem_free(ale_te_lipy_se_nevzdam[pomint].text);
				if(ale_te_lipy_se_nevzdam[pomint].value)mem_free(ale_te_lipy_se_nevzdam[pomint].value);
			}
			if(ale_te_lipy_se_nevzdam)
				mem_free(ale_te_lipy_se_nevzdam);
			idebug("and exited!\n");
		break;
/*		case Cselectindex:
			idebug("Cselectindex called ");
			*typ=INTEGER;
			*value=js_upcall_get_select_index(context->ptr,pna->handler,pna->mid);
			if(*value==-1) js_error("Invalid select ",context);
			idebug("and exited\n");
		break;*/
		case Cselectlength:
                        idebug("Cselectlength called ");
                        *typ=INTEGER;
                        *value=js_upcall_get_select_length(context->ptr,pna->handler,pna->mid);
                        if(*value==-1) js_error("Invalid select ",context);
                        idebug("and exited\n");
                break;
		case Cradiolength:
			idebug("Cradiolength called ");
			*typ=INTEGER;
			*value=js_upcall_get_radio_length(context->ptr,pna->handler,pna->mid);
			if(*value==-1) js_error("Invalid radio ",context);
			idebug("and exited\n");
		break;
		case Cchecked:
			idebug("Cchecked called ");
			*typ=BOOLEAN;
			*value=js_upcall_get_checkbox_radio_checked(context->ptr,pna->handler,pna->mid);
			idebug("and exited\n");
		break;
		case CdefaultChecked:
			idebug("CdefaultChecked called ");
			*typ=BOOLEAN;
			*value=js_upcall_get_checkbox_radio_default_checked(context->ptr,pna->handler,pna->mid);
			idebug("and exited\n");
		break;
		case Cfename:
			idebug("Cfename called ");
			*typ=STRING;
			pomstr=js_upcall_get_form_element_name(context->ptr,pna->handler,pna->mid);
			*value=(long)stracpy1(pomstr);
			if(pomstr)mem_free(pomstr);
			idebug("and exited\n");
		break;
		case Cvalue:
			idebug("Cvalue called ");
			*typ=STRING;
			pomstr=js_upcall_get_form_element_value(context->ptr,pna->handler,pna->mid);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
			idebug("and exited\n");
					
		break;
		case CdefaultValue:
			idebug("CdefaultValue called ");
			*typ=STRING;
			pomstr=js_upcall_get_form_element_default_value(context->ptr,pna->handler,pna->mid);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
			idebug("and exited\n");
		break;
		case Ctarget:
			idebug("Ctarget called\n");
			*typ=STRING;
			pomstr=js_upcall_get_link_target(context->ptr,pna->handler,pna->mid);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
			idebug("Ctarget exited\n");
		break;
		case Caction:
			idebug("Caction called ");
			*typ=STRING;
			pomstr=js_upcall_get_form_action(context->ptr,pna->handler,pna->mid);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
			idebug("and exited\n");
		break;
		case Celements:
			idebug("Celements called ");
			*typ=ARRAY;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomstr=js_mem_alloc(DELKACISLA+1);
			pomns->next=context->lnamespace;
			pomns->mid=0;
			pomns->handler=0;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0; /* Bezprizorny namespace */
			i=0;
			pomint=0;
			policko=js_upcall_get_form_elements(context->ptr,pna->handler,pna->mid,&pomint);
			while(i<pomint)
			{	snprintf(pomstr,DELKACISLA,"%d",i);
				pomstr[DELKACISLA-1]='\0';
				switch(jsint_object_type(policko[i]))
				{	case JS_OBJ_T_TEXT:
						BIVAR1(pomstr,Ctextptr,policko[i],pna->handler);
					break;
					case JS_OBJ_T_PASSWORD:
						BIVAR1(pomstr,Cpasswdptr,policko[i],pna->handler);
					break;
					case JS_OBJ_T_TEXTAREA:
						BIVAR1(pomstr,Ctextarptr,policko[i],pna->handler);
					break;
					case JS_OBJ_T_CHECKBOX:
						BIVAR1(pomstr,Cchkboxptr,policko[i],pna->handler);
					break;
					case JS_OBJ_T_RADIO:
						BIVAR1(pomstr,Cradioptr,policko[i],pna->handler);
					break;
					case JS_OBJ_T_SELECT:
						BIVAR1(pomstr,Cselectptr,policko[i],pna->handler);
					break;
					case JS_OBJ_T_SUBMIT:
						BIVAR1(pomstr,Csubmitptr,policko[i],pna->handler);
					break;
					case JS_OBJ_T_RESET:
						BIVAR1(pomstr,Cresetptr,policko[i],pna->handler);
					break;
					case JS_OBJ_T_HIDDEN:
						BIVAR1(pomstr,Chiddenptr,policko[i],pna->handler);
					break;
					case JS_OBJ_T_BUTTON:
						BIVAR1(pomstr,Cbuttonptr,policko[i],pna->handler);
					break;
					case JS_OBJ_T_FRAME:
						BIVAR1(pomstr,Cframeptr,policko[i],policko[i]);
					break;
					case JS_OBJ_T_IMAGE:
						BIVAR1(pomstr,Cimageptr,policko[i],pna->handler);
					break;
					default: my_internal("GNU! To je typek!\n",context);
					break;
				}
				i++;
			}
			js_mem_free(pomstr);
			if(policko)mem_free(policko);
			BUILDVAR("length",Clength);
			js_lengthid=pomvar->identifier;
			pomvar->handler=pomint; /* Nastavim inicialni delku */
			BUILDFCE("join",Cjoin);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("reverse",Creverse);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("sort",Csort);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("toString",CtoString);
			pomvar->handler=C_OBJ_elements;

			context->lnamespace=context->lnamespace->next;
			idebug("Celements exited!\n");
			
		break;
		case Call:
#if 0
			*typ=UNDEFINED;
			*value=0;
#endif		
#if 1
			*typ=ARRAY;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomstr=js_mem_alloc(DELKACISLA+1);
			pomns->next=context->lnamespace;
			pomns->mid=0;
			pomns->handler=0;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0; /* Bezprizorny namespace */
			i=0;
			pomint=0;
			policko=js_upcall_get_all(context->ptr,pna->handler,&pomint);
			while(i<pomint)
			{	snprintf(pomstr,DELKACISLA,"%d",i);
				pomstr[DELKACISLA-1]='\0';
				switch(jsint_object_type(policko[i+1]))
				{	case JS_OBJ_T_LINK:
						BIVAR1(pomstr,Clinkptr,policko[i+1],policko[i]);
						CBIVAR1((char*)(policko[i+2]),Clinkptr,policko[i+1],policko[i]);
					break;
					case JS_OBJ_T_FORM:
						BIVAR1(pomstr,Cformptr,policko[i+1],policko[i]);
						CBIVAR1((char*)(policko[i+2]),Cformptr,policko[i+1],policko[i]);
					break;
					case JS_OBJ_T_ANCHOR:
						BIVAR1(pomstr,Canchorptr,policko[i+1],policko[i]);
						CBIVAR1((char*)(policko[i+2]),Canchorptr,policko[i+1],policko[i]);
					break;
					case JS_OBJ_T_TEXT:
						BIVAR1(pomstr,Ctextptr,policko[i+1],policko[i]);
						CBIVAR1((char*)(policko[i+2]),Ctextptr,policko[i+1],policko[i]);
					break;
					case JS_OBJ_T_PASSWORD:
						BIVAR1(pomstr,Cpasswdptr,policko[i+1],policko[i]);
						CBIVAR1((char*)(policko[i+2]),Cpasswdptr,policko[i+1],policko[i]);
					break;
					case JS_OBJ_T_TEXTAREA:
						BIVAR1(pomstr,Ctextarptr,policko[i+1],policko[i]);
						CBIVAR1((char*)(policko[i+2]),Ctextarptr,policko[i+1],policko[i]);
					break;
					case JS_OBJ_T_CHECKBOX:
						BIVAR1(pomstr,Cchkboxptr,policko[i+1],policko[i]);
						CBIVAR1((char*)(policko[i+2]),Cchkboxptr,policko[i+1],policko[i]);
					break;
					case JS_OBJ_T_RADIO:
						BIVAR1(pomstr,Cradioptr,policko[i+1],policko[i]);
						CBIVAR1((char*)(policko[i+2]),Cradioptr,policko[i+1],policko[i]);
					break;
					case JS_OBJ_T_SELECT:
						BIVAR1(pomstr,Cselectptr,policko[i+1],policko[i]);
						CBIVAR1((char*)(policko[i+2]),Cselectptr,policko[i+1],policko[i]);
					break;
					case JS_OBJ_T_SUBMIT:
						BIVAR1(pomstr,Csubmitptr,policko[i+1],policko[i]);
						CBIVAR1((char*)(policko[i+2]),Csubmitptr,policko[i+1],policko[i]);
					break;
					case JS_OBJ_T_RESET:
						BIVAR1(pomstr,Cresetptr,policko[i+1],policko[i]);
						CBIVAR1((char*)(policko[i+2]),Cresetptr,policko[i+1],policko[i]);
					break;
					case JS_OBJ_T_HIDDEN:
						BIVAR1(pomstr,Chiddenptr,policko[i+1],policko[i]);
						CBIVAR1((char*)(policko[i+2]),Chiddenptr,policko[i+1],policko[i]);
					break;
					case JS_OBJ_T_BUTTON:
						BIVAR1(pomstr,Cbuttonptr,policko[i+1],policko[i]);
						CBIVAR1((char*)(policko[i+2]),Cbuttonptr,policko[i+1],policko[i]);
					break;
					case JS_OBJ_T_FRAME:
						BIVAR1(pomstr,Cframeptr,policko[i],policko[i]);
						CBIVAR1((char*)(policko[i+2]),Cframeptr,policko[i],policko[i]);
					break;
					case JS_OBJ_T_IMAGE:
						BIVAR1(pomstr,Cimageptr,policko[i+1],policko[i]);
						CBIVAR1((char*)(policko[i+2]),Cimageptr,policko[i+1],policko[i]);
					break;
					default: my_internal("GNU! To je typek!\n",context);
					break;
				}
				i+=3;
			}
			js_mem_free(pomstr);
			if(policko)mem_free(policko);
			BUILDVAR("length",Clength);
			js_lengthid=pomvar->identifier;
			pomvar->handler=pomint; /* Nastavim inicialni delku */
			BUILDFCE("join",Cjoin);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("reverse",Creverse);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("sort",Csort);
			pomvar->handler=(long)context->lnamespace;
			BUILDFCE("toString",CtoString);
			pomvar->handler=C_OBJ_elements;

			context->lnamespace=context->lnamespace->next;
#endif			
			idebug("Call exited!\n");
	
		break;
		case Cencoding:
			idebug("Cencoding called ");
			*typ=STRING;
			pomstr=js_upcall_get_form_encoding(context->ptr,pna->handler,pna->mid);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
			idebug("and exited!\n");
		break;
		case Cmethod:
			idebug("Cmethod called ");
			*typ=STRING;
			pomstr=js_upcall_get_form_method(context->ptr,pna->handler,pna->mid);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
			idebug("and exited!\n");
		break;
		case Cftarget:
			idebug("Cftarget called ");
			*typ=STRING;
			pomstr=js_upcall_get_form_target(context->ptr,pna->handler,pna->mid);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
			idebug("and exited!\n");
		break;
		case Chistorylength:
			idebug("Chistorylength called ");
			*typ=INTEGER;
			*value=(long)js_upcall_get_history_length(context->ptr);
			idebug("and exited!\n");
		break;
		case Cstatus:
			idebug("Cstatus called ");
			*typ=STRING;
			pomstr=js_upcall_get_status(context->ptr);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
			idebug("and exited!\n");
		break;
		case CdefaultStatus:
			idebug("CdefaultStatus called ");
			*typ=STRING;
			pomstr=js_upcall_get_default_status(context->ptr);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
			idebug("and exited!\n");
		break;
		case Ccookie:
			idebug("Ccookie called ");
			*typ=STRING;
			pomstr=js_upcall_get_cookies(context->ptr);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
			idebug("and exited!\n");
		break;
		case Cimages:
			idebug("Cimages called ");
                        *typ=ARRAY;
                        *value=(long)(pomns=js_mem_alloc(sizeof(plns)));
                        pomstr=js_mem_alloc(DELKACISLA+1);
                        pomns->next=context->lnamespace;
                        pomns->mid=pna->mid;
			pomns->handler=pna->handler;
                        pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
                        pomint=0;
                        while(pomint<HASHNUM)pomns->ns[pomint++]=0;
                        context->lnamespace=pomns;
                        pomvar=create(MIN1KEY,context->lnamespace,context);
                        pomvar->type=PARLIST;
                        pomvar->value=0; /* Bezprizorny namespace */
                        i=0;
                        pomint=0; /* Brain je ... */
			/* No jen to hezky rekni PerMe, at si te muzu podat!
			 *
			 *     Brain
			 */
                        policko=js_upcall_get_images(context->ptr,pna->mid,&pomint);
                        while(i<pomint)
                        {       snprintf(pomstr,DELKACISLA,"%d",i);
                                pomstr[DELKACISLA-1]='\0';
                                BIVAR1(pomstr,Cimageptr,policko[i++],pna->mid);
                        }
                        js_mem_free(pomstr);
                        if(policko)mem_free(policko);
                        BUILDVAR("length",Clength);
                        js_lengthid=pomvar->identifier;
                        pomvar->handler=pomint; /* Nastavim inicialni delku */
                        BUILDFCE("join",Cjoin);
                        pomvar->handler=(long)context->lnamespace;
                        BUILDFCE("reverse",Creverse);
                        pomvar->handler=(long)context->lnamespace;
                        BUILDFCE("sort",Csort);
                        pomvar->handler=(long)context->lnamespace;
                        BUILDFCE("toString",CtoString);
                        pomvar->handler=C_OBJ_image;
                        context->lnamespace=context->lnamespace->next;
			idebug("and exited!\n");
		break;
		case Cbuttonptr:
			idebug("Cbuttonptr called");
			*typ=ADDRSPACE;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
			pomns->next=context->lnamespace;
			pomns->mid=0;
			pomns->handler=0;
			pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
			pomint=0;
			while(pomint<HASHNUM)pomns->ns[pomint++]=0;
			context->lnamespace=pomns;
			pomvar=create(MIN1KEY,context->lnamespace,context);
			pomvar->type=PARLIST;
			pomvar->value=0; /* Bezprizorny namespace */
			BUILDFCE("click",Cclick);
			pomvar->handler=pna->handler;
			BUILDFCE("toString",CtoString);
			pomvar->handler=C_OBJ_form;
			context->lnamespace=context->lnamespace->next;
			idebug("and exited\n");
		break;
		case Cimageptr:
			idebug("Cimageptr called");
                        *typ=ADDRSPACE;
                        *value=(long)(pomns=js_mem_alloc(sizeof(plns)));
                        pomns->next=context->lnamespace;
                        pomns->mid=0;
			pomns->handler=0;
                        pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
                        pomint=0;
                        while(pomint<HASHNUM)pomns->ns[pomint++]=0;
                        context->lnamespace=pomns;
                        pomvar=create(MIN1KEY,context->lnamespace,context);
                        pomvar->type=PARLIST;
                        pomvar->value=0; /* Bezprizorny namespace */
                        BIVAR1("src",Csrc,pna->mid,pna->handler);
			BIVAR1("alt",Cimagealt,pna->mid,pna->handler);
			BIVAR1("border",Cborder,pna->mid,pna->handler);
			BIVAR1("complete",Ccomplete,pna->mid,pna->handler);
			BIVAR1("height",Cheight,pna->mid,pna->handler);
			BIVAR1("hspace",Chspace,pna->mid,pna->handler);
			BIVAR1("lowsrc",Clowsrc,pna->mid,pna->handler);
			BIVAR1("name",Cimagename,pna->mid,pna->handler);
			BIVAR1("vspace",Cvspace,pna->mid,pna->handler);
			BIVAR1("width",Cwidth,pna->mid,pna->handler);

                        BUILDFCE("toString",CtoString);
                        pomvar->handler=C_OBJ_form;

                        context->lnamespace=context->lnamespace->next;
                        idebug("and exited\n");	
		break;
		case Cimagealt:
			idebug("Cimagealt called ");
			*typ=STRING;
			pomstr=js_upcall_get_image_alt(context->ptr,pna->handler,pna->mid);
			*value=(long)stracpy1(pomstr);
			mem_free(pomstr);
			idebug("and exited!\n");
		break;
		case Cborder:
			idebug("Cborder called ");
                        *typ=INTEGER;
                        *value=(long)js_upcall_get_image_border(context->ptr,pna->handler,pna->mid);
                        idebug("and exited!\n");
		break;
		case Csrc:
			idebug("Csrc called ");
                        *typ=STRING;
                        pomstr=js_upcall_get_image_src(context->ptr,pna->handler,pna->mid);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
                        idebug("and exited!\n");
		break;
		case Ccomplete:
			idebug("Ccomplete called ");
                        *typ=INTEGER;
                        *value=(long)js_upcall_image_complete(context->ptr,pna->handler,pna->mid);
                        idebug("and exited!\n");
		break;
		case Cheight:
			idebug("Cheight called ");
                        *typ=INTEGER;
                        *value=(long)js_upcall_get_image_height(context->ptr,pna->handler,pna->mid);
                        idebug("and exited!\n");
		
		case Chspace:
			idebug("Chspace called ");
                        *typ=INTEGER;
                        *value=(long)js_upcall_get_image_hspace(context->ptr,pna->handler,pna->mid);
                        idebug("and exited!\n");
		break;
/*		case Clowsrc:
			idebug("Clowsrc called ");
                        *typ=INTEGER;
                        *value=(long)js_upcall_get_image_lowsrc(context->ptr,pna->handler,pna->mid);
                        idebug("and exited!\n");
		break;*/
		case Cimagename:
			idebug("Cname called ");
                        *typ=STRING;
                        pomstr=js_upcall_get_image_name(context->ptr,pna->handler,pna->mid);
			*value=(long)stracpy1(pomstr);
                        if(pomstr)mem_free(pomstr);
                        idebug("and exited!\n");
		break;
		case Cvspace:
			idebug("Cvspace called ");
                        *typ=INTEGER;
                        *value=(long)js_upcall_get_image_vspace(context->ptr,pna->handler,pna->mid);
                        idebug("and exited!\n");
		break;
		case Cwidth:
			idebug("Cwidth called ");
                        *typ=INTEGER;
                        *value=(long)js_upcall_get_image_width(context->ptr,pna->handler,pna->mid);
                        idebug("and exited!\n");
		break;
		case Cframeptr:
			idebug("Cframeptr called ");
			*typ=ADDRSPACE;
			*value=(long)(pomns=js_mem_alloc(sizeof(plns)));
                        pomns->next=context->lnamespace;
                        pomns->mid=pna->mid;
			pomns->handler=pna->mid;
                        pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
                        pomint=0;
                        while(pomint<HASHNUM)pomns->ns[pomint++]=0;
                        context->lnamespace=pomns;
                        pomvar=create(MIN1KEY,context->lnamespace,context);
                        pomvar->type=PARLIST;
                        pomvar->value=0; /* Bezprizorny namespace */
			buildin_document(context,pna->mid);
			BIVAR1("location",Clocation,pna->mid,pna->handler);
                        BIVAR1("frames",Csubframes,pna->mid,pna->handler);
			BIVAR1("parent",Cframeparent,pna->mid,pna->handler);
			BIVAR1("self",Cframeparent,pna->mid,pna->handler);
			BIVAR1("top",Cframeparent,pna->mid,pna->handler);
			BIVAR1("window",Cframeparent,pna->mid,pna->handler);
			BUILDFCE("alert",Calert);
			BUILDFCE("close",Cwclose);
			BUILDFCE("confirm",Cconfirm);
			BUILDFCE("open",Cwopen);
			BUILDFCE("prompt",Cprompt);
			BUILDFCE("setTimeout",CsetTimeout);
			BUILDFCE("clearTimeout",CclearTimeout);
			BUILDFCE("toString",CtoString);
                        pomvar->handler=C_OBJ_frame;

                        context->lnamespace=context->lnamespace->next;
                        idebug("and exited\n");

		break;

		default: 
			if(!options_get_bool("js_all_conversions"))
				js_error("get_var_value doesn't work yet\n",context);
			*typ=UNDEFINED;
			*value=0;
		break;
	}
	if(*typ==STRING && !*value)*typ=UNDEFINED;
}

/* Sada pomocnych funkci konvertujicich prvky vsech typu, co jich byl kdy
 * stvoril svet do vsech moznych typu, co jich byl kdy vymyslel clovek
 */

char* iatostring(long typ, long value,js_context*context)
{	abuf*pombuf;
	float*pomfloat;
	char*pomstr,*pomstr1;
	lns*fotri;
	switch(typ)
	{	case UNDEFINED:
		case NULLOVY:
		case BOOLEAN:
		case INTEGER:
		case FUNKCE:
		case FUNKINT:
			/* Nic se neztrati */
		break;
		case FLOAT: 
			pomfloat=js_mem_alloc(sizeof(float));
			*pomfloat=*(float*)value;
			value=(long)pomfloat;
		break;
		case STRING:
			pomstr=js_mem_alloc(strlen((char*)value)+1);
			strcpy(pomstr,(char*)value);
			value=(long)pomstr;
		break;
		case ADDRSPACE:
		case ADDRSPACEP:
		case ARRAY:
			fotri=lookup(MIN1KEY,(plns*)value,context);
			if(fotri->type!=PARLIST)
				my_internal("Parentlist corrupted!\n",context);
			add_to_parlist(&fotr_je_lotr,fotri);
			pombuf=js_mem_alloc(sizeof(abuf));
			pombuf->typ=typ;
			pombuf->argument=value;
			pomstr=tostring(pombuf,context);
			delete_from_parlist(&fotr_je_lotr,fotri);
			pomstr1=stracpy(pomstr);
			js_mem_free(pomstr);
			return pomstr1;
			idebug("iatostring: Pozor! objekt->string dosud neni vyladen\n");
		break;
		default:
			my_internal("Unknown type for assign to intern. var.!\n",context);
		break;
	}
	pombuf=js_mem_alloc(sizeof(abuf));
	pombuf->typ=typ;
	pombuf->argument=value;
	pomstr= tostring(pombuf,context);
	pomstr1=stracpy(pomstr);
	js_mem_free(pomstr);
	return pomstr1;
}

int iato32int(long typ,long value, js_context*context)
{	abuf*pombuf;
	float*pomfloat;
	int pom;
	lns*fotri;
	char*pomstr;
	switch(typ)
	{	case UNDEFINED:
		case NULLOVY:
		case BOOLEAN:
		case INTEGER:
		case FUNKCE:
		case FUNKINT:
		/* Nic se neztrati */
		break;
		case FLOAT:
			pomfloat=js_mem_alloc(sizeof(float));
			*pomfloat=*(float*)value;
			value=(long)pomfloat;
		break;
		case STRING:
			pomstr=js_mem_alloc(strlen((char*)value)+1);
			strcpy(pomstr,(char*)value);
			value=(long)pomstr;
		break;
		case ADDRSPACE:
		case ADDRSPACEP:
		case ARRAY:
			fotri=lookup(MIN1KEY,(plns*)value,context);
                        if(fotri->type!=PARLIST)
                                my_internal("Parentlist corrupted!\n",context);
                        add_to_parlist(&fotr_je_lotr,fotri);
                        pombuf=js_mem_alloc(sizeof(abuf));
                        pombuf->typ=typ;
                        pombuf->argument=value;
                        pom=to32int(pombuf,context);
                        delete_from_parlist(&fotr_je_lotr,fotri);
                        return pom;
/*			idebug("iatobool: Pozor, objekt->boolean neni vyladeno!\n");*/
		break;
		default:
			internal("Unknown type for assign to intern. var.!\n");
		break;
	}
	pombuf=js_mem_alloc(sizeof(abuf));
	pombuf->typ=typ;
	pombuf->argument=value;
	return to32int(pombuf,context);
}

int iatobool(long typ, long value, js_context*context)
{	abuf*pombuf;
	float*pomfloat;
	int pom;
	lns*fotri;
	char*pomstr;
	switch(typ)
	{	case UNDEFINED:
		case NULLOVY:
		case BOOLEAN:
		case INTEGER:
		case FUNKCE:
		case FUNKINT:
			/* Nic se neztrati */
		break;
		case FLOAT:
			pomfloat=js_mem_alloc(sizeof(float));
			*pomfloat=*(float*)value;
			value=(long)pomfloat;
		break;
		case STRING:
			pomstr=js_mem_alloc(strlen((char*)value)+1);
			strcpy(pomstr,(char*)value);
			value=(long)pomstr;
		break;
		case ADDRSPACE:
		case ADDRSPACEP:
		case ARRAY:
			fotri=lookup(MIN1KEY,(plns*)value,context);
                        if(fotri->type!=PARLIST)
                                my_internal("Parentlist corrupted!\n",context);
                        add_to_parlist(&fotr_je_lotr,fotri);
                        pombuf=js_mem_alloc(sizeof(abuf));
                        pombuf->typ=typ;
                        pombuf->argument=value;
                        pom=tobool(pombuf,context);
                        delete_from_parlist(&fotr_je_lotr,fotri);
                        return pom;
/*			idebug("iatobool: Pozor, objekt->boolean neni vyladeno!\n");*/
		break;
		default:
			my_internal("Unknown type for assign to intern. var.!\n",context);
		break;
	}
	pombuf=js_mem_alloc(sizeof(abuf));
	pombuf->typ=typ;
	pombuf->argument=value;
	return tobool(pombuf,context);
}

void set_var_value(lns*pna,long typ, long value,js_context*context)
{	abuf*pombuf;
/*	float*pomfloat;*/
	char*pomstr;
	int pomint;
	switch(pna->value)
	{	case Cmatika:
			idebug("To nejde!\n");
		break;
		case Cbarvicka:
			pna->handler=iato32int(typ,value,context);
			idebug("Nastavena barvicka!\n");
		break;
		case Ctitle:
			js_upcall_set_title(context->ptr,iatostring(typ,value,context));
			idebug("Nastavena promenna Ctitle\n");
		break;
		case Cchecked:
			pomint=iatobool(typ,value,context);
			js_upcall_set_checkbox_radio_checked(context->ptr,pna->handler,pna->mid,pomint);
			idebug("Nastavena pna Cchecked\n");
		break;
		case CdefaultChecked:
			pomint=iatobool(typ,value,context);
			js_upcall_set_checkbox_radio_default_checked(context->ptr,pna->handler,pna->mid,pomint);
			idebug("Nastavena pna CdefaultChecked\n");
		break;
		case Cfename:
			js_upcall_set_form_element_name(context->ptr,pna->handler,pna->mid,iatostring(typ,value,context));
			idebug("Nastavena promenna Cfename\n");
		break;
		case CIntMETFUN:
			js_error("You're trying to assign internal properties!!\n",context);
		break;
		case Cvalue:
			js_upcall_set_form_element_value(context->ptr,pna->handler,pna->mid,iatostring(typ,value,context)); 
			idebug("Nastavena promenna Cvalue\n");
		break;
		case CdefaultValue:
			js_upcall_set_form_element_default_value(context->ptr,pna->handler,pna->mid,iatostring(typ,value,context));
			idebug("Nastavena promenna CdefaultValue\n");
		break;
		case Chref:
		case Clocation:
			pomstr=iatostring(typ,value,context);
			call_open(context,stracpy1(pomstr),0);
			if(pomstr)mem_free(pomstr);
			js_durchfall=1;
			pombuf=pulla(context);
			delarg(pombuf,context);
			pombuf=js_mem_alloc(sizeof(abuf));
			pombuf->typ=UNDEFINED;
			pombuf->argument=0;
			pusha(pombuf,context);
			/* Po location=... se bude cekat na odpoved. */
			idebug("Nastavena locationa\n");
		break;
		case Cstatus:
			js_upcall_set_status(context->ptr,iatostring(typ,value,context));
			idebug("Nastaveny status\n");
		break;
		case CdefaultStatus:
			js_upcall_set_default_status(context->ptr,iatostring(typ,value,context));
			idebug("Nastaveny defaultstatus\n");
		break;
		case Clength:
			pna->handler=iato32int(typ,value,context);
			idebug("Nastavena delka seznamu\n");
		break;
		case Ccookie:
			if(context->cookies) js_mem_free(context->cookies);
			pomstr=iatostring(typ,value,context);
			context->cookies=stracpy1(pomstr);
			if(pomstr)mem_free(pomstr);
			jsint_set_cookies(context->ptr,0);
			idebug("Nastaveno pecivo\n");
		break;
		case Csrc:
#ifdef PRO_DEBILY
			if(context->zlomeny_ramecek_jako_u_netchcipu)
			{	idebug("U Bucku zhasli...");
				iatobool(typ,value,context);
				/* Memory-leak avoidance */
			}else {	
#endif
			call_setsrc(context,iatostring(typ,value,context),pna);
			pombuf=pulla(context);
                        delarg(pombuf,context);
                        pombuf=js_mem_alloc(sizeof(abuf));
                        pombuf->typ=UNDEFINED;
                        pombuf->argument=0;
                        pusha(pombuf,context);
			idebug("Nastaven obrazek\n");
#ifdef PRO_DEBILY
			}
#endif
		break;
		case Cimagename:
			js_upcall_set_image_name(context->ptr,pna->handler,pna->mid,iatostring(typ,value,context));
			idebug("Nastaveno jmeno obrazku\n");
		break;
		case Cimagealt:
			js_upcall_set_image_alt(context->ptr,pna->handler,pna->mid,iatostring(typ,value,context));
			idebug("Nastavena altituda image\n");
		break;
		default: js_error("set_var_value doesn't work yet\n",context);
		break;
	}
}

/* kill_var je funkce nutna pro hladky pad javascriptu. Kdyz se odhlodame, ze
 * chceme cely kontext javascriptu zrusit, tato funkce musi pouklizet v pameti
 * po vsech internich promennych. Obcas se vola i za letu, to kdyz se pod
 * nejakou interni promennou "propadne rodic"
 */

void kill_var(lns*pna)
{	switch(pna->value)
	{	case Cmatika:
			js_mem_free((float*)pna->handler);
		break;
/*		case Cforms:
			js_mem_free(pna);
		break;*/
		case Cbarvicka:
		case Clength:
		case Canchors:
		case Ccookie:
		case Cforms: 
		case ClastModified:   /* hotovo */
		case Clinks:
		case Clocation:  
		case Creferer:   /* get hotovo, set nema smysl */
		case Ctitle:   /* get i set hotovo */
		case CappCodeName:   /* hotovo */
		case CappName:   /* hotovo */
		case CappVersion:   /* hotovo */
		case CuserAgent: /* hotovo */
		case CdefaultStatus:
		case Cframes:
		case Cwlength:
		case Cname:
		case Cstatus:
		case Chash:
		case Chost:
		case Chostname:
		case Chref:
		case Cpathname:
		case Cport:
		case Cprotocol:
		case Csearch:
		case Clinkptr:
		case Ctarget:
		case Cformptr:
		case Caction:
		case Celements:
		case Cencoding:
		case Cmethod:
		case Cftarget:
		case Ctextptr:
		case Cpasswdptr:
		case Ctextarptr:
		case Cchkboxptr:
		case Cradioptr:
		case Cselectptr:
		case Csubmitptr:
		case Cresetptr:
		case Chiddenptr:
		case Canchorptr:
		case Cchecked:
		case CdefaultChecked:
		case Cfename:
		case Cvalue:
		case Cradiolength:
		case CdefaultValue:
		case Cselectlength:
		case Cselectname:
		case Cselectoptions:
		case CselectmrchadefaultSelected:
		case Cselectindex:
		case CselectselectedIndex:
		case Cselectmrchatext:
		case Cselectmrchavalue:
		case Cselectmrchaselected:
		case Cselectmrcha:
		case Chistorylength:
		case Cimages:
		case Cimageptr:
		case Csrc:
		case Cborder:
		case Ccomplete:
		case Cheight:
		case Chspace:
		case Clowsrc:
		case Cimagename:
		case Cvspace:
		case Cwidth:
		case Cframeptr:
		case Csubframes:
		case Cframeparent:
		case Cimagealt:
		case Cbuttonptr:
		case Ctop:
		case Cself:
		case Cparent:
		case Call:
		break;
		default:
			printf("%d - \n",(int)pna->value);
			internal("Killing unknown variable!\n");
		break;
	}
}

/* Sada downcallu, kterou se mi oznamuje, ze BFU kliklo na alertitko,
 * odkliklo v textovem okenku text, v nemz souhlasi s tim, ze je BFU...,
 * reklo, ze mluvi pravdu, nebo ze lze...
 */

void js_downcall_vezmi_null(void*jezis_to_je_ale_krawina)
{	js_context*context=jezis_to_je_ale_krawina;
	js_bordylek*bordylek;
	abuf*bufet;
	int timerno=0;
	if(context->running)
	{	bufet=pulla(context);
		if(!bufet)my_internal("Downcall narazil hlavou do skaly!\n",context);
		if(bufet->typ!=UNDEFINED)my_internal("Nevyzadany downcall!!\n",context);
		idebug("Vracim se z upcallu s nullovym vysledkem!\n");
		if(context->current)
		{	pusha(bufet,context);
			while(timerno<TIMERNO &&(context->t[timerno]!=-1))timerno++;
			if(timerno>=TIMERNO)
			{	js_error("Too many timers",context);
/*				delarg(bufet,context);*/
				return;
			}
			bordylek=js_mem_alloc(sizeof(js_bordylek));
			bordylek->context=context;
			bordylek->mytimer=&context->t[timerno];
			context->bordely[timerno]=bordylek;
			context->t[timerno]=install_timer(1,(void(*)(void*))ipret,bordylek);
		}
		else {	delarg(bufet,context);
			idebug("Downcall to zapichuje!!\n");
			context->running=0;
			js_volej_kolbena(context);
		}
	}
}

void js_downcall_vezmi_true(void*jezis_to_je_ale_krawina)
{	js_context*context=jezis_to_je_ale_krawina;
	abuf*bufet;
	js_bordylek*bordylek;
	int timerno=0;
	if(context->running)
	{	bufet=pulla(context);
		if(!bufet)my_internal("Downcall narazil hlavou do skaly!\n",context);
		if(bufet->typ!=UNDEFINED)my_internal("Nevyzadany downcall!!\n",context);
		idebug("Vracim se z upcallu s pravdivym vysledkem!\n");
		if(context->current)
		{	pusha(bufet,context);
			bufet->typ=BOOLEAN;
			bufet->argument=TRUE;
			while(timerno<TIMERNO &&(context->t[timerno]!=-1))timerno++;
			if(timerno>=TIMERNO)
			{	js_error("Too many timers",context);
/*				delarg(bufet,context);*/
				return;
			}
			bordylek=js_mem_alloc(sizeof(js_bordylek));
			bordylek->context=context;
			bordylek->mytimer=&context->t[timerno];
			context->bordely[timerno]=bordylek;
			context->t[timerno]=install_timer(1,(void(*)(void*))ipret,bordylek);
		}
		else {	delarg(bufet,context);
			idebug("Downcall to zapichuje!!\n");
			context->zaplatim=1;
			context->running=0;
			js_volej_kolbena(context);
		}
	}
}

void js_downcall_vezmi_false(void*jezis_to_je_ale_krawina)
{	js_context*context=jezis_to_je_ale_krawina;
	abuf*bufet;
	js_bordylek*bordylek;
	int timerno=0;
	if(context->running)
	{	bufet=pulla(context);
		if(!bufet)my_internal("Baudys narazil hlavou do skaly! B-)\n",context);
		if(bufet->typ!=UNDEFINED)my_internal("Nevyzadany downcall!!\n",context);
		idebug("Vracim se z upcallu s pravdivym vysledkem!\n");
		if(context->current)
		{	pusha(bufet,context);
			bufet->typ=BOOLEAN;
			bufet->argument=FALSE;
			while(timerno<TIMERNO &&(context->t[timerno]!=-1))timerno++;
			if(timerno>=TIMERNO)
			{	js_error("Too many timers",context);
/*      	                delarg(bufet,context);*/
				return;
			}
			bordylek=js_mem_alloc(sizeof(js_bordylek));
			bordylek->context=context;
			bordylek->mytimer=&context->t[timerno];
			context->bordely[timerno]=bordylek;
			context->t[timerno]=install_timer(1,(void(*)(void*))ipret,bordylek);
		}
		else {	delarg(bufet,context);
			idebug("Downcall to zapichuje!!\n");
			context->running=0;
			js_volej_kolbena(context);
		}
	}
}

void js_downcall_vezmi_string(void*jezis_to_je_ale_krawina,unsigned char*to_je_on_padre)
{	js_context*context=jezis_to_je_ale_krawina;
	abuf*bufet;
	js_bordylek*bordylek;
	int timerno=0;
	if(context->running)
	{	bufet=pulla(context);
		if(!bufet)my_internal("Downcall narazil hlavou do futra!\n",context);
		if(bufet->typ!=UNDEFINED)my_internal("Nevyzadany downcall!!\n",context);
		idebug("Vracim se z upcallu s retezcem na krku!\n");
		bufet->typ=STRING;
		bufet->argument=(long)stracpy1(to_je_on_padre);
		if(!bufet->argument)
			bufet->typ=UNDEFINED;
		if(to_je_on_padre)mem_free(to_je_on_padre);
		if(context->current)
		{	pusha(bufet,context);
			while(timerno<TIMERNO &&(context->t[timerno]!=-1))timerno++;
			if(timerno>=TIMERNO)
			{	js_error("Too many timers",context);
/*              	        delarg(bufet,context);*/
				return;
			}
			bordylek=js_mem_alloc(sizeof(js_bordylek));
			bordylek->context=context;
			bordylek->mytimer=&context->t[timerno];
			context->bordely[timerno]=bordylek;
			context->t[timerno]=install_timer(1,(void(*)(void*))ipret,bordylek);
		}
		else {	context->zaplatim=to32int(bufet,context);
/*			delarg(bufet,context);*/
			idebug("Downcall to zapichuje!!\n");
			context->running=0;
			js_volej_kolbena(context);
		}
	}
}
#endif
