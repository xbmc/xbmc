/* pomocny.c
 * (c) 2002 Martin 'PerM' Pergel
 * This file is a part of the Links program, released under GPL.
 */

#include "../cfg.h"

#ifdef JS

#include "struct.h"

extern vrchol* previous;
extern vrchol* js_last;
extern long c_radku;

char* js_temp_var_for_stracpy1;

static void smaz(vrchol*v)
{	v->arg[1]=v->arg[2]=v->arg[3]=v->arg[4]=v->arg[5]=v->arg[0]=0;
	v->in=0;
	v->lineno=c_radku;
}

vrchol * terminal(void)
{	return neterminal();
/*vrchol * a=js_mem_alloc(sizeof(vrchol));
//	a->term=TERM;
	smaz(a);
	js_last=a;
	return a;*/
}
vrchol * neterminal(void)
{	vrchol * a=js_mem_alloc(sizeof(vrchol));
/*	a->term=NETERM;*/
	a->prev=previous;
	previous=a;
	smaz(a);
	js_last=a;
	return a;
}

void js_spec_vykill_timer(js_context*context,int i)
{
	if(context->upcall_timer==-1)
		internal("Upcallovy timer uz byl vykillen!\n");
	if(i)
	{	kill_timer(context->upcall_timer);
		switch(context->upcall_typek)
		{	case TYPEK_2_STRINGY:
				if(((struct fax_me_tender_2_stringy*)context->upcall_data)->string1)js_mem_free(((struct fax_me_tender_2_stringy*)context->upcall_data)->string1);
				if(((struct fax_me_tender_2_stringy*)context->upcall_data)->string2)js_mem_free(((struct fax_me_tender_2_stringy*)context->upcall_data)->string2);
				js_mem_free(context->upcall_data);
			break;
			case TYPEK_INT_STRING:
				if(((struct fax_me_tender_int_string*)context->upcall_data)->string)js_mem_free(((struct fax_me_tender_int_string*)context->upcall_data)->string);
				js_mem_free(context->upcall_data);
			break;
			case TYPEK_STRING_2_LONGY:
				if(((struct fax_me_tender_string_2_longy*)context->upcall_data)->string)js_mem_free(((struct fax_me_tender_string_2_longy*)context->upcall_data)->string);
				js_mem_free(context->upcall_data);
			break;
			case TYPEK_STRING:
				if(((struct fax_me_tender_string*)context->upcall_data)->string)js_mem_free(((struct fax_me_tender_string*)context->upcall_data)->string);
				js_mem_free(context->upcall_data);
			break;
			case TYPEK_NIC:
				js_mem_free(context->upcall_data);
			break;
			default:	internal("Neexistujici typ dat!\n");
			break;
		}
	}
	context->upcall_timer=-1;
}

void* js_js_temp_var1;
size_t js_js_temp_var2;
int js_zaflaknuto_pameti=0;

#endif
