#ifndef UTILS_H
#define UTILS_H

#include <angelscript.h>
#include "../../add_on/scriptstring/scriptstring.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <string>
#include <assert.h>

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

#endif

