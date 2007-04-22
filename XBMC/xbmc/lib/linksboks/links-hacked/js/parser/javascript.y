%{
/* ipret.c
 * Javascript interpreter
 * (c) 2002 Martin 'PerM' Pergel
 * This file is a part of the Links program, released under GPL.
 */

#include "../cfg.h"

#ifdef JS

#include "struct.h"
#include "tree.h"
#define YYSTYPE long
#define BRUTALDEBUG
#undef BRUTALDEBUG
static vrchol*pom_vrchol; /*Snad to znamena to, co myslim*/
vrchol*js_strom; 
extern int c_radku; 
int yyerror(char*); /* This only stops warning - hope it isn't buggy! */
vrchol* js_last=0;
extern js_context*js_context_ptr;
int yylex(); /* This kills warning -hope is'nt buggy! */

void js_warning_fn(struct dialog_data *dlg);

/* aux function for js_warning */
int __js_shut_up_pressed(struct dialog_data *dlg, struct dialog_item_data* di)
{
	int smazano;
	di=di;
	js_verbose_warnings=0;

	do {
		struct window *w1;
		smazano = 0;
		/* podivame se, jestli nase wokno neni nekde na terminalu */
		foreach(w1,dlg->win->term->windows)
			/* pokud ano, tak ho smazeme, ale nesmime smazat dlg->win */
			if (w1!=dlg->win&&w1->handler == dialog_func && ((struct dialog_data *)w1->data)->dlg->fn == js_warning_fn)
			{
				delete_window(w1);
				smazano = 1;
				break;
			}
	} while (smazano);
	delete_window(dlg->win);
	return 0;
}

/* aux function for js_warning */
int __js_dismiss_pressed(struct dialog_data *dlg, struct dialog_item_data* ignore)
{
	delete_window(dlg->win);
	return 0;
}


void js_warning_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, rw;
	int y = 0;

	max_text_width(term, dlg->dlg->udata, &max, AL_CENTER);
	min_text_width(term, dlg->dlg->udata, &min, AL_CENTER);
	max_buttons_width(term, dlg->items + dlg->n - 2, 2, &max);
	min_buttons_width(term, dlg->items + dlg->n - 2, 2, &min);
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > term->x - 2 * DIALOG_LB) w = term->x - 2 * DIALOG_LB;
	if (w < 5) w = 5;
	rw = 0;
	dlg_format_text(dlg, NULL, dlg->dlg->udata, 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(1, 1 * G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, NULL, dlg->items + dlg->n - 2, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = rw + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB + gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text(dlg, term, dlg->dlg->udata, dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, term, dlg->items + dlg->n - 2, 2, dlg->x + DIALOG_LB, &y, w, &rw, AL_CENTER);
}


void js_warning(char*a,int cislo_bugovity_lajny,js_context*context)
{
        unsigned char *txt,*txt2;
	int i=0,j=0,k=0;
        struct f_data_c *fd=(struct f_data_c*)context->ptr;
        struct terminal *term=fd->ses->term;
	int l;

	if(!js_verbose_errors) return;
	if(!js_verbose_warnings) return;

/*	snprintf(txt1,MAX_STR_LEN,"%s in line %d!!!\b\b\n",a,cislo_bugovity_lajny);*/
	while(i<cislo_bugovity_lajny && context->code[j])
		if(context->code[j++]=='\n')i++;
	if(context->code[j])
	{	k=j;
		while(context->code[k] && context->code[k]!='\n')k++;
		if(context->code[k])
		{	i=1;
			context->code[k]='\0';
		} else	i=0;
	}

	txt2=stracpy(&context->code[j]);
	if (strlen(txt2)>MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU)
	{
		txt2[MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU-1]='.';
		txt2[MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU-2]='.';
		txt2[MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU-3]='.';
		txt2[MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU-4]=' ';
		txt2[MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU]=0;
	}
	skip_nonprintable(txt2);
	if (fd->f_data)
	{
		struct conv_table* ct;
			
		ct=get_translation_table(fd->f_data->cp,fd->f_data->opt.cp);
		txt=convert_string(ct,txt2,strlen(txt2),&(fd->f_data->opt));
	}
	else
		txt=stracpy(txt2);
	mem_free(txt2);
	txt2=txt;

	txt=init_str();
	l=0;
	add_to_str(&txt,&l,a);
	add_to_str(&txt,&l," in: ");
	add_to_str(&txt,&l,txt2);
	add_to_str(&txt,&l,"\n");
	mem_free(txt2);
	if(i)   context->code[k]='\n';

	{
		struct dialog *d;

		if (!(d = mem_alloc(sizeof(struct dialog) + 3 * sizeof(struct dialog_item)))) return;
		memset(d, 0, sizeof(struct dialog) + 3 * sizeof(struct dialog_item));
		d->title = TXT(T_JAVASCRIPT_WARNING);
		d->fn = js_warning_fn;
		d->udata = txt;
		d->items[0].type = D_BUTTON;
		d->items[0].gid = B_ENTER|B_ESC;
		d->items[0].fn = __js_dismiss_pressed;
		d->items[0].text = TXT(T_DISMISS);
		d->items[1].type = D_BUTTON;
		d->items[1].fn = __js_shut_up_pressed;
		d->items[1].text = TXT(T_TURN_OFF_WARNINGS);
		d->items[2].type = D_END;

 	 	do_dialog(term, d, getml(d, txt, NULL));
	}
}

%}

%token BREAK
%token CASE
%token CATCH
%token CONTINUE
%token DEFAULT
%token DELETE
%token DO
%token ELSE
%token FINALLY
%token FOR
%token FUNCTION
%token IF
%token IN
%token INSTANCEOF
%token NEW
%token RETURN
%token SWITCH
%token THIS
%token THROW
%token TYPEOF
%token TRY
%token VAR
%token VOID
%token WHILE
%token WITH
%token LEXERROR
%token THREERIGHTEQUAL
%token IDENTIFIER
%token NULLLIT
%token FALSELIT
%token TRUELIT
%token NUMLIT
%token STRINGLIT
%token BUGGY_TOKEN
%token PLUSPLUS 11051
%token MINMIN 11565
%token SHLEQ 15676
%token SHREQ 15678
%token SHLSHL 15420
%token SHRSHR 15934
%token SHRSHRSHR 
%token EQEQ 15677
%token EXCLAMEQ 15649 
%token EQEQEQ
%token EXCLAMEQEQ 
%token ANDAND 9766
%token OROR 31868
%token PLUSEQ 15659
%token MINEQ 15661
%token TIMESEQ 15658
%token MODEQ 15653
%token DIVEQ 15663
%token ANDEQ 15654
%token OREQ 15740
%token XOREQ 15710
%token SHLSHLEQ 
%token SHRSHREQ 


%left '<' '>' SHLEQ SHREQ INSTANCEOF IN
%left SHLSHL SHRSHR SHRSHRSHR

%left '+' '-'
%left '*' '/'


%%

Program:
           {
#ifdef BRUTALDEBUG
printf("Program ->           \n");
#endif
	js_strom=0;
	$$=0;
}
          |Program Element {
#ifdef BRUTALDEBUG
printf("Program -> Program Element\n");
#endif
	if(!$2) js_strom=(vrchol*)$1;
	else
	{	pom_vrchol=neterminal();
		pom_vrchol->opcode=TPROGRAM;
		pom_vrchol->arg[0]=(void*)$1;
		pom_vrchol->arg[1]=(void*)$2;
		$$=(long)pom_vrchol;
		js_strom=pom_vrchol;
	}
	
};

Element:
	Statement {
#ifdef BRUTALDEBUG
printf("Element->Statement\n");
#endif
	$$=$1;
};

AssignmentExpression:
        FUNCTION Identifier '(' ParameterListOpt ')' CompoundStatement {
#ifdef BRUTALDEBUG
	printf("AssignmentExpression -> FUNCTION Identifier (ParameterListOpt) CompoundStatement\n");
#endif
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TFUNCTIONDECL;
	pom_vrchol->arg[0]=(void*)$2;
	pom_vrchol->arg[1]=(void*)$4;
	pom_vrchol->arg[2]=(void*)$6;
	$$=(long)pom_vrchol;
}
	| FUNCTION '(' ParameterListOpt ')' CompoundStatement {
#ifdef BRUTALDEBUG
	printf("AssignmentExpression -> FUNCTION Identifier (ParameterListOpt) CompoundStatement\n");
#endif
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TFUNCTIONDECL;
	pom_vrchol->arg[0]=neterminal();
	pom_vrchol->arg[0]=0;
	pom_vrchol->arg[1]=(void*)$3;
	pom_vrchol->arg[2]=(void*)$5;
	$$=(long)pom_vrchol;
};

/*	|Statement {
#ifdef BRUTALDEBUG
printf("Element -> Statement\n");
#endif
	$$=$1;
};*/

AssignmentOperator:
	  '=' {
#ifdef BRUTALDEBUG
printf("AssignmentOperator -> =\n");
#endif
	pom_vrchol=terminal();
        pom_vrchol->opcode=TEQ;
        $$=(long)pom_vrchol;
}
	  |TIMESEQ {
#ifdef BRUTALDEBUG
printf("AssignmentOperator -> *=\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TTIMESEQ;
        $$=(long)pom_vrchol;
}
	  |DIVEQ {
#ifdef BRUTALDEBUG
printf("AssignmentOperator -> /=\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TDIVEQ;
        $$=(long)pom_vrchol;
}
	  |MODEQ {
#ifdef BRUTALDEBUG
puts("AssignmentOperator -> %=\n");
#endif
	pom_vrchol=terminal();
        pom_vrchol->opcode=TMODEQ;
        $$=(long)pom_vrchol;
}
	  |PLUSEQ {
#ifdef BRUTALDEBUG
printf("AssignmentOperator -> +=\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TPLUSEQ;
        $$=(long)pom_vrchol;
}
	  |MINEQ {
#ifdef BRUTALDEBUG
printf("AssignmentOperator -> -=\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TMINEQ;
        $$=(long)pom_vrchol;
}
	  |SHLSHLEQ {
#ifdef BRUTALDEBUG
printf("AssignmentOperator -> <<=\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TSHLSHLEQ;
        $$=(long)pom_vrchol;
}
	  |SHRSHREQ {
#ifdef BRUTALDEBUG
printf("AssignmentOperator -> >>=\n");
#endif
	pom_vrchol=terminal();
        pom_vrchol->opcode=TSHRSHREQ;
        $$=(long)pom_vrchol;
}
	  |THREERIGHTEQUAL {
#ifdef BRUTALDEBUG
printf("AssignmentOperator -> >>>=\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TTHREERIGHTEQUAL;
        $$=(long)pom_vrchol;
}
	  |ANDEQ {
#ifdef BRUTALDEBUG
printf("AssignmentOperator -> &=\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TANDEQ;
        $$=(long)pom_vrchol;
}
	  |XOREQ {
#ifdef BRUTALDEBUG
printf("AssignmentOperator -> ^=\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TXOREQ;
        $$=(long)pom_vrchol;
}
	  |OREQ {
#ifdef BRUTALDEBUG
printf("AssignmentOperator -> !=\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TOREQ;
        $$=(long)pom_vrchol;
};

EqualityOperator:
	  EQEQ {
#ifdef BRUTALDEBUG
printf("EqualityOperator -> ==\n");
#endif
	pom_vrchol=terminal();
        pom_vrchol->opcode=TEQEQ;
        $$=(long)pom_vrchol;
}
	  |EXCLAMEQ {
#ifdef BRUTALDEBUG
printf("EqualityOperator -> !=\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TEXCLAMEQ;
        $$=(long)pom_vrchol;
};

RelationalOperator:
	  '<' {
#ifdef BRUTALDEBUG
printf("RelationalOperator -> <\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TSHL;
        $$=(long)pom_vrchol;
}
	  |'>' {
#ifdef BRUTALDEBUG
printf("RelationalOperator -> >\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TSHR;
        $$=(long)pom_vrchol;
}
	  |SHLEQ {
#ifdef BRUTALDEBUG
printf("RelationalOperator -> <=\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TSHLEQ;
        $$=(long)pom_vrchol;
}
	  |SHREQ {
#ifdef BRUTALDEBUG
printf("RelationalOperator -> >=\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TSHREQ;
        $$=(long)pom_vrchol;
};

ShiftOperator:
	  SHLSHL {
#ifdef BRUTALDEBUG
printf("ShiftOperator -> <<\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TSHLSHL;
        $$=(long)pom_vrchol;
}
	  |SHRSHR {
#ifdef BRUTALDEBUG
printf("ShiftOperator -> >>\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TSHRSHR;
        $$=(long)pom_vrchol;
}
	  |SHRSHRSHR {
#ifdef BRUTALDEBUG
printf("ShiftOperator -> >>>\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TSHRSHRSHR;
        $$=(long)pom_vrchol;
};

MultiplicativeOperator:
	  '*' {
#ifdef BRUTALDEBUG
printf("MultiplicativeOperator -> *\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TTIMES;
        $$=(long)pom_vrchol;
}
	  |'/' {
#ifdef BRUTALDEBUG
printf("MultiplicativeOperator -> /\n");
#endif
	pom_vrchol=terminal();
        pom_vrchol->opcode=TSLASH;
        $$=(long)pom_vrchol;
}
	  |'%' {
#ifdef BRUTALDEBUG
puts("MultiplicativeOperator -> %\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TMOD;
        $$=(long)pom_vrchol;
};

UnaryOperator:
	  '!' {
#ifdef BRUTALDEBUG
printf("UnaryOperator -> !\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TEXCLAM;
        $$=(long)pom_vrchol;
}
	  |'~' {
#ifdef BRUTALDEBUG
printf("UnaryOperator -> ~\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TCOMPL;
        $$=(long)pom_vrchol;
};

IncrementOperator:
	  PLUSPLUS {
#ifdef BRUTALDEBUG
printf("IncrementOperator -> ++\n");
#endif
	pom_vrchol=terminal();
        pom_vrchol->opcode=TPLUSPLUS;
        $$=(long)pom_vrchol;
}
	  |MINMIN {
#ifdef BRUTALDEBUG
printf("IncrementOperator -> --\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TMINMIN;
        $$=(long)pom_vrchol;
};

ParameterListOpt:
           {
#ifdef BRUTALDEBUG
printf("ParameterListOpt ->           \n");
#endif
	$$=0;
}
          |ParameterList {
#ifdef BRUTALDEBUG
printf("ParameterListOpt -> ParameterList\n");
#endif
	$$=$1;
};

ParameterList:
          Identifier {
#ifdef BRUTALDEBUG
printf("ParameterList -> Identifier\n");
#endif
	$$=$1;
}
          |Identifier ',' ParameterList {
#ifdef BRUTALDEBUG
printf("ParameterList -> Identifier, ParameterList\n");
#endif
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TParameterList;
	pom_vrchol->arg[0]=(void*)$1;
	pom_vrchol->arg[1]=(void*)$3;
	$$=(long)pom_vrchol;
};

Identifier:
        IDENTIFIER      {
#ifdef BRUTALDEBUG
printf("Identifier -> IDENTIFIER\n");
#endif
                pom_vrchol=terminal(); 
                pom_vrchol->opcode=TIDENTIFIER; 
                pom_vrchol->arg[0]=(void*)yylval;
                $$=(long)pom_vrchol; 
        };

CompoundStatement:
          '{' Statements '}' {
#ifdef BRUTALDEBUG
printf("CompoundStatement -> {Statements}\n");
#endif
	$$=$2;
};

Statements:
           {
#ifdef BRUTALDEBUG
printf("Statements ->           \n");
#endif
	$$=0;
}
          |Statement Statements {
#ifdef BRUTALDEBUG
printf("Statements -> Statement Statements\n");
#endif
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TStatements;
	pom_vrchol->arg[0]=(void*)$1;
	pom_vrchol->arg[1]=(void*)$2;
	$$=(long)pom_vrchol;
};

Statement:
          ';' {
#ifdef BRUTALDEBUG
printf("Statement -> ;\n");
#endif
	$$=0;
}
	  |MINMIN '>' {
#ifdef BRUTALDEBUG
printf("Zahazuji konec HTML komentare!\n");
#endif
	js_warning("HTML comment end in javascript ",c_radku,js_context_ptr);
	$$=0;
}
	  |'<' '!' MINMIN {
#ifdef BRUTALDEBUG
printf("Zahazuji zacatek HTML komentare!\n");
#endif
	js_warning("HTML comment begin in javascript ",c_radku,js_context_ptr);
	$$=0;
}
          |IF Condition Statement {
#ifdef BRUTALDEBUG
printf("Statement -> IF Condition Statement\n");
#endif
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TIF;
	pom_vrchol->arg[0]=(void*)$2;
	pom_vrchol->arg[1]=(void*)$3;
	$$=(long)pom_vrchol;
}
          |IF Condition Statement ELSE Statement {
#ifdef BRUTALDEBUG
printf("Statement -> IF Condition Statement ELSE Statement\n");
#endif
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TIF;
	pom_vrchol->arg[0]=(void*)$2;
	pom_vrchol->arg[1]=(void*)$3;
	pom_vrchol->arg[2]=(void*)$5;
	$$=(long)pom_vrchol;
}
          |WHILE Condition Statement {
#ifdef BRUTALDEBUG
printf("Statement -> WHILE Condition Statement\n");
#endif
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TWHILE;
	pom_vrchol->arg[0]=(void*)$2;
	pom_vrchol->arg[1]=(void*)$3;
	$$=(long)pom_vrchol;
}
          |ForParen ';' ExpressionOpt ';' ExpressionOpt ')' Statement {
#ifdef BRUTALDEBUG
printf("Statement -> ForParen;ExpressionOpt;ExpressionOpt) Statement\n");
#endif
	pom_vrchol=terminal();
        pom_vrchol->opcode=TFOR1;
	pom_vrchol->arg[0]=0;
	pom_vrchol->arg[1]=(void*)$3;
	pom_vrchol->arg[2]=(void*)$5;
	pom_vrchol->arg[3]=(void*)$7;
        $$=(long)pom_vrchol;
}
          |ForBegin ';' ExpressionOpt ';' ExpressionOpt ')' Statement {
#ifdef BRUTALDEBUG
printf("Statement -> ForBegin;ExpressionOpt;ExpressionOpt) Statement\n");
#endif
	pom_vrchol=(vrchol*)$1;
	pom_vrchol->arg[1]=(void*)$3;
	pom_vrchol->arg[2]=(void*)$5;
	pom_vrchol->arg[3]=(void*)$7;
	$$=(long)pom_vrchol;
}

          |ForBegin IN Expression ')' Statement {
#ifdef BRUTALDEBUG
printf("Statement -> ForBegin IN Expression) Statement\n");
#endif
	pom_vrchol=(vrchol*)$1;
	pom_vrchol->opcode=TFOR3;
	pom_vrchol->arg[1]=(void*)$3;
	pom_vrchol->arg[2]=(void*)$5;
	$$=(long)pom_vrchol;
}
          |BREAK ';' {
#ifdef BRUTALDEBUG
printf("Statement -> BREAK ;\n");
#endif
	pom_vrchol=terminal();
        pom_vrchol->opcode=TBREAK;
        $$=(long)pom_vrchol;
}
	  |BREAK {
#ifdef BRUTALDEBUG
printf("Statement -> BREAK\n");
#endif
	pom_vrchol=terminal();
	pom_vrchol->opcode=TBREAK;
	$$=(long)pom_vrchol;
	js_warning("Missing ';' after break ",c_radku,js_context_ptr);
}
          |CONTINUE ';' {
#ifdef BRUTALDEBUG
printf("Statement -> CONTINUE;\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TCONTINUE;
        $$=(long)pom_vrchol;
}
	  |CONTINUE {
#ifdef BRUTALDEBUG
printf("Statement -> CONTINUE\n");
#endif
	pom_vrchol=terminal();
	pom_vrchol->opcode=TCONTINUE;
	$$=(long)pom_vrchol;
	js_warning("Missing ';' after continue ",c_radku,js_context_ptr);
}
          |WITH '(' Expression ')' Statement {
#ifdef BRUTALDEBUG
printf("Statement -> WITH (Expression) Statement\n");
#endif
        pom_vrchol=neterminal();
        pom_vrchol->opcode=TWITH;
	pom_vrchol->arg[0]=(void*)$3;
	pom_vrchol->arg[1]=(void*)$5;
        $$=(long)pom_vrchol;
}
          |RETURN ExpressionOpt ';' {
#ifdef BRUTALDEBUG
printf("Statement -> RETURN ExpressionOpt;\n");
#endif
        pom_vrchol=neterminal();
        pom_vrchol->opcode=TRETURN;
	pom_vrchol->arg[0]=(void*)$2;
        $$=(long)pom_vrchol;
}
	  |RETURN ExpressionOpt {
#ifdef BRUTALDEBUG
printf("Statement -> RETURN ExpressionOpt\n");
#endif
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TRETURN;
	pom_vrchol->arg[0]=(void*)$2;
	$$=(long)pom_vrchol;
	js_warning("Missing ';' after return ",c_radku,js_context_ptr);
}
          |CompoundStatement {
#ifdef BRUTALDEBUG
printf("Statement -> CompoundStatement\n");
#endif
	$$=$1;
}
          |VariablesOrExpression ';' {
#ifdef BRUTALDEBUG
printf("Statement -> VariablesOrExpression;\n");
#endif
	$$=$1;
}
	  |VariablesOrExpression {
#ifdef BRUTALDEBUG
printf("Statement -> VariablesOrExpression\n");
#endif
	$$=$1;
	js_warning("Missing ';' at the end of the statement",c_radku-1,js_context_ptr);
};

Condition:
          '(' Expression ')' {
#ifdef BRUTALDEBUG
printf("Condition -> (Expression)\n");
#endif
	$$=$2;
};

ForParen:
          FOR '(' {
#ifdef BRUTALDEBUG
printf("ForParen -> FOR (\n");
#endif
	$$=0;
};

ForBegin:
          ForParen VariablesOrExpression {
#ifdef BRUTALDEBUG
printf("ForBegin -> ForParen VariablesOrExpression\n");
#endif
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TFOR1;
	pom_vrchol->arg[0]=(void*)$2;
	$$=(long)pom_vrchol;
};

VariablesOrExpression:
          VAR Variables {
#ifdef BRUTALDEBUG
printf("VariablesOrExpression -> VAR Variables\n");
#endif
        $$=$2;
}
          |Expression {
#ifdef BRUTALDEBUG
printf("VariablesOrExpression -> Expression\n");
#endif
	$$=$1;
};

Variables:
          Variable {
#ifdef BRUTALDEBUG
printf("Variables -> Variable\n");
#endif
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TVAR;
	pom_vrchol->arg[0]=(void*)$1;
	$$=(long)pom_vrchol;
}
          |Variable ',' Variables {
#ifdef BRUTALDEBUG
printf("Variables -> Variable, Variables\n");
#endif
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TVariables;
	pom_vrchol->arg[0]=(void*)$1;
	pom_vrchol->arg[1]=(void*)$3;
	$$=(long)pom_vrchol;
};

Variable:
          Identifier {
#ifdef BRUTALDEBUG
printf("Variable -> Identifier\n");
#endif
	$$=$1;
}
          |Identifier '=' AssignmentExpression {
#ifdef BRUTALDEBUG
printf("Variable -> Identifier = AssignmentExpression\n");
#endif
	pom_vrchol=neterminal();
	if(((vrchol*)$3)->opcode==TFUNCTIONDECL)
	{	pom_vrchol->opcode=TFUNCTIONDECL;
		pom_vrchol->arg[0]=(void*)$1; /* Odted je to DAG a zadnej podradnej strom :-) */
		pom_vrchol->arg[1]=((vrchol*)$3)->arg[1];
		pom_vrchol->arg[2]=((vrchol*)$3)->arg[2];
		pom_vrchol->arg[3]=(void*)$3;
	} else
	{
		pom_vrchol->opcode=TLocAssign;
		pom_vrchol->arg[0]=(void*)$1;
		pom_vrchol->arg[1]=(void*)$3;
	}
	$$=(long)pom_vrchol;
};

ExpressionOpt:
           {
#ifdef BRUTALDEBUG
printf("ExpressionOpt ->           \n");
#endif
	$$=0;
}
          |Expression {
#ifdef BRUTALDEBUG
printf("ExpressionOpt -> Expression\n");
#endif
	$$=$1;
};

Expression:
          AssignmentExpression {
#ifdef BRUTALDEBUG
printf("Expression -> AssignmentExpression\n");
#endif
	$$=$1;
}
          |AssignmentExpression ',' Expression {
#ifdef BRUTALDEBUG
printf("Expression -> AssignmentExpression, Expression\n");
#endif
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TCARKA;
	pom_vrchol->arg[0]=(void*)$1;
	pom_vrchol->arg[1]=(void*)$3;
	$$=(long)pom_vrchol;	
};

AssignmentExpression:
          ConditionalExpression {
#ifdef BRUTALDEBUG
printf("AssignmentExpression -> ConditionalExpression\n");
#endif
	$$=$1;
}
          |ConditionalExpression AssignmentOperator AssignmentExpression {
#ifdef BRUTALDEBUG
printf("AssignmentExpression -> ConditionalExpression AssignmentOperator AssignmentExpression\n");
#endif
	pom_vrchol=(vrchol*)$2;
/*        pom_vrchol->term=NETERM;*/
	if(((vrchol*)$3)->opcode==TFUNCTIONDECL)
	{	if(pom_vrchol->opcode!=TEQ)
		{	yyerror("");
			return 0; /* Mozna je to blbe */
		}else {	pom_vrchol->opcode=TFUNCTIONDECL;
			pom_vrchol->arg[0]=(void*)$1;
			pom_vrchol->arg[1]=((vrchol*)$3)->arg[1];
			pom_vrchol->arg[2]=((vrchol*)$3)->arg[2];
			pom_vrchol->arg[3]=(void*)$3;
		}
	}else{
	        pom_vrchol->arg[0]=(void*)$1;
        	pom_vrchol->arg[1]=(void*)$3;
	}
        $$=(long)pom_vrchol;
};

ConditionalExpression:
          OrExpression {
#ifdef BRUTALDEBUG
printf("ConditionalExpression -> OrExpression\n");
#endif
	$$=$1;
}
          |OrExpression '?' AssignmentExpression ':' AssignmentExpression {
#ifdef BRUTALDEBUG
printf("ConditionalExpression -> OrExpression?AssignmentExpression:AssignmentExpression\n");
#endif

};

OrExpression:
          AndExpression {
#ifdef BRUTALDEBUG
printf("OrExpression -> AndExpression\n");
#endif
	$$=$1;
}
          |OrExpression OROR AndExpression {
#ifdef BRUTALDEBUG
printf("OrExpression -> AndExpression || OrExpression\n");
#endif
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TOROR;
	pom_vrchol->arg[0]=(void*)$1;
	pom_vrchol->arg[1]=(void*)$3;
	$$=(long)pom_vrchol;
};

AndExpression:
          BitwiseOrExpression {
#ifdef BRUTALDEBUG
printf("AndExpression -> BitwiseOrExpression\n");
#endif
	$$=$1;
}
          |AndExpression ANDAND BitwiseOrExpression {
#ifdef BRUTALDEBUG
printf("AndExpression -> BitwiseOrExpression && AndExpression\n");
#endif
	pom_vrchol=neterminal();
        pom_vrchol->opcode=TANDAND;
        pom_vrchol->arg[0]=(void*)$1;
        pom_vrchol->arg[1]=(void*)$3;
        $$=(long)pom_vrchol;
};

BitwiseOrExpression:
          BitwiseXorExpression {
#ifdef BRUTALDEBUG
printf("BitwiseOrExpression -> BitwiseXorExpression\n");
#endif
	$$=$1;
}
          |BitwiseOrExpression '|' BitwiseXorExpression {
#ifdef BRUTALDEBUG
printf("BitwiseOrExpression -> BitwiseXorExpression|BitwiseOrExpression\n");
#endif
        pom_vrchol=neterminal();
        pom_vrchol->opcode=TOR;
        pom_vrchol->arg[0]=(void*)$1;
        pom_vrchol->arg[1]=(void*)$3;
        $$=(long)pom_vrchol;
};

BitwiseXorExpression:
          BitwiseAndExpression {
#ifdef BRUTALDEBUG
printf("BitwiseXorExpression -> BitwiseAndExpression\n");
#endif
	$$=$1;
}
          |BitwiseXorExpression '^' BitwiseAndExpression {
#ifdef BRUTALDEBUG
printf("BitwiseXorExpression -> BitwiseAndExpression^BitwiseXorExpression\n");
#endif
	pom_vrchol=neterminal();
        pom_vrchol->opcode=TXOR;
        pom_vrchol->arg[0]=(void*)$1;
        pom_vrchol->arg[1]=(void*)$3;
        $$=(long)pom_vrchol;
};

BitwiseAndExpression:
          EqualityExpression {
#ifdef BRUTALDEBUG
printf("BitwiseAndExpression -> EqualityExpression\n");
#endif
	$$=$1;
}
          |BitwiseAndExpression '&' EqualityExpression {
#ifdef BRUTALDEBUG
printf("BitwiseAndExpression -> EqualityExpression & BitwiseAndExpression\n");
#endif
        pom_vrchol=neterminal();
        pom_vrchol->opcode=TAND;
        pom_vrchol->arg[0]=(void*)$1;
        pom_vrchol->arg[1]=(void*)$3;
        $$=(long)pom_vrchol;
};

EqualityExpression:
          RelationalExpression {
#ifdef BRUTALDEBUG
printf("EqualityExpression -> RelationalExpression\n");
#endif
	$$=$1;
}
          |EqualityExpression EqualityOperator RelationalExpression {
#ifdef BRUTALDEBUG
printf("EqualityExpression -> RelationalExpression EqualityOperator EqualityExpression\n");
#endif
        pom_vrchol=(vrchol*)$2;
/*        pom_vrchol->term=NETERM;*/
        pom_vrchol->arg[0]=(void*)$1;
        pom_vrchol->arg[1]=(void*)$3;
        $$=(long)pom_vrchol;
};


RelationalExpression:
          ShiftExpression {
#ifdef BRUTALDEBUG
printf("RelationalExpression -> ShiftExpression\n");
#endif
	$$=$1;
}
          |RelationalExpression RelationalOperator ShiftExpression {
#ifdef BRUTALDEBUG
printf("RelationalExpression -> RelationalExpression RelationalOperator ShiftExpression\n");
#endif
        pom_vrchol=(vrchol*)$2;
/*        pom_vrchol->term=NETERM;*/
        pom_vrchol->arg[0]=(void*)$1;
        pom_vrchol->arg[1]=(void*)$3;
        $$=(long)pom_vrchol;
};

ShiftExpression:
          AdditiveExpression {
#ifdef BRUTALDEBUG
printf("ShiftExpression -> AdditiveExpression\n");
#endif
	$$=$1;
}
          |ShiftExpression ShiftOperator AdditiveExpression {
#ifdef BRUTALDEBUG
printf("ShiftExpression -> ShiftExpression ShiftOperator AdditiveExpression\n");
#endif
        pom_vrchol=(vrchol*)$2;
/*        pom_vrchol->term=NETERM;*/
        pom_vrchol->arg[0]=(void*)$1;
	pom_vrchol->arg[1]=(void*)$3;
        $$=(long)pom_vrchol;
};

AdditiveExpression:
          MultiplicativeExpression {
#ifdef BRUTALDEBUG
printf("AdditiveExpression -> MultiplicativeExpression\n");
#endif
	$$=$1;
}
          |AdditiveExpression '+' MultiplicativeExpression {
#ifdef BRUTALDEBUG
printf("AdditiveExpression -> MultiplicativeExpression + AdditiveExpression\n");
#endif
        pom_vrchol=neterminal();
        pom_vrchol->opcode=TPLUS;
        pom_vrchol->arg[0]=(void*)$1;
        pom_vrchol->arg[1]=(void*)$3;
        $$=(long)pom_vrchol;
}
          |AdditiveExpression '-' MultiplicativeExpression {
#ifdef BRUTALDEBUG
printf("AdditiveExpression -> MultiplicativeExpression - AdditiveExpression\n");
#endif
        pom_vrchol=neterminal();
        pom_vrchol->opcode=TMINUS;
        pom_vrchol->arg[0]=(void*)$1;
        pom_vrchol->arg[1]=(void*)$3;
        $$=(long)pom_vrchol;
};

MultiplicativeExpression:
          UnaryExpression {
#ifdef BRUTALDEBUG
printf("MultiplicativeExpression -> UnaryExpression\n");
#endif
	$$=$1;
}
          |MultiplicativeExpression MultiplicativeOperator UnaryExpression {
#ifdef BRUTALDEBUG
printf("MultiplicativeExpression -> MultiplicativeExpression MultiplicativeOperator UnaryExpression\n");
#endif
        pom_vrchol=(vrchol*)$2;
/*        pom_vrchol->term=NETERM;*/
        pom_vrchol->arg[0]=(void*)$1;
        pom_vrchol->arg[1]=(void*)$3;
        $$=(long)pom_vrchol;
};

/*ConvedUnaryExpression:
	UnaryExpression {
#ifdef BRUTALDEBUG
	printf("ConvedUnaryExpression->UnaryExpression\n");
#endif	
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TCONVERT;
	pom_vrchol->arg[0]=(void*)$1;
	$$=(long)pom_vrchol;
}*/

UnaryExpression:
          MemberExpression {
#ifdef BRUTALDEBUG
printf("UnaryExpression -> MemberExpression\n");
#endif
	$$=$1;
}
          |UnaryOperator UnaryExpression {
#ifdef BRUTALDEBUG
printf("UnaryExpression -> UnaryOperator UnaryExpression\n");
#endif
	pom_vrchol=(vrchol*)$1;
/*	pom_vrchol->term=NETERM;*/
	pom_vrchol->arg[0]=(void*)$2;
	$$=(long)pom_vrchol;
}
          |'-' UnaryExpression {
#ifdef BRUTALDEBUG
printf("UnaryExpression -> -UnaryExpression\n");
#endif
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TUNMIN;
	pom_vrchol->arg[0]=(void*)$2;
	$$=(long)pom_vrchol;
}
	  |'+' UnaryExpression {
#ifdef BRUTALDEBUG
printf("UnaryExpression -> +UnaryExpression\n");
#endif
	$$=$2;
}
          |IncrementOperator MemberExpression {
#ifdef BRUTALDEBUG
printf("UnaryExpression -> IncrementOperator MemberExpression\n");
#endif
	pom_vrchol=(vrchol*)$1;
/*	pom_vrchol->term=NETERM;*/
	pom_vrchol->arg[0]=(void*)$2;
	$$=(long)pom_vrchol;
}
          |MemberExpression IncrementOperator {
#ifdef BRUTALDEBUG
printf("UnaryExpression -> MemberExpression IncrementOperator\n");
#endif
	pom_vrchol=(vrchol*)$2;
/*	pom_vrchol->term=NETERM;*/
	pom_vrchol->arg[0]=(void*)$1;
	if(pom_vrchol->opcode==TPLUSPLUS)pom_vrchol->opcode=TPLUSPLUSPOST;
	else	if(pom_vrchol->opcode==TMINMIN)pom_vrchol->opcode=TMINMINPOST;
		else	{internal("Error! Divne opcody!\n");}
	$$=(long)pom_vrchol;
}
          |NEW Constructor {
#ifdef BRUTALDEBUG
printf("UnaryExpression -> NEW Constructor\n");
#endif
/*	pom_vrchol=neterminal();
	pom_vrchol->opcode=TNEW;
	pom_vrchol->arg[0]=(void*)$2;
	$$=(long)pom_vrchol;
*/
	$$=$2;
}
          |DELETE MemberExpression {
#ifdef BRUTALDEBUG
printf("UnaryExpression -> DELETE MemberExpression\n");
#endif
        pom_vrchol=neterminal();
        pom_vrchol->opcode=TDELETE;
        pom_vrchol->arg[0]=(void*)$2;
        $$=(long)pom_vrchol;
}
	  |TYPEOF UnaryExpression {
#ifdef BRUTALDEBUG
printf("UnaryExpression -> TYPEOF UnaryExpression\n");
#endif
	pom_vrchol=neterminal();
        pom_vrchol->opcode=TTYPEOF;
	pom_vrchol->arg[0]=(void*)$2;
        $$=(long)pom_vrchol;
}
	  |VOID UnaryExpression {
#ifdef BRUTALDEBUG
printf("UnaryExpression -> VOID UnaryExpression\n");
#endif
	pom_vrchol=neterminal();
        pom_vrchol->opcode=TVOID;
	pom_vrchol->arg[0]=(void*)$2;
        $$=(long)pom_vrchol;
};

Constructor:
          THIS '.' ConstructorCall {
#ifdef BRUTALDEBUG
printf("Constructor -> THIS . ConstructorCall\n");
#endif
	pom_vrchol=neterminal();
        pom_vrchol->opcode=TTHISCCall;
	pom_vrchol->arg[0]=(void*)$3;
        $$=(long)pom_vrchol;
}
          |ConstructorCall {
#ifdef BRUTALDEBUG
printf("Constructor -> ConstructorCall\n");
#endif
	$$=$1;
};

ConstructorCall:
          Identifier {
#ifdef BRUTALDEBUG
printf("ConstructorCall -> Identifier\n");
#endif
	pom_vrchol=neterminal();
	/* pom_vrchol->opcode=TIdCCall; */
	pom_vrchol->opcode=TECCall;
	pom_vrchol->arg[1]=0;
	pom_vrchol->arg[0]=(void*)$1;
	$$=(long)pom_vrchol;
}
          |Identifier '(' ArgumentListOpt ')' {
#ifdef BRUTALDEBUG
printf("ConstructorCall -> Identifier (ArgumentListOpt)\n");
#endif
	pom_vrchol=neterminal();
        pom_vrchol->opcode=TECCall;
	pom_vrchol->arg[0]=(void*)$1;
	pom_vrchol->arg[1]=(void*)$3;
        $$=(long)pom_vrchol;
}
          |Identifier '.' ConstructorCall {
#ifdef BRUTALDEBUG
printf("ConstructorCall -> Identifier.ConstructorCall\n");
#endif
	pom_vrchol=neterminal();
/*	pom_vrchol->opcode=TConsCall;*/
	pom_vrchol->opcode=TMember;
	pom_vrchol->arg[0]=(void*)$1;
	pom_vrchol->arg[1]=(void*)$3;
	$$=(long)pom_vrchol;
};

MemberExpression:
/*         M1Expression {
#ifdef BRUTALDEBUG
printf("MemberExpression -> PrimaryExpression\n");
#endif
	$$=$1;
}
          |ArrayExpression '.' MemberExpression {
#ifdef BRUTALDEBUG
printf("MemberExpression -> PrimaryExpression.MemberExpression\n");
#endif
	pom_vrchol=neterminal();
        pom_vrchol->opcode=TMember;
	pom_vrchol->arg[0]=(void*)$1;
	pom_vrchol->arg[1]=(void*)$3;
        $$=(long)pom_vrchol;
}
          |PrimaryExpression '[' Expression ']' {
#ifdef BRUTALDEBUG
printf("MemberExpression -> PrimaryExpression [Expression]\n");
#endif
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TArray;
	pom_vrchol->arg[0]=(void*)$1;
	pom_vrchol->arg[1]=(void*)$3;
	$$=(long)pom_vrchol;
}
          |M1Expression '(' ArgumentListOpt ')' {
#ifdef BRUTALDEBUG
printf("MemberExpression -> PrimaryExpression (ArgumentListOpt)\n");
#endif
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TFunctionCall;
	pom_vrchol->arg[0]=(void*)$1;
	pom_vrchol->arg[1]=(void*)$3;
	$$=(long)pom_vrchol;
}
;

M1Expression:*/
	ArrayExpression {
#ifdef BRUTALDEBUG
printf("MemberExpression -> PrimaryExpression\n");
#endif
	$$=$1;
}
	|MemberExpression '.' PrimaryExpression {
#ifdef BRUTALDEBUG
printf("MemberExpression -> ArrayExpression.M1Expression\n");
#endif
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TMember;
	pom_vrchol->arg[0]=(void*)$1;
	pom_vrchol->arg[1]=(void*)$3;
	$$=(long)pom_vrchol;
	if(((vrchol*)$1)->opcode==TArray)
		js_warning("Array operator followed by member operator ",c_radku,js_context_ptr);
}
	|MemberExpression '[' Expression ']' {
#ifdef BRUTALDEBUG
printf("MemberExpression -> M1Expression [Expression]\n");
#endif
	pom_vrchol=neterminal();
        pom_vrchol->opcode=TArray;
        pom_vrchol->arg[0]=(void*)$1;
        pom_vrchol->arg[1]=(void*)$3;
        $$=(long)pom_vrchol;
	if(((vrchol*)$1)->opcode==TArray)js_warning("Two array operators in same expression ",c_radku,js_context_ptr);
}
	|MemberExpression '(' ArgumentListOpt ')' {
#ifdef BRUTALDEBUG
	printf("MemberExpression -> PrimaryExpression (ArgumentListOpt)\n");
#endif
        pom_vrchol=neterminal();
        pom_vrchol->opcode=TFunctionCall;
        pom_vrchol->arg[0]=(void*)$1;
        pom_vrchol->arg[1]=(void*)$3;
        $$=(long)pom_vrchol;
};	

ArrayExpression:
	PrimaryExpression {
#ifdef BRUTALDEBUG
printf("ArrayExpression->PrimaryExpression\n");
#endif
	$$=$1;
}
	|ArrayExpression '[' Expression ']' {
#ifdef BRUTALDEBUG
printf("MemberExpression -> PrimaryExpression [Expression]\n");
#endif
	pom_vrchol=neterminal();
	pom_vrchol->opcode=TArray;
	pom_vrchol->arg[0]=(void*)$1;
	pom_vrchol->arg[1]=(void*)$3;
	$$=(long)pom_vrchol;
	if(((vrchol*)$1)->opcode==TArray)js_warning("Two array operators in same expression ",c_radku,js_context_ptr);
};	
	
ArgumentListOpt:
           {
#ifdef BRUTALDEBUG
printf("ArgumentListOpt ->      \n");
#endif
	$$=0;
}
          |ArgumentList {
#ifdef BRUTALDEBUG
printf("ArgumentListOpt -> ArgumentList\n");
#endif
	$$=$1;
};

ArgumentList:
          AssignmentExpression {
#ifdef BRUTALDEBUG
printf("ArgumentList -> AssignmentExpression\n");
#endif
	$$=$1;
}
          |AssignmentExpression ',' ArgumentList {
#ifdef BRUTALDEBUG
printf("ArgumentList -> AssignmentExpression, ArgumentList\n");
#endif
	pom_vrchol=terminal();
        pom_vrchol->opcode=TArgumentList;
	pom_vrchol->arg[0]=(void*)$1;
	pom_vrchol->arg[1]=(void*)$3;
        $$=(long)pom_vrchol;
};

PrimaryExpression:
          '(' Expression ')' {
#ifdef BRUTALDEBUG
printf("PrimaryExpression -> (Expression)\n");
#endif
	$$=$2;
}
          |Identifier {
#ifdef BRUTALDEBUG
printf("PrimaryExpression -> Identifier\n");
#endif
	$$=$1;
}
          |NUMLIT {
#ifdef BRUTALDEBUG
printf("PrimaryExpression -> NUMLIT\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TNUMLIT;
	pom_vrchol->arg[0]=(void*)yylval;
        $$=(long)pom_vrchol;
}
          |STRINGLIT {
#ifdef BRUTALDEBUG
printf("PrimaryExpression -> STRINGLIT\n");
#endif
	pom_vrchol=terminal();
        pom_vrchol->opcode=TSTRINGLIT;
	pom_vrchol->arg[0]=(void*)yylval;
        $$=(long)pom_vrchol;
}
          |FALSELIT {
#ifdef BRUTALDEBUG
printf("PrimaryExpression -> FALSELIT\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TFALSELIT;
        $$=(long)pom_vrchol;
}
          |TRUELIT {
#ifdef BRUTALDEBUG
printf("PrimaryExpression -> TRUELIT\n");
#endif
        pom_vrchol=terminal();
        pom_vrchol->opcode=TTRUELIT;
        $$=(long)pom_vrchol;
}
          |NULLLIT {
#ifdef BRUTALDEBUG
printf("PrimaryExpression -> NULLLIT\n");
#endif
	pom_vrchol=terminal();
	pom_vrchol->opcode=TNULLLIT;
	$$=(long)pom_vrchol;
}
          |THIS {
#ifdef BRUTALDEBUG
printf("PrimaryExpression -> THIS\n");
#endif
	pom_vrchol=terminal();
	pom_vrchol->opcode=TTHIS;
	$$=(long)pom_vrchol;
};

%%
int yyerror(char*a){
	unsigned char txt1[MAX_STR_LEN];
	unsigned char *txt;
	int i=0,j=0,k=0;
	struct f_data_c *fd=(struct f_data_c*)(js_context_ptr->ptr);
	struct terminal *term=fd->ses->term;
/*	snprintf(txt1,MAX_STR_LEN,"Bug in line %d!\n",c_radku);*/
	

	if((yychar==STRINGLIT)||(yychar==NUMLIT))
	{	if(yylval)js_mem_free((void*)yylval);
		else internal("Trying to free null pointer!");
	}
	while(i<c_radku && js_context_ptr->code[j])
	{	if((js_context_ptr->code[j]=='\n')||(js_context_ptr->code[j]=='\r'))i++;
		j++;
	}
	if(js_context_ptr->code[j])
	{	k=j;
		while(js_context_ptr->code[k] && js_context_ptr->code[k]!='\n' && js_context_ptr->code[k]!='\r')k++;
		if(js_context_ptr->code[k])
		{	i=1;
			js_context_ptr->code[k]='\0';
		} else	i=0;
	}
	snprintf(txt1,MAX_STR_LEN,"Bug in line: %s\n",&js_context_ptr->code[j]);
	if(i)	js_context_ptr->code[k]='\n';
	txt1[MAX_STR_LEN -1]='\0';

	if (strlen(txt1)>MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU)
	{
		txt1[MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU-1]='.';
		txt1[MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU-2]='.';
		txt1[MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU-3]='.';
		txt1[MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU-4]=' ';
		txt1[MAGICKA_KONSTANTA_NA_MAXIMALNI_DYLKU_JS_KODU_PRI_ERRORU]=0;
	}
	
	if (js_verbose_errors)
	{
		skip_nonprintable(txt1);
		if (fd->f_data)
		{
			struct conv_table* ct;
				
			ct=get_translation_table(fd->f_data->cp,fd->f_data->opt.cp);
			txt=convert_string(ct,txt1,strlen(txt1),&(fd->f_data->opt));
		}
		else
		{
			txt=stracpy(txt1);
		}
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

	zrus_strom(js_last);
	js_last=0;
	js_strom=0;
	js_context_ptr->js_tree=0;
	js_context_ptr->jsem_dead=1;
	js_context_ptr->current=0; /* Fix in order to race condition */
	js_context_ptr->zaplatim=1;
	return 0;
}

#endif
