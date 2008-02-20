 ////////////////////////////////////////////////////////////////////////
// RegExp.h
//
// Changes by JM for XBMC:
//  replaced all TCHAR with char
//  replaced all new/delete's with malloc/free's.
//
// This code has been derived from work by Henry Spencer. 
// The main changes are
// 1. All char variables and functions have been changed to char
//    counterparts
// 2. Added GetFindLen() & GetReplaceString() to enable search
//    and replace operations.
// 3. And of course, added the C++ Wrapper
//
// Comments from Russell Moss who posted UNICODE fixes to codeguru, herein adopted:
// Both CRegExp::regnode and CRegExp::reginsert were leaving 
// 2 "locations" for a short.  In the case of UNICODE, a 
// char is a short so leaving room for a short is equivalent 
// to skipping only one char
//
//
// The original copyright notice follows:
//
// Copyright (c) 1986, 1993, 1995 by University of Toronto.
// Written by Henry Spencer.  Not derived from licensed software.
//
// Permission is granted to anyone to use this software for any
// purpose on any computer system, and to redistribute it in any way,
// subject to the following restrictions:
//
// 1. The author is not responsible for the consequences of use of
// this software, no matter how awful, even if they arise
// from defects in it.
//
// 2. The origin of this software must not be misrepresented, either
// by explicit claim or by omission.
//
// 3. Altered versions must be plainly marked as such, and must not
// be misrepresented (by explicit claim or omission) as being
// the original software.
//
// 4. This notice must not be removed or altered.
/////////////////////////////////////////////////////////////////////////////
#pragma once

#ifndef REGEXP_H
#define REGEXP_H

#define NSUBEXP  10

#ifdef _XBOX
#include <xtl.h>
#endif
#include <stdio.h>

#define FALSE 0
#define TRUE 1
#ifndef NULL
#define NULL 0
#endif
typedef char* LPTSTR;

#ifdef HAS_PCRE

#include <string>
namespace PCRE {
#include <pcre.h>
}

// maximum of 20 backreferences
// OVEVCOUNT must be a multiple of 3
const int OVECCOUNT=(20+1)*3;

class CRegExp
{
public:
  CRegExp();
  ~CRegExp();
  
  CRegExp *RegComp( const char *re );
  int RegFind(const char *str);
  char* GetReplaceString( const char* sReplaceExp );
  int GetFindLen()
  {
    if (!m_re || !m_bMatched)
      return 0;
    
    return (m_iOvector[1] - m_iOvector[0]);
  };
  int GetSubCount() { return m_iMatchCount; }
  int GetSubStart(int iSub) { return m_iOvector[iSub*2]; }
  int GetSubLenght(int iSub) { return (m_iOvector[iSub*2+1] - m_iOvector[iSub*2+1]); }

private:
  void Cleanup() { if (m_re) { PCRE::pcre_free(m_re); m_re = NULL; } }

private:
  PCRE::pcre* m_re;
  int         m_iOvector[OVECCOUNT];
  int         m_iMatchCount;
  int         m_iOptions;
  bool        m_bMatched;
  std::string m_subject;
};

#else // HAS_PCRE

class CRegExp
{
public:
	CRegExp();
	~CRegExp();

	CRegExp *RegComp( const char *re );
	int RegFind(const char *str);
	char* GetReplaceString( const char* sReplaceExp );
	int GetFindLen()
	{
		if( startp[0] == NULL || endp[0] == NULL )
			return 0;

		return (int)(endp[0] - startp[0]);
	};
  int GetSubCount();
  int GetSubStart(int iSub);
  int GetSubLenght(int iSub);

private:
	char *regnext(char *node);
	void reginsert(char op, char *opnd);

	int regtry(char *string);
	int regmatch(char *prog);
	size_t regrepeat(char *node);
	char *reg(int paren, int *flagp);
	char *regbranch(int *flagp);
	void regtail(char *p, char *val);
	void regoptail(char *p, char *val);
	char *regpiece(int *flagp);
	char *regatom(int *flagp);

	// Inline functions
private:
	char OP(char *p) {return *p;};
	char *OPERAND( char *p) {return (char*)((short *)(p+1)+1); };

	// regc - emit (if appropriate) a byte of code
	void regc(char b)
	{
		if (bEmitCode)
			*regcode++ = b;
		else
			regsize++;
	};

	// regnode - emit a node
	char *	regnode(char op)
	{
		if (!bEmitCode) {
			regsize += ( sizeof(short) + 2 - sizeof(char) ) ;
			return regcode;
		}

		*regcode++ = op;
		*regcode++ = '\0';		/* Null next pointer. */
#ifndef _UNICODE
		*regcode++ = '\0';
#endif // _UNICODE

		return regcode - (sizeof(short) + 2 - sizeof(char)) ;
	};


private:
	bool bEmitCode;
	bool bCompiled;
	char *sFoundText;

	char *startp[NSUBEXP];
	char *endp[NSUBEXP];
	char regstart;		// Internal use only. 
	char reganch;		// Internal use only. 
	char *regmust;		// Internal use only. 
	int regmlen;		// Internal use only. 
	char *program;		// Unwarranted chumminess with compiler. 

	char *regparse;	// Input-scan pointer. 
	int regnpar;		// () count. 
	char *regcode;		// Code-emit pointer; ®dummy = don't. 
	char regdummy[3];	// NOTHING, 0 next ptr 
	long regsize;		// Code size. 

	char *reginput;	// String-input pointer. 
	char *regbol;		// Beginning of input, for ^ check. 
};

#endif // HAS_PCRE

#endif

