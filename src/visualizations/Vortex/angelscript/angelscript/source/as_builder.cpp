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
// as_builder.cpp
//
// This is the class that manages the compilation of the scripts
//


#include "as_config.h"
#include "as_builder.h"
#include "as_parser.h"
#include "as_compiler.h"
#include "as_tokendef.h"
#include "as_string_util.h"
#include "as_outputbuffer.h"
#include "as_texts.h"
#include "as_scriptobject.h"

BEGIN_AS_NAMESPACE

asCBuilder::asCBuilder(asCScriptEngine *engine, asCModule *module)
{
	this->engine = engine;
	this->module = module;
}

asCBuilder::~asCBuilder()
{
	asUINT n;

	// Free all functions
	for( n = 0; n < functions.GetLength(); n++ )
	{
		if( functions[n] )
		{
			if( functions[n]->node )
			{
				functions[n]->node->Destroy(engine);
			}

			asDELETE(functions[n],sFunctionDescription);
		}

		functions[n] = 0;
	}

	// Free all global variables
	for( n = 0; n < globVariables.GetLength(); n++ )
	{
		if( globVariables[n] )
		{
			if( globVariables[n]->nextNode )
			{
				globVariables[n]->nextNode->Destroy(engine);
			}

			asDELETE(globVariables[n],sGlobalVariableDescription);
			globVariables[n] = 0;
		}
	}

	// Free all the loaded files
	for( n = 0; n < scripts.GetLength(); n++ )
	{
		if( scripts[n] )
		{
			asDELETE(scripts[n],asCScriptCode);
		}

		scripts[n] = 0;
	}

	// Free all class declarations
	for( n = 0; n < classDeclarations.GetLength(); n++ )
	{
		if( classDeclarations[n] )
		{
			if( classDeclarations[n]->node )
			{
				classDeclarations[n]->node->Destroy(engine);
			}

			asDELETE(classDeclarations[n],sClassDeclaration);
			classDeclarations[n] = 0;
		}
	}

	for( n = 0; n < interfaceDeclarations.GetLength(); n++ )
	{
		if( interfaceDeclarations[n] )
		{
			if( interfaceDeclarations[n]->node )
			{
				interfaceDeclarations[n]->node->Destroy(engine);
			}

			asDELETE(interfaceDeclarations[n],sClassDeclaration);
			interfaceDeclarations[n] = 0;
		}
	}

	for( n = 0; n < namedTypeDeclarations.GetLength(); n++ )
	{
		if( namedTypeDeclarations[n] )
		{
			if( namedTypeDeclarations[n]->node )
			{
				namedTypeDeclarations[n]->node->Destroy(engine);
			}

			asDELETE(namedTypeDeclarations[n],sClassDeclaration);
			namedTypeDeclarations[n] = 0;
		}
	}
}

int asCBuilder::AddCode(const char *name, const char *code, int codeLength, int lineOffset, int sectionIdx, bool makeCopy)
{
	asCScriptCode *script = asNEW(asCScriptCode);
	script->SetCode(name, code, codeLength, makeCopy);
	script->lineOffset = lineOffset;
	script->idx = sectionIdx;
	scripts.PushLast(script);

	return 0;
}

int asCBuilder::Build()
{
	numErrors = 0;
	numWarnings = 0;
	preMessage.isSet = false;

	ParseScripts();
	CompileClasses();
	CompileGlobalVariables();
	CompileFunctions();

	if( numErrors > 0 )
		return asERROR;

	return asSUCCESS;
}

#ifdef AS_DEPRECATED
// Deprecated since 2009-12-08, 2.18.0
int asCBuilder::BuildString(const char *string, asCContext *ctx)
{
	asCScriptFunction *execFunc = 0;
	int r = CompileFunction(TXT_EXECUTESTRING, string, -1, 0, &execFunc);
	if( r >= 0 )
	{
		ctx->SetExecuteStringFunction(execFunc);
	}

	return r;
}
#endif

int asCBuilder::CompileGlobalVar(const char *sectionName, const char *code, int lineOffset)
{
	numErrors = 0;
	numWarnings = 0;
	preMessage.isSet = false;

	// Add the string to the script code
	asCScriptCode *script = asNEW(asCScriptCode);
	script->SetCode(sectionName, code, true);
	script->lineOffset = lineOffset;
	scripts.PushLast(script);

	// Parse the string
	asCParser parser(this);
	if( parser.ParseScript(scripts[0]) < 0 )
		return asERROR;

	asCScriptNode *node = parser.GetScriptNode();

	// Make sure there is nothing else than the global variable in the script code
	if( node == 0 ||
		node->firstChild == 0 ||
		node->firstChild != node->lastChild ||
		node->firstChild->nodeType != snGlobalVar )
	{
		WriteError(script->name.AddressOf(), TXT_ONLY_ONE_VARIABLE_ALLOWED, 0, 0);
		return asERROR;
	}

	node = node->firstChild;
	node->DisconnectParent();
	RegisterGlobalVar(node, script);

	CompileGlobalVariables();

	if( numErrors > 0 )
	{
		// Remove the variable from the module, if it was registered
		if( globVariables.GetLength() > 0 )
		{
			module->RemoveGlobalVar(module->GetGlobalVarCount()-1);
		}

		return asERROR;
	}

	return 0;
}

int asCBuilder::CompileFunction(const char *sectionName, const char *code, int lineOffset, asDWORD compileFlags, asCScriptFunction **outFunc)
{
	asASSERT(outFunc != 0);

	numErrors = 0;
	numWarnings = 0;
	preMessage.isSet = false;

	// Add the string to the script code
	asCScriptCode *script = asNEW(asCScriptCode);
	script->SetCode(sectionName, code, true);
	script->lineOffset = lineOffset; 
	scripts.PushLast(script);

	// Parse the string
	asCParser parser(this);
	if( parser.ParseScript(scripts[0]) < 0 )
		return asERROR;

	asCScriptNode *node = parser.GetScriptNode();

	// Make sure there is nothing else than the function in the script code
	if( node == 0 || 
		node->firstChild == 0 || 
		node->firstChild != node->lastChild || 
		node->firstChild->nodeType != snFunction )
	{
		WriteError(script->name.AddressOf(), TXT_ONLY_ONE_FUNCTION_ALLOWED, 0, 0);
		return asERROR;
	}

	// Find the function node
	node = node->firstChild;

	// Create the function
	bool isConstructor, isDestructor;
	asCScriptFunction *func = asNEW(asCScriptFunction)(engine,module,asFUNC_SCRIPT);
	GetParsedFunctionDetails(node, scripts[0], 0, func->name, func->returnType, func->parameterTypes, func->inOutFlags, func->isReadOnly, isConstructor, isDestructor);
	func->id               = engine->GetNextScriptFunctionId();
	func->scriptSectionIdx = engine->GetScriptSectionNameIndex(sectionName ? sectionName : "");

	// Tell the engine that the function exists already so the compiler can access it
	if( compileFlags & asCOMP_ADD_TO_MODULE )
	{
		int r = CheckNameConflict(func->name.AddressOf(), node, scripts[0]);
		if( r < 0 )
		{
			func->Release();
			return asERROR;
		}

		module->globalFunctions.PushLast(func);
		func->AddRef();
		module->AddScriptFunction(func);
	}
	else
		engine->SetScriptFunction(func);

	// Fill in the function info for the builder too
	node->DisconnectParent();
	sFunctionDescription *funcDesc = asNEW(sFunctionDescription);
	functions.PushLast(funcDesc);
	funcDesc->script = scripts[0];
	funcDesc->node   = node;
	funcDesc->name   = func->name;
	funcDesc->funcId = func->id;

	asCCompiler compiler(engine);
	if( compiler.CompileFunction(this, functions[0]->script, functions[0]->node, func) >= 0 )
	{
		// Return the function
		*outFunc = func;
	}
	else
	{
		// If the function was added to the module then remove it again
		if( compileFlags & asCOMP_ADD_TO_MODULE )
		{
			module->globalFunctions.RemoveValue(func);
			module->scriptFunctions.RemoveValue(func);
			func->Release();
			func->Release();
		}

		// Clear the global variables to avoid releasing them
		// TODO: This shouldn't be necessary
		func->globalVarPointers.SetLength(0);
		func->Release();

		return asERROR;
	}

	return asSUCCESS;
}

void asCBuilder::ParseScripts()
{
	asCArray<asCParser*> parsers((int)scripts.GetLength());

	// Parse all the files as if they were one
	asUINT n = 0;
	for( n = 0; n < scripts.GetLength(); n++ )
	{
		asCParser *parser = asNEW(asCParser)(this);
		parsers.PushLast(parser);

		// Parse the script file
		parser->ParseScript(scripts[n]);
	}

	if( numErrors == 0 )
	{
		// Find all type declarations
		for( n = 0; n < scripts.GetLength(); n++ )
		{
			asCScriptNode *node = parsers[n]->GetScriptNode();

			// Find structure definitions first
			node = node->firstChild;
			while( node )
			{
				asCScriptNode *next = node->next;
				if( node->nodeType == snClass )
				{
					node->DisconnectParent();
					RegisterClass(node, scripts[n]);
				}
				else if( node->nodeType == snInterface )
				{
					node->DisconnectParent();
					RegisterInterface(node, scripts[n]);
				}
				//	Handle enumeration
				else if( node->nodeType == snEnum )
				{
					node->DisconnectParent();
					RegisterEnum(node, scripts[n]);
				}
				//	Handle typedef
				else if( node->nodeType == snTypedef )
				{
					node->DisconnectParent();
					RegisterTypedef(node, scripts[n]);
				}

				node = next;
			}
		}

		// Register script methods found in the interfaces
		for( n = 0; n < interfaceDeclarations.GetLength(); n++ )
		{
			sClassDeclaration *decl = interfaceDeclarations[n];

			asCScriptNode *node = decl->node->firstChild->next;
			while( node )
			{
				asCScriptNode *next = node->next;
				if( node->nodeType == snFunction )
				{
					node->DisconnectParent();
					RegisterScriptFunction(engine->GetNextScriptFunctionId(), node, decl->script, decl->objType, true);
				}

				node = next;
			}
		}

		// Now the interfaces have been completely established, now we need to determine if
		// the same interface has already been registered before, and if so reuse the interface id.
		module->ResolveInterfaceIds();

		// Register script methods found in the structures
		for( n = 0; n < classDeclarations.GetLength(); n++ )
		{
			sClassDeclaration *decl = classDeclarations[n];

			asCScriptNode *node = decl->node->firstChild->next;

			// Skip list of classes and interfaces
			while( node && node->nodeType == snIdentifier )
				node = node->next;

			while( node )
			{
				asCScriptNode *next = node->next;
				if( node->nodeType == snFunction )
				{
					node->DisconnectParent();
					RegisterScriptFunction(engine->GetNextScriptFunctionId(), node, decl->script, decl->objType);
				}

				node = next;
			}

			// Make sure the default factory & constructor exists for classes
			if( decl->objType->beh.construct == engine->scriptTypeBehaviours.beh.construct )
			{
				AddDefaultConstructor(decl->objType, decl->script);
			}
		}

		// Find other global nodes
		for( n = 0; n < scripts.GetLength(); n++ )
		{
			// Find other global nodes
			asCScriptNode *node = parsers[n]->GetScriptNode();
			node = node->firstChild;
			while( node )
			{
				asCScriptNode *next = node->next;
				node->DisconnectParent();

				if( node->nodeType == snFunction )
				{
					RegisterScriptFunction(engine->GetNextScriptFunctionId(), node, scripts[n], 0, false, true);
				}
				else if( node->nodeType == snGlobalVar )
				{
					RegisterGlobalVar(node, scripts[n]);
				}
				else if( node->nodeType == snImport )
				{
					RegisterImportedFunction(module->GetNextImportedFunctionId(), node, scripts[n]);
				}
				else
				{
					// Unused script node
					int r, c;
					scripts[n]->ConvertPosToRowCol(node->tokenPos, &r, &c);

					WriteWarning(scripts[n]->name.AddressOf(), TXT_UNUSED_SCRIPT_NODE, r, c);

					node->Destroy(engine);
				}

				node = next;
			}
		}
	}

	for( n = 0; n < parsers.GetLength(); n++ )
	{
		asDELETE(parsers[n],asCParser);
	}
}

void asCBuilder::CompileFunctions()
{
	// Compile each function
	for( asUINT n = 0; n < functions.GetLength(); n++ )
	{
		if( functions[n] == 0 ) continue;

		asCCompiler compiler(engine);
		asCScriptFunction *func = engine->scriptFunctions[functions[n]->funcId];

		if( functions[n]->node )
		{
			int r, c;
			functions[n]->script->ConvertPosToRowCol(functions[n]->node->tokenPos, &r, &c);

			asCString str = func->GetDeclarationStr();
			str.Format(TXT_COMPILING_s, str.AddressOf());
			WriteInfo(functions[n]->script->name.AddressOf(), str.AddressOf(), r, c, true);

			compiler.CompileFunction(this, functions[n]->script, functions[n]->node, func);

			preMessage.isSet = false;
		}
		else
		{
			// This is the default constructor, that is generated
			// automatically if not implemented by the user.
			asASSERT( functions[n]->name == functions[n]->objType->name );
			compiler.CompileDefaultConstructor(this, functions[n]->script, func);
		}
	}
}

int asCBuilder::ParseDataType(const char *datatype, asCDataType *result)
{
	numErrors = 0;
	numWarnings = 0;
	preMessage.isSet = false;

	asCScriptCode source;
	source.SetCode("", datatype, true);

	asCParser parser(this);
	int r = parser.ParseDataType(&source);
	if( r < 0 )
		return asINVALID_TYPE;

	// Get data type and property name
	asCScriptNode *dataType = parser.GetScriptNode()->firstChild;

	*result = CreateDataTypeFromNode(dataType, &source, true);

	if( numErrors > 0 )
		return asINVALID_TYPE;

	return asSUCCESS;
}

int asCBuilder::ParseTemplateDecl(const char *decl, asCString *name, asCString *subtypeName)
{
	numErrors = 0;
	numWarnings = 0;
	preMessage.isSet = false;

	asCScriptCode source;
	source.SetCode("", decl, true);

	asCParser parser(this);
	int r = parser.ParseTemplateDecl(&source);
	if( r < 0 )
		return asINVALID_TYPE;

	// Get the template name and subtype name
	asCScriptNode *node = parser.GetScriptNode()->firstChild;

	name->Assign(&decl[node->tokenPos], node->tokenLength);
	node = node->next;
	subtypeName->Assign(&decl[node->tokenPos], node->tokenLength);

	// TODO: template: check for name conflicts

	if( numErrors > 0 )
		return asINVALID_DECLARATION;

	return asSUCCESS;
}

int asCBuilder::VerifyProperty(asCDataType *dt, const char *decl, asCString &name, asCDataType &type)
{
	numErrors = 0;
	numWarnings = 0;
	preMessage.isSet = false;

	if( dt )
	{
		// Verify that the object type exist
		if( dt->GetObjectType() == 0 )
			return asINVALID_OBJECT;
	}

	// Check property declaration and type
	asCScriptCode source;
	source.SetCode(TXT_PROPERTY, decl, true);

	asCParser parser(this);
	int r = parser.ParsePropertyDeclaration(&source);
	if( r < 0 )
		return asINVALID_DECLARATION;

	// Get data type and property name
	asCScriptNode *dataType = parser.GetScriptNode()->firstChild;

	asCScriptNode *nameNode = dataType->next;

	type = CreateDataTypeFromNode(dataType, &source);
	name.Assign(&decl[nameNode->tokenPos], nameNode->tokenLength);

	// Verify property name
	if( dt )
	{
		if( CheckNameConflictMember(*dt, name.AddressOf(), nameNode, &source) < 0 )
			return asNAME_TAKEN;
	}
	else
	{
		if( CheckNameConflict(name.AddressOf(), nameNode, &source) < 0 )
			return asNAME_TAKEN;
	}

	if( numErrors > 0 )
		return asINVALID_DECLARATION;

	return asSUCCESS;
}

asCObjectProperty *asCBuilder::GetObjectProperty(asCDataType &obj, const char *prop)
{
	asASSERT(obj.GetObjectType() != 0);

	// TODO: Only search in config groups to which the module has access
	// TODO: optimize: Improve linear search
	asCArray<asCObjectProperty *> &props = obj.GetObjectType()->properties;
	for( asUINT n = 0; n < props.GetLength(); n++ )
		if( props[n]->name == prop )
			return props[n];

	return 0;
}

asCGlobalProperty *asCBuilder::GetGlobalProperty(const char *prop, bool *isCompiled, bool *isPureConstant, asQWORD *constantValue)
{
	asUINT n;

	if( isCompiled ) *isCompiled = true;
	if( isPureConstant ) *isPureConstant = false;

	// TODO: optimize: Improve linear search
	// Check application registered properties
	asCArray<asCGlobalProperty *> *props = &(engine->registeredGlobalProps);
	for( n = 0; n < props->GetLength(); ++n )
		if( (*props)[n] && (*props)[n]->name == prop )
		{
			if( module )
			{
				// Find the config group for the global property
				asCConfigGroup *group = engine->FindConfigGroupForGlobalVar((*props)[n]->id);
				if( !group || group->HasModuleAccess(module->name.AddressOf()) )
					return (*props)[n];
			}
			else
			{
				// We're not compiling a module right now, so it must be a registered global property
				return (*props)[n];
			}
		}

	// TODO: optimize: Improve linear search
	// Check properties being compiled now
	asCArray<sGlobalVariableDescription *> *gvars = &globVariables;
	for( n = 0; n < gvars->GetLength(); ++n )
	{
		if( (*gvars)[n] && (*gvars)[n]->name == prop )
		{
			if( isCompiled ) *isCompiled = (*gvars)[n]->isCompiled;

			if( isPureConstant ) *isPureConstant = (*gvars)[n]->isPureConstant;
			if( constantValue  ) *constantValue  = (*gvars)[n]->constantValue;

			return (*gvars)[n]->property;
		}
	}

	// TODO: optimize: Improve linear search
	// Check previously compiled global variables
	if( module )
	{
		props = &module->scriptGlobals;
		for( n = 0; n < props->GetLength(); ++n )
			if( (*props)[n]->name == prop )
				return (*props)[n];
	}

	return 0;
}

int asCBuilder::ParseFunctionDeclaration(asCObjectType *objType, const char *decl, asCScriptFunction *func, bool isSystemFunction, asCArray<bool> *paramAutoHandles, bool *returnAutoHandle)
{
	numErrors = 0;
	numWarnings = 0;
	preMessage.isSet = false;

	asCScriptCode source;
	source.SetCode(TXT_SYSTEM_FUNCTION, decl, true);

	asCParser parser(this);
	int r = parser.ParseFunctionDefinition(&source);
	if( r < 0 )
		return asINVALID_DECLARATION;

	asCScriptNode *node = parser.GetScriptNode();

	// Find name
	asCScriptNode *n = node->firstChild->next->next;
	func->name.Assign(&source.code[n->tokenPos], n->tokenLength);

	// Initialize a script function object for registration
	bool autoHandle;

	// Scoped reference types are allowed to use handle when returned from application functions
	func->returnType = CreateDataTypeFromNode(node->firstChild, &source, true, objType);
	func->returnType = ModifyDataTypeFromNode(func->returnType, node->firstChild->next, &source, 0, &autoHandle);
	if( autoHandle && (!func->returnType.IsObjectHandle() || func->returnType.IsReference()) )
			return asINVALID_DECLARATION;
	if( returnAutoHandle ) *returnAutoHandle = autoHandle;

	// Reference types cannot be returned by value from system functions
	if( isSystemFunction &&
		(func->returnType.GetObjectType() &&
		 (func->returnType.GetObjectType()->flags & asOBJ_REF)) &&
        !(func->returnType.IsReference() ||
		  func->returnType.IsObjectHandle()) )
		return asINVALID_DECLARATION;

	// Count number of parameters
	int paramCount = 0;
	n = n->next->firstChild;
	while( n )
	{
		paramCount++;
		n = n->next->next;
		if( n && n->nodeType == snIdentifier )
			n = n->next;
	}

	// Preallocate memory
	func->parameterTypes.Allocate(paramCount, false);
	func->inOutFlags.Allocate(paramCount, false);
	if( paramAutoHandles ) paramAutoHandles->Allocate(paramCount, false);

	n = node->firstChild->next->next->next->firstChild;
	while( n )
	{
		asETypeModifiers inOutFlags;
		asCDataType type = CreateDataTypeFromNode(n, &source, false, objType);
		type = ModifyDataTypeFromNode(type, n->next, &source, &inOutFlags, &autoHandle);

		// Reference types cannot be passed by value to system functions
		if( isSystemFunction &&
			(type.GetObjectType() &&
		     (type.GetObjectType()->flags & asOBJ_REF)) &&
			!(type.IsReference() ||
			  type.IsObjectHandle()) )
			return asINVALID_DECLARATION;

		// Store the parameter type
		func->parameterTypes.PushLast(type);
		func->inOutFlags.PushLast(inOutFlags);

		// Don't permit void parameters
		if( type.GetTokenType() == ttVoid )
			return asINVALID_DECLARATION;

		if( autoHandle && (!type.IsObjectHandle() || type.IsReference()) )
			return asINVALID_DECLARATION;

		if( paramAutoHandles ) paramAutoHandles->PushLast(autoHandle);

		// Make sure that var type parameters are references
		if( type.GetTokenType() == ttQuestion &&
			!type.IsReference() )
			return asINVALID_DECLARATION;

		// Move to next parameter
		n = n->next->next;
		if( n && n->nodeType == snIdentifier )
			n = n->next;
	}

	// Set the read-only flag if const is declared after parameter list
	if( node->lastChild->nodeType == snUndefined && node->lastChild->tokenType == ttConst )
		func->isReadOnly = true;
	else
		func->isReadOnly = false;

	if( numErrors > 0 || numWarnings > 0 )
		return asINVALID_DECLARATION;

	return 0;
}

int asCBuilder::ParseVariableDeclaration(const char *decl, asCObjectProperty *var)
{
	numErrors = 0;
	numWarnings = 0;
	preMessage.isSet = false;

	asCScriptCode source;
	source.SetCode(TXT_VARIABLE_DECL, decl, true);

	asCParser parser(this);

	int r = parser.ParsePropertyDeclaration(&source);
	if( r < 0 )
		return asINVALID_DECLARATION;

	asCScriptNode *node = parser.GetScriptNode();

	// Find name
	asCScriptNode *n = node->firstChild->next;
	var->name.Assign(&source.code[n->tokenPos], n->tokenLength);

	// Initialize a script variable object for registration
	var->type = CreateDataTypeFromNode(node->firstChild, &source);

	if( numErrors > 0 || numWarnings > 0 )
		return asINVALID_DECLARATION;

	return 0;
}

int asCBuilder::CheckNameConflictMember(asCDataType &dt, const char *name, asCScriptNode *node, asCScriptCode *code)
{
	// It's not necessary to check against object types

	// Check against other members
	asCObjectType *t = dt.GetObjectType();

	// TODO: optimize: Improve linear search
	asCArray<asCObjectProperty *> &props = t->properties;
	for( asUINT n = 0; n < props.GetLength(); n++ )
	{
		if( props[n]->name == name )
		{
			if( code )
			{
				int r, c;
				code->ConvertPosToRowCol(node->tokenPos, &r, &c);

				asCString str;
				str.Format(TXT_NAME_CONFLICT_s_OBJ_PROPERTY, name);
				WriteError(code->name.AddressOf(), str.AddressOf(), r, c);
			}

			return -1;
		}
	}

	// TODO: Property names must be checked against method names

	return 0;
}

int asCBuilder::CheckNameConflict(const char *name, asCScriptNode *node, asCScriptCode *code)
{
	// TODO: Must verify object types in all config groups, whether the module has access or not
	// Check against object types
	if( engine->GetObjectType(name) != 0 )
	{
		if( code )
		{
			int r, c;
			code->ConvertPosToRowCol(node->tokenPos, &r, &c);

			asCString str;
			str.Format(TXT_NAME_CONFLICT_s_EXTENDED_TYPE, name);
			WriteError(code->name.AddressOf(), str.AddressOf(), r, c);
		}

		return -1;
	}

	// TODO: Must verify global properties in all config groups, whether the module has access or not
	// Check against global properties
	asCGlobalProperty *prop = GetGlobalProperty(name, 0, 0, 0);
	if( prop )
	{
		if( code )
		{
			int r, c;
			code->ConvertPosToRowCol(node->tokenPos, &r, &c);

			asCString str;
			str.Format(TXT_NAME_CONFLICT_s_GLOBAL_PROPERTY, name);

			WriteError(code->name.AddressOf(), str.AddressOf(), r, c);
		}

		return -1;
	}

	// TODO: Property names must be checked against function names

	// Check against class types
	asUINT n;
	for( n = 0; n < classDeclarations.GetLength(); n++ )
	{
		if( classDeclarations[n]->name == name )
		{
			if( code )
			{
				int r, c;
				code->ConvertPosToRowCol(node->tokenPos, &r, &c);

				asCString str;
				str.Format(TXT_NAME_CONFLICT_s_STRUCT, name);

				WriteError(code->name.AddressOf(), str.AddressOf(), r, c);
			}

			return -1;
		}
	}

	// Check against named types
	for( n = 0; n < namedTypeDeclarations.GetLength(); n++ )
	{
		if( namedTypeDeclarations[n]->name == name )
		{
			if( code )
			{
				int r, c;
				code->ConvertPosToRowCol(node->tokenPos, &r, &c);

				asCString str;

				str.Format(TXT_NAME_CONFLICT_s_IS_NAMED_TYPE, name);

				WriteError(code->name.AddressOf(), str.AddressOf(), r, c);
			}

			return -1;
		}
	}

	return 0;
}


int asCBuilder::RegisterGlobalVar(asCScriptNode *node, asCScriptCode *file)
{
	// What data type is it?
	asCDataType type = CreateDataTypeFromNode(node->firstChild, file);

	if( !type.CanBeInstanciated() )
	{
		asCString str;
		// TODO: Change to "'type' cannot be declared as variable"
		str.Format(TXT_DATA_TYPE_CANT_BE_s, type.Format().AddressOf());

		int r, c;
		file->ConvertPosToRowCol(node->tokenPos, &r, &c);

		WriteError(file->name.AddressOf(), str.AddressOf(), r, c);
	}

	asCScriptNode *n = node->firstChild->next;

	while( n )
	{
		// Verify that the name isn't taken
		asCString name(&file->code[n->tokenPos], n->tokenLength);
		CheckNameConflict(name.AddressOf(), n, file);

		// Register the global variable
		sGlobalVariableDescription *gvar = asNEW(sGlobalVariableDescription);
		globVariables.PushLast(gvar);

		gvar->script      = file;
		gvar->name        = name;
		gvar->isCompiled  = false;
		gvar->datatype    = type;
		gvar->isEnumValue = false;

		// TODO: Give error message if wrong
		asASSERT(!gvar->datatype.IsReference());

		gvar->idNode = n;
		gvar->nextNode = 0;
		if( n->next &&
			(n->next->nodeType == snAssignment ||
			 n->next->nodeType == snArgList    ||
			 n->next->nodeType == snInitList     ) )
		{
			gvar->nextNode = n->next;
			n->next->DisconnectParent();
		}

		gvar->property = module->AllocateGlobalProperty(name.AddressOf(), gvar->datatype);
		gvar->index    = gvar->property->id;

		n = n->next;
	}

	node->Destroy(engine);

	return 0;
}

int asCBuilder::RegisterClass(asCScriptNode *node, asCScriptCode *file)
{
	asCScriptNode *n = node->firstChild;
	asCString name(&file->code[n->tokenPos], n->tokenLength);

	int r, c;
	file->ConvertPosToRowCol(n->tokenPos, &r, &c);

	CheckNameConflict(name.AddressOf(), n, file);

	sClassDeclaration *decl = asNEW(sClassDeclaration);
	classDeclarations.PushLast(decl);
	decl->name       = name;
	decl->script     = file;
	decl->validState = 0;
	decl->node       = node;

	asCObjectType *st = asNEW(asCObjectType)(engine);
	st->flags = asOBJ_REF | asOBJ_SCRIPT_OBJECT;

	if( node->tokenType == ttHandle )
		st->flags |= asOBJ_IMPLICIT_HANDLE;

	st->size      = sizeof(asCScriptObject);
	st->name      = name;
	module->classTypes.PushLast(st);
	engine->classTypes.PushLast(st);
	st->AddRef();
	decl->objType = st;

	// Add script classes to the GC
	engine->gc.AddScriptObjectToGC(st, &engine->objectTypeBehaviours);

	// Use the default script class behaviours
	st->beh = engine->scriptTypeBehaviours.beh;

	// TODO: Move this to asCObjectType so that the asCRestore can reuse it
	engine->scriptFunctions[st->beh.addref]->AddRef();
	engine->scriptFunctions[st->beh.release]->AddRef();
	engine->scriptFunctions[st->beh.gcEnumReferences]->AddRef();
	engine->scriptFunctions[st->beh.gcGetFlag]->AddRef();
	engine->scriptFunctions[st->beh.gcGetRefCount]->AddRef();
	engine->scriptFunctions[st->beh.gcReleaseAllReferences]->AddRef();
	engine->scriptFunctions[st->beh.gcSetFlag]->AddRef();
	engine->scriptFunctions[st->beh.copy]->AddRef();
	engine->scriptFunctions[st->beh.factory]->AddRef();
	engine->scriptFunctions[st->beh.construct]->AddRef();
	for( asUINT i = 1; i < st->beh.operators.GetLength(); i += 2 )
		engine->scriptFunctions[st->beh.operators[i]]->AddRef();


	return 0;
}

int asCBuilder::RegisterInterface(asCScriptNode *node, asCScriptCode *file)
{
	asCScriptNode *n = node->firstChild;
	asCString name(&file->code[n->tokenPos], n->tokenLength);

	int r, c;
	file->ConvertPosToRowCol(n->tokenPos, &r, &c);

	CheckNameConflict(name.AddressOf(), n, file);

	sClassDeclaration *decl = asNEW(sClassDeclaration);
	interfaceDeclarations.PushLast(decl);
	decl->name       = name;
	decl->script     = file;
	decl->validState = 0;
	decl->node       = node;

	// Register the object type for the interface
	asCObjectType *st = asNEW(asCObjectType)(engine);
	st->flags = asOBJ_REF | asOBJ_SCRIPT_OBJECT;
	st->size = 0; // Cannot be instanciated
	st->name = name;
	module->classTypes.PushLast(st);
	engine->classTypes.PushLast(st);
	st->AddRef();
	decl->objType = st;

	// Use the default script class behaviours
	st->beh.construct = 0;
	st->beh.addref = engine->scriptTypeBehaviours.beh.addref;
	engine->scriptFunctions[st->beh.addref]->AddRef();
	st->beh.release = engine->scriptTypeBehaviours.beh.release;
	engine->scriptFunctions[st->beh.release]->AddRef();
	st->beh.copy = 0;

	return 0;
}


void asCBuilder::CompileGlobalVariables()
{
	asUINT n;

	bool compileSucceeded = true;

	// Store state of compilation (errors, warning, output)
	int currNumErrors = numErrors;
	int currNumWarnings = numWarnings;

	// Backup the original message stream
	bool                       msgCallback     = engine->msgCallback;
	asSSystemFunctionInterface msgCallbackFunc = engine->msgCallbackFunc;
	void                      *msgCallbackObj  = engine->msgCallbackObj;

	// Set the new temporary message stream
	asCOutputBuffer outBuffer;
	engine->SetMessageCallback(asMETHOD(asCOutputBuffer, Callback), &outBuffer, asCALL_THISCALL);

	asCOutputBuffer finalOutput;
	asCScriptFunction func(engine, module, -1);

	asCArray<asCGlobalProperty*> initOrder;

	// We first try to compile all the primitive global variables, and only after that
	// compile the non-primitive global variables. This permits the constructors 
	// for the complex types to use the already initialized variables of primitive 
	// type. Note, we currently don't know which global variables are used in the 
	// constructors, so we cannot guarantee that variables of complex types are 
	// initialized in the correct order, so we won't reorder those.
	bool compilingPrimitives = true;

	// Compile each global variable
	while( compileSucceeded )
	{
		compileSucceeded = false;

		int accumErrors = 0;
		int accumWarnings = 0;

		// Restore state of compilation
		finalOutput.Clear();
		for( asUINT n = 0; n < globVariables.GetLength(); n++ )
		{
			asCByteCode init(engine);
			numWarnings = 0;
			numErrors = 0;
			outBuffer.Clear();
			func.globalVarPointers.SetLength(0);

			sGlobalVariableDescription *gvar = globVariables[n];
			if( gvar->isCompiled )
				continue;

			// Skip this for now if we're not compiling complex types yet
			if( compilingPrimitives && !gvar->datatype.IsPrimitive() )
				continue;

			if( gvar->nextNode )
			{
				int r, c;
				gvar->script->ConvertPosToRowCol(gvar->nextNode->tokenPos, &r, &c);
				asCString str = gvar->datatype.Format();
				str += " " + gvar->name;
				str.Format(TXT_COMPILING_s, str.AddressOf());
				WriteInfo(gvar->script->name.AddressOf(), str.AddressOf(), r, c, true);
			}

			if( gvar->isEnumValue )
			{
				int r;
				if( gvar->nextNode )
				{
					asCCompiler comp(engine);

					// Temporarily switch the type of the variable to int so it can be compiled properly
					asCDataType saveType;
					saveType = gvar->datatype;
					gvar->datatype = asCDataType::CreatePrimitive(ttInt, true);
					r = comp.CompileGlobalVariable(this, gvar->script, gvar->nextNode, gvar, &func);
					gvar->datatype = saveType;
				}
				else
				{
					r = 0;

					// When there is no assignment the value is the last + 1
					int enumVal = 0;
					if( n > 0 )
					{
						sGlobalVariableDescription *gvar2 = globVariables[n-1];
						if( gvar2->datatype == gvar->datatype )
						{
							// The integer value is stored in the lower bytes
							enumVal = (*(int*)&gvar2->constantValue) + 1;

							if( !gvar2->isCompiled )
							{
								// TODO: Need to get the correct script position
								int row, col;
								gvar->script->ConvertPosToRowCol(0, &row, &col);

								asCString str = gvar->datatype.Format();
								str += " " + gvar->name;
								str.Format(TXT_COMPILING_s, str.AddressOf());
								WriteInfo(gvar->script->name.AddressOf(), str.AddressOf(), row, col, true);

								str.Format(TXT_UNINITIALIZED_GLOBAL_VAR_s, gvar2->name.AddressOf());
								WriteError(gvar->script->name.AddressOf(), str.AddressOf(), row, col);
								r = -1;
							}
						}
					}

					// The integer value is stored in the lower bytes
					*(int*)&gvar->constantValue = enumVal;
				}

				if( r >= 0 )
				{
					// Set the value as compiled
					gvar->isCompiled = true;
					compileSucceeded = true;
				}
			}
			else
			{
				// Compile the global variable
				asCCompiler comp(engine);
				int r = comp.CompileGlobalVariable(this, gvar->script, gvar->nextNode, gvar, &func);
				if( r >= 0 )
				{
					// Compilation succeeded
					gvar->isCompiled = true;
					compileSucceeded = true;

					init.AddCode(&comp.byteCode);
				}
			}

			if( gvar->isCompiled )
			{
				// Add warnings for this constant to the total build
				if( numWarnings )
				{
					currNumWarnings += numWarnings;
					if( msgCallback )
						outBuffer.SendToCallback(engine, &msgCallbackFunc, msgCallbackObj);
				}

				// Determine order of variable initializations
				if( gvar->property && !gvar->isEnumValue )
					initOrder.PushLast(gvar->property);

				if( init.GetSize() > 0 )
				{
					// Create the init function for this variable
					asCScriptFunction *initFunc = asNEW(asCScriptFunction)(engine,module,asFUNC_SCRIPT);
					initFunc->id = engine->GetNextScriptFunctionId();
					engine->SetScriptFunction(initFunc);

					// Finalize the init function for this variable
					init.Ret(0);
					init.Finalize();
					initFunc->byteCode.SetLength(init.GetSize());
					init.Output(initFunc->byteCode.AddressOf());
					initFunc->stackNeeded = init.largestStackUsed;
					initFunc->returnType = asCDataType::CreatePrimitive(ttVoid, false);
					initFunc->globalVarPointers = func.globalVarPointers;
					initFunc->AddReferences();

					// The function's refCount was already initialized to 1 
					gvar->property->initFunc = initFunc;
				}
			}
			else
			{
				// Add output to final output
				finalOutput.Append(outBuffer);
				accumErrors += numErrors;
				accumWarnings += numWarnings;
			}

			preMessage.isSet = false;
		}

		if( !compileSucceeded )
		{
			if( compilingPrimitives )
			{
				// No more primitives could be compiled, so 
				// switch to compiling the complex variables
				compilingPrimitives = false;
				compileSucceeded    = true;
			}
			else
			{
				// No more variables can be compiled
				// Add errors and warnings to total build
				currNumWarnings += accumWarnings;
				currNumErrors   += accumErrors;
				if( msgCallback )
					finalOutput.SendToCallback(engine, &msgCallbackFunc, msgCallbackObj);
			}
		}
	}

	// Restore states
	engine->msgCallback     = msgCallback;
	engine->msgCallbackFunc = msgCallbackFunc;
	engine->msgCallbackObj  = msgCallbackObj;

	numWarnings = currNumWarnings;
	numErrors   = currNumErrors;

	// Set the correct order of initialization
	if( numErrors == 0 )
	{
		// If the length of the arrays are not the same, then this is the compilation 
		// of a single variable, in which case the initialization order of the previous 
		// variables must be preserved.
		if( module->scriptGlobals.GetLength() == initOrder.GetLength() )
			module->scriptGlobals = initOrder;
	}

	// Convert all variables compiled for the enums to true enum values
	for( n = 0; n < globVariables.GetLength(); n++ )
	{
		asCObjectType *objectType;
		sGlobalVariableDescription *gvar = globVariables[n];
		if( !gvar->isEnumValue )
			continue;

		objectType = gvar->datatype.GetObjectType();
		asASSERT(NULL != objectType);

		asSEnumValue *e = asNEW(asSEnumValue);
		e->name = gvar->name;
		e->value = *(int*)&gvar->constantValue;

		objectType->enumValues.PushLast(e);

		// Destroy the gvar property
		if( gvar->nextNode )
			gvar->nextNode->Destroy(engine);
		if( gvar->property )
			asDELETE(gvar->property, asCGlobalProperty);

		asDELETE(gvar, sGlobalVariableDescription);
		globVariables[n] = 0;
	}

	// Clear the local function's global var pointers. Otherwise it will try to release them
	// TODO: This shouldn't be necessary. Find a better way to do it
	func.globalVarPointers.SetLength(0);
}

void asCBuilder::CompileClasses()
{
	asUINT n;
	asCArray<sClassDeclaration*> toValidate((int)classDeclarations.GetLength());

	// Determine class inheritances and interfaces
	for( n = 0; n < classDeclarations.GetLength(); n++ )
	{
		sClassDeclaration *decl = classDeclarations[n];
		asCScriptCode *file = decl->script;

		// Find the base class that this class inherits from
		bool multipleInheritance = false;
		asCScriptNode *node = decl->node->firstChild->next;
		while( node && node->nodeType == snIdentifier )
		{
			// Get the interface name from the node
			asCString name(&file->code[node->tokenPos], node->tokenLength);

			// Find the object type for the interface
			asCObjectType *objType = GetObjectType(name.AddressOf());

			if( objType == 0 )
			{
				int r, c;
				file->ConvertPosToRowCol(node->tokenPos, &r, &c);
				asCString str;
				str.Format(TXT_IDENTIFIER_s_NOT_DATA_TYPE, name.AddressOf());
				WriteError(file->name.AddressOf(), str.AddressOf(), r, c);
			}
			else if( !(objType->flags & asOBJ_SCRIPT_OBJECT) )
			{
				int r, c;
				file->ConvertPosToRowCol(node->tokenPos, &r, &c);
				asCString str;
				str.Format(TXT_CANNOT_INHERIT_FROM_s, objType->name.AddressOf());
				WriteError(file->name.AddressOf(), str.AddressOf(), r, c);
			}
			else if( objType->size != 0 )
			{
				// The class inherits from another script class
				if( decl->objType->derivedFrom != 0 )
				{
					if( !multipleInheritance )
					{
						int r, c;
						file->ConvertPosToRowCol(node->tokenPos, &r, &c);
						WriteError(file->name.AddressOf(), TXT_CANNOT_INHERIT_FROM_MULTIPLE_CLASSES, r, c);
						multipleInheritance = true;
					}
				}
				else
				{
					// Make sure none of the base classes inherit from this one
					asCObjectType *base = objType;
					bool error = false;
					while( base != 0 )
					{
						if( base == decl->objType )
						{
							int r, c;
							file->ConvertPosToRowCol(node->tokenPos, &r, &c);
							WriteError(file->name.AddressOf(), TXT_CANNOT_INHERIT_FROM_SELF, r, c);
							error = true;
							break;
						}

						base = base->derivedFrom;
					}

					if( !error )
					{
						decl->objType->derivedFrom = objType;
						objType->AddRef();
					}
				}
			}
			else
			{
				// The class implements an interface
				if( decl->objType->Implements(objType) )
				{
					int r, c;
					file->ConvertPosToRowCol(node->tokenPos, &r, &c);
					asCString msg;
					msg.Format(TXT_INTERFACE_s_ALREADY_IMPLEMENTED, objType->GetName());
					WriteWarning(file->name.AddressOf(), msg.AddressOf(), r, c);
				}
				else
				{
					decl->objType->interfaces.PushLast(objType);

					// Make sure all the methods of the interface are implemented
					for( asUINT i = 0; i < objType->methods.GetLength(); i++ )
					{
						if( !DoesMethodExist(decl->objType, objType->methods[i]) )
						{
							int r, c;
							file->ConvertPosToRowCol(decl->node->tokenPos, &r, &c);
							asCString str;
							str.Format(TXT_MISSING_IMPLEMENTATION_OF_s,
								engine->GetFunctionDeclaration(objType->methods[i]).AddressOf());
							WriteError(file->name.AddressOf(), str.AddressOf(), r, c);
						}
					}
				}
			}

			node = node->next;
		}
	}

	// Order class declarations so that base classes are compiled before derived classes.
	// This will allow the derived classes to copy properties and methods in the next step.
	for( n = 0; n < classDeclarations.GetLength(); n++ )
	{
		sClassDeclaration *decl = classDeclarations[n];
		asCObjectType *derived = decl->objType;
		asCObjectType *base = derived->derivedFrom;

		if( base == 0 ) continue;

		// If the base class is found after the derived class, then move the derived class to the end of the list
		for( asUINT m = n+1; m < classDeclarations.GetLength(); m++ )
		{
			sClassDeclaration *declBase = classDeclarations[m];
			if( base == declBase->objType )
			{
				classDeclarations.RemoveIndex(n);
				classDeclarations.PushLast(decl);

				// Decrease index so that we don't skip an entry
				n--;
				break;
			}
		}
	}

	// Go through each of the classes and register the object type descriptions
	for( n = 0; n < classDeclarations.GetLength(); n++ )
	{
		sClassDeclaration *decl = classDeclarations[n];

		// Add all properties and methods from the base class
		if( decl->objType->derivedFrom )
		{
			asCObjectType *baseType = decl->objType->derivedFrom;

			// The derived class inherits all interfaces from the base class
			for( unsigned int n = 0; n < baseType->interfaces.GetLength(); n++ )
			{
				if( !decl->objType->Implements(baseType->interfaces[n]) )
				{
					decl->objType->interfaces.PushLast(baseType->interfaces[n]);
				}
				else
				{
					// Warn if derived class already implements the interface
					int r, c;
					decl->script->ConvertPosToRowCol(decl->node->tokenPos, &r, &c);
					asCString msg;
					msg.Format(TXT_INTERFACE_s_ALREADY_IMPLEMENTED, baseType->interfaces[n]->GetName());
					WriteWarning(decl->script->name.AddressOf(), msg.AddressOf(), r, c);
				}
			}

			// TODO: Need to check for name conflict with new class methods

			// Copy properties from base class to derived class
			for( asUINT p = 0; p < baseType->properties.GetLength(); p++ )
			{
				asCObjectProperty *prop = AddPropertyToClass(decl, baseType->properties[p]->name, baseType->properties[p]->type);

				// The properties must maintain the same offset
				asASSERT(prop->byteOffset == baseType->properties[p]->byteOffset); UNUSED_VAR(prop);
			}

			// Copy methods from base class to derived class
			for( asUINT m = 0; m < baseType->methods.GetLength(); m++ )
			{
				// If the derived class implements the same method, then don't add the base class' method
				asCScriptFunction *baseFunc = GetFunctionDescription(baseType->methods[m]);
				asCScriptFunction *derivedFunc = 0;
				bool found = false;
				for( asUINT d = 0; d < decl->objType->methods.GetLength(); d++ )
				{
					derivedFunc = GetFunctionDescription(decl->objType->methods[d]);
					if( derivedFunc->IsSignatureEqual(baseFunc) )
					{
						// Move the function from the methods array to the virtualFunctionTable
						decl->objType->methods.RemoveIndex(d);
						decl->objType->virtualFunctionTable.PushLast(derivedFunc);
						found = true;
						break;
					}
				}

				if( !found )
				{
					// Push the base class function on the virtual function table
					decl->objType->virtualFunctionTable.PushLast(baseType->virtualFunctionTable[m]);
					baseType->virtualFunctionTable[m]->AddRef();
				}

				decl->objType->methods.PushLast(baseType->methods[m]);
				engine->scriptFunctions[baseType->methods[m]]->AddRef();
			}
		}

		// Move this class' methods into the virtual function table
		for( asUINT m = 0; m < decl->objType->methods.GetLength(); m++ )
		{
			asCScriptFunction *func = GetFunctionDescription(decl->objType->methods[m]);
			if( func->funcType != asFUNC_VIRTUAL )
			{
				// Move the reference from the method list to the virtual function list
				decl->objType->methods.RemoveIndex(m);
				decl->objType->virtualFunctionTable.PushLast(func);

				// Substitute the function description in the method list for a virtual method
				// Make sure the methods are in the same order as the virtual function table
				decl->objType->methods.PushLast(CreateVirtualFunction(func, (int)decl->objType->virtualFunctionTable.GetLength() - 1));
				m--;
			}
		}

		// Enumerate each of the declared properties
		asCScriptNode *node = decl->node->firstChild->next;

		// Skip list of classes and interfaces
		while( node && node->nodeType == snIdentifier )
			node = node->next;

		while( node )
		{
			if( node->nodeType == snDeclaration )
			{
				asCScriptCode *file = decl->script;
				asCDataType dt = CreateDataTypeFromNode(node->firstChild, file);
				asCString name(&file->code[node->lastChild->tokenPos], node->lastChild->tokenLength);

				if( dt.IsReadOnly() )
				{
					int r, c;
					file->ConvertPosToRowCol(node->tokenPos, &r, &c);

					WriteError(file->name.AddressOf(), TXT_PROPERTY_CANT_BE_CONST, r, c);
				}

				asCDataType st;
				st.SetObjectType(decl->objType);
				CheckNameConflictMember(st, name.AddressOf(), node->lastChild, file);

				AddPropertyToClass(decl, name, dt, file, node);
			}
			else
				asASSERT(false);

			node = node->next;
		}

		toValidate.PushLast(decl);
	}

	// Verify that the declared structures are valid, e.g. that the structure
	// doesn't contain a member of its own type directly or indirectly
	while( toValidate.GetLength() > 0 )
	{
		asUINT numClasses = (asUINT)toValidate.GetLength();

		asCArray<sClassDeclaration*> toValidateNext((int)toValidate.GetLength());
		while( toValidate.GetLength() > 0 )
		{
			sClassDeclaration *decl = toValidate[toValidate.GetLength()-1];
			int validState = 1;
			for( asUINT n = 0; n < decl->objType->properties.GetLength(); n++ )
			{
				// A valid structure is one that uses only primitives or other valid objects
				asCObjectProperty *prop = decl->objType->properties[n];
				asCDataType dt = prop->type;

				if( dt.IsTemplate() )
				{
					asCDataType sub = dt;
					while( sub.IsTemplate() && !sub.IsObjectHandle() )
						sub = sub.GetSubType();

					dt = sub;
				}

				if( dt.IsObject() && !dt.IsObjectHandle() )
				{
					// Find the class declaration
					sClassDeclaration *pdecl = 0;
					for( asUINT p = 0; p < classDeclarations.GetLength(); p++ )
					{
						if( classDeclarations[p]->objType == dt.GetObjectType() )
						{
							pdecl = classDeclarations[p];
							break;
						}
					}

					if( pdecl )
					{
						if( pdecl->objType == decl->objType )
						{
							int r, c;
							decl->script->ConvertPosToRowCol(decl->node->tokenPos, &r, &c);
							WriteError(decl->script->name.AddressOf(), TXT_ILLEGAL_MEMBER_TYPE, r, c);
							validState = 2;
							break;
						}
						else if( pdecl->validState != 1 )
						{
							validState = pdecl->validState;
							break;
						}
					}
				}
			}

			if( validState == 1 )
			{
				decl->validState = 1;
				toValidate.PopLast();
			}
			else if( validState == 2 )
			{
				decl->validState = 2;
				toValidate.PopLast();
			}
			else
			{
				toValidateNext.PushLast(toValidate.PopLast());
			}
		}

		toValidate = toValidateNext;
		toValidateNext.SetLength(0);

		if( numClasses == toValidate.GetLength() )
		{
			int r, c;
			toValidate[0]->script->ConvertPosToRowCol(toValidate[0]->node->tokenPos, &r, &c);
			WriteError(toValidate[0]->script->name.AddressOf(), TXT_ILLEGAL_MEMBER_TYPE, r, c);
			break;
		}
	}

	if( numErrors > 0 ) return;

	// TODO: The declarations form a graph, all circles in
	//       the graph must be flagged as potential circles

	// Verify potential circular references
	for( n = 0; n < classDeclarations.GetLength(); n++ )
	{
		sClassDeclaration *decl = classDeclarations[n];
		asCObjectType *ot = decl->objType;

		// Is there some path in which this structure is involved in circular references?
		for( asUINT p = 0; p < ot->properties.GetLength(); p++ )
		{
			asCDataType dt = ot->properties[p]->type;
			if( dt.IsObject() )
			{
				if( dt.IsObjectHandle() )
				{
					// TODO: Can this handle really generate a circular reference?
					// Only if the handle is of a type that can reference this type, either directly or indirectly

					ot->flags |= asOBJ_GC;
				}
				else if( dt.GetObjectType()->flags & asOBJ_GC )
				{
					// TODO: Just because the member type is a potential circle doesn't mean that this one is
					// Only if the object is of a type that can reference this type, either directly or indirectly

					ot->flags |= asOBJ_GC;
				}

				if( dt.IsArrayType() )
				{
					asCDataType sub = dt.GetSubType();
					while( sub.IsObject() )
					{
						if( sub.IsObjectHandle() || (sub.GetObjectType()->flags & asOBJ_GC) )
						{
							decl->objType->flags |= asOBJ_GC;

							// Make sure the array object is also marked as potential circle
							sub = dt;
							while( sub.IsTemplate() )
							{
								sub.GetObjectType()->flags |= asOBJ_GC;
								sub = sub.GetSubType();
							}

							break;
						}

						if( sub.IsTemplate() )
							sub = sub.GetSubType();
						else
							break;
					}
				}
			}
		}
	}
}

int asCBuilder::CreateVirtualFunction(asCScriptFunction *func, int idx)
{
	asCScriptFunction *vf =  asNEW(asCScriptFunction)(engine, module, asFUNC_VIRTUAL);

	vf->funcType = asFUNC_VIRTUAL;
	vf->name = func->name;
	vf->returnType = func->returnType;
	vf->parameterTypes = func->parameterTypes;
	vf->inOutFlags = func->inOutFlags;
	vf->id = engine->GetNextScriptFunctionId();
	vf->scriptSectionIdx = func->scriptSectionIdx;
	vf->isReadOnly = func->isReadOnly;
	vf->objectType = func->objectType;
	vf->signatureId = func->signatureId;
	vf->vfTableIdx = idx;

	module->AddScriptFunction(vf);

	// Add a dummy to the builder so that it doesn't mix up function ids
	functions.PushLast(0);

	return vf->id;
}

asCObjectProperty *asCBuilder::AddPropertyToClass(sClassDeclaration *decl, const asCString &name, const asCDataType &dt, asCScriptCode *file, asCScriptNode *node)
{
	// Store the properties in the object type descriptor
	asCObjectProperty *prop = asNEW(asCObjectProperty);
	prop->name = name;
	prop->type = dt;

	int propSize;
	if( dt.IsObject() )
	{
		propSize = dt.GetSizeOnStackDWords()*4;
		if( !dt.IsObjectHandle() )
		{
			if( !dt.CanBeInstanciated() )
			{
				asASSERT( file && node );

				int r, c;
				file->ConvertPosToRowCol(node->tokenPos, &r, &c);
				asCString str;
				str.Format(TXT_DATA_TYPE_CANT_BE_s, dt.Format().AddressOf());
				WriteError(file->name.AddressOf(), str.AddressOf(), r, c);
			}
			prop->type.MakeReference(true);
		}
	}
	else
	{
		propSize = dt.GetSizeInMemoryBytes();
		if( propSize == 0 && file && node )
		{
			int r, c;
			file->ConvertPosToRowCol(node->tokenPos, &r, &c);
			asCString str;
			str.Format(TXT_DATA_TYPE_CANT_BE_s, dt.Format().AddressOf());
			WriteError(file->name.AddressOf(), str.AddressOf(), r, c);
		}
	}

	// Add extra bytes so that the property will be properly aligned
	if( propSize == 2 && (decl->objType->size & 1) ) decl->objType->size += 1;
	if( propSize > 2 && (decl->objType->size & 3) ) decl->objType->size += 4 - (decl->objType->size & 3);

	prop->byteOffset = decl->objType->size;
	decl->objType->size += propSize;

	decl->objType->properties.PushLast(prop);

	// Make sure the struct holds a reference to the config group where the object is registered
	asCConfigGroup *group = engine->FindConfigGroupForObjectType(prop->type.GetObjectType());
	if( group != 0 ) group->AddRef();

	return prop;
}

bool asCBuilder::DoesMethodExist(asCObjectType *objType, int methodId)
{
	asCScriptFunction *method = GetFunctionDescription(methodId);

	for( asUINT n = 0; n < objType->methods.GetLength(); n++ )
	{
		asCScriptFunction *m = GetFunctionDescription(objType->methods[n]);

		if( m->name           != method->name           ) continue;
		if( m->returnType     != method->returnType     ) continue;
		if( m->isReadOnly     != method->isReadOnly     ) continue;
		if( m->parameterTypes != method->parameterTypes ) continue;
		if( m->inOutFlags     != method->inOutFlags     ) continue;

		return true;
	}

	return false;
}

void asCBuilder::AddDefaultConstructor(asCObjectType *objType, asCScriptCode *file)
{
	int funcId = engine->GetNextScriptFunctionId();

	asCDataType returnType = asCDataType::CreatePrimitive(ttVoid, false);
	asCArray<asCDataType> parameterTypes;
	asCArray<asETypeModifiers> inOutFlags;

	// Add the script function
	module->AddScriptFunction(file->idx, funcId, objType->name.AddressOf(), returnType, parameterTypes.AddressOf(), inOutFlags.AddressOf(), (asUINT)parameterTypes.GetLength(), false, objType);

	// Set it as default constructor
	if( objType->beh.construct )
		engine->scriptFunctions[objType->beh.construct]->Release();
	objType->beh.construct = funcId;
	objType->beh.constructors[0] = funcId;
	engine->scriptFunctions[funcId]->AddRef();

	// The bytecode for the default constructor will be generated
	// only after the potential inheritance has been established
	sFunctionDescription *func = asNEW(sFunctionDescription);
	functions.PushLast(func);

	func->script  = file;
	func->node    = 0;
	func->name    = objType->name;
	func->objType = objType;
	func->funcId  = funcId;

	// Add a default factory as well
	funcId = engine->GetNextScriptFunctionId();
	if( objType->beh.factory )
		engine->scriptFunctions[objType->beh.factory]->Release();
	objType->beh.factory = funcId;
	objType->beh.factories[0] = funcId;
	returnType = asCDataType::CreateObjectHandle(objType, false);
	module->AddScriptFunction(file->idx, funcId, objType->name.AddressOf(), returnType, parameterTypes.AddressOf(), inOutFlags.AddressOf(), (asUINT)parameterTypes.GetLength(), false);
	functions.PushLast(0);
	asCCompiler compiler(engine);
	compiler.CompileFactory(this, file, engine->scriptFunctions[funcId]);
	engine->scriptFunctions[funcId]->AddRef();
}

int asCBuilder::RegisterEnum(asCScriptNode *node, asCScriptCode *file)
{
	// Grab the name of the enumeration
	asCScriptNode *tmp = node->firstChild;
	asASSERT(snDataType == tmp->nodeType);

	asCString name;
	asASSERT(snIdentifier == tmp->firstChild->nodeType);
	name.Assign(&file->code[tmp->firstChild->tokenPos], tmp->firstChild->tokenLength);

	// Check the name and add the enum
	int r = CheckNameConflict(name.AddressOf(), tmp->firstChild, file);
	if( asSUCCESS == r )
	{
		asCObjectType *st;
		asCDataType dataType;

		st = asNEW(asCObjectType)(engine);
		dataType.CreatePrimitive(ttInt, false);

		st->flags     = asOBJ_ENUM;
		st->size      = dataType.GetSizeInMemoryBytes();
		st->name      = name;

		module->enumTypes.PushLast(st);
		st->AddRef();
		engine->classTypes.PushLast(st);

		// Store the location of this declaration for reference in name collisions
		sClassDeclaration *decl = asNEW(sClassDeclaration);
		decl->name       = name;
		decl->script     = file;
		decl->validState = 0;
		decl->node       = NULL;
		decl->objType    = st;
		namedTypeDeclarations.PushLast(decl);

		asCDataType type = CreateDataTypeFromNode(tmp, file);
		asASSERT(!type.IsReference());

		tmp = tmp->next;

		while( tmp )
		{
			asASSERT(snIdentifier == tmp->nodeType);

			asCString name(&file->code[tmp->tokenPos], tmp->tokenLength);

			// TODO: Should only have to check for conflicts within the enum type
			// Check for name conflict errors
			r = CheckNameConflict(name.AddressOf(), tmp, file);
			if(asSUCCESS != r)
			{
				continue;
			}

			//	check for assignment
			asCScriptNode *asnNode = tmp->next;
			if( asnNode && snAssignment == asnNode->nodeType )
				asnNode->DisconnectParent();
			else
				asnNode = 0;

			// Create the global variable description so the enum value can be evaluated
			sGlobalVariableDescription *gvar = asNEW(sGlobalVariableDescription);
			globVariables.PushLast(gvar);

			gvar->script		  = file;
			gvar->idNode          = 0;
			gvar->nextNode		  = asnNode;
			gvar->name			  = name;
			gvar->datatype		  = type;
			// No need to allocate space on the global memory stack since the values are stored in the asCObjectType
			gvar->index			  = 0;
			gvar->isCompiled	  = false;
			gvar->isPureConstant  = true;
			gvar->isEnumValue     = true;
			gvar->constantValue   = 0xdeadbeef;

			// Allocate dummy property so we can compile the value. 
			// This will be removed later on so we don't add it to the engine.
			gvar->property        = asNEW(asCGlobalProperty);
			gvar->property->name  = name;
			gvar->property->type  = gvar->datatype;
			gvar->property->id    = 0;

			tmp = tmp->next;
		}
	}

	node->Destroy(engine);

	return r;
}

int asCBuilder::RegisterTypedef(asCScriptNode *node, asCScriptCode *file)
{
	// Get the native data type
	asCScriptNode *tmp = node->firstChild;
	asASSERT(NULL != tmp && snDataType == tmp->nodeType);
	asCDataType dataType;
	dataType.CreatePrimitive(tmp->tokenType, false);
	dataType.SetTokenType(tmp->tokenType);
	tmp = tmp->next;

	// Grab the name of the typedef
	asASSERT(NULL != tmp && NULL == tmp->next);
	asCString name;
	name.Assign(&file->code[tmp->tokenPos], tmp->tokenLength);

	// If the name is not already in use add it
 	int r = CheckNameConflict(name.AddressOf(), tmp, file);
	if( asSUCCESS == r )
	{
		// Create the new type
		asCObjectType *st = asNEW(asCObjectType)(engine);

		st->flags           = asOBJ_TYPEDEF;
		st->size            = dataType.GetSizeInMemoryBytes();
		st->name            = name;
		st->templateSubType = dataType;

		st->AddRef();

		module->typeDefs.PushLast(st);
		engine->classTypes.PushLast(st);

		// Store the location of this declaration for reference in name collisions
		sClassDeclaration *decl = asNEW(sClassDeclaration);
		decl->name       = name;
		decl->script     = file;
		decl->validState = 0;
		decl->node       = NULL;
		decl->objType    = st;
		namedTypeDeclarations.PushLast(decl);
	}

	node->Destroy(engine);

	if( r < 0 )
	{
		engine->ConfigError(r);
	}

	return 0;
}

void asCBuilder::GetParsedFunctionDetails(asCScriptNode *node, asCScriptCode *file, asCObjectType *objType, asCString &name, asCDataType &returnType, asCArray<asCDataType> &parameterTypes, asCArray<asETypeModifiers> &inOutFlags, bool &isConstMethod, bool &isConstructor, bool &isDestructor)
{
	// Find the name
	isConstructor = false;
	isDestructor = false;
	asCScriptNode *n = 0;
	if( node->firstChild->nodeType == snDataType )
		n = node->firstChild->next->next;
	else
	{
		// If the first node is a ~ token, then we know it is a destructor
		if( node->firstChild->tokenType == ttBitNot )
		{
			n = node->firstChild->next;
			isDestructor = true;
		}
		else
		{
			n = node->firstChild;
			isConstructor = true;
		}
	}
	name.Assign(&file->code[n->tokenPos], n->tokenLength);

	// Initialize a script function object for registration
	if( !isConstructor && !isDestructor )
	{
		returnType = CreateDataTypeFromNode(node->firstChild, file);
		returnType = ModifyDataTypeFromNode(returnType, node->firstChild->next, file, 0, 0);
	}
	else
		returnType = asCDataType::CreatePrimitive(ttVoid, false);

	// Is this a const method?
	if( objType && n->next->next && n->next->next->tokenType == ttConst )
		isConstMethod = true;
	else
		isConstMethod = false;

	// Count the number of parameters
	int count = 0;
	asCScriptNode *c = n->next->firstChild;
	while( c )
	{
		count++;
		c = c->next->next;
		if( c && c->nodeType == snIdentifier )
			c = c->next;
	}

	// Get the parameter types
	parameterTypes.Allocate(count, false);
	inOutFlags.Allocate(count, false);
	n = n->next->firstChild;
	while( n )
	{
		asETypeModifiers inOutFlag;
		asCDataType type = CreateDataTypeFromNode(n, file);
		type = ModifyDataTypeFromNode(type, n->next, file, &inOutFlag, 0);

		// Store the parameter type
		parameterTypes.PushLast(type);
		inOutFlags.PushLast(inOutFlag);

		// Move to next parameter
		n = n->next->next;
		if( n && n->nodeType == snIdentifier )
			n = n->next;
	}
}


int asCBuilder::RegisterScriptFunction(int funcID, asCScriptNode *node, asCScriptCode *file, asCObjectType *objType, bool isInterface, bool isGlobalFunction)
{
	asCString                  name;
	asCDataType                returnType;
	asCArray<asCDataType>      parameterTypes;
	asCArray<asETypeModifiers> inOutFlags;
	bool                       isConstMethod;
	bool                       isConstructor;
	bool                       isDestructor;

	GetParsedFunctionDetails(node, file, objType, name, returnType, parameterTypes, inOutFlags, isConstMethod, isConstructor, isDestructor);

	// Check for name conflicts
	if( !isConstructor && !isDestructor )
	{
		asCDataType dt = asCDataType::CreateObject(objType, false);
		if( objType )
			CheckNameConflictMember(dt, name.AddressOf(), node, file);
		else
			CheckNameConflict(name.AddressOf(), node, file);
	}
	else
	{
		// Verify that the name of the constructor/destructor is the same as the class
		if( name != objType->name )
		{
			int r, c;
			file->ConvertPosToRowCol(node->tokenPos, &r, &c);
			WriteError(file->name.AddressOf(), TXT_CONSTRUCTOR_NAME_ERROR, r, c);
		}

		if( isDestructor )
			name = "~" + name;
	}

	if( !isInterface )
	{
		sFunctionDescription *func = asNEW(sFunctionDescription);
		functions.PushLast(func);

		func->script  = file;
		func->node    = node;
		func->name    = name;
		func->objType = objType;
		func->funcId  = funcID;
	}

	// Destructors may not have any parameters
	if( isDestructor && parameterTypes.GetLength() > 0 )
	{
		int r, c;
		file->ConvertPosToRowCol(node->tokenPos, &r, &c);

		WriteError(file->name.AddressOf(), TXT_DESTRUCTOR_MAY_NOT_HAVE_PARM, r, c);
	}

	// TODO: Much of this can probably be reduced by using the IsSignatureEqual method
	// Check that the same function hasn't been registered already
	asCArray<int> funcs;
	GetFunctionDescriptions(name.AddressOf(), funcs);
	if( funcs.GetLength() )
	{
		for( asUINT n = 0; n < funcs.GetLength(); ++n )
		{
			asCScriptFunction *func = GetFunctionDescription(funcs[n]);

			if( parameterTypes.GetLength() == func->parameterTypes.GetLength() )
			{
				bool match = true;
				if( func->objectType != objType )
				{
					match = false;
					break;
				}

				for( asUINT p = 0; p < parameterTypes.GetLength(); ++p )
				{
					if( parameterTypes[p] != func->parameterTypes[p] )
					{
						match = false;
						break;
					}
				}

				if( match )
				{
					int r, c;
					file->ConvertPosToRowCol(node->tokenPos, &r, &c);

					WriteError(file->name.AddressOf(), TXT_FUNCTION_ALREADY_EXIST, r, c);
					break;
				}
			}
		}
	}

	// Register the function
	module->AddScriptFunction(file->idx, funcID, name.AddressOf(), returnType, parameterTypes.AddressOf(), inOutFlags.AddressOf(), (asUINT)parameterTypes.GetLength(), isInterface, objType, isConstMethod, isGlobalFunction);

	if( objType )
	{
		engine->scriptFunctions[funcID]->AddRef();
		if( isConstructor )
		{
			int factoryId = engine->GetNextScriptFunctionId();
			if( parameterTypes.GetLength() == 0 )
			{
				// Overload the default constructor
				engine->scriptFunctions[objType->beh.construct]->Release();
				objType->beh.construct = funcID;
				objType->beh.constructors[0] = funcID;

				// Register the default factory as well
				engine->scriptFunctions[objType->beh.factory]->Release();
				objType->beh.factory = factoryId;
				objType->beh.factories[0] = factoryId;
			}
			else
			{
				objType->beh.constructors.PushLast(funcID);

				// Register the factory as well
				objType->beh.factories.PushLast(factoryId);
			}

			asCDataType dt = asCDataType::CreateObjectHandle(objType, false);
			module->AddScriptFunction(file->idx, factoryId, name.AddressOf(), dt, parameterTypes.AddressOf(), inOutFlags.AddressOf(), (asUINT)parameterTypes.GetLength(), false);

			// Add a dummy function to the module so that it doesn't mix up the fund Ids
			functions.PushLast(0);

			// Compile the factory immediately
			asCCompiler compiler(engine);
			compiler.CompileFactory(this, file, engine->scriptFunctions[factoryId]);
			engine->scriptFunctions[factoryId]->AddRef();
		}
		else if( isDestructor )
			objType->beh.destruct = funcID;
		else
			objType->methods.PushLast(funcID);
	}

	// We need to delete the node already if this is an interface method
	if( isInterface && node )
	{
		node->Destroy(engine);
	}

	return 0;
}

int asCBuilder::RegisterImportedFunction(int importID, asCScriptNode *node, asCScriptCode *file)
{
	// Find name
	asCScriptNode *f = node->firstChild;
	asCScriptNode *n = f->firstChild->next->next;

	// Check for name conflicts
	asCString name(&file->code[n->tokenPos], n->tokenLength);
	CheckNameConflict(name.AddressOf(), n, file);

	// Initialize a script function object for registration
	asCDataType returnType;
	returnType = CreateDataTypeFromNode(f->firstChild, file);
	returnType = ModifyDataTypeFromNode(returnType, f->firstChild->next, file, 0, 0);

	// Count the parameters
	int count = 0;
	asCScriptNode *c = n->next->firstChild;
	while( c )
	{
		count++;
		c = c->next->next;
		if( c && c->nodeType == snIdentifier )
			c = c->next;
	}

	asCArray<asCDataType> parameterTypes(count);
	asCArray<asETypeModifiers> inOutFlags(count);
	n = n->next->firstChild;
	while( n )
	{
		asETypeModifiers inOutFlag;
		asCDataType type = CreateDataTypeFromNode(n, file);
		type = ModifyDataTypeFromNode(type, n->next, file, &inOutFlag, 0);

		// Store the parameter type
		n = n->next->next;
		parameterTypes.PushLast(type);
		inOutFlags.PushLast(inOutFlag);

		// Move to next parameter
		if( n && n->nodeType == snIdentifier )
			n = n->next;
	}

	// Check that the same function hasn't been registered already
	asCArray<int> funcs;
	GetFunctionDescriptions(name.AddressOf(), funcs);
	if( funcs.GetLength() )
	{
		for( asUINT n = 0; n < funcs.GetLength(); ++n )
		{
			asCScriptFunction *func = GetFunctionDescription(funcs[n]);

			// TODO: Isn't the name guaranteed to be equal, because of GetFunctionDescriptions()?
			if( name == func->name &&
				parameterTypes.GetLength() == func->parameterTypes.GetLength() )
			{
				bool match = true;
				for( asUINT p = 0; p < parameterTypes.GetLength(); ++p )
				{
					if( parameterTypes[p] != func->parameterTypes[p] )
					{
						match = false;
						break;
					}
				}

				if( match )
				{
					int r, c;
					file->ConvertPosToRowCol(node->tokenPos, &r, &c);

					WriteError(file->name.AddressOf(), TXT_FUNCTION_ALREADY_EXIST, r, c);
					break;
				}
			}
		}
	}

	// Read the module name as well
	n = node->firstChild->next;
	asCString moduleName;
	moduleName.Assign(&file->code[n->tokenPos+1], n->tokenLength-2);

	node->Destroy(engine);

	// Register the function
	module->AddImportedFunction(importID, name.AddressOf(), returnType, parameterTypes.AddressOf(), inOutFlags.AddressOf(), (asUINT)parameterTypes.GetLength(), moduleName);

	return 0;
}


asCScriptFunction *asCBuilder::GetFunctionDescription(int id)
{
	// TODO: This should be improved
	// Get the description from the engine
	if( (id & 0xFFFF0000) == 0 )
		return engine->scriptFunctions[id];
	else
		return engine->importedFunctions[id & 0xFFFF]->importedFunctionSignature;
}

void asCBuilder::GetFunctionDescriptions(const char *name, asCArray<int> &funcs)
{
	asUINT n;
	// TODO: optimize: Improve linear search
	for( n = 0; n < module->scriptFunctions.GetLength(); n++ )
	{
		if( module->scriptFunctions[n]->name == name &&
			module->scriptFunctions[n]->objectType == 0 )
			funcs.PushLast(module->scriptFunctions[n]->id);
	}

	// TODO: optimize: Improve linear search
	for( n = 0; n < module->bindInformations.GetLength(); n++ )
	{
		if( module->bindInformations[n]->importedFunctionSignature->name == name )
			funcs.PushLast(module->bindInformations[n]->importedFunctionSignature->id);
	}

	// TODO: optimize: Improve linear search
	// TODO: optimize: Use the registeredGlobalFunctions array instead
	for( n = 0; n < engine->scriptFunctions.GetLength(); n++ )
	{
		if( engine->scriptFunctions[n] &&
			engine->scriptFunctions[n]->funcType == asFUNC_SYSTEM &&
			engine->scriptFunctions[n]->objectType == 0 &&
			engine->scriptFunctions[n]->name == name )
		{
			// Find the config group for the global function
			asCConfigGroup *group = engine->FindConfigGroupForFunction(engine->scriptFunctions[n]->id);
			if( !group || group->HasModuleAccess(module->name.AddressOf()) )
				funcs.PushLast(engine->scriptFunctions[n]->id);
		}
	}
}

void asCBuilder::GetObjectMethodDescriptions(const char *name, asCObjectType *objectType, asCArray<int> &methods, bool objIsConst, const asCString &scope)
{
	if( scope != "" )
	{
		// Find the base class with the specified scope
		while( objectType && objectType->name != scope )
			objectType = objectType->derivedFrom;

		// If the scope is not any of the base classes, then return no methods
		if( objectType == 0 )
			return;
	}

	// TODO: optimize: Improve linear search
	if( objIsConst )
	{
		// Only add const methods to the list
		for( asUINT n = 0; n < objectType->methods.GetLength(); n++ )
		{
			if( engine->scriptFunctions[objectType->methods[n]]->name == name &&
				engine->scriptFunctions[objectType->methods[n]]->isReadOnly )
			{
				// When the scope is defined the returned methods should be the true methods, not the virtual method stubs
				if( scope == "" )
					methods.PushLast(engine->scriptFunctions[objectType->methods[n]]->id);
				else
				{
					asCScriptFunction *virtFunc = engine->scriptFunctions[objectType->methods[n]];
					asCScriptFunction *realFunc = objectType->virtualFunctionTable[virtFunc->vfTableIdx];
					methods.PushLast(realFunc->id);
				}
			}
		}
	}
	else
	{
		// TODO: Prefer non-const over const
		for( asUINT n = 0; n < objectType->methods.GetLength(); n++ )
		{
			if( engine->scriptFunctions[objectType->methods[n]]->name == name )
			{
				// When the scope is defined the returned methods should be the true methods, not the virtual method stubs
				if( scope == "" )
					methods.PushLast(engine->scriptFunctions[objectType->methods[n]]->id);
				else
				{
					asCScriptFunction *virtFunc = engine->scriptFunctions[objectType->methods[n]];
					asCScriptFunction *realFunc = objectType->virtualFunctionTable[virtFunc->vfTableIdx];
					methods.PushLast(realFunc->id);
				}
			}
		}
	}
}

void asCBuilder::WriteInfo(const char *scriptname, const char *message, int r, int c, bool pre)
{
	// Need to store the pre message in a structure
	if( pre )
	{
		preMessage.isSet = true;
		preMessage.c = c;
		preMessage.r = r;
		preMessage.message = message;
	}
	else
	{
		preMessage.isSet = false;
		engine->WriteMessage(scriptname, r, c, asMSGTYPE_INFORMATION, message);
	}
}

void asCBuilder::WriteError(const char *scriptname, const char *message, int r, int c)
{
	numErrors++;

	// Need to pass the preMessage first
	if( preMessage.isSet )
		WriteInfo(scriptname, preMessage.message.AddressOf(), preMessage.r, preMessage.c, false);

	engine->WriteMessage(scriptname, r, c, asMSGTYPE_ERROR, message);
}

void asCBuilder::WriteWarning(const char *scriptname, const char *message, int r, int c)
{
	numWarnings++;

	// Need to pass the preMessage first
	if( preMessage.isSet )
		WriteInfo(scriptname, preMessage.message.AddressOf(), preMessage.r, preMessage.c, false);

	engine->WriteMessage(scriptname, r, c, asMSGTYPE_WARNING, message);
}


asCDataType asCBuilder::CreateDataTypeFromNode(asCScriptNode *node, asCScriptCode *file, bool acceptHandleForScope, asCObjectType *templateType)
{
	asASSERT(node->nodeType == snDataType);

	asCDataType dt;

	asCScriptNode *n = node->firstChild;

	bool isConst = false;
	bool isImplicitHandle = false;
	if( n->tokenType == ttConst )
	{
		isConst = true;
		n = n->next;
	}

	if( n->tokenType == ttIdentifier )
	{
		asCString str;
		str.Assign(&file->code[n->tokenPos], n->tokenLength);

		asCObjectType *ot = 0;

		// If this is for a template type, then we must first determine if the 
		// identifier matches any of the template subtypes
		// TODO: template: it should be possible to have more than one subtypes
		if( templateType && (templateType->flags & asOBJ_TEMPLATE) && str == templateType->templateSubType.GetObjectType()->name )
			ot = templateType->templateSubType.GetObjectType();

		if( ot == 0 )
			ot = GetObjectType(str.AddressOf());

		if( ot == 0 )
		{
			asCString msg;
			msg.Format(TXT_IDENTIFIER_s_NOT_DATA_TYPE, (const char *)str.AddressOf());

			int r, c;
			file->ConvertPosToRowCol(n->tokenPos, &r, &c);

			WriteError(file->name.AddressOf(), msg.AddressOf(), r, c);

			dt.SetTokenType(ttInt);
		}
		else
		{
			if( ot->flags & asOBJ_IMPLICIT_HANDLE )
				isImplicitHandle = true;

			// Find the config group for the object type
			asCConfigGroup *group = engine->FindConfigGroupForObjectType(ot);
			if( !module || !group || group->HasModuleAccess(module->name.AddressOf()) )
			{
				if(asOBJ_TYPEDEF == (ot->flags & asOBJ_TYPEDEF))
				{
					// TODO: typedef: A typedef should be considered different from the original type (though with implicit conversions between the two)
					// Create primitive data type based on object flags
					dt = ot->templateSubType;
					dt.MakeReadOnly(isConst);
				}
				else
				{
					if( ot->flags & asOBJ_TEMPLATE )
					{
						n = n->next;
						
						// Check if the subtype is a type or the template's subtype
						// if it is the template's subtype then this is the actual template type,
						// orderwise it is a template instance.
						asCDataType subType = CreateDataTypeFromNode(n, file, false, ot);
						if( subType.GetObjectType() != ot->templateSubType.GetObjectType() )
						{
							// This is a template instance
							// Need to find the correct object type
							asCObjectType *otInstance = engine->GetTemplateInstanceType(ot, subType);

							if( !otInstance )
							{
								asCString msg;
								msg.Format(TXT_CANNOT_INSTANCIATE_TEMPLATE_s_WITH_s, ot->name.AddressOf(), subType.Format().AddressOf());
								int r, c;
								file->ConvertPosToRowCol(n->tokenPos, &r, &c);
								WriteError(file->name.AddressOf(), msg.AddressOf(), r, c);
							}

							ot = otInstance;
						}
					}

					// Create object data type
					if( ot )
						dt = asCDataType::CreateObject(ot, isConst);
					else
						dt = asCDataType::CreatePrimitive(ttInt, isConst);
				}
			}
			else
			{
				asCString msg;
				msg.Format(TXT_TYPE_s_NOT_AVAILABLE_FOR_MODULE, (const char *)str.AddressOf());

				int r, c;
				file->ConvertPosToRowCol(n->tokenPos, &r, &c);

				WriteError(file->name.AddressOf(), msg.AddressOf(), r, c);

				dt.SetTokenType(ttInt);
			}
		}
	}
	else
	{
		// Create primitive data type
		dt = asCDataType::CreatePrimitive(n->tokenType, isConst);
	}

	// Determine array dimensions and object handles
	n = n->next;
	while( n && (n->tokenType == ttOpenBracket || n->tokenType == ttHandle) )
	{
		if( n->tokenType == ttOpenBracket )
		{
			// Make sure the sub type can be instanciated
			if( !dt.CanBeInstanciated() )
			{
				int r, c;
				file->ConvertPosToRowCol(n->tokenPos, &r, &c);

				asCString str;
				// TODO: Change to "Array sub type cannot be 'type'"
				str.Format(TXT_DATA_TYPE_CANT_BE_s, dt.Format().AddressOf());

				WriteError(file->name.AddressOf(), str.AddressOf(), r, c);
			}

			// Make the type an array (or multidimensional array)
			if( dt.MakeArray(engine) < 0 )
			{
				int r, c;
				file->ConvertPosToRowCol(n->tokenPos, &r, &c);
				WriteError(file->name.AddressOf(), TXT_TOO_MANY_ARRAY_DIMENSIONS, r, c);
				break;
			}
		}
		else
		{
			// Make the type a handle
			if( dt.MakeHandle(true, acceptHandleForScope) < 0 )
			{
				int r, c;
				file->ConvertPosToRowCol(n->tokenPos, &r, &c);
				WriteError(file->name.AddressOf(), TXT_OBJECT_HANDLE_NOT_SUPPORTED, r, c);
				break;
			}
		}
		n = n->next;
	}

	if( isImplicitHandle )
	{
		// Make the type a handle
		if( dt.MakeHandle(true, acceptHandleForScope) < 0 )
		{
			int r, c;
			file->ConvertPosToRowCol(n->tokenPos, &r, &c);
			WriteError(file->name.AddressOf(), TXT_OBJECT_HANDLE_NOT_SUPPORTED, r, c);
		}
	}

	return dt;
}

asCDataType asCBuilder::ModifyDataTypeFromNode(const asCDataType &type, asCScriptNode *node, asCScriptCode *file, asETypeModifiers *inOutFlags, bool *autoHandle)
{
	asCDataType dt = type;

	if( inOutFlags ) *inOutFlags = asTM_NONE;

	// Is the argument sent by reference?
	asCScriptNode *n = node->firstChild;
	if( n && n->tokenType == ttAmp )
	{
		dt.MakeReference(true);
		n = n->next;

		if( n )
		{
			if( inOutFlags )
			{
				if( n->tokenType == ttIn )
					*inOutFlags = asTM_INREF;
				else if( n->tokenType == ttOut )
					*inOutFlags = asTM_OUTREF;
				else if( n->tokenType == ttInOut )
					*inOutFlags = asTM_INOUTREF;
				else
					asASSERT(false);
			}

			n = n->next;
		}
		else
		{
			if( inOutFlags )
				*inOutFlags = asTM_INOUTREF; // ttInOut
		}

		if( !engine->ep.allowUnsafeReferences &&
			inOutFlags && *inOutFlags == asTM_INOUTREF )
		{
			// Verify that the base type support &inout parameter types
			if( !dt.IsObject() || dt.IsObjectHandle() || !dt.GetObjectType()->beh.addref || !dt.GetObjectType()->beh.release )
			{
				int r, c;
				file->ConvertPosToRowCol(node->firstChild->tokenPos, &r, &c);
				WriteError(file->name.AddressOf(), TXT_ONLY_OBJECTS_MAY_USE_REF_INOUT, r, c);
			}
		}
	}

	if( autoHandle ) *autoHandle = false;

	if( n && n->tokenType == ttPlus )
	{
		if( autoHandle ) *autoHandle = true;
	}

	return dt;
}

asCObjectType *asCBuilder::GetObjectType(const char *type)
{
	// TODO: Only search in config groups to which the module has access
	asCObjectType *ot = engine->GetObjectType(type);
	if( !ot && module )
		ot = module->GetObjectType(type);

	return ot;
}

int asCBuilder::GetEnumValueFromObjectType(asCObjectType *objType, const char *name, asCDataType &outDt, asDWORD &outValue)
{
	if( !objType || !(objType->flags & asOBJ_ENUM) )
		return 0;

	for( asUINT n = 0; n < objType->enumValues.GetLength(); ++n )
	{
		if( objType->enumValues[n]->name == name )
		{
			outDt = asCDataType::CreateObject(objType, true);
			outValue = objType->enumValues[n]->value;
			return 1;
		}
	}

	return 0;
}

int asCBuilder::GetEnumValue(const char *name, asCDataType &outDt, asDWORD &outValue)
{
	bool found = false;

	// Search all available enum types
	asUINT t;
	for( t = 0; t < engine->objectTypes.GetLength(); t++ )
	{
		asCObjectType *ot = engine->objectTypes[t];
		if( GetEnumValueFromObjectType( ot, name, outDt, outValue ) )
		{
			if( !found )
			{
				found = true;
			}
			else
			{
				// Found more than one value in different enum types
				return 2;
			}
		}
	}

	for( t = 0; t < module->enumTypes.GetLength(); t++ )
	{
		asCObjectType *ot = module->enumTypes[t];
		if( GetEnumValueFromObjectType( ot, name, outDt, outValue ) )
		{
			if( !found )
			{
				found = true;
			}
			else
			{
				// Found more than one value in different enum types
				return 2;
			}
		}
	}

	if( found )
		return 1;

	// Didn't find any value
	return 0;
}

END_AS_NAMESPACE
