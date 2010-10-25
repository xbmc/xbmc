/**

\page doc_adv_single_ref_type Registering a single-reference type

A variant of the uninstanciable reference types is the single-reference
type. This is a type that have only 1 reference accessing it, i.e. the script
cannot store any extra references to the object during execution. The script
is forced to use the reference it receives from the application at the moment
the application passes it on to the script.

The reference can be passed to the script through a property, either global
or a class member, or it can be returned from an application registered
function or class method.

The script engine will not permit declaration of functions that take this
type as a parameter, neither as a reference nor as a handle. If that was allowed
it would mean that a reference to the instance is placed on the stack, which 
in turn means that it is no longer a single-reference type.

\code
// Registering the type so that it cannot be instanciated
// by the script, nor allow scripts to store references to the type
r = engine->RegisterObjectType("single", 0, asOBJ_REF | asOBJ_NOHANDLE); assert( r >= 0 );
\endcode

This sort of type is most useful when you want to have complete control over
references to an object, for example so that the application can destroy and
recreate objects of the type without having to worry about potential references
held by scripts. This allows the application to control when a script has access
to an object and it's members.


\see \ref doc_reg_basicref



*/
