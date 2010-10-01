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
#include "as_scriptstruct.h"

BEGIN_AS_NAMESPACE

asCBuilder::asCBuilder(asCScriptEngine *engine, asCModule *module)
{
	this->engine = engine;
	this->module = module;
	out = 0;
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
				delete functions[n]->node;

			delete functions[n];
		}

		functions[n] = 0;
	}

	// Free all global variables
	for( n = 0; n < globVariables.GetLength(); n++ )
	{
		if( globVariables[n] )
		{
			if( globVariables[n]->node )
				delete globVariables[n]->node;

			delete globVariables[n];
			globVariables[n] = 0;
		}
	}

	// Free all the loaded files
	for( n = 0; n < scripts.GetLength(); n++ )
	{
		if( scripts[n] )
			delete scripts[n];

		scripts[n] = 0;
	}

	// Free all struct declarations
	for( n = 0; n < structDeclarations.GetLength(); n++ )
	{
		if( structDeclarations[n] )
		{
			if( structDeclarations[n]->node )
				delete structDeclarations[n]->node;

			delete structDeclarations[n];
			structDeclarations[n] = 0;
		}
	}
}

void asCBuilder::SetOutputStream(asIOutputStream *out)
{
	this->out = out;
}

int asCBuilder::AddCode(const char *name, const char *code, int codeLength, int lineOffset, int sectionIdx, bool makeCopy)
{
	asCScriptCode *script = new asCScriptCode;
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

	ParseScripts();
	CompileStructs();
	CompileGlobalVariables();
	CompileFunctions();

	if( numErrors > 0 )
		return asERROR;

	return asSUCCESS;
}

int asCBuilder::BuildString(const char *string, asCContext *ctx)
{
	numErrors = 0;
	numWarnings = 0;

	// Add the string to the script code
	asCScriptCode *script = new asCScriptCode;
	script->SetCode(TXT_EXECUTESTRING, string, true);
	script->lineOffset = -1; // Compensate for "void ExecuteString() {\n"
	scripts.PushLast(script);

	// Parse the string
	asCParser parser(this);
	if( parser.ParseScript(scripts[0]) >= 0 )
	{
		// Find the function
		asCScriptNode *node = parser.GetScriptNode();
		node = node->firstChild;
		if( node->nodeType == snFunction )
		{
			node->DisconnectParent();

			sFunctionDescription *func = new sFunctionDescription;
			functions.PushLast(func);

			func->script = scripts[0];
			func->node = node;
			func->name = "";
		}
		else
		{
			// TODO: An error occurred
			assert(false);
		}
	}

	if( numErrors == 0 )
	{
		// Compile the function
		asCCompiler compiler;
		asCScriptFunction *execfunc = new asCScriptFunction;
		if( compiler.CompileFunction(this, functions[0]->script, functions[0]->node, execfunc) >= 0 )
		{
			execfunc->id = asFUNC_STRING | module->moduleID;

			// Copy byte code to the registered function
			execfunc->byteCode.SetLength(compiler.byteCode.GetSize());
			// TODO: pass the function pointer directly
			compiler.byteCode.Output(execfunc->byteCode.AddressOf());
			execfunc->stackNeeded = compiler.byteCode.largestStackUsed;
			execfunc->lineNumbers = compiler.byteCode.lineNumbers;

			execfunc->objVariablePos = compiler.objVariablePos;
			execfunc->objVariableTypes = compiler.objVariableTypes;

			ctx->SetExecuteStringFunction(execfunc);

#ifdef AS_DEBUG
			// DEBUG: output byte code
			compiler.byteCode.DebugOutput("__ExecuteString.txt", module, engine);
#endif
		}
		else
			delete execfunc;
	}

	if( numErrors > 0 )
		return asERROR;

	return asSUCCESS;
}

void asCBuilder::ParseScripts()
{
	asCArray<asCParser*> parsers;
	
	// Parse all the files as if they were one
	asUINT n = 0;
	for( n = 0; n < scripts.GetLength(); n++ )
	{
		asCParser *parser = new asCParser(this);
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
				if( node->nodeType == snStruct )
				{
					node->DisconnectParent();
					RegisterStruct(node, scripts[n]);
				}

				node = next;
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
					RegisterScriptFunction(module->GetNextFunctionId(), node, scripts[n]);
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

					delete node;
				}

				node = next;
			}
		}
	}

	for( n = 0; n < parsers.GetLength(); n++ )
		delete parsers[n];
}

void asCBuilder::CompileFunctions()
{
	// Compile each function
	for( asUINT n = 0; n < functions.GetLength(); n++ )
	{
		asCCompiler compiler;

		int r, c;
		functions[n]->script->ConvertPosToRowCol(functions[n]->node->tokenPos, &r, &c);
		asCString str = module->scriptFunctions[n]->GetDeclaration(engine);
		str.Format(TXT_COMPILING_s, str.AddressOf());
		WriteInfo(functions[n]->script->name.AddressOf(), str.AddressOf(), r, c, true);

		if( compiler.CompileFunction(this, functions[n]->script, functions[n]->node, module->scriptFunctions[n]) >= 0 )
		{
			// Copy byte code to the registered function
			module->scriptFunctions[n]->byteCode.SetLength(compiler.byteCode.GetSize());
			// TODO: Pass the function pointer directly
			compiler.byteCode.Output(module->scriptFunctions[n]->byteCode.AddressOf());
			module->scriptFunctions[n]->stackNeeded = compiler.byteCode.largestStackUsed;
			module->scriptFunctions[n]->lineNumbers = compiler.byteCode.lineNumbers;

			module->scriptFunctions[n]->objVariablePos = compiler.objVariablePos;
			module->scriptFunctions[n]->objVariableTypes = compiler.objVariableTypes;

#ifdef AS_DEBUG
			// DEBUG: output byte code
			compiler.byteCode.DebugOutput(("__" + functions[n]->name + ".txt").AddressOf(), module, engine);
#endif
		}

		preMessage = "";
	}
}

int asCBuilder::ParseDataType(const char *datatype, asCDataType *result)
{
	numErrors = 0;
	numWarnings = 0;

	asCScriptCode source;
	source.SetCode("", datatype, true);

	asCParser parser(this);
	int r = parser.ParseDataType(&source);
	if( r < 0 )
		return asINVALID_TYPE;

	// Get data type and property name
	asCScriptNode *dataType = parser.GetScriptNode()->firstChild;

	*result = CreateDataTypeFromNode(dataType, &source);

	if( numErrors > 0 )
		return asINVALID_TYPE;

	return asSUCCESS;
}

int asCBuilder::VerifyProperty(asCDataType *dt, const char *decl, asCString &name, asCDataType &type)
{
	numErrors = 0;
	numWarnings = 0;

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
		if( CheckNameConflictMember(*dt, name.AddressOf(), nameNode, &source) < 0 )
			return asINVALID_NAME;
	else
		if( CheckNameConflict(name.AddressOf(), nameNode, &source) < 0 )
			return asINVALID_NAME;

	if( numErrors > 0 )
		return asINVALID_DECLARATION;

	return asSUCCESS;
}

asCProperty *asCBuilder::GetObjectProperty(asCDataType &obj, const char *prop)
{
	assert(obj.GetObjectType() != 0);

	// TODO: Only search in config groups to which the module has access
	// TODO: Improve linear search
	asCArray<asCProperty *> &props = obj.GetObjectType()->properties;
	for( asUINT n = 0; n < props.GetLength(); n++ )
		if( props[n]->name == prop )
			return props[n];

	return 0;
}

asCProperty *asCBuilder::GetGlobalProperty(const char *prop, bool *isCompiled, bool *isPureConstant, asQWORD *constantValue)
{
	asUINT n;

	if( isCompiled ) *isCompiled = true;
	if( isPureConstant ) *isPureConstant = false;

	// TODO: Improve linear search
	// Check application registered properties
	asCArray<asCProperty *> *props = &(engine->globalProps);
	for( n = 0; n < props->GetLength(); ++n )
		if( (*props)[n] && (*props)[n]->name == prop )
		{
			// Find the config group for the global property
			asCConfigGroup *group = engine->FindConfigGroupForGlobalVar((*props)[n]->index);
			if( !group || group->HasModuleAccess(module->name.AddressOf()) )
				return (*props)[n];
		}

	// TODO: Improve linear search
	// Check properties being compiled now
	asCArray<sGlobalVariableDescription *> *gvars = &globVariables;
	for( n = 0; n < gvars->GetLength(); ++n )
	{
		if( (*gvars)[n]->name == prop )
		{
			if( isCompiled ) *isCompiled = (*gvars)[n]->isCompiled;

			if( isPureConstant ) *isPureConstant = (*gvars)[n]->isPureConstant;
			if( constantValue  ) *constantValue  = (*gvars)[n]->constantValue;

			return (*gvars)[n]->property;
		}
	}

	// TODO: Improve linear search
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

int asCBuilder::ParseFunctionDeclaration(const char *decl, asCScriptFunction *func, asCArray<bool> *paramAutoHandles, bool *returnAutoHandle)
{
	numErrors = 0;
	numWarnings = 0;

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
	func->returnType = CreateDataTypeFromNode(node->firstChild, &source);
	func->returnType = ModifyDataTypeFromNode(func->returnType, node->firstChild->next, 0, &autoHandle);
	if( autoHandle && (!func->returnType.IsObjectHandle() || func->returnType.IsReference()) )
			return asINVALID_DECLARATION;			
	if( returnAutoHandle ) *returnAutoHandle = autoHandle;

	n = n->next->firstChild;
	while( n )
	{
		int inOutFlags;
		asCDataType type = CreateDataTypeFromNode(n, &source);
		type = ModifyDataTypeFromNode(type, n->next, &inOutFlags, &autoHandle);
		
		// Store the parameter type
		func->parameterTypes.PushLast(type);
		func->inOutFlags.PushLast(inOutFlags);

		if( autoHandle && (!type.IsObjectHandle() || type.IsReference()) )
			return asINVALID_DECLARATION;			

		if( paramAutoHandles ) paramAutoHandles->PushLast(autoHandle);

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

int asCBuilder::ParseVariableDeclaration(const char *decl, asCProperty *var)
{
	numErrors = 0;
	numWarnings = 0;

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

	// TODO: Improve linear search
	asCArray<asCProperty *> &props = t->properties;
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
	asCProperty *prop = GetGlobalProperty(name, 0, 0, 0);
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

	// Check against struct types
	for( asUINT n = 0; n < structDeclarations.GetLength(); n++ )
	{
		if( structDeclarations[n]->name == name )
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

	return 0;
}


int asCBuilder::RegisterGlobalVar(asCScriptNode *node, asCScriptCode *file)
{
	// What data type is it?
	asCDataType type = CreateDataTypeFromNode(node->firstChild, file);

	if( type.GetSizeOnStackDWords() == 0 || 
		(type.IsObject() && !type.IsObjectHandle() && type.GetSizeInMemoryBytes() == 0) )
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
		GETSTRING(name, &file->code[n->tokenPos], n->tokenLength);
		CheckNameConflict(name.AddressOf(), n, file);

		// Register the global variable
		sGlobalVariableDescription *gvar = new sGlobalVariableDescription;
		globVariables.PushLast(gvar);

		gvar->script     = file;
		gvar->name       = name;
		gvar->isCompiled = false;
		gvar->datatype   = type;

		// TODO: Give error message if wrong
		assert(!gvar->datatype.IsReference());

		// Allocate space on the global memory stack
		gvar->index = module->AllocGlobalMemory(gvar->datatype.GetSizeOnStackDWords());
		gvar->node = 0;
		if( n->next && 
			(n->next->nodeType == snAssignment ||
			 n->next->nodeType == snArgList    || 
			 n->next->nodeType == snInitList     ) )
		{
			gvar->node = n->next;
			n->next->DisconnectParent();
		}

		// Add script variable to engine
		asCProperty *prop = new asCProperty;
		prop->index      = gvar->index;
		prop->name       = name;
		prop->type       = gvar->datatype;
		module->scriptGlobals.PushLast(prop);

		gvar->property = prop;

		n = n->next;
	}

	delete node;

	return 0;
}

int asCBuilder::RegisterStruct(asCScriptNode *node, asCScriptCode *file)
{
	asCScriptNode *n = node->firstChild;
	GETSTRING(name, &file->code[n->tokenPos], n->tokenLength);
	
	int r, c;
	file->ConvertPosToRowCol(n->tokenPos, &r, &c);

	CheckNameConflict(name.AddressOf(), n, file);

	sStructDeclaration *decl = new sStructDeclaration;
	structDeclarations.PushLast(decl);
	decl->name = name;
	decl->script = file;
	decl->validState = 0;
	decl->node = node;

	asCObjectType *st = new asCObjectType(engine);
	st->arrayType = 0;
	st->flags = asOBJ_CLASS_CDA | asOBJ_SCRIPT_STRUCT;
	st->size = sizeof(asCScriptStruct);
	st->name = name;
	st->tokenType = ttIdentifier;
	module->structTypes.PushLast(st);
	engine->structTypes.PushLast(st);
	st->refCount++;
	decl->objType = st;

	// Use the default script struct behaviours
	st->beh.construct = engine->scriptTypeBehaviours.beh.construct;
	st->beh.constructors.PushLast(st->beh.construct);
	st->beh.addref = engine->scriptTypeBehaviours.beh.addref;
	st->beh.release = engine->scriptTypeBehaviours.beh.release;
	st->beh.copy = engine->scriptTypeBehaviours.beh.copy;
	st->beh.operators.PushLast(ttAssignment);
	st->beh.operators.PushLast(st->beh.copy);

	return 0;
}

void asCBuilder::CompileGlobalVariables()
{
	bool compileSucceeded = true;

	asCByteCode finalInit;

	// Store state of compilation (errors, warning, output)
	int currNumErrors = numErrors;
	int currNumWarnings = numWarnings;
	asIOutputStream *stream = out;
	asCOutputBuffer outBuffer;
	out = &outBuffer;

	asCString finalOutput;

	while( compileSucceeded )
	{
		compileSucceeded = false;

		int accumErrors = 0;
		int accumWarnings = 0;

		// Restore state of compilation
		finalOutput = "";

		for( asUINT n = 0; n < globVariables.GetLength(); n++ )
		{
			numWarnings = 0;
			numErrors = 0;
			outBuffer.output = "";
			asCByteCode init;

			sGlobalVariableDescription *gvar = globVariables[n];
			if( gvar->isCompiled == false )
			{
				if( gvar->node )
				{
					int r, c;
					gvar->script->ConvertPosToRowCol(gvar->node->tokenPos, &r, &c);
					asCString str = gvar->datatype.Format();
					str += " " + gvar->name;
					str.Format(TXT_COMPILING_s, str.AddressOf());
					WriteInfo(gvar->script->name.AddressOf(), str.AddressOf(), r, c, true);
				}

				asCCompiler comp;
				int r = comp.CompileGlobalVariable(this, gvar->script, gvar->node, gvar);
				if( r >= 0 )
				{
					// Compilation succeeded
					gvar->isCompiled = true;
					compileSucceeded = true;

					init.AddCode(&comp.byteCode);
				}

				if( gvar->isCompiled )
				{
					// Add warnings for this constant to the total build
					if( numWarnings )
					{
						currNumWarnings += numWarnings;
						stream->Write(outBuffer.output.AddressOf());
					}

					// Add compiled byte code to the final init and exit functions
					finalInit.AddCode(&init);
				}
				else
				{
					// Add output to final output
					finalOutput += outBuffer.output;
					accumErrors += numErrors;
					accumWarnings += numWarnings;
				}

				preMessage = "";
			}
		}

		if( !compileSucceeded )
		{
			// Add errors and warnings to total build
			currNumWarnings += accumWarnings;
			currNumErrors += accumErrors;

			if( stream && finalOutput != "" ) stream->Write(finalOutput.AddressOf());
		}
	}

	// Restore states
	out = stream;
	numWarnings = currNumWarnings;
	numErrors = currNumErrors;

	// Register init code and clean up code
	finalInit.Ret(0);

	finalInit.Finalize();

	asCByteCode cleanInit;
	asCByteCode cleanExit;

	module->initFunction.byteCode.SetLength(finalInit.GetSize());
	// TODO: Pass the function pointer directly
	finalInit.Output(module->initFunction.byteCode.AddressOf());
	module->initFunction.stackNeeded = finalInit.largestStackUsed;

#ifdef AS_DEBUG
	// DEBUG: output byte code
	finalInit.DebugOutput("__@init.txt", module, engine);
#endif

}

void asCBuilder::CompileStructs()
{
	asUINT n;
	asCArray<sStructDeclaration*> toValidate;

	// Go through each of the structs and register the object type descriptions
	for( n = 0; n < structDeclarations.GetLength(); n++ )
	{
		sStructDeclaration *decl = structDeclarations[n];

		// Enumerate each of the declared properties
		asCScriptNode *node = decl->node->firstChild->next;
		while( node )
		{
			asCScriptCode *file = decl->script;
			asCDataType dt = CreateDataTypeFromNode(node->firstChild, file);
			GETSTRING(name, &file->code[node->lastChild->tokenPos], node->lastChild->tokenLength);

			if( dt.IsReadOnly() )
			{
				int r, c;
				file->ConvertPosToRowCol(node->tokenPos, &r, &c);

				WriteError(file->name.AddressOf(), TXT_PROPERTY_CANT_BE_CONST, r, c);
			}

			asCDataType st;
			st.SetObjectType(decl->objType);
			CheckNameConflictMember(st, name.AddressOf(), node->lastChild, file);

			// Store the properties in the object type descriptor
			asCProperty *prop = new asCProperty;
			prop->name = name;
			prop->type = dt;

			int propSize;
			if( dt.IsObject() )
			{
				propSize = dt.GetSizeOnStackDWords()*4;
				if( !dt.IsObjectHandle() )
					prop->type.MakeReference(true);
			}
			else
				propSize = dt.GetSizeInMemoryBytes();

			// Add extra bytes so that the property will be properly aligned
			if( propSize == 2 && (decl->objType->size & 1) ) decl->objType->size += 1;
			if( propSize > 2 && (decl->objType->size & 3) ) decl->objType->size += 3 - (decl->objType->size & 3);

			prop->byteOffset = decl->objType->size;
			decl->objType->size += propSize;

			decl->objType->properties.PushLast(prop);

			// Make sure the module holds a reference to the config group where the object is registered
			module->RefConfigGroupForObjectType(dt.GetObjectType());

			node = node->next;
		}

		toValidate.PushLast(decl);
	}

	// Verify that the declared structures are valid, e.g. that the structure
	// doesn't contain a member of its own type directly or indirectly
	while( toValidate.GetLength() > 0 ) 
	{
		asUINT numStructs = toValidate.GetLength();

		asCArray<sStructDeclaration*> toValidateNext;
		while( toValidate.GetLength() > 0 )
		{
			sStructDeclaration *decl = toValidate[toValidate.GetLength()-1];
			int validState = 1;
			for( asUINT n = 0; n < decl->objType->properties.GetLength(); n++ )
			{
				// A valid structure is one that uses only primitives or other valid objects
				asCProperty *prop = decl->objType->properties[n];
				asCDataType dt = prop->type;

				if( dt.IsScriptArray() )
				{
					asCDataType sub = dt;
					while( sub.IsScriptArray() && !sub.IsObjectHandle() )
						sub = sub.GetSubType();

					dt = sub;
				}

				if( dt.IsObject() && !dt.IsObjectHandle() )
				{
					// Find the struct declaration
					sStructDeclaration *pdecl = 0;
					for( asUINT p = 0; p < structDeclarations.GetLength(); p++ )
					{
						if( structDeclarations[p]->objType == dt.GetObjectType() )
						{
							pdecl = structDeclarations[p];
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

		if( numStructs == toValidate.GetLength() )
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
	for( n = 0; n < structDeclarations.GetLength(); n++ )
	{
		sStructDeclaration *decl = structDeclarations[n];
		asCObjectType *ot = decl->objType;

		// Is there some path in which this structure is involved in circular references?
		for( asUINT p = 0; p < ot->properties.GetLength(); p++ )
		{
			asCDataType dt = ot->properties[p]->type;
			if( dt.IsObject() )
			{
				// Any structure that contains an any type can generate circular references
				if( dt.GetObjectType()->flags & asOBJ_CONTAINS_ANY )
				{
					ot->flags |= asOBJ_POTENTIAL_CIRCLE | asOBJ_CONTAINS_ANY;
				}

				if( dt.IsObjectHandle() )
				{
					// TODO:
					// Can this handle really generate a circular reference

					ot->flags |= asOBJ_POTENTIAL_CIRCLE;
				}
				else if( dt.GetObjectType()->flags & asOBJ_POTENTIAL_CIRCLE )
				{
					// TODO:
					// Just because the member type is a potential circle doesn't mean that this one is

					ot->flags |= asOBJ_POTENTIAL_CIRCLE;
				}

				if( dt.IsArrayType() )
				{
					asCDataType sub = dt.GetSubType();
					while( sub.IsObject() )
					{
						if( sub.IsObjectHandle() || (sub.GetObjectType()->flags & asOBJ_POTENTIAL_CIRCLE) )
						{
							decl->objType->flags |= asOBJ_POTENTIAL_CIRCLE;

							// Make sure the array object is also marked as potential circle
							sub = dt;
							while( sub.IsScriptArray() )
							{
								sub.GetObjectType()->flags |= asOBJ_POTENTIAL_CIRCLE;
								sub = sub.GetSubType();
							}

							break;
						}

						if( sub.IsScriptArray() )
							sub = sub.GetSubType();
						else
							break;
					}
				}
			}
		}
	}
}

int asCBuilder::RegisterScriptFunction(int funcID, asCScriptNode *node, asCScriptCode *file)
{
	// Find name 
	asCScriptNode *n = node->firstChild->next->next;

	// Check for name conflicts
	GETSTRING(name, &file->code[n->tokenPos], n->tokenLength);
	CheckNameConflict(name.AddressOf(), n, file);

	sFunctionDescription *func = new sFunctionDescription;
	functions.PushLast(func);

	func->script = file;
	func->node   = node;
	func->name   = name;

	// Initialize a script function object for registration
	asCDataType returnType;
	returnType = CreateDataTypeFromNode(node->firstChild, file);
	returnType = ModifyDataTypeFromNode(returnType, node->firstChild->next, 0, 0);
		
	module->RefConfigGroupForObjectType(returnType.GetObjectType());

	asCArray<asCDataType> parameterTypes;
	asCArray<int> inOutFlags;
	n = n->next->firstChild;
	while( n )
	{
		int inOutFlag;
		asCDataType type = CreateDataTypeFromNode(n, file);
		type = ModifyDataTypeFromNode(type, n->next, &inOutFlag, 0);

		module->RefConfigGroupForObjectType(type.GetObjectType());

		// Store the parameter type
		parameterTypes.PushLast(type);
		inOutFlags.PushLast(inOutFlag);

		// Move to next parameter
		n = n->next->next;
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

			if( parameterTypes.GetLength() == func->parameterTypes.GetLength() )
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

	// Register the function
	module->AddScriptFunction(file->idx, funcID, func->name.AddressOf(), returnType, parameterTypes.AddressOf(), inOutFlags.AddressOf(), parameterTypes.GetLength());

	return 0;
}

int asCBuilder::RegisterImportedFunction(int importID, asCScriptNode *node, asCScriptCode *file)
{
	// Find name 
	asCScriptNode *f = node->firstChild;
	asCScriptNode *n = f->firstChild->next->next;

	// Check for name conflicts
	GETSTRING(name, &file->code[n->tokenPos], n->tokenLength);
	CheckNameConflict(name.AddressOf(), n, file);

	// Initialize a script function object for registration
	asCDataType returnType;
	returnType = CreateDataTypeFromNode(f->firstChild, file);
	returnType = ModifyDataTypeFromNode(returnType, f->firstChild->next, 0, 0);
		
	asCArray<asCDataType> parameterTypes;
	asCArray<int> inOutFlags;
	n = n->next->firstChild;
	while( n )
	{
		int inOutFlag;
		asCDataType type = CreateDataTypeFromNode(n, file);
		type = ModifyDataTypeFromNode(type, n->next, &inOutFlag, 0);

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
	int moduleNameString = module->AddConstantString(&file->code[n->tokenPos+1], n->tokenLength-2);

	delete node;

	// Register the function
	module->AddImportedFunction(importID, name.AddressOf(), returnType, parameterTypes.AddressOf(), inOutFlags.AddressOf(), parameterTypes.GetLength(), moduleNameString);

	return 0;
}


asCScriptFunction *asCBuilder::GetFunctionDescription(int id)
{
	// The top 16 bits holds the moduleID

	// Get the description from the engine
	if( id < 0 )
		return engine->systemFunctions[-id - 1];
	else if( (id & 0xFFFF0000) == 0 )
		return module->scriptFunctions[id];
	else 
		return module->importedFunctions[id & 0xFFFF];
}

void asCBuilder::GetFunctionDescriptions(const char *name, asCArray<int> &funcs)
{
	asUINT n;
	// TODO: Improve linear search
	for( n = 0; n < module->scriptFunctions.GetLength(); n++ )
	{
		if( module->scriptFunctions[n]->name == name )
			funcs.PushLast(module->scriptFunctions[n]->id);
	}

	// TODO: Improve linear search
	for( n = 0; n < module->importedFunctions.GetLength(); n++ )
	{
		if( module->importedFunctions[n]->name == name )
			funcs.PushLast(module->importedFunctions[n]->id);
	}

	// TODO: Improve linear search
	for( n = 0; n < engine->systemFunctions.GetLength(); n++ )
	{
		if( engine->systemFunctions[n] && engine->systemFunctions[n]->objectType == 0 && engine->systemFunctions[n]->name == name )
		{
			// Find the config group for the global function
			asCConfigGroup *group = engine->FindConfigGroupForFunction(engine->systemFunctions[n]->id);
			if( !group || group->HasModuleAccess(module->name.AddressOf()) )
				funcs.PushLast(engine->systemFunctions[n]->id);
		}
	}
}

void asCBuilder::GetObjectMethodDescriptions(const char *name, asCObjectType *objectType, asCArray<int> &methods, bool objIsConst)
{
	// TODO: Improve linear search
	if( objIsConst )
	{
		// Only add const methods to the list
		for( asUINT n = 0; n < objectType->methods.GetLength(); n++ )
		{
			if( engine->systemFunctions[objectType->methods[n]]->name == name &&
				engine->systemFunctions[objectType->methods[n]]->isReadOnly )
				methods.PushLast(engine->systemFunctions[objectType->methods[n]]->id);
		}
	}
	else
	{
		// TODO: Prefer non-const over const
		for( asUINT n = 0; n < objectType->methods.GetLength(); n++ )
		{
			if( engine->systemFunctions[objectType->methods[n]]->name == name )
				methods.PushLast(engine->systemFunctions[objectType->methods[n]]->id);
		}
	}
}

void asCBuilder::WriteInfo(const char *scriptname, const char *message, int r, int c, bool pre)
{
	if( out )
	{
		asCString str;
		str.Format("%s (%d, %d) : %s : %s\n", scriptname, r, c, TXT_INFO, message);

		if( pre )
			preMessage = str;
		else
			out->Write(str.AddressOf());
	}
}

void asCBuilder::WriteError(const char *scriptname, const char *message, int r, int c)
{
	numErrors++;

	if( out )
	{
		if( preMessage != "" )
		{
			out->Write(preMessage.AddressOf());
			preMessage = "";
		}

		asCString str;
		str.Format("%s (%d, %d) : %s : %s\n", scriptname, r, c, TXT_ERROR, message);

		out->Write(str.AddressOf());
	}
}

void asCBuilder::WriteWarning(const char *scriptname, const char *message, int r, int c)
{
	numWarnings++;

	if( out )
	{
		if( preMessage != "" )
		{
			out->Write(preMessage.AddressOf());
			preMessage = "";
		}

		asCString str;
		str.Format("%s (%d, %d) : %s : %s\n", scriptname, r, c, TXT_WARNING, message);

		out->Write(str.AddressOf());
	}
}


asCDataType asCBuilder::CreateDataTypeFromNode(asCScriptNode *node, asCScriptCode *file)
{
	assert(node->nodeType == snDataType);

	asCDataType dt;

	asCScriptNode *n = node->firstChild;

	bool isConst = false;
	if( n->tokenType == ttConst )
	{
		isConst = true;
		n = n->next;
	}

	if( n->tokenType == ttIdentifier )
	{
		asCString str;
		str.Assign(&file->code[n->tokenPos], n->tokenLength);

		asCObjectType *ot = GetObjectType(str.AddressOf());
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
			// Find the config group for the object type
			asCConfigGroup *group = engine->FindConfigGroupForObjectType(ot);
			if( !module || !group || group->HasModuleAccess(module->name.AddressOf()) )
			{
				// Create object data type
				dt = asCDataType::CreateObject(ot, isConst);
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
			if( dt.MakeHandle(true) < 0 )
			{
				int r, c;
				file->ConvertPosToRowCol(n->tokenPos, &r, &c);
				WriteError(file->name.AddressOf(), TXT_OBJECT_HANDLE_NOT_SUPPORTED, r, c);
				break;
			}
		}
		n = n->next;
	}

	return dt;
}

asCDataType asCBuilder::ModifyDataTypeFromNode(const asCDataType &type, asCScriptNode *node, int *inOutFlags, bool *autoHandle)
{
	asCDataType dt = type;

	if( inOutFlags ) *inOutFlags = 0;

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
					*inOutFlags = 1;
				else if( n->tokenType == ttOut )
					*inOutFlags = 2;
				else if( n->tokenType == ttInOut )
					*inOutFlags = 3;
				else
					assert(false);
			}

			n = n->next;
		}
#ifdef AS_ALLOW_UNSAFE_REFERENCES
		else
		{
			if( inOutFlags )
				*inOutFlags = 3; // ttInOut
		}
#endif
	}

	if( autoHandle ) *autoHandle = false;

	if( n && n->tokenType == ttPlus )
	{
		if( autoHandle ) *autoHandle = true;
	}

	return dt;
}

const asCString &asCBuilder::GetConstantString(int strID)
{
	return module->GetConstantString(strID);
}

asCObjectType *asCBuilder::GetObjectType(const char *type)
{
	// TODO: Only search in config groups to which the module has access
	asCObjectType *ot = engine->GetObjectType(type);
	if( !ot && module )
		ot = module->GetObjectType(type);

	return ot;
}

END_AS_NAMESPACE
