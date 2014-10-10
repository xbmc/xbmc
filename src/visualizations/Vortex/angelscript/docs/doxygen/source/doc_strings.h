/**

\page doc_strings The string type

Almost all applications have some need to manipulate text strings in one way or another. However
most applications have very different needs, and also different ways of representing their string
types. For that reason, AngelScript doesn't come with its own built-in string type that the application
would be forced to adapt to, instead AngelScript allows the application to \ref doc_register_api "register its own string type".

This article presents the options for customizing the script language to the application's needs with regards 
to strings. If you do not want, or do not need to have AngelScript use your own string type, then I suggest
you use one of the standard string add-ons, either the \ref doc_addon_std_string "std::string registration", or the \ref 
doc_addon_string "reference counted string type".

\section doc_strings_1 Unicode vs ASCII

Is you're application using Unicode or plain ASCII for the text? If you use Unicode, then you'll want to encode the scripts
in UTF-8, which the AngelScript compiler supports natively. By default AngelScript expects the scripts to have been encoded 
in UTF-8, but should you prefer ASCII you can turn this off by setting the engine property \ref asEP_SCRIPT_SCANNER to 0 right
after creating the engine.

\code
// Set engine to use ASCII scanner for script code
engine->SetEngineProperty(asEP_SCRIPT_SCANNER, 0);
\endcode

If you do use Unicode, then you'll also want to choose the desired encoding for the string literals, either UTF-8 or UTF-16. By
default the string literals in AngelScript are encoded with UTF-8, but if you're application is better prepared for UTF-16 then
you'll want to change this by setting the engine property \ref asEP_STRING_ENCODING to 1 before compiling your scripts.

\code
// Set engine to use UTF-16 encoding for string literals
engine->SetEngineProperty(asEP_STRING_ENCODING, 1);
\endcode

Observe that the \ref asIScriptEngine::RegisterStringFactory "string factory" called by the engine to create new strings gives the 
size of the string data in bytes even for UTF-16 encoded strings, so you'll need to divide the size by two to get the number of 
characters in the string.

\section doc_string_2 Multiline string literals

There is also a couple of options that affect the script language itself a bit. If you like the convenience of allowing string
literals to span multiple lines of code, then you can turn this on by setting the engine property \ref asEP_ALLOW_MULTILINE_STRINGS
to true. Without this the compiler will give an error if it encounters a line break before the end of the string.

\code
// Set engine to allow string literals with line breaks
engine->SetEngineProperty(asEP_ALLOW_MULTILINE_STRINGS, true);
\endcode

Observe that the line ending encoding of the source file will not be modified by the script compiler, so depending on how the file
has been saved, you may get strings using different line endings.

The \ref doc_datatypes_strings "heredoc strings" are not affected by this setting, as they are designed to support multiline text sections.

\section doc_string_3 Character literals

By default AngelScript doesn't have character literals as C and C++ does. A string literal can be written with double quotes or single
quotes and still have the same meaning. This makes it convenient to embed scripts in XML files or C/C++ source files where double
quotes would otherwise end the script code.

If you want to have single quoted literals mean a single character literal instead of a string, then you can do so by setting the 
engine property \ref asEP_USE_CHARACTER_LITERALS to true. The compiler will then convert single quoted literals to an integer number
representing the first character.

\code
// Set engine to use character literals 
engine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, true);
\endcode

Observe that the \ref asEP_SCRIPT_SCANNER property has great importance in this case, as an ASCII character can only represent values
between 0 and 255, whereas a Unicode character can represent values between 0 and 1,114,111.


*/
