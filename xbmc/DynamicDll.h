#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "cores/DllLoader/LibraryLoader.h"
#include <string>
#include "DllPaths.h"

///////////////////////////////////////////////////////////
//
//  DECLARE_DLL_WRAPPER
//
//  Declares the constructor of the wrapper class.
//  This must be followed by one or more
//  DEFINE_METHODX/DEFINE_METHOD_LINKAGEX and
//  one BEGIN_METHOD_RESOLVE/END_METHOD_RESOLVE block.
//
//  classname: name of the wrapper class to construct
//  dllname: file including path of the dll to wrap

#define DECLARE_DLL_WRAPPER(classname, dllname) \
XDECLARE_DLL_WRAPPER(classname,dllname)

#define XDECLARE_DLL_WRAPPER(classname, dllname) \
public: \
  classname () : DllDynamic( dllname ) {}

///////////////////////////////////////////////////////////
//
//  DECLARE_DLL_WRAPPER_TEMPLATE_BEGIN
//
//  Declares the constructor of the wrapper class.
//  The method SetFile(strDllName) can be used to set the
//  dll of this wrapper.
//  This must be followed by one or more
//  DEFINE_METHODX/DEFINE_METHOD_LINKAGEX and
//  one BEGIN_METHOD_RESOLVE/END_METHOD_RESOLVE block.
//
//  classname: name of the wrapper class to construct
//
#define DECLARE_DLL_WRAPPER_TEMPLATE(classname) \
public: \
  classname () {} \


///////////////////////////////////////////////////////////
//
//  LOAD_SYMBOLS
//
//  Tells the dllloader to load Debug symblos when possible
#define LOAD_SYMBOLS() \
  protected: \
    virtual bool LoadSymbols() { return true; }

///////////////////////////////////////////////////////////
//
//  DEFINE_GLOBAL
//
//  Defines a global for export from the dll as well as
//  a function for accessing it (Get_name).
//
//  type: The variables type.
//  name: Name of the variable.
//

#define DEFINE_GLOBAL_PTR(type, name) \
  protected: \
    union { \
      type* m_##name; \
      void* m_##name##_ptr; \
    }; \
  public: \
    virtual type* Get_##name (void) \
    { \
      return m_##name; \
    }

#define DEFINE_GLOBAL(type, name) \
  protected: \
    union { \
      type* m_##name; \
      void* m_##name##_ptr; \
    }; \
  public: \
    virtual type Get_##name (void) \
    { \
      return *m_##name; \
    }

///////////////////////////////////////////////////////////
//
//  DEFINE_METHOD_LINKAGE
//
//  Defines a function for an export from a dll, if the
//  calling convention is not __cdecl.
//  Use DEFINE_METHOD_LINKAGE for each function to be resolved.
//
//  result:  Result of the function
//  linkage: Calling convention of the function
//  name:    Name of the function
//  args:    Arguments of the function, enclosed in parentheses
//
#define DEFINE_METHOD_LINKAGE_FP(result, linkage, name, args) \
  protected: \
    typedef result (linkage * name##_METHOD) args; \
  public: \
    union { \
      name##_METHOD name; \
      void*         name##_ptr; \
    };

#define DEFINE_METHOD_LINKAGE_BASE(result, linkage, name, args, args2) \
  protected: \
    typedef result (linkage * name##_METHOD) args; \
    union { \
      name##_METHOD m_##name; \
      void*         m_##name##_ptr; \
    }; \
  public: \
    virtual result name args \
    { \
      return m_##name args2; \
    }

#define DEFINE_METHOD_LINKAGE0(result, linkage, name) \
        DEFINE_METHOD_LINKAGE_BASE(result, linkage, name, ()  , ())

#define DEFINE_METHOD_LINKAGE1(result, linkage, name, args) \
        DEFINE_METHOD_LINKAGE_BASE(result, linkage, name, args, (p1))

#define DEFINE_METHOD_LINKAGE2(result, linkage, name, args) \
        DEFINE_METHOD_LINKAGE_BASE(result, linkage, name, args, (p1, p2))

#define DEFINE_METHOD_LINKAGE3(result, linkage, name, args) \
        DEFINE_METHOD_LINKAGE_BASE(result, linkage, name, args, (p1, p2, p3))

#define DEFINE_METHOD_LINKAGE4(result, linkage, name, args) \
        DEFINE_METHOD_LINKAGE_BASE(result, linkage, name, args, (p1, p2, p3, p4))

#define DEFINE_METHOD_LINKAGE5(result, linkage, name, args) \
        DEFINE_METHOD_LINKAGE_BASE(result, linkage, name, args, (p1, p2, p3, p4, p5))

#define DEFINE_METHOD_LINKAGE6(result, linkage, name, args) \
        DEFINE_METHOD_LINKAGE_BASE(result, linkage, name, args, (p1, p2, p3, p4, p5, p6))

#define DEFINE_METHOD_LINKAGE7(result, linkage, name, args) \
        DEFINE_METHOD_LINKAGE_BASE(result, linkage, name, args, (p1, p2, p3, p4, p5, p6, p7))

#define DEFINE_METHOD_LINKAGE8(result, linkage, name, args) \
        DEFINE_METHOD_LINKAGE_BASE(result, linkage, name, args, (p1, p2, p3, p4, p5, p6, p7, p8))

#define DEFINE_METHOD_LINKAGE9(result, linkage, name, args) \
        DEFINE_METHOD_LINKAGE_BASE(result, linkage, name, args, (p1, p2, p3, p4, p5, p6, p7, p8, p9))

#define DEFINE_METHOD_LINKAGE10(result, linkage, name, args) \
        DEFINE_METHOD_LINKAGE_BASE(result, linkage, name, args, (p1, p2, p3, p4, p5, p6, p7, p8, p9, p10))

#define DEFINE_METHOD_LINKAGE11(result, linkage, name, args) \
        DEFINE_METHOD_LINKAGE_BASE(result, linkage, name, args, (p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11))

///////////////////////////////////////////////////////////
//
//  DEFINE_METHOD_FP
//
//  Defines a function for an export from a dll as a fuction pointer.
//  Use DEFINE_METHOD_FP for each function to be resolved. Functions
//  defined like this are not listed by IntelliSence.
//
//  result: Result of the function
//  name:   Name of the function
//  args:   Arguments of the function, enclosed in parentheses
//          The parameter names can be anything
//
#define DEFINE_METHOD_FP(result, name, args) DEFINE_METHOD_LINKAGE_FP(result, __cdecl, name, args)

///////////////////////////////////////////////////////////
//
//  DEFINE_METHODX
//
//  Defines a function for an export from a dll.
//  Use DEFINE_METHODX for each function to be resolved.
//  where X is the number of parameter the function has.
//
//  result: Result of the function
//  name:   Name of the function
//  args:   Arguments of the function, enclosed in parentheses
//          The parameter names have to be renamed to px, where
//          x is the number of the parameter
//
#define DEFINE_METHOD0(result, name) DEFINE_METHOD_LINKAGE0(result, __cdecl, name)
#define DEFINE_METHOD1(result, name, args) DEFINE_METHOD_LINKAGE1(result, __cdecl, name, args)
#define DEFINE_METHOD2(result, name, args) DEFINE_METHOD_LINKAGE2(result, __cdecl, name, args)
#define DEFINE_METHOD3(result, name, args) DEFINE_METHOD_LINKAGE3(result, __cdecl, name, args)
#define DEFINE_METHOD4(result, name, args) DEFINE_METHOD_LINKAGE4(result, __cdecl, name, args)
#define DEFINE_METHOD5(result, name, args) DEFINE_METHOD_LINKAGE5(result, __cdecl, name, args)
#define DEFINE_METHOD6(result, name, args) DEFINE_METHOD_LINKAGE6(result, __cdecl, name, args)
#define DEFINE_METHOD7(result, name, args) DEFINE_METHOD_LINKAGE7(result, __cdecl, name, args)
#define DEFINE_METHOD8(result, name, args) DEFINE_METHOD_LINKAGE8(result, __cdecl, name, args)
#define DEFINE_METHOD9(result, name, args) DEFINE_METHOD_LINKAGE9(result, __cdecl, name, args)
#define DEFINE_METHOD10(result, name, args) DEFINE_METHOD_LINKAGE10(result, __cdecl, name, args)
#define DEFINE_METHOD11(result, name, args) DEFINE_METHOD_LINKAGE11(result, __cdecl, name, args)

#ifdef TARGET_WINDOWS
///////////////////////////////////////////////////////////
//
//  DEFINE_FUNC_ALIGNED 0-X
//
//  Defines a function for an export from a dll, wich
//  require a aligned stack on function call
//  Use DEFINE_FUNC_ALIGNED for each function to be resolved.
//
//  result:  Result of the function
//  linkage: Calling convention of the function
//  name:    Name of the function
//  args:    Argument types of the function
//
//  Actual function call will expand to something like this
//  this will align the stack (esp) at the point of function
//  entry as required by gcc compiled dlls, it is abit abfuscated
//  to allow for different sized variables
//
//  int64_t test(int64_t p1, char p2, char p3)
//  {
//    int o,s = ((sizeof(p1)+3)&~3)+((sizeof(p2)+3)&~3)+((sizeof(p3)+3)&~3);
//    __asm mov [o],esp;
//    __asm sub esp, [s];
//    __asm and esp, ~15;
//    __asm add esp, [s]
//    m_test(p1, p2, p3);  //return value will still be correct aslong as we don't mess with it
//    __asm mov esp,[o];
//  };

#define ALS(a) ((sizeof(a)+3)&~3)
#define DEFINE_FUNC_PART1(result, linkage, name, args) \
  private:                                             \
    typedef result (linkage * name##_type)##args;      \
    union { \
      name##_type m_##name;                            \
      void*       m_##name##_ptr;                      \
    }; \
  public:                                              \
    virtual result name##args

#define DEFINE_FUNC_PART2(size) \
  {                             \
    int o,s = size;             \
    __asm {                     \
      __asm mov [o], esp        \
      __asm sub esp, [s]        \
      __asm and esp, ~15        \
      __asm add esp, [s]        \
    }

#define DEFINE_FUNC_PART3(name,args) \
    m_##name##args;                  \
    __asm {                          \
      __asm mov esp,[o]              \
    }                                \
  }

#define DEFINE_FUNC_ALIGNED0(result, linkage, name) \
    DEFINE_FUNC_PART1(result, linkage, name, ()) \
    DEFINE_FUNC_PART2(0) \
    DEFINE_FUNC_PART3(name,())

#define DEFINE_FUNC_ALIGNED1(result, linkage, name, t1) \
    DEFINE_FUNC_PART1(result, linkage, name, (t1 p1)) \
    DEFINE_FUNC_PART2(ALS(p1)) \
    DEFINE_FUNC_PART3(name,(p1))

#define DEFINE_FUNC_ALIGNED2(result, linkage, name, t1, t2) \
    DEFINE_FUNC_PART1(result, linkage, name, (t1 p1, t2 p2)) \
    DEFINE_FUNC_PART2(ALS(p1)+ALS(p2)) \
    DEFINE_FUNC_PART3(name,(p1, p2))

#define DEFINE_FUNC_ALIGNED3(result, linkage, name, t1, t2, t3) \
    DEFINE_FUNC_PART1(result, linkage, name, (t1 p1, t2 p2, t3 p3)) \
    DEFINE_FUNC_PART2(ALS(p1)+ALS(p2)+ALS(p3)) \
    DEFINE_FUNC_PART3(name,(p1, p2, p3))

#define DEFINE_FUNC_ALIGNED4(result, linkage, name, t1, t2, t3, t4) \
    DEFINE_FUNC_PART1(result, linkage, name, (t1 p1, t2 p2, t3 p3, t4 p4)) \
    DEFINE_FUNC_PART2(ALS(p1)+ALS(p2)+ALS(p3)+ALS(p4)) \
    DEFINE_FUNC_PART3(name,(p1, p2, p3, p4))

#define DEFINE_FUNC_ALIGNED5(result, linkage, name, t1, t2, t3, t4, t5) \
    DEFINE_FUNC_PART1(result, linkage, name, (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5)) \
    DEFINE_FUNC_PART2(ALS(p1)+ALS(p2)+ALS(p3)+ALS(p4)+ALS(p5)) \
    DEFINE_FUNC_PART3(name,(p1, p2, p3, p4, p5))

#define DEFINE_FUNC_ALIGNED6(result, linkage, name, t1, t2, t3, t4, t5, t6) \
    DEFINE_FUNC_PART1(result, linkage, name, (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6)) \
    DEFINE_FUNC_PART2(ALS(p1)+ALS(p2)+ALS(p3)+ALS(p4)+ALS(p5)+ALS(p6)) \
    DEFINE_FUNC_PART3(name,(p1, p2, p3, p4, p5, p6))

#define DEFINE_FUNC_ALIGNED7(result, linkage, name, t1, t2, t3, t4, t5, t6, t7) \
    DEFINE_FUNC_PART1(result, linkage, name, (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7)) \
    DEFINE_FUNC_PART2(ALS(p1)+ALS(p2)+ALS(p3)+ALS(p4)+ALS(p5)+ALS(p6)+ALS(p7)) \
    DEFINE_FUNC_PART3(name,(p1, p2, p3, p4, p5, p6, p7))

#define DEFINE_FUNC_ALIGNED8(result, linkage, name, t1, t2, t3, t4, t5, t6, t7, t8) \
    DEFINE_FUNC_PART1(result, linkage, name, (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8)) \
    DEFINE_FUNC_PART2(ALS(p1)+ALS(p2)+ALS(p3)+ALS(p4)+ALS(p5)+ALS(p6)+ALS(p7)+ALS(p8)) \
    DEFINE_FUNC_PART3(name,(p1, p2, p3, p4, p5, p6, p7, p8))

#define DEFINE_FUNC_ALIGNED9(result, linkage, name, t1, t2, t3, t4, t5, t6, t7, t8, t9) \
    DEFINE_FUNC_PART1(result, linkage, name, (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9)) \
    DEFINE_FUNC_PART2(ALS(p1)+ALS(p2)+ALS(p3)+ALS(p4)+ALS(p5)+ALS(p6)+ALS(p7)+ALS(p8)+ALS(p9)) \
    DEFINE_FUNC_PART3(name,(p1, p2, p3, p4, p5, p6, p7, p8, p9))

#else

#define DEFINE_FUNC_ALIGNED0(result, linkage, name)                                            DEFINE_METHOD_LINKAGE0 (result, linkage, name)
#define DEFINE_FUNC_ALIGNED1(result, linkage, name, t1)                                        DEFINE_METHOD_LINKAGE1 (result, linkage, name, (t1 p1) )
#define DEFINE_FUNC_ALIGNED2(result, linkage, name, t1, t2)                                    DEFINE_METHOD_LINKAGE2 (result, linkage, name, (t1 p1, t2 p2) )
#define DEFINE_FUNC_ALIGNED3(result, linkage, name, t1, t2, t3)                                DEFINE_METHOD_LINKAGE3 (result, linkage, name, (t1 p1, t2 p2, t3 p3) )
#define DEFINE_FUNC_ALIGNED4(result, linkage, name, t1, t2, t3, t4)                            DEFINE_METHOD_LINKAGE4 (result, linkage, name, (t1 p1, t2 p2, t3 p3, t4 p4) )
#define DEFINE_FUNC_ALIGNED5(result, linkage, name, t1, t2, t3, t4, t5)                        DEFINE_METHOD_LINKAGE5 (result, linkage, name, (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5) )
#define DEFINE_FUNC_ALIGNED6(result, linkage, name, t1, t2, t3, t4, t5, t6)                    DEFINE_METHOD_LINKAGE6 (result, linkage, name, (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6) )
#define DEFINE_FUNC_ALIGNED7(result, linkage, name, t1, t2, t3, t4, t5, t6, t7)                DEFINE_METHOD_LINKAGE7 (result, linkage, name, (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7) )
#define DEFINE_FUNC_ALIGNED8(result, linkage, name, t1, t2, t3, t4, t5, t6, t7, t8)            DEFINE_METHOD_LINKAGE8 (result, linkage, name, (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8) )
#define DEFINE_FUNC_ALIGNED9(result, linkage, name, t1, t2, t3, t4, t5, t6, t7, t8, t9)        DEFINE_METHOD_LINKAGE9 (result, linkage, name, (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9) )
#define DEFINE_FUNC_ALIGNED10(result, linkage, name, t1, t2, t3, t4, t5, t6, t7, t8, t10)      DEFINE_METHOD_LINKAGE10(result, linkage, name, (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9, t10 p10) )
#define DEFINE_FUNC_ALIGNED11(result, linkage, name, t1, t2, t3, t4, t5, t6, t7, t8, t10, t11) DEFINE_METHOD_LINKAGE11(result, linkage, name, (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9, t10 p10, t11 p11) )

#endif

///////////////////////////////////////////////////////////
//
//  BEGIN_METHOD_RESOLVE/END_METHOD_RESOLVE
//
//  Defines a method that resolves the exported functions
//  defined with DEFINE_METHOD or DEFINE_METHOD_LINKAGE.
//  There must be a RESOLVE_METHOD or RESOLVE_METHOD_RENAME
//  for each DEFINE_METHOD or DEFINE_METHOD_LINKAGE within this
//  block. This block must be followed by an END_METHOD_RESOLVE.
//
#define BEGIN_METHOD_RESOLVE() \
  protected: \
  virtual bool ResolveExports() \
  {

#define END_METHOD_RESOLVE() \
    return true; \
  }

///////////////////////////////////////////////////////////
//
//  RESOLVE_METHOD
//
//  Resolves a method from a dll
//
//  method: Name of the method defined with DEFINE_METHOD
//          or DEFINE_METHOD_LINKAGE
//
#define RESOLVE_METHOD(method) \
  if (!m_dll->ResolveExport( #method , & m_##method##_ptr )) \
    return false;

#define RESOLVE_METHOD_FP(method) \
  if (!m_dll->ResolveExport( #method , & method##_ptr )) \
    return false;


///////////////////////////////////////////////////////////
//
//  RESOLVE_METHOD_OPTIONAL
//
//  Resolves a method from a dll. does not abort if the
//  method is missing
//
//  method: Name of the method defined with DEFINE_METHOD
//          or DEFINE_METHOD_LINKAGE
//

#define RESOLVE_METHOD_OPTIONAL(method) \
   m_dll->ResolveExport( #method , & m_##method##_ptr );

#define RESOLVE_METHOD_OPTIONAL_FP(method) \
   method##_ptr = NULL; \
   m_dll->ResolveExport( #method , & method##_ptr );



///////////////////////////////////////////////////////////
//
//  RESOLVE_METHOD_RENAME
//
//  Resolves a method from a dll
//
//  dllmethod: Name of the function exported from the dll
//  method: Name of the method defined with DEFINE_METHOD
//          or DEFINE_METHOD_LINKAGE
//
#define RESOLVE_METHOD_RENAME(dllmethod, method) \
  if (!m_dll->ResolveExport( #dllmethod , & m_##method##_ptr )) \
    return false;

#define RESOLVE_METHOD_RENAME_FP(dllmethod, method) \
  if (!m_dll->ResolveExport( #dllmethod , & method##_ptr )) \
    return false;


////////////////////////////////////////////////////////////////////
//
//  Example declaration of a dll wrapper class
//
//  1.  Define a class with pure virtual functions with all functions
//      exported from the dll. This is needed to use the IntelliSence
//      feature of the Visual Studio Editor.
//
//  class DllExampleInterface
//  {
//  public:
//    virtual void foo (unsigned int type, char* szTest)=0;
//    virtual void bar (char* szTest, unsigned int type)=0;
//  };
//
//  2.  Define a class, derived from DllDynamic and the previously defined
//      interface class. Define the constructor of the class using the
//      DECLARE_DLL_WRAPPER macro. Use the DEFINE_METHODX/DEFINE_METHOD_LINKAGEX
//      macros to define the functions from the interface above, where X is number of
//      parameters the function has. The function parameters
//      have to be enclosed in parentheses. The parameter names have to be changed to px
//      where x is the number on which position the parameter appears.
//      Use the RESOLVE_METHOD/RESOLVE_METHOD_RENAME to do the actually resolve the functions
//      from the dll when it's loaded. The RESOLVE_METHOD/RESOLVE_METHOD_RENAME have to
//      be between the BEGIN_METHOD_RESOLVE/END_METHOD_RESOLVE block.
//
//  class DllExample : public DllDynamic, DllExampleInterface
//  {
//    DECLARE_DLL_WRAPPER(DllExample, special://xbmcbin/system/Example.dll)
//    LOAD_SYMBOLS()  // add this if you want to load debug symbols for the dll
//    DEFINE_METHOD2(void, foo, (int p1, char* p2))
//    DEFINE_METHOD_LINKAGE2(void, __stdcall, bar, (char* p1, int p2))
//    DEFINE_METHOD_FP(void, foobar, (int type, char* szTest))  //  No need to define this function in the
//                                                              //  interface class, as it's a function pointer.
//                                                              //  But its not recognised by IntelliSence
//    BEGIN_METHOD_RESOLVE()
//      RESOLVE_METHOD(foo)
//      RESOLVE_METHOD_RENAME("_bar@8", bar)
//      RESOLVE_METHOD_FP(foobar)
//    END_METHOD_RESOLVE()
//  };
//
//  The above macros will expand to a class that will look like this
//
//  class DllExample : public DllDynamic, DllExampleInterface
//  {
//  public:
//    DllExample() : DllDynamic( "special://xbmcbin/system/Example.dll" ) {}
//  protected:
//    virtual bool LoadSymbols() { return true; }
//  protected:
//    typedef void (* foo_METHOD) ( int p1, char* p2 );
//    foo_METHOD m_foo;
//  public:
//    virtual void foo( int p1, char* p2 )
//    {
//      return m_foo(p1, p2);
//    }
//  protected:
//    typedef void (__stdcall * bar_METHOD) ( char* p1, int p2 );
//    bar_METHOD m_bar;
//  public:
//    virtual void bar( char* p1, int p2 )
//    {
//      return m_bar(p1, p2);
//    }
//  protected:
//    typedef void (* foobar_METHOD) (int type, char* szTest);
//  public:
//    foobar_METHOD foobar;
//  protected:
//    virtual bool ResolveExports()
//    {
//      return (
//              m_dll->ResolveExport( "foo", (void**)& m_foo ) &&
//              m_dll->ResolveExport( "_bar@8", (void**)& m_bar ) &&
//              m_dll->ResolveExport( "foobar" , (void**)& foobar ) &&
//             1
//             );
//    }
//  };
//
//  Usage of the class
//
//  DllExample dll;
//  dll.Load();
//  if (dll.IsLoaded())
//  {
//    dll.foo(1, "bar");
//    dll.Unload();
//  }
//

///////////////////////////////////////////////////////////
//
//  Baseclass for a Dynamically loaded dll
//  use the above macros to create a dll wrapper
//
class DllDynamic
{
public:
  DllDynamic();
  DllDynamic(const std::string& strDllName);
  virtual ~DllDynamic();
  virtual bool Load();
  virtual void Unload();
  virtual bool IsLoaded() const { return m_dll!=NULL; }
  bool CanLoad();
  bool EnableDelayedUnload(bool bOnOff);
  bool SetFile(const std::string& strDllName);
  const std::string &GetFile() const { return m_strDllName; }

protected:
  virtual bool ResolveExports()=0;
  virtual bool LoadSymbols() { return false; }
  bool  m_DelayUnload;
  LibraryLoader* m_dll;
  std::string m_strDllName;
};
