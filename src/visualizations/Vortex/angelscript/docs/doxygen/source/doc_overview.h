/**

\page doc_overview Overview

AngelScript is structured around an \ref asIScriptEngine "engine" where the application should
\ref doc_register_api "register" the \ref doc_register_func "functions", \ref doc_register_prop "properties", and even \ref doc_register_type "types", 
that the scripts will be able to use. The scripts are then compiled into \ref asIScriptModule "modules",
where the application may have one or more modules, depending on the need of the application. The application can also expose
a different interface to each module through the use of \ref asIScriptEngine::BeginConfigGroup "configuration groups". 
This is especially useful when the application works with multiple types of scripts, e.g. GUI, AI control, etc.

Each \ref asIScriptModule "module" has it's own scope of script functions, global variables, and classes. These
are normally not shared between modules, even though multiple modules may be built from the same source scripts.
However modules may interact with each others through \ref asIScriptModule::BindImportedFunction "function binding".

As the scripts are compiled into bytecode AngelScript also provides a virtual machine, also known 
as a \ref asIScriptContext "script context", for \ref doc_call_script_func "executing" the bytecode. The application can have any number of 
script context at the same time, though most applications will probably only need one. The contexts support
\ref asIScriptContext::Suspend "suspending" the execution and then resuming it, so the application can easily 
implement features such as \ref doc_adv_concurrent "concurrent scripts" and \ref doc_adv_coroutine "co-routines". 
The script context also provides an interface for extracting run-time information, useful for \ref doc_debug "debugging" scripts.

The \ref doc_script "script language" is based on the well known syntax of C++ and more modern languages such as Java, C#, and D. 
Anyone with some knowledge of those languages, or other script languages with similar syntax, such as Javascript and ActionScript, 
should feel right at home with AngelScript. Contrary to most script languages, AngelScript is a strongly typed language, which 
permits faster execution of the code and smoother interaction with the host application as there will be less need for runtime 
evaluation of the true type of values. 

The \ref doc_memory "memory management" in AngelScript is based on reference counting with an incremental \ref doc_gc "garbage collector" 
for detecting and freeing objects with circular references. This provides for a controlled environment without application freezes as the 
garbage collector steps in to free up memory.


*/
