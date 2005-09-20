#pragma once
#include "cores/DllLoader/DllLoader.h"

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
//
#define DECLARE_DLL_WRAPPER(classname, dllname) \
public: \
  classname##() : DllDynamic( #dllname ) {}

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
  classname##() {} \

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
#define DEFINE_METHOD_FP(result, name, args) \
  protected: \
    typedef result (* name##_METHOD) args; \
  public: \
    name##_METHOD name;

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

#define DEFINE_METHOD0(result, name) \
  protected: \
    typedef result (* name##_METHOD) (); \
    name##_METHOD m_##name; \
  public: \
    virtual result name() \
    { \
      return m_##name(); \
    }

#define DEFINE_METHOD1(result, name, args) \
  protected: \
    typedef result (* name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1); \
    }

#define DEFINE_METHOD2(result, name, args) \
  protected: \
    typedef result (* name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2); \
    }

#define DEFINE_METHOD3(result, name, args) \
  protected: \
    typedef result (* name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2, p3); \
    }

#define DEFINE_METHOD4(result, name, args) \
  protected: \
    typedef result (* name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2, p3, p4); \
    }

#define DEFINE_METHOD5(result, name, args) \
  protected: \
    typedef result (* name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2, p3, p4, p5); \
    }

#define DEFINE_METHOD6(result, name, args) \
  protected: \
    typedef result (* name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2, p3, p4, p5, p6); \
    }

#define DEFINE_METHOD7(result, name, args) \
  protected: \
    typedef result (* name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2, p3, p4, p5, p6, p7); \
    }

#define DEFINE_METHOD8(result, name, args) \
  protected: \
    typedef result (* name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2, p3, p4, p5, p6, p7, p8); \
    }

#define DEFINE_METHOD9(result, name, args) \
  protected: \
    typedef result (* name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2, p3, p4, p5, p6, p7, p8, p9); \
    }

#define DEFINE_METHOD10(result, name, args) \
  protected: \
    typedef result (* name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10); \
    }

#define DEFINE_METHOD11(result, name, args) \
  protected: \
    typedef result (* name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11); \
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
    name##_METHOD name;

#define DEFINE_METHOD_LINKAGE0(result, linkage, name) \
  protected: \
    typedef result (linkage * name##_METHOD) (); \
    name##_METHOD m_##name; \
  public: \
    virtual result name() \
    { \
      return m_##name(); \
    }

#define DEFINE_METHOD_LINKAGE1(result, linkage, name, args) \
  protected: \
    typedef result (linkage * name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1); \
    }

#define DEFINE_METHOD_LINKAGE2(result, linkage, name, args) \
  protected: \
    typedef result (linkage * name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2); \
    }

#define DEFINE_METHOD_LINKAGE3(result, linkage, name, args) \
  protected: \
    typedef result (linkage * name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2, p3); \
    }

#define DEFINE_METHOD_LINKAGE4(result, linkage, name, args) \
  protected: \
    typedef result (linkage * name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2, p3, p4); \
    }

#define DEFINE_METHOD_LINKAGE5(result, linkage, name, args) \
  protected: \
    typedef result (linkage * name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2, p3, p4, p5); \
    }

#define DEFINE_METHOD_LINKAGE6(result, linkage, name, args) \
  protected: \
    typedef result (linkage * name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2, p3, p4, p5, p6); \
    }

#define DEFINE_METHOD_LINKAGE7(result, linkage, name, args) \
  protected: \
    typedef result (linkage * name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2, p3, p4, p5, p6, p7); \
    }

#define DEFINE_METHOD_LINKAGE8(result, linkage, name, args) \
  protected: \
    typedef result (linkage * name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2, p3, p4, p5, p6, p7, p8); \
    }

#define DEFINE_METHOD_LINKAGE9(result, linkage, name, args) \
  protected: \
    typedef result (linkage * name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2, p3, p4, p5, p6, p7, p8, p9); \
    }

#define DEFINE_METHOD_LINKAGE10(result, linkage, name, args) \
  protected: \
    typedef result (linkage * name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10); \
    }

#define DEFINE_METHOD_LINKAGE11(result, linkage, name, args) \
  protected: \
    typedef result (linkage * name##_METHOD) args; \
    name##_METHOD m_##name; \
  public: \
    virtual result name args \
    { \
      return m_##name(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11); \
    }

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
  { \
    return (

#define END_METHOD_RESOLVE() \
              1 \
              ); \
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
  m_dll->ResolveExport( #method , (void**)& m_##method ) &&

#define RESOLVE_METHOD_FP(method) \
  m_dll->ResolveExport( #method , (void**)& method ) &&

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
  m_dll->ResolveExport( #dllmethod , (void**)& m_##method ) &&

#define RESOLVE_METHOD_RENAME_FP(dllmethod, method) \
  m_dll->ResolveExport( #dllmethod , (void**)& method ) &&


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
//    DECLARE_DLL_WRAPPER(DllExample, Q:\\system\\Example.dll)
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
//    DllExample() : DllDynamic( "Q:\\system\\Example.dll" ) {}
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
//  protected: \
//    typedef void (* foobar_METHOD) (int type, char* szTest); \
//  public: \
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
  DllDynamic(const CStdString& strDllName);
  virtual ~DllDynamic();
  virtual bool Load();
  virtual void Unload();
  bool IsLoaded() { return m_dll!=NULL; }
  bool CanLoad();
  bool EnableDelayedUnload(bool bOnOff);
  bool SetFile(const CStdString& strDllName);

protected:
  virtual bool ResolveExports()=0;
  bool  m_DelayUnload;
  DllLoader* m_dll;
  CStdString m_strDllName;
};
