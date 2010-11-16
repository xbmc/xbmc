//
// Script std::string
//
// This function registers the std::string type with AngelScript to be used as the default string type.
//
// The string type is registered as a value type, thus may have performance issues if a lot of 
// string operations are performed in the script. However, for relatively few operations, this should
// not cause any problem for most applications.
//

#ifndef SCRIPTSTDSTRING_H
#define SCRIPTSTDSTRING_H

#include <angelscript.h>
#include <string>

BEGIN_AS_NAMESPACE

void RegisterStdString(asIScriptEngine *engine);

END_AS_NAMESPACE

#endif
