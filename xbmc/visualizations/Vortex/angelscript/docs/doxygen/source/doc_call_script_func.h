/**

\page doc_call_script_func Calling a script function


\section doc_call_script_1 Preparing context and executing the function

Normally a script function is executed in a few steps:

<ol>
<li>Prepare the context
<li>Set the function arguments
<li>Execute the function
<li>Retrieve the return value
</ol>

The code for this might look something like this:

\code
// Get a script context instance. Usually you'll want to reuse a previously
// created instance to avoid the overhead of allocating the instance with
// each call.
asIScriptContext *ctx = engine->CreateContext();

// Obtain the function id from the module. This value should preferrably  
// be cached if the same function is called multiple times.
int funcId = engine->GetModule(module_name)->GetFunctionIdByDecl(function_declaration);

// Prepare() must be called to allow the context to prepare the stack
ctx->Prepare(funcId);

// Set the function arguments
ctx->SetArgDWord(...);

int r = ctx->Execute();
if( r == asEXECUTION_FINISHED )
{
  // The return value is only valid if the execution finished successfully
  asDWORD ret = ctx->GetReturnDWord();
}

// Release the context when you're done with it
ctx->Release();
\endcode

If your application allow the execution to be suspended, either by using
the callback function or registering a function that allow the script to 
manually suspend the execution, then the execution function may return 
before finishing with the return code asEXECUTION_SUSPENDED. In that case you
can later resume the execution by simply calling the execution function again.

Note that the return value retrieved with GetReturnValue() is only valid
if the script function returned successfully, i.e. if Execute() returned 
asEXECUTION_FINISHED.

\section doc_call_script_2 Passing and returning primitives

When calling script functions that take arguments, the values of these 
arguments must be set after the call to Prepare() and before Execute(). 
The arguments are set using a group of SetArg methods:

\code
int SetArgDWord(int arg, asDWORD value);
int SetArgQWord(int arg, asQWORD value);
int SetArgFloat(int arg, float value);
int SetArgDouble(int arg, double value);
\endcode

<code>arg</code> is the argument number, where the first argument is on 0, the second 
on 1, and so on. <code>value</code> is the value of the argument. What method to use is 
determined by the type of the parameter. For primitive types you can use any of these. 
If the parameter type is a reference to a primitive type it is recommended to use the 
SetArgDWord() method and pass the pointer as the value. For non-primitive types the 
method SetArgObject() should be used, which will be described in the next section.

\code
// The context has been prepared for a script 
// function with the following signature:
// int function(int, double, int&in)

// Put the arguments on the context stack, starting with the first one
ctx->SetArgDWord(0, 1);
ctx->SetArgDouble(1, 3.141592);
int val;
ctx->SetArgDWord(2, (asDWORD)&val);
\endcode

Once the script function has been executed the return value is retrieved in 
a similar way using the group of GetReturn methods:

\code
asDWORD GetReturnDWord();
asQWORD GetReturnQWord();
float   GetReturnFloat();
double  GetReturnDouble();
\endcode

Note that you must make sure the returned value is in fact valid, for 
example if the script function was interrupted by a script exception the value
would not be valid. You do this by verifying the return code from Execute() or
GetState(), where the return code should be asEXECUTION_FINISHED.

\section doc_call_script_3 Passing and returning objects

Passing registered object types to a script function is done in a similar
way to how primitive types are passed. The function to use is SetArgObject():

\code
int SetArgObject(int arg, void *object);
\endcode

<code>arg</code> is the argument number, just like the other SetArg methods. 
<code>object</code> is a pointer to the object you wish to pass.

This same method is used both for parameters passed by value and for those
passed by reference. The library will automatically make a copy of the object
if the parameter is defined to be passed by value.

\code
// The complex object we wish to pass to the script function
CObject obj;

// Pass the object to the function
ctx->SetArgObject(0, &obj);
\endcode

Getting an object returned by a script function is done in a similar way, using 
GetReturnObject(): 

\code
void *GetReturnObject();
\endcode

This method will return a pointer to the object returned by the script function.
The library will still hold a reference to the object, which will only be freed as 
the context is released.

\code
// The object where we want to store the return value
CObject obj;

// Execute the function
int r = ctx->Execute();
if( r == asEXECUTION_FINISHED )
{
  // Get a pointer to the returned object and copy it to our object
  obj = *(CObject*)ctx->GetReturnObject();
}
\endcode

It is important to make a copy of the returned object, or if it is managed by 
reference counting add a reference to it. If this is not done the pointer obtained 
with GetReturnObject() will be invalidated as the context is released, or reused for 
another script function call.

\section doc_call_script_4 Exception handling

If the script performs an illegal action, e.g. calling a method on a null handle, then
the script engine will throw a script exception. The virtual machine will then abort
the execution and the \ref asIScriptContext::Execute "Execute" method will return with the value
\ref asEXECUTION_EXCEPTION.

At this time it is possible to obtain information about the exception through the 
\ref asIScriptContext's methods. Example:

\code
void PrintExceptionInfo(asIScriptContext *ctx)
{
  asIScriptEngine *engine = ctx->GetEngine();

  // Determine the exception that occurred
  printf("desc: %s\n", ctx->GetExceptionString());

  // Determine the function where the exception occurred
  int funcId = ctx->GetExceptionFunction();
  const asIScriptFunction *function = engine->GetFunctionDescriptorById(funcId);
  printf("func: %s\n", function->GetDeclaration());
  printf("modl: %s\n", function->GetModuleName());
  printf("sect: %s\n", function->GetScriptSectionName());
  
  // Determine the line number where the exception occurred
  printf("line: %d\n", ctx->GetExceptionLineNumber());
}
\endcode

If desired, it is also possible to \ref asIScriptContext::SetExceptionCallback "register a callback function" 
that will be called at the moment the exception occurred, before the \ref asIScriptContext::Execute "Execute" method returns. 

\see \ref doc_debug for information on examining the callstack


*/
