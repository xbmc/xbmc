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
// as_callfunc.cpp
//
// These functions handle the actual calling of system functions
//



#include "as_config.h"
#include "as_callfunc.h"
#include "as_scriptengine.h"
#include "as_texts.h"

BEGIN_AS_NAMESPACE

int DetectCallingConvention(bool isMethod, const asSFuncPtr &ptr, int callConv, asSSystemFunctionInterface *internal)
{
	memset(internal, 0, sizeof(asSSystemFunctionInterface));

	internal->func = (size_t)ptr.ptr.f.func;

	// Was a compatible calling convention specified?
	if( internal->func )
	{
		if( ptr.flag == 1 && callConv != asCALL_GENERIC )
			return asWRONG_CALLING_CONV;
		else if( ptr.flag == 2 && (callConv == asCALL_GENERIC || callConv == asCALL_THISCALL) )
			return asWRONG_CALLING_CONV;
		else if( ptr.flag == 3 && callConv != asCALL_THISCALL )
			return asWRONG_CALLING_CONV;
	}

	asDWORD base = callConv;
	if( !isMethod )
	{
		if( base == asCALL_CDECL )
			internal->callConv = ICC_CDECL;
		else if( base == asCALL_STDCALL )
			internal->callConv = ICC_STDCALL;
		else if( base == asCALL_GENERIC )
			internal->callConv = ICC_GENERIC_FUNC;
		else
			return asNOT_SUPPORTED;
	}
	else
	{
#ifndef AS_NO_CLASS_METHODS
		if( base == asCALL_THISCALL )
		{
			internal->callConv = ICC_THISCALL;
#ifdef GNU_STYLE_VIRTUAL_METHOD
			if( (size_t(ptr.ptr.f.func) & 1) )
				internal->callConv = ICC_VIRTUAL_THISCALL;
#endif
			internal->baseOffset = ( int )MULTI_BASE_OFFSET(ptr);
#if defined(AS_ARM) && defined(__GNUC__)
			// As the least significant bit in func is used to switch to THUMB mode
			// on ARM processors, the LSB in the __delta variable is used instead of
			// the one in __pfn on ARM processors.
			if( (size_t(internal->baseOffset) & 1) )
				internal->callConv = ICC_VIRTUAL_THISCALL;
#endif

#ifdef HAVE_VIRTUAL_BASE_OFFSET
			// We don't support virtual inheritance
			if( VIRTUAL_BASE_OFFSET(ptr) != 0 )
				return asNOT_SUPPORTED;
#endif
		}
		else
#endif
		if( base == asCALL_CDECL_OBJLAST )
			internal->callConv = ICC_CDECL_OBJLAST;
		else if( base == asCALL_CDECL_OBJFIRST )
			internal->callConv = ICC_CDECL_OBJFIRST;
		else if( base == asCALL_GENERIC )
			internal->callConv = ICC_GENERIC_METHOD;
		else
			return asNOT_SUPPORTED;
	}

	return 0;
}

// This function should prepare system functions so that it will be faster to call them
int PrepareSystemFunctionGeneric(asCScriptFunction *func, asSSystemFunctionInterface *internal, asCScriptEngine * /*engine*/)
{
	asASSERT(internal->callConv == ICC_GENERIC_METHOD || internal->callConv == ICC_GENERIC_FUNC);

	// Calculate the size needed for the parameters
	internal->paramSize = func->GetSpaceNeededForArguments();

	return 0;
}

// This function should prepare system functions so that it will be faster to call them
int PrepareSystemFunction(asCScriptFunction *func, asSSystemFunctionInterface *internal, asCScriptEngine *engine)
{
#ifdef AS_MAX_PORTABILITY
	// This should never happen, as when AS_MAX_PORTABILITY is on, all functions 
	// are asCALL_GENERIC, which are prepared by PrepareSystemFunctionGeneric
	asASSERT(false);
#endif

	// References are always returned as primitive data
	if( func->returnType.IsReference() || func->returnType.IsObjectHandle() )
	{
		internal->hostReturnInMemory = false;
		internal->hostReturnSize     = sizeof(void*)/4;
		internal->hostReturnFloat    = false;
	}
	// Registered types have special flags that determine how they are returned
	else if( func->returnType.IsObject() )
	{
		asDWORD objType = func->returnType.GetObjectType()->flags;
	
		// Only value types can be returned by value
		asASSERT( objType & asOBJ_VALUE );

		if( !(objType & (asOBJ_APP_CLASS | asOBJ_APP_PRIMITIVE | asOBJ_APP_FLOAT)) )
		{
			// If the return is by value then we need to know the true type
			engine->WriteMessage("", 0, 0, asMSGTYPE_INFORMATION, func->GetDeclarationStr().AddressOf());

			asCString str;
			str.Format(TXT_CANNOT_RET_TYPE_s_BY_VAL, func->returnType.GetObjectType()->name.AddressOf());
			engine->WriteMessage("", 0, 0, asMSGTYPE_ERROR, str.AddressOf());
			engine->ConfigError(asINVALID_CONFIGURATION);
		}
		else if( objType & asOBJ_APP_CLASS )
		{
			internal->hostReturnFloat = false;
			if( objType & COMPLEX_MASK )
			{
				internal->hostReturnInMemory = true;
				internal->hostReturnSize     = sizeof(void*)/4;
			}
			else
			{
#ifdef HAS_128_BIT_PRIMITIVES
				if( func->returnType.GetSizeInMemoryDWords() > 4 )
#else
				if( func->returnType.GetSizeInMemoryDWords() > 2 )
#endif
				{
					internal->hostReturnInMemory = true;
					internal->hostReturnSize = sizeof(void*)/4;
				}
				else
				{
					internal->hostReturnInMemory = false;
					internal->hostReturnSize     = func->returnType.GetSizeInMemoryDWords();
				}

#ifdef THISCALL_RETURN_SIMPLE_IN_MEMORY
				if((internal->callConv == ICC_THISCALL ||
					internal->callConv == ICC_VIRTUAL_THISCALL) &&
					func->returnType.GetSizeInMemoryDWords() >= THISCALL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE)
				{
					internal->hostReturnInMemory = true;
					internal->hostReturnSize     = sizeof(void*)/4;
				}
#endif
#ifdef CDECL_RETURN_SIMPLE_IN_MEMORY
				if((internal->callConv == ICC_CDECL         ||
					internal->callConv == ICC_CDECL_OBJLAST ||
					internal->callConv == ICC_CDECL_OBJFIRST) &&
					func->returnType.GetSizeInMemoryDWords() >= CDECL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE)
				{
					internal->hostReturnInMemory = true;
					internal->hostReturnSize     = sizeof(void*)/4;
				}
#endif
#ifdef STDCALL_RETURN_SIMPLE_IN_MEMORY
				if( internal->callConv == ICC_STDCALL &&
					func->returnType.GetSizeInMemoryDWords() >= STDCALL_RETURN_SIMPLE_IN_MEMORY_MIN_SIZE)
				{
					internal->hostReturnInMemory = true;
					internal->hostReturnSize     = sizeof(void*)/4;
				}
#endif
			}
		}
		else if( objType & asOBJ_APP_PRIMITIVE )
		{
			internal->hostReturnInMemory = false;
			internal->hostReturnSize     = func->returnType.GetSizeInMemoryDWords();
			internal->hostReturnFloat    = false;
		}
		else if( objType & asOBJ_APP_FLOAT )
		{
			internal->hostReturnInMemory = false;
			internal->hostReturnSize     = func->returnType.GetSizeInMemoryDWords();
			internal->hostReturnFloat    = true;
		}
	}
	// Primitive types can easily be determined
#ifdef HAS_128_BIT_PRIMITIVES
	else if( func->returnType.GetSizeInMemoryDWords() > 4 )
	{
		// Shouldn't be possible to get here
		asASSERT(false);
	}
	else if( func->returnType.GetSizeInMemoryDWords() == 4 )
	{
		internal->hostReturnInMemory = false;
		internal->hostReturnSize     = 4;
		internal->hostReturnFloat    = false;
	}
#else
	else if( func->returnType.GetSizeInMemoryDWords() > 2 )
	{
		// Shouldn't be possible to get here
		asASSERT(false);
	}
#endif
	else if( func->returnType.GetSizeInMemoryDWords() == 2 )
	{
		internal->hostReturnInMemory = false;
		internal->hostReturnSize     = 2;
		internal->hostReturnFloat    = func->returnType.IsEqualExceptConst(asCDataType::CreatePrimitive(ttDouble, true));
	}
	else if( func->returnType.GetSizeInMemoryDWords() == 1 )
	{
		internal->hostReturnInMemory = false;
		internal->hostReturnSize     = 1;
		internal->hostReturnFloat    = func->returnType.IsEqualExceptConst(asCDataType::CreatePrimitive(ttFloat, true));
	}
	else
	{
		internal->hostReturnInMemory = false;
		internal->hostReturnSize     = 0;
		internal->hostReturnFloat    = false;
	}

	// Calculate the size needed for the parameters
	internal->paramSize = func->GetSpaceNeededForArguments();

	// Verify if the function takes any objects by value
	asUINT n;
	internal->takesObjByVal = false;
	for( n = 0; n < func->parameterTypes.GetLength(); n++ )
	{
		if( func->parameterTypes[n].IsObject() && !func->parameterTypes[n].IsObjectHandle() && !func->parameterTypes[n].IsReference() )
		{
			internal->takesObjByVal = true;

			// Can't pass objects by value unless the application type is informed
			if( !(func->parameterTypes[n].GetObjectType()->flags & (asOBJ_APP_CLASS | asOBJ_APP_PRIMITIVE | asOBJ_APP_FLOAT)) )
			{
				engine->WriteMessage("", 0, 0, asMSGTYPE_INFORMATION, func->GetDeclarationStr().AddressOf());
	
				asCString str;
				str.Format(TXT_CANNOT_PASS_TYPE_s_BY_VAL, func->parameterTypes[n].GetObjectType()->name.AddressOf());
				engine->WriteMessage("", 0, 0, asMSGTYPE_ERROR, str.AddressOf());
				engine->ConfigError(asINVALID_CONFIGURATION);
			}


#ifdef SPLIT_OBJS_BY_MEMBER_TYPES
			// It's not safe to pass objects by value because different registers
			// will be used depending on the memory layout of the object
#ifdef COMPLEX_OBJS_PASSED_BY_REF
			if( !(func->parameterTypes[n].GetObjectType()->flags & COMPLEX_MASK) )	
#endif
			{
				engine->WriteMessage("", 0, 0, asMSGTYPE_INFORMATION, func->GetDeclarationStr().AddressOf());

				asCString str;
				str.Format(TXT_DONT_SUPPORT_TYPE_s_BY_VAL, func->parameterTypes[n].GetObjectType()->name.AddressOf());
				engine->WriteMessage("", 0, 0, asMSGTYPE_ERROR, str.AddressOf());
				engine->ConfigError(asINVALID_CONFIGURATION);
			}
#endif
			break;
		}
	}

	// Verify if the function has any registered autohandles
	internal->hasAutoHandles = false;
	for( n = 0; n < internal->paramAutoHandles.GetLength(); n++ )
	{
		if( internal->paramAutoHandles[n] )
		{
			internal->hasAutoHandles = true;
			break;
		}
	}

	return 0;
}

#ifdef AS_MAX_PORTABILITY

int CallSystemFunction(int id, asCContext *context, void *objectPointer)
{
	asCScriptEngine *engine = context->engine;
	asSSystemFunctionInterface *sysFunc = engine->scriptFunctions[id]->sysFuncIntf;
	int callConv = sysFunc->callConv;
	if( callConv == ICC_GENERIC_FUNC || callConv == ICC_GENERIC_METHOD )
		return context->CallGeneric(id, objectPointer);

	context->SetInternalException(TXT_INVALID_CALLING_CONVENTION);

	return 0;
}

#endif // AS_MAX_PORTABILITY

END_AS_NAMESPACE

