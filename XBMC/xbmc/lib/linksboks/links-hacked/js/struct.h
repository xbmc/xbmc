/* struct.h
 * (c) 2002 Martin 'PerM' Pergel && Petr 'Brain' Kulhavy
 * This file is a part of the Links program, released under GPL.
 */

#include "../links.h"

#ifdef HAVE_MATH_H
#include <math.h>
#endif

#ifdef HAVE_IEEE_H
#include <ieee.h>
#endif

extern void* js_js_temp_var1;
extern size_t js_js_temp_var2;
extern int js_zaflaknuto_pameti;

#define J_A_S ((sizeof(int) + 15) & ~15)

#define js_mem_alloc(a) ((js_js_temp_var1=mem_alloc((js_js_temp_var2=(a))+J_A_S)),(js_zaflaknuto_pameti+=js_js_temp_var2),(*(int*)js_js_temp_var1=js_js_temp_var2),(void*)(((char*)js_js_temp_var1)+J_A_S))

#define js_mem_free(a) ((js_zaflaknuto_pameti-=*(int*)((char*)((js_js_temp_var1=(a)))-J_A_S)), (mem_free((void*)((char*)js_js_temp_var1-J_A_S))))

#if (DEBUGLEVEL >= 2)
#define DEBUGMEMORY
#define DEBUZ_KRACHY
#endif

#define float double

#define CELE 1
#define NECELE 2

#define TERM 0
#define NETERM 1

typedef struct _js_id_name {	long klic;
				char*jmeno;
				struct _js_id_name*next;
			  } js_id_name;


typedef struct {int typ;
		union {int cele;
			float necele;
		}nr;}cislo; 
#define HASHNUM 128
#define MAGIC 17
#define TIMERNO 128
typedef struct _vrchol { long opcode; int in;
/*			 long term; terminal nebo neterminal*/
			 struct _vrchol* prev;
			 void* arg[7]; /*mozna to tu bude chtit typecast*/
			 struct _vrchol * otec;
/*			 int prolezany; uvidime, jestli se to hodi - bude urcovat v kolikatem synovi jsme ted brouzdali*/
			 long lineno;
			} vrchol;

typedef struct _ns{	long identifier;
			vrchol*kdeje;
			struct _ns* next;
		 }ns;

typedef struct _argument_buffer {
		long argument;
		long a1;/*Almost never used second argument. Is here because of arguments submitting to function.*/
		long typ;
		struct _argument_buffer * next;
	}abuf;

typedef struct _parent_buffer {
		vrchol* parent;
		int in;
		void* in1;
		struct _parent_buffer *next;
	}pbuf;

typedef struct localnamespace {
		long identifier;
		long mid; /* Jako Mikulasovo ID - to je byro! B;-) */
		long type;
		long value;/*tohle se bude casto typecastovat na pointer*/
		long handler; /* smysl bude mit jen u vnitrnich pnych */
		long index; /* smysl ma jen u selectitek */
		struct localnamespace* next;
	}lns;

typedef struct ptrlns {/*jako pointer na localnamespace*/
		lns** ns;
		long mid; /* Jako Mikulasovo ID */
		long handler;
		struct ptrlns* next;
	}plns;

typedef struct _bordel_list { /* jako seznam bordelu urceneho k pozdnimu svozu */
		void* binec;
		struct _bordel_list * next;
	} borlist; /* Uzije se pro veci, ktere potrebujeme nekdy asynchronne obslouzit,
		      ale dosud k tomu nebyla prilezitost. */

typedef struct _variablovy_list {
		lns* lekla_ryba;
		struct _variablovy_list *next;
	} varlist;

/* Struktura, ktera bude predavana funkci ipret. Je potreba, aby context
 * zustaval stejny, zatimco timerovy pointer se bude lisit. mytimer ukazuje
 * na cislo naseho timeru, ktere mame za povinnost po startu snulovat. Jeste 
 * bude potreba zmenit struktury fax_me_tender_data tak,
 * ze tam bude stejny pointer.
 */
typedef struct javascript_bordylek
	       {
		 void*context;
		 int*mytimer;
	       } js_bordylek;

struct jsnslist{plns* pns;
		struct jsnslist* next;
	};

#define PRO_DEBILY 1

typedef struct javascript_context 
	       { 
		 void * ptr ;
		 int running; 
		 int t[TIMERNO];
/*		 int timernum;*/
		 js_bordylek* bordely[TIMERNO];
		 int upcall_timer;
#ifdef PRO_DEBILY
		 int zlomeny_ramecek_jako_u_netchcipu;
#endif
		 void* upcall_data;
		 int upcall_typek;	 
		 void (*callback)(void *);
		 long id;       /* id of frame in which javascript is running */
		 long js_id;	/* jednoznacny identifikator kontextu */
		 unsigned char *code;
		 int codelen; /* Az sem jsou to Mikulasovy napady */
		 
		 int lock; /* bezime? */
		 vrchol* js_tree;
		 long lineno; /* Celkem lajn kodu */
		 vrchol* current;/* Tohle ted zpracovavame*/
		 abuf* argbuf;
		 pbuf* parbuf;
		 int jsem_dead; /* indikator mrtvosti javascriptu */
		 plns* lnamespace; /* aktualni addrspace */
		 struct jsnslist * first_ns;
		 struct jsnslist** last_ns;
		 borlist* bordel;
		 varlist* mrtve_promenne; /* pne, ktere jeste bude treba */
		 int zaplatim; /* Rika, jestli vysledek js je true(!0)/false(0) */
		 unsigned char *cookies;	/* alokovany string se susenkama vznikly od zapisovani do document.cookie */
		 int utraticka; /* rika, jestli se autor rozsoup s errorem */
		 plns*fotrisko; /* pointer na HLAVNI addrspace */
		 int depth;
		 int depth1;
		 /*jeste tu bude spojak returnu a spojak argumentu. Kdyz bude spojak returnu prazdny, tak nebezime.*/
		 js_id_name *namespace [HASHNUM];
		/* Tady bude jeste fura dalsich veci, ale ted si nevzpomenu, 
		   ktere. */
		}js_context;


typedef struct _parlist {
	lns* parent;
	struct _parlist* next;
} parlist;

vrchol*terminal(void);
vrchol*neterminal(void);
extern js_context* js_create_context(void*,long);
extern void js_destroy_context(js_context*);
void ipret(js_bordylek*);
lns*lookup(long,plns*,js_context*);
lns*create(long,plns*,js_context*);
plns*newnamespace(abuf*,abuf*,js_context*);
void deletenamespace(plns*,js_context*);
#define RESOLV(A) if((A->typ==VARIABLE)||(A->typ==INTVAR)) \
	vartoarg((lns*)A->argument,A,context);

lns* buildin(char*,js_id_name**,plns*,js_context*);
void js_warning(char*,int,js_context*);
void zrus_strom(vrchol*);
#define MAX_FCALL_DEPTH 50

#ifdef DEBUZ_KRACHY

#include <stdio.h>
extern FILE*js_file;
#define my_internal(a,b) {js_file=fopen("links_debug.err","a"); \
	fprintf(js_file,"%s\n On: %s \n Code: %s \n V lajne: %d, z %d, delka kodu je %d \n",a,((struct f_data_c*)b->ptr)->loc->url,b->code,(int)b->current->lineno,(int)b->current->lineno,(int)b->codelen);\
	fclose(js_file); \
	internal(a);}
#else
#define my_internal(a,b) internal(a,b);
#endif
void js_volej_kolbena(js_context*context);
void js_spec_vykill_timer(js_context*context,int i);
#define TYPEK_NIC 1
#define TYPEK_STRING 2
#define TYPEK_INT_STRING 3
#define TYPEK_2_STRINGY 4
#define TYPEK_STRING_2_LONGY 5

extern char* js_temp_var_for_stracpy1;

#define stracpy1(a)	((js_temp_var_for_stracpy1=(a))?strcpy(js_mem_alloc(strlen(js_temp_var_for_stracpy1)+1),js_temp_var_for_stracpy1):NULL)

/* static inline unsigned char * stracpy1(unsigned char*a)
{	unsigned char*kopie;
	if(a)
	{	kopie=js_mem_alloc(strlen(a)+1);
		strcpy(kopie,a);
		return kopie;
	}
	else	return 0;
}*/

#ifndef MAXDOUBLE
#define MAXDOUBLE 3.40282347e+38
#endif

/* mikulas: don't use bigger numbers  -- it would crash on Alpha */

#define MY_MAXDOUBLE (MAXDOUBLE*0.01)
#define MY_MINDOUBLE (-MAXDOUBLE*0.01)
#define MY_NAN (MAXDOUBLE*0.03)
#define MY_INFINITY (MAXDOUBLE*0.02)
#define MY_MININFINITY (-MAXDOUBLE*0.02)

void js_cistka_kontextu(js_context*);
