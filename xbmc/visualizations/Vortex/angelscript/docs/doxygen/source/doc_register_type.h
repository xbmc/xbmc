/**

\page doc_register_type Registering an object type


The are two principal paths to take when registering a new type, either the
type is a reference type that is located in dynamic memory, or the type is a
value type that is located on the stack. Complex types are usually registered
as reference types, while simple types that are meant to be used as primitives
are registered as value types. A reference type support object handles (unless restricted by application), but
cannot be passed by value to application registered functions, a value type
doesn't support handles and can be passed by value to application registered
functions.

 - \subpage doc_reg_basicref
 - \subpage doc_register_val_type
 - \subpage doc_reg_opbeh
 - \subpage doc_reg_objmeth
 - \subpage doc_reg_objprop







\page doc_reg_basicref Registering a reference type

The basic reference type should be registered with the following behaviours:
\ref asBEHAVE_FACTORY, \ref asBEHAVE_ADDREF, and \ref asBEHAVE_RELEASE. 

\code
// Registering the reference type
r = engine->RegisterObjectType("ref", 0, asOBJ_REF); assert( r >= 0 );
\endcode

\see The \ref doc_addon_string "string" add-on for an example of a reference type.

\see \ref doc_gc_object, \ref doc_adv_class_hierarchy, \ref doc_adv_scoped_type, and \ref doc_adv_single_ref_type for more advanced types.


\section doc_reg_basicref_1 Factory function

The factory function is the one that AngelScript will use to instanciate
objects of this type when a variable is declared. It is responsible for
allocating and initializing the object memory.

The default factory function doesn't take any parameters and should return
an object handle for the new object. Make sure the object's reference counter
is accounting for the reference being returned by the factory function, so
that the object is properly released when all references to it are removed.

\code
CRef::CRef()
{
    // Let the constructor initialize the reference counter to 1
    refCount = 1;
}

CRef *Ref_Factory()
{
    // The class constructor is initializing the reference counter to 1
    return new CRef();
}

// Registering the factory behaviour
r = engine->RegisterObjectBehaviour("ref", asBEHAVE_FACTORY, "ref@ f()", asFUNCTION(Ref_Factory), asCALL_CDECL); assert( r >= 0 );
\endcode

You may also register factory functions that take parameters, which may
then be used when initializing the object.

The factory function must be registered as a global function, but can be
implemented as a static class method, common global function, or a global
function following the generic calling convention.

\section doc_reg_basicref_2 Addref and release behaviours

\code
void CRef::Addref()
{
    // Increase the reference counter
    refCount++;
}

void CRef::Release()
{
    // Decrease ref count and delete if it reaches 0
    if( --refCount == 0 )
        delete this;
}

// Registering the addref/release behaviours
r = engine->RegisterObjectBehaviour("ref", asBEHAVE_ADDREF, "void f()", asMETHOD(CRef,AddRef), asCALL_THISCALL); assert( r >= 0 );
r = engine->RegisterObjectBehaviour("ref", asBEHAVE_RELEASE, "void f()", asMETHOD(CRef,Release), asCALL_THISCALL); assert( r >= 0 );
\endcode




\section doc_reg_noinst Registering an uninstanciable reference type

Sometimes it may be useful to register types that cannot be instanciated by
the scripts, yet can be interacted with. You can do this by registering the
type as a normal reference type, but omit the registration of the factory
behaviour. You can later register global properties, or functions that allow the
scripts to access objects created by the application via object handles.

This would be used when the application has a limited number of objects
available and doesn't want to create new ones. For example singletons, or
pooled objects.












\page doc_register_val_type Registering a value type

When registering a value type, the size of the type must be given so that AngelScript knows how much space is needed for it.
If the type doesn't require any special treatment, i.e. doesn't contain any pointers or other resource references that must be
maintained, then the type can be registered with the flag \ref asOBJ_POD. In this case AngelScript doesn't require the default
constructor, assignment behaviour, or destructor as it will be able to automatically handle these cases the same way it handles
built-in primitives.

If you plan on passing the or returning the type by value to registered functions that uses native calling convention, you also
need to inform \ref doc_reg_val_2 "how the type is implemented in the application", but if you only plan on using generic
calling conventions, or don't pass these types by value then you don't need to worry about that.



\code
// Register a primitive type, that doesn't need any special management of the content
r = engine->RegisterObjectType("pod", sizeof(pod), asOBJ_VALUE | asOBJ_POD); assert( r >= 0 );

// Register a class that must be properly initialized and uninitialized
r = engine->RegisterObjectType("val", sizeof(val), asOBJ_VALUE); assert( r >= 0 );
\endcode

\see The \ref doc_addon_std_string or \ref doc_addon_math3d "vector3" add-on for examples of value types


\section doc_reg_val_1 Constructor and destructor

If a constructor or destructor is needed they shall be registered the following way:

\code
void Constructor(void *memory)
{
  // Initialize the pre-allocated memory by calling the
  // object constructor with the placement-new operator
  new(memory) Object();
}

void Destructor(void *memory)
{
  // Uninitialize the memory by calling the object destructor
  ((Object*)memory)->~Object();
}

// Register the behaviours
r = engine->RegisterObjectBehaviour("val", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Constructor), asCALL_CDECL_OBJLAST); assert( r >= 0 );
r = engine->RegisterObjectBehaviour("val", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destructor), asCALL_CDECL_OBJLAST); assert( r >= 0 );
\endcode




\section doc_reg_val_2 Value types and native calling conventions

If the type will be passed to and from the application by value using native calling conventions, it is important to inform
AngelScript of its real type in C++, otherwise AngelScript won't be able to determine exactly how C++ is treating the type in
a parameter or return value. 

There are a few different flags for this:

<table border=0 cellspacing=0 cellpadding=0>
<tr><td>\ref asOBJ_APP_CLASS             &nbsp; </td><td>The C++ type is a class, struct, or union</td></tr>
<tr><td>\ref asOBJ_APP_CLASS_CONSTRUCTOR &nbsp; </td><td>The C++ type has a defined constructor</td></tr>
<tr><td>\ref asOBJ_APP_CLASS_DESTRUCTOR  &nbsp; </td><td>The C++ type has a defined destructor</td></tr>
<tr><td>\ref asOBJ_APP_CLASS_ASSIGNMENT  &nbsp; </td><td>The C++ type has a defined assignment operator</td></tr>
<tr><td>\ref asOBJ_APP_PRIMITIVE         &nbsp; </td><td>The C++ type is a C++ primitive, but not a float or double</td></tr>
<tr><td>\ref asOBJ_APP_FLOAT             &nbsp; </td><td>The C++ type is a float or double</td></tr>
</table>

Note that these don't represent how the type will behave in the script language, only what the real type is in the host 
application. So if you want to register a C++ class that you want to behave as a primitive type in the script language
you should still use the flag \ref asOBJ_APP_CLASS. The same thing for the flags to identify that the class has a constructor, 
destructor, or assignment. These flags tell AngelScript that the class has the respective function, but not that the type
in the script language should have these behaviours.

For class types there are also a shorter form of the flags for each combination of the 4 flags. They are of the form \ref asOBJ_APP_CLASS_CDA, 
where the existance of the last letters determine if the constructor, destructor, and/or assignment behaviour are available. For example
\ref asOBJ_APP_CLASS_CDA is defined as \ref asOBJ_APP_CLASS | \ref asOBJ_APP_CLASS_CONSTRUCTOR | \ref asOBJ_APP_CLASS_DESTRUCTOR | \ref asOBJ_APP_CLASS_ASSIGNMENT.

\code
// Register a complex type that will be passed by value to the application
r = engine->RegisterObjectType("complex", sizeof(complex), asOBJ_VALUE | asOBJ_APP_CLASS_CDA); assert( r >= 0 );
\endcode

Make sure you inform these flags correctly, because if you do not you may get various errors when executing the scripts. 
Common problems are stack corruptions, and invalid memory accesses. In some cases you may face more silent errors that
may be difficult to detect, e.g. the function is not returning the expected values.





\page doc_reg_opbeh Registering operator behaviours

In order for AngelScript to know how to work with the application registered types, it is 
necessary to register some behaviours, for example for memory management.

The memory management behaviours are described with the registeration of registering 
\ref doc_reg_basicref "reference types" and \ref doc_register_val_type "value types".

Other advanced behaviours are described with the \ref doc_advanced_api "advanced types".

Only a few operators have special behaviours for them, the other operators are registered as 
ordinary \ref doc_script_class_ops "class methods with predefined names".

\section doc_reg_opbeh_1 Index operator

The index operator is usually used to access an element by index, e.g. the elements of an array.

\code
// Simple implementation of the index operator
int &MyClass::operator[] (int index)
{
  return internal_array[index];
}

// Non-mutable variant that works on const references to the object
const int &MyClass::operator[] (int index) const
{
  return internal_array[index];
}

// Register both the const and non-const alternative for const correctness
r = engine->RegisterObjectBehaviour("mytype", asBEHAVE_INDEX, "int &f(int)", asMETHODPR(MyClass, operator[], (int), int&), asCALL_THISCALL); assert( r >= 0 );
r = engine->RegisterObjectBehaviour("mytype", asBEHAVE_INDEX, "const int &f(int) const", asMETHODPR(MyClass, operator[], (int) const, const int&), asCALL_THISCALL); assert( r >= 0 );
\endcode

\section doc_reg_opbeh_2 Value cast operators

The value cast operators are used to allow the scripts to convert an object type to another 
type by constructing a new value. This is different from a \ref doc_adv_class_hierarchy "reference cast",
that do not construct new values, but rather changes the way it is perceived.

By registering the behaviour either as \ref asBEHAVE_VALUE_CAST or \ref asBEHAVE_IMPLICIT_VALUE_CAST you
let AngelScript know whether the behaviour may be used to implicitly cast the type or not.

\code
// Convert a string to an int
int ConvStringToInt(const std::string &s)
{
  return atoi(s.c_str());
}

// Register the behaviour
r = engine->RegisterObjectBehaviour("string", asBEHAVE_VALUE_CAST, "int f() const", asFUNCTION(ConvStringToInt), asCALL_CDECL_OBJLAST); assert( r >= 0 );
\endcode

The return type for the cast behaviour can be any type except bool and void. The value cast is meant to create a new value, so if the function
returns a reference or an object handle make sure it points to a new value and not the original one.

The object constructors and factories also serve as alternative explicit value cast operators, so if a constructor or factory is already available
then there is no need to register the explicit value cast operator. 














\page doc_reg_objmeth Registering object methods

Class methods are registered with the RegisterObjectMethod call.

\code
// Register a class method
void MyClass::ClassMethod()
{
  // Do something
}

r = engine->RegisterObjectMethod("mytype", "void ClassMethod()", asMETHOD(MyClass,ClassMethod), asCALL_THISCALL); assert( r >= 0 );
\endcode

It is also possible to register a global function that takes a pointer to
the object as a class method. This can be used to extend the functionality of
a class when accessed via AngelScript, without actually changing the C++
implementation of the class.

\code
// Register a global function as an object method
void MyClass_MethodWrapper(MyClass *obj)
{
  // Access the object
  obj->DoSomething();
}

r = engine->RegisterObjectMethod("mytype", "void MethodWrapper()", asFUNCTION(MyClass_MethodWrapper), asCALL_CDECL_OBJLAST); assert( r >= 0 );
\endcode

\page doc_reg_objprop Registering object properties

Class member variables can be registered so that they can be directly
accessed by the script without the need for any method calls.

\code
struct MyStruct
{
  int a;
};

r = engine->RegisterObjectProperty("mytype", "int a", offsetof(MyStruct,a)); assert( r >= 0 );
\endcode

offsetof() is a macro declared in stddef.h header file.

It is also possible to expose properties through \ref doc_script_class_prop "property accessors", 
which are a pair of class methods for getting and setting the property value. This is especially
useful when the offset of the property cannot be determined, or if the type of the property is 
not registered in the script and some translation must occur, i.e. from <tt>char*</tt> to <tt>string</tt>.






*/
