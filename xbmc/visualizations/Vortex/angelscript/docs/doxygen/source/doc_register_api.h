/**

\page doc_register_api Registering the application interface

AngelScript requires the application developer to register the interface
that the scripts should use to interact with anything outside the script itself.

It's possible to register \ref doc_register_func "global functions" and
\ref doc_register_prop "global properties" that can be used directly by the 
scripts.

For more complex scripts it may be useful to register new \ref doc_register_type "object types" 
to complement the built-in data types. 

AngelScript doesn't have a \ref doc_strings "built-in string type" as there is no de-facto standard for string types 
in C++. Instead AngelScript permits the application to register its own preferred string type, and 
a \ref asIScriptEngine::RegisterStringFactory "string factory" that the script engine will use
to instanciate the strings.

\ref asIScriptEngine::RegisterInterface "Class interfaces" can be registered if you want 
to guarantee that script classes implement a specific set of class methods. Interfaces can 
be easier to use when working with script classes from the application, but they are not 
necessary as the application can easily enumerate available methods and properties even
without the interfaces.

\ref asIScriptEngine::RegisterEnum "enumeration types" and 
\ref asIScriptEngine::RegisterTypedef "typedefs" can also be registered to improve readability of the scripts.

\section doc_register_api_1 Topics

 - \subpage doc_register_func
 - \subpage doc_register_prop
 - \subpage doc_register_type
 - \subpage doc_generic
 - \subpage doc_advanced_api


\page doc_advanced_api Advanced application interface

 - \subpage doc_gc_object
 - \subpage doc_adv_scoped_type
 - \subpage doc_adv_single_ref_type
 - \subpage doc_adv_class_hierarchy
 - \subpage doc_adv_var_type
 - \subpage doc_adv_template
 


*/
