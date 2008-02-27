////////////////////////////////////////////////////////////////////////////////
// RegExp.cpp
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>
#include "RegExp.h"

#ifdef HAS_PCRE

#ifdef _WIN32
#include "lib/libpcre/pcre.h"
#else
#include <pcre.h>
#endif
#include "include.h"
#include "log.h"

using namespace PCRE;

CRegExp::CRegExp()
{
  m_re          = NULL;
//  m_iOptions    = PCRE_NEWLINE_ANY | PCRE_DOTALL | PCRE_UTF8;
#if PCRE_MAJOR>6
  m_iOptions    = PCRE_NEWLINE_ANY | PCRE_DOTALL;
#else
#ifdef __GNUC__
#warning Old version of PCRE detected, XBMC requires PCRE version 7.0+ for full functionality.
#endif
  m_iOptions    = PCRE_DOTALL;
#endif
  m_bMatched    = false;
  m_iMatchCount = 0;
}

CRegExp::~CRegExp()
{
  Cleanup();
}

CRegExp* CRegExp::RegComp(const char *re)
{
  if (!re)
    return NULL;

  m_bMatched         = false;
  m_iMatchCount      = 0;
  const char *errMsg = NULL;
  int errOffset      = 0;

  Cleanup();
 
  m_re = pcre_compile(re, m_iOptions, &errMsg, &errOffset, NULL);
  if (!m_re)
  {
    CLog::Log(LOGERROR, "PCRE: %s. Compilation failed at offset %d in expression '%s'",
              errMsg, errOffset, re);
    return NULL;
  }

  return this;
}

int CRegExp::RegFind(const char* str)
{
  m_bMatched    = false;
  m_iMatchCount = 0;

  if (!m_re)
  {
    CLog::Log(LOGERROR, "PCRE: Called before compilation");
    return -1;
  }
  
  if (!str)
  {
    CLog::Log(LOGERROR, "PCRE: Called without a string to match");
    return -1;
  }
  
  m_subject = str;
  int rc = pcre_exec(m_re, NULL, str, strlen(str), 0, 0, m_iOvector, OVECCOUNT);

  if (rc<1)
  {
    switch(rc)
    {
    case PCRE_ERROR_NOMATCH:
      return -1;
      
    case PCRE_ERROR_MATCHLIMIT:
      CLog::Log(LOGERROR, "PCRE: Match limit reached");
      return -1;

    default:
      CLog::Log(LOGERROR, "PCRE: Unknown error: %d", rc);
      return -1;
    }
  }
  m_bMatched = true;
  m_iMatchCount = rc;
  return m_iOvector[0];
}

char* CRegExp::GetReplaceString( const char* sReplaceExp )
{
  char *src = (char *)sReplaceExp;
  char *buf;
  char c;
  int no;
  size_t len;
  
  if( sReplaceExp == NULL || !m_bMatched )
    return NULL;
  
  
  // First compute the length of the string
  int replacelen = 0;
  while ((c = *src++) != '\0') 
  {
    if (c == '&')
      no = 0;
    else if (c == '\\' && isdigit(*src))
      no = *src++ - '0';
    else
      no = -1;
    
    if (no < 0) 
    {	
      // Ordinary character. 
      if (c == '\\' && (*src == '\\' || *src == '&'))
        c = *src++;
      replacelen++;
    } 
    else if (no < m_iMatchCount && (m_iOvector[no*2]>=0))
    {
      // Get tagged expression
      len = m_iOvector[no*2+1] - m_iOvector[no*2];
      replacelen += len;
    }
  }
  
  // Now allocate buf
  buf = (char *)malloc((replacelen + 1)*sizeof(char));
  if( buf == NULL )
    return NULL;
  
  char* sReplaceStr = buf;
  
  // Add null termination
  buf[replacelen] = '\0';
  
  // Now we can create the string
  src = (char *)sReplaceExp;
  while ((c = *src++) != '\0') 
  {
    if (c == '&')
      no = 0;
    else if (c == '\\' && isdigit(*src))
      no = *src++ - '0';
    else
      no = -1;
    
    if (no < 0) 
    {	
      // Ordinary character. 
      if (c == '\\' && (*src == '\\' || *src == '&'))
        c = *src++;
      *buf++ = c;
    }
    else if (no < m_iMatchCount && (m_iOvector[no*2]>=0)) 
    {
      // Get tagged expression
      len = m_iOvector[no*2+1] - m_iOvector[no*2];
      strncpy(buf, m_subject.c_str()+m_iOvector[no*2], len);
      buf += len;
    }
  }
  
  return sReplaceStr;  
}

#else // HAS_PCRE

// definition	number	opnd?	meaning 
#define	END		0		// no	End of program. 
#define	BOL		1		// no	Match beginning of line. 
#define	EOL		2		// no	Match end of line. 
#define	ANY		3		// no	Match any character. 
#define	ANYOF	4		// str	Match any of these. 
#define	ANYBUT	5		// str	Match any but one of these. 
#define	BRANCH	6		// node	Match this, or the next..\&. 
#define	BACK	7		// no	"next" ptr points backward. 
#define	EXACTLY	8		// str	Match this string. 
#define	NOTHING	9		// no	Match empty string. 
#define	STAR	10		// node	Match this 0 or more times. 
#define	PLUS	11		// node	Match this 1 or more times. 
#define	OPEN	20		// no	Sub-RE starts here. 
						//		OPEN+1 is number 1, etc. 
#define	CLOSE	30		// no	Analogous to OPEN. 

// Utility definitions.
 
#define	FAIL(m)		{ regerror(m); return(NULL); }
#define	ISREPN(c)	((c) == '*' || (c) == '+' || (c) == '?')
#define	META		"^$.[()|?+*\\"

// Flags to be passed up and down.
 
#define	HASWIDTH	01	// Known never to match null string. 
#define	SIMPLE		02	// Simple enough to be STAR/PLUS operand. 
#define	SPSTART		04	// Starts with * or +. 
#define	WORST		0	// Worst case. 

int isdigit(char ch)
{
  if (ch >= '0' && ch <= '9') return 1;
  return 0;
}

CRegExp::CRegExp()
{
	bCompiled = FALSE;
	program = NULL;
	sFoundText = NULL;

	for( int i = 0; i < NSUBEXP; i++ )
	{
		startp[i] = NULL;
		endp[i] = NULL;
	}
}

CRegExp::~CRegExp()
{
  if (program)
	  free(program);
	if (sFoundText)
    free(sFoundText);
}


CRegExp* CRegExp::RegComp(const char *exp)
{
	char *scan;
	int flags;

	if (exp == NULL)
		return NULL;
	
	bCompiled = TRUE;

	// First pass: determine size, legality. 
	bEmitCode = FALSE;
	regparse = (char *)exp;
	regnpar = 1;
	regsize = 0L;
	regdummy[0] = NOTHING;
	regdummy[1] = regdummy[2] = 0;
	regcode = regdummy;
	if (reg(0, &flags) == NULL)
		return(NULL);

	// Allocate space.
  if (program)
	  free(program);
	program = (char *)malloc(regsize*sizeof(char));
	memset( program, 0, regsize * sizeof(char) );

	if (program == NULL)
		return NULL;

	// Second pass: emit code. 
	bEmitCode = TRUE;
	regparse = (char *)exp;
	regnpar = 1;
	regcode = program;
	if (reg(0, &flags) == NULL)
		return NULL;

	// Dig out information for optimizations. 
	regstart = '\0';		// Worst-case defaults. 
	reganch = 0;
	regmust = NULL;
	regmlen = 0;
	scan = program;		// First BRANCH. 
	if (OP(regnext(scan)) == END) 
	{	
		// Only one top-level choice. 
		scan = OPERAND(scan);

		// Starting-point info. 
		if (OP(scan) == EXACTLY)
			regstart = *OPERAND(scan);
		else if (OP(scan) == BOL)
			reganch = 1;

		// If there's something expensive in the r.e., find the
		// longest literal string that must appear and make it the
		// regmust.  Resolve ties in favor of later strings, since
		// the regstart check works with the beginning of the r.e.
		// and avoiding duplication strengthens checking.  Not a
		// strong reason, but sufficient in the absence of others.
		 
		if (flags&SPSTART) 
		{
			LPTSTR longest = NULL;
			size_t len = 0;

			for (; scan != NULL; scan = regnext(scan))
				if (OP(scan) == EXACTLY && strlen(OPERAND(scan)) >= len) 
				{
					longest = OPERAND(scan);
					len = strlen(OPERAND(scan));
				}
			regmust = longest;
			regmlen = (int)len;
		}
	}

	return this;
}

// reg - regular expression, i.e. main body or parenthesized thing
//
// Caller must absorb opening parenthesis.
//
// Combining parenthesis handling with the base level of regular expression
// is a trifle forced, but the need to tie the tails of the branches to what
// follows makes it hard to avoid.
 


char *CRegExp::reg(int paren, int *flagp)
{
	char *ret = NULL;
	char *br;
	char *ender;
	int parno = 0;
	int flags;

	*flagp = HASWIDTH;	// Tentatively. 

	if (paren) 
	{
		// Make an OPEN node. 
		if (regnpar >= NSUBEXP)
		{
//			TRACE1("Too many (). NSUBEXP is set to %d\n", NSUBEXP );
			return NULL;
		}
		parno = regnpar;
		regnpar++;
		ret = regnode((char)(OPEN+parno));
	}

  // Pick up the branches, linking them together. 
	br = regbranch(&flags);
	if (br == NULL)
		return(NULL);
	if (paren)
		regtail(ret, br);	// OPEN -> first. 
	else
		ret = br;
	*flagp &= ~(~flags&HASWIDTH);	// Clear bit if bit 0. 
	*flagp |= flags&SPSTART;
	while (*regparse == '|') {
		regparse++;
		br = regbranch(&flags);
		if (br == NULL)
			return(NULL);
		regtail(ret, br);	// BRANCH -> BRANCH. 
		*flagp &= ~(~flags&HASWIDTH);
		*flagp |= flags&SPSTART;
	}

	// Make a closing node, and hook it on the end. 
	ender = regnode((char)((paren) ? CLOSE+parno : END));
	regtail(ret, ender);

	// Hook the tails of the branches to the closing node. 
	for (br = ret; br != NULL; br = regnext(br))
		regoptail(br, ender);

	// Check for proper termination. 
	if (paren && *regparse++ != ')') 
	{
		printf("unterminated ()\n");
		return NULL;
	} 
	else if (!paren && *regparse != '\0') 
	{
		if (*regparse == ')') 
		{
			printf("unmatched ()\n");
			return NULL;
		} 
		else
		{
			printf("internal error: junk on end\n");
			return NULL;
		}
		// NOTREACHED 
	}

	return(ret);
}




//
// regbranch - one alternative of an | operator
//
// Implements the concatenation operator.
 
char *CRegExp::regbranch(int *flagp)
{
	char *ret;
	char *chain;
	char *latest;
	int flags;
	int c;

	*flagp = WORST;				// Tentatively. 

	ret = regnode(BRANCH);
	chain = NULL;
	while ((c = *regparse) != '\0' && c != '|' && c != ')') {
		latest = regpiece(&flags);
		if (latest == NULL)
			return(NULL);
		*flagp |= flags&HASWIDTH;
		if (chain == NULL)		// First piece. 
			*flagp |= flags&SPSTART;
		else
			regtail(chain, latest);
		chain = latest;
	}
	if (chain == NULL)			// Loop ran zero times. 
		(void) regnode(NOTHING);

	return(ret);
}

//
// regpiece - something followed by possible [*+?]
//
// Note that the branching code sequences used for ? and the general cases
// of * and + are somewhat optimized:  they use the same NOTHING node as
// both the endmarker for their branch list and the body of the last branch.
// It might seem that this node could be dispensed with entirely, but the
// endmarker role is not redundant.
 
char *CRegExp::regpiece(int *flagp)
{
	char *ret;
	char op;
	char *next;
	int flags;

	ret = regatom(&flags);
	if (ret == NULL)
		return(NULL);

	op = *regparse;
	if (!ISREPN(op)) {
		*flagp = flags;
		return(ret);
	}

	if (!(flags&HASWIDTH) && op != '?')
	{
		printf("*+ operand could be empty\n");
		return NULL;
	}

	switch (op) {
	case '*':	*flagp = WORST|SPSTART;			break;
	case '+':	*flagp = WORST|SPSTART|HASWIDTH;	break;
	case '?':	*flagp = WORST;				break;
	}

	if (op == '*' && (flags&SIMPLE))
		reginsert(STAR, ret);
	else if (op == '*') {
		// Emit x* as (x&|), where & means "self". 
		reginsert(BRANCH, ret);		// Either x 
		regoptail(ret, regnode(BACK));	// and loop 
		regoptail(ret, ret);		// back 
		regtail(ret, regnode(BRANCH));	// or 
		regtail(ret, regnode(NOTHING));	// null. 
	} else if (op == '+' && (flags&SIMPLE))
		reginsert(PLUS, ret);
	else if (op == '+') {
		// Emit x+ as x(&|), where & means "self". 
		next = regnode(BRANCH);		// Either 
		regtail(ret, next);
		regtail(regnode(BACK), ret);	// loop back 
		regtail(next, regnode(BRANCH));	// or 
		regtail(ret, regnode(NOTHING));	// null. 
	} else if (op == '?') {
		// Emit x? as (x|) 
		reginsert(BRANCH, ret);		// Either x 
		regtail(ret, regnode(BRANCH));	// or 
		next = regnode(NOTHING);		// null. 
		regtail(ret, next);
		regoptail(ret, next);
	}
	regparse++;
	if (ISREPN(*regparse))
	{
		printf("nested *?+\n");
		return NULL;
	}

	return(ret);
}

//
// regatom - the lowest level
//
// Optimization:  gobbles an entire sequence of ordinary characters so that
// it can turn them into a single node, which is smaller to store and
// faster to run.  Backslashed characters are exceptions, each becoming a
// separate node; the code is simpler that way and it's not worth fixing.
 
char *CRegExp::regatom(int *flagp)
{
	char *ret;
	int flags;

	*flagp = WORST;		// Tentatively. 

	switch (*regparse++) {
	case '^':
		ret = regnode(BOL);
		break;
	case '$':
		ret = regnode(EOL);
		break;
	case '.':
		ret = regnode(ANY);
		*flagp |= HASWIDTH|SIMPLE;
		break;
	case '[': {
		int range;
		int rangeend;
		int c;

		if (*regparse == '^') {	// Complement of range. 
			ret = regnode(ANYBUT);
			regparse++;
		} else
			ret = regnode(ANYOF);
		if ((c = *regparse) == ']' || c == '-') {
			regc((char)c);
			regparse++;
		}
		while ((c = *regparse++) != '\0' && ( c != ']' || *(regparse-2) == '\\') ) {
			if (c != '-' || *(regparse-2) == '\\')
				regc((char)c);
			else if ((c = *regparse) == ']' || c == '\0')
				regc('-');
			else 
			{
				range = (unsigned) (char)*(regparse-2);
				rangeend = (unsigned) (char)c;
				if (range > rangeend)
				{
					printf("invalid [] range\n");
					return NULL;
				}
				for (range++; range <= rangeend; range++)
					regc((char)range);
				regparse++;
			}
		}
		regc('\0');
		if (c != ']')
		{
			printf("unmatched []\n");
			return NULL;
		}
		*flagp |= HASWIDTH|SIMPLE;
		break;
		}
	case '(':
		ret = reg(1, &flags);
		if (ret == NULL)
			return(NULL);
		*flagp |= flags&(HASWIDTH|SPSTART);
		break;
	case '\0':
	case '|':
	case ')':
		// supposed to be caught earlier 
		printf("internal error: \\0|) unexpected\n");
		return NULL;
		break;
	case '?':
	case '+':
	case '*':
		printf("?+* follows nothing\n");
		return NULL;
		break;
	case '\\':
		if (*regparse == '\0')
		{
			printf("trailing \\\n");
			return NULL;
		}
		ret = regnode(EXACTLY);
		regc(*regparse++);
		regc('\0');
		*flagp |= HASWIDTH|SIMPLE;
		break;
	default: {
		size_t len;
		char ender;

		regparse--;
		len = strcspn(regparse, META);
		if (len == 0)
		{
			printf("internal error: strcspn 0\n");
			return NULL;
		}
		ender = *(regparse+len);
		if (len > 1 && ISREPN(ender))
			len--;		// Back off clear of ?+* operand. 
		*flagp |= HASWIDTH;
		if (len == 1)
			*flagp |= SIMPLE;
		ret = regnode(EXACTLY);
		for (; len > 0; len--)
			regc(*regparse++);
		regc('\0');
		break;
		}
	}

	return(ret);
}



// reginsert - insert an operator in front of already-emitted operand
//
// Means relocating the operand.
// Adopted UNICODE fixes by Russell Moss
 
void CRegExp::reginsert(char op, char *opnd)
{
	char *place;

	if (!bEmitCode) {
		regsize += ( sizeof(short) + 2 - sizeof(char) ) ;
		return;
	}

	(void) memmove(opnd+( sizeof(short) + 2 - sizeof(char) ), opnd, (size_t)((regcode - opnd)*sizeof(char)));
	regcode += ( sizeof(short) + 2 - sizeof(char) ) ;

	place = opnd;		// Op node, where operand used to be. 
	*place++ = op;
	*place++ = '\0';
#ifndef _UNICODE
	*place++ = '\0';
#endif // _UNICODE
}

//
// regtail - set the next-pointer at the end of a node chain
 
void CRegExp::regtail(char *p, char *val)
{
	char *scan;
	char *temp;
//	int offset;

	if (!bEmitCode)
		return;

	// Find last node. 
	for (scan = p; (temp = regnext(scan)) != NULL; scan = temp)
		continue;

	*((short *)(scan+1)) = (short)((OP(scan) == BACK) ? scan - val : val - scan);
}


// regoptail - regtail on operand of first argument; nop if operandless
 
void CRegExp::regoptail(char *p, char *val)
{
	// "Operandless" and "op != BRANCH" are synonymous in practice. 
	if (!bEmitCode || OP(p) != BRANCH)
		return;
	regtail(OPERAND(p), val);
}


// RegFind	- match a regexp against a string
// Returns	- Returns position of regexp or -1
//			  if regular expression not found
// Note		- The regular expression should have been
//			  previously compiled using RegComp
int CRegExp::RegFind(const char *str)
{
	char *string = (char *)str;	// avert const poisoning 
	char *s;

	// Delete any previously stored found string
	if (sFoundText)
    free(sFoundText);
	sFoundText = NULL;

	// Be paranoid. 
	if(string == NULL) 
	{
		printf("NULL argument to regexec\n");
		return(-1);
	}

	// Check validity of regex
	if (!bCompiled) 
	{
		printf("No regular expression provided yet.\n");
		return(-1);
	}

	// If there is a "must appear" string, look for it. 
	if (regmust != NULL && strstr(string, regmust) == NULL)
		return(-1);

	// Mark beginning of line for ^
	regbol = string;

	// Simplest case:  anchored match need be tried only once. 
	if (reganch)
	{
		if( regtry(string) )
		{
			// Save the found substring in case we need it
			sFoundText = (char *)malloc((GetFindLen()+1)*sizeof(char));
			sFoundText[GetFindLen()] = '\0';
			strncpy(sFoundText, string, GetFindLen() );

			return 0;
		}
		//String not found
		return -1;
	}

	// Messy cases:  unanchored match. 
	if (regstart != '\0') 
	{
		// We know what char it must start with. 
		for (s = string; s != NULL && *s != '\0'; s = strchr(s+1, regstart)) // fixed by JM to include the *s != '\0' 
			if (regtry(s))
			{
				int nPos = s-str;

				// Save the found substring in case we need it later
			  sFoundText = (char *)malloc((GetFindLen()+1)*sizeof(char));
				sFoundText[GetFindLen()] = '\0';
				strncpy(sFoundText, s, GetFindLen() );

				return nPos;
			}
		return -1;
	} 
	else 
	{
		// We don't -- general case
		for (s = string; !regtry(s); s++)
			if (*s == '\0')
				return(-1);

		int nPos = s-str;

		// Save the found substring in case we need it later
		sFoundText = (char *)malloc((GetFindLen()+1)*sizeof(char));
		sFoundText[GetFindLen()] = '\0';
		strncpy(sFoundText, s, GetFindLen() );

		return nPos;
	}
	// NOTREACHED 
}


// regtry - try match at specific point
 
int	CRegExp::regtry(char *string)
{
	int i;
	char **stp;
	char **enp;

	reginput = string;

	stp = startp;
	enp = endp;
	for (i = NSUBEXP; i > 0; i--) 
	{
		*stp++ = NULL;
		*enp++ = NULL;
	}
	if (regmatch(program)) 
	{
		startp[0] = string;
		endp[0] = reginput;
		return(1);
	} 
	else
		return(0);
}

// regmatch - main matching routine
//
// Conceptually the strategy is simple:  check to see whether the current
// node matches, call self recursively to see whether the rest matches,
// and then act accordingly.  In practice we make some effort to avoid
// recursion, in particular by going through "ordinary" nodes (that don't
// need to know whether the rest of the match failed) by a loop instead of
// by recursion.
 
int	CRegExp::regmatch(char *prog)
{
	char *scan;	// Current node. 
	char *next;		// Next node. 

	for (scan = prog; scan != NULL; scan = next) {
		next = regnext(scan);

		switch (OP(scan)) {
		case BOL:
			if (reginput != regbol)
				return(0);
			break;
		case EOL:
			if (*reginput != '\0')
				return(0);
			break;
		case ANY:
			if (*reginput == '\0')
				return(0);
			reginput++;
			break;
		case EXACTLY: {
			size_t len;
			char *const opnd = OPERAND(scan);

			// Inline the first character, for speed. 
			if (*opnd != *reginput)
				return(0);
			len = strlen(opnd);
			if (len > 1 && strncmp(opnd, reginput, len) != 0)
				return(0);
			reginput += len;
			break;
			}
		case ANYOF:
			if (*reginput == '\0' ||
					strchr(OPERAND(scan), *reginput) == NULL)
				return(0);
			reginput++;
			break;
		case ANYBUT:
			if (*reginput == '\0' ||
					strchr(OPERAND(scan), *reginput) != NULL)
				return(0);
			reginput++;
			break;
		case NOTHING:
			break;
		case BACK:
			break;
		case OPEN+1: case OPEN+2: case OPEN+3:
		case OPEN+4: case OPEN+5: case OPEN+6:
		case OPEN+7: case OPEN+8: case OPEN+9: {
			const int no = OP(scan) - OPEN;
			char *const input = reginput;

			if (regmatch(next)) {
				// Don't set startp if some later
				// invocation of the same parentheses
				// already has.
				 
				if (startp[no] == NULL)
					startp[no] = input;
				return(1);
			} else
				return(0);
			break;
			}
		case CLOSE+1: case CLOSE+2: case CLOSE+3:
		case CLOSE+4: case CLOSE+5: case CLOSE+6:
		case CLOSE+7: case CLOSE+8: case CLOSE+9: {
			const int no = OP(scan) - CLOSE;
			char *const input = reginput;

			if (regmatch(next)) {
				// Don't set endp if some later
				// invocation of the same parentheses
				// already has.
				 
				if (endp[no] == NULL)
					endp[no] = input;
				return(1);
			} else
				return(0);
			break;
			}
		case BRANCH: {
			char *const save = reginput;

			if (OP(next) != BRANCH)		// No choice. 
				next = OPERAND(scan);	// Avoid recursion. 
			else {
				while (OP(scan) == BRANCH) {
					if (regmatch(OPERAND(scan)))
						return(1);
					reginput = save;
					scan = regnext(scan);
				}
				return(0);
				// NOTREACHED 
			}
			break;
			}
		case STAR: 
		case PLUS: {
			const char nextch =
				(OP(next) == EXACTLY) ? *OPERAND(next) : '\0';
			size_t no;
			char *const save = reginput;
			const size_t min = (OP(scan) == STAR) ? 0 : 1;

			for (no = regrepeat(OPERAND(scan)) + 1; no > min; no--) {
				reginput = save + no - 1;
				// If it could work, try it. 
				if (nextch == '\0' || *reginput == nextch)
					if (regmatch(next))
						return(1);
			}
			return(0);
			break;
			}
		case END:
			return(1);	// Success! 
			break;
		default:
			printf("regexp corruption\n");
			return(0);
			break;
		}
	}

	// We get here only if there's trouble -- normally "case END" is
	// the terminating point.
	 
	printf("corrupted pointers\n");
	return(0);
}


// regrepeat - report how many times something simple would match
 
size_t CRegExp::regrepeat(char *node)
{
	size_t count;
	char *scan;
	char ch;

	switch (OP(node)) 
	{
	case ANY:
		return(strlen(reginput));
		break;
	case EXACTLY:
		ch = *OPERAND(node);
		count = 0;
		for (scan = reginput; *scan == ch; scan++)
			count++;
		return(count);
		break;
	case ANYOF:
		return(strspn(reginput, OPERAND(node)));
		break;
	case ANYBUT:
		return(strcspn(reginput, OPERAND(node)));
		break;
	default:		// Oh dear.  Called inappropriately. 
		printf("internal error: bad call of regrepeat\n");
		return(0);	// Best compromise. 
		break;
	}
	// NOTREACHED 
}

// regnext - dig the "next" pointer out of a node
 
char *CRegExp::regnext(char *p)
{
	const short &offset = *((short*)(p+1));

	if (offset == 0)
		return(NULL);

	return((OP(p) == BACK) ? p-offset : p+offset);
}


// GetValueCount - Get the number of subvalues available
int CRegExp::GetSubCount()
{
  int i=0;
  for(int no=1;no<NSUBEXP;no++)
  {
    if( startp[no] != NULL && endp[no] != NULL )
      i++;
    else
      break;
  }
  return i;
}

int CRegExp::GetSubStart(int iSub)
{
  if( startp[iSub] != NULL )
    return startp[iSub]-startp[0];
  else
    return 0;
}

int CRegExp::GetSubLenght(int iSub)
{
  if( startp[iSub] != NULL && endp[iSub] != NULL )
    return endp[iSub]-startp[iSub];
  else
    return 0;
}

// GetReplaceString	- Converts a replace expression to a string
// Returns			- Pointer to newly allocated string
//					  Caller is responsible for deleting it
char* CRegExp::GetReplaceString( const char* sReplaceExp )
{
	char *src = (char *)sReplaceExp;
	char *buf;
	char c;
	int no;
	size_t len;

	if( sReplaceExp == NULL || sFoundText == NULL )
		return NULL;


	// First compute the length of the string
	int replacelen = 0;
	while ((c = *src++) != '\0') 
	{
		if (c == '&')
			no = 0;
		else if (c == '\\' && isdigit(*src))
			no = *src++ - '0';
		else
			no = -1;

		if (no < 0) 
		{	
			// Ordinary character. 
			if (c == '\\' && (*src == '\\' || *src == '&'))
				c = *src++;
			replacelen++;
		} 
		else if (startp[no] != NULL && endp[no] != NULL &&
					endp[no] > startp[no]) 
		{
			// Get tagged expression
			len = endp[no] - startp[no];
			replacelen += len;
		}
	}

	// Now allocate buf
	buf = (char *)malloc((replacelen + 1)*sizeof(char));
	if( buf == NULL )
		return NULL;

	char* sReplaceStr = buf;

	// Add null termination
	buf[replacelen] = '\0';
	
	// Now we can create the string
	src = (char *)sReplaceExp;
	while ((c = *src++) != '\0') 
	{
		if (c == '&')
			no = 0;
		else if (c == '\\' && isdigit(*src))
			no = *src++ - '0';
		else
			no = -1;

		if (no < 0) 
		{	
			// Ordinary character. 
			if (c == '\\' && (*src == '\\' || *src == '&'))
				c = *src++;
			*buf++ = c;
		} 
		else if (startp[no] != NULL && endp[no] != NULL &&
					endp[no] > startp[no]) 
		{
			// Get tagged expression
			len = endp[no] - startp[no];
			int tagpos = startp[no] - startp[0];

			strncpy(buf, sFoundText + tagpos, len);
			buf += len;
		}
	}

	return sReplaceStr;
}

//Here's a function that will do global search and replace using regular expressions. Note that the CStringEx class described in the earlier section is being used here. The main reason for using CStringEx is that it provides the Replace() function which makes our task easier. 

/*int RegSearchReplace( CStringEx& string, LPCTSTR sSearchExp, 
					 LPCTSTR sReplaceExp )
{
	int nPos = 0;
	int nReplaced = 0;
	CRegExp r;
	LPTSTR str = (LPTSTR)(LPCTSTR)string;

	r.RegComp( sSearchExp );
	while( (nPos = r.RegFind((LPTSTR)str)) != -1 )
	{
		nReplaced++;
		char *pReplaceStr = r.GetReplaceString( sReplaceExp );

		int offset = str-(LPCTSTR)string+nPos;
		string.Replace( offset, r.GetFindLen(), 
				pReplaceStr );

		// Replace might have caused a reallocation
		str = (LPTSTR)(LPCTSTR)string + offset + strlen(pReplaceStr);
		delete pReplaceStr;
	}
	return nReplaced;
}*/

#endif //HAS_PCRE

