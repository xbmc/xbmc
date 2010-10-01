/*
   AngelCode Scripting Library
   Copyright (c) 2003-2006 Andreas Jönsson

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product 
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jönsson
   andreas@angelcode.com
*/


//
// as_builder.h
//
// This is the class that manages the compilation of the scripts
//


#ifndef AS_BUILDER_H
#define AS_BUILDER_H

#include "as_config.h"
#include "as_scriptengine.h"
#include "as_module.h"
#include "as_array.h"
#include "as_scriptcode.h"
#include "as_scriptnode.h"
#include "as_datatype.h"
#include "as_property.h"

BEGIN_AS_NAMESPACE

struct sFunctionDescription
{
	asCScriptCode *script;
	asCScriptNode *node;
	asCString name;
};

struct sGlobalVariableDescription
{
	asCScriptCode *script;
	asCScriptNode *node;
	asCString name;
	asCProperty *property;
	asCDataType datatype;
	int index;
	bool isCompiled;
	bool isPureConstant;
	asQWORD constantValue;
};

struct sStructDeclaration
{
	asCScriptCode *script;
	asCScriptNode *node;
	asCString name;
	int validState;
	asCObjectType *objType;
};

class asCCompiler;

class asCBuilder
{
public:
	asCBuilder(asCScriptEngine *engine, asCModule *module);
	~asCBuilder();

	void SetOutputStream(asIOutputStream *out);

	int VerifyProperty(asCDataType *dt, const char *decl, asCString &outName, asCDataType &outType);

	int ParseDataType(const char *datatype, asCDataType *result);

	int ParseFunctionDeclaration(const char *decl, asCScriptFunction *func, asCArray<bool> *paramAutoHandles = 0, bool *returnAutoHandle = 0);
	int ParseVariableDeclaration(const char *decl, asCProperty *var);

	int AddCode(const char *name, const char *code, int codeLength, int lineOffset, int sectionIdx, bool makeCopy);
	int Build();

	int BuildString(const char *string, asCContext *ctx);

	void WriteInfo(const char *scriptname, const char *msg, int r, int c, bool preMessage);
	void WriteError(const char *scriptname, const char *msg, int r, int c);
	void WriteWarning(const char *scriptname, const char *msg, int r, int c);

	int CheckNameConflict(const char *name, asCScriptNode *node, asCScriptCode *code);
	int CheckNameConflictMember(asCDataType &dt, const char *name, asCScriptNode *node, asCScriptCode *code);

protected:
	friend class asCCompiler;
	friend class asCModule;

	const asCString &GetConstantString(int strID);

	asCProperty *GetObjectProperty(asCDataType &obj, const char *prop);
	asCProperty *GetGlobalProperty(const char *prop, bool *isCompiled, bool *isPureConstant, asQWORD *constantValue);

	asCScriptFunction *GetFunctionDescription(int funcID);
	void GetFunctionDescriptions(const char *name, asCArray<int> &funcs);
	void GetObjectMethodDescriptions(const char *name, asCObjectType *objectType, asCArray<int> &methods, bool objIsConst);

	int RegisterScriptFunction(int funcID, asCScriptNode *node, asCScriptCode *file);
	int RegisterImportedFunction(int funcID, asCScriptNode *node, asCScriptCode *file);
	int RegisterGlobalVar(asCScriptNode *node, asCScriptCode *file);
	int RegisterStruct(asCScriptNode *node, asCScriptCode *file);
	void CompileStructs();

	asCObjectType *GetObjectType(const char *type);

	void ParseScripts();
	void CompileFunctions();
	void CompileGlobalVariables();

	asIOutputStream *out;
	asCString preMessage;

	int numErrors;
	int numWarnings;

	asCArray<asCScriptCode *> scripts;
	asCArray<sFunctionDescription *> functions;
	asCArray<sGlobalVariableDescription *> globVariables;
	asCArray<sStructDeclaration *> structDeclarations;

	asCScriptEngine *engine;
	asCModule *module;

	asCDataType CreateDataTypeFromNode(asCScriptNode *node, asCScriptCode *file);
	asCDataType ModifyDataTypeFromNode(const asCDataType &type, asCScriptNode *node, int *inOutFlag, bool *autoHandle);
};

END_AS_NAMESPACE

#endif
