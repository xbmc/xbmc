/*
 * Bob Denny 28-Aug-82  Remove reference to FILE *lexin to
 * 						eliminate dependency on standard I/O library. Only
 * 						lexgetc() used it, and it's there now.
 *						Add EOF	definition for standalone uses.
 *						Corrected comment for llnxtmax.
 *
 * Scott Guthery 20-Nov-83	Adapt for IBM PC & DeSmet C.  Removed
 *							equivalence of yylval and lexval since
 *							a multi-typed parser wants yylval to be
 *							typed to be the union of the types (YYSTYPE).
 */

/*
 * lex library header file -- accessed through
 *      #include <lex.h>
 */

#include <stdio.h>

/*
 * Description of scanning tables. The entries at the front of
 * the struct must remain in place for the assembler routines to find.
 */
struct  lextab {
        int     llendst;                /* Last state number            */
        char    *lldefault;             /* Default state table          */
        char    *llnext;                /* Next state table             */
        char    *llcheck;               /* Check table                  */
        int     *llbase;                /* Base table                   */
        int     llnxtmax;               /* Last in next table           */
        int     (*llmove)();            /* Move between states          */
        char     *llfinal;               /* Final state descriptions     */
        int     (*llactr)();            /* Action routine               */
        int     *lllook;                /* Look ahead vector if != NULL */
        char    *llign;                 /* Ignore char vec if != NULL   */
        char    *llbrk;                 /* Break char vec if != NULL    */
        char    *llill;                 /* Illegal char vec if != NULL  */
};

#define NBPW		 16
#define LEXERR		256
#define LEXSKIP		(-1)
#define EOF			(-1)
//#define NULL		 (0)
#define LEXECHO(fp) {lexecho((fp));}

#define lextext llbuf
#define lexlast llend

extern FILE *lexin;
extern llstin();

