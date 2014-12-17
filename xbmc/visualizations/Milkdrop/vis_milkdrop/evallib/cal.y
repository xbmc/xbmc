
       %{
       #define YYSTYPE double
       #include <malloc.h>
       #include <memory.h>
       #include "Compiler.h"
       #include "eval.h"

       yyerror(char *);
       yylex();

       extern int yyStackSize;
       extern double result;

       int regs[26];
       int base;

       %}

       %token VALUE IDENTIFIER FUNCTION1 FUNCTION2 FUNCTION3

       %left '|' 
       %left '&'
       %left '+' '-'
       %left '*' '/' '%'
       %left UMINUS /*supplies precedence for unary minus */
       %left UPLUS /*supplies precedence for unary plus */

       %%      /*beginning of rules section */

       stat    :       math_expr 
		       {       $$ = $1; result = $1;     }
	       |       IDENTIFIER '=' math_expr 
		       {       if (parseType == PARSE_EVAL)
                                 {
                                 setVar((int)$1, $3);  
                                 $$ = $3;
			         result = $3;
                                 }
                               else
                                 {
                                 double i = setVar((int)$1, 0);
                                 double v = createCompiledValue(0, &(varTable[(int)i].value));
                                 $$ = createCompiledFunction2(MATH_SIMPLE, FN_ASSIGN, v, $3);
                                 result = $$;
                                 }
                       }
	       ;

       value   :       VALUE { $$ = $1 }


       primary_expr
               :       IDENTIFIER
		       {       $$ = getVar((int)$1);}
               |       value
		       {       $$ = $1;}
               |       '(' math_expr ')'
		       {       $$ = $2;}
               ;

       math_expr
               :       primary_expr
                       { $$ = $1;              }
               |       math_expr '*' math_expr
		       {       if (parseType == PARSE_EVAL)
                                  $$ = $1 * $3;
                               else
                                  $$ = createCompiledFunction2(MATH_SIMPLE, FN_MULTIPLY, $1, $3);
                       }
	       |       math_expr '/' math_expr
		       {       if (parseType == PARSE_EVAL)
                                  $$ = $1 / $3;
                               else
                                  $$ = createCompiledFunction2(MATH_SIMPLE, FN_DIVIDE, $1, $3);
                       }
	       |       math_expr '%' math_expr
		       {       if (parseType == PARSE_EVAL)
                                  $$ = (double)((int)$1 % (int)$3);
                               else
                                  $$ = createCompiledFunction2(MATH_SIMPLE, FN_MODULO, $1, $3);
                       }
	       |       math_expr '+' math_expr
		       {       if (parseType == PARSE_EVAL)
		                  $$ = $1 + $3;   
                               else
                                  $$ = createCompiledFunction2(MATH_SIMPLE, FN_ADD, $1, $3);
                       }
	       |       math_expr '-' math_expr
		       {       if (parseType == PARSE_EVAL)
		                  $$ = $1 - $3;   
                               else
                                  $$ = createCompiledFunction2(MATH_SIMPLE, FN_SUB, $1, $3);
                       }
	       |       math_expr '&' math_expr 
		       {       if (parseType == PARSE_EVAL)
		                  $$ = (double)((int)$1 & (int)$3);   
                               else
                                  $$ = createCompiledFunction2(MATH_SIMPLE, FN_AND, $1, $3);
                       }
	       |       math_expr '|' math_expr
		       {       if (parseType == PARSE_EVAL)
		                  $$ = (double)((int)$1 | (int)$3);   
                               else
                                  $$ = createCompiledFunction2(MATH_SIMPLE, FN_OR, $1, $3);
                       }
	       |       '-' math_expr %prec UMINUS
		       {       if (parseType == PARSE_EVAL)
		                  $$ = -$2;       
                               else
                                  $$ = createCompiledFunction1(MATH_SIMPLE, FN_UMINUS, $2);
                       }
	       |       '+' math_expr %prec UPLUS
		       {       if (parseType == PARSE_EVAL)
		                  $$ = +$2;       
                               else
                                  $$ = createCompiledFunction1(MATH_SIMPLE, FN_UPLUS, $2);
                       }
               |       fonction
                       {       $$ = $1;        }
	       ;

       fonction
               :       FUNCTION1 '(' math_expr ')'
		       {       if (parseType == PARSE_EVAL)
                                  $$ = calcFunction1((int)$1, $3);
                               else
                                  $$ = createCompiledFunction1(MATH_FN, (int)$1, $3);
                       }
               |       FUNCTION2 '(' math_expr ',' math_expr ')'
		       {       if (parseType == PARSE_EVAL)
                                  $$ = calcFunction2((int)$1, $3, $5);
                               else
                                  $$ = createCompiledFunction2(MATH_FN, (int)$1, $3, $5);
                       }
               |       FUNCTION3 '(' math_expr ',' math_expr ',' math_expr ')'
		       {       if (parseType == PARSE_EVAL)
                                  $$ = calcFunction3((int)$1, $3, $5, $7);
                               else
                                  $$ = createCompiledFunction3(MATH_FN, (int)$1, $3, $5, $7);
                       }
               ;
             


       %%
       main()
       {
	       return(yyparse());
       }

       yywrap()
       {
	       return(1);
       }
