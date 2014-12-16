/**

\page doc_compile_script Compiling scripts

After \ref doc_register_api "registering the application interface" it's time to 
compile the scripts that will be executed.

\section doc_compile_script_msg Message callback

Before starting the compilation, remember to have the message callback set in the
engine so that you can get more information on compilation errors than just an
error code. In fact, it is recommended to set the message callback right after
creating the script engine, as the message callback may even be helpful while 
registering the application interface.

The message callback has been designed so that it doesn't output anything if there 
are no errors or warnings, so when everything is ok, you shouldn't get anything from
it. But if the \ref asIScriptModule::Build "Build" method returns an error, the message 
callback will have received detailed information about the error.

If desired, the application may send its own messages to the callback via the 
\ref asIScriptEngine::WriteMessage "WriteMessage" method on the engine.

\code
// Implement a simple message callback function
void MessageCallback(const asSMessageInfo *msg, void *param)
{
  const char *type = "ERR ";
  if( msg->type == asMSGTYPE_WARNING ) 
    type = "WARN";
  else if( msg->type == asMSGTYPE_INFORMATION ) 
    type = "INFO";
  printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
}

// Set the message callback when creating the engine
asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);
\endcode

\section doc_compile_script_load Loading and compiling scripts

To build a script module you first obtain a module from the engine, then add 
the script sections, and finally compile them. A compiled script module may be 
composed of one or more script sections, so your application may store each 
section in different files, or even generate them dynamically. It doesn't matter
in which order the script sections are added to the module, as the compiler
is able to resolve all names regardless of where they are declared in the script.

\code
// Create a new script module
asIScriptModule *mod = engine->GetModule("module", asGM_ALWAYS_CREATE);

// Load and add the script sections to the module
string script;
LoadScriptFile("script.as", script);
mod->AddScriptSection("script.as", script.c_str());

// Build the module
int r = mod->Build();
if( r < 0 )
{
  // The build failed. The message stream will have received  
  // compiler errors that shows what needs to be fixed
}
\endcode

AngelScript doesn't provide built-in functions for loading script files as most
applications have their own way of loading files. However, it is quite easy to write
your own routines for loading script files, for example:

\code
// Load the entire script file into a string buffer
void LoadScriptFile(const char *fileName, string &script)
{
  // Open the file in binary mode
  FILE *f = fopen("test.as", "rb");
  
  // Determine the size of the file
  fseek(f, 0, SEEK_END);
  int len = ftell(f);
  fseek(f, 0, SEEK_SET);
  
  // Load the entire file in one call
  script.resize(len);
  fread(&script[0], len, 1, f);
  
  fclose(f);
} 
\endcode

As AngelScript doesn't load the files itself, it also doesn't have built-in support 
for including other files from within the script. However, if you look in the add-on
directory, you'll find a \ref doc_addon_build "CScriptBuilder" class that provides
this support and more. It is a helper class for loading files, perform a pre-processing pass, 
and then building the module. With it the code for building a module looks like this:

\code
CScriptBuilder builder;
int r = builder.BuildScriptFromFile(engine, "module", "script.as");
if( r < 0 )
{
  // The build failed. The message stream will have received  
  // compiler errors that shows what needs to be fixed
}
\endcode

\see \ref doc_adv_precompile



*/
