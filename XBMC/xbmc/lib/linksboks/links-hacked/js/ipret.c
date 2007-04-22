/* ipret.c
 * Javascript interpreter
 * (c) 2002 Martin 'PerM' Pergel
 * This file is a part of the Links program, released under GPL.
 */

/* Tady bude funkce ipret(bordylek);, ktera bude interpretovat z kontextu, dale tu budou zit funkce
pushp(parent) a pullp(void), stejne jako pusha(argument) a pulla(void), ktere se tu budou huste pouzivat. pushp a pullp budou obsluhovat zasobnik callu (rodicu
daneho uzle, pusha a pulla nebudou jako u INTELat, ale budou obsluhovat 
zasobniky argumentu pro operatory. (mimochodem push a pull maji Motoroily)
*/
/* Invariant: strukturu abuf si musi naalokovat a odalokovat ten, kdo ji dela, nikoliv ten, kdo ji cpe na buffer. */
#include "../cfg.h"

#ifdef JS

#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include "struct.h"
#define RASODEBUG 1
#undef RASODEBUG
#include "tree.h"
#include "typy.h"
#include "ipret.h"
#include "ns.h"
#include "builtin.h"
#include "builtin_keys.h"

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

FILE *js_file;

static int js_temp_var_for_vartest=0;

#define VARTEST1(A)	js_temp_var_for_vartest=0; \
			if(A->typ!=VARIABLE)\
			{	delarg(A,context); /* Konsolidace memory-leaku */ \
				js_temp_var_for_vartest=1; \
			}

#define VARTEST(A,B)	VARTEST1(A); \
		if(js_temp_var_for_vartest) \
		{	js_temp_var_for_vartest=0; \
			js_error(B,context); \
			return; \
		} 
#undef debug
/* #define JS_DEBUG_2 1 */
#define debug(a) \
	fprintf(stderr,a)
#undef debug
#define debug(a)
#define KROKU 100

extern lns fotr_je_lotr;
extern long js_lengthid;

extern long MIN1KEY;
extern long CStoString,CSvalueOf,CSMIN_VALUE,CSMAX_VALUE,CSNaN,CSlength,
	CSindexOf,CSlastIndexOf,CSsubstring,CScharAt,CStoLowerCase,CSsubstr,
	CStoUpperCase,CSsplit,CSparse,CSUTC;

static int sezvykan;
static int schedule;

int js_durchfall=0; /* Rika, jestli mame propadnout nebo ne */
		    /* Invariant: Pokud vypadneme, musime ho snulova! */

/* Pomocne funkce, ktere ovladaji zasobniky rodicu ve strome a argumentu
 * (to, co je u kompilatoru realizovano zasobnikem).
 */

void pushp(vrchol* parent,js_context*context)
{	pbuf*pom=js_mem_alloc(sizeof(pbuf));
	pom->parent=parent;
	pom->in=parent->in; /* Tolikaty argument jsme cetli */
	parent->in=0; /* Kvuli rekurzi */
	pom->in1=parent->arg[3];
	parent->arg[3]=0;
	pom->next=context->parbuf;
	context->parbuf=pom;
}

vrchol* pullp(js_context*context)
{	pbuf*pom=context->parbuf;
	vrchol*vrat;

	if(!pom){
		debug("Podtekl zasobnik otcu!\n");
		vrat=0;
	}else {	vrat=pom->parent;
		vrat->in=pom->in; /* Vratime se do tolikateho cteni - kvuli rekurzi */
		vrat->arg[3]=pom->in1;
		context->parbuf=pom->next;
		js_mem_free(pom);
	}
	return vrat;
}

double mojeinv(double a)
{	if(a==MY_NAN)
		return MY_NAN;
	if(a==MY_INFINITY || a==MY_MININFINITY)
		return 0;
	if(a==0)
		return MY_NAN;
	if(a>0)
	{	if(a<((double)1/MY_MAXDOUBLE))
			return MY_INFINITY;
		else	return 1/a;
	}else{	if(a>((double)1/MY_MINDOUBLE))
			return MY_MININFINITY;
		else	return 1/a;
	}
}

double mojenas(double a,double b)
{	if(a==MY_NAN || b==MY_NAN||((a==MY_INFINITY || a==MY_MININFINITY)&&(b==MY_INFINITY||b==MY_MININFINITY))||
		(a==0 &&(b==MY_INFINITY||b==MY_MININFINITY))||(b==0 &&(a==MY_INFINITY || a==MY_MININFINITY)))
		return MY_NAN;
	if(a==0 || b==0) return 0;
	if(a>0)
	{	if(b==MY_INFINITY)
			return MY_INFINITY;
		if(b==MY_MININFINITY)
			return MY_MININFINITY;
	}
	if(a<0)
	{	if(b==MY_INFINITY)
			return MY_MININFINITY;
		if(b==MY_MININFINITY)
			return MY_INFINITY;
	}
	if(b>0)
	{	if(a==MY_INFINITY)
			return MY_INFINITY;
		if(a==MY_MININFINITY)
			return MY_MININFINITY;
	}
	if(b<0)
	{	if(a==MY_INFINITY)
			return MY_MININFINITY;
		if(a==MY_MININFINITY)
			return MY_INFINITY;
	}
	if((a>=-1 && a<=1)||(b>=-1 && b<=1))
		return a*b;
	if(a>0 && b>0)
	{	if(MY_MAXDOUBLE/a<b)
			return MY_INFINITY;
		else
			if(a*b>MY_MAXDOUBLE)
				return MY_INFINITY;
			else	return a*b;
	}
	if(a>0 && b<0)
	{	if(MY_MINDOUBLE/b<a)
			return MY_MININFINITY;
		else
			if(a*b<MY_MINDOUBLE)
				return MY_MININFINITY;
			else
				return a*b;
	}
	if(a<0 && b>0)
	{	if(MY_MINDOUBLE/a<b)
			return MY_MININFINITY;
		else
			if(a*b<MY_MINDOUBLE)
				return MY_MININFINITY;
			else
				return a*b;
	}
	if(a<0 && b<0)
	{	if(MY_MAXDOUBLE/a>b)
			return MY_INFINITY;
		else
			if(a*b>MY_MAXDOUBLE)
				return MY_INFINITY;
			else	return a*b;
	}
	else return a*b; /* Sem by se nemelo jit dostat! */
}

double mojescit(double a,double b)
{	if(a==MY_NAN || b==MY_NAN||(a==MY_INFINITY && b==MY_MININFINITY)||(a==MY_MININFINITY && b==MY_INFINITY))
		return MY_NAN;
	if(a==MY_INFINITY || b==MY_INFINITY)
		return MY_INFINITY;
	if(a==MY_MININFINITY|| b==MY_MININFINITY)
		return MY_MININFINITY;
	if(a>0 && b>0)
	{	if(MY_MAXDOUBLE-a<b)
			return MY_INFINITY;
		else	return a+b;
	}else{	if(a<0 && b<0)
		{	if(MY_MINDOUBLE-a>b)
				return MY_INFINITY;
			else	return a+b;
		} else	return a+b;
	}
}

#ifdef DEBUGMEMORY
char komentar[100];/* Tohle je jenom debuzi! */
void real_pusha(abuf *a,js_context*context,unsigned char*soubor,int lajna)
#else
void pusha(abuf*a,js_context*context)
#endif
{	a->next=context->argbuf;
	context->argbuf=a;
#ifdef DEBUGMEMORY
	snprintf(komentar,99,"%s %d",soubor,lajna);
	komentar[99]='\0';
	set_mem_comment((void*)((char*)a-J_A_S),komentar,strlen(komentar));
#endif
}

#ifdef DEBUGMEMORY
abuf* real_pulla(js_context*context,unsigned char*soubor,int lajna)
#else
abuf* pulla(js_context*context)
#endif
{	abuf*pom=context->argbuf;
	if(!pom){
		debug("Podtekl zasobnik argumentu!\n");
	/* Je pouze debuzi, protoze podteceni zasobniku argu bude OK pri
	breaku, continue, returnu apod.*/
		pom=0;
	} else {
		context->argbuf=pom->next;
		pom->next=0;
#ifdef DEBUGMEMORY
	        snprintf(komentar,99,"%s %d ",soubor,lajna);
        	komentar[99]='\0';
		strncat(komentar,get_mem_comment((char *)pom-J_A_S),99-strlen(komentar));
		set_mem_comment((void*)((char*)pom-J_A_S),komentar,strlen(komentar));
#endif
	}
	return pom;
}

/* js_error za kazdeho pocasi ukonci interpretaci. Kdo chce interpretaci 
 * ukoncit aniz by js_error nezavolal, vystavuje se riziku toho, ze script 
 * bude drsnacky pokracovat dal. Jedinou vyjimku ma js_destroy_context, ktery
 * vidi jeste dal a rozboura vsechno, tedy i to, co probori js_error
 */

void js_error(char*a,js_context*context)
{

	unsigned char *txt,*txt2;
	int timerno;
	int i=0,j=0,k=0;
	int l;
	struct f_data_c *fd=(struct f_data_c*)(context->ptr);
	struct terminal *term=fd->ses->term;
	if(!context->jsem_dead)
	{	if(options_get_bool("js_verbose_errors"))
		{/*	snprintf(txt,MAX_STR_LEN,"%s in line %d!!!\b\b\n",a,(int)context->current->lineno);*/
			if(context->current) {
				while(i<context->current->lineno && context->code[j])
				{	if((context->code[j]=='\n')||(context->code[j]=='\r'))i++;
					j++;
				}
				if(context->code[j])
				{	k=j;
					while(context->code[k] && context->code[k]!='\n' && context->code[k]!='\r')k++;
					if(context->code[k])
					{	i=1;
						context->code[k]='\0';
					} else	i=0;
				}
				txt=stracpy(&context->code[j]);
				if (strlen(txt)>MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU)
				{
					txt[MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU-1]='.';
					txt[MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU-2]='.';
					txt[MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU-3]='.';
					txt[MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU-4]=' ';
					txt[MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU]=0;
				}
				skip_nonprintable(txt);
				if (fd->f_data)
				{
					struct conv_table* ct;
					
					ct=get_translation_table(fd->f_data->cp,fd->f_data->opt.cp);
					txt2=convert_string(ct,txt,strlen(txt),NULL);
				}
				else
					txt2=stracpy(txt);
				mem_free(txt);

				txt=init_str();
				l=0;
				add_to_str(&txt,&l,a);
				add_to_str(&txt,&l," in: ");
				add_to_str(&txt,&l,txt2);
				add_to_str(&txt,&l,"\n");
				mem_free(txt2);

				if(i)	context->code[k]='\n';
			} else	txt=stracpy(a);

#ifdef DEBUZ_KRACHY
/*			js_file=fopen("links_debug.err","a");
			fprintf(js_file,"%s\n in line %d\n Code:\n%s\n",txt,context->current->lineno,context->code);
			fclose(js_file); */
#endif

			msg_box(
				term,   /* terminal */
				getml(txt,NULL),   /* memory blocks to free */
				TXT(T_JAVASCRIPT_ERROR),   /* title */
				AL_CENTER,   /* alignment */
				txt,   /* message */
				NULL,   /* data for button functions */
				1,   /* # of buttons */
				TXT(T_DISMISS),NULL,B_ENTER|B_ESC  /* first button */
			);
		}
		context->utraticka=1;
	}
	for(timerno=0;timerno<TIMERNO;timerno++)
		if(context->t[timerno]!=-1)
		{	kill_timer(context->t[timerno]);
			context->t[timerno]=-1;
			js_mem_free(context->bordely[timerno]);
		}
	if(context->upcall_timer!=-1)
		js_spec_vykill_timer(context,1);
	js_durchfall=1;
	context->jsem_dead=1;
	context->zaplatim=1;
}

void delete_name_space(plns*arg,js_context*context)
{	struct jsnslist * seznam,*pom;
/*	seznam->pns=arg;
	seznam->next=0;
	*context->last_ns=seznam;
	context->last_ns=&seznam->next;*/
	pom=context->first_ns;
	if(pom)
		while(pom->next && pom->pns!=arg)
			pom=pom->next;

	if((!pom)||(pom->pns!=arg))
	{	seznam=js_mem_alloc(sizeof(struct jsnslist));
		seznam->pns=arg;
		seznam->next=0;
		if(pom)	pom->next=seznam;
		else	context->first_ns=seznam;
	}
/* Sypeme rusene namespacy do listu, zrusime je pri derivovani na statement pro
 * pripad, ze je jeste nekdy budeme potrebovat. */
}

void delarg(abuf*pom,js_context*context)
{	lns*zrusit;
	switch(pom->typ)
	{	case FLOAT:
		case STRING:
			js_mem_free((void*)pom->argument); 			
		case NULLOVY:
		case INTEGER:
		case UNDEFINED:
		case BOOLEAN:
			js_mem_free(pom);
		break;
		case VARIABLE:
			zrusit=(lns*)pom->argument;
			if(zrusit)
			{	if(zrusit->value==CIntMETFUN)
				{	delarg((abuf*)zrusit->handler,context);
					js_mem_free(zrusit); /* Kdyz zustane trcet na zasobniku
						* interni funkce nebo metoda, tak ji je potreba znicit, stejne
						* se nici okamzite po pouziti */
				}
			} else	my_internal("Killing null variable!\n",context);
			js_mem_free(pom);
		break;
		case ARGUMENTY:
			zrusargy(pom,context);
		break;
		case ADDRSPACE:
		case ADDRSPACEP:
		case ARRAY:
			if(pom->argument)
			{	zrusit=lookup(MIN1KEY,(plns*)pom->argument,context);
				if(zrusit->type!=PARLIST)
				{	my_internal("Internal: Parentlist corrupted!\n",context); return;
				}
				if(!zrusit->value)/*volsovej namespace - nekdo udelal:
						    ;new blbost(); */
					delete_name_space((plns*)pom->argument,context);
			}
			js_mem_free(pom);
		break;
		case FUNKCE:
		case FUNKINT:
		case INTVAR:
			js_mem_free(pom);
		break;
		default: my_internal("Error! Rusim divny typ!\n",context);
	}
}

void vartoarg(lns*variable,abuf*kam,js_context*context)
{	switch(kam->typ=variable->type)
	{	case UNDEFINED:
		case NULLOVY:
		case INTEGER:
		case BOOLEAN:
			kam->argument=variable->value;
		break;
		case FLOAT:
			kam->argument=(long)js_mem_alloc(sizeof(float));
			*(float*)kam->argument=*(float*)variable->value;
		break;
		case STRING:
			kam->argument=(long)js_mem_alloc(strlen((char*)variable->value)+1);
			strcpy((char*)kam->argument,(char*)variable->value);
		break;
		case ADDRSPACEP:
			kam->typ=ADDRSPACE;
		case ARRAY:
			kam->argument=variable->value;
		break;
		case FUNKCE:
			kam->argument=variable->value;
		break;
		case FUNKINT:
			debug("Strange handling internal function!\n");
/*			kam->argument=(long)js_mem_alloc(strlen("function f(){ [native code] }")+1);*/
			kam->argument=(long)stracpy1("function f(){ [native code] }");
			kam->typ=STRING;
		break;
		case INTVAR:
			debug("Getting value of internal variable\n");
			get_var_value(variable,&kam->typ,&kam->argument,context);
		break;
		default: my_internal("Error! Tahle konverze jeste nefunguje!\n",context);
			kam->typ=INTEGER;
			kam->argument=0;
		break;
	}
}

void clearvar(lns*pna,js_context*context)
{	lns*zrusit;
	switch(pna->type)
	{	case UNDEFINED:
		case NULLOVY:
		case BOOLEAN:
		case INTEGER:
		case FUNKCE:
		case FUNKINT:
		break;
		case INTVAR:
			kill_var(pna);
		break;
		case FLOAT:
		case STRING:
			js_mem_free((void*)pna->value);
		break;
		case ADDRSPACEP:
		case ARRAY:
			zrusit=lookup(MIN1KEY,(plns*)pna->value,context);
			if(zrusit->type!=PARLIST)
			{	my_internal("Internal: Parent list corrupted!!\n",context);	return;}
			if(!((parlist*)zrusit->value) || !(((parlist*)zrusit->value)->next)) /*variable became garbage*/
				deletenamespace((plns*)pna->value,context);
			else
				delete_from_parlist(pna,zrusit);
			/* Pokud existuje vice odkazu, tak jen soucasny 
			 * vyhodime z te databaze. */
		break;
		default:
			my_internal("Aiee! Unknown variable type!\n",context);
			return; /* Tak pod to aspon nebudeme zapisovat! */
		break;
	}
	pna->type=UNDEFINED;
	pna->value=0;
}

int vartoint(lns*pna,js_context*context)
{	abuf*pomocny;
	float pomfloat;
	int retval=0;
	lns*promenna;
	switch(pna->type)
	{	case UNDEFINED: if(!options_get_bool("js_all_conversions"))
					js_error("You use UNDEFINED value!\n",context);
			retval=1; 
			/* aby Brain mohl pocitat screen.depth/screen.depka */
		break;
		case NULLOVY:
		break;
		case BOOLEAN:
		case INTEGER:
			retval=pna->value;
		break;
		case FLOAT:
			retval=(int)rint(pomfloat=*(double*)pna->value);
#ifdef RASODEBUG
			printf("vartoint vraci: %d\n",retval);
#endif
			if((pomfloat==MY_NAN)||(pomfloat==MY_INFINITY)||(pomfloat==MY_MININFINITY))
			{	retval=0;
				js_warning("Converting not a number to integer,\nreturning 0!",context->current->lineno,context);
			}
		break;
		case STRING:
			if(!strlen((char*)pna->value))
				if(!options_get_bool("js_all_conversions"))
					js_error("Converting empty string to number!\n",context);
			retval=atoi((char*)pna->value);
		break;
		case INTVAR:
			pomocny=js_mem_alloc(sizeof(abuf));
			get_var_value(pna,&pomocny->typ,&pomocny->argument,context);
			retval=to32int(pomocny,context);
		break;
		case FUNKCE:
		case FUNKINT:
			if(!options_get_bool("js_all_conversions"))
				js_error("Error converting function -> integer",context);
		break;
		case ADDRSPACEP:
		case ARRAY:
			if(context->depth>MAX_FCALL_DEPTH)
			{	if(!options_get_bool("js_all_conversions"))
					js_error("Too deep structure by valueOf!",context);
				context->depth=0;
			} else
			{
				promenna=llookup("valueOf",context->namespace,(plns*)pna->value,context);
				context->depth++;
				retval=vartoint(promenna,context);
				context->depth=0;
			}
		break;
		default:
			my_internal("Tato konverze jeste neni napsana!\n",context);
		break;
	}
	return retval;
}

float tofloat(abuf*,js_context*);

float vartofloat(lns*pna,js_context*context)
{	abuf*pomocny;
	lns*promenna;
	float retval=0;
	switch(pna->type)
	{	case UNDEFINED: 
			if(!options_get_bool("js_all_conversions"))
				js_error("You use UNDEFINED value!\n",context);
			retval=1;
		break;
		case NULLOVY:
		break;
		case BOOLEAN:
		case INTEGER:
			retval=(float)pna->value;
		break;
		case FLOAT:
			retval=*(float*)pna->value;
		break;
		case STRING:
			if(!strlen((char*)pna->value))
				if(!options_get_bool("js_all_conversions"))
					js_error("Converting empty string to number is prohibited!\n",context);
			retval=atof((char*)pna->value);
			if(retval>MY_MAXDOUBLE) retval=MY_INFINITY;
			if(retval<MY_MINDOUBLE) retval=MY_MININFINITY;
		break;
		case INTVAR:
			pomocny=js_mem_alloc(sizeof(abuf));
			get_var_value(pna,&pomocny->typ,&pomocny->argument,context);
			retval=tofloat(pomocny,context);
		break;
		case FUNKCE:
		case FUNKINT:
			if(!options_get_bool("js_all_conversions"))
				js_error("Error converting function -> number",context);
			retval=MY_NAN;
		break;
		case ADDRSPACE:
			debug("Strange conversion!\n");
		case ADDRSPACEP:
		case ARRAY:
			if(context->depth>MAX_FCALL_DEPTH)
			{	if(!options_get_bool("js_all_conversions"))
					js_error("Too deep structure by valueOf!",context);
				context->depth=0;
			} else
			{
				promenna=llookup("valueOf",context->namespace,(plns*)pna->value,context);
				context->depth++;
				retval=vartofloat(promenna,context);
				context->depth=0;
			}
		break;
		default:
			my_internal("Tato konverze jeste neni napsana!\n",context);
		break;
	}
	return retval;
}

int to32int(abuf*buffer,js_context*context)
{	int retval=0;
	lns*promenna;
	float pomfloat;
	switch(buffer->typ)
	{	case UNDEFINED:	if(!options_get_bool("js_all_conversions"))
					js_error("You use UNDEFINED value!\n",context);
			retval=1;
		break;
		case NULLOVY:
		break;
		case BOOLEAN:
		case INTEGER:
			retval=(int)buffer->argument;
		break;
		case FLOAT:
			retval=(int)rint(pomfloat=*(float*)buffer->argument);
#ifdef RASODEBUG
			printf("to32int vraci: %d\n",retval);
#endif
			js_mem_free((void*)buffer->argument);
			if((pomfloat==MY_NAN)||(pomfloat==MY_INFINITY)||(pomfloat==MY_MININFINITY))
			{	js_warning("Converting non-numeric float to integer\nreturning 0!",context->current->lineno,context);
				retval=0;
			}
		break;
		case STRING:
			if(!strlen((char*)buffer->argument))
				if(!options_get_bool("js_all_conversions"))
					js_error("Converting empty string to number!\n",context);
			retval=atoi((char*)buffer->argument);
			js_mem_free((void*)buffer->argument);
		break;
		case VARIABLE:
		case INTVAR:
			retval=vartoint((lns*)buffer->argument,context);
		break;
		case FUNKCE:
		case FUNKINT:
			if(!options_get_bool("js_all_conversions"))
				js_error("Conversion not allowed! ",context);
		break;
		case ADDRSPACE:
		case ADDRSPACEP:
		case ARRAY:
			/* Tady je potreba zavolat valueOf a udelat break; */
			if(context->depth>MAX_FCALL_DEPTH)
			{	if(!options_get_bool("js_all_conversions"))
					js_error("Too deep structure by valueOf!",context);
				context->depth=0;
			} else
			{
				promenna=llookup("valueOf",context->namespace,(plns*)buffer->argument,context);
				context->depth++;
				retval=vartoint(promenna,context);
				context->depth=0;
			}
			delarg(buffer,context);
			buffer=js_mem_alloc(sizeof(abuf));
			buffer->typ=UNDEFINED;
			buffer->argument=0;
		break;
		default:	my_internal("Aiee! Unknown conversion!\n",context);
				return 0; /* aspon uhneme freeovani */
		break;
	}
	js_mem_free(buffer);
	return retval;
}

char* tostring(abuf*bafr,js_context*context)
{	char*a=0;
	abuf*b1;
	lns*promenna;
	RESOLV(bafr)
	switch(bafr->typ)
	{	case UNDEFINED:
			a=stracpy1("undefined");
		break;
		case NULLOVY:
			a=stracpy1("null");
		break;
		case INTEGER:
			a=js_mem_alloc(DELKACISLA);
			snprintf(a,DELKACISLA,"%d",(int)bafr->argument);
			a[DELKACISLA-1]='\0';
		break;
		case FLOAT:
			a=js_mem_alloc(DELKACISLA);
			if(*(double*)bafr->argument==MY_NAN)
				strcpy(a,"NaN");
			else	if(*(double*)bafr->argument==MY_INFINITY)
					strcpy(a,"Infinity");
				else	if(*(double*)bafr->argument==MY_MININFINITY)
						strcpy(a,"-Infinity");
					else	snprintf(a,DELKACISLA,"%g",*(float*)bafr->argument);
			js_mem_free((void*)bafr->argument);
			a[DELKACISLA-1]='\0';
/*			if(!strcmp(a,"nan"))
				strcpy(a,"NaN");
			else
				if(!strcmp(a,"inf"))
					strcpy(a,"Infinity");
				else
					if(!strcmp(a,"-inf"))
						strcpy(a,"-Infinity");*/
			if(!strncmp(a,"-0",3))
			{	js_mem_free(a);
				a=stracpy1("0");
			}
		break;
		case BOOLEAN:
			a=js_mem_alloc(strlen("false")+1);/* false je delsi B-) */
			if(bafr->argument)strcpy(a,"true"); else strcpy (a,"false");
		break;
		case STRING:
			a=(char*)bafr->argument;
		break;
		case FUNKCE:
			a=stracpy1("function f(){ alert('Not implemented yet!'); }");
		break;
		case FUNKINT:
			a=stracpy1("function f(){ [native code] }");
		break;
		case ADDRSPACE:
		case ADDRSPACEP:
		case ARRAY:
			if(context->depth>MAX_FCALL_DEPTH)
			{	if(!options_get_bool("js_all_conversions"))
					js_error("Too deep structure by valueOf!",context);
				context->depth=0;
			} else
			{
				promenna=llookup("toString",context->namespace,(plns*)bafr->argument,context);
				if(promenna->type==UNDEFINED)
				{	promenna=llookup("valueOf",context->namespace,(plns*)bafr->argument,context);
					if(promenna->type==STRING)
					{	a=js_mem_alloc(strlen((char*)promenna->value)+1);
						strcpy(a,(char*)promenna->value);
					} else
					{	a=js_mem_alloc(strlen("[object Object]")+1);
						strcpy(a,"[object Object]");
					}
				} else
				{	/* toString exists */
					context->depth++;
					b1=js_mem_alloc(sizeof(abuf));
					b1->typ=VARIABLE;/* Nosime drivi do lesa, ale ono se to prezije... :-) */
					b1->argument=(long)promenna;
					a=tostring(b1,context);
					context->depth=0;
				}
			}
			delarg(bafr,context);
			bafr=js_mem_alloc(sizeof(abuf));
			bafr->typ=UNDEFINED;
			bafr->argument=0;
		break;
		default:
			my_internal("Tato konverze jeste neni hotova",context);
			a=stracpy1("null");
			return a; /* aspon nebudeme freeovat */
		break;
	}
	js_mem_free(bafr);
	return a;
}


float tofloat(abuf*arg,js_context*context)
{	float vysl=0;
	lns*promenna;
	RESOLV(arg);
	switch(arg->typ)
	{	case UNDEFINED:
			if(!options_get_bool("js_all_conversions"))
				js_error("Undefined conversion!\n",context);
			vysl=1;
		break;
		case NULLOVY:
			vysl=(float)0;
		break;
		case INTEGER:
		case BOOLEAN:
			vysl=(float)(int)arg->argument;
		break;
		case FLOAT:
			vysl=*(float*)arg->argument;
			js_mem_free((void*)arg->argument);
		break;
		case STRING:
			if(!strcmp((char*)arg->argument,"NaN"))
				vysl=MY_NAN;
			else {	vysl=atof((char*)arg->argument);
				if(vysl>MY_MAXDOUBLE)
					vysl=MY_INFINITY;
				if(vysl<MY_MINDOUBLE)
					vysl=MY_MININFINITY;
			}
			js_mem_free((void*)arg->argument);
		break;
		case FUNKCE:
		case FUNKINT:
			if(!options_get_bool("js_all_conversions"))
				js_error("Conversion fction -> float not allowed!",context);
			vysl=MY_NAN;
		break;
		case ADDRSPACE:
		case ADDRSPACEP:
		case ARRAY:
			if(context->depth>MAX_FCALL_DEPTH)
			{	if(!options_get_bool("js_all_conversions"))
					js_error("Too deep structure by valueOf!",context);
				context->depth=0;
			} else
			{
				promenna=llookup("valueOf",context->namespace,(plns*)arg->argument,context);
				context->depth++;
				vysl=vartoint(promenna,context);
				context->depth=0;
			}
			delarg(arg,context);
			arg=js_mem_alloc(sizeof(abuf));
			arg->typ=UNDEFINED;
			arg->argument=0;
		break;
		default:
			my_internal("Tato konverze jeste neni hotova!\n",context);
			return 0;
		break;
	}
	js_mem_free(arg);
	return vysl;
}

int tobool(abuf*buffer,js_context*context)
{	int retval=0;
	RESOLV(buffer);
	switch(buffer->typ)
	{	case UNDEFINED:
			js_mem_free(buffer);
			retval=FALSE; /* Zbytecne, ale paranoia je paranoia! */
		break;
		case STRING:
			if(strlen((char*)buffer->argument))retval=TRUE; else retval=FALSE;
			js_mem_free((void*)buffer->argument);
			js_mem_free(buffer);
		break;
		default: retval=to32int(buffer,context);
	}
	return retval;
}


void moc(char*a)
{	printf("Prosel jsem moc argumentu ve funkci: %s\n",a);
	internal("exiting!");
}

/* Zde zacina jadro interpretu. Je realizovano funkcemi, do nichz se odskakuje
 * podle obsahu polozky opcode kazdeho uzlu v mezikodovem strome. Do kazde
 * funkce se typicky skace nekolikrat, funkce si ridi, co se ma vyhodnotit
 * (sve argumenty). Kdyz ma hotovo, nastavi sezvykan=1, jinak sezvykan=0.
 * Podle toho funkce ipret pozna, jestli se ma shanet po rodici soucasneho
 * vrcholu, nebo jestli se leze do syna. Je-li sezvykan==1 a na parent-bufferu
 * uz nic neni, interpretace uspesne konci. Konci-li interpretace neuspesne,
 * pozna se podle toho, ze funkce js_error nastavi jsem_dead=1 a na promennou
 * sezvykan se kasle.
 */

void bitand(js_context*context)
{	abuf*retval=0;/*paranoia*/
	debug("Vjizdim do funkce bitand ");
	switch(context->current->in)
	{	case 0: 
			debug("Zpracovavam prvni argument\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=(vrchol*)context->current->arg[0]; /* prolezeme cyklem podstrom sveho prvniho potomka */
			/* Neni treba prenastavovat stav z prolezeny na neprolezeny */
		break;
		case 1: 
			debug("Bitand ctu druhy argument\n");
			retval=pulla(context); /* Vyhodnocuji zleva doprava */
			RESOLV(retval);
			pusha(retval,context);
			context->current->in=2;
			pushp(context->current,context);
			context->current=(vrchol*)context->current->arg[1]; /* prolezeme cyklem podstrom sveho druheho potomka */
			sezvykan=0;
		break;
		case 2:
			debug("Pocitam bitand\n");
			retval=js_mem_alloc(sizeof(abuf));
			retval->typ=INTEGER;
			retval->argument=(to32int(pulla(context),context)&to32int(pulla(context),context));
	/* to32int bude advancovana technika, ktera vrati integer32 ze svince na bufferu.  */
			/* Prekonvertit do 32-bitovych integeru a spocitat &. Integer je cislo v desitkove soustave (neni-li receno jinak) */
			pusha(retval,context);
			sezvykan=1;
			context->current->in=0;
			context->current=pullp(context);
		break;
		default:
			moc("bitand");
		break;
	}
}

void logand(js_context*context)
{       abuf*retval=0;/*paranoia*/
        debug("Vjizdim do funkce logand ");
        switch(context->current->in)
        {       case 0:
        debug("Zpracovavam prvni argument\n");
                        context->current->in=1;
			pushp(context->current,context);
                        context->current=(vrchol*)context->current->arg[0]; /* prolezeme cyklem podstrom sveho prvniho potomka */
                        /* Neni treba prenastavovat stav z prolezeny na neprolezeny */
                break;
                case 1:
			if(!tobool(pulla(context),context))
			{	/* Est post aves */ 
				debug("logand prvni arg neplati! "); 
				goto false;
			}
        		debug("ctu druhy argument\n");
                        context->current->in=2;
			pushp(context->current,context);
                        context->current=(vrchol*)context->current->arg[1]; /* prolezeme cyklem podstrom sveho druheho potomka */
                        sezvykan=0;
                break;
                case 2:
			if(tobool(pulla(context),context)){
				retval=js_mem_alloc(sizeof(abuf));
				debug("vracim true!\n");
				retval->argument=TRUE;
				goto basso_continuo;
			}
			else {
false:
                        	retval=js_mem_alloc(sizeof(abuf));
				retval->argument=FALSE;
				debug("vracim false!\n");
basso_continuo:                 retval->typ=BOOLEAN;
                        	pusha(retval,context);
                        	sezvykan=1;
                        	context->current->in=0;
                        	context->current=pullp(context);
			}
                break;
                default:
                        moc("logand");
                break;
        }
}


void andassign(js_context*context)
{	abuf*retval=0;
	abuf*par1;
	lns* pna;
	long pna1;
	debug("Vjizdim do funkce andassign\n");
	switch(context->current->in)
	{	case 0:	debug("left-hand-side\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	debug("ctu 1. arg... ");
			sezvykan=0;
			par1=pulla(context);
			VARTEST(par1,"You can assign only to variable!\n");
			pusha(par1,context);
			pna=(lns*)par1->argument;
			pna1=vartoint(pna,context);/*Norma chce vyhodnocovat zleva doprava, tak nech sa paci...*/
			par1=js_mem_alloc(sizeof(abuf));
			par1->typ=INTEGER;
			par1->argument=pna1;
			pusha(par1,context);
			debug("right-hand-side... \n");
			context->current->in=2;
			pushp(context->current,context);
			context->current=context->current->arg[1];
		break;
		case 2:	debug("pocitam... \n");
			context->current->in=0;
			retval=js_mem_alloc(sizeof(abuf));
			retval->typ=INTEGER;
			retval->argument=to32int(pulla(context),context)&to32int(pulla(context),context);
			par1=pulla(context);/*Tady je klic promenne*/
			pna=(lns*)par1->argument;
			pusha(retval,context);
			js_mem_free(par1);
			if(pna->type==INTVAR)
				set_var_value(pna,INTEGER,retval->argument,context);
			else
			{
				clearvar(pna,context);
				pna->type=INTEGER;
				pna->value=retval->argument;
			}
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("andassign");
		break;
	}
}




void delete(js_context*context) 
{	abuf*objekt;
	lns*variable;
	plns*pom;
	debug("Vjizdim do operatoru delete ");
	switch(context->current->in)
	{	case 0:	debug("zjistuji koho\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	debug("nicim ho...\n");
			context->current->in=0;
			objekt=pulla(context);
			VARTEST(objekt,"Delete na nevariabli!\n");
			variable=(lns*)objekt->argument;
			if(variable->type!=ADDRSPACEP && variable->type!=ARRAY)
			{	delarg(objekt,context); /* variable leakovat nebude */
				js_error("Deleting not an objekt!\n",context);
				return;
			}
			if(!variable->value)
			{	my_internal("Deleting null object!",context);
				objekt->typ=UNDEFINED;
				objekt->argument=0;
				sezvykan=1;
				pusha(objekt,context);
				context->current=pullp(context);
				return;
			}
			if(((plns*)variable->value)->mid!=0)
			{	delarg(objekt,context);
				js_error("Trying to unalloc internal structs!\n",context);
				return;
			}
			pom=context->lnamespace;
			while(pom)
			{	if(pom==(plns*)variable->value)
				{	js_error("Trying to delete parent obj!\n",context);
					delarg(objekt,context);
					return;
				}
				pom=pom->next;
			}
			deletenamespace((plns*)variable->value,context);/* clean contain */
			variable->value=0;
			variable->type=UNDEFINED; /*undefine it*/
			objekt->typ=UNDEFINED; /* to argbuf put void */
			objekt->argument=0;
			pusha(objekt,context);
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("delete");
		break;
	}
}	

void divassign(js_context*context)
{       abuf*retval=0;
	abuf*par1;
        lns* pna;
	int pna2;
	float pna1,pna3;
        debug("Vjizdim do funkce divassign\n");
        switch(context->current->in)
	{	case 0: debug("left-hand-side\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	debug("ctu 1. arg... ");
			sezvykan=0;
			par1=pulla(context);
			VARTEST(par1,"You can assign only to variable!\n");
			pusha(par1,context);
			pna=(lns*)par1->argument;
			pna1=vartofloat(pna,context);/*Norma chce vyhodnocovat zleva doprava, tak nech sa paci...*/
			par1=js_mem_alloc(sizeof(abuf));
			par1->typ=FLOAT;
			par1->argument=(long)js_mem_alloc(sizeof(float));
			*(float*)par1->argument=pna1;
			pusha(par1,context);
			debug("right-hand-side... \n");
			context->current->in=2;
			pushp(context->current,context);
			context->current=context->current->arg[1];
		break;
		case 2: debug("pocitam... \n");
			context->current->in=0;
			retval=js_mem_alloc(sizeof(abuf));
			retval->typ=FLOAT;
			retval->argument=(long)js_mem_alloc(sizeof(float));
			pna3=tofloat(pulla(context),context);
			pna1=tofloat(pulla(context),context);
			*(float*)retval->argument=pna1=mojenas(pna1,mojeinv(pna3));
			par1=pulla(context);/*Tady je klic promenne*/
			pna=(lns*)par1->argument;
			pusha(retval,context);
			js_mem_free(par1);
			if(pna->type==INTVAR) set_var_value(pna,FLOAT,retval->argument,context);
			else
			{	clearvar(pna,context);

				pna->type=FLOAT;
				pna->value=(long)js_mem_alloc(sizeof(float));
				*(float*)pna->value=*(float*)retval->argument;
				if(((float)(pna2=(int)rint(pna1)))==pna1)
				{	pna->type=INTEGER;
					js_mem_free((void*)pna->value);
					pna->value=pna2;
					retval->typ=INTEGER;
					js_mem_free((void*)retval->argument);
					retval->argument=pna2;
					debug("je to cele!\n");
				}
			}
			sezvykan=1;
			context->current=pullp(context);
break;
		default:
			moc("divassign");
		break;
	}
}

void assign(js_context*context)/*Mel 'sem to pojmenovat Assasin*/
{	abuf* kam,*rightside;
/*	int i; */
	lns* vysl,*pom;
	plns* pomns; 
	debug("Vjizdim do fce assign ");
	switch(context->current->in)
	{	case 0:	debug("left-hand-side\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	debug("right-hand-side...\n");
			sezvykan=0;
			context->current->in=2;
			pushp(context->current,context);
			context->current=context->current->arg[1];
		break;
		case 2:	debug("prirazuji...\n");
			context->current->in=0;
			rightside=pulla(context);
			RESOLV(rightside);
			kam=pulla(context);
			pusha(rightside,context);
			VARTEST(kam,"You can assign only to variable!\n");
			vysl=(lns*)kam->argument;
			if(vysl->type==INTVAR)
				set_var_value(vysl,rightside->typ,rightside->argument,context);
			else
			{	if(rightside->typ==ADDRSPACE||rightside->typ==ADDRSPACEP||rightside->typ==ARRAY)
				{	pom=lookup(MIN1KEY,(plns*)rightside->argument,context);
					if(pom->type!=PARLIST)
						my_internal("Parentlist corrupted!\n",context);
					add_to_parlist(&fotr_je_lotr,pom);
				
					clearvar(vysl,context);
					delete_from_parlist(&fotr_je_lotr,pom);
				}else	clearvar(vysl,context);
				switch(vysl->type=rightside->typ)
				{	case UNDEFINED:
					case NULLOVY:
					case BOOLEAN:
					case INTEGER:
						vysl->value=rightside->argument;
					break;
					case FLOAT:
						vysl->value=(long)js_mem_alloc(sizeof(float));
						*(float*)vysl->value=*(float*)rightside->argument;
					break;
					case STRING:
						vysl->value=(long)js_mem_alloc(strlen((char*)rightside->argument)+1);
						strcpy((char*)vysl->value,(char*)rightside->argument);
					break;
					case ADDRSPACE:
						vysl->type=ADDRSPACEP;
					case ARRAY:
					case ADDRSPACEP:
						pomns=context->lnamespace->next;
						while(pomns && pomns!=((plns*)rightside->argument)->next)
							pomns=pomns->next;
						vysl->value=rightside->argument;
						if(!pomns)
							((plns*)rightside->argument)->next=0;
						pom=lookup(MIN1KEY,(plns*)rightside->argument,context);
						if(pom->type!=PARLIST)
						{	my_internal("Parent list corrupted!!!!\n",context); 
							sezvykan=1;
							context->current=pullp(context);
							return;
						}
						add_to_parlist(vysl,pom);
					break;
					case FUNKCE:
						vysl->value=rightside->argument;
					break;
					default:
						my_internal("Internal: Unknown types assigned!\n",context);
					break;

				}
			}
			js_mem_free(kam);
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("assign");
		break;
	}
}

void localassign(js_context*context)
{	abuf* kam,*rightside;
/*	int i;*/
	lns* vysl,*pom;
	plns* pomns;
	debug("Vjizdim do fce localassign ");
	switch(context->current->in)
	{	case 0:	debug("left-hand-side\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	debug("right-hand-side...\n");
			sezvykan=0;
			context->current->in=2;
			pushp(context->current,context);
			context->current=context->current->arg[1];
		break;
		case 2:	debug("prirazuji...\n");
			context->current->in=0;
			rightside=pulla(context);
			RESOLV(rightside);
			kam=pulla(context);
			pusha(rightside,context);
			VARTEST(kam,"You can assign only to variable!\n");
			vysl=(lns*)kam->argument;

			vysl=create(vysl->identifier,context->lnamespace,context);
			if(vysl->type==INTVAR)
				set_var_value(vysl,rightside->typ,rightside->argument,context);
			else
			{
/*				clearvar(vysl,context); */
				switch(vysl->type=rightside->typ)
				{	case UNDEFINED:
					case NULLOVY:
					case BOOLEAN:
					case INTEGER:
						vysl->value=rightside->argument;
					break;
					case FLOAT:
						vysl->value=(long)js_mem_alloc(sizeof(float));
						*(float*)vysl->value=*(float*)rightside->argument;
					break;
					case STRING:
						vysl->value=(long)js_mem_alloc(strlen((char*)rightside->argument)+1);
						strcpy((char*)vysl->value,(char*)rightside->argument);
					break;
					case ADDRSPACE:
						vysl->type=ADDRSPACEP;
					case ARRAY:
					case ADDRSPACEP:
						vysl->value=(long)rightside->argument;
						pomns=context->lnamespace->next;
						while(pomns && pomns!=((plns*)rightside->argument)->next)
							pomns=pomns->next;
						if(!pomns)
							((plns*)rightside->argument)->next=0;
						pom=lookup(MIN1KEY,(plns*)rightside->argument,context);
						if(pom->type!=PARLIST)
						{	my_internal("Internal: Parentlist corrupted!!\n",context);
							sezvykan=1; context->current=pullp(context);
						}
						add_to_parlist(vysl,pom);
					break;
					case FUNKCE:
						vysl->value=rightside->argument;
					break;
					default:
						js_mem_free(kam); /* Pokus o konsolidaci*/
						my_internal("Internal: Unknown type turned up!\n",context);
					break;
				}
			}
			js_mem_free(kam);
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("localassign");
		break;
	}
}


void equal(js_context*context)
{	abuf*retval=0;
	abuf*par1,*par2;
	debug("Vjizdim do fce equal ");
	switch(context->current->in)
	{	case 0:
			debug("prvni argument\n");
			context->current->in=1; /* Kdo napovida!? */
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:
			debug("druhy argument\n");
			sezvykan=0;
			par1=pulla(context); /* Vyhodnocuji zleva doprava */
			RESOLV(par1); 
			pusha(par1,context);
			context->current->in=2; /* Odkud budete napovidat!? */
			pushp(context->current,context);
			context->current=context->current->arg[1];
		break;
		case 2:
			debug("pocitam; ");
			context->current->in=0;
			retval=js_mem_alloc(sizeof(abuf));
			retval->typ=BOOLEAN;
			par2=pulla(context);
			par1=pulla(context);
			RESOLV(par2);
/*			RESOLV(par1);*/
			if((par1->typ==STRING) &&(par2->typ==STRING)){/*Jeste neni uplne koser!*/
				retval->argument=!strcmp((char*)par1->argument,(char*)par2->argument);
				debug("jsou to stringy!\n");
				js_mem_free((void*)par1->argument);
				js_mem_free((void*)par2->argument);
				js_mem_free(par1);
				js_mem_free(par2);
			} else
			{	retval->argument=(tofloat(par1,context)==tofloat(par2,context)); /* Ja to slysim z obou stran prakticky stejne, takze 
		    hazejte mi to z teto strany. */
				debug("jsou to cisla!\n");
			}
			sezvykan=1;
			pusha(retval,context);
			context->current=pullp(context);
		break;
		default:
			moc("equal");
		break;
	}
}

void not(js_context*context)
{	abuf*retval;
	debug("Vjizdim do fce not ");
	switch(context->current->in)
	{	case 0:	debug("zjistuji argument\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	debug("pocitam!\n");
			retval=js_mem_alloc(sizeof(abuf));
			retval->typ=BOOLEAN;
			retval->argument=!tobool(pulla(context),context);
			pusha(retval,context);
			context->current->in=0;
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("not");
		break;
	}
} 

void notassign(js_context*context)
{       abuf*retval=0;
        abuf*par1,*par2;
        debug("Vjizdim do fce notassign ");
        switch(context->current->in)
        {       case 0:
                        debug("prvni argument\n");
                        context->current->in=1;
                        pushp(context->current,context);
                        context->current=context->current->arg[0];
                break;
                case 1:
                        debug("druhy argument\n");
			sezvykan=0;
			par1=pulla(context); /* Vyhodnocuji zleva doprava */
			RESOLV(par1);
			pusha(par1,context);
                        context->current->in=2;
                        pushp(context->current,context);
                        context->current=context->current->arg[1];
                break;
                case 2:
                        debug("pocitam; ");
                        context->current->in=0;
                        retval=js_mem_alloc(sizeof(abuf));
                        retval->typ=BOOLEAN;
                        par2=pulla(context);
                        par1=pulla(context);
			RESOLV(par2);
/*			RESOLV(par1);*/
			if((par1->typ==STRING) &&(par2->typ==STRING)){/*Jeste neni uplne koser!*/
                                retval->argument=strcmp((char*)par1->argument,(char*)par2->argument);
                                debug("jsou to stringy!\n");
                                js_mem_free((void*)par1->argument);
                                js_mem_free((void*)par2->argument);
                                js_mem_free(par1);
                                js_mem_free(par2);
                        } else
                        {       retval->argument=(tofloat(par1,context)!=tofloat(par2,context));
                                debug("jsou to cisla!\n");
                        }
                        sezvykan=1;
                        pusha(retval,context);
                        context->current=pullp(context);
                break;
                default:
                        moc("notassign");
                break;
        }

}

void false(js_context*context)
{	abuf*retval=js_mem_alloc(sizeof(abuf));
	debug("Vracim FALSE literal\n");
	retval->typ=BOOLEAN;
	retval->argument=FALSE;
	pusha(retval,context);
	context->current=pullp(context);
	sezvykan=1;
}
	
void myif(js_context*context)
{	switch(context->current->in)
	{	case 0:	debug("Spoustim vyhodnoceni podminky\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	context->current->in=0;
			if(tobool(pulla(context),context)) {debug("If: Je to pravda!\n");
				context->current=context->current->arg[1];
			} else
			{	debug("If: Je to lez jako vez!\n");
				context->current=context->current->arg[2];
			}
		break;
		default:
			moc("myif");
		break;
	}
}

void unassign(js_context*context)
{	abuf* par1,*par2,*retval;
	float pna1=0,pna3;
	int pna2;
	lns*  pna;
	debug("Vjizdim do fce unassign ");
	switch(context->current->in)
	{	case 0:	debug("prvni argument\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	sezvykan=0;
			debug("ctu prvni arg; ");
			par1=pulla(context);
			VARTEST(par1,"You can assign only to variable!\n");
			pusha(par1,context);
			pna=(lns*)par1->argument;

			par1=js_mem_alloc(sizeof(abuf));
			if(pna->type==INTEGER)
			{	par1->argument=pna->value;
				par1->typ=INTEGER;

			} else{	par1->typ=FLOAT;
				par1->argument=(long)js_mem_alloc(sizeof(float));
				*(float*)par1->argument=vartofloat(pna,context);
			}
			pusha(par1,context);
			debug("right-hand-side... \n");
			context->current->in=2;
			pushp(context->current,context);
			context->current=context->current->arg[1];
		break;
		case 2: debug("pocitam... \n");
			context->current->in=0;
			retval=js_mem_alloc(sizeof(abuf));
			par2=pulla(context);/*second argument*/
			par1=pulla(context);/*first argument*/
			if((par1->typ==INTEGER) &&(par2->typ==INTEGER))
			{	retval->typ=INTEGER;
				retval->argument=par1->argument-par2->argument;
				js_mem_free(par1);
				js_mem_free(par2);
			}
			else{	retval->typ=FLOAT;
				retval->argument=(long)js_mem_alloc(sizeof(float));
				pna3=tofloat(par2,context);
				pna1=tofloat(par1,context);
				if(pna3>-MY_MINDOUBLE)
					pna3=MY_MININFINITY;
				*(float*)retval->argument=pna1=mojescit(pna1,-pna3);
			}
			par1=pulla(context);/*Tady je klic promenne*/
			pna=(lns*)par1->argument;
			pusha(retval,context);
			js_mem_free(par1);
			if(pna->type==INTVAR)
				set_var_value(pna,retval->typ,retval->argument,context);
			else {
				clearvar(pna,context);
				pna->type=retval->typ;
				if(retval->typ==FLOAT){
					pna->value=(long)js_mem_alloc(sizeof(float));
					*(float*)pna->value=*(float*)retval->argument;
					if(((float)(pna2=(int)rint(pna1)))==pna1)
					{       pna->type=INTEGER;
						js_mem_free((void*)pna->value);
						pna->value=pna2;
						retval->typ=INTEGER;
						js_mem_free((void*)retval->argument);
						retval->argument=pna2;
						debug("je to cele!\n");
					}
				} else{	
					pna->value=retval->argument;
					debug("bylo to cele...\n");
				}
			}
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("unassign");
		break;
	}		
}

void decpref(js_context*context)
{	abuf*par;
	float par1;
	int pna2;
	lns* pna;
	debug("Vjizdim do fce decpref ");
	switch(context->current->in)
	{	case 0:	debug("zjistuji argument\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	debug("pocitam!\n");
			par=pulla(context);
			VARTEST(par,"You can decrement only variables!\n");
			pna=(lns*)par->argument;
			pusha(par,context);
			if(pna->type==INTEGER)
			{	pna->value--;
				par->typ=INTEGER;
				par->argument=pna->value;
			}else {	
				par->typ=FLOAT;
				par->argument=(long)js_mem_alloc(sizeof(float));
				par1=vartofloat(pna,context);
				if(par1!=MY_NAN && par1!=MY_INFINITY && par1!=MY_MININFINITY)
					par1=par1+1;
				if(par1>MY_MAXDOUBLE)
					par1=MY_INFINITY;
				if(par1<MY_MINDOUBLE)
					par1=MY_MININFINITY;
				*(float*)par->argument=par1;
				if(pna->type==INTVAR)
				{	set_var_value(pna,FLOAT,(long)&par1,context);
				} else
				{	clearvar(pna,context);
					pna->type=FLOAT;
					pna->value=(long)js_mem_alloc(sizeof(float));
					*(float*)pna->value=par1;
					if(((float)(pna2=(int)rint(par1)))==par1)
					{	pna->type=INTEGER;
						js_mem_free((void*)pna->value);
						pna->value=pna2;
						par->typ=INTEGER;
						js_mem_free((void*)par->argument);
						par->argument=pna2;
						debug("je to cele!\n");
					}
				}
			}
			context->current->in=0;
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("decpref");
		break;
	}
}

void minus(js_context*context) 
{	abuf*retval=0;
        abuf*par1,*par2;
        debug("Vjizdim do fce minus ");
        switch(context->current->in)
        {       case 0:
                        debug("prvni argument\n");
                        context->current->in=1;
                        pushp(context->current,context);
                        context->current=context->current->arg[0];
                break;
                case 1:
                        debug("druhy argument\n");
			sezvykan=0;
			par1=pulla(context); /* Vyhodnocuji zleva doprava */
			RESOLV(par1);
			pusha(par1,context);
                        context->current->in=2;
                        pushp(context->current,context);
                        context->current=context->current->arg[1];
                break;
                case 2:
                        debug("pocitam; ");
                        context->current->in=0;
                        retval=js_mem_alloc(sizeof(abuf));
			par2=pulla(context);
			RESOLV(par2)				
			par1=pulla(context);
/*			RESOLV(par1);*/
			if((par2->typ==INTEGER) && (par1->typ==INTEGER))
			{	retval->typ=INTEGER;
				retval->argument=((int)par1->argument-(int)par2->argument);
#ifdef RASODEBUG
				printf("je to %d.\n",(int)retval->argument);
#endif
				js_mem_free(par1);
				js_mem_free(par2);
			}
			else{
				retval->typ=FLOAT;
				retval->argument=(long)js_mem_alloc(sizeof(float));
				*(float*)retval->argument=mojescit(tofloat(par1,context),-tofloat(par2,context));
#ifdef RASODEBUG
				printf("je to %g.\n",*(float*)retval->argument);
#endif
			}
			sezvykan=1;
			pusha(retval,context);
			context->current=pullp(context);
		break;
		default: moc("minus");
		break;
	}
}

void modulo(js_context*context)
{	abuf*retval=0;
        abuf*par1,*par2;
        debug("Vjizdim do fce modulo ");
        switch(context->current->in)
        {       case 0:
                        debug("prvni argument\n");
                        context->current->in=1;
                        pushp(context->current,context);
                        context->current=context->current->arg[0];
                break;
                case 1:
                        debug("druhy argument\n");
			sezvykan=0;
			retval=pulla(context); /* Vyhodnocuji zleva doprava */
			RESOLV(retval);
			pusha(retval,context);
                        context->current->in=2;
                        pushp(context->current,context);
                        context->current=context->current->arg[1];
                break;
                case 2:
                        debug("pocitam; ");
                        context->current->in=0;
                        retval=js_mem_alloc(sizeof(abuf));
                        par2=pulla(context);
                        RESOLV(par2);
                        par1=pulla(context);
/*			RESOLV(par1);*/
                        if((par2->typ==INTEGER) && (par1->typ==INTEGER))
                        {       retval->typ=INTEGER;
                                retval->argument=((int)par1->argument%(int)par2->argument);
#ifdef RASODEBUG
                                printf("je to %d.\n",(int)retval->argument);
#endif
                                js_mem_free(par1);
                                js_mem_free(par2);
                        }
                        else{
                                retval->typ=FLOAT;
                                retval->argument=(long)js_mem_alloc(sizeof(float));
                                *(float*)retval->argument=fmod(tofloat(par1,context),tofloat(par2,context));
#ifdef RASODEBUG
                                printf("je to %g.\n",*(float*)retval->argument);
#endif
                        }
                        sezvykan=1;
                        pusha(retval,context);
                        context->current=pullp(context);
                break;
                default: moc("modulo");
                break;
        }
}

void modassign(js_context*context) 
{	abuf* par1,*par2,*retval;
	float pna1=0,pna3;
	int pna2;
	lns*  pna;
	debug("Vjizdim do fce modassign ");
	switch(context->current->in)
	{	case 0:	debug("prvni argument\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	sezvykan=0;
			debug("ctu prvni arg; ");
			par1=pulla(context);
			VARTEST(par1,"You can assign only to variable!\n");
			pusha(par1,context);
			pna=(lns*)par1->argument;

			par1=js_mem_alloc(sizeof(abuf));
			if(pna->type==INTEGER)
			{	par1->argument=pna->value;
				par1->typ=INTEGER;

			} else{	par1->typ=FLOAT;
				par1->argument=(long)js_mem_alloc(sizeof(float));
				*(float*)par1->argument=vartofloat(pna,context);
			}
			pusha(par1,context);
			debug("right-hand-side... \n");
			context->current->in=2;
			pushp(context->current,context);
			context->current=context->current->arg[1];
		break;
		case 2: debug("pocitam... \n");
			context->current->in=0;
			retval=js_mem_alloc(sizeof(abuf));
			par2=pulla(context);/*second argument*/
			RESOLV(par2);
			par1=pulla(context);/*first argument*/
			RESOLV(par1);
			if((par1->typ==INTEGER) &&(par2->typ==INTEGER))
			{	retval->typ=INTEGER;
				retval->argument=par1->argument%par2->argument;
				js_mem_free(par1);
				js_mem_free(par2);
			}
			else{	retval->typ=FLOAT;
				retval->argument=(long)js_mem_alloc(sizeof(float));
				pna3=tofloat(par2,context);
				pna1=tofloat(par1,context);
				*(float*)retval->argument=pna1=fmod(pna1,pna3);
			}
			par1=pulla(context);/*Tady je klic promenne*/
			pna=(lns*)par1->argument;
			pusha(retval,context);
			js_mem_free(par1);
			if(pna->type==INTVAR)
				set_var_value(pna,retval->typ,retval->argument,context);
			else
			{	clearvar(pna,context);
				pna->type=retval->typ;
				if(retval->typ==FLOAT){
					pna->value=(long)js_mem_alloc(sizeof(float));
					*(float*)pna->value=*(float*)retval->argument;
					if(((float)(pna2=(int)rint(pna1)))==pna1)
					{       pna->type=INTEGER;
						js_mem_free((void*)pna->value);
						pna->value=pna2;
						retval->typ=INTEGER;
						js_mem_free((void*)retval->argument);
						retval->argument=pna2;
						debug("je to cele!\n");
					}
				} else{	
					pna->value=retval->argument;
					debug("bylo to cele...\n");
				}
			}
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("modassign");
		break;
	}		
}

void mynull(js_context*context)
{       abuf*retval=js_mem_alloc(sizeof(abuf));
        debug("Vracim NULLOVY literal\n");
        retval->typ=NULLOVY;
        retval->argument=0;/*bezpecnostni konstanta -- nehraje roli*/
        pusha(retval,context);
        context->current=pullp(context);
        sezvykan=1;
}

void number(js_context*context)
{       abuf*retval=js_mem_alloc(sizeof(abuf));
	cislo*mynum;
        debug("Vracim numericky literal ");
	
	switch((mynum=(cislo*)context->current->arg[0])->typ)
	{	case CELE:
		debug("celocis.\n");
			retval->typ=INTEGER;
			retval->argument=mynum->nr.cele;
		break;
		case NECELE:
		debug("floatovy\n");
			retval->typ=FLOAT;
			retval->argument=(long)js_mem_alloc(sizeof(float));
			*(float*)retval->argument=mynum->nr.necele;
		break;
		default: my_internal("Bug! Cislo neni ani cele, ani necele!\n",context);
			retval->typ=INTEGER;
			retval->argument=0;
		break;
	}
	pusha(retval,context);
        context->current=pullp(context);
        sezvykan=1;
}

void bitor(js_context*context)
{       abuf*retval=0;/*paranoia*/
        debug("Vjizdim do funkce bitor ");
        switch(context->current->in)
        {       case 0:
		        debug("Zpracovavam prvni argument\n");
                        context->current->in=1;
			pushp(context->current,context);
                        context->current=(vrchol*)context->current->arg[0]; /* prolezeme cyklem podstrom sveho prvniho potomka */
                        /* Neni treba prenastavovat stav z prolezeny na neprolezeny */
                break;
                case 1:
		        debug("ctu druhy argument\n");
                        context->current->in=2;
			pushp(context->current,context);
                        context->current=(vrchol*)context->current->arg[1]; /* prolezeme cyklem podstrom sveho druheho potomka */
                        sezvykan=0;
                break;
                case 2:
			debug("Pocitam bitor\n");
                        retval=js_mem_alloc(sizeof(abuf));
                        retval->typ=INTEGER;
                        retval->argument=(to32int(pulla(context),context)|to32int(pulla(context),context));
                        /* Prekonvertit do 32-bitovych integeru a spocitat |. Integer je cislo v desitkove soustave (neni-li receno jinak) */
                        pusha(retval,context);
                        sezvykan=1;
                        context->current->in=0;
                        context->current=pullp(context);
                break;
                default:
                        moc("bitor");
                break;
        }
}

void orassign(js_context*context) 
{	abuf*retval=0;
	abuf*par1;
	lns* pna;
	long pna1;
	debug("Vjizdim do funkce orassign\n");
	switch(context->current->in)
	{	case 0:	debug("left-hand-side\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	debug("ctu 1. arg... ");
			sezvykan=0;
			par1=pulla(context);
			VARTEST(par1,"You can assign only to variable!\n");
			pusha(par1,context);
			pna=(lns*)par1->argument;
			pna1=vartoint(pna,context);/*Norma chce vyhodnocovat zleva doprava, tak nech sa paci...*/
			par1=js_mem_alloc(sizeof(abuf));
			par1->typ=INTEGER;
			par1->argument=pna1;
			pusha(par1,context);
			debug("right-hand-side... \n");
			context->current->in=2;
			pushp(context->current,context);
			context->current=context->current->arg[1];
		break;
		case 2:	debug("pocitam... \n");
			context->current->in=0;
			retval=js_mem_alloc(sizeof(abuf));
			retval->typ=INTEGER;
			retval->argument=to32int(pulla(context),context)|to32int(pulla(context),context);
			par1=pulla(context);/*Tady je klic promenne*/
			pna=(lns*)par1->argument;
			pusha(retval,context);
			js_mem_free(par1);
			clearvar(pna,context);
			if(pna->type==INTVAR)
				set_var_value(pna,INTEGER,retval->argument,context);
			else
			{	pna->type=INTEGER;
				pna->value=retval->argument;
			}
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("orassign");
		break;
	}
}

void logor(js_context*context)
{       abuf*retval=0;/*paranoia*/
        debug("Vjizdim do funkce logor ");
        switch(context->current->in)
        {       case 0:
		        debug("Zpracovavam prvni argument\n");
                        context->current->in=1;
			pushp(context->current,context);
                        context->current=(vrchol*)context->current->arg[0]; /* prolezeme cyklem podstrom sveho prvniho potomka */
                        /* Neni treba prenastavovat stav z prolezeny na neprolezeny */
                break;
                case 1:
                        if(tobool(pulla(context),context))
                        {       /* Est post aves */
                                debug("logor prvni arg plati! ");
                                goto true;
                        }
                        debug("ctu druhy argument\n");
                        context->current->in=2;
			pushp(context->current,context);
                        context->current=(vrchol*)context->current->arg[1]; /* prolezeme cyklem podstrom sveho druheho potomka */
                        sezvykan=0;
                break;
                case 2:
                        if(tobool(pulla(context),context)) {
true:                           retval=js_mem_alloc(sizeof(abuf));
                                debug("vracim true!\n");
                                retval->argument=TRUE;
                                goto basso_continuo;
			}
                        else {
                                retval=js_mem_alloc(sizeof(abuf));
                                retval->argument=FALSE;
                                debug("vracim false!\n");
basso_continuo:                 retval->typ=BOOLEAN;
                                pusha(retval,context);
                                sezvykan=1;
                                context->current->in=0;
                                context->current=pullp(context);
			}
                break;
                default:
                        moc("logor");
                break;
        }
}

void plus(js_context*context) 
{	abuf*retval=0;
        abuf*par1,*par2;
	char*str1,*str2;
        debug("Vjizdim do fce plus ");
        switch(context->current->in)
        {       case 0:
                        debug("prvni argument\n");
                        context->current->in=1;
                        pushp(context->current,context);
                        context->current=context->current->arg[0];
                break;
                case 1:
                        debug("druhy argument\n");
			sezvykan=0;
			retval=pulla(context); /* Vyhodnocuji zleva doprava */
			RESOLV(retval);
			pusha(retval,context);
                        context->current->in=2;
                        pushp(context->current,context);
                        context->current=context->current->arg[1];
                break;
                case 2:
                        debug("pocitam; ");
                        context->current->in=0;
                        retval=js_mem_alloc(sizeof(abuf));
			par2=pulla(context);
			RESOLV(par2);
			par1=pulla(context);
			RESOLV(par1);
 			if((par2->typ==STRING) || (par1->typ==STRING))
			{	retval->typ=STRING;
				retval->argument=(long)js_mem_alloc(1+strlen(str1=tostring(par1,context))+
					strlen(str2=tostring(par2,context)));
				strcpy((char*)retval->argument,str1);
				strcat((char*)retval->argument,str2);
#ifdef RASODEBUG
				printf("je to %s.\n",(char*)retval->argument);
#endif
				js_mem_free(str1);
				js_mem_free(str2);
			}else {
				if((par2->typ==INTEGER) && (par1->typ==INTEGER))
        	                {       retval->typ=INTEGER;
                	                retval->argument=((int)par1->argument+(int)par2->argument);
#ifdef RASODEBUG
                        	        printf("je to %d.\n",(int)retval->argument);
#endif
                                	js_mem_free(par1);
	                                js_mem_free(par2);
        	                }
                	        else{
	                                retval->typ=FLOAT;
        	                        retval->argument=(long)js_mem_alloc(sizeof(float));
                	                *(float*)retval->argument=mojescit(tofloat(par1,context),tofloat(par2,context));
#ifdef RASODEBUG
	                                printf("je to %g.\n",*(float*)retval->argument);
#endif
        	                }
			}
               	        sezvykan=1;
                        pusha(retval,context);
                       context->current=pullp(context);
                break;
                default: moc("plus");
                break;
        }
}

void plusassign(js_context*context) 
{	abuf* par1,*par2,*retval;
	char* str1,*str2;
	lns*  pna;
	debug("Vjizdim do fce plusassign ");
	switch(context->current->in)
	{	case 0:	debug("prvni argument\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	sezvykan=0;
			debug("ctu prvni arg; ");
			par1=pulla(context);
			VARTEST(par1,"You can assign only to variable\n");
			pusha(par1,context);
			pna=(lns*)par1->argument;

			par1=js_mem_alloc(sizeof(abuf));
			vartoarg(pna,par1,context);
			pusha(par1,context);
			debug("right-hand-side... \n");
			context->current->in=2;
			pushp(context->current,context);
			context->current=context->current->arg[1];
		break;
		case 2: debug("pocitam... \n");
			context->current->in=0;
			retval=js_mem_alloc(sizeof(abuf));
			par2=pulla(context);/*second argument*/
			RESOLV(par2);
			par1=pulla(context);/*first argument*/
			RESOLV(par1);
			
			if((par2->typ==STRING) || (par1->typ==STRING))
			{	retval->typ=STRING;
				retval->argument=(long)js_mem_alloc(1+strlen(str1=tostring(par1,context))+
					strlen(str2=tostring(par2,context)));
				strcpy((char*)retval->argument,str1);
				strcat((char*)retval->argument,str2);
#ifdef RASODEBUG
				printf("je to %s.\n",(char*)retval->argument);
#endif
				js_mem_free(str1);
				js_mem_free(str2);
			}else {
				if((par2->typ==INTEGER) && (par1->typ==INTEGER))
        	                {       retval->typ=INTEGER;
                	                retval->argument=((int)par1->argument+(int)par2->argument);
#ifdef RASODEBUG
                        	        printf("je to %d.\n",(int)retval->argument);
#endif
                                	js_mem_free(par1);
	                                js_mem_free(par2);
        	                }
                	        else{
	                                retval->typ=FLOAT;
        	                        retval->argument=(long)js_mem_alloc(sizeof(float));
                	                *(float*)retval->argument=mojescit(tofloat(par1,context),tofloat(par2,context));
#ifdef RASODEBUG
	                                printf("je to %g.\n",*(float*)retval->argument);
#endif
        	                }
			}
			
			par1=pulla(context);/*Tady je klic promenne*/
			pna=(lns*)par1->argument;
			pusha(retval,context);
			js_mem_free(par1);
			if(pna->type==INTVAR)
				set_var_value(pna,retval->typ,retval->argument,context);
			else {
				clearvar(pna,context);
				switch(pna->type=retval->typ)
				{	case STRING:
						pna->value=(long)js_mem_alloc(strlen((char*)retval->argument)+1);
						strcpy((char*)pna->value,(char*)retval->argument);
						
					break;
					case FLOAT:
						pna->value=(long)js_mem_alloc(sizeof(float));
						*(float*)pna->value=*(float*)retval->argument;
					break;
					case INTEGER:
						pna->value=retval->argument;
					break;
					default:
						my_internal("Internal: Unknown types in plusassign!\n",context);
						pna->type=INTEGER;
						pna->value=0;
					break;
				}
			}
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("plusassign");
		break;
	}		
}

void incpref(js_context*context) 
{	abuf*par;
	float par1;
	int pna2;
	lns* pna;
	debug("Vjizdim do fce incpref ");
	switch(context->current->in)
	{	case 0:	debug("zjistuji argument\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1: debug("pocitam!\n");
			par=pulla(context);
			VARTEST(par,"You can increment only variable!\n");
			pna=(lns*)par->argument;
			pusha(par,context);
			if(pna->type==INTEGER)
			{	pna->value++;
				par->typ=INTEGER;
				par->argument=pna->value;
			}else {
				par->typ=FLOAT;
				par->argument=(long)js_mem_alloc(sizeof(float));
				par1=vartofloat(pna,context);
				if(par1!=MY_NAN && par1!=MY_INFINITY && par1!=MY_MININFINITY)
					par1=par1+1;
				if(par1>MY_MAXDOUBLE)
					par1=MY_INFINITY;
				if(par1<MY_MINDOUBLE)
					par1=MY_MININFINITY;
				*(float*)par->argument=par1;
				if(pna->type==INTVAR)
				{	set_var_value(pna,FLOAT,(long)&par1,context);
				} else
				{
					clearvar(pna,context);
					pna->type=FLOAT;
					pna->value=(long)js_mem_alloc(sizeof(float));
					*(float*)pna->value=par1;
					if(((float)(pna2=(int)rint(par1)))==par1)
					{	pna->type=INTEGER;
						js_mem_free((void*)pna->value);
						pna->value=pna2;
						par->typ=INTEGER;
						js_mem_free((void*)par->argument);
						par->argument=pna2;
						debug("je to cele!\n");
					}
				}
			}
			context->current->in=0;
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("incpref");
		break;
	}
}

void lt(js_context*context)
{	abuf*retval=0;
        abuf*par1,*par2;
        debug("Vjizdim do fce lt ");
        switch(context->current->in)
        {       case 0:
                        debug("prvni argument\n");
                        context->current->in=1;
                        pushp(context->current,context);
                        context->current=context->current->arg[0];
                break;
                case 1:
                        debug("druhy argument\n");
			sezvykan=0;
			retval=pulla(context); /* Vyhodnocuji zleva doprava */
			RESOLV(retval);
			pusha(retval,context);
                        context->current->in=2;
                        pushp(context->current,context);
                        context->current=context->current->arg[1];
                break;
                case 2:
                        debug("pocitam; ");
                        context->current->in=0;
                        retval=js_mem_alloc(sizeof(abuf));
                        retval->typ=BOOLEAN;
                        par2=pulla(context);
                        RESOLV(par2);
                        par1=pulla(context);
                        RESOLV(par1);
                        if((par1->typ==STRING) &&(par2->typ==STRING)){/*Jeste neni uplne koser!*/
                                retval->argument=((strcmp((char*)par1->argument,(char*)par2->argument)<0)?1:0);
                                debug("jsou to stringy!\n");
                                js_mem_free((void*)par1->argument);
                                js_mem_free((void*)par2->argument);
                                js_mem_free(par1);
                                js_mem_free(par2);
                        } else
                        {       retval->argument=(tofloat(par1,context)<tofloat(par2,context));
                                debug("jsou to cisla!\n");
                        }
                        sezvykan=1;
                        pusha(retval,context);
                        context->current=pullp(context);
                break;
                default:
                        moc("lt");
                break;
        }
}

void le(js_context*context)
{	abuf*retval=0;
        abuf*par1,*par2;
        debug("Vjizdim do fce le ");
        switch(context->current->in)
        {       case 0:
                        debug("prvni argument\n");
                        context->current->in=1;
                        pushp(context->current,context);
                        context->current=context->current->arg[0];
                break;
                case 1:
                        debug("druhy argument\n");
			sezvykan=0;
			retval=pulla(context); /* Vyhodnocuji zleva doprava */
			RESOLV(retval);
			pusha(retval,context);
                        context->current->in=2;
                        pushp(context->current,context);
                        context->current=context->current->arg[1];
                break;
                case 2:
                        debug("pocitam; ");
                        context->current->in=0;
                        retval=js_mem_alloc(sizeof(abuf));
                        retval->typ=BOOLEAN;
                        par2=pulla(context);
                        RESOLV(par2);
                        par1=pulla(context);
                        RESOLV(par1);
                        if((par1->typ==STRING) &&(par2->typ==STRING)){/*Jeste neni uplne koser!*/
                                retval->argument=((strcmp((char*)par1->argument,(char*)par2->argument)<=0)?1:0);
                                debug("jsou to stringy!\n");
                                js_mem_free((void*)par1->argument);
                                js_mem_free((void*)par2->argument);
                                js_mem_free(par1);
                                js_mem_free(par2);
                        } else
                        {       retval->argument=(tofloat(par1,context)<=tofloat(par2,context));
                                debug("jsou to cisla!\n");
                        }
                        sezvykan=1;
                        pusha(retval,context);
                        context->current=pullp(context);
                break;
                default:
                        moc("le");
                break;
        }
}

void shl(js_context*context) /*1*/
	{	abuf*retval=0;/*paranoia*/
	unsigned int i,j;
	debug("Vjizdim do funkce shl ");
	switch(context->current->in)
	{	case 0: 
			debug("Zpracovavam prvni argument\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=(vrchol*)context->current->arg[0]; /* prolezeme cyklem podstrom sveho prvniho potomka */
			/* Neni treba prenastavovat stav z prolezeny na neprolezeny */
		break;
		case 1:	debug("ctu druhy argument\n");
			retval=pulla(context); /* Vyhodnocuji zleva doprava */
			RESOLV(retval);
			pusha(retval,context);
			context->current->in=2;
			pushp(context->current,context);
			context->current=(vrchol*)context->current->arg[1]; /* prolezeme cyklem podstrom sveho druheho potomka */
			sezvykan=0;
		break;
		case 2:	debug("pocitam...\n");
			retval=js_mem_alloc(sizeof(abuf));
			retval->typ=INTEGER;
			i=to32int(pulla(context),context);
			j=to32int(pulla(context),context);
			retval->argument=(long)(signed int)(j<<i);
			pusha(retval,context);
			sezvykan=1;
			context->current->in=0;
			context->current=pullp(context);
		break;
		default:
			moc("shl");
		break;
	}
	debug("Funkce shl je blbe!!!!!!!!!!!\n"); }

void shlshleq(js_context*context) /*2*/
{	abuf*retval=0;
	abuf*par1;
	lns* pna;
	unsigned int i,j;
	long pna1;
	debug("Vjizdim do funkce shlshleq\n");
	switch(context->current->in)
	{	case 0:	debug("left-hand-side\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	debug("ctu 1. arg... ");
			sezvykan=0;
			par1=pulla(context);
			VARTEST(par1,"You can assign only to variable!\n");
			pusha(par1,context);
			pna=(lns*)par1->argument;
			pna1=vartoint(pna,context);/*Norma chce vyhodnocovat zleva doprava, tak nech sa paci...*/
			par1=js_mem_alloc(sizeof(abuf));
			par1->typ=INTEGER;
			par1->argument=pna1;
			pusha(par1,context);
			debug("right-hand-side... \n");
			context->current->in=2;
			pushp(context->current,context);
			context->current=context->current->arg[1];
		break;
		case 2:	debug("pocitam... \n");
			context->current->in=0;
			i=to32int(pulla(context),context);
			j=to32int(pulla(context),context);
			retval=js_mem_alloc(sizeof(abuf));
			retval->typ=INTEGER;
			retval->argument=(long)(signed int)(j<<i);
			par1=pulla(context);/*Tady je klic promenne*/
			pna=(lns*)par1->argument;
			pusha(retval,context);
			js_mem_free(par1);
			if(pna->type==INTVAR)
				set_var_value(pna,INTEGER,retval->argument,context);
			else
			{	clearvar(pna,context);
				pna->type=INTEGER;
				pna->value=retval->argument;
			}
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("shlshleq");
		break;
	}
	debug("Funkce shlshleq je blbe!!!!!!!!!!!!!!!\b\n"); }

void gt(js_context*context) 
{	abuf*retval=0;
        abuf*par1,*par2;
        debug("Vjizdim do fce gt ");
        switch(context->current->in)
        {       case 0:
                        debug("prvni argument\n");
                        context->current->in=1;
                        pushp(context->current,context);
                        context->current=context->current->arg[0];
                break;
                case 1:
                        debug("druhy argument\n");
			sezvykan=0;
			retval=pulla(context); /* Vyhodnocuji zleva doprava */
			RESOLV(retval);
			pusha(retval,context);
                        context->current->in=2;
                        pushp(context->current,context);
                        context->current=context->current->arg[1];
                break;
                case 2:
                        debug("pocitam; ");
                        context->current->in=0;
                        retval=js_mem_alloc(sizeof(abuf));
                        retval->typ=BOOLEAN;
                        par2=pulla(context);
                        RESOLV(par2);
                        par1=pulla(context);
                        RESOLV(par1);
                        if((par1->typ==STRING) &&(par2->typ==STRING)){/*Jeste neni uplne koser!*/
                                retval->argument=((strcmp((char*)par1->argument,(char*)par2->argument)>0)?1:0);
                                debug("jsou to stringy!\n");
                                js_mem_free((void*)par1->argument);
                                js_mem_free((void*)par2->argument);
                                js_mem_free(par1);
                                js_mem_free(par2);
                        } else
                        {       retval->argument=(tofloat(par1,context)>tofloat(par2,context));
                                debug("jsou to cisla!\n");
                        }
                        sezvykan=1;
                        pusha(retval,context);
                        context->current=pullp(context);
                break;
                default:
                        moc("gt");
                break;
        }
}

void ge(js_context*context)
{	abuf*retval=0;
        abuf*par1,*par2;
        debug("Vjizdim do fce ge ");
        switch(context->current->in)
        {       case 0:
                        debug("prvni argument\n");
                        context->current->in=1;
                        pushp(context->current,context);
                        context->current=context->current->arg[0];
                break;
                case 1:
                        debug("druhy argument\n");
			sezvykan=0;
			retval=pulla(context); /* Vyhodnocuji zleva doprava */
			RESOLV(retval);
			pusha(retval,context);
                        context->current->in=2;
                        pushp(context->current,context);
                        context->current=context->current->arg[1];
                break;
                case 2:
                        debug("pocitam; ");
                        context->current->in=0;
                        retval=js_mem_alloc(sizeof(abuf));
                        retval->typ=BOOLEAN;
                        par2=pulla(context);
                        RESOLV(par2);
                        par1=pulla(context);
                        RESOLV(par1);
                        if((par1->typ==STRING) &&(par2->typ==STRING)){/*Jeste neni uplne koser!*/
                                retval->argument=((strcmp((char*)par1->argument,(char*)par2->argument)>=0)?1:0);
                                debug("jsou to stringy!\n");
                                js_mem_free((void*)par1->argument);
                                js_mem_free((void*)par2->argument);
                                js_mem_free(par1);
                                js_mem_free(par2);
                        } else
                        {       retval->argument=(tofloat(par1,context)>=tofloat(par2,context));
                                debug("jsou to cisla!\n");
                        }
                        sezvykan=1;
                        pusha(retval,context);
                        context->current=pullp(context);
                break;
                default:
                        moc("ge");
                break;
        }
}

void shr(js_context*context) /*1*/
{	abuf*retval=0;/*paranoia*/
	unsigned int i,j;
	debug("Vjizdim do funkce shl ");
	switch(context->current->in)
	{	case 0: 
			debug("Zpracovavam prvni argument\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=(vrchol*)context->current->arg[0]; /* prolezeme cyklem podstrom sveho prvniho potomka */
			/* Neni treba prenastavovat stav z prolezeny na neprolezeny */
		break;
		case 1:	debug("ctu druhy argument\n");
			retval=pulla(context); /* Vyhodnocuji zleva doprava */
			RESOLV(retval);
			pusha(retval,context);
			context->current->in=2;
			pushp(context->current,context);
			context->current=(vrchol*)context->current->arg[1]; /* prolezeme cyklem podstrom sveho druheho potomka */
			sezvykan=0;
		break;
		case 2:	debug("pocitam...\n");
			retval=js_mem_alloc(sizeof(abuf));
			retval->typ=INTEGER;
			i=to32int(pulla(context),context);
			j=to32int(pulla(context),context);
			retval->argument=(long)(signed int)(j>>i);
			pusha(retval,context);
			sezvykan=1;
			context->current->in=0;
			context->current=pullp(context);
		break;
		default:
			moc("shr");
		break;
	}
	debug("Funkce shr je blbe!!!!!!!!!!!!!!!!\b\n"); }

void shrshreq(js_context*context) /*2*/
{	abuf*retval=0;
	abuf*par1;
	lns* pna;
	unsigned int i,j;
	long pna1;
	debug("Vjizdim do funkce shrshreq\n");
	switch(context->current->in)
	{	case 0:	debug("left-hand-side\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	debug("ctu 1. arg... ");
			sezvykan=0;
			par1=pulla(context);
			VARTEST(par1,"You can assign only to variable!\n");
			pusha(par1,context);
			pna=(lns*)par1->argument;
			pna1=vartoint(pna,context);/*Norma chce vyhodnocovat zleva doprava, tak nech sa paci...*/
			par1=js_mem_alloc(sizeof(abuf));
			par1->typ=INTEGER;
			par1->argument=pna1;
			pusha(par1,context);
			debug("right-hand-side... \n");
			context->current->in=2;
			pushp(context->current,context);
			context->current=context->current->arg[1];
		break;
		case 2:	debug("pocitam... \n");
			context->current->in=0;
			i=to32int(pulla(context),context);
			j=to32int(pulla(context),context);
			retval=js_mem_alloc(sizeof(abuf));
			retval->typ=INTEGER;
			retval->argument=(long)(signed int)(j>>i);
			par1=pulla(context);/*Tady je klic promenne*/
			pna=(lns*)par1->argument;
			pusha(retval,context);
			js_mem_free(par1);
			if(pna->type==INTVAR)
				set_var_value(pna,INTEGER,retval->argument,context);
			else {
				clearvar(pna,context);
				pna->type=INTEGER;
				pna->value=retval->argument;
			}
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("shrshreq");
		break;
	}
	debug("Funkce shrshreq je blbe!!!!!!!!!!!!!!!!\b\n"); }

void shrshrshr(js_context*context) /*1*/
{	abuf*retval=0;/*paranoia*/
	unsigned int i,j;
	debug("Vjizdim do funkce shl ");
	switch(context->current->in)
	{	case 0: 
			debug("Zpracovavam prvni argument\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=(vrchol*)context->current->arg[0]; /* prolezeme cyklem podstrom sveho prvniho potomka */
			/* Neni treba prenastavovat stav z prolezeny na neprolezeny */
		break;
		case 1:	debug("ctu druhy argument\n");
			retval=pulla(context); /* Vyhodnocuji zleva doprava */
			RESOLV(retval);
			pusha(retval,context);
			context->current->in=2;
			pushp(context->current,context);
			context->current=(vrchol*)context->current->arg[1]; /* prolezeme cyklem podstrom sveho druheho potomka */
			sezvykan=0;
		break;
		case 2:	debug("pocitam...\n");
			retval=js_mem_alloc(sizeof(abuf));
			retval->typ=INTEGER;
			i=to32int(pulla(context),context);
			j=to32int(pulla(context),context);
			retval->argument=(long)(signed int)(j>>i);
			pusha(retval,context);
			sezvykan=1;
			context->current->in=0;
			context->current=pullp(context);
		break;
		default:
			moc("shrshrshr");
		break;
	}
	debug("Funkce shrshrshr je blbe!!!!!!!!!!!!!\b\n"); }

void string(js_context*context)
{       abuf*retval=js_mem_alloc(sizeof(abuf));
        debug("Vracim STRING literal\n");
        retval->typ=STRING;
        retval->argument=(long)js_mem_alloc(1+strlen(context->current->arg[0]));
	strcpy((char*)retval->argument,(char*)context->current->arg[0]);
	pusha(retval,context);
        context->current=pullp(context);
        sezvykan=1;
}

void this(js_context*context) /* Tohle bude masite */
{	abuf*retval=js_mem_alloc(sizeof(abuf));
	debug("Funkce this - vracim current namespace\n");
	retval->typ=MAINADDRSPC; /*Tady 'de do tuhyho!! :-0 */
	retval->argument=(long)context->lnamespace;
	pusha(retval,context);
	context->current=pullp(context);
	sezvykan=1;
}

void threerighteq(js_context*context) /*2*/
{	abuf*retval=0;
	abuf*par1;
	lns* pna;
	unsigned int i,j;
	long pna1;
	debug("Vjizdim do funkce threerighteq\n");
	switch(context->current->in)
	{	case 0:	debug("left-hand-side\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	debug("ctu 1. arg... ");
			sezvykan=0;
			par1=pulla(context);
			VARTEST(par1,"You can assign only to variable\n");
			pusha(par1,context);
			pna=(lns*)par1->argument;
			pna1=vartoint(pna,context);/*Norma chce vyhodnocovat zleva doprava, tak nech sa paci...*/
			par1=js_mem_alloc(sizeof(abuf));
			par1->typ=INTEGER;
			par1->argument=pna1;
			pusha(par1,context);
			debug("right-hand-side... \n");
			context->current->in=2;
			pushp(context->current,context);
			context->current=context->current->arg[1];
		break;
		case 2:	debug("pocitam... \n");
			context->current->in=0;
			i=to32int(pulla(context),context);
			j=to32int(pulla(context),context);
			retval=js_mem_alloc(sizeof(abuf));
			retval->typ=INTEGER;
			retval->argument=(long)(signed int)(j>>i);
			par1=pulla(context);/*Tady je klic promenne*/
			pna=(lns*)par1->argument;
			pusha(retval,context);
			js_mem_free(par1);
			if(pna->type==INTVAR)
				set_var_value(pna,INTEGER,retval->argument,context);
			else {
				clearvar(pna,context);
				pna->type=INTEGER;
				pna->value=retval->argument;
			}
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("threerighteq");
		break;
	}
 	debug("Funkce threerighteq je blbe!!!!!!!!!!!\n"); }

void mulassign(js_context*context)
{	abuf*retval=0;
	abuf*par1;
	lns* pna;
	int pna2;
	float pna1,pna3;
        debug("Vjizdim do funkce mulassign\n");
        switch(context->current->in)
	{	case 0: debug("left-hand-side\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	debug("ctu 1. arg... ");
			sezvykan=0;
			par1=pulla(context);
			VARTEST(par1,"You can assign only to variable!\n");
			pusha(par1,context);
			pna=(lns*)par1->argument;
			pna1=vartofloat(pna,context);/*Norma chce vyhodnocovat zleva doprava, tak nech sa paci...*/
			par1=js_mem_alloc(sizeof(abuf));
			par1->typ=FLOAT;
			par1->argument=(long)js_mem_alloc(sizeof(float));
			*(float*)par1->argument=pna1;
			pusha(par1,context);
			debug("right-hand-side... \n");
			context->current->in=2;
			pushp(context->current,context);
			context->current=context->current->arg[1];
		break;
		case 2: debug("pocitam... \n");
			context->current->in=0;
			retval=js_mem_alloc(sizeof(abuf));
			retval->typ=FLOAT;
			retval->argument=(long)js_mem_alloc(sizeof(float));
			pna3=tofloat(pulla(context),context);
			pna1=tofloat(pulla(context),context);
			*(float*)retval->argument=pna1=mojenas(pna1,pna3);
			par1=pulla(context);/*Tady je klic promenne*/
			pna=(lns*)par1->argument;
			pusha(retval,context);
			js_mem_free(par1);
			if(pna->type==INTVAR)
				set_var_value(pna,retval->typ,retval->argument,context);
			else {
				clearvar(pna,context);

				pna->type=FLOAT;
				pna->value=(long)js_mem_alloc(sizeof(float));
				*(float*)pna->value=*(float*)retval->argument;
				if(((float)(pna2=(int)rint(pna1)))==pna1)
				{	pna->type=INTEGER;
					js_mem_free((void*)pna->value);
					pna->value=pna2;
					retval->typ=INTEGER;
					js_mem_free((void*)retval->argument);
					retval->argument=pna2;
					debug("je to cele!\n");
				}
			}
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("mulassign");
		break;
	}
}

void true(js_context*context)
{       abuf*retval=js_mem_alloc(sizeof(abuf));
        debug("Vracim TRUE literal\n");
	retval->typ=BOOLEAN;
        retval->argument=TRUE;
        pusha(retval,context);
        context->current=pullp(context);
        sezvykan=1;
}

void mytypeof(js_context*context)
{	abuf* arg;
	int typ;
	debug("Vjizdim do funkce mytypeof ");
	switch(context->current->in)
	{	case 0:	debug("zjistuji argument\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=(vrchol*)context->current->arg[0];
		break;
		case 1:	debug("zjistuji typ ");
			sezvykan=1;
			arg=pulla(context);
			RESOLV(arg);
			typ=arg->typ;
			delarg(arg,context);
			arg=js_mem_alloc(sizeof(abuf));
			arg->typ=STRING;
			arg->argument=(long)js_mem_alloc(15); /* eye-built constant! */
			switch(typ)
			{	case INTEGER:
				case FLOAT:
					strcpy((char*)arg->argument,"number");
				break;
				case UNDEFINED:
					strcpy((char*)arg->argument,"undefined");
				break;
				case NULLOVY:
				case ADDRSPACE:
				case ADDRSPACEP:
				case ARRAY:
					strcpy((char*)arg->argument,"object");
				break;
				case BOOLEAN:
					strcpy((char*)arg->argument,"boolean");
				break;
				case STRING:
					strcpy((char*)arg->argument,"string");
				break;
				case FUNKCE:
				case FUNKINT:
					strcpy((char*)arg->argument,"function");
				break;
				default:
					my_internal("Internal: Detecting strange type!\n",context);
					strcpy((char*)arg->argument,"undefined");
				break;
			}
			pusha(arg,context);
			context->current->in=0;
			context->current=pullp(context);
		break;
		default:
			moc("mytypeof");
		break;
	}
}

void var(js_context*context) /* Tohle bude masite */
{	abuf*var;
	debug("Vjizdim do funkce var ");
	switch(context->current->in)
	{	case 0:	debug("hledam ji\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=(vrchol*)context->current->arg[0];
		break;
		case 1:	debug("uz ji mam!\n");
			context->current->in=0;
			var=pulla(context);
			if(var->typ==VARIABLE)
				create(((lns*)var->argument)->identifier,context->lnamespace,context);
			else	if(((vrchol*)context->current->arg[0])->opcode!=TLocAssign) 
				{	
					my_internal("Divny typ v inicializaci promenne!\n",context);
					var=js_mem_alloc(sizeof(abuf));
					var->typ=UNDEFINED;
					var->argument=0;
					sezvykan=1;
					pusha(var,context);
					context->current=pullp(context);
					return;
				}
			delarg(var,context);
			var=js_mem_alloc(sizeof(abuf));
			var->typ=UNDEFINED;
			var->argument=0;
			pusha(var,context);
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("var");
		break;
	}
}

void myvoid(js_context*context)
{	abuf*retval=0; /*Spise paranoia nez bezpecnost*/
	debug("Vjizdim do funkce myvoid ");
	switch(context->current->in)
	{	case 0:	debug("jedu dolu\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=(vrchol*)context->current->arg[0];
		break;
		case 1:	debug("jedu nahoru\n");
			sezvykan=1; /*asi zbytecne, ale paranoia je matka moudrosti*/
			retval=js_mem_alloc(sizeof(abuf));/*preallokujeme*/
			retval->typ=UNDEFINED;/*Fixme! doufam, ze void je undefined*/
			retval->argument=0;
			delarg(pulla(context),context);
			pusha(retval,context);
			context->current->in=0;
			context->current=pullp(context);
		break;
		default:
			moc("myvoid");
		break;
	}

}

void xor(js_context*context)
{       abuf*retval=0;/*paranoia*/
        debug("Vjizdim do funkce xor ");
        switch(context->current->in)
        {       case 0:
		        debug("Zpracovavam prvni argument\n");
                        context->current->in=1;
			pushp(context->current,context);
                        context->current=(vrchol*)context->current->arg[0]; /* prolezeme cyklem podstrom sveho prvniho potomka */
                        /* Neni treba prenastavovat stav z prolezeny na neprolezeny */
                break;
                case 1:
		        debug("ctu druhy argument\n");
                        context->current->in=2;
			pushp(context->current,context);
                        context->current=(vrchol*)context->current->arg[1]; /* prolezeme cyklem podstrom sveho druheho potomka */
                        sezvykan=0;
                break;
                case 2:
        debug("Pocitam xor\n");
                        retval=js_mem_alloc(sizeof(abuf));
                        retval->typ=INTEGER;
                        retval->argument=(to32int(pulla(context),context)^to32int(pulla(context),context));
                        /* Prekonvertit do 32-bitovych integeru a spocitat ^. Integer je cislo v desitkove soustave (neni-li receno jinak) */
                        pusha(retval,context);
                        sezvykan=1;
                        context->current->in=0;
                        context->current=pullp(context);
                break;
                default:
                        moc("xor");
                break;
        }
}

void xorassign(js_context*context) 
{	abuf*retval=0;
	abuf*par1;
	lns* pna;
	long pna1;
	debug("Vjizdim do funkce xorassign\n");
	switch(context->current->in)
	{	case 0:	debug("left-hand-side\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	debug("ctu 1. arg... ");
			sezvykan=0;
			par1=pulla(context);
			VARTEST(par1,"You can assign only to variable!\n");
			pusha(par1,context);
			pna=(lns*)par1->argument;
			pna1=vartoint(pna,context);/*Norma chce vyhodnocovat zleva doprava, tak nech sa paci...*/
			par1=js_mem_alloc(sizeof(abuf));
			par1->typ=INTEGER;
			par1->argument=pna1;
			pusha(par1,context);
			debug("right-hand-side... \n");
			context->current->in=2;
			pushp(context->current,context);
			context->current=context->current->arg[1];
		break;
		case 2:	debug("pocitam... \n");
			context->current->in=0;
			retval=js_mem_alloc(sizeof(abuf));
			retval->typ=INTEGER;
			retval->argument=to32int(pulla(context),context)^to32int(pulla(context),context);
			par1=pulla(context);/*Tady je klic promenne*/
			pna=(lns*)par1->argument;
			pusha(retval,context);
			js_mem_free(par1);
			if(pna->type==INTVAR)
				set_var_value(pna,INTEGER,retval->argument,context);
			else {
				clearvar(pna,context);
				pna->type=INTEGER;
				pna->value=retval->argument;
			}
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("xorassign");
		break;
	}
}

void mywhile(js_context*context)
{	abuf*retval=0;
	switch(context->current->in)
        {	case 0:	debug("While, vyhodnocuji podminku\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	context->current->in=2;
			if(tobool(pulla(context),context)) {debug("While: Je to pravda!\n");
				sezvykan=0;
				pushp(context->current,context);
				context->current=context->current->arg[1];
			} else
			{       debug("While: Je to lez jako vez!\n");
				retval=js_mem_alloc(sizeof(abuf));
				retval->typ=UNDEFINED;
				retval->argument=0;
				pusha(retval,context);
				sezvykan=1;
				context->current->in=0;
				context->current=pullp(context);
			}
		break;
		case 2:	debug("While, opakuji vyhodnoceni podminky\n");
			sezvykan=0;
			delarg(pulla(context),context);
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		default:
			moc("mywhile");
		break;
	}
}

void for1(js_context*context) 
{	vrchol*pomocny_byva_casto_nemocny=0;
	abuf*retval=0;
	switch(context->current->in)
	{	case 0:	debug("For1, inicializacni kodek\n");
			context->current->in=2;
			pushp(context->current,context);
			context->current=context->current->arg[0];
                break;
		case 1: context->current->in=3;
			if(tobool(pulla(context),context)) {debug("For1: Je to pravda!\n");
				sezvykan=0;
				pomocny_byva_casto_nemocny=context->current;
				context->current=context->current->arg[3];
				pushp(pomocny_byva_casto_nemocny,context);
			} else
			{	debug("For1: Je to lez jako vez!\n");
				retval=js_mem_alloc(sizeof(abuf));
				retval->typ=UNDEFINED;
				retval->argument=0;
				pusha(retval,context);
				sezvykan=1;
				context->current->in=0;
				context->current=pullp(context);
			}
		break;
		case 2:	debug("For1, vyhodnocuji podminku\n");
			sezvykan=0;
			delarg(pulla(context),context);
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[1];
		break;
		case 3:	debug("For1, inkrementuji\n");
			sezvykan=0;
			delarg(pulla(context),context);
			context->current->in=2;
			pushp(context->current,context);
			context->current=context->current->arg[2];
		break;
		default:
			moc("for1");
		break;
	}
}

void for2(js_context*context)
{	my_internal("Ale h0vn0!!!\n",context); 
	js_error("Unimplemented version of for command ",context);
}

/* for3 je interpretace prikazu for(promenna in object){}, ktere ma
   postupne vracet jednotlive propriety toho objektu. Takze my si
   najdeme prvni seznam, toho objektu, ktery je netrivialni (seznamu je
   HASHNUM a ten seznam je typu lns*seznam[HASHNUM] a vracime postupne
   prvky tohoto seznamu. Kdyz dojdeme na konec (null), tak hledame dalsi
   netrivialni seznam */
void for3(js_context*context) 
{	abuf*retval,*pombuf;
	lns*fakevar,*pomvar;
	js_id_name * pna;
	char* jmeno_pne;
	switch(context->current->in)
	{	case 0:	debug("For3 zjistuji smirovany objektik\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[1];
			sezvykan=0;
		break;
		case 1: context->current->in=2;
			debug("For3 prejimam ho ");
			pombuf=pulla(context);
			RESOLV(pombuf);
			if(pombuf->typ!=ADDRSPACE && pombuf->typ!=ADDRSPACEP)
			{	js_error("for ... in allowed only for objects!",context);
				retval=js_mem_alloc(sizeof(abuf));
				retval->typ=UNDEFINED;
				retval->argument=0;
				pusha(retval,context);
				delarg(pombuf,context);
				return;
			}
			fakevar=js_mem_alloc(sizeof(lns));
			context->current->arg[3]=fakevar;
			fakevar->type=ADDRSPACEP;
			fakevar->value=pombuf->argument;
			js_mem_free(pombuf);
			pomvar=lookup(MIN1KEY,(plns*)fakevar->value,context);
			if(pomvar->type!=PARLIST)
			{	my_internal("Parent list corrupted!\n",context);
				js_error("Error in for...in ",context);
				retval=js_mem_alloc(sizeof(abuf));
				retval->typ=UNDEFINED;
				retval->argument=0;
				pusha(retval,context);
				return;
			}
			add_to_parlist(fakevar,pomvar);
			pushp(context->current,context);
			context->current=context->current->arg[0];
			debug("zjistuji expression\n");
			sezvykan=0;
		break;
		case 2:	debug("For3 vybiram prirazovanou promennou\n");
			pombuf=pulla(context);
			if(pombuf->typ!=VARIABLE)
				js_error("for ... in should assign to variable!",context);
			context->current->in=3;
			context->current->arg[4]=(void*)pombuf->argument;
			js_mem_free(pombuf);
			pombuf=js_mem_alloc(sizeof(abuf));
			pombuf->typ=UNDEFINED;
			pombuf->argument=0;
			pusha(pombuf,context);
			context->current->arg[5]=0;
			context->current->arg[6]=0;
/*			context->current->in=3;*/
			sezvykan=0;
		break;
		case 3:	/* Tady se bude cyklit, dokud neskonci for3.
			   Nakonec se musi dat clearvar na arg[3]. arg[4] je
			   snad ukotveny, takze snad leakovat nezustane.
			   arg[5] a arg[6] se jenom snuluji a rekneme in=0.
			 */
			debug("For3 vracim identifier\n");
			if(!context->current->arg[6])
nebo_skrtnem_sirkou:		while(!context->current->arg[6] && ((int)context->current->arg[5])<HASHNUM)
				{
					context->current->arg[6]=((plns*)((lns*)context->current->arg[3])->value)->ns[(int)context->current->arg[5]];
					/* co tim chtel basnik rict ?? */
/* Basnik: context->ptr->arg[3] je typu lns*, jeho value je plns* a my
   koukneme do namespacu pod timto plns (to znamena pointer na localnamespace)
   a to konkretne do context->current->arg[5]-te pozice. */
					/*(int)context->current->arg[5]=(int)context->current->arg[5]+1;*/
					context->current->arg[5]=(void *)((int)context->current->arg[5]+1);
				}
			else {	context->current->arg[6]=((lns*)context->current->arg[6])->next;
				while(!context->current->arg[6] && ((int)context->current->arg[5])<HASHNUM)
				{	context->current->arg[6]=((plns*)((lns*)context->current->arg[3])->value)->ns[(int)context->current->arg[5]];
					/* co tim chtel basnik rict ?? */
					/*(int)context->current->arg[5]=(int)context->current->arg[5]+1;*/
					context->current->arg[5]=(void *)((int)context->current->arg[5]+1);
				}
			}
			if(!context->current->arg[6])
			{	context->current->in=0;
				context->current->arg[5]=0;
				context->current->arg[4]=0; /* kdo vi */
				clearvar((lns*)context->current->arg[3],context);
				js_mem_free(context->current->arg[3]);
				context->current->arg[3]=0;
				delarg(pulla(context),context);
				retval=js_mem_alloc(sizeof(abuf));
				retval->typ=UNDEFINED;
				retval->argument=0;
				pusha(retval,context);
				context->current=pullp(context);
				sezvykan=1;
				return;
			}
			if((((lns*)context->current->arg[6])->type==FUNKCE) ||
				(((lns*)context->current->arg[6])->type==FUNKINT)
				|| (((lns*)context->current->arg[6])->identifier==MIN1KEY)) 
			{	context->current->arg[6]=((lns*)context->current->arg[6])->next;
				goto nebo_skrtnem_sirkou;
			}
			pna=context->namespace[((int)context->current->arg[5])-1];
			while(pna && (pna->klic!=((lns*)context->current->arg[6])->identifier/HASHNUM))pna=pna->next;
			if(!pna){ my_internal("Kalim mimo misu!\n",context);
				retval=js_mem_alloc(sizeof(abuf));
				retval->typ=UNDEFINED;
				retval->argument=0;
				pusha(retval,context);
				js_error("Error in for...in ",context);
				return;
			}
			jmeno_pne=js_mem_alloc(strlen(pna->jmeno)+1);
			strcpy(jmeno_pne,pna->jmeno);
			clearvar((lns*)context->current->arg[4],context);
			((lns*)context->current->arg[4])->type=STRING;
			((lns*)context->current->arg[4])->value=(long)jmeno_pne;
			pushp(context->current,context);
			context->current=context->current->arg[2];
			sezvykan=0;
			delarg(pulla(context),context);
		break;
	}
}

void pokracuj(js_context*context)
{	vrchol*parenti;
	abuf*arg1;
	plns*pom;
	/* Najit nejblizsi while, for, for3 (dobre pripady), funkce, nebo
	 * podtect buffer parentu (spatne pripady). Pritom musime dat pozor
	 * a vsem parentum nastavit, ze je in=0; pokud vjedeme do with,
	 * tak musime vykuchat z argbufu puvodni namespace */
	debug("Vjizdim do funkce pokracuj, ");
	parenti=pullp(context);
	while((parenti)&&(parenti->opcode !=TFOR1)&&(parenti->opcode!=TFOR3)&&
			(parenti->opcode!=TWHILE)&&(parenti->opcode!=TFUNCTIONDECL))
	{	if(parenti->opcode==TWITH)
		{	debug("kucham with, ");
			arg1=pulla(context);
			if((arg1->typ!=ADDRSPACE) && (arg1->typ!=MAINADDRSPC))
			{	my_internal("Internal: Lost addrspace by continue!!\n",context);
				js_error("Strange addrspace manipulation by scope resolution ",context);
				return;
			}	
			pom=context->lnamespace;
			context->lnamespace=pom->next;
			pom->next=(plns*)arg1->argument;
			js_mem_free(arg1);
		}
		parenti->in=0;
		parenti=pullp(context);
	}
	if(!parenti || parenti->opcode==TFUNCTIONDECL)
	{	js_error("You called continue out of cycle!!\n",context);
		return;
		/* Snad jiz konsolidovano */
	}
	arg1=js_mem_alloc(sizeof(abuf));
	arg1->typ=UNDEFINED;
	arg1->argument=0;
	pusha(arg1,context); /* vyrabim retval */
	sezvykan=1;
	context->current=parenti;
	debug("to je konec\n");
}

void breakni(js_context*context)
{	vrchol*parenti;
	abuf*arg1;
	plns*pom;
	/* Najit nejblizsi while, for, for3 (dobre pripady), funkce, nebo
	 * podtect buffer parentu (spatne pripady). Pritom musime dat pozor
	 * a vsem parentum nastavit, ze je in=0; pokud vjedeme do with,
	 * tak musime vykuchat z argbufu puvodni namespace */
	debug("Vjizdim do funkce breakni, ");
	parenti=pullp(context);
	while((parenti)&&(parenti->opcode !=TFOR1)&&(parenti->opcode!=TFOR3)&&
			(parenti->opcode!=TWHILE)&&(parenti->opcode!=TFUNCTIONDECL))
	{	if(parenti->opcode==TWITH)
		{	debug("kucham with, ");
			arg1=pulla(context);
			if((arg1->typ!=ADDRSPACE)&&(arg1->typ!=MAINADDRSPC))
			{	my_internal("Internal: Lost addrspace by break!!\n",context);
				js_error("Invalid addrspace manipulation by break ",context);
				return;
			}
			pom=context->lnamespace;
			context->lnamespace=pom->next;
			pom->next=(plns*)arg1->argument;
			js_mem_free(arg1);
		}
		parenti->in=0;
		parenti=pullp(context);
	}
	if(!parenti || parenti->opcode==TFUNCTIONDECL)
	{	js_error("You called break out of cycle!!\n",context);
		return;
		/* Snad jiz konsolidovano */
	}
	arg1=js_mem_alloc(sizeof(abuf));
	arg1->typ=UNDEFINED;
	arg1->argument=0;
	pusha(arg1,context); /* vyrabim retval */
	parenti->in=0;
	if(parenti->opcode==TFOR3)
	{	parenti->arg[5]=parenti->arg[4]=0;
		clearvar((lns*)parenti->arg[3],context);
		js_mem_free(parenti->arg[3]);
		debug("vracim se do for3 ");
	}
	sezvykan=1;
	context->current=pullp(context);
	debug("to je konec\n");
}

void vrat(js_context*context)
{	vrchol*parenti;
	abuf*arg1,*arg2;
	plns*pom;
	/* Najit nejblizsi funkce, nebo
	 * podtect buffer parentu. Pritom musime dat pozor
	 * a vsem parentum nastavit, ze je in=0; pokud vjedeme do with,
	 * tak musime vykuchat z argbufu puvodni namespace */
	debug("Vjizdim do funkce return, ");
	switch(context->current->in)
	{	case 0:	debug("zjistuji retval\n");
			context->current->in=1;
			if(context->current->arg[0])
			{	pushp(context->current,context);
				context->current=(vrchol*)context->current->arg[0];
			} else
			{	arg1=js_mem_alloc(sizeof(abuf));
				arg1->typ=UNDEFINED;
				arg1->argument=0;
				pusha(arg1,context);
			}
		break;
		case 1:	debug("returnim, ");
			context->current->in=0;
			arg2=pulla(context);
			RESOLV(arg2);
			parenti=pullp(context);
			while((parenti)&&(parenti->opcode!=TFUNCTIONDECL))
			{	if(parenti->opcode==TWITH)
				{	debug("kucham with, ");
					arg1=pulla(context);
					if((arg1->typ!=ADDRSPACE)&&(arg1->typ!=MAINADDRSPC))
					{	my_internal("Internal: Lost addrspace by return!!\n",context);
						js_error("Bad addrspace manipulation near return ",context);
						return;
					}
					pom=context->lnamespace;
					context->lnamespace=pom->next;
					pom->next=(plns*)arg1->argument;
					js_mem_free(arg1);
				}
				if(parenti->opcode==TFOR3)
				{	debug("preskakuji for3 ");
					parenti->arg[5]=parenti->arg[4]=0;
					clearvar((lns*)parenti->arg[3],context);
					js_mem_free(parenti->arg[3]);
				}
				parenti->in=0;
				parenti=pullp(context);
			}
			pusha(arg2,context); /* vracim retval na buffer */
			if(!parenti)
				debug("You called return out of function!!\n");
				/* Snad jiz konsolidovano */
			sezvykan=1;
			context->current=parenti;
			debug("to je konec\n");
		break;
		default:
			moc("vrat");
		break;
	}
}

void with(js_context*context) /* Tohle bude masite */
{	abuf* arg,*prevpar;
	plns* pom;
	debug("Vstup do funkce with ");
	switch(context->current->in)
	{	case 0:	debug("zjistuji argument\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=(vrchol*)context->current->arg[0];
		break;
		case 1:	debug("menim namespace\n");
			context->current->in=2;
			arg=pulla(context);
			VARTEST(arg,"\"With\" is better with variable!\n");
			if((((lns*)arg->argument)->type!=ADDRSPACE)&&(((lns*)arg->argument)->type!=ADDRSPACEP)) 
			{	delarg(arg,context); /* Konsolidace pred krachem */
				js_error("Usually we do \"with\" with object!\n",context);
				return;
			}
			prevpar=js_mem_alloc(sizeof(abuf));
			prevpar->typ=MAINADDRSPC;
			prevpar->argument=(long)(pom=(plns*)((lns*)arg->argument)->value)->next;
			pom->next=context->lnamespace;
			context->lnamespace=pom;
			pusha(prevpar,context);
			delarg(arg,context);
			sezvykan=0;
			pushp(context->current,context);
			context->current=context->current->arg[1];
		break;
		case 2:	debug("vracim namespace\n");
			context->current->in=0;
			prevpar=pulla(context);/* retval of with statement */
			arg=pulla(context);
			pusha(prevpar,context);
			if(arg->typ!=MAINADDRSPC)
			{	my_internal("Internal: Lost addrspace in with statement!!!\n",context);
				js_error("Error handling addrspace near with statement ",context);
				return;
			}
			pom=context->lnamespace;
			context->lnamespace=pom->next;
			pom->next=(plns*)arg->argument;
			js_mem_free(arg);
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("with");
		break;
	}
}

void identifier(js_context*context)
{	abuf*retval=js_mem_alloc(sizeof(abuf));
	debug("Vracim identifikator\n");
	retval->typ=VARIABLE;
	retval->argument=(long)lookup((long)context->current->arg[0],context->lnamespace,context);/*Vrazime na zasobnik klic pne*/
	pusha(retval,context);
	sezvykan=1;
	context->current=pullp(context);
}

void program(js_context*context)
{	abuf*pending_argument;
	varlist*svinec;
	struct jsnslist*sirotci;
	debug("Vstup do fce program,");
	switch(context->current->in)
	{	case 0:	debug("pass 1\n");
			context->current->in=1;
/*			if(context->current->arg[0])
			{*/	pushp(context->current,context);
				context->current=(vrchol*)context->current->arg[0];
/*			} else{
				debug("Bug! funkce program nedostala prvni argument!\n");
				pending_argument=js_mem_alloc(sizeof(abuf));
				pending_argument->typ=UNDEFINED;
				pending_argument->argument=0;
				pusha(pending_argument,context);
			} Tohle je od chvile kdy takove veci zeru v zvykni zbytecne*/
		break;
		case 1:
			debug("fce program pass 2\n");
			context->current->in=0;
			if(context->current->arg[1])
			{	pending_argument=pulla(context);
				delarg(pending_argument,context);
				context->current=(vrchol*)context->current->arg[1];
			}
			else {	context->current=pullp(context);
				sezvykan=1;
			}
			if(!context->depth1)
			{	while((svinec=context->mrtve_promenne))
				{	context->mrtve_promenne=context->mrtve_promenne->next;
					clearvar(svinec->lekla_ryba,context);
					js_mem_free(svinec->lekla_ryba);
					js_mem_free(svinec);
					debug("Killim leklou promennou!\n");
				}
				while((sirotci=context->first_ns))
				{	deletenamespace(sirotci->pns,context);
					context->first_ns=context->first_ns->next;
					js_mem_free(sirotci);
					debug("Vrazdim sirotka!\n");
				}
				context->last_ns=&context->first_ns;
			}
		break;
		default:
			moc("program");
		break;
	}
}

void funkce(js_context*context)/*function f(){}*/
{	abuf*arg,*a1,*a2;
	plns*nnspc;
	lns* funk,*pom;
	switch(context->current->in)
	{	case 0:	debug("Ctu jmeno nove funkce\n");
			if(context->current->arg[0])
			{	context->current->in=1;
				pushp(context->current,context);
				context->current=(vrchol*)context->current->arg[0];
			} else{ debug("Nedefinuje se nic!\n");
				context->current->in=0; /* zbytecne */
				sezvykan=1;
				context->current=pullp(context);
				arg=js_mem_alloc(sizeof(abuf));
				arg->typ=UNDEFINED;
				arg->argument=0;
				pusha(arg,context);
			}
		break;
		case 1:	debug("Definuji funkci\n");
			arg=pulla(context);
			if(arg->typ!=VARIABLE)
			{	js_error("Defining function nowhere ",context);
				delarg(arg,context);
				return;
			}
			funk=create(((lns*)arg->argument)->identifier,context->lnamespace,context);
			funk->type=FUNKCE;
			funk->value=(long)context->current;/*pointer na nas nas zavola*/
			arg->typ=UNDEFINED;
			arg->argument=0;
			pusha(arg,context);/*Nesmyslny vysledek - vracime prazdny*/
			context->current->in=0;
			if(!context->current->arg[3])
			{	sezvykan=1;
				context->current=pullp(context);
			} else{ context->current=context->current->arg[3];
				sezvykan=0;
			}
		break;
		case 100:
			context->current->in=101;
			debug("Functioncall zjistuji nazvy svych argumentu\n");
sto:			sezvykan=0;
			if(context->depth1++>options_get_int("js_fun_depth"))
				js_error("Too deep function-call structure",context);
			pushp(context->current,context);
			context->current=context->current->arg[1]; 
		break;
		case 101:
			debug("Functioncall stavim novy namespace a volam\n");
			context->current->in=102;
sto_jenda:		sezvykan=0;
			a1=pulla(context);
			a2=pulla(context);
			nnspc=newnamespace(a1,a2,context);
			pom=create(MIN1KEY,nnspc,context);
			pom->type=PARLIST;
			pom->value=0;
			add_to_parlist(&fotr_je_lotr,pom);
			nnspc->next=context->lnamespace;
			context->lnamespace=nnspc;
			context->fotrisko=context->lnamespace; /* Kvuli memory-leakum */
			pushp(context->current,context);
			context->current=context->current->arg[2];
		break;
		case 102:
			if(--context->depth1<0)
			{	debug("function-call structure has negative depth!\n");
				context->depth1=0;
			}
			debug("Functioncall uklizi namespace\n");
			context->current->in=0;
			nnspc=context->lnamespace;
			context->lnamespace=context->lnamespace->next;
			context->fotrisko=context->lnamespace;

			arg=pulla(context);
			RESOLV(arg);
			pom=lookup(MIN1KEY,nnspc,context);
			if(pom->type!=PARLIST) my_internal("Parentlist je podplacen!\n",context);
			delete_from_parlist(&fotr_je_lotr,pom);
			if((arg->typ==ADDRSPACE)||(arg->typ==ADDRSPACEP)||(arg->typ==ARRAY))
			{	pom=lookup(MIN1KEY,(plns*)arg->argument,context);
				if(pom->type!=PARLIST)	my_internal("Parentlist podplacen!\n",context);
				add_to_parlist(&fotr_je_lotr,pom);
			} else 	pom=0;

			a1=js_mem_alloc(sizeof(abuf));
			a1->typ=ADDRSPACE;
			a1->argument=(long)nnspc;
			
			delarg(a1,context);

			if(pom)	delete_from_parlist(&fotr_je_lotr,pom);
			pusha(arg,context);

			if(!context->lnamespace)
			{	my_internal("Internal: Bacha! Podtekl namespace!!\n",context);
				js_error("Invalid namespace manipulation ",context);
				return;
			}
			sezvykan=1;
			context->current=pullp(context);
		break;
		case 200:
			debug("Constructor fcall pass 1\n");
			context->current->in=201;
			goto sto;
		break;
		case 201:
			debug("Constructor fcall pass 2\n");
			context->current->in=202;
			goto sto_jenda;
		break;
		case 202:
			if(--context->depth1<0)
			{	debug("Fcall structure has negative depth!\n");
				context->depth1=0;
			}
			debug("Constructor fcall po sobe neuklizi namespace\n");
			context->current->in=0;
			delarg(pulla(context),context);
			arg=js_mem_alloc(sizeof(abuf));
			arg->typ=ADDRSPACE;
			arg->argument=(long)context->lnamespace;
			pom=lookup(MIN1KEY,(plns*)context->lnamespace,context);
			if(pom->type!=PARLIST)
			{	my_internal("Parent list corrupted!\n",context);
				sezvykan=1;
				context->current=pullp(context);
				return;
			}
			delete_from_parlist(&fotr_je_lotr,pom);
			pusha(arg,context);
			context->lnamespace=context->lnamespace->next;
			context->fotrisko=context->lnamespace;
			if(!context->lnamespace)
			{	my_internal("Internal: Bacha! Podtekl namespace!!!\n",context);
				js_error("Invalid namespace manipulation ",context);
				return;
			}
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("funkce");
		break;
	}
}
			  
void multiply(js_context*context)/*a*b*/
{	abuf*retval=0;
	float pna1,pna3;
	int pna2;
        debug("Vjizdim do fce multiply ");
        switch(context->current->in)
        {       case 0:
                        debug("prvni argument\n");
                        context->current->in=1;
                        pushp(context->current,context);
                        context->current=context->current->arg[0];
                break;
                case 1:
                        debug("druhy argument\n");
			sezvykan=0;
			retval=pulla(context); /* Vyhodnocuji zleva doprava */
			RESOLV(retval);
			pusha(retval,context);
                        context->current->in=2;
                        pushp(context->current,context);
                        context->current=context->current->arg[1];
                break;
                case 2:
                        debug("pocitam; ");
                        context->current->in=0;
                        retval=js_mem_alloc(sizeof(abuf));
			retval->typ=FLOAT;
			retval->argument=(long)js_mem_alloc(sizeof(float));
			pna3=tofloat(pulla(context),context);
			pna1=tofloat(pulla(context),context);
			*(float*)retval->argument=pna1=mojenas(pna1,pna3);
			pusha(retval,context);
			sezvykan=1;
			context->current=pullp(context);
			if(((float)(pna2=(int)rint(pna1)))==pna1)
			{	retval->typ=INTEGER;
				js_mem_free((void*)retval->argument);
				retval->argument=pna2;
				debug("je to cele!\n");
			}
	
		break;
		default: moc("multiply");
                break;
        }
}

void divide(js_context*context)/*a/b*/
{	abuf*retval=0;
	float pna1,pna3;
	int pna2;
        debug("Vjizdim do fce divide ");
        switch(context->current->in)
        {       case 0:
                        debug("prvni argument\n");
                        context->current->in=1;
                        pushp(context->current,context);
                        context->current=context->current->arg[0];
                break;
                case 1:
                        debug("druhy argument\n");
			sezvykan=0;
			retval=pulla(context); /* Vyhodnocuji zleva doprava */
			RESOLV(retval);
			pusha(retval,context);
                        context->current->in=2;
                        pushp(context->current,context);
                        context->current=context->current->arg[1];
                break;
                case 2:
                        debug("pocitam; ");
                        context->current->in=0;
                        retval=js_mem_alloc(sizeof(abuf));
			retval->typ=FLOAT;
			retval->argument=(long)js_mem_alloc(sizeof(float));
			pna3=tofloat(pulla(context),context);
			pna1=tofloat(pulla(context),context);
			*(float*)retval->argument=pna1=mojenas(pna1,mojeinv(pna3));
			pusha(retval,context);
			sezvykan=1;
			context->current=pullp(context);
			if(((float)(pna2=(int)rint(pna1)))==pna1)
			{	retval->typ=INTEGER;
				js_mem_free((void*)retval->argument);
				retval->argument=pna2;
				debug("je to cele!\n");
			}
	
		break;
		default: moc("divide");
                break;
        }
}

void complement(js_context*context)/*~a*/
{	abuf*retval;
	debug("Vjizdim do fce complement ");
	switch(context->current->in)
	{	case 0: debug("zjistuji argument\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1: debug("pocitam!\n");
			retval=js_mem_alloc(sizeof(abuf));
			
			retval->typ=INTEGER;
			retval->argument=~to32int(pulla(context),context);
			pusha(retval,context);
			context->current->in=0;
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("complement");
		break;
	}
	
}

void parametry(js_context*context)/*(a,b,c)*/
{	/*Nashiftuje parametry do sebe*/
	abuf * a1,*a2,*a3;
	switch(context->current->in)
	{	case 0:	debug("Parametry: pars prima\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	debug("Parametry: pars secunda\n");
			sezvykan=0;
			context->current->in=2;
			pushp(context->current,context);
			context->current=context->current->arg[1];
		break;
		case 2:	debug("Parametry: pars tertia\n");
			context->current->in=0;
			a1=pulla(context);
			a2=pulla(context);
			a3=js_mem_alloc(sizeof(abuf));
			a3->typ=ARGUMENTY;
			a3->argument=(long)a2;
			a3->a1=(long)a1;
			pusha(a3,context);
			sezvykan=1;
			context->current=pullp(context);
		break;
		default: moc("parametry");
		break;
	}
}

void statementy(js_context*context)/*a;b;*/
{	debug("Statementy ");
	program(context);
}/*patrne se bude dit totez jako kdyz derivujeme program->program program*/

void zaporne(js_context*context)/*-a*/
{	abuf*retval;
	int pna2;
	float hodnota;
	debug("Vjizdim do fce zaporne ");
	switch(context->current->in)
	{	case 0: debug("zjistuji argument\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1: debug("pocitam!\n");
			retval=js_mem_alloc(sizeof(abuf));
			
			retval->typ=FLOAT;
			hodnota=tofloat(pulla(context),context);
			if(hodnota!=MY_NAN && hodnota!=MY_INFINITY && hodnota!=MY_MININFINITY)
				hodnota=-hodnota;
			if(hodnota!=MY_NAN)
			{	if(hodnota>MY_MAXDOUBLE)
					hodnota=MY_INFINITY;
				if(hodnota<MY_MINDOUBLE)
					hodnota=MY_MININFINITY;
			}
			retval->argument=(long)js_mem_alloc(sizeof(float));
			*(float*)retval->argument=hodnota;
			
			if(((float)(pna2=(int)rint(hodnota)))==hodnota)
			{	retval->typ=INTEGER;
				js_mem_free((void*)retval->argument);
				retval->argument=pna2;
				debug("je to cele!\n");
			}
			
			pusha(retval,context);
			context->current->in=0;
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("zaporne");
		break;
	}
}

void thisccall(js_context*context) /* Tohle bude masite */
{	abuf*pomarg=0,*p1=0;
	debug("ThisCCall: ");
	switch(context->current->in)
	{	case 0:	debug("pass1, sekam namespace\n");
			pomarg=js_mem_alloc(sizeof(abuf));
			pomarg->typ=MAINADDRSPC;
			pomarg->argument=(long)context->lnamespace->next;
			pusha(pomarg,context);
			context->lnamespace->next=0;
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	debug("pass2, uklizim namespace\n");
			p1=pulla(context);
			pomarg=pulla(context);
			pusha(p1,context);
			if(pomarg->typ!=MAINADDRSPC)
			{	my_internal("FATAL Error! Buffer leak implies namespace leak!\n",context);
				js_error("Strange addrspace ",context);
				return;
			}
			context->lnamespace->next=(plns*)pomarg->argument;
			context->current->in=0;
			sezvykan=1;
			context->current=pullp(context);
			js_mem_free(pomarg);
		break;
		default:
			moc("thisccall");
		break;
	}
}

void idccall(js_context*context) /*Tohle bude masite *//*3*/
{	my_internal("Ale h0vn0!!\n",context);
	js_error("Unsuported constructor call ",context);
}

void functioncall(js_context*);

void eccall(js_context*context) /* Tohle bude masite */
{	abuf*arg;
	if(context->current->in!=2)	{debug("Eccall "); functioncall(context);}
	else {	debug("Eccall - volam funkci\n");
                arg=pulla(context);
                context->current->in=0;
                VARTEST(arg,"You can execute only function-type vars!\n");
                if(((lns*)arg->argument)->type==FUNKCE)
                {       context->current=(vrchol*)((lns*)arg->argument)->value;
                        context->current->in=200; /*Persistentni zavolani*/
			js_mem_free(arg);
		}
                else
                        if(((lns*)arg->argument)->type==FUNKINT)
			{	pusha(arg,context);
				context->current->in=2;
				functioncall(context);
			}
/*				js_intern_fupcall(context,((lns*)arg->argument)->value,(lns*)arg->argument);*/
                        else{	delarg(arg,context); /* Konsolidace pred krachem */
				js_error("You can call only function-type vars!\n",context);
				return;
			}
	}
}

void conscall(js_context*context)/*volani konstruktoru*/ /* 3 */ 
{	my_internal("Funkce conscall byla preci obsoletizovana!!!\n",context); 
	js_error("Unsuported constructor call ",context);
}

void member(js_context*context)/*a.b*/
{	abuf*retval,*nans,*pom;/* Myslenka: Prvni syn nam na buffer da namespace, ktery my nesmime odallokovat (za zadnou cenu). Taktez tomu namespacu musime urvat potomky, dat si je na buffer, dat si na buffer current namespace, misto nej prdnout ten, co nam prisel od prvniho syna, pak zavolame druheho syna, at se s tim serve jak umi, pak vsechno vratime do poradku (snad) */
	vrchol*pomvrch;
	long pomv;
	lns*pomvar,*pomvar1;
	varlist* svinec;
	debug("Vjizdim do fce member ");
	switch(context->current->in)
	{	case 0:	debug("zjistuji strukturu\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	debug("omezuji namespace\n");
			context->current->in=2;
			nans=pulla(context); /* Here'll be variable or ADDRSPACE*/
			RESOLV(nans); /* Here IS ADDRSPACE */
#ifdef JS_DEBUG_2
			printf("Member v kontextu %x",context->lnamespace);
#endif
			if(nans->typ!=ADDRSPACE && nans->typ!=ARRAY)
/*			{	delarg(nans,context);
				js_error("Trying member operator to non-object!\n",context);
			}*/
			{	if((pomvrch=(vrchol*)context->current->arg[1])->opcode!=TIDENTIFIER)    
				{	delarg(nans,context);
					if(!options_get_bool("js_all_conversions"))
						js_error("Strange dereference by non-object type variable!\n",context);
					nans=js_mem_alloc(sizeof(abuf));
					nans->typ=UNDEFINED;
					nans->argument=0;
					pusha(nans,context);
					context->current->in=0;
					sezvykan=1;
					context->current=pullp(context);
					return;
				}
				pusha(nans,context);
				pomv=(long)pomvrch->arg[0];
				if(pomv!=CStoString && pomv!=CSvalueOf && pomv!= CSMIN_VALUE && pomv!=CSMAX_VALUE &&pomv!=CSNaN && pomv!=CSlength && pomv!=CSindexOf && pomv!=CSlastIndexOf && pomv!=CSsubstring && pomv!=CSsubstr && pomv!=CScharAt &&pomv!=CStoLowerCase && pomv!= CStoUpperCase && pomv!= CSsplit && pomv!= CSparse && pomv!= CSUTC) {
					if(!options_get_bool("js_all_conversions"))
						js_error("Calling strange method/property of non-object typed variable!\n",context);
					nans=pulla(context);
					delarg(nans,context);
					nans=js_mem_alloc(sizeof(abuf));
					nans->typ=UNDEFINED;
					nans->argument=0;
					pusha(nans,context);
					context->current->in=0;
                                        sezvykan=1;
                                        context->current=pullp(context);
					return; /* Fixme - tady to urcite nebude dobre! */
				}
				nans=js_mem_alloc(sizeof(abuf));
				nans->typ=VARIABLE;
				nans->argument=(long)(pomvar=js_mem_alloc(sizeof(lns)));
				if(pomv== CSMIN_VALUE || pomv==CSMAX_VALUE || pomv==CSNaN || pomv==CSlength)
					pomvar->type=INTVAR;
				else    pomvar->type=FUNKINT;
				pomvar->identifier=pomv;
				pomvar->value=CIntMETFUN;
				pomvar->handler=(long)pulla(context);
				pusha(nans,context);
				context->current->in=0;
				sezvykan=1;
				context->current=pullp(context);
			} else{
				pom=js_mem_alloc(sizeof(abuf)); /*push current addrspace*/
				pom->typ=MAINADDRSPC;
				pom->argument=(long)context->lnamespace;
				pusha(pom,context); /* current addrspace pushed */

				pom=js_mem_alloc(sizeof(abuf)); /* prohibit parents */
				pom->typ=MAINADDRSPC;
				pom->argument=(long)((plns*)nans->argument)->next;
				pusha(pom,context); 
				((plns*)nans->argument)->next=0;/* parents forbidden! */
			
				context->lnamespace=(plns*)nans->argument;
				js_mem_free(nans);
				/* ... et omnis tera tremuit! */

				pushp(context->current,context);
				context->current=context->current->arg[1];
				sezvykan=0;
			}
#ifdef JS_DEBUG_2
			printf(" zkoumam %x ",context->lnamespace);
#endif
			break;
		case 2:	debug("vracim zemetras!\n");
			context->current->in=0;
			retval=pulla(context); /*value that should be returned*/
			nans=pulla(context);/*parents of current addrspc*/
			pom=pulla(context);/*previous namespace*/
			if((nans->typ!=MAINADDRSPC)||(pom->typ!=MAINADDRSPC))
			{	my_internal("Internal: Namespace lost by function member!!!\n",context);
				js_error("Invalid member operation ",context);
				return;
			}
			context->lnamespace->next=(plns*)nans->argument;
			js_mem_free(nans); /* limited namespace - we can forget */
			pomvar=lookup(MIN1KEY,context->lnamespace,context);
			if(!pomvar->value)
			{	if(retval->typ==INTVAR)
				{	debug("Nazdar!\n");/*	pomvar1=context->lnamespace->ns[((lns*)retval->argument)->identifier%HASHNUM];
					if(pomvar1&&pomvar1->identifier==((lns*)retval->argument)->identifier)
						context->lnamespace->ns[((lns*)retval->argument)->identifier%HASHNUM]=pomvar1->next;
					else
					{	while((pomvar1)&&(pomvar1->next)&&(pomvar1->next->identifier!=((lns*)retval->argument)->identifier))pomvar1=pomvar1->next;	
						if(!pomvar1)
						{	my_internal("Lost variable!\n",context);
							js_error("Invalid member operation ",context);
							return;
						}
						if(!pomvar1->next)
						{	my_internal("Lost variable!\n",context);
							js_error("Invalid member operation ",context);
							return;
						}
						svinec=js_mem_alloc(sizeof(varlist));
						svinec->lekla_ryba=pomvar1->next;
                                                svinec->next=context->mrtve_promenne;
						pomvar->next=pomvar->next->next; 
					}
					pomvar=(lns*)retval->argument;
					pomvar1=js_mem_alloc(sizeof(lns));
					pomvar1->type=pomvar->type;
					pomvar1->identifier=pomvar->identifier;
					pomvar1->mid=pomvar->mid;
					pomvar1->value=pomvar->value;
					pomvar1->handler=pomvar->handler;
					pomvar1->next=0;
					pomvar->type=UNDEFINED;
					pomvar->value=0;
					retval->argument=(long)pomvar1;*/
				} else	debug("Divne, divne, ale jedeme dal!\n");

				if(retval->typ!=UNDEFINED){
					pomvar1=context->lnamespace->ns[((lns*)retval->argument)->identifier%HASHNUM];
	                                if(pomvar1&&pomvar1->identifier==((lns*)retval->argument)->identifier)
					{	svinec=js_mem_alloc(sizeof(varlist));
						svinec->lekla_ryba=pomvar1;
						svinec->next=context->mrtve_promenne;
						context->mrtve_promenne=svinec;
						context->lnamespace->ns[((lns*)retval->argument)->identifier%HASHNUM]=pomvar1->next;
					}	
        	                        else
                	                {       while((pomvar1)&&(pomvar1->next)&&(pomvar1->next->identifier!=((lns*)retval->argument)->identifier))pomvar1=pomvar1->next;
						if(pomvar1)
						{	if(pomvar1->next)
							{/*	my_internal("Lost variable!\n",context);
								js_error("Invalid variable manipulation ",context);
								return;
							}*/
								svinec=js_mem_alloc(sizeof(varlist)); /* Zapamatujeme si promennou, aby se neztratila */
								svinec->lekla_ryba=pomvar1->next;
								svinec->next=context->mrtve_promenne;
								context->mrtve_promenne=svinec;
								pomvar1->next=pomvar1->next->next; /* Vyhodime variabli z addrspacu, pokud
ji nikdo neassignuje, bude z ni garbage, cili FIXME! Tady se vyrobi bezprizorna promenna, kterou je potreba pri eventualnim
assignu odallokovat! */
							}
						}
        	                        }
				}	

				delete_name_space(context->lnamespace,context);
			}/* Zde to bylo!*/
			context->lnamespace=(plns*)pom->argument;
			js_mem_free(pom);
			pusha(retval,context);
#ifdef JS_DEBUG_2
			printf(" nasel jsem %x %d\n",retval->argument, ((lns*)retval->argument)->type);
#endif
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("member");
		break;
	}		
}

int isanumber(char*a)
{	int i=0;
	if(a[0]=='-')i++;
	while(a[i]>=48 && a[i]<=57)i++;
	return !a[i];
}

void array(js_context*context)/*a[b]*/
{	abuf* array,*retval;
	lns*  variable,*vari=0,*pomvar=0,*pomvar1;
	long* typ,*value;
	varlist* svinec;
	char* index; int pomint,je_to_cislo;
	char pomstr[DELKACISLA]; 
		/*Hopefully number has less than 25 digits! Alert! */
	debug("Vjizdim do fce array ");
	switch(context->current->in)
	{	case 0:	debug("ctu jmeno pole\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1:	debug("ctu index\n");
			context->current->in=2;
			pushp(context->current,context);
			context->current=context->current->arg[1];
			sezvykan=0;
		break;
		case 2:	debug("pocitam...\n");
			/* Tady budou zmeny pro asociativni pole */
/* Jak na to, aneb co do komentare nepatri: Zjisti se, jestli jsou to sama
   cisla => pretypovani na string, jestli jo, tak se udela atoi a bude,
   jinak se neudela nic. Trik: Zjednoduseni. Tady se neudela to32int, ale
   pri testu delky se udela atoi B-) */
			index=tostring(pulla(context),context);
#ifdef JS_DEBUG_2
			printf("Hledam index %s ",index);
#endif
			pomint=0;
			je_to_cislo=isanumber(index);
			if(je_to_cislo && (atoi(index)==-1))
			{	js_error("Index -1 is prohibited!\n",context);
				js_mem_free(index);
				return;
			}
			array=pulla(context);
			VARTEST1(array);
			if(js_temp_var_for_vartest)
			{	js_temp_var_for_vartest=0;
				js_mem_free(index);
				js_error("Array is usually type of variable!\n",context);
				return;
			}
			typ=js_mem_alloc(sizeof(long));
			value=js_mem_alloc(sizeof(long));
			je_to_cislo=(je_to_cislo&&(((lns*)array->argument)->type==ARRAY));
			if(((variable=(lns*)array->argument)->type!=ARRAY) && (variable->type!= ADDRSPACE) && (variable->type!=ADDRSPACEP))
			{	if(variable->type!=INTVAR)	
				{	js_error("Index operator is allowed only for arrays!\n",context);
					js_mem_free(typ);
					js_mem_free(value);
					delarg(array,context);
					js_mem_free(index);
					return;
				}
				else {
					get_var_value(variable,typ,value,context);
					if((*typ!=ARRAY) && (*typ!=ADDRSPACE) && (*typ!=ADDRSPACEP)) 
					{	js_error("Index operator is allowed only for arrays!\n",context);
						js_mem_free(typ);
						js_mem_free(value);
						delarg(array,context);
						js_mem_free(index);
						return;
					}
					else {	
						pomvar=llookup(index,context->namespace,(plns*)*value,context);
#ifdef JS_DEBUG_2
						printf("1. zpus. v kontextu %x",*value);
#endif
						variable=((plns*)*value)->ns[pomvar->identifier%HASHNUM];
						if(variable&&variable->identifier==pomvar->identifier)
						{	svinec=js_mem_alloc(sizeof(varlist));
							svinec->lekla_ryba=variable;
							svinec->next=context->mrtve_promenne;
							context->mrtve_promenne=svinec;
							((plns*)*value)->ns[pomvar->identifier%HASHNUM]=variable->next;
						}
						else
						{	while((variable)&&(variable->next)&&(variable->next->identifier!=pomvar->identifier))variable=variable->next;
							if(variable)
							{	if(!variable->next)
								{	my_internal("Lost variable!\n",context);
									js_error("Invalid variable manipulation ",context);
									return;
								}
								svinec=js_mem_alloc(sizeof(varlist)); /* Zapamatujeme si promennou, aby se neztratila */
								svinec->lekla_ryba=variable->next;
								svinec->next=context->mrtve_promenne;
								context->mrtve_promenne=svinec;
								variable->next=variable->next->next; 
							}
						} 

						delarg(array,context);
						array=js_mem_alloc(sizeof(abuf));
						array->typ=ARRAY;/* Nevadi ze je laxne udelane */
						array->argument=*value;
						vari=variable;
						
					}
				}
			}
			else { 	vari=llookup(index,context->namespace,(plns*)variable->value,context);
#ifdef JS_DEBUG_2
				printf("2. zpusobem v kont. %x",variable->value);
#endif
				if(je_to_cislo){
					pomvar=lookup(js_lengthid,(plns*)variable->value,context);
					if(pomvar->type!=INTVAR)
					{	my_internal("Internal:Strange type of \"length\" property!\n",context);
						js_error("Invalid array/member operation ",context);
						return;
					}
					if(pomvar->handler<atoi(index))
					{
						pomint=pomvar->handler;
						pomvar->handler=atoi(index)+1;
						pomstr[DELKACISLA-1]='\0';
						while((pomint<=atoi(index)) &&(options_get_int("js_memory_limit")>=(js_zaflaknuto_pameti/1024)))
						{	snprintf(pomstr,DELKACISLA-1,"%d",pomint);
							pomvar=llookup(pomstr,context->namespace,(plns*)variable->value,context);
							clearvar(pomvar,context);
							pomint++;
						}
						if(options_get_int("js_memory_limit")<(js_zaflaknuto_pameti/1024))
							js_error("Too much memory allocated by javascript",context);
					}
					if(pomvar->handler==atoi(index))pomvar->handler++;
				}

/* Prevention of devastating array from parent - e.g. Cradioptr -> ARRAY -> demolition after dereference */
				pomvar=lookup(MIN1KEY,(plns*)variable->value,context);
				if(pomvar->type!=PARLIST){
					my_internal("Parentlist corrupted!\n",context);
					js_error("Invalid array/member operation ",context);
                                        return;
                                }
				if(!pomvar->value)
				{	pomvar=llookup(index,context->namespace,(plns*)variable->value,context);
					pomvar1=((plns*)variable->value)->ns[pomvar->identifier%HASHNUM];
                                        if(pomvar1&&pomvar1->identifier==pomvar->identifier)
					{	svinec=js_mem_alloc(sizeof(varlist));
						svinec->lekla_ryba=pomvar1;
						svinec->next=context->mrtve_promenne;
						context->mrtve_promenne=svinec;
						((plns*)variable->value)->ns[pomvar->identifier%HASHNUM]=pomvar1->next;
					}
                                        else
                                        {       while((pomvar1)&&(pomvar1->next)&&(pomvar1->next->identifier!=pomvar->identifier))pomvar1=pomvar1->next;
                                                if(pomvar1)
                                                {       if(pomvar1->next)
							{	my_internal("Lost variable!\n",context);
								js_error("Invalid array/member operation ",context);
							        return;
							}
							svinec=js_mem_alloc(sizeof(varlist)); /* Zapamatujeme si promennou, aby se neztratila */
							svinec->lekla_ryba=pomvar1->next;
							svinec->next=context->mrtve_promenne;
							context->mrtve_promenne=svinec;
                                                        pomvar1->next=pomvar1->next->next;
                                                }
                                        }
				}

			}
			delarg(array,context);
			sezvykan=1;
#ifdef JS_DEBUG_2
			printf("\n");
#endif
			retval=js_mem_alloc(sizeof(abuf));
			retval->typ=VARIABLE;
			retval->argument=(long)vari;
			pusha(retval,context);
			context->current->in=0;
			context->current=pullp(context);
			js_mem_free(typ);
			js_mem_free(value);
			js_mem_free(index);
		break;
		default:
			moc("array");
		break;
	}
}

void carka(js_context*context)/*Operator zapominani - carka*/
{	debug("Carka ");
	program(context);/*Patrne se bude dit totez jako kdyz derivujeme program->program program*/
}

void incpost(js_context*context)
{ 	abuf*par;
	float par1;
	int pna2;
	lns* pna;
	debug("Vjizdim do fce incpost ");
	switch(context->current->in)
	{	case 0:	debug("zjistuji argument\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1: debug("pocitam!\n");
			par=pulla(context);
			VARTEST(par,"You can increment only variables!\n");
			pna=(lns*)par->argument;
			pusha(par,context);
			if(pna->type==INTEGER)
			{	par->typ=INTEGER;
				par->argument=pna->value;
				pna->value++;
			}else {
				par->typ=FLOAT;
				par->argument=(long)js_mem_alloc(sizeof(float));
				*(float*)par->argument=par1=vartofloat(pna,context);
				if(pna->type==INTVAR)
				{	if(par1!=MY_NAN && par1!=MY_INFINITY && par1!=MY_MININFINITY)
						par1=par1+1;
					if(par1>MY_MAXDOUBLE)
						par1=MY_INFINITY;
					if(par1<MY_MINDOUBLE)
						par1=MY_MININFINITY;
					set_var_value(pna,FLOAT,(long)&par1,context);
				}
				else
				{
					clearvar(pna,context);
					pna->type=FLOAT;
					pna->value=(long)js_mem_alloc(sizeof(float));
					if(par1!=MY_NAN && par1!=MY_INFINITY && par1!=MY_MININFINITY)
						par1=par1+1;
					if(par1>MY_MAXDOUBLE)
						par1=MY_INFINITY;
					if(par1<MY_MINDOUBLE)
						par1=MY_MININFINITY;
					*(float*)pna->value=par1;
					if(((float)(pna2=(int)rint(par1)))==par1)
					{	pna->type=INTEGER;
						js_mem_free((void*)pna->value);
						pna->value=pna2;
						par->typ=INTEGER;
						js_mem_free((void*)par->argument);
						par->argument=pna2-1;
						debug("je to cele!\n");
					}
				}
			}
			context->current->in=0;
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("incpost");
		break;
	}
}

void decpost(js_context*context)/*Postfixni decrement*/
{ 	abuf*par;
	float par1;
	int pna2;
	lns* pna;
	debug("Vjizdim do fce decpost ");
	switch(context->current->in)
	{	case 0:	debug("zjistuji argument\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=context->current->arg[0];
		break;
		case 1: debug("pocitam!\n");
			par=pulla(context);
			VARTEST(par,"You can increment only variables!\n");
			pna=(lns*)par->argument;
			pusha(par,context);
			if(pna->type==INTEGER)
			{	par->typ=INTEGER;
				par->argument=pna->value;
				pna->value--;
			}else {
				par->typ=FLOAT;
				par->argument=(long)js_mem_alloc(sizeof(float));
				*(float*)par->argument=par1=vartofloat(pna,context);
				if(pna->type==INTVAR)
				{	if(par1!=MY_NAN && par1!=MY_INFINITY && par1!=MY_MININFINITY)
						par1=par1-1;
					if(par1>MY_MAXDOUBLE)
						par1=MY_INFINITY;
					if(par1<MY_MINDOUBLE)
						par1=MY_MININFINITY;
					set_var_value(pna,FLOAT,(long)&par1,context);
				} else
				{	clearvar(pna,context);
					pna->type=FLOAT;
					pna->value=(long)js_mem_alloc(sizeof(float));
					if(par1!=MY_NAN && par1!=MY_INFINITY && par1!=MY_MININFINITY)
						par1=par1-1;
					if(par1>MY_MAXDOUBLE)
						par1=MY_INFINITY;
					if(par1<MY_MINDOUBLE)
						par1=MY_MININFINITY;
					*(float*)pna->value=par1;
					if(((float)(pna2=(int)rint(par1)))==par1)
					{	pna->type=INTEGER;
						js_mem_free((void*)pna->value);
						pna->value=pna2;
						par->typ=INTEGER;
						js_mem_free((void*)par->argument);
						par->argument=pna2-1;
						debug("je to cele!\n");
					}
				}
			}
			context->current->in=0;
			sezvykan=1;
			context->current=pullp(context);
		break;
		default:
			moc("decpost");
		break;
	}
}

/*Myslenka funkce functioncall:
 * 1. Zjistime sve argumenty,
 * 2. zjistime svoje jmeno, (pozor prasarna, napred prolezam vetev 1, pak az 0)
 * 3. najdeme si cilovou funkci
 * 4. ulozime se opet do parbufu a nastavime jako context->current uzel volane
 *    funkce. Bude-li funkce interni, tak ji vyvolame z nejakeho pole. */
void functioncall(js_context*context)
{	abuf*arg,*a1;
	switch(context->current->in)
	{	case 0:	debug("functioncall zjistuje sve argumenty\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=(vrchol*)context->current->arg[1];
		break;
		case 1: debug("functioncall zjistuje sve jmeno\n");
			context->current->in=2;
			pushp(context->current,context);
			context->current=(vrchol*)context->current->arg[0];
		break;
		case 2:	debug("functioncall - volam funkci\n");
			arg=pulla(context);	
			context->current->in=0;
			VARTEST(arg,"You're trying to call not a function!\n");
			if(((lns*)arg->argument)->type==FUNKCE)
			{	context->current=(vrchol*)((lns*)arg->argument)->value; /* Ja se dekuju! Ted to bouchne! */
				/* dalsi operace pobezi uz nad definici fce */
				context->current->in=100;/* je to call, nikoli decl */
			}
			else
			{	if(((lns*)arg->argument)->type==FUNKINT)
					js_intern_fupcall(context,((lns*)arg->argument)->value,(lns*)arg->argument);
				else
				{	/* js_mem_free(arg); Pokus o konsolidaci */
					if(!options_get_bool("js_all_conversions"))
					/*{*/	js_error("Calling not a function ",context);
					delarg(pulla(context),context);
				/*	}*/
					a1=js_mem_alloc(sizeof(abuf));
					a1->typ=UNDEFINED;
					a1->argument=0;
					pusha(a1,context);
					/* context->current->in==0 viz vyse */
					context->current=pullp(context);
				}
				sezvykan=1;
			}
			js_mem_free(arg);
		break;
		default:
			moc("functioncall");
		break;
	}
}

/*void convert(js_context*context)
{	abuf*vysl;
	long key;
	lns* variable;
	debug("Vstupuji do funkce convert ");
	if(context->current->in==0){
		debug("jedu dolu\n");

		context->current->in=1;
		if(context->current->arg[0]){
			pushp(context->current,context);
			context->current=(vrchol*)context->current->arg[0];
		}else
		{
			my_internal("Bug! Fce convert nema syna ve strome!\n",context);
			js_error("Trying to convert nothing ",context);
			return;
		}
	} else
		if(context->current->in==1)
		{
			debug("current - jedu nahoru - konvertim!\n");
			vysl=pulla(context);
			switch(vysl->typ)
			{	case INTEGER:
				break;
				case FLOAT:
				break;
				case BOOLEAN:
				break;
				case UNDEFINED:
				break;
				case NULLOVY:
				break;
				case STRING:
				break;
				case VARIABLE:
					debug("Tohle je experimentalni!\n");
					key=vysl->argument;
					variable=lookup(key,context->lnamespace,context);
					switch(vysl->typ=variable->type){
						case INTEGER:
						case BOOLEAN:
						case UNDEFINED:
						case NULLOVY:
							debug("snadna konverze\n");
							vysl->argument=variable->value;
						break;
						case FLOAT:
							debug("konvertuji float\n");
							vysl->argument=(long)js_mem_alloc(sizeof(float));
							*(float*)(vysl->argument)=*(float*)(variable->value);
						break;
						case STRING:
							debug("konvertuji string\n");
							vysl->argument=(long)js_mem_alloc(strlen((char*)variable->value)+1);
							strcpy((char*)vysl->argument,(char*)variable->value);
						break;
						default: 
							my_internal("Divna konverze variable!\n",context);
							js_error("Invalid variable conversion ",context);
							return;
						break;
					}
				break;
				default: 
					my_internal("Tato konverze jeste neni napsana!\n",context);
					js_error("Invalid variable conversion ",context);
					return;
				break;
			}
			context->current->in=0;
			pusha(vysl,context);
			sezvykan=1;			
			context->current=pullp(context);

		}
		else moc("convert");
} */

void variable(js_context*context)/*Tohle bude deklovat promenne*/
{	abuf*var;
	debug("Vjizdim do funkce variable ");
	switch(context->current->in)
	{	case 0:	debug("hledam ji\n");
			context->current->in=1;
			pushp(context->current,context);
			context->current=(vrchol*)context->current->arg[0];
		break;
		case 1:	debug("uz ji mam!\n");
			context->current->in=0;
			var=pulla(context);

			if(var->typ==VARIABLE)
				create(((lns*)var->argument)->identifier,context->lnamespace,context);
			else	if(((vrchol*)context->current->arg[0])->opcode!=TLocAssign){
				my_internal("Internal:Divny typ v inicializaci promenne!\n",context);
				js_error("Error inicializing variable ",context);
				return;
			}
			
			delarg(var,context);
			sezvykan=0;
			context->current=(vrchol*)context->current->arg[1];
		break;
		default:
			moc("variable");
		break;
	}
}

/* Podle opcode soucasneho uzlu zavola obsluhujici funkci. Funkce si v uzlu
 * pamatuje, kolik uz toho napocitala a uhodne z toho i kolik toho ma na
 * argbufferu. Pokud ne, je maler.
 */

void zvykni(js_context*context)
{	abuf*pom=0;
	if(!context->current)
	{	sezvykan=1;/* Rozhodne se deje neco divneho */
		debug("Zahazuji prazdny pytlik!\n");
		context->current=pullp(context);
		if(! context->current)debug("Deje se neco moc divneho!\n");
		pom=js_mem_alloc(sizeof(abuf));
		pom->typ=UNDEFINED;
		pom->argument=0;
		pusha(pom,context); /* Je to tu kvuli prazdnym vetvim napr. u if, takze predstirame, ze neco probehlo */
		return;
	}
	context->zaplatim=1; /* Tady jsem mel bugu - skoro primo pod nosem Pivrncovi - to by to dopadlo B-) */
	/* Za tohle by me tendle chlapek zabil! Lezu po strome cyklem, rekurzim si sam...                  `,".   */
	switch(context->current->opcode)		/*                 D~~)w  */
	{	case TAND:	bitand(context);	/*                /    |  */
		break;					/*             /'m_O   /  */
		case TANDAND:	logand(context);	/*           /'."//~~~~\  */
		break;					/*           `\/`\`--') , */
		case TANDEQ:	andassign(context);	/*             /  `~~~  | */
		break;					/*            |         | */
		case TDELETE:	delete(context);	/*            |         , */
		break;					/*            `_'p~~~~~/  */
		case TDIVEQ:	divassign(context);	/*              .  ||_|   */
		break;					/*          `  .  _|| |   */
		case TEQ:	assign(context);	/*           `, ((____|   */
		break;									
		case TEQEQ:	equal(context);
		break;
		case TEXCLAM:	not(context);
		break;
		case TEXCLAMEQ:	notassign(context);
		break;
		case TFALSELIT:	false(context);
		break;
		case TIF:	myif(context);
		break;
		case TMINEQ:	unassign(context);
		break;
		case TMINMIN:	decpref(context);	
		break;
		case TMINUS:	minus(context);
		break;
		case TMOD:	modulo(context);
		break;
		case TMODEQ:	modassign(context);
		break;
		case TNULLLIT:	mynull(context);
		break;
		case TNUMLIT:	number(context);
		break;
		case TOR:	bitor(context);
		break;
		case TOREQ:	orassign(context);
		break;
		case TOROR:	logor(context);
		break;
		case TPLUS:	plus(context);
		break;
		case TPLUSEQ:	plusassign(context);
		break;
		case TPLUSPLUS:	incpref(context);
		break;
		case TSHL:	lt(context);
		break;
		case TSHLEQ:	le(context);
		break;
		case TSHLSHL:	shl(context);
		break;
		case TSHLSHLEQ:	shlshleq(context);
		break;
		case TSHR:	gt(context);
		break;
		case TSHREQ:	ge(context);
		break;
		case TSHRSHR:	shr(context);
		break;
		case TSHRSHREQ:	shrshreq(context);
		break;
		case TSHRSHRSHR:	shrshrshr(context);
		break;
		case TSTRINGLIT:	string(context);
		break;
		case TTHIS:	this(context);
		break;
		case TTHREERIGHTEQUAL:	threerighteq(context);
		break;
		case TTIMESEQ:	mulassign(context);
		break;
		case TTRUELIT:	true(context);
		break;
		case TTYPEOF:	mytypeof(context);
		break;
		case TVAR:	var(context);
		break;
		case TVOID:	myvoid(context);
		break;
		case TXOR:	xor(context);
		break;
		case TXOREQ:	xorassign(context);
		break;
		case TWHILE:	mywhile(context);
		break;
		case TFOR1:	for1(context);
		break;
		case TFOR2:	for2(context);
		break;
		case TFOR3:	for3(context);
		break;
		case TCONTINUE:	pokracuj(context);
		break;
		case TBREAK:	breakni(context);
		break;
		case TRETURN:	vrat(context);
		break;
		case TWITH:	with(context);
		break;
		case TIDENTIFIER:	identifier(context);
		break;
		case TPROGRAM:	program(context);
		break;
		case TFUNCTIONDECL:	funkce(context);
		break;
		case TTIMES:	multiply(context);
		break;
		case TSLASH:	divide(context);
		break;
		case TCOMPL:	complement(context);
		break;
		case TParameterList:	parametry(context);
		break;
		case TArgumentList:	parametry(context);
		break;	/*Doufam, ze parametry se od argumentu vubec nelisi!*/
		case TStatements:	statementy(context);
		break;
		case TUNMIN:	zaporne(context);
		break;
		case TTHISCCall:	thisccall(context);
		break;
		case TIdCCall:	idccall(context);
		break;
		case TECCall:	eccall(context);
		break;
		case TConsCall:	conscall(context); /* Zavola konstruktor a ty okolo to je podobne */
		break;
		case TMember:	member(context);
		break;
		case TArray:	array(context);
		break;
		case TCARKA:	carka(context);
		break;
		case TPLUSPLUSPOST:	incpost(context);
		break;
		case TMINMINPOST:	decpost(context);
		break;
		case TFunctionCall:	functioncall(context);
		break;
/*		case TCONVERT:	convert(context);
		break;*/
		case TVariables:	variable(context);
		break;
		case TLocAssign:	localassign(context);
		break;
		default:
			my_internal("Error! Udelal jsem si spatny intercode; tento opcode neznam!\n",context);
			js_error("Error in javascript code ",context);
			return;
		break;
	}
}

void ipret(js_bordylek*bordylek)
{	int whatnow,timerno=0;
	abuf*pomarg;
	js_context*context=(js_context*)bordylek->context;
	js_bordylek*pombordylek;
	*bordylek->mytimer=-1;
	js_mem_free(bordylek);
	sezvykan=0; /* promenna sezvykan se nastavi pri ceste stromem nahoru na jednotku. */
/*	context->t=-1; timery jsou reseny jinak! */
	if(context->jsem_dead)
	{	debug("Poustis mrtvy zabascript!\n");
		context->utraticka=1;
	}	
	else {
		schedule=0; /* schedulneme po urcitem poctu zvyknuti */
		while((whatnow=(!sezvykan || (context->current) ||(context->parbuf))) && schedule < KROKU && !js_durchfall){
			zvykni(context);
			schedule++;
                        if(options_get_int("js_memory_limit")<(js_zaflaknuto_pameti/1024))
			{	js_error("Too much memory allocated by javascript",context);
				js_cistka_kontextu(context);
#ifdef RASODEBUG
				printf("Zaplacnuto %d bytu!\n",js_zaflaknuto_pameti);
#endif				
			}
#ifdef RASODEBUG
			else	printf("Zaflaknuto %d bytu.\n",js_zaflaknuto_pameti);
#endif
		}
		if(!whatnow && !js_durchfall) {
			if(context->argbuf)
			{	pomarg=pulla(context);/*zrusime posledni neterminal;*/
				if(pomarg->typ==BOOLEAN && pomarg->argument==FALSE)
					context->zaplatim=0;
				else	context->zaplatim=1;
				delarg(pomarg,context);
			}
#ifdef RASODEBUG
			pulla(context);
			debug("Nemas nahodou buffer leak?\n");
			printf("Vracim %d\n",context->zaplatim);
#endif
			context->running=0;
			js_volej_kolbena(context);
		} else{ 
			if(!js_durchfall){
				debug("Scheduluji!\n");
				while(timerno<TIMERNO &&(context->t[timerno]!=-1))timerno++;
				if(timerno>=TIMERNO)
				{	js_error("Too many timers",context);
					return;
				}
				pombordylek=js_mem_alloc(sizeof(js_bordylek));
				pombordylek->context=context;
				pombordylek->mytimer=&context->t[timerno];
				context->bordely[timerno]=pombordylek;
				context->t[timerno]=install_timer(1,(void(*)(void*))ipret,pombordylek);
			}
		}
		js_durchfall=0;
	}
	if(context->utraticka)
	{
#ifdef RASODEBUG
		printf("Vracim %d\n",context->zaplatim);
#endif
		context->utraticka=0;
		context->running=0;
		js_volej_kolbena(context);
	}
}

#endif
