/*
   AngelCode Scripting Library
   Copyright (c) 2003-2009 Andreas Jonsson

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

   Andreas Jonsson
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
	asCObjectType *objType;
	int funcId;
};

struct sGlobalVariableDescription
{
	asCScriptCode *script;
	asCScriptNode *idNode;
	asCScriptNode *nextNode;
	asCString name;
	asCGlobalProperty *property;
	asCDataType datatype;
	int index;
	bool isCompiled;
	bool isPureConstant;
	bool isEnumValue;
	asQWORD constantValue;
};

struct sClassDeclaration
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

	int VerifyProperty(asCDataType *dt, const char *decl, asCString &outName, asCDataType &outType);

	int ParseDataType(const char *datatype, asCDataType *result);
	int ParseTemplateDecl(const char *decl, asCString *name, asCString *subtypeName);

	int ParseFunctionDeclaration(asCObjectType *type, const char *decl, asCScriptFunction *func, bool isSystemFunction, asCArray<bool> *paramAutoHandles = 0, bool *returnAutoHandle = 0);
	int ParseVariableDeclaration(const char *decl, asCObjectProperty *var);

	int AddCode(const char *name, const char *code, int codeLength, int lineOffset, int sectionIdx, bool makeCopy);
	int Build();

#ifdef AS_DEPRECATED
// Deprecated since 2009-12-08, 2.18.0
	int BuildString(const char *string, asCContext *ctx);
#endif
	int CompileFunction(const char *sectionName, const char *code, int lineOffset, asDWORD compileFlags, asCScriptFunction **outFunc);
	int CompileGlobalVar(const char *sectionName, const char *code, int lineOffset);

	void WriteInfo(const char *scriptname, const char *msg, int r, int c, bool preMessage);
	void WriteError(const char *scriptname, const char *msg, int r, int c);
	void WriteWarning(const char *scriptname, const char *msg, int r, int c);

	int CheckNameConflict(const char *name, asCScriptNode *node, asCScriptCode *code);
	int CheckNameConflictMember(asCDataType &dt, const char *name, asCScriptNode *node, asCScriptCode *code);

protected:
	friend class asCCompiler;
	friend class asCModule;
	friend class asCParser;

	asCObjectProperty *GetObjectProperty(asCDataType &obj, const char *prop);
	asCGlobalProperty *GetGlobalProperty(const char *prop, bool *isCompiled, bool *isPureConstant, asQWORD *constantValue);

	asCScriptFunction *GetFunctionDescription(int funcID);
	void GetFunctionDescriptions(const char *name, asCArray<int> &funcs);
	void GetObjectMethodDescriptions(const char *name, asCObjectType *objectType, asCArray<int> &methods, bool objIsConst, const asCString &scope = "");

	int RegisterScriptFunction(int funcID, asCScriptNode *node, asCScriptCode *file, asCObjectType *object = 0, bool isInterface = false, bool isGlobalFunction = false);
	int RegisterImportedFunction(int funcID, asCScriptNode *node, asCScriptCode *file);
	int RegisterGlobalVar(asCScriptNode *node, asCScriptCode *file);
	int RegisterClass(asCScriptNode *node, asCScriptCode *file);
	int RegisterInterface(asCScriptNode *node, asCScriptCode *file);
	int RegisterEnum(asCScriptNode *node, asCScriptCode *file);
	int RegisterTypedef(asCScriptNode *node, asCScriptCode *file);
	void CompileClasses();

	void GetParsedFunctionDetails(asCScriptNode *node, asCScriptCode *file, asCObjectType *objType, asCString &name, asCDataType &returnType, asCArray<asCDataType> &parameterTypes, asCArray<asETypeModifiers> &inOutFlags, bool &isConstMethod, bool &isConstructor, bool &isDestructor);

	bool DoesMethodExist(asCObjectType *objType, int methodId);

	void AddDefaultConstructor(asCObjectType *objType, asCScriptCode *file);
	asCObjectProperty *AddPropertyToClass(sClassDeclaration *c, const asCString &name, const asCDataType &type, asCScriptCode *file = 0, asCScriptNode *node = 0);
	
	int CreateVirtualFunction(asCScriptFunction *func, int idx);

	asCObjectType *GetObjectType(const char *type);

	int GetEnumValueFromObjectType(asCObjectType *objType, const char *name, asCDataType &outDt, asDWORD &outValue);
	int GetEnumValue(const char *name, asCDataType &outDt, asDWORD &outValue);

	void ParseScripts();
	void CompileFunctions();
	void CompileGlobalVariables();

	struct preMessage_t
	{
		bool isSet;
		asCString message;
		int r;
		int c;
	} preMessage;

	int numErrors;
	int numWarnings;

	asCArray<asCScriptCode *>              scripts;
	asCArray<sFunctionDescription *>       functions;
	asCArray<sGlobalVariableDescription *> globVariables;
	asCArray<sClassDeclaration *>          classDeclarations;
	asCArray<sClassDeclaration *>          interfaceDeclarations;
	asCArray<sClassDeclaration *>          namedTypeDeclarations;

	asCScriptEngine *engine;
	asCModule *module;

	asCDataType CreateDataTypeFromNode(asCScriptNode *node, asCScriptCode *file, bool acceptHandleForScope = false, asCObjectType *templateType = 0);
	asCDataType ModifyDataTypeFromNode(const asCDataType &type, asCScriptNode *node, asCScriptCode *file, asETypeModifiers *inOutFlag, bool *autoHandle);
};

END_AS_NAMESPACE

#endif
