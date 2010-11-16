/**

\page doc_hello_world Your first script

Being an embedded scripting library there isn't much that AngelScript allows the scripts 
to do by themselves, so the first thing the application must do is to register the interface 
that the script will have to interact with the application. The interface may consist of 
functions, variables, and even complete classes.

\code
// Create the script engine
asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

// Set the message callback to receive information on errors in human readable form.
// It's recommended to do this right after the creation of the engine, because if
// some registration fails the engine may send valuable information to the message
// stream.
r = engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL); assert( r >= 0 );

// AngelScript doesn't have a built-in string type, as there is no definite standard 
// string type for C++ applications. Every developer is free to register it's own string type.
// The SDK do however provide a standard add-on for registering a string type, so it's not
// necessary to implement the registration yourself if you don't want to.
RegisterStdString(engine);

// Register the function that we want the scripts to call 
r = engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_CDECL); assert( r >= 0 );
\endcode

After the engine has been configured, the next step is to compile the scripts that should be executed.

The following is our script that will call the registered <tt>print</tt> function to write <tt>Hello world</tt> on the 
standard output stream. Let's say it's stored in the file <tt>test.as</tt>.

<pre>
  void main()
  {
    print("Hello world\n");
  }
</pre>

Here's the code for loading the script file and compiling it.

\code
// The CScriptBuilder helper is an add-on that loads the file,
// performs a pre-processing pass if necessary, and then tells
// the engine to build a script module.
CScriptBuilder builder;
r = builder.BuildScriptFromFile(engine, "MyModule", "test.as");
if( r < 0 )
{
  // An error occurred. Instruct the script writer to fix the 
  // compilation errors that were listed in the output stream.
  printf("Please correct the errors in the script and try again.\n");
  return;
}
\endcode

The last step is to identify the function that is to be called, and set up a context
for executing it.

\code
// Find the function that is to be called. 
asIScriptModule *mod = engine->GetModule("MyModule");
int funcId = mod->GetFunctionIdByDecl("void main()");
if( funcId < 0 )
{
  // The function couldn't be found. Instruct the script writer
  // to include the expected function in the script.
  printf("The script must have the function 'void main()'. Please add it and try again.\n");
  return;
}

// Create our context, prepare it, and then execute
asIScriptContext *ctx = engine->CreateContext();
ctx->Prepare(funcId);
r = ctx->Execute()
if( r != asEXECUTION_FINISHED )
{
  // The execution didn't complete as expected. Determine what happened.
  if( r == asEXECUTION_EXCEPTION )
  {
    // An exception occurred, let the script writer know what happened so it can be corrected.
    printf("An exception '%s' occurred. Please correct the code and try again.\n", ctx->GetExceptionString());
  }
}
\endcode

The exception handling above is very basic. The application may also obtain information about line number,
function, call stack, and even values of local and global variables if wanted.

Don't forget to clean up after you're done with the engine.

\code
// Clean up
ctx->Release();
engine->Release();
\endcode

\section doc_hello_world_1 Helper functions

The print function is implemented as a very simple wrapper on the printf function.

\code
// Print the script string to the standard output stream
void print(string &msg)
{
  printf("%s", msg.c_str());
}
\endcode

\see \ref doc_compile_script_msg, \ref doc_addon_build, and \ref doc_addon_std_string


*/
