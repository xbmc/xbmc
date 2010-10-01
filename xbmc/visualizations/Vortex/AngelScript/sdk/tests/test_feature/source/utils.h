#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <string>
#include <assert.h>

#include <angelscript.h>

#include "../../../add_on/scriptstring/scriptstring.h"

#ifdef AS_USE_NAMESPACE
using namespace AngelScript;
#endif

class COutStream : public asIOutputStream
{
public:
	void Write(const char *text) { printf(text); }
};

class CBufferedOutStream : public asIOutputStream
{
public:
	void Write(const char *text) { buffer += text; }

	std::string buffer;
};


void PrintException(asIScriptContext *ctx);
void Assert(bool expr);

#endif

