%{
/* ipret.c
 * Javascript interpreter
 * (c) 2002 Martin 'PerM' Pergel
 * This file is a part of the Links program, released under GPL.
 */
#include <stdlib.h>
#include <stdio.h>

#include "../cfg.h"

#ifndef JS

static void *yy_flex_alloc(yy_size_t a){
return 0;}

static void *yy_flex_realloc(void* a,yy_size_t b){
return 0;}

static void yy_flex_free(void*a){
}

static yy_state_type yy_get_previous_state(void){
return 0;}

static yy_state_type yy_try_NUL_trans(yy_state_type current_state)
{ return 0;
}

static int yy_get_next_buffer(void)
{ return 0;
}

static void yy_fatal_error(yyconst char msg[])
{
}

void js_fake_function_to_stop_silly_warnings()
{ yy_last_accepting_state=0;
  yy_last_accepting_cpos=0;
  yy_n_chars=0;
  yy_c_buf_p=0;
  yy_init=yy_start=0;
  yy_did_buffer_switch_on_eof=0;
  yy_hold_char=0;
  yy_current_buffer=0;
  yy_flex_alloc(0);
  yy_flex_realloc(0,0);
  yy_flex_free(0);
  yy_get_previous_state();
  yy_try_NUL_trans(0);
  yy_get_next_buffer();
  yy_fatal_error(0);
}


#endif
#ifdef JS

#include "tree.h"
#include "struct.h"
#include "ipret.h"
#define YYSTYPE long
#include "javascript.h"
#define DEBUZIM
#undef DEBUZIM
#undef getc

#define YY_ALWAYS_INTERACTIVE 1
#define getc mygetc
#define ungetc myungetc
void yyparse(); /* stops silly warning - hope prototype isn't buggy! */

char * js_get_char;
char * js_get_char_upto;
char * js_get_char_from;
char * rawstr;
js_context * js_context_ptr;

extern vrchol*js_strom;
extern vrchol*js_last;

int mygetc(FILE *stream)
{	int i=EOF;
	if(js_get_char<=js_get_char_upto)i=*js_get_char++;
	return i;
}

int myungetc(int c, FILE *stream)
{	int i=EOF;
	if(js_get_char>js_get_char_from)i=*js_get_char--=c;
	return i;
}

long civilize(char* string)
{	char* result=js_mem_alloc(strlen(string)+1);
	int i=0,j=0;
	while(string[i])
	{	while(string[i]&&string[i]!='\\') { result[j++]=string[i++];}
		if(string[i])
		{	switch(string[++i])
			{	case 'n':
				case 'r':
					result[j++]='\n';
				break;
				case 't':
					result[j++]='\t';
				break;
				default:
					result[j++]=string[i];
				break;
			}
		i++;
		}
	}
	result[j]='\0';
	if(j>strlen(string))internal("Strilim do pameti!\n");
	js_mem_free(string);
	return (long) result;
}

static int i,j;
int linenumber=1;
long c_radku=0;
js_id_name * * names;
static js_id_name*lastname;
char*retezec;
int f(void){return 0;}

%}
%%

^[ \t]*\<\!\-\-[^\n\r]*	{
	js_warning("HTML comment begin in javascript ",c_radku,js_context_ptr);
#ifdef DEBUZIM
	printf("Creeping featurism comment stripping!\n");
#endif
	}

break	{ 
#ifdef DEBUZIM
	printf("break\n");
#endif
	return BREAK; /*Keywordy*/
	}

case	{ 
#ifdef DEBUZIM
        printf("case\n");
#endif
       	return CASE;}

catch	{ 
#ifdef DEBUZIM
        printf("catch\n");
#endif
              return CATCH;}

continue	{ 
#ifdef DEBUZIM
        printf("continue\n");
#endif
              return CONTINUE;}

default	{ 
#ifdef DEBUZIM
        printf("default\n");
#endif
              return DEFAULT;}

delete	{ 
#ifdef DEBUZIM
        printf("delete\n");
#endif
              return DELETE;}

do	{ 
#ifdef DEBUZIM
        printf("do\n");
#endif
              return DO;}

else	{ 
#ifdef DEBUZIM
        printf("else\n");
#endif
              return ELSE;}

finally	{ 
#ifdef DEBUZIM
        printf("finally\n");
#endif
              return FINALLY;}

for	{ 
#ifdef DEBUZIM
        printf("for\n");
#endif
              return FOR;}

function	{ /*zde vratime dva tokeny FUNCTION*/
#ifdef DEBUZIM
        printf("function\n");
#endif
              return FUNCTION;}

if	{ 
#ifdef DEBUZIM
        printf("if\n");
#endif
              return IF;}

in	{ 
#ifdef DEBUZIM
        printf("in\n");
#endif
              return IN;}

instanceof	{ 
#ifdef DEBUZIM
        printf("instanceof\n");
#endif
              return INSTANCEOF;}

new	{ 
#ifdef DEBUZIM
        printf("new\n");
#endif
              return NEW;}

return	{ 
#ifdef DEBUZIM
        printf("return\n");
#endif
              return RETURN;}

switch	{ 
#ifdef DEBUZIM
        printf("switch\n");
#endif
              return SWITCH;}

this	{ 
#ifdef DEBUZIM
        printf("this\n");
#endif
              return THIS;}

throw	{ 
#ifdef DEBUZIM
        printf("throw\n");
#endif
              return THROW;}

try	{ 
#ifdef DEBUZIM
        printf("try\n");
#endif
              return TRY;}

typeof	{ 
#ifdef DEBUZIM
        printf("typeof\n");
#endif
              return TYPEOF;}

var	{ 
#ifdef DEBUZIM
        printf("var\n");
#endif
              return VAR;}

void	{ 
#ifdef DEBUZIM
        printf("void\n");
#endif
              return VOID;}

while	{ 
#ifdef DEBUZIM
        printf("while\n");
#endif
              return WHILE;}

with	{ 
#ifdef DEBUZIM
        printf("with\n");
#endif
              return WITH;}

abstract|boolean|byte|char|class|const|debugger|double|enum|export|extends|final|float	{	
#ifdef DEBUZIM
	printf("You use future keywords?\n");
#endif
		return LEXERROR;}
goto|implements|import|int|interface|long|native|package|private|protected|public|short|static|super|synchronized|throws|transient|volatile	{
#ifdef DEBUZIM
	printf("You use future keywords?\n");
#endif
		return LEXERROR;}

[\{\}\(\)\[\]\.\;\,\<\>\+\-\*\/\%\&\|\^\!\?\:\=\~]	{	 
#ifdef DEBUZIM
        printf("symbol %c\n",yytext[0]);
#endif
       return yytext[0];}

\<\=|\>\=|\=\=|\!\=|\+\+|\-\-|\<\<|\>\>|\&\&|\|\||\+\=|\-\=|\*\=|\/\=|\%\=|\&\=|\|\=|\^\=	{ 
#ifdef DEBUZIM
        printf("symbol %d\n",yytext[0]+256*yytext[1]);
#endif
       return (yytext[0]+256*yytext[1]);}

\=\=\=	{
#ifdef DEBUZIM
        printf("symbol ===\n");
#endif
       return EQEQEQ;}

\!\=\=	{
#ifdef DEBUZIM
        printf("symbol !==\n");
#endif
       return EXCLAMEQEQ;}

\>\>\>	{
#ifdef DEBUZIM
        printf("symbol >>>\n");
#endif
       return SHRSHRSHR;}

\<\<\=	{
#ifdef DEBUZIM
        printf("symbol <<=\n");
#endif
       return SHLSHLEQ;}

\>\>\=	{ 
#ifdef DEBUZIM
        printf("symbol >>=\n");
#endif
       return SHRSHREQ;}

\>\>\>\=	{ 
#ifdef DEBUZIM
        printf("symbol >>>=\n");
#endif
       return THREERIGHTEQUAL;

	/*POZOR, prasarna, tohle se bude predelavat! Jinymi slovy tedka nemam spravne numericke a retezcove literaly a identifikatory!*/
	}

null	{ 
#ifdef DEBUZIM
        printf("nullit\n");
#endif
       return NULLLIT;}

true	{ 
#ifdef DEBUZIM
        printf("truelit\n");
#endif
       return TRUELIT;}
false	{ 
#ifdef DEBUZIM
        printf("falselit\n");
#endif
       return FALSELIT;}


0x[0-9AaBbCcDdEeFf]*	{	yylval=(long)js_mem_alloc(sizeof(cislo));
				((cislo*)yylval)->typ=CELE;
				((cislo*)yylval)->nr.cele=strtol(yytext+2,0,16); /*NUMERICKE LITERALY -- sestnactkovy*/
				 
#ifdef DEBUZIM
        printf("hexnr.: %x\n",((cislo*)yylval)->nr.cele);
#endif
       				return NUMLIT;}


0|[1-9][0-9]*	{	yylval=(long)js_mem_alloc(sizeof(cislo));
			((cislo*)yylval)->typ=CELE;
			((cislo*)yylval)->nr.cele=strtol(yytext,0,10);
#ifdef DEBUZIM
	printf("decint.: %d\n",((cislo*)yylval)->nr.cele);
#endif
				return NUMLIT;}

[0-9]*	{	yylval=(long)js_mem_alloc(sizeof(cislo));
		((cislo*)yylval)->typ=CELE;
		((cislo*)yylval)->nr.cele=strtol(yytext,0,8);
#ifdef DEBUZIM
	printf("octint.: %d\n",((cislo*)yylval)->nr.cele);
#endif
		return NUMLIT;
	}


(((0|[1-9][0-9]*){0,1}\.[0-9]*)|(0|[1-9][0-9]*))((e|E)(\+|\-){0,1}([0-9]+)){0,1}	{	yylval=(long)js_mem_alloc(sizeof(cislo));
			((cislo*)yylval)->typ=NECELE;
                        ((cislo*)yylval)->nr.necele=(float)atof(yytext);
#ifdef DEBUZIM
        printf("decf.: %f\n",((cislo*)yylval)->nr.necele);
#endif
				return NUMLIT;}










[A-Za-z\_\$][A-Za-z0-9\_\$]*	{ 
#ifdef DEBUZIM
        printf("IDENTIFIER ");
#endif
/* Tohle prijde kompletne prekopat: Nebudeme na zacatku zhuverile mallocovat,
   nybrz jen okopirujeme retezec a pak si ifneme: Budto je kolej prazdna, pak
   rovnou udelame vagonek s promennou, jinak koukneme, jestli tam identif. uz
   je, jestli ne, tak dojdeme na konec, pak zname posledniho a za nej to
   povesime
*/
	retezec=(char*)js_mem_alloc(yyleng+1);
	for(j=0;j<yyleng;j++)retezec[j]=yytext[j];
	retezec[j]=0;
#ifdef DEBUZIM
	printf("%s\n",retezec);
#endif
	if(127>=HASHNUM)/*Takova trapna assertiona*/
		{internal("Error! HASHNUM musi byt vetsi nez 127!\n");}
/*	if(yyleng==1)j=((retezec[0])&127); 
	else	j=((retezec[0]^retezec[yyleng-1])&127);
*/
/*MJovo prurazne #ovani:*/
	j=i=0;while(retezec[i])j=j*MAGIC+retezec[i++];
	j&=127;/*a je to... Jeste bude potreba ohandlovat dynamitickou velikost hashoveho pole*/

	if((lastname=names[j])){
		while(strcmp(retezec,lastname->jmeno) &&lastname->next)
			lastname=lastname->next;
		if(!strcmp(retezec,lastname->jmeno)){ js_mem_free(retezec);
			yylval=(lastname->klic)*128+j;
		} else{	/*Koukame na posledni vagonek a ten nematchuje!*/
			lastname->next=js_mem_alloc(sizeof(js_id_name));
			yylval=lastname->klic+1;
			lastname=lastname->next;
			lastname->jmeno=retezec;
			lastname->next=0;
			lastname->klic=yylval;
			yylval=yylval*128+j;
		}
	} else{	names[j]=(js_id_name*)js_mem_alloc(sizeof(js_id_name));
		names[j]->klic=0;
		names[j]->jmeno=retezec;
		names[j]->next=0;
		yylval=j;
	}

/*	yylval=(long)js_mem_alloc(sizeof(js_id_name));
	(((js_id_name*)yylval)->jmeno)=retezec=(char*)js_mem_alloc(yyleng+1);
	(((js_id_name*)yylval)->next)=0;/nema dosud naslednika/
	for(j=0;j<yyleng;j++)retezec[j]=yytext[j];
	retezec[j]=0;
	printf("%s\n",retezec);
	if(yyleng==1)j=((retezec[0])&127); 
	else	j=((retezec[0]^retezec[yyleng-1])&127);
	if(lastname=names[j]){
		while(!strcmp(retezec,lastname->
		(((js_id_name*)yylval)->klic)=lastns[j]->klic+1;
		lastns[j]->next=(js_id_name*)yylval;
		lastns[j]=lastns[j]->next;
	}else{	names[j]=(js_id_name*)yylval;
		f();
		names[j]->klic=0;
	}
*/
	return IDENTIFIER; /*tohle je tezce debuzi, budu to stejne cele prepisovat.*/
	}



'((\\.)|(\\([ \t]*)((\r)|(\n)|(\r\n)))|[^\\\'])*'		{/*RETEZCOVY LITERAL*/
		j=1;	rawstr=js_mem_alloc(yyleng-1);	while(j<(yyleng-1)){rawstr[j-1]=yytext[j];j++;}
		rawstr[j-1]=0;/*konec stringu, spadla klec!*/
		yylval=civilize(rawstr);
#ifdef DEBUZIM
        printf("string1: %s\n",(char*)yylval);
#endif
       		return STRINGLIT;}

\"((\\.)|(\\([ \t]*)((\r)|(\n)|(\r\n)))|[^\\\"])*\"	{
		j=1;	rawstr=js_mem_alloc(yyleng-1);	while(j<(yyleng-1)){rawstr[j-1]=yytext[j];j++;}
		rawstr[j-1]=0;
		yylval=civilize(rawstr); 
#ifdef DEBUZIM
	printf("%d ",yyleng);
        printf("string2: %s\n",(char*)yylval);
#endif
       		return STRINGLIT; /*pozor, prasarna kvuli '"', Jestli to je escsekv, tak to budu prepisovat na 2 pravidla!*/
	}

\t|\ 	/*zahodime Whitespace*/

\n	c_radku++;/*Line Terminator*/

\r	c_radku++;/*ultrapervers!*/

\/\*\/*([^\/]|[^\*]\/)*\*\/	{i=0; 
			while(i<yyleng)
			{	if((yytext[i]=='\n')||(yytext[i]=='\r'))
					c_radku++;/* viceradkovy komentar*/
				i++;
			}
	}

\/\/[^\n\r]*	{/*c_radku++;*/ /*Jednoradkovy komentar*/
	}
	
.	{	
#ifdef DEBUZIM
		printf("Lexical bug '%c' in line %d !\n",yytext[0],(int)c_radku);
#endif
/*		yyterminate();*/
		return BUGGY_TOKEN;
	}

%%
/*Todle se bude jmenovat js_execute_code(struct javascript_context*context,unsigned char*code, int len), ja sam si tu budu muset pretizit funkce (f)getc
a (f)ungetc, aby to fungovalo. js_create_context a js_destroy_context budou
jinde.*/

extern vrchol*js_strom;
static vrchol*pom_js_strom;
vrchol* previous;/*Tady se bude ukladat pointer na posledni allokovany kus stromu*/


/*Todle udela javascr_execute_code:*/ /* Zbyva: opravit getc a ungetc, aby to fungovalo */
void js_execute_code(js_context* context,unsigned char * code,int len, void (*callback)(void *))
{	char*pomstr;
	int timerno=0;
	js_bordylek*bordylek;
	names=(context->namespace);
/*	for(i=0;i<HASHNUM;i++)if(names[i]){pom=names[i];while(pom->next)pom=pom->next;
			lastns[i]=pom;
		}else	lastns[i]=0;
*/
	previous=context->js_tree;/* Jsme na zacatku*/
	js_last=context->js_tree; /* Kdyby to spadlo driv nez to co naalokuje */
	js_get_char=js_get_char_from=code;
	js_get_char_upto=js_get_char+len-1;
	c_radku=context->lineno;
	context->callback=callback;
	js_context_ptr=context;
        if(context->code)
        {       pomstr=js_mem_alloc(context->codelen+len+1);
                strncpy(pomstr,context->code,context->codelen);
                pomstr[context->codelen]='\0';
                strncat(pomstr,code,len);
                pomstr[context->codelen+len]='\0';
                js_mem_free(context->code);
                context->code=pomstr;
                pomstr=0;
        }else { context->code=js_mem_alloc(len+1);
                strncpy(context->code,code,len);
                context->code[len]='\0';
        }
	if(!context->jsem_dead){
/*		js_file=fopen("links_debug.err","a");
		fprintf(js_file,context->code);
		fclose(js_file);*/
		yyparse();
		YY_FLUSH_BUFFER;
		/*yy_delete_buffer(YY_CURRENT_BUFFER);*/
		if(!context->jsem_dead)
			context->lineno=c_radku;
		else {	js_volej_kolbena(context);
			return;
		}
	}
	else 
	{	js_volej_kolbena(context);
		return;
	}
	/*context->lnamespace=projdi(js_strom); POZOR! Tady je to blbe!
Tadyhle prolezeme vsechny funkce a zapamatujeme si, kam s nimi.*/

/* Tadyhle je potreba pridelat kontrolu neprazdnosti stromu pro pripad */
/* ze autor stranky je prase a udelal samomodifikujici kod! */
	js_context_ptr=0;
	if(!context->js_tree)
		context->js_tree=js_strom;
	else {	pom_js_strom=context->js_tree;
		context->js_tree=js_mem_alloc(sizeof(vrchol));
		context->js_tree->prev=previous;
		previous=context->js_tree; /*shift pointer to last node */
		context->js_tree->opcode=TPROGRAM;
		context->js_tree->arg[0]=pom_js_strom;
		context->js_tree->arg[1]=js_strom;
	}
	context->current=js_strom;

	/* odsud se zadne upcally voalt nemuzou. muzou se volat az z funkce js_do_code volane z nuloveho timeru   !!! Mikulasova sekce !!! */
/*        if (context->t != -1) internal("there is already code executing in this context");*/
/*	if(context->code)
	{	pomstr=js_mem_alloc(context->codelen+len+1);
		strncpy(pomstr,context->code,context->codelen);
		pomstr[context->codelen]='\0';
		strncat(pomstr,code,len);
		pomstr[context->codelen+len]='\0';
		js_mem_free(context->code);
		context->code=pomstr;
		pomstr=0;
	}else {	context->code=js_mem_alloc(len+1);
		strncpy(context->code,code,len);
		context[code]='\0';
	}
	context->code = code;*/
	context->codelen += len;
	while(timerno<TIMERNO &&(context->t[timerno]!=-1))timerno++;
	if(timerno>=TIMERNO)
	{	js_error("Too many timers",context);
		return;
	}
	if(!(bordylek=js_mem_alloc(sizeof(js_bordylek))))
		internal("Out of memory!\n");
	bordylek->context=context;
	bordylek->mytimer=&context->t[timerno];
	context->running=1;
	context->t[timerno]=install_timer(1,(void(*)(void*))ipret,bordylek);
	context->bordely[timerno]=bordylek;
}

/*	Tady zkonci funkce js_execute_code*/

#ifdef CHCEME_FLEXI_LIBU
int yywrap() { return 1; }
#endif

#endif

