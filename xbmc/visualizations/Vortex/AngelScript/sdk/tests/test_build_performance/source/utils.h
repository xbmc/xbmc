#ifndef UTILS_H
#define UTILS_H

#include "angelscript.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>

double GetSystemTimer();

class COutStream : public asIOutputStream
{
public:
	void Write(const char *text) { printf(text); }
};


#endif

