# Code Guidelines

![Kodi Logo](https://github.com/xbmc/xbmc/raw/master/docs/resources/banner_slim.png)

## Table of Contents
* [1. Motivation](#1-motivation)
* [2. Language standard](#2-language-standard)
* [3. Formatting](#3-formatting)
  * [3.1. Braces](#31-braces)
  * [3.2. Indentation](#32-indentation)
  * [3.3. Control statements](#33-control-statements)
    * [3.3.1. if else](#331-if-else)
    * [3.3.2. switch case](#332-switch-case)
    * [3.3.3. try catch](#333-try-catch)
  * [3.4. Whitespace](#34-whitespace)
  * [3.5. Vertical alignment](#35-vertical-alignment)
* [4. Statements](#4-statements)
* [5. Declarations](#5-declarations)
  * [5.1. Declarations](#51-declarations)
  * [5.2. Multiple declarations](#52-multiple-declarations)
  * [5.3. Pointer and reference types](#53-pointer-and-reference-types)
  * [5.4. const and other modifiers](#54-const-and-other-modifiers)
  * [5.5. Initialization](#55-initialization)
* [6. Scoping](#6-scoping)
  * [6.1. Namespaces](#61-namespaces)
  * [6.2. Local functions](#62-local-functions)
* [7. Headers](#7-headers)
  * [7.1. Header order](#71-header-order)
  * [7.2. Use C++ wrappers for C headers](#72-use-c-wrappers-for-c-headers) 
* [8. Naming](#8-naming)
  * [8.1. Namespaces](#81-namespaces)
  * [8.2. Constants](#82-constants)
  * [8.3. Enums](#83-enums)
  * [8.4. Interfaces](#84-interfaces)
  * [8.5. Classes](#85-classes)
  * [8.6. Structs](#86-structs)
  * [8.7. Methods](#87-methods)
  * [8.8. Variables](#88-variables)
    * [Member variables](#member-variables)
    * [Global variables](#global-variables)
* [9. Comments](#9-comments)
  * [9.1. General](#91-general)
  * [9.2. Doxygen](#92-doxygen)
* [10. Logging](#10-logging)
* [11. Classes](#11-classes)
  * [11.1. Member visibility](#111-member-visibility)
  * [11.2. Const correctness](#112-const-correctness)
  * [11.3. Overriding virtual functions](#113-overriding-virtual-functions)
  * [11.4. Default member initialization](#114-default-member-initialization)
  * [11.5. Destructors in interfaces](#115-destructors-in-interfaces)
  * [11.6. Constructor Initialization Lists](#116-constructor-initialization-lists)
* [12. Other conventions](#12-other-conventions)
  * [12.1. Output parameters](#121-output-parameters)
  * [12.2. Casts](#122-casts)
  * [12.3. `NULL` vs `nullptr`](#123-null-vs-nullptr)
  * [12.4. auto](#124-auto)
  * [12.5. `for` loops](#125-for-loops)
  * [12.6. Include guards](#126-include-guards)
  * [12.7. Type aliases](#127-type-aliases)
  * [12.8. `goto`](#128-goto)
  * [12.9. Macros](#129-macros)
  * [12.10. constexpr](#1210-constexpr)
  * [12.11. `std::string` vs `std::string_view`](#1211-stdstring-vs-stdstring_view)

## 1. Motivation
When working in a large group, the two most important values are readability and maintainability. We code for other people, not computers. To accomplish these goals, we have created a unified set of code conventions.

In the repository root directory, there is a [`.clang-format`](https://github.com/xbmc/xbmc/blob/master/.clang-format) file that implements the rules as specified here. You are encouraged to run [`clang-format`](https://clang.llvm.org/docs/ClangFormat.html) on any newly created files. It is currently not recommended to do so on preexisting files because all the formatting changes will clutter your commits and pull request.

When you create a pull request, the PR build job will run `clang-format` on your commits and provide patches for any parts that don't satisfy the current `.clang-format` rules. You should apply these patches and amend your pull request accordingly.

The coding guidelines should be met by every code change, be it editing existing code, adding new code to existing source files, or adding completely new source files. For changes in existing files, at least the changed code needs to pass the clang-format check.

Conventions can be bent or broken in the interest of making code more readable and maintainable. However, if you submit a patch that contains excessive style conflicts, you may be asked to improve your code before your pull request is reviewed.

**[back to top](#table-of-contents)**

## 2. Language standard

We currently target the C++17 language standard. Do use C++17 features when possible (and supported by all target platforms). Do not use C++20 features.

**[back to top](#table-of-contents)**

## 3. Formatting

### Line length
The `ColumnLimit` in `.clang-format` is set to `100` which defines line length (in general where lines should be broken) that allows two editors side by side on a 1080p screen for diffs.

### 3.1. Braces
Curly braces always go on a new line.

```cpp
for (int i = 0; i < t; ++i)
{
  [...]
}

if (true)
{
  [...]
}

class Dummy
{
  [...]
};
```

### 3.2. Indentation
Use spaces as tab policy with an indentation size of 2.
Opening curly braces increase the level of indentation. Closing curly braces decrease the level of indentation.

**Exception:** Do not indent namespaces to simplify nesting them and wrapping *.cpp* files in a namespace.

```cpp
namespace KODI
{
namespace UTILS
{

class ILogger
{
public:
  virtual void Log(...) = 0;
  virtual ~ILogger() {}
}

}
}
```

### 3.3. Control statements
Insert a new line before every:
* else in an if statement
* catch in a try statement

#### 3.3.1. if else
Put the consequent on a new line if not in curly braces anyway. Keep `else if` statements on one line. Do not put a condition and a following statement on a single line.

✅ Good:
```cpp
if (true)
  return;

if (true)
{
  [...]
}
else if (false)
{
  return;
}
else
  return;
```

❌ Bad:
```cpp
if (true) return;
```

#### 3.3.2. switch case

```cpp
switch (cmd)
{
  case x:
  {
    doSomething();
    break;
  }
  case x:
  case z:
    return true;
  default:
    doSomething();
}
```

#### 3.3.3. try catch

```cpp
try
{
  [...]
}
catch (std::exception& e)
{
  [...]
  throw;
}
catch (...)
{
  [...]
}
```

### 3.4. Whitespace
Conventional operators have to be surrounded by one space on each side.
```cpp
a = (b + c) * d;
```
Control statement keywords have to be separated from opening parentheses by one space.
```cpp
while (true);
for (int i = 0; i < x; ++i);
```
When conditions are used without parentheses, it is preferable to add a new line, to make the next block of code more readable.
```cpp
if (true)
  return;

if (true)
```
Commas have to be followed by one space.
```cpp
void Dummy::Method(int a, int b, int c);
```
Initializer lists have one space after each element (including comma), but no surrounding spaces.
```cpp
constexpr int aNiceArray[] = {1, 2, 3};
```

### 3.5. Vertical alignment
Do not use whitespace to vertically align around operators or values. This causes problems on code review if one needs to realign all values to their new position, producing unnecessarily large diffs.

✅ Good:
```cpp
int value1{1};
int value2{2};
CExampleClass* exampleClass{};
CBiggerExampleClass* biggerExampleClass{};
exampleClass = new CExampleClass(value1, value2);
biggerExampleClass = new CBiggerExampleClass(value1, value2);
exampleClass->InitExample();
biggerExampleClass->InitExample();
```

❌ Bad:
```cpp
int                  value1             {1};
int                  value2             {2};
[...]
CExampleClass       *exampleClass       {};
CBiggerExampleClass *biggerExampleClass {};
[...]
exampleClass       = new CExampleClass      (value1, value2);
biggerExampleClass = new CBiggerExampleClass(value1, value2);
[...]
exampleClass      ->InitExample();
biggerExampleClass->InitExample();
```

### 3.6. Superfluous `void`

Do not write `void` in empty function parameter declarations.

✅ Good:
```cpp
void Test();
```

❌ Bad:
```cpp
void Test(void);
```

### 3.7. Exceptions to the Formatting Rules For Better Readability
There are some special situations where vertical alignment and longer lines does greatly aid readability, for example the initialization of some table-like multiple row structures. In these **rare** cases exceptions can be made to the formatting rules on vertical alignment, and the defined line length can be exceeded. 

To prevent the layout from being reformatted, tell `clang-format` to [disable formatting](https://clang.llvm.org/docs/ClangFormatStyleOptions.html#disabling-formatting-on-a-piece-of-code) on that section of code by surrounding it with the special comments `// clang-format off` and `// clang-format on`.
For example:
```
// clang-format off
static const CGUIDialogMediaFilter::Filter filterList[] = {
  { "movies",       FieldTitle,         556,    SettingType::String,  "edit",   "string",   CDatabaseQueryRule::OPERATOR_CONTAINS },
  { "movies",       FieldRating,        563,    SettingType::Number,  "range",  "number",   CDatabaseQueryRule::OPERATOR_BETWEEN },
  { "movies",       FieldUserRating,    38018,  SettingType::Integer, "range",  "integer",  CDatabaseQueryRule::OPERATOR_BETWEEN },
  ...
  { "songs",        FieldSource,        39030,  SettingType::List,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
};  
// clang-format on
 ```
The other code guidelines will still need to be applied within the delimited lines of code, but with `clang-format` off care will be needed to check these manually. Using vertical alignment means that sometimes the entire block of code may need to be realigned, good judgement should be used in each case with the objective of preserving readability yet minimising impact.
 
This is to be used with discretion, marking large amounts of code to be left unformatted by `clang-format` without reasonable justification will be rejected.

**[back to top](#table-of-contents)**

## 4. Statements

### 4.1. Multiple statements
Do not put multiple statements on a single line. Always use a new line for a new statement. It is much easier to debug if one can pinpoint a precise line number.

✅ Good:
```cpp
std::vector<std::string> test;
test.push_back("foobar");
```

❌ Bad:
```cpp
std::vector<std::string> test; test.push_back("foobar");
```

### 4.2. `switch` default case

In every `switch` structure, always include a `default` case, unless switching on an enum and all enum values are explicitly handled.

**[back to top](#table-of-contents)**

## 5. Declarations

### 5.1. Declarations

Always declare a variable close to its use and not before a block of code that does not use it.

✅ Good:
```cpp
int x{3};
CLog::Log("test: {}", x); // variable used just after its declaration
```

❌ Bad:
```cpp
int x{3}; // variable not immediately used by the next block of code 
[...many lines of code that do not use variable x...]
CLog::Log("test: {}", x);
```

### 5.2. Multiple declarations
Do not put multiple declarations on a single line. This avoids confusion with differing pointers, references, and initialization values on the same line (cf. [ISO C++ guidelines](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#es10-declare-one-name-only-per-declaration)).

✅ Good:
```cpp
char* a;
char b;
```

❌ Bad:
```cpp
char* a, b;
```

### 5.3. Pointer and reference types
Left-align `*` and `&` to the base type they modify.

✅ Good:
```cpp
char* a;
void test(const std::string& b);
```

❌ Bad:
```cpp
char *a;
char * b;
void test(const std::string &b);
```

(This is adopted from the HP C++ Coding Guidelines: "The characters * and & are to be written with the type of variables instead of with the name of variables in order to emphasize that they are part of the type definition.")

### 5.4. `const` and other modifiers
Place `const` and similar modifiers in front of the type they modify.

✅ Good:
```cpp
void Test(const std::string& a);
const int* const someIntPointer;
```

❌ Bad:
```cpp
void Test(std::string const& a);
int const * const someIntPointer;
```

### 5.5. Initialization

Make sure that variables are initialized appropriately at declaration or soon afterwards. This is especially important for fundamental type variables that do not have any constructor. Zero-initialize with `{}`.

✅ Good:
```cpp
int x{3};
int* y{nullptr};
bool z = false;
std::string text; // not primitive
KindOfStruct theStruct{}; // POD structures or structures with uninitalised members must be initialised with empty brackets
Log::Log("test: {}, {}, {}", x, y, z);
```

❌ Bad:
```cpp
int x; // used uninitialized
int* y = nullptr; // default-initialization not using {}
bool z{}; // no value explicitly declared for fundamental type, preferable for better reading
std::string text{}; // has its default constructor
Log::Log("test: {}, {}, {}", x, y, z);
```

We allow variable initialization using any of the C++ forms `{}`, `=` or `()`.

However, we would like to point out some optional suggestions to follow:
- Prefer the `{}` form to others, because this permits explicit type checking to avoid unwanted narrowing conversions.
- Prefer the `{}` form when initializing a class/struct variable.
- Specify an explicit initialization value, at least for fundamental types.

**[back to top](#table-of-contents)**

## 6. Scoping

### 6.1. Namespaces

Try to put all code into appropriate namespaces (e.g. following directory structure) and avoid polluting the global namespace.

### 6.2. Local functions

Put functions local to a compilation unit into an anonymous namespace.

✅ Good:
```cpp
namespace
{

void test();

} // unnamed namespace
```

❌ Bad:
```cpp
static void test();
```

**[back to top](#table-of-contents)**

## 7. Headers
### 7.1. Header order
Included header files have to be sorted (case sensitive) alphabetically to prevent duplicates and allow better overview, with an empty line clearly separating sections.

Header order has to be:
* Own header file
* Other Kodi includes (platform independent)
* Other Kodi includes (platform specific)
* C and C++ system files
* Other libraries' header files
* special Kodi headers (PlatformDefs.h, system.h and system_gl.h)

```cpp
#include "PVRManager.h"

#include "Application.h"
#include "ServiceBroker.h"
#include "addons/AddonInstaller.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/ThreadMessage.h"
#include "messaging/helpers/DialogHelper.h"
#include "music/MusicDatabase.h"
#include "music/tags/MusicInfoTag.h"
#include "network/Network.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/JobManager.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include <cassert>
#include <utility>

#include <libavutil/pixfmt.h>
```

If the headers aren't sorted, either do your best to match the existing order, or precede your commit with an alphabetization commit.

If possible, avoid including headers in another header. Instead, you can forward-declare the class and use a `std::unique_ptr` (or similar):

```cpp
class CFileItem;

class Example
{
  ...
  std::unique_ptr<CFileItem> m_fileItem;
}
```

### 7.2. Use C++ wrappers for C headers
To use C symbols use C++ wrappers headers, by using the `std` namespace prefix.

✅ Good:
```cpp
#include <cstring>
[...]
size_t n = std::strlen(str);
```

❌ Bad:
```cpp
#include <string.h> // C header
[...]
size_t n = strlen(str); // missing std namespace
```

**[back to top](#table-of-contents)**

## 8. Naming
### 8.1. Namespaces
Use upper case with underscores.
```cpp
namespace KODI
{
[...]
} // namespace KODI
```

### 8.2. Constants
Use upper case with underscores.
```cpp
constexpr int MY_CONSTANT = 1;
```

### 8.3. Enums
Use PascalCase for the enum name and upper case with underscores for the values.
```cpp
enum class Dummy
{
  VALUE_X,
  VALUE_Y
};
```

### 8.4. Interfaces
Use PascalCase and prefix with an uppercase I. Filename has to match the interface name without the prefixed I, e.g. Logger.h
```cpp
class ILogger
{
public:
  virtual void Log(...) = 0;
  virtual ~ILogger() {}
}
```

### 8.5. Classes
Use PascalCase and prefix with an uppercase C. Filename has to match the class name without the prefixed C, e.g. Logger.cpp
```cpp
class CLogger : public ILogger
{
public:
  void Log(...) override;
}
```

### 8.6. Structs
Use PascalCase.
```cpp
struct InfoChar
{
  bool m_isBold{false};
}
```

### 8.7. Methods
Use PascalCase always, uppercasing the first letter even if the methods are private or protected.
Method parameters start with lower case and follow CamelCase style, without type prefixing (Systems Hungarian notation).
```cpp
void MyDummyClass::DoSomething(int limitBound);
```

### 8.8. Variables
Variables start with lower case and follow CamelCase style. Type prefixing (Systems Hungarian notation) is discouraged.

✅ Good:
```cpp
bool isSunYellow{true};
```

❌ Bad:
```cpp
bool bSunYellow{true}; // type prefixing
```

#### Member variables
Prefix non-static member variables with `m_`. Prefix static member variables with `ms_`.
```cpp
int m_variableA;
static int ms_variableB;
```

#### Global variables
Prefix global variables with `g_`
```cpp
int g_globalVariableA;
```

> [!WARNING]  
> Avoid use of globals as far as reasonably possible. We generally do not want to introduce new global variables.

**[back to top](#table-of-contents)**

## 9. Comments

### 9.1. General

Use `// ` for inline single-line and multi-line comments. Use `/* */` for the copyright comment at the beginning of the file. SPDX license headers are required for all code files (see example below).

✅ Good:
```cpp
/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// Nice comment

// This can also continue for multiple lines:
// I am the second line.
```

❌ Bad:
```cpp
/* A comment */
//another comment
```

### 9.2. Doxygen

New classes and functions are expected to have Doxygen comments describing their purpose, parameters, and behavior in the header file. However, do not describe trivialities - it only adds visual noise. Use the Qt style with exclamation mark (`/*! */`) and backslash for doxygen commands (e.g. `\brief`).

✅ Good:
```cpp
/*!
 * \brief Splits the given input string using the given delimiter into separate strings.
 *
 * If the given input string is empty, nothing will be put into the target iterator.
 *
 * \param destination the beginning of the destination range
 * \param input input string to be split
 * \param delimiter delimiter to be used to split the input string
 * \param maxStrings (optional) maximum number of splitted strings
 * \return output iterator to the element in the destination range one past the last element
 *         that was put there
 */
template<typename OutputIt>
static OutputIt SplitTo(OutputIt destination, const std::string& input, const std::string& delimiter, unsigned int maxStrings = 0);
```

❌ Bad:
```cpp
/**
 * @brief Function for documentation purposes (javadoc style)
 */
void TestFunction();

void ReallyComplicatedFunction(); // does something really complicated

/*!
 * \brief Frobnicate a parameter
 * \param param parameter to frobnicate
 * \result frobnication result
 */
int Frobnicate(int param);
```

**[back to top](#table-of-contents)**

## 10. Logging

Use the provided logging function `CLog::Log`. Do not log to standard output or standard error using e.g. `printf` or `std::cout`.

The `Log` function uses the [fmt library](http://fmtlib.net/) for formatting log messages. Basically, you can use `{}` as placeholder for anything and list the parameters to insert after the message similarly to `printf`. See [here](http://fmtlib.net/latest/syntax.html) for the detailed syntax and below for an example.

✅ Good:
```cpp
CLog::Log(LOGDEBUG, "Window size: {}x{}", width, height);
CLog::LogF(LOGERROR, "An error occurred in window \"{}\"", name); // Use helper function `CLog::LogF` to print also the name of method.
```

❌ Bad:
```cpp
CLog::Log(LOGWARNING, "Window size: %dx%d", width, height); // printf-style format strings are possible, but discouraged; also the message does not warrant the warning level
CLog::Log(LOGERROR, "{}: An error occurred in window \"{}\"", __FUNCTION__, name); // Do not use __FUNCTION__ macro, use `CLog::LogF` instead.
printf("Window size: %dx%d", width, height);
std::cout << "Window size: " << width << "x" << height << std::endl;
```

The predefined logging levels are `DEBUG`, `INFO`, `WARNING`, `ERROR`, and `FATAL`. Use anything `INFO` and above sparingly since it will be written to the log by default. Too many messages will clutter the log and reduce visibility of important information. `DEBUG` messages are only written when debug logging is enabled.

## 11. Classes

### 11.1. Member visibility

Make class data members `private`. Think twice before using `protected` for data members and functions, as its level of encapsulation is effectively equivalent to `public`.

### 11.2. Const correctness

Try to mark member functions of classes as `const` whenever reasonable.

### 11.3. Overriding virtual functions

When overriding virtual functions of a base class, add the `override` keyword. Do not add the `virtual` keyword.

✅ Good:
```cpp
class CLogger : public ILogger
{
public:
  void Log(...) override;
}
```

❌ Bad:
```cpp
class CLogger : public ILogger
{
public:
  virtual void Log(...) override;
}
```

### 11.4. Default member initialization
Use default member initialization instead of initializer lists or constructor assignments whenever it makes sense.
```cpp
class Foo
{
  bool m_fooBar{false};
};
```

### 11.5. Destructors in interfaces

A class with any virtual functions should have a destructor that is either public and virtual or else protected and non-virtual (cf. [ISO C++ guidelines](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-dtor-virtual)).

### 11.6. Constructor Initialization Lists

For lines up to [line length](#line-length) everything stays on one line, excluding the braces which must be on the following lines.

```cpp
MyClass::CMyClass(bool argOne, int argTwo) : m_argOne(argOne), m_argTwo(argTwo)
{
}
```

For longer lines, insert a line break before the colon and/or after the comma.

```cpp
MyClass::CMyClass(bool argOne,
                  int argTwo,
                  const std::string& textArg,
                  const std::shared_ptr<CMyOtherClass>& myOtherClass)
  : m_argOne(argOne),
    m_argTwo(argTwo),
    m_textArg(textArg),
    m_myOtherClass(myOtherClass)
{
}
```

## 12. Other conventions

### 12.1. Output parameters
For functions that have multiple output values, prefer using a `struct` or `tuple` return value over output parameters that use pointers or references (cf. [ISO C++ guidelines](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rf-out-multi)). In general, try to avoid output parameters completely (cf. [ISO C++ guidelines](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rf-out), [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html#Output_Parameters)). At the function call site, it is completely invisible that actually a reference is being passed and the value might be modified. Return semantics make it clear what is happening.

### 12.2. Casts
New code has to use C++ style casts and not older C style casts. When modifying existing code the developer can choose to update it to C++ style casts or leave as is. Whenever a `dynamic_cast` is used to cast to a pointer type, the result can be `nullptr` and needs to be checked accordingly.

✅ Good:
```cpp
char m_dataChar{...};
uint8_t m_dataInt = static_cast<uint8_t>(m_dataChar);
```

❌ Bad:
```cpp
char m_dataChar{...};
uint8_t m_dataInt = (uint8_t)m_dataChar;
```

### 12.3. `NULL` vs `nullptr`
Prefer the use of `nullptr` instead of `NULL`. `nullptr` is a typesafe version and as such can't be implicitly converted to `int` or anything else.

### 12.4. `auto`

Feel free to use `auto` wherever it improves readability, without abusing it when it is not the case.
- Good places are iterators or types that have multiple sub-levels in a namespace.
- Bad places are code that expects a certain type that is not immediately clear from the context, or when you declare fundamental types.

✅ Good:
```cpp
[...]
constexpr float START_POINT = 5;
[...]
auto i = var.begin();

std::vector<CSizeInt> list;
for (const auto j : list)
{
  [...]
}
```

❌ Bad:
```cpp
[...]
constexpr auto START_POINT = 5; // may cause problems due to wrong type deduced, many auto variables make reading difficult
[...]
std::map<std::string, std::vector<int>>::iterator i = var.begin();
```

### 12.5. `for` loops
Use range-based for loops wherever it makes sense. If iterators are used, see above about using `auto`.
```cpp
for (const auto& : var)
{
  [...]
}
```
Remove `const` if the value has to be modified. Do not use references to fundamental types that are not modified.

In traditional for loops, for the `increment statement` of the loop, use prefix increment/decrement operator, not postfix.

✅ Good:
```cpp
[...]
for (int i = 0; i < 100; ++i)
{
  [...]
}
```

❌ Bad:
```cpp
for (int i = 0; i < 100; i++)
{
  [...]
}
```

### 12.6. Include guards

Use `#pragma once`.

✅ Good:
```cpp
#pragma once
```

❌ Bad:
```cpp
#ifndef SOME_FILE_H_INCLUDED
#define SOME_FILE_H_INCLUDED
[...]
#endif
```

### 12.7. Type aliases

Use the C++ `using` syntax when aliasing types (encouraged when it improves readability).

✅ Good:
```cpp
using SizeType = int;
```

❌ Bad:
```cpp
typedef int SizeType;
```

### 12.8. `goto`

Usage of `goto` is discouraged.

### 12.9. Macros

Try to avoid using C macros. In many cases, they can be easily substituted with other non-macro constructs.

### 12.10. `constexpr`

Prefer `constexpr` over `const` for constants when possible. Try to mark functions `constexpr` when reasonable.

### 12.11. `std::string` vs `std::string_view`

Prefer `std::string_view` over `std::string` when reasonable. Good examples are constants or method arguments. In the latter case, it is not required to declare the argument as reference or const, since the data source of string views are immutable by definition. A bad example is when you need a NUL-terminated C string, e.g. to interact with a C API. `std::string_view` does not offer a `c_str()` function like `std::string` does, but if you do not need a C string you can use `data()` to get the raw source of the data.

Main reasons why we prefer `std::string_view` are: execution performance, no memory allocations, substrings can be made without copy, and the possibility to reuse the same data without reallocation.

Avoid using `std::string_view` when you are not sure where the source of data is allocated, or as return value of a method. If not handled properly, the source (storage) of the data may go out of scope. As a consequence, the program enters undefined behavior and may crash, behave strangely, or introduce potential security issues.

✅ Good:
```cpp
namespace
{
constexpr std::string_view CONSTANT_FOO{"foo-bar"};
} // unnamed namespace

void CClass::SetText(std::string_view value) 
{
  CLog::LogF(LOGDEBUG, "My name is {}", value);
}

[...]
SetText(CONSTANT_FOO.substr(0, 3)); // substr returns a modified view of the same string_view, thus without allocations
```

❌ Bad:
```cpp
namespace
{
constexpr std::string CONSTANT_FOO{"foo-bar"}; // using string_view will avoid a memory allocation 
} // unnamed namespace

void CClass::SetText(const std::string& value) // despite being declared as a reference, using string_view will result in a lower overhead in many cases (e.g., when passing a C string literal)
{
  CLog::LogF(LOGDEBUG, "My name is {}", value);
}

[...]
SetText(CONSTANT_FOO.substr(0, 3)); // using string_view will avoid a memory allocation due to substr
```

**[back to top](#table-of-contents)**
