/*
  LICENSE
  -------
Copyright 2005 Nullsoft, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer. 

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 

  * Neither the name of Nullsoft nor the names of its contributors may be used to 
    endorse or promote products derived from this software without specific prior written permission. 
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include "lex.h"

#define ERROR   256     /* yacc's value */

static int llset(void);
static int llinp(char **exp);
static int lexgetc(char **exp)
{
  char c= **exp;
  if (c) (*exp)++;
  return( c != 0 ? c : -1);
}
static int tst__b(register int c, char tab[])
{
  return (tab[(c >> 3) & 037] & (1 << (c & 07)) );
}

static char    *llsave[16];             /* Look ahead buffer            */
static char    llbuf[100];             /* work buffer                          */
static char    *llp1   = &llbuf[0];    /* pointer to next avail. in token      */
static char    *llp2   = &llbuf[0];    /* pointer to end of lookahead          */
static char    *llend  = &llbuf[0];    /* pointer to end of token              */
static char    *llebuf = &llbuf[sizeof llbuf];
static int     lleof;
static int     yyline  = 0;
extern struct lextab lextab;

int gettoken(char *lltb, int lltbsiz)
{
        register char *lp, *tp, *ep;

        tp = lltb;
        ep = tp+lltbsiz-1;
        for (lp = llbuf; lp < llend && tp < ep;)
                *tp++ = *lp++;
        *tp = 0;
        return(tp-lltb);
}


int yylex(char **exp)
{
  register int c, st;
  int final, l, llk, i;
  register struct lextab *lp;
  char *cp;

  while (1)
  {
    llk = 0;
    if (llset()) return(0);
    st = 0;
    final = -1;
    lp = &lextab;

    do {
            if (lp->lllook && (l = lp->lllook[st])) {
                    for (c=0; c<NBPW; c++)
                            if (l&(1<<c))
                                    llsave[c] = llp1;
                    llk++;
            }
            if ((i = lp->llfinal[st]) != -1) {
                    final = i;
                    llend = llp1;
            }
            if ((c = llinp(exp)) < 0)
                    break;
            if ((cp = lp->llbrk) && llk==0 && tst__b(c, cp)) {
                    llp1--;
                    break;
            }
    } while ((st = (*lp->llmove)(lp, c, st)) != -1);


    if (llp2 < llp1)
            llp2 = llp1;
    if (final == -1) {
            llend = llp1;
            if (st == 0 && c < 0)
                    return(0);
            if ((cp = lp->llill) && tst__b(c, cp)) {
                    continue;
            }
            return(ERROR);
    }
    if (c = (final >> 11) & 037)
            llend = llsave[c-1];
    if ((c = (*lp->llactr)(final&03777)) >= 0)
            return(c);
  }
}

void llinit(viud)
{
   llp1 = llp2 = llend = llbuf;
   llebuf = llbuf + sizeof(llbuf);
   lleof = yyline = 0;
}


static int llinp(char **exp)
{
        register c;
        register struct lextab *lp;
        register char *cp;

        lp = &lextab;
        cp = lp->llign;                         /* Ignore class         */
        for (;;) {
                /*
                 * Get the next character from the save buffer (if possible)
                 * If the save buffer's empty, then return EOF or the next
                 * input character.  Ignore the character if it's in the
                 * ignore class.
                 */
                c = (llp1 < llp2) ? *llp1 & 0377 : (lleof) ? EOF : lexgetc(exp);
                if (c >= 0) {                   /* Got a character?     */
                        if (cp && tst__b(c, cp))
                                continue;       /* Ignore it            */
                        if (llp1 >= llebuf) {   /* No, is there room?   */
                                return -1;
                        }
                        *llp1++ = c;            /* Store in token buff  */
                } else
                        lleof = 1;              /* Set EOF signal       */
                return(c);
        }
}

static int llset(void)
/*
 * Return TRUE if EOF and nothing was moved in the look-ahead buffer
 */
{
        register char *lp1, *lp2;

        for (lp1 = llbuf, lp2 = llend; lp2 < llp2;)
                *lp1++ = *lp2++;
        llend = llp1 = llbuf;
        llp2 = lp1;
        return(lleof && lp1 == llbuf);
}
