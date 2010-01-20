/*
 * lexswitch -- switch lex tables
 */

/*
 * Bob Denny	 28-Aug-82  Remove reference to stdio.h
 * Scott Guthery 20-Nov-83	Adapt for IBM PC & DeSmet C
 */

#include "lex.h"

extern struct lextab *_tabp;

struct lextab *
lexswitch(lp)
struct lextab *lp;
{
        register struct lextab *olp;

        olp = _tabp;
        _tabp = lp;
        return(olp);
}
