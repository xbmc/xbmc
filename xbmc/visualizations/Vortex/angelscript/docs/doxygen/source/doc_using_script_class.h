/**

\page doc_use_script_class Using script classes 

When there are multiple objects controlled by the same script implementation it may be favourable to use script classes,
rather than global script functions. Using script classes each instance can have it's own set of variables within the 
class, contrary to the global functions that needs to rely on global variables to store persistent information. 

Of course, it would be possible to duplicate the script modules, so that there is one module for each object instance, but
that would be impose a rather big overhead for the application. Script classes don't have that overhead, as all instances
share the same module, and thus the same bytecode and function ids, etc. 

\section doc_use_script_class_1 Instanciating the script class

Before instanciating the script class you need to know which class to instanciate. Exactly how this is done 
depends on the application, but here are some suggestions.

If the application knows the name of the class, either hardcoded or from some configuration, the class type
can easily be obtained by calling the module's \ref asIScriptModule::GetTypeIdByDecl "GetTypeIdByDecl" with the name
of the class. The application can also choose to identify the class through some properties of the class, e.g.
if the class implements a predefined \ref asIScriptEngine::RegisterInterface "interface". Then the application 
can enumerate the class types implemented in the script with \ref asIScriptModule::GetObjectTypeByIndex "GetObjectTypeByIndex"
and then examine the type through the \ref asIObjectType interface.

A third option, if you're using the \ref doc_addon_build "script builder add-on", is to use the metadata to identify
the class. If you choose this option, use the \ref asIScriptModule to enumerate the declared types and then query the
\ref doc_addon_build "CScriptBuilder" for their metadata.

Once the object type is known you create the instance by calling the class' factory function, passing it the necessary
arguments, e.g. a pointer to the application object which the script class should be bound to. The factory function id
is found by querying the \ref asIObjectType. 

\code
// Get the object type
asIScriptModule *module = engine->GetModule("MyModule");
asIObjectType *type = engine->GetObjectTypeById(module->GetTypeIdByDecl("MyClass"));

// Get the factory function id from the object type
int factoryId = type->GetFactoryIdByDecl("MyClass @MyClass(int)");
\endcode

The factory function is \ref doc_call_script_func "called as a regular global function" and returns a handle to 
the newly instanciated class.

\section doc_use_script_class_2 Calling a method on the script class

Calling the methods of the script classes are similar to \ref doc_call_script_func "calling global functions" except
that you obtain the function id from the \ref asIObjectType, and you must set the object pointer along with the 
rest of the function arguments.

\code
// Obtain the id of the class method
int funcId = type->GetMethodIdByDecl("void method()");

// Prepare the context for calling the method
ctx->Prepare(funcId);

// Set the object pointer
ctx->SetObject(obj);

// Execute the call
ctx->Execute();
\endcode

*/
