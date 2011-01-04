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
// as_texts.h
//
// These are text strings used through out the library
//


#ifndef AS_TEXTS_H
#define AS_TEXTS_H

// Compiler messages

#define TXT_s_ALREADY_DECLARED            "'%s' already declared"
#define TXT_ARG_NOT_LVALUE                "Argument cannot be assigned. Output will be discarded."
#define TXT_ASSIGN_IN_GLOBAL_EXPR         "Assignments are not allowed in global expressions"

#define TXT_BOTH_MUST_BE_SAME                     "Both expressions must have the same type"
#define TXT_BOTH_CONDITIONS_MUST_CALL_CONSTRUCTOR "Both conditions must call constructor"

#define TXT_CALLING_NONCONST_METHOD_ON_TEMP      "A non-const method is called on temporary object. Changes to the object may be lost."
#define TXT_CANDIDATES_ARE                       "Candidates are:"
#define TXT_CANNOT_CALL_CONSTRUCTOR_IN_LOOPS     "Can't call a constructor in loops"
#define TXT_CANNOT_CALL_CONSTRUCTOR_IN_SWITCH    "Can't call a constructor in switch"
#define TXT_CANNOT_CALL_CONSTRUCTOR_TWICE        "Can't call a constructor multiple times"
#define TXT_CANNOT_INHERIT_FROM_s                "Can't inherit from '%s'"
#define TXT_CANNOT_INHERIT_FROM_MULTIPLE_CLASSES "Can't inherit from multiple classes"
#define TXT_CANNOT_INHERIT_FROM_SELF             "Can't inherit from itself, or another class that inherits from this class"
#define TXT_CANNOT_INSTANCIATE_TEMPLATE_s_WITH_s "Can't instanciate template '%s' with subtype '%s'"
#define TXT_CANT_IMPLICITLY_CONVERT_s_TO_s       "Can't implicitly convert from '%s' to '%s'."
#define TXT_CANT_RETURN_VALUE                    "Can't return value when return type is 'void'"
#define TXT_CHANGE_SIGN                          "Implicit conversion changed sign of value"
#define TXT_COMPILING_s                          "Compiling %s"
#define TXT_COMPOUND_ASGN_WITH_PROP              "Compound assignments with property accessors are not allowed"
#define TXT_CONSTRUCTOR_NAME_ERROR               "The constructor name must be the same as the class"

#define TXT_DATA_TYPE_CANT_BE_s           "Data type can't be '%s'"
#define TXT_DEFAULT_MUST_BE_LAST          "The default case must be the last one"
#define TXT_DESTRUCTOR_MAY_NOT_HAVE_PARM  "The destructor must not have any parameters"
#define TXT_DUPLICATE_SWITCH_CASE         "Duplicate switch case"

#define TXT_ELSE_WITH_EMPTY_STATEMENT     "Else with empty statement"
#define TXT_EMPTY_SWITCH                  "Empty switch statement"
#define TXT_EXPECTED_s                    "Expected '%s'"
#define TXT_EXPECTED_CONSTANT             "Expected constant"
#define TXT_EXPECTED_DATA_TYPE            "Expected data type"
#define TXT_EXPECTED_EXPRESSION_VALUE     "Expected expression value"
#define TXT_EXPECTED_IDENTIFIER           "Expected identifier"
#define TXT_EXPECTED_METHOD_OR_PROPERTY   "Expected method or property"
#define TXT_EXPECTED_ONE_OF               "Expected one of: "
#define TXT_EXPECTED_OPERATOR             "Expected operator"
#define TXT_EXPECTED_s_OR_s               "Expected '%s' or '%s'"
#define TXT_EXPECTED_POST_OPERATOR        "Expected post operator"
#define TXT_EXPECTED_PRE_OPERATOR         "Expected pre operator"
#define TXT_EXPECTED_STRING               "Expected string"
#define TXT_EXPR_MUST_BE_BOOL             "Expression must be of boolean type"

#define TXT_FOUND_MULTIPLE_ENUM_VALUES    "Found multiple matching enum values"
#define TXT_FUNCTION_IN_GLOBAL_EXPR       "Function calls are not allowed in global expressions"
#define TXT_FUNCTION_ALREADY_EXIST        "A function with the same name and parameters already exist"
#define TXT_FUNCTION_s_NOT_FOUND          "Function '%s' not found"

#define TXT_GET_SET_ACCESSOR_TYPE_MISMATCH_FOR_s "The property '%s' has mismatching types for the get and set accessors"

#define TXT_HANDLE_COMPARISON             "The operand is implicitly converted to handle in order to compare them"

#define TXT_IDENTIFIER_s_NOT_DATA_TYPE          "Identifier '%s' is not a data type"
#define TXT_IF_WITH_EMPTY_STATEMENT             "If with empty statement"
#define TXT_ILLEGAL_MEMBER_TYPE                 "Illegal member type"
// TODO: Should be TXT_ILLEGAL_OPERATION_ON_s
#define TXT_ILLEGAL_OPERATION                   "Illegal operation on this datatype"
#define TXT_ILLEGAL_OPERATION_ON_s              "Illegal operation on '%s'"
#define TXT_ILLEGAL_TARGET_TYPE_FOR_REF_CAST    "Illegal target type for reference cast"
#define TXT_ILLEGAL_VARIABLE_NAME_s             "Illegal variable name '%s'."
#define TXT_INC_OP_IN_GLOBAL_EXPR               "Incremental operators are not allowed in global expressions"
#define TXT_INIT_LIST_CANNOT_BE_USED_WITH_s     "Initialization lists cannot be used with '%s'"
#define TXT_INTERFACE_s_ALREADY_IMPLEMENTED     "The interface '%s' is already implemented"
#define TXT_INVALID_BREAK                       "Invalid 'break'"
#define TXT_INVALID_CHAR_LITERAL                "Invalid character literal"
#define TXT_INVALID_CONTINUE                    "Invalid 'continue'"
#define TXT_INVALID_ESCAPE_SEQUENCE             "Invalid escape sequence"
#define TXT_INVALID_SCOPE                       "Invalid scope resolution"
#define TXT_INVALID_TYPE                        "Invalid type"
#define TXT_INVALID_UNICODE_FORMAT_EXPECTED_d   "Invalid unicode escape sequence, expected %d hex digits"
#define TXT_INVALID_UNICODE_VALUE               "Invalid unicode code point"
#define TXT_INVALID_UNICODE_SEQUENCE_IN_SRC     "Invalid unicode sequence in source"

#define TXT_METHOD_IN_GLOBAL_EXPR                   "Object method calls are not allowed in global expressions"
#define TXT_MISSING_IMPLEMENTATION_OF_s             "Missing implementation of '%s'"
#define TXT_MORE_THAN_ONE_MATCHING_OP               "Found more than one matching operator"
#define TXT_MULTIPLE_MATCHING_SIGNATURES_TO_s       "Multiple matching signatures to '%s'"
#define TXT_MULTIPLE_PROP_GET_ACCESSOR_FOR_s        "Found multiple get accessors for property '%s'"
#define TXT_MULTIPLE_PROP_SET_ACCESSOR_FOR_s        "Found multiple set accessors for property '%s'"
#define TXT_MULTILINE_STRINGS_NOT_ALLOWED           "Multiline strings are not allowed in this application"
#define TXT_MUST_BE_OBJECT                          "Only objects have constructors"
#define TXT_MUST_RETURN_VALUE                       "Must return a value"

#define TXT_NAME_CONFLICT_s_EXTENDED_TYPE          "Name conflict. '%s' is an extended data type."
#define TXT_NAME_CONFLICT_s_GLOBAL_PROPERTY        "Name conflict. '%s' is a global property."
#define TXT_NAME_CONFLICT_s_IS_NAMED_TYPE          "Name conflict. '%s' is a named type."
#define TXT_NAME_CONFLICT_s_STRUCT                 "Name conflict. '%s' is a class."
#define TXT_NAME_CONFLICT_s_OBJ_PROPERTY           "Name conflict. '%s' is an object property."
#define TXT_NO_APPROPRIATE_INDEX_OPERATOR          "No appropriate indexing operator found"
#define TXT_NO_CONVERSION_s_TO_s                   "No conversion from '%s' to '%s' available."
#define TXT_NO_CONVERSION_s_TO_MATH_TYPE           "No conversion from '%s' to math type available."
#define TXT_NO_DEFAULT_CONSTRUCTOR_FOR_s           "No default constructor for object of type '%s'."
#define TXT_NO_DEFAULT_COPY_OP                     "There is no copy operator for this type available."
#define TXT_NO_MATCHING_SIGNATURES_TO_s            "No matching signatures to '%s'"
#define TXT_NO_MATCHING_OP_FOUND_FOR_TYPE_s        "No matching operator that takes the type '%s' found"
#define TXT_NO_MATCHING_OP_FOUND_FOR_TYPES_s_AND_s "No matching operator that takes the types '%s' and '%s' found"
#define TXT_NON_CONST_METHOD_ON_CONST_OBJ          "Non-const method call on read-only object reference"
#define TXT_NOT_ALL_PATHS_RETURN                   "Not all paths return a value"
#define TXT_s_NOT_DECLARED                         "'%s' is not declared"
#define TXT_NOT_EXACT                              "Implicit conversion of value is not exact"
#define TXT_s_NOT_INITIALIZED                      "'%s' is not initialized."
#define TXT_s_NOT_MEMBER_OF_s                      "'%s' is not a member of '%s'"
#define TXT_NOT_VALID_REFERENCE                    "Not a valid reference"
#define TXT_NOT_VALID_LVALUE                       "Not a valid lvalue"

#define TXT_OBJECT_DOESNT_SUPPORT_INDEX_OP "Type '%s' doesn't support the indexing operator"
#define TXT_OBJECT_HANDLE_NOT_SUPPORTED    "Object handle is not supported for this type"
#define TXT_ONLY_OBJECTS_MAY_USE_REF_INOUT "Only object types that support object handles can use &inout. Use &in or &out instead"
#define TXT_ONLY_ONE_ARGUMENT_IN_CAST      "A cast operator has one argument"
#define TXT_ONLY_ONE_FUNCTION_ALLOWED      "The code must contain one and only one function"
#define TXT_ONLY_ONE_VARIABLE_ALLOWED      "The code must contain one and only one global variable"

#define TXT_PARAMETER_ALREADY_DECLARED    "Parameter already declared"
#define TXT_PARAMETER_CANT_BE_s           "Parameter type can't be '%s'"
#define TXT_POSSIBLE_LOSS_OF_PRECISION    "Conversion from double to float, possible loss of precision"
#define TXT_PROPERTY_CANT_BE_CONST        "Class properties cannot be declared as const"
#define TXT_PROPERTY_HAS_NO_GET_ACCESSOR  "The property has no get accessor"
#define TXT_PROPERTY_HAS_NO_SET_ACCESSOR  "The property has no set accessor"

#define TXT_REF_IS_READ_ONLY              "Reference is read-only"
#define TXT_REF_IS_TEMP                   "Reference is temporary"
#define TXT_RETURN_CANT_BE_s              "Return type can't be '%s'"

#define TXT_SCRIPT_FUNCTIONS_DOESNT_SUPPORT_RETURN_REF "Script functions must not return references"
#define TXT_SIGNED_UNSIGNED_MISMATCH                   "Signed/Unsigned mismatch"
#define TXT_STRINGS_NOT_RECOGNIZED                     "Strings are not recognized by the application"
#define TXT_SWITCH_CASE_MUST_BE_CONSTANT               "Case expressions must be constants"
#define TXT_SWITCH_MUST_BE_INTEGRAL                    "Switch expressions must be integral numbers"

#define TXT_TOO_MANY_ARRAY_DIMENSIONS        "Too many array dimensions"
#define TXT_TYPE_s_NOT_AVAILABLE_FOR_MODULE  "Type '%s' is not available for this module"

#define TXT_UNEXPECTED_END_OF_FILE        "Unexpected end of file"
#define TXT_UNEXPECTED_TOKEN_s            "Unexpected token '%s'"
#define TXT_UNINITIALIZED_GLOBAL_VAR_s    "Use of uninitialized global variable '%s'."
#define TXT_UNREACHABLE_CODE              "Unreachable code"
#define TXT_UNUSED_SCRIPT_NODE            "Unused script node"

#define TXT_VALUE_TOO_LARGE_FOR_TYPE      "Value is too large for data type"

// Engine message

#define TXT_INVALID_CONFIGURATION                  "Invalid configuration"
#define TXT_VALUE_TYPE_MUST_HAVE_SIZE              "A value type must be registered with a non-zero size"
#define TXT_TYPE_s_IS_MISSING_BEHAVIOURS           "Type '%s' is missing behaviours"
#define TXT_ILLEGAL_BEHAVIOUR_FOR_TYPE             "The behaviour is not compatible with the type"
#define TXT_GC_REQUIRE_ADD_REL_GC_BEHAVIOUR        "A garbage collected type must have the addref, release, and all gc behaviours"
#define TXT_SCOPE_REQUIRE_REL_BEHAVIOUR            "A scoped reference type must have the release behaviour"
#define TXT_REF_REQUIRE_ADD_REL_BEHAVIOUR          "A reference type must have the addref and release behaviours"
#define TXT_NON_POD_REQUIRE_CONSTR_DESTR_BEHAVIOUR "A non-pod value type must have the constructor and destructor behaviours"
#define TXT_CANNOT_PASS_TYPE_s_BY_VAL              "Can't pass type '%s' by value unless the application type is informed in the registration"
#define TXT_CANNOT_RET_TYPE_s_BY_VAL               "Can't return type '%s' by value unless the application type is informed in the registration"

// Internal names

#ifdef AS_DEPRECATED
// Deprecated since 2.18.0, 2009-12-08
#define TXT_EXECUTESTRING                 "ExecuteString"
#endif
#define TXT_PROPERTY                      "Property"
#define TXT_SYSTEM_FUNCTION               "System function"
#define TXT_VARIABLE_DECL                 "Variable declaration"

// Exceptions

#define TXT_STACK_OVERFLOW                "Stack overflow"
#define TXT_NULL_POINTER_ACCESS           "Null pointer access"
#define TXT_DIVIDE_BY_ZERO                "Divide by zero"
#define TXT_UNRECOGNIZED_BYTE_CODE        "Unrecognized byte code"
#define TXT_INVALID_CALLING_CONVENTION    "Invalid calling convention"
#define TXT_UNBOUND_FUNCTION              "Unbound function called"
#define TXT_OUT_OF_BOUNDS                 "Out of range"

#endif
