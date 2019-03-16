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
  * [5.1. Multiple declarations](#51-multiple-declarations)
  * [5.2. Pointer and reference types](#52-pointer-and-reference-types)
  * [5.3. const and other modifiers](#53-const-and-other-modifiers)
  * [5.4. Initialization](#54-initialization)
* [6. Scoping](#6-scoping)
  * [6.1. Namespaces](#61-namespaces)
  * [6.2. Local functions](#62-local-functions)
* [7. Headers](#7-headers)
* [8. Naming](#8-naming)
  * [8.1. Namespaces](#81-namespaces)
  * [8.2. Constants](#82-constants)
  * [8.3. Enums](#83-enums)
  * [8.4. Interfaces](#84-interfaces)
  * [8.5. Classes](#85-classes)
  * [8.6. Methods](#86-methods)
  * [8.7. Variables](#87-variables)
    * [Member variables](#member-variables)
    * [Global variables](#global-variables)
* [9. Comments](#9-comments)
  * [9.1. General](#91-general)
  * [9.2. Doxygen](#92-doxygen)
* [10. Other conventions](#10-other-conventions)
  * [10.1. Destructors in interfaces](#101-destructors-in-interfaces)
  * [10.2. Output parameters](#102-output-parameters)
  * [10.3. Casts](#103-casts)
  * [10.4. `NULL` vs `nullptr`](#104-null-vs-nullptr)
  * [10.5. auto](#105-auto)
  * [10.6. `for` loops](#106-for-loops)
  * [10.7. Default member initialization](#107-default-member-initialization)
  * [10.8. Include guards](#108-include-guards)
  * [10.9. Type aliases](#109-type-aliases)
  * [10.10. `goto`](#1010goto)
  * [10.11. Macros](#1011-macros)

## 1. Motivation
When working in a large group, the two most important values are readability and maintainability. We code for other people, not computers. To accomplish these goals, we have created a unified set of code conventions.

Conventions can be bent or broken in the interest of making code more readable and maintainable. However, if you submit a patch that contains excessive style conflicts, you may be asked to improve your code before your pull request is reviewed.

In the repository root directory, there is a `.clang-format` file that implements the rules as specified here. You are encouraged to run `clang-format` on any newly created files. It is currently not recommended to do so on preexisting files because all the formatting changes will clutter your commits and pull request.

**[back to top](#table-of-contents)**

## 2. Language standard

We currently target the C++11 language standard. Do use C++11 features when possible. Do not use C++14 or C++17 features.

**[back to top](#table-of-contents)**

## 3. Formatting

### 3.1. Braces
Curly braces always go on a new line.

```cpp
for (int i = 0; i < t; i++)
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

**Exception:** Do not increase indentation after a `switch` statements.

### 3.3. Control statements
Insert a new line before every:
* else in an if statement
* catch in a try statement
* while in a do statement

#### 3.3.1. if else
Put the consequent on a new line if not in curly braces anyway. Keep `else if` statements on one line.

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

#### 3.3.2. switch case
Do not indent `case` and `default`.

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
for (int i = 0; i < x; i++);
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

### 5.1. Multiple declarations
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

### 5.2. Pointer and reference types
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

### 5.3. `const` and other modifiers
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

### 5.4. Initialization

Make sure that variables are initialized appropriately at declaration or soon afterwards. This is especially important for fundamental type variables that do not have any constructor. Zero-initialize with `{}`.

✅ Good:
```cpp
int x{};
int* y{};
CLog::Log("test: {} {}", x, y);
```

❌ Bad:
```cpp
int x; // used uninitialized
int* y = nullptr; // default-initialization not using {}
CLog::Log("test: {} {}", x, y);
```

In general, prefer the `{}` initializer syntax over alternatives. This syntax is less ambiguous and disallows narrowing (cf. [ISO C++ guidelines](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Res-list)).

✅ Good:
```cpp
int x{5};
int y{x};
```

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

}
```

❌ Bad:
```cpp
static void test();
```

**[back to top](#table-of-contents)**

## 7. Headers
Included header files have to be sorted alphabetically to prevent duplicates and allow better overview, with an empty line clearly separating sections.

Header order has to be:
* Own header file
* Other Kodi includes
* C and C++ system files
* Other libraries' header files

```cpp
#include "PVRManager.h"

#include "addons/AddonInstaller.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/ThreadMessage.h"
#include "music/tags/MusicInfoTag.h"
#include "music/MusicDatabase.h"
#include "network/Network.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/JobManager.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "video/VideoDatabase.h"
#include "Application.h"
#include "ServiceBroker.h"

#include <cassert>
#include <utility>

#include <libavutil/pixfmt.h>
```

Place directories before files. If the headers aren't sorted, either do your best to match the existing order, or precede your commit with an alphabetization commit.

If possible, avoid including headers in another header. Instead, you can forward-declare the class and use a `std::unique_ptr` (or similar):

```cpp
class CFileItem;

class Example
{
  ...
  std::unique_ptr<CFileItem> m_fileItem;
}
```

**[back to top](#table-of-contents)**

## 8. Naming
### 8.1. Namespaces
Use upper case with underscores.
```cpp
namespace KODI
{
[...]
}
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

### 8.6. Methods
Use PascalCase always, uppercasing the first letter even if the methods are private or protected.
```cpp
void MyDummyClass::DoSomething();
```

### 8.7. Variables
Use CamelCase. Type prefixing (Systems Hungarian notation) is discouraged.

#### Member variables
Prefix nonstatic member variables with `m_`. Prefix static member variables with `ms_`.
```cpp
int m_variableA;
static int ms_variableB;
```

#### Global variables
Prefix global variables with `g_`
```cpp
int g_globalVariableA;
```
**WARNING:** Avoid use of globals as far as reasonably possible. We generally do not want to introduce new global variables.

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
```

❌ Bad:
```cpp
CLog::Log(LOGWARNING, "Window size: %dx%d", width, height); // printf-style format strings are possible, but discouraged; also the message does not warrant the warning level
printf("Window size: %dx%d", width, height);
std::cout << "Window size: " << width << "x" << height << std::endl;
```

The predefined logging levels are `DEBUG`, `INFO`, `NOTICE`, `WARNING`, `ERROR`, `SEVERE`, and `FATAL`. Use anything `INFO` and above sparingly since it will be written to the log by default. Too many messages will clutter the log and reduce visibility of important information. `DEBUG` messages are only written when debug logging is enabled.

## 10. Classes

### 10.1. Member visibility

Make class data members `private`. Think twice before using `protected` for data members and functions, as its level of encapsulation is effectively equivalent to `public`.

### 10.13. Const correctness

Try to mark member functions of classes as `const` whenever reasonable.

### 10.10. Overriding virtual functions

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

### 10.7. Default member initialization
Use default member initialization instead of initializer lists or constructor assignments whenever it makes sense.
```cpp
class Foo
{
  bool bar{false};
};
```

### 10.1. Destructors in interfaces

A class with any virtual functions should have a destructor that is either public and virtual or else protected and nonvirtual (cf. [ISO C++ guidelines](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-dtor-virtual)).

## 10. Other conventions

### 10.2. Output parameters
For functions that have multiple output values, prefer using a `struct` or `tuple` return value over output parameters that use pointers or references (cf. [ISO C++ guidelines](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rf-out-multi)). In general, try to avoid output parameters completely (cf. [ISO C++ guidelines](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rf-out), [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html#Output_Parameters)). At the function call site, it is completely invisible that actually a reference is being passed and the value might be modified. Return semantics make it clear what is happening.

### 10.3. Casts
New code has to use C++ style casts and not older C style casts. When modifying existing code the developer can choose to update it to C++ style casts or leave as is. Whenever a `dynamic_cast` is used to cast to a pointer type, the result can be `nullptr` and needs to be checked accordingly.

### 10.4. `NULL` vs `nullptr`
Prefer the use of `nullptr` instead of `NULL`. `nullptr` is a typesafe version and as such can't be implicitly converted to `int` or anything else.

### 10.5. `auto`

Feel free to use `auto` wherever it improves readability, which is not always the case. Good places are iterators or when dealing with containers. Bad places are code that expects a certain type that is not immediately clear from the context.

✅ Good:
```cpp
auto i = var.begin();

std::vector<CSizeInt> list;
for (const auto j : list)
{
  [...]
}
```

❌ Bad:
```cpp
std::map<std::string, std::vector<int>>::iterator i = var.begin();
```

### 10.6. `for` loops
Use range-based for loops wherever it makes sense. If iterators are used, see above about using `auto`.
```cpp
for (const auto& : var)
{
  [...]
}
```
Remove `const` if the value has to be modified. Do not use references to fundamental types that are not modified.

### 10.8. Include guards

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

### 10.9. Type aliases

Use the C++ `using` syntax when aliasing types (encouraged when it improves readability).

✅ Good:
```cpp
using SizeType = int;
```

❌ Bad:
```cpp
typedef int SizeType;
```

### 10.11. `goto`

Usage of `goto` is discouraged.

### 10.12. Macros

Try to avoid using C macros. In many cases, they can be easily substituted with other non-macro constructs.

### 10.14. `constexpr`

Prefer `constexpr` over `const` for constants when possible. Try to mark functions `constexpr` when reasonable.

**[back to top](#table-of-contents)**
