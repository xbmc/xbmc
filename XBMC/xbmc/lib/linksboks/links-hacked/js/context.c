/* context.c
 * (c) 2002 Martin 'PerM' Pergel
 * This file is a part of the Links program, released under GPL.
 */

#include "../cfg.h"

#ifdef JS

#include "struct.h"
#include "tree.h"
#include "typy.h"
#include "ipret.h"
#include "builtin.h"
#define idebug(a) printf(a)
#undef idebug
#define idebug(a) 

extern long MIN1KEY;
#include "ns.h"

extern int js_durchfall;

/* typedef struct _vrchol { long opcode; int nargs;
 *                          void*arg[6];
 *                          struct _vrchol * otec;
 *                          int prolezany; } vrchol;
 * 
 * typedef struct _ns{     long identifier; vrchol*kdeje; struct _ns*next;}ns;
 * typedef struct javascript_context
 *                { intercode* start,*now;
 *                  int lock; ns*namespace[HASHNUM]; }js_context;
 */ 

js_context* js_create_context(void*p,long id)/*nevim co budu delat s tim p*/
{	int i=0;
	lns** pomlns;
	static long links__browser_pro_opravdove_muze_a_pochlapenou_kolegyni_Dolezalovou=0;
	
	js_context*context=js_mem_alloc(sizeof(js_context));
	if(!context) return NULL;
	context->running=0;
/*	context->start=context->now=0;*/
	context->lock=0;/*nebezime*/
	while(i<HASHNUM)
		(context->namespace)[i++]=0;/*Semhle se pridelaji jmena, ktera
je potreba umet z fabriky (OnClick apod.)*/
	context->js_tree=0; /* Strom je zatim prazdny */
	context->parbuf=0;
	context->argbuf=0;
	context->current=0;
	context->lineno=0;
	context->bordel=0;
	context->mrtve_promenne=0;
	context->depth=0;
	context->depth1=0;
	context->first_ns=0;
	context->last_ns=&context->first_ns;
	context->fotrisko=context->lnamespace=js_mem_alloc(sizeof(plns));
	context->lnamespace->next=0;
	if(options_get_bool("js_global_resolve"))
		context->lnamespace->mid=js_upcall_get_window_id(p);
	else
		context->lnamespace->mid=0;
	context->lnamespace->handler=js_upcall_get_document_id(p);
	context->code=0; /* Sanity nula */
	context->codelen=0;
	context->utraticka=0;
#ifdef PRO_DEBILY
	context->zlomeny_ramecek_jako_u_netchcipu=0;
#endif
	context->cookies=NULL;
	context->zaplatim=0;/* Co bych platil! Jeste jsem nebezel! */
	pomlns=context->lnamespace->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
	i=0;
	while(i<HASHNUM)/*Tvorime globalni prostor pro promenne*/
	{	pomlns[i++]=0;
	}
	
	context->ptr = p; /* Mikulas following section - almost copied */
	for(i=0;i<TIMERNO;context->t[i++]=-1);
	context->upcall_timer = -1;
	context->jsem_dead=0;
	context->js_id=links__browser_pro_opravdove_muze_a_pochlapenou_kolegyni_Dolezalovou++;
	context->id = id; /* End of Mikulas compatibility section */
	add_builtin(context);
	return context;
}

void vymaz(js_id_name*namespace[]) /* There's a man going round taking names. */
{	int i=0;
	js_id_name*a,*b;
	while(i<HASHNUM)
	{	if((a=namespace[i]))
			while(a)
			{	b=a->next;
				js_mem_free(a->jmeno);
				js_mem_free(a);
				a=b;
			}
		i++;
	}
}

void zrus_strom(vrchol*pom_vrchol) /* deletes intercode-tree */
{	vrchol*p;
	while(pom_vrchol){
		p=pom_vrchol->prev;
		if(pom_vrchol->opcode==TNUMLIT)js_mem_free(pom_vrchol->arg[0]);
		if(pom_vrchol->opcode==TSTRINGLIT)js_mem_free(pom_vrchol->arg[0]);
		if(pom_vrchol->opcode==TFOR3)if(pom_vrchol->arg[3])js_mem_free(pom_vrchol->arg[3]);
		js_mem_free(pom_vrchol);
		pom_vrchol=p;
	}/*deallokujeme strom*/
}

void del_ns(plns*,js_context*);

void total_clearvar(lns*pna,js_context*context,plns*fotri)
{	lns*zrusit;
/*	plns*fotri;*/
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
/*			fotri=((plns*)pna->value)->next;*/
			while(fotri && fotri!=(plns*)pna->value)
				fotri=fotri->next;
			if(!fotri /*&& ((plns*)pna->value)->next*/)
			{	del_ns((plns*)pna->value,context);
			} else
			{	zrusit=lookup(MIN1KEY,(plns*)pna->value,context);
				if(zrusit->type!=PARLIST)
				internal("Parent-list corrupted!\n");
				delete_from_parlist(pna,zrusit);
				/* Pokus na nas ukazuje nej. parent, konec. */
			}
		break;
		default:
			internal("Aiee! Unknown variable type!\n");
		break;
	}
/*	js_mem_free(pna);*/
}

void clearvars(parlist*list);

void del_ns(plns*pns,js_context*context) /* deletes table of keys and values of vars */
{	int i=0;
	lns**ns; /* Pozor mozna prasarna! */
	lns*a,*b;
	ns=pns->ns;
	if(!ns)	my_internal("Del_ns is trying to free NULL addrspace!\n",context);
	while(i<HASHNUM)
	{	if((a=ns[i++]))
		while(a && a->type != PARLIST)
			a=a->next;
		if(a) {	clearvars((parlist*)a->value);
			a->value=0;
			a->type=UNDEFINED;
		}
	}
	
	i=0;
			
	while(i<HASHNUM)
	{	if((a=ns[i]))
			while(a)
			{	b=a->next;
				if(a->type==PARLIST) 
					my_internal("Too many parlists!\n",context) /* ;*/
				else	total_clearvar(a,context,pns);
				js_mem_free(a);
				a=b;
			}
		i++;
	}
	js_mem_free(ns);
	js_mem_free(pns);
}

/* Tohle by melo kontext trochu procistit, aby utocnik nemohl vykillit
   vsechny kontexty tim, ze pretece a ostatni jak budou alokovat, tak
   do toho ridkeho trusu sahnou taky. */

void js_cistka_kontextu(js_context*context)
{	borlist* na_bordel=context->bordel,*pom;
	abuf*bufer;
	struct jsnslist* odlozene_deti;
	varlist*svinec;
	idebug("Cistka kontextu!\n");
	while(na_bordel){
                js_mem_free(na_bordel->binec);
                pom=na_bordel->next;
                js_mem_free(na_bordel);
                na_bordel=pom;
        }
	context->bordel=0;
        while(pullp(context))idebug("Killing parent in buffer!\n");
        while((bufer=pulla(context)))
        {       delarg(bufer,context);
                idebug("Killing buffer entry!\n");
        } /*Neco takoveho tu bude potreba, ale lepe a radostneji :-) */
	while(context->first_ns)
        {       odlozene_deti=context->first_ns->next;
                deletenamespace(context->first_ns->pns,context);
                js_mem_free(context->first_ns);
                context->first_ns=odlozene_deti;
        }
	zrus_strom(context->js_tree);
	context->js_tree=0;
	context->current=0;
	/* Carovani se stromem alla Rumcajs */

        while((svinec=context->mrtve_promenne))
        {       context->mrtve_promenne=context->mrtve_promenne->next;
                clearvar(svinec->lekla_ryba,context);
                js_mem_free(svinec->lekla_ryba);
                js_mem_free(svinec);
                idebug("Killim leklou promennou!\n");
        }
}

void js_destroy_context(js_context* context)
{	plns *ns1,*ns2;
	abuf*bufer;
	int timerno;
	struct jsnslist* odlozene_deti;
	borlist*pom,*na_bordel=context->bordel;
	varlist*svinec;
	lns*promenna;
#if 0		/* PerM je cune. Hrabe kam nema a pak to quli tomu pada */
	struct __warning_list_t *wokno;
/*	js_durchfall=0;	*/
	foreach(wokno,warning_list)/* Smahneme Brainovi warningy B-) */
	{
		struct __warning_list_t *w=wokno;
		wokno=wokno->prev;
		del_from_list(w);
		js_mem_free(w);
	}
#endif

	while(na_bordel){
		js_mem_free(na_bordel->binec);
		pom=na_bordel->next;
		js_mem_free(na_bordel);
		na_bordel=pom;
	}
	while(pullp(context))idebug("Killing parent in buffer!\n");
	while((bufer=pulla(context)))
	{	delarg(bufer,context);
		idebug("Killing buffer entry!\n");
	} /*Neco takoveho tu bude potreba, ale lepe a radostneji :-) */

	promenna=lookup(MIN1KEY,context->lnamespace,context);
	if(promenna->type!=PARLIST)
		my_internal("Parentlist corrupted!",context);
	if(!promenna->value)
		deletenamespace(context->lnamespace,context);

	while(context->first_ns)
	{	odlozene_deti=context->first_ns->next;
		deletenamespace(context->first_ns->pns,context);
		js_mem_free(context->first_ns);
		context->first_ns=odlozene_deti;
	}
	ns1=context->fotrisko;
	while(ns1){
		ns2=ns1->next;
		del_ns(ns1,context);
		ns1=ns2;
	}
	/* To jsou deallokace promennych */
	zrus_strom(context->js_tree);

	while((svinec=context->mrtve_promenne))
        {	context->mrtve_promenne=context->mrtve_promenne->next;
                clearvar(svinec->lekla_ryba,context);
                js_mem_free(svinec->lekla_ryba);
                js_mem_free(svinec);
                idebug("Killim leklou promennou!\n");
        }

	if(context->code)js_mem_free(context->code);
	else	idebug("Strange things happen!\n");
	vymaz(context->namespace);
	for(timerno=0;timerno<TIMERNO;timerno++)
		if(context->t[timerno]!=-1)
		{	kill_timer(context->t[timerno]);
			context->t[timerno]=-1;
			idebug("Killim timer!\n");
			js_mem_free(context->bordely[timerno]);
		}
	if(context->upcall_timer!=-1)
		js_spec_vykill_timer(context,1);
	if(context->depth)idebug("Za jizdy z vlaku NE-SE-SKA-KO-VAT!\n");
	if(context->depth1)idebug("Za jizdy z vlaku NESESKAKOVAT!\n");
	if (context->cookies)js_mem_free(context->cookies);
	js_mem_free(context); /* A je to*/
}

#endif
