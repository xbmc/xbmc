%{
#define YYSTYPE double
#include "cal_tab.h"
int c;

extern YYSTYPE yylval;
double translate(int type);
void count(void);
void setLastVar(void);
int lookup(int *typeOfObject);
struct lextab *lexswitch(struct lextab *lp);


#define INTCONST 1
#define DBLCONST 2
#define HEXCONST 3
#define VARIABLE 4
#define OTHER    5

%}

digit =     [0-9];
letter =    [a-zA-Z_];
hex =       [a-fA-F0-9];
/* -- */
space           = [\40];
/*number          =  (digit* | "-" digit*);*/
number          = digit*;
exp             = [Ee] number;
doubl           = number "." (digit* | digit* exp);

%%
hex hex* [hH]             { count(); yylval = translate(HEXCONST); return VALUE; }
digit*                    { count(); yylval = translate(INTCONST); return VALUE; }
digit digit* [Dd]         { count(); yylval = translate(INTCONST); return VALUE; }
doubl               	  { count(); yylval = translate(DBLCONST); return VALUE; }
letter*                   { count(); { int typeOfObject; setLastVar(); yylval = lookup(&typeOfObject); return typeOfObject; }}
letter (letter|digit)*    { count(); { int typeOfObject; setLastVar(); yylval = lookup(&typeOfObject); return typeOfObject; }}
'+'			  { count(); return '+'; }
'-'			  { count(); return '-'; }
'*'			  { count(); return '*'; }
'/'			  { count(); return '/'; }
'%'			  { count(); return '%'; }
'&'			  { count(); return '&'; }
'|'			  { count(); return '|'; }
'('			  { count(); return '('; }
')'			  { count(); return ')'; }
'='			  { count(); return '='; }
','                       { count(); return ','; }
';'                       { count(); return ';'; }
[ \t\v\f]		  { count(); }
.               	  { count(); }

%%
