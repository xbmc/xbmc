/*
 * Bob Denny	 28-Aug-82  Remove reference to stdio.h
 * Scott Guthery 20-Nov-83	Adapt for IBM PC & DeSmet C
 */

#include "lex.h"

_lmovb(lp, c, st)
register int c, st;
register struct lextab *lp;
{
        int base;

        while ((base = lp->llbase[st]+c) > lp->llnxtmax ||
                        (lp->llcheck[base] & 0377) != st) {

                if (st != lp->llendst) {
/*
 * This miscompiled on Decus C many years ago:
 *                      st = lp->lldefault[st] & 0377;
 */
                        base = lp->lldefault[st] & 0377;
                        st = base;
                }
                else
                        return(-1);
        }
        return(lp->llnext[base]&0377);
}
