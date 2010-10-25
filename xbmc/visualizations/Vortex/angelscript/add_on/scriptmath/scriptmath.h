#ifndef SCRIPTMATH_H
#define SCRIPTMATH_H

#include <angelscript.h>

BEGIN_AS_NAMESPACE

// This function will determine the configuration of the engine
// and use one of the two functions below to register the math functions
void RegisterScriptMath(asIScriptEngine *engine);

// Call this function to register the math functions
// using native calling conventions
void RegisterScriptMath_Native(asIScriptEngine *engine);

// Use this one instead if native calling conventions
// are not supported on the target platform
void RegisterScriptMath_Generic(asIScriptEngine *engine);

END_AS_NAMESPACE

#endif
