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
// as_texts.h
//
// These are text strings used through out the library
//


#ifndef AS_TEXTS_H
#define AS_TEXTS_H

// Compiler messages

#define TXT_s_ALREADY_DECLARED            "'%s' already declared"
#define TXT_ARG_NOT_LVALUE                "Argument cannot be assigned. Output will be discarded."
#define TXT_ARGUMENT_COUNT                "Argument count does not match definition."
#define TXT_ASSIGN_IN_GLOBAL_EXPR         "Assignments are not allowed in global expressions"

#define TXT_BOTH_MUST_BE_SAME             "Both expressions must have the same type"

#define TXT_CANT_GUARANTEE_REF            "Cannot guarantee safety of reference. Copy the value to a local variable first"
#define TXT_CANT_IMPLICITLY_CONVERT_s_TO_s "Can't implicitly convert from '%s' to '%s'."
#define TXT_CANT_RETURN_VALUE             "Can't return value when return type is 'void'"
#define TXT_CHANGE_SIGN_u_d               "Implicit conversion changed sign of value, %u -> %d."
#define TXT_CHANGE_SIGN_d_u               "Implicit conversion changed sign of value, %d -> %u."
#define TXT_COMPILING_s                   "Compiling %s"
#define TXT_CONST_NOT_PRIMITIVE           "Only primitives may be declared as const"

#define TXT_DATA_TYPE_CANT_BE_s           "Data type can't be '%s'"
#define TXT_DEFAULT_MUST_BE_LAST          "The default case must be the last one"

#define TXT_EMPTY_CHAR_LITERAL            "Empty character literal"
#define TXT_EXPECTED_s                    "Expected '%s'"
#define TXT_EXPECTED_CONSTANT             "Expected constant"
// TODO: Should be TXT_NO_CONVERSION
#define TXT_EXPECTED_CONSTANT_s_FOUND_s   "Expected constant of type '%s', found '%s'"
#define TXT_EXPECTED_DATA_TYPE            "Expected data type"
#define TXT_EXPECTED_EXPRESSION_VALUE     "Expected expression value"
#define TXT_EXPECTED_IDENTIFIER           "Expected identifier"
#define TXT_EXPECTED_ONE_OF               "Expected one of: "
#define TXT_EXPECTED_OPERATOR             "Expected operator"
#define TXT_EXPECTED_s_OR_s               "Expected '%s' or '%s'"
#define TXT_EXPECTED_POST_OPERATOR        "Expected post operator"
#define TXT_EXPECTED_PRE_OPERATOR         "Expected pre operator"
#define TXT_EXPECTED_STRING               "Expected string"
#define TXT_EXPR_MUST_BE_BOOL             "Expression must be of boolean type"
#define TXT_EXPR_MUST_EVAL_TO_CONSTANT    "Expression must evaluate to a constant"


#define TXT_FUNCTION_IN_GLOBAL_EXPR       "Function calls are not allowed in global expressions"
#define TXT_FUNCTION_ALREADY_EXIST        "A function with the same name and parameters already exist"

#define TXT_IDENTIFIER_s_NOT_DATA_TYPE    "Identifier '%s' is not a data type"
#define TXT_ILLEGAL_CALL                  "Illegal call"
// TODO: Should be TXT_ILLEGAL_OPERATION_ON_s
#define TXT_ILLEGAL_OPERATION             "Illegal operation on this datatype"
#define TXT_ILLEGAL_OPERATION_ON_s        "Illegal operation on '%s'"
#define TXT_INC_OP_IN_GLOBAL_EXPR         "Incremental operators are not allowed in global expressions"
#define TXT_INIT_LIST_CANNOT_BE_USED_WITH_s "Initialization lists cannot be used with '%s'"
#define TXT_INVALID_BREAK                 "Invalid 'break'"
#define TXT_INVALID_CONTINUE              "Invalid 'continue'"
#define TXT_INVALID_TYPE                  "Invalid type"
#define TXT_ILLEGAL_MEMBER_TYPE           "Illegal member type"
#define TXT_ILLEGAL_VARIABLE_NAME_s       "Illegal variable name '%s'."

#define TXT_METHOD_IN_GLOBAL_EXPR         "Object method calls are not allowed in global expressions"
#define TXT_MORE_THAN_ONE_MATCHING_OP     "Found more than one matching operator"
#define TXT_MULTIPLE_MATCHING_SIGNATURES_TO_s "Multiple matching signatures to '%s'"
#define TXT_MUST_BE_OBJECT                "Only objects have constructors"
#define TXT_s_MUST_BE_SENT_BY_REF         "'%s' must be sent by reference"
#define TXT_MUST_RETURN_VALUE             "Must return a value"

#define TXT_NAME_CONFLICT_s_CONSTANT        "Name conflict. '%s' is a constant."
#define TXT_NAME_CONFLICT_s_EXTENDED_TYPE   "Name conflict. '%s' is an extended data type."
#define TXT_NAME_CONFLICT_s_FUNCTION        "Name conflict. '%s' is a function."
#define TXT_NAME_CONFLICT_s_GLOBAL_VAR      "Name conflict. '%s' is a global variable."
#define TXT_NAME_CONFLICT_s_GLOBAL_PROPERTY "Name conflict. '%s' is a global property."
#define TXT_NAME_CONFLICT_s_STRUCT          "Name conflict. '%s' is a struct."
#define TXT_NAME_CONFLICT_s_OBJ_PROPERTY    "Name conflict. '%s' is an object property."
#define TXT_NAME_CONFLICT_s_OBJ_METHOD      "Name conflict. '%s' is an object method."
#define TXT_NAME_CONFLICT_s_SYSTEM_FUNCTION "Name conflict. '%s' is a system function."
#define TXT_NEED_TO_BE_A_HANDLE             "Need to be a handle"
#define TXT_NO_APPROPRIATE_INDEX_OPERATOR   "No appropriate indexing operator found"
#define TXT_NO_CONVERSION_s_TO_s            "No conversion from '%s' to '%s' available."
#define TXT_NO_CONVERSION_s_TO_MATH_TYPE    "No conversion from '%s' to math type available."
#define TXT_NO_DEFAULT_COPY_OP              "There is no copy operator for this type available."
#define TXT_NO_MATCHING_SIGNATURES_TO_s     "No matching signatures to '%s'"
#define TXT_NO_MATCHING_OP_FOUND_FOR_TYPE_s "No matching operator that takes the type '%s' found"
#define TXT_NOT_ALL_PATHS_RETURN            "Not all paths return a value"
#define TXT_s_NOT_AVAILABLE_FOR_s           "'%s' is not available for '%s'"
#define TXT_s_NOT_DECLARED                  "'%s' is not declared"
#define TXT_NOT_EXACT_g_d_g                 "Implicit conversion of value is not exact, %g -> %d -> %g."
#define TXT_NOT_EXACT_d_g_d                 "Implicit conversion of value is not exact, %d -> %g -> %d."
#define TXT_NOT_EXACT_g_u_g                 "Implicit conversion of value is not exact, %g -> %u -> %g."
#define TXT_NOT_EXACT_u_g_u                 "Implicit conversion of value is not exact, %u -> %g -> %u."
#define TXT_s_NOT_FUNCTION                  "Function '%s' not found"
#define TXT_s_NOT_INITIALIZED               "'%s' is not initialized."
#define TXT_s_NOT_MEMBER_OF_s               "'%s' is not a member of '%s'"
#define TXT_NOT_SUPPORTED_YET               "Not supported yet"
#define TXT_NOT_VALID_REFERENCE             "Not a valid reference"
#define TXT_NOT_VALID_LVALUE                "Not a valid lvalue"

#define TXT_OBJECT_DOESNT_SUPPORT_INDEX_OP "Type '%s' doesn't support the indexing operator"
#define TXT_OBJECT_DOESNT_SUPPORT_NEGATE_OP "Object doesn't have the negate operator"
#define TXT_OBJECT_HANDLE_NOT_SUPPORTED   "Object handle is not supported for this type"
#define TXT_ONLY_ONE_ARGUMENT_IN_CAST     "There is only one argument in a cast"

#define TXT_PARAMETER_ALREADY_DECLARED    "Parameter already declared"
#define TXT_PARAMETER_CANT_BE_s           "Parameter type can't be '%s'"
#define TXT_POSSIBLE_LOSS_OF_PRECISION    "Conversion from double to float, possible loss of precision"
#define TXT_PROPERTY_CANT_BE_CONST        "Struct properties cannot be declared as const"

#define TXT_REF_IS_READ_ONLY              "Reference is read-only"
#define TXT_REF_IS_TEMP                   "Reference is temporary"
#define TXT_RETURN_CANT_BE_s              "Return type can't be '%s'"

#define TXT_SIGNED_UNSIGNED_MISMATCH      "Signed/Unsigned mismatch"
#define TXT_STRINGS_NOT_RECOGNIZED        "Strings are not recognized by the application"
#define TXT_SWITCH_CASE_MUST_BE_CONSTANT  "Case expressions must be constants"
#define TXT_SWITCH_MUST_BE_INTEGRAL       "Switch expressions must be integral numbers"

#define TXT_TOO_MANY_ARRAY_DIMENSIONS        "Too many array dimensions"
#define TXT_TYPE_s_NOT_AVAILABLE_FOR_MODULE  "Type '%s' is not available for this module"

#define TXT_UNEXPECTED_END_OF_FILE        "Unexpected end of file"
#define TXT_UNEXPECTED_TOKEN_s            "Unexpected token '%s'"
#define TXT_UNINITIALIZED_GLOBAL_VAR_s    "Use of uninitialized global variable '%s'."
#define TXT_UNREACHABLE_CODE              "Unreachable code"
#define TXT_UNUSED_SCRIPT_NODE            "Unused script node"

#define TXT_VALUE_TOO_LARGE_FOR_TYPE      "Value is too large for data type"

// Engine message

#define TXT_INVALID_CONFIGURATION         "Invalid configuration\n"

// Message types

#define TXT_INFO                          "Info   "
#define TXT_ERROR                         "Error  "
#define TXT_WARNING                       "Warning"

// Internal names

#define TXT_EXECUTESTRING                 "ExecuteString"
#define TXT_PROPERTY                      "Property"
#define TXT_SYSTEM_FUNCTION               "System function"
#define TXT_VARIABLE_DECL                 "Variable declaration"

// Exceptions

#define TXT_STACK_OVERFLOW                "Stack overflow"
#define TXT_NULL_POINTER_ACCESS           "Null pointer access"
#define TXT_MISALIGNED_STACK_POINTER      "Misaligned stack pointer"
#define TXT_DIVIDE_BY_ZERO                "Divide by zero"
#define TXT_UNRECOGNIZED_BYTE_CODE        "Unrecognized byte code"
#define TXT_INVALID_CALLING_CONVENTION    "Invalid calling convention"
#define TXT_UNBOUND_FUNCTION              "Unbound function called"
#define TXT_OUT_OF_BOUNDS                 "Out of range"

#endif
