/**

\page doc_adv_scoped_type Registering a scoped reference type

Some C++ value types have special requirements for the memory where they
are located, e.g. specific alignment needs, or memory pooling. Since
AngelScript doesn't provide that much control over where and how value types
are allocated, they must be registered as reference types. In this case you'd
register the type as a scoped reference type.

A scoped reference type will have the life time controlled by the scope of
the variable that instanciate it, i.e. as soon as the variable goes out of
scope the instance is destroyed. This means that the type doesn't permit
handles to be taken for the type.

A scoped reference type requires only that the release behaviour is registered. 
The addref behaviour is not permitted. If the factory behaviour is not registered
the script will not be able to instanciate objects of this type, but it can
still receive them as parameters from the application.

Since no handles can be taken for the object type, there is no need to keep
track of the number of references held to the object. This means that the
release behaviour should simply destroy and deallocate the object as soon as
it's called.

\code
scoped *Scoped_Factory()
{
  return new scoped;
}

void Scoped_Release(scoped *s)
{
  if( s ) delete s;
}

// Registering a scoped reference type
r = engine->RegisterObjectType("scoped", 0, asOBJ_REF | asOBJ_SCOPED); assert( r >= 0 );
r = engine->RegisterObjectBehaviour("scoped", asBEHAVE_FACTORY, "scoped @f()", asFUNCTION(Scoped_Factory), asCALL_CDECL); assert( r >= 0 );
r = engine->RegisterObjectBehaviour("scoped", asBEHAVE_RELEASE, "void f()", asFUNCTION(Scoped_Release), asCALL_CDECL_OBJLAST); assert( r >= 0 );
\endcode


\see \ref doc_reg_basicref







*/
