/* ns.c
 * Javascript namespace
 * (c) 2002 Martin 'PerM' Pergel
 * This file is a part of the Links program, released under GPL.
 */

/*Kterak udrzovat prostor s promennymi:
  Bude to hashove pole. Kolize bude resena spojakem. Na tech spojacich udelam
  MFR (to je to, za co jsem loni letel od zkousky)*/
#include "../cfg.h"

#ifdef JS

#include <stdio.h>
#include "struct.h"
#include "typy.h"
#include "ns.h"
#include "ipret.h"
#include "builtin_keys.h"
#undef debug
#define debug(a) fprintf(stderr,a)
#undef debug
#define debug(a) 
extern long MIN1KEY;

/* Funkce, ktere vyhledavaji v "adresnych prostorech" promenne. Jedna sada
 * ma promenne jenom nachazet, cast z nich kdekoliv, cast z nich jenom
 * lokalne, druha cast z nich ma ty promenne vyrobit (existuje-li dana
 * promenna, pak ji budto smazat, nebo nechat). Tyto funkce se pouzivaji
 * pomerne masivne. Adresne prostory jsou zatim v pevne velikosti
 * (HASHNUM). Bude-li cas, tak udelam adresne prostory velikosti od
 * 16 do 128 (ted je to natvrdo 128). Hashuje se tu, aby se promenne 
 * nevyhledavaly prilis dlouho. Maler je, ze ja jsem zavedl interni 
 * reprezentaci identifikatoru longem (na radu Mgr. Bednarka), zatimco
 * Mikulas reprezentuje vsechno stringem, takze kdyz se potrebuju dotazat,
 * jestli nekdo v dokumentu neslysel o objektu _takhle_ pojmenovanem, tak
 * musim zase prevest ten long na string (=> vyhledavani v hashovem poli).
 * Aby bylo hledani rychlejsi, je tu implementovano MFR (move front rule).
 * Promenna, ktera se pouziva casteji, "vybubla" v tom spojaku na zacatek.
 * Co je MFR, viz Datove struktury (RNDr. Koubek)
 */

lns* loklookup(long key,plns*pns,js_context*context)
{	lns*current=(pns->ns)[key%HASHNUM],*pom=0;
	long * sperhaky=0,pomkey=key/HASHNUM;
	int delka_vole,pomint;
	plns* pomns;
	char pomstr[DELKACISLA];
	js_id_name*zamecnictvi_u_sperhaku=context->namespace[key%HASHNUM];
	debug("Loklookup: ");
	while(current &&current->identifier!=key)
		current=(pom=current)->next;
	if(current)
	{       debug("promenna nalezena\n");
		if(pom){ /* MFR - to by Rumcajs mrkal na drat! */
			pom->next=current->next;
			current->next=(pns->ns)[key%HASHNUM];
			(pns->ns)[key%HASHNUM]=current;
		}
		
	}
	else {  debug("tvorim novou promennou\n");
		current=js_mem_alloc(sizeof(lns));
		current->identifier=key;
		current->mid=pns->mid;
		current->next=pns->ns[key%HASHNUM];
		pns->ns[key%HASHNUM]=current;
		current->type=UNDEFINED;
		current->value=0;
		current->handler=0;
		while(zamecnictvi_u_sperhaku && zamecnictvi_u_sperhaku->klic!=pomkey)
			zamecnictvi_u_sperhaku=zamecnictvi_u_sperhaku->next;
		if(!zamecnictvi_u_sperhaku)
			my_internal("Out of namespace!",context);
		if(pns->mid) sperhaky=jsint_resolve(context->ptr,pns->mid,zamecnictvi_u_sperhaku->jmeno,&delka_vole);
		if(sperhaky)
		{	current->mid=sperhaky[0];
			current->handler=pns->handler;
			current->type=INTVAR;
			switch(jsint_object_type(sperhaky[0]))
			{	case JS_OBJ_T_TEXT:	current->value=Ctextptr;
				break;
				case JS_OBJ_T_PASSWORD:	current->value=Cpasswdptr;
				break;
				case JS_OBJ_T_TEXTAREA:	current->value=Ctextarptr;
				break;
				case JS_OBJ_T_CHECKBOX:	current->value=Cchkboxptr;
				break;
				case JS_OBJ_T_RADIO:	
					current->type=ARRAY;
					current->value=(long)(pomns=js_mem_alloc(sizeof(plns)));
					pomns->mid=0;
					pomns->handler=pns->handler;
					pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
					pomint=0;
					while(pomint<HASHNUM)pomns->ns[pomint++]=0; /* Kriziku! B-( */
					pomns->next=context->lnamespace;
					/* Tady se budou vstavat radiocudlitka do pole.*/
					pomint=0;
					while(pomint<delka_vole)
					{	snprintf(pomstr,DELKACISLA,"%d",pomint);
						pomstr[DELKACISLA-1]='\0';
						pom=buildin(pomstr,context->namespace,pomns,context);
						pom->type=INTVAR;
						pom->value=Cradioptr;
						pom->mid=sperhaky[pomint];
						pom->handler=pns->handler;
						pomint++;
					}
					pom=create(MIN1KEY,pomns,context);
					pom->type=PARLIST;
					pom->value=0;
					add_to_parlist(current,pom);
					pom=buildin("length",context->namespace,pomns,context);
					pom->type=INTVAR;
					pom->value=Clength;
					pom->mid=0;
					pom->handler=delka_vole;
					pom=buildin("toString",context->namespace,pomns,context);
					pom->type=FUNKINT;
					pom->value=CtoString;
					pom->handler=C_OBJ_radio;
				break;
				case JS_OBJ_T_SELECT:	current->value=Cselectptr;
				break;
				case JS_OBJ_T_SUBMIT:	current->value=Csubmitptr;
				break;
				case JS_OBJ_T_RESET:	current->value=Cresetptr;
				break;
				case JS_OBJ_T_HIDDEN:	current->value=Chiddenptr;
				break;
				case JS_OBJ_T_BUTTON:	current->value=Cbuttonptr;
				break;
				case JS_OBJ_T_FORM:	current->value=Cformptr;
				break;
				case JS_OBJ_T_FRAME:	current->value=Cframeptr;
							current->handler=current->mid;
				break;
				case JS_OBJ_T_IMAGE:
							current->value=Cimageptr;
				break;
				default:	my_internal("Semerad! to je typek!\n",context); 
				break;				/* miEro je jeste vetsi typek!! */
			}
/*			current->handler=0; */
			mem_free(sperhaky);
		}
	}
	return current;
}
	
lns * lookup(long key,plns*pns,js_context*context)
/*Vyhleda v prostoru na promenne nejblizsi promennou s klicem key. 
Neexistuje-li takova, tak ji vyrobi v globalnim ns*/
{	lns *current=0,*pom=0;
	long pomkey=key/HASHNUM;
	int konec=0,delka_vole,pomint;
	plns * pomns;
	long *sperhaky=0;
	char pomstr[DELKACISLA];
	js_id_name*zamecnictvi_u_sperhaku=context->namespace[key%HASHNUM];
	debug("Lookup: ");
	while(zamecnictvi_u_sperhaku && zamecnictvi_u_sperhaku->klic!=pomkey)
		zamecnictvi_u_sperhaku=zamecnictvi_u_sperhaku->next;
	if(!zamecnictvi_u_sperhaku)
		my_internal("Out of namespace!",context);
	while(!current && !konec && (!sperhaky))
	{	debug("prohlizim... ");
		pom=0;
		current=(pns->ns)[key%HASHNUM];
		while(current && (current->identifier!=key))
			current=(pom=current)->next;
		if(pns->mid)sperhaky=jsint_resolve(context->ptr,pns->mid,zamecnictvi_u_sperhaku->jmeno,&delka_vole);
		if(!(pns->next)) konec=1;
		else	if(!current) /* dulezite je, ze zachovame lns ve kterem se to naslo */
				pns=pns->next;
	}
	if(current)
	{	debug("promenna nalezena\n");
		if(pom){/* MFR  - to by Rumcise mrkal na drat! */
			pom->next=current->next;
			current->next=(pns->ns)[key%HASHNUM];
			(pns->ns)[key%HASHNUM]=current;
		}
		if(sperhaky)
			mem_free(sperhaky); /* Pri opakovanem hledani by byl memory-leak */
	}
	else{	debug("tvorim novou promennou\n");
		current=js_mem_alloc(sizeof(lns));
		current->identifier=key;
		current->mid=pns->mid; /* pna ma toto MID, aby se na nej slo odkazat pozdeji, az uz nebude videt na cely namespace */
		current->handler=pns->handler;
		current->next=pns->ns[key%HASHNUM];
		pns->ns[key%HASHNUM]=current;/*Nacpeme ho na zacatek*/
		current->type=UNDEFINED;
		current->value=0; /*Promenna je jako nova :-)*/
		if(sperhaky)
		{	current->type=INTVAR;
			current->handler=pns->handler;
			current->mid=sperhaky[0];
			switch(jsint_object_type(sperhaky[0]))
                        {       case JS_OBJ_T_TEXT:     current->value=Ctextptr;
                                break;
                                case JS_OBJ_T_PASSWORD: current->value=Cpasswdptr;
                                break;
                                case JS_OBJ_T_TEXTAREA: current->value=Ctextarptr;
                                break;
                                case JS_OBJ_T_CHECKBOX: current->value=Cchkboxptr;
                                break;
                                case JS_OBJ_T_RADIO:  /*  current->value=Cradioptr;*/
					current->type=ARRAY;
                                        current->value=(long)(pomns=js_mem_alloc(sizeof(plns)));
                                        pomns->mid=0;
					pomns->handler=pns->handler;
					pomns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
					pomint=0;
					while(pomint<HASHNUM)pomns->ns[pomint++]=0;
                                        pomns->next=context->lnamespace;
                                        /* Tady se budou vstavat radiocudlitka do pole.*/
                                        pomint=0;
                                        while(pomint<delka_vole)
                                        {       snprintf(pomstr,DELKACISLA,"%d",pomint);
                                                pomstr[DELKACISLA-1]='\0';
                                                pom=buildin(pomstr,context->namespace,pomns,context);
                                                pom->type=INTVAR;
                                                pom->value=Cradioptr;
                                                pom->mid=sperhaky[pomint];
                                                pom->handler=pns->handler;
                                                pomint++;
                                        }
					pom=create(MIN1KEY,pomns,context);
                                        pom->type=PARLIST;
                                        pom->value=0;
					add_to_parlist(current,pom);
                                        pom=buildin("length",context->namespace,pomns,context);
                                        pom->type=INTVAR;
                                        pom->value=Clength;
                                        pom->mid=0;
                                        pom->handler=delka_vole;
					pom=buildin("toString",context->namespace,pomns,context);
					pom->type=FUNKINT;
					pom->value=CtoString;
					pom->handler=C_OBJ_radio;
				break;
				case JS_OBJ_T_SELECT: current->value=Cselectptr;
                                break;
                                case JS_OBJ_T_SUBMIT:   current->value=Csubmitptr;
                                break;
                                case JS_OBJ_T_RESET:    current->value=Cresetptr;
                                break;
                                case JS_OBJ_T_HIDDEN:   current->value=Chiddenptr;
                                break;
				case JS_OBJ_T_FORM:	current->value=Cformptr;
				break;
				case JS_OBJ_T_FRAME:	current->value=Cframeptr;
							current->handler=current->mid;
				break;
				case JS_OBJ_T_BUTTON:	current->value=Cbuttonptr;
				break;
				case JS_OBJ_T_IMAGE:
							current->value=Cimageptr;
				break;
                                default:        my_internal("Semerad! To je typek!\n",context);
                                break;
                        }
			mem_free(sperhaky);
		}
	}
	return current;
}

lns * create(long key,plns*pns,js_context*context) /*ns je pointer na prvni "uzel"*/
/*Vytvori v lokalnim ns promennou*/
{	lns *current=(pns->ns)[key%HASHNUM],*pom=0;
	debug("Create: ");
	while(current &&(current->identifier!=key))
		current=(pom=current)->next;
	if(current)
	{	debug("promenna nalezena\n");
		clearvar(current,context);
		if(pom){ /* MFR - to by Rumcajs mrkal na drat! */
			pom->next=current->next;
			current->next=(pns->ns)[key%HASHNUM];
			(pns->ns)[key%HASHNUM]=current;
		}
	}
	else {	debug("tvorim novou promennou\n");
		current=js_mem_alloc(sizeof(lns));
		current->identifier=key;
		current->mid=pns->mid;
		if(pns->handler<0)internal("Handler<0\n");
		current->handler=pns->handler;
		current->next=pns->ns[key%HASHNUM];
		pns->ns[key%HASHNUM]=current;
		current->type=UNDEFINED;
		current->value=0;
	}
	return current;
}

abuf * getarg(abuf**odkud)/*Vrati hodnotu prvniho argumentu ve vyhledu*/
{	abuf**pom=odkud,*p1;
	if(!*pom)return 0;/*uzivatel je hovado a nedal vsechny argy*/
	while((*pom)->typ==ARGUMENTY)
	{	if(!(*pom)->argument)/*Fixme! Jestli prijde pravy argument 0, tak to sebou flakne!*/
		{	p1=(abuf*)(*pom)->a1;
			(*pom)->argument=p1->argument;
			(*pom)->a1=p1->a1;
			(*pom)->typ=p1->typ;
			js_mem_free(p1);
		} else /*trochu perverzni pouziti else k dosazeni mutualni excluse*/
			if((*pom)->argument)
			{	pom=(abuf**)&((*pom)->argument);	
			}
	}
	if(!pom)return 0;/*Spise paranoia, ale co kdybychom vyhazeli vsechno na neplatnost*/
	p1=(*pom);
	*pom=0;
	return p1; /*trik strasnyho Davea s pointerem na pointer. Snad je OK*/
}

void zrusargy(abuf*co,js_context*context)
{	if(!co) return;
	if(co->typ==ARGUMENTY)
	{	zrusargy((abuf*)co->argument,context);
		zrusargy((abuf*)co->a1,context);
		js_mem_free(co);
	}else	delarg(co,context);
}

void add_to_parlist(lns*parent,lns*list);

lns* buildin(char*,js_id_name **,plns*,js_context*);

plns * newnamespace(abuf*jmena,abuf*hod,js_context*context)
{	plns * nns=js_mem_alloc(sizeof(plns));
	lns*p,*po;
	abuf*jm,*val;
	int i=0;
	nns->ns=js_mem_alloc(sizeof(lns*)*HASHNUM);
	while(i<HASHNUM)nns->ns[i++]=0;
	/*Ted se tam pridaji promenne*/
	nns->next=0;/*cvicna nula - spise paranoia nez sanity*/
	nns->handler=nns->mid=0;/* ostra nula - sem Mikulas nevidi! */
	p=buildin("toString",context->namespace,nns,context);
	p->type=FUNKINT;
	p->value=CtoString;
	p->handler=C_OBJ_Objekt;
	if(jmena->typ!=UNDEFINED) /* nejsou argumenty */
		while((jm=getarg(&jmena)))
		{	val=getarg(&hod);
			p=create(((lns*)jm->argument)->identifier,nns,context);
			if(val)
			{	RESOLV(val);
				p->type=val->typ;
				p->value=val->argument;
				if(p->type==ADDRSPACE)
					p->type=ADDRSPACEP;
				if((p->type==ADDRSPACEP)|(p->type==ARRAY))
				{	po=lookup(MIN1KEY,(plns*)val->argument,context);
					if(po->type!=PARLIST)
						my_internal("Parentlist corrupted!\n",context);
					add_to_parlist(p,po);
				}
				js_mem_free(val);
			}
			else{	p->type=UNDEFINED;
				p->value=0;
			}
			js_mem_free(jm);
		}
	else
		js_mem_free(jmena);
	zrusargy(hod,context);
	
	return nns;
}

void clearvars(parlist*list)
{	parlist*next;
	while(list)
	{	list->parent->type=UNDEFINED;
		list->parent->value=0;
		next=list->next;
		js_mem_free(list);
		list=next;
	}
}

void deletenamespace(plns* pns,js_context*context)
{	lns ** ns=pns->ns,*pom,*p1;
	int i=0;
	js_mem_free(pns);
	if(!ns)	my_internal("Freeing NULL addrspace!\n",context);
	while(i<HASHNUM)
	{	pom=ns[i++];
		while(pom && pom->type!=PARLIST)
			pom=pom->next;
		if(pom) /* Parent-list is highly volatile, so we devastate it
			   by other function (than clearvar) */
		{	clearvars((parlist*)pom->value);
			pom->type=UNDEFINED;
			pom->value=0;
		}
	}
	i=0;
	while(i<HASHNUM)
	{	pom=ns[i++];
		while(pom)			
		{	if(pom->type==PARLIST) 
				my_internal("Too many parentlists!\n",context) /*;*/
			else
				clearvar(pom,context);
			p1=pom->next;
			js_mem_free(pom);
			pom=p1;
		}
	}
	js_mem_free(ns);
}

lns* buildin(char* retezec,js_id_name **names,plns*lnamespace,js_context*context)
{	long klic,i,pom;
	js_id_name*lastname;
	/* We acquire a key of variable: */

	klic=i=0;while(retezec[i])klic=klic*MAGIC+retezec[i++];
	klic&=127;/*Bude potreba ohandlovat dynamitickou velikost hashoveho pole*/
	if((lastname=names[klic])){
		while(strcmp(retezec,lastname->jmeno) &&lastname->next)
			lastname=lastname->next;
		if(!strcmp(retezec,lastname->jmeno)){ 
			klic=(lastname->klic)*128+klic;/*tady uz je klic cely*/
		} else{ /*Koukame na posledni vagonek a ten nematchuje!*/
			lastname->next=js_mem_alloc(sizeof(js_id_name));
			pom=lastname->klic+1;/*poradi vagonku*/
			lastname=lastname->next;
			lastname->jmeno=js_mem_alloc(strlen(retezec)+1);
			strcpy(lastname->jmeno,retezec);
			lastname->next=0;
			lastname->klic=pom;
			klic=pom*128+klic;
		}
	} else{	names[klic]=(js_id_name*)js_mem_alloc(sizeof(js_id_name));
		names[klic]->klic=0;
		names[klic]->jmeno=js_mem_alloc(strlen(retezec)+1);
		strcpy(names[klic]->jmeno,retezec);
		names[klic]->next=0;
	}
	
	return create(klic,lnamespace,context);
}


lns* llookup(char* retezec,js_id_name **names,plns*lnamespace,js_context*context)
{       long klic,i,pom;
	js_id_name*lastname;
	/* We acquire a key of variable: */

	klic=i=0;while(retezec[i])klic=klic*MAGIC+retezec[i++];
	klic&=127;/*Bude potreba ohandlovat dynamitickou velikost hashoveho pole*/
	if((lastname=names[klic])){
		while(strcmp(retezec,lastname->jmeno) &&lastname->next)
			lastname=lastname->next;
		if(!strcmp(retezec,lastname->jmeno)){
			klic=(lastname->klic)*128+klic;/*tady uz je klic cely*/
		} else{ /*Koukame na posledni vagonek a ten nematchuje!*/
			lastname->next=js_mem_alloc(sizeof(js_id_name));
			pom=lastname->klic+1;/*poradi vagonku*/
			lastname=lastname->next;
			lastname->jmeno=js_mem_alloc(strlen(retezec)+1);
			strcpy(lastname->jmeno,retezec);
			lastname->next=0;
			lastname->klic=pom;
			klic=pom*128+klic;
		}
	} else{ names[klic]=(js_id_name*)js_mem_alloc(sizeof(js_id_name));
		names[klic]->klic=0;
		names[klic]->jmeno=js_mem_alloc(strlen(retezec)+1);
		strcpy(names[klic]->jmeno,retezec);
		names[klic]->next=0;
	}
	return loklookup(klic,lnamespace,context);
}

void add_to_parlist(lns*parent,lns*list)
{	parlist*pom=js_mem_alloc(sizeof(parlist));
	pom->next=(parlist*)list->value;
	pom->parent=parent;
	list->value=(long)pom; /*we add to head of list*/
}

void delete_from_parlist(lns*parent,lns*list)
{	parlist*pom=(parlist*)list->value;
	parlist*prev;
	if(pom)
	{	if(pom->parent==parent)
		{	prev=pom->next;
			js_mem_free(pom);
			list->value=(long)prev;
		}else
		{	prev=pom;
			pom=pom->next;
			while(pom && pom->parent!=parent){prev=pom;pom=pom->next;}
			if(pom)
			{	prev->next=pom->next;
				js_mem_free(pom);
			}else
				internal("Internal: Parent not in list\n");
		}
	} else 	internal("Internal: Performing delete_from_parlist to empty list!\n");
}

#endif
