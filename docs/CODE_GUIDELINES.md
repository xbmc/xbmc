![Kodi Logo](resources/banner_slim.png)

# Code Guidelines

## Table of Contents
1. **[Motivation](#1-motivation)**
2. **[Document Conventions](#2-document-conventions)**
3. **[Indentation](#3-indentation)**
4. **[Statements](#4-statements)**
5. **[Namespaces](#5-namespaces)**
6. **[Headers](#6-headers)**
7. **[Braces](#7-braces)**
8. **[Whitespace](#8-whitespace)**
9. **[Vertical Alignment](#9-vertical-alignment)**
10. **[Controls Statements](#10-controls-statements)**  
  10.1. **[if else](#101-if-else)**  
  10.2. **[switch case](#102-switch-case)**
11. **[Naming](#11-naming)**  
  11.1. **[Namespaces](#111-namespaces)**  
  11.2. **[Constants](#112-constants)**  
  11.3. **[Enums](#113-enums)**  
  11.4. **[Interfaces](#114-interfaces)**  
  11.5. **[Classes](#115-classes)**  
  11.6. **[Methods](#116-methods)**  
  11.7. **[Variables](#117-variables)**
12. **[Conventions](#12-conventions)**  
  12.1. **[Casts](#121-casts)**  
  12.2. **[NULL vs nullptr](#122-null-vs-nullptr)**  
  12.3. **[auto](#123-auto)**  
  12.4. **[for loops](#124-for-loops)**

## 1. Motivation
When working in a large group, the two most important values are readability and maintainability. We code for other people, not computers. To accomplish these goals, we have created a unified set of code conventions.

Conventions can be bent or broken in the interest of making code more readable and maintainable. However, if you submit a patch that contains excessive style conflicts, you may be asked to improve your code before your pull request is reviewed.

## 2. Document Conventions
Several different strategies are used to draw your attention to certain pieces of information. In order of how critical the information is, these items are marked as a note, tip, or warning. For example: 
 
**NOTE:** Linux is user friendly... It's just very particular about who its friends are.  
**TIP:** Algorithm is what developers call code they do not want to explain.  
**WARNING:** Developers don't change light bulbs. It's a hardware problem.

**[back to top](#table-of-contents)**

## 3. Indentation
Use spaces as tab policy with an indentation size of 2.

**[back to top](#table-of-contents)**

## 4. Statements
No multiple statements on a single line.
```cpp
std::vector<std::string> test; test.push_back("foobar");
```
Always use a new line for a new statement.
```cpp
std::vector<std::string> test;
test.push_back("foobar");
```
It is much easier to debug if one can pinpoint a precise line number.

**[back to top](#table-of-contents)**

## 5. Namespaces
Indentation is not required to simplify nested namespaces and wrapping *.cpp* files in a namespace.
```cpp
namespace KODI
{
namespace UTILS
{
class ILogger
{
  void Log(...) = 0;
}
}
}
```

**[back to top](#table-of-contents)**

## 6. Headers
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

If possible, avoid including headers in another header. Instead, you can forward-declare the class and use a `std::unique_ptr`:

```cpp
class CFileItem;

class Example
{
  ...
  std::unique_ptr<CFileItem> m_fileItem;
}
```

**[back to top](#table-of-contents)**

## 7. Braces
Braces have to go to a new line.
```cpp
if (int i = 0; i < t; i++)
{
  [...]
}
else
{
  [...]
}
class Dummy()
{
  [...]
}
```

**[back to top](#table-of-contents)**

## 8. Whitespace
Conventional operators have to be surrounded by a whitespace.
```cpp
a = (b + c) * d;
```
Reserved words have to be separated from opening parentheses by a whitespace.
```cpp
while (true)
for (int i = 0; i < x; ++i)
```
Commas have to be followed by a whitespace.
```cpp
void Dummy::Method(int a, int b, int c);
int d, e;
```
Semicolons have to be followed by a newline.
```cpp
for (int i = 0; i < x; ++i)
  doSomething(e);
  doSomething(f);
```

**[back to top](#table-of-contents)**

## 9. Vertical Alignment
Do not use whitespaces to align value names together. This causes problems on code review if one needs to realign all values to their new position.

Wrong:
```cpp
int                  value1             = 0;
int                  value2             = 0;
[...]
CExampleClass       *exampleClass       = nullptr;
CBiggerExampleClass *biggerExampleClass = nullptr;
[...]
exampleClass       = new CExampleClass      (value1, value2);
biggerExampleClass = new CBiggerExampleClass(value1, value2);
[...]
exampleClass      ->InitExample();
biggerExampleClass->InitExample();
```

Right:
```cpp
int value1 = 0;
int value2 = 0;
CExampleClass *exampleClass = nullptr;
CBiggerExampleClass *biggerExampleClass = nullptr;
exampleClass = new CExampleClass(value1, value2);
biggerExampleClass = new CBiggerExampleClass(value1, value2);
exampleClass->InitExample();
biggerExampleClass->InitExample();
```

**[back to top](#table-of-contents)**

## 10. Controls Statements
Insert a new line before every:
* else in an if statement
* catch in a try statement
* while in a do statement

### 10.1. if else
Put `then`, `return` or `throw` statements on a new line. Keep `else if` statements on one line.
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

### 10.2. switch case
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

**[back to top](#table-of-contents)**

## 11. Naming
### 11.1. Namespaces
Namespaces have to be in uppercase.
```cpp
namespace KODI
{
  [...]
}
```

### 11.2. Constants
Use uppercase with underscore spacing where necessary.
```cpp
const int MY_CONSTANT = 1;
```

### 11.3. Enums
Use CamelCase for the enum name and uppercase for the values.
```cpp
enum Dummy
{
  VALUE_X,
  VALUE_Y
};
```

### 11.4. Interfaces
Use CamelCase for interface names and they have to be prefixed with an uppercase I. Filename has to match the interface name without the prefixed I, e.g. ILogger.h
```cpp
class ILogger
{
  void Log(...) = 0;
}
```

### 11.5. Classes
Use CamelCase for class names and they have to be prefixed with an uppercase C. Filename has to match the class name without the prefixed C, e.g. Logger.cpp
```cpp
class CLogger : public ILogger
{
  void Log(...)
}
```

### 11.6. Methods
Use CamelCase for method names and first letter has to be uppercase, even if the methods are private or protected.
```cpp
void MyDummyClass::DoSomething();
```

### 11.7. Variables
Use CamelCase for variables. Type prefixing is discouraged.

#### Global Variables
Prefix global variables with *g_*
```cpp
int g_globalVariableA;
```
**WARNING:** Avoid globals use. It increases the chances of submitted code to be rejected.

#### Member Variables
Prefix member variables with *m_*
```cpp
int m_variableA;
```

**[back to top](#table-of-contents)**

## 12. Conventions
### 12.1. Casts
New code has to use C++ style casts and not older C style casts. When modifying existing code the developer can choose to update it to C++ style casts or leave as is. Remember that whenever a `dynamic_cast` is used on a pointer object the result can be a `nullptr` and needs to be checked accordingly.

### 12.2. NULL vs nullptr
Prefer the use of `nullptr` instead of `NULL`. `nullptr` is a typesafe version and as such can't be implicitly converted to `int` or anything else.

### 12.3. auto

Feel free to use `auto` wherever it improves readability. Good places are iterators or when dealing with containers.
```cpp
std::map<std::string, std::vector<int>>::iterator i = var.begin();
```
vs
```cpp
auto i = var.being();
```
### 12.4. for loops
Use range-based for loops wherever it makes sense. If iterators are used see above about using `auto`.
```cpp
for (const auto& : var)
{
  [...]
}
```
Remove `const` if the value has to be modified.

**[back to top](#table-of-contents)**

