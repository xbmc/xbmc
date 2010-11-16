/**

\page doc_adv_var_type The variable parameter type



The application can register functions that take a reference to a variable
type, which means that the function can receive a reference to a variable of
any type. This is useful when making generic containers.


When a function is registered with this special parameter type, the
function will receive both the reference and an extra argument with the type
id of the variable type. The reference refers to the actual value that the
caller sent, i.e. if the expression is an object handle then the reference
will refer to the handle, not the actual object. 

\code
// An example usage with a native function
engine->RegisterGlobalFunction("void func_c(?&in)", asFUNCTION(func_c), asCALL_CDECL);

void func_c(void *ref, int typeId)
{
    // Do something with the reference

    // The type of the reference is determined through the type id
}

// An example usage with a generic function
engine->RegisterGlobalFunction("void func_g(?&in)", asFUNCTION(func_g), asCALL_GENERIC);

void func_g(asIScriptGeneric *gen)
{
    void *ref = gen->GetArgAddress(0);
    int typeId = gen->GetArgTypeId(0);

    func_c(ref, typeId);
}
\endcode

The variable type can also be used with <code>out</code> references, but not
with <code>inout</code> references. Currently it can only be used with global
functions, object constructors, and object methods. It cannot be used with
other behaviours and operators.

The variable type is not available within scripts, so it can only be used
to register application functions.

\see \ref doc_addon_any and \ref doc_addon_dict for examples


*/
