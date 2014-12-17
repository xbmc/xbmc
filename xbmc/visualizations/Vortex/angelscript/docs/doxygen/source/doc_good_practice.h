/**

\page doc_good_practice Good practices


This article will try to explain some good practices, that will help you get going faster and easier find the solution when a problem occurs.

\section doc_checkretval Always check return values for registrations


When configuring the script engine you should always check the return values, at least in debug mode. 
All error codes are negative so a simple <code>assert( r >= 0 )</code> where r is the returned value is sufficient 
to pinpoint where the configuration failed.


If a function failed during the configuration, the \ref asIScriptModule::Build "Build" method will always fail with a return code 
of \ref asINVALID_CONFIGURATION. Unless you already verified the error codes for all the configuration 
calls, it will not be possible to determine what the error was.

\code
// Verifying the return code with an assert is simple and won't pollute the code
r = engine->RegisterGlobalFunction("void func()", asFUNCTION(func), asCALL_CDECL); assert( r >= 0 );
\endcode

<code>assert()</code> can safely be used with engine registrations, since the engine will set the internal state 
to invalid configuration if a function fails. Even in release mode the failure is discovered when 
a script is built.

\section doc_usemsgcallbck Use the message callback to receive detailed error messages

The return code from the register functions, \ref asIScriptModule::Build "Build", and 
\ref asIScriptModule::CompileFunction "CompileFunction", can only tell you that something was wrong, 
not what it was. To help identify the exact problem the message callback should be used. The script 
library will then send messages explaining the error or warning in clear text.

See \ref doc_compile_script_msg for more information on the message callback.





*/
