#ifndef BSTR_H
#define BSTR_H

#include <angelscript.h>

#ifdef AS_USE_NAMESPACE
namespace AngelScript
{
#endif

void RegisterBStr(asIScriptEngine *engine);

typedef unsigned char * asBSTR;

asBSTR  asBStrAlloc(asUINT length);
void    asBStrFree(asBSTR str);
asUINT  asBStrLength(asBSTR str);

asBSTR  asBStrConcatenate(const asBSTR *left, const asBSTR *right);

int     asBStrCompare(asBSTR s1, asBSTR s2);

bool    asBStrEqual(const asBSTR *left, const asBSTR *right);
bool    asBStrNotEqual(const asBSTR *left, const asBSTR *right);
bool    asBStrLessThan(const asBSTR *left, const asBSTR *right);
bool    asBStrLessThanOrEqual(const asBSTR *left, const asBSTR *right);
bool    asBStrGreaterThan(const asBSTR *left, const asBSTR *right);
bool    asBStrGreaterThanOrEqual(const asBSTR *left, const asBSTR *right);

asBSTR  asBStrFormat(int number);
asBSTR  asBStrFormat(unsigned int number);
asBSTR  asBStrFormat(float number);
asBSTR  asBStrFormat(double number);
asBSTR  asBStrFormatBits(asDWORD bits);

asBSTR  asBStrSubstr(const asBSTR &str, asUINT start, asUINT count);

#ifdef AS_USE_NAMESPACE
}
#endif

#endif
