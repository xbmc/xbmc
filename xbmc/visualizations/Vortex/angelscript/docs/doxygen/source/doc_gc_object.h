/**

\page doc_gc_object Garbage collected objects

Reference counting as memory management has a drawback in that it is
difficult to detect circular references when determining dead objects.
AngelScript allows the application to register types with special behaviours
to support the garbage collection for detecting circular references. These
behaviours make the class a bit more complex, but you should only have to
register them for a few types, e.g. generic container classes.

\code
// Registering the garbage collected reference type
r = engine->RegisterObjectType("ref", 0, asOBJ_REF | asOBJ_GC); assert( r >= 0 );
\endcode

The difference between the garbage collected and non-garbage collected
types is in the addref and release behaviours, the class constructor, and
the extra support behaviours.

\see The \ref doc_addon_dict "dictionary" add-on for an example of a garbage collected object

\section doc_reg_gcref_1 GC support behaviours

The GC determines when objects should be destroyed by counting the
references it can follow for each object. If the GC can see all references
that points to an object, it knows that the object is part of a circular
reference. If all the objects involved in that circular reference have no
outside references it means that they should be destroyed.

The process of determining the dead objects uses the first for of the
behaviours below, while the destruction of the objects is done by forcing the
release of the object's references.

\code
void CGCRef::SetGCFlag()
{
    // Set the gc flag as the high bit in the reference counter
    refCount |= 0x80000000;
}

bool CGCRef::GetGCFlag()
{
    // Return the gc flag
    return (refCount & 0x80000000) ? true : false;
}

int CGCRef::GetRefCount()
{
    // Return the reference count, without the gc flag
    return (refCount & 0x7FFFFFFF);
}

void CGCRef::EnumReferences()
{
    // Call the engine::GCEnumCallback for all references to other objects held
    engine->GCEnumCallback(myref);
}

void CGCRef::ReleaseAllReferences()
{
    // When we receive this call, we are as good as dead, but
    // the garbage collector will still hold a references to us, so we
    // cannot just delete ourself yet. Just free all references to other
    // objects that we hold
    if( myref )
    {
        myref->Release();
        myref = 0;
    }
}

// Register the GC support behaviours
r = engine->RegisterObjectBehaviour("gc", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(CGCRef,SetGCFlag), asCALL_THISCALL); assert( r >= 0 );
r = engine->RegisterObjectBehaviour("gc", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(CGCRef,GetGCFlag), asCALL_THISCALL); assert( r >= 0 );
r = engine->RegisterObjectBehaviour("gc", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(CGCRef,GetRefCount), asCALL_THISCALL); assert( r >= 0 );
r = engine->RegisterObjectBehaviour("gc", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(CGCRef,EnumReferences), asCALL_THISCALL); assert( r >= 0 );
r = engine->RegisterObjectBehaviour("gc", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(CGCRef,ReleaseAllReferences), asCALL_THISCALL); assert( r >= 0 );
\endcode

\section doc_reg_gcref_2 Factory for garbage collection

Whenever a garbage collected class is created, the garbage collector must
be notified of it's existence. The easiest way of doing that is to have the
factory behaviour, or the class constructor call the
<code>NotifyGarbageCollectorOfNewObject()</code> method on the engine when initializing the
class.

\code
CGCRef *GCRef_Factory()
{
    // Create the object and then notify the GC of its existence
    CGCRef *obj = new CGCRef();
    int typeId = engine->GetTypeIdByDecl("gc");
    engine->NotifyGarbageCollectorOfNewObject(obj, typeId);
    return obj;
}
\endcode

You may want to consider caching the typeId, so that it doesn't have to be
looked up through the relatively expensive call to GetTypeIdByDecl every time
an object of this type is created.

Note, if you create objects of this type from the application side, you
must also notify the garbage collector of its existence, so it's a good idea
to make sure all code use the same way of creating objects of this type.

\section doc_reg_gcref_3 Addref and release for garbage collection

For garbage collected objects it is important to make sure the AddRef and
Release behaviours clear the GC flag. Otherwise it is possible that the GC
incorrectly determine that the object should be destroyed.

\code
void CGCRef::AddRef()
{
    // Clear the gc flag and increase the reference counter
    refCount = (refCount&0x7FFFFFFF) + 1;
}

void CGCRef::Release()
{
    // Clear the gc flag, decrease ref count and delete if it reaches 0
    refCount &= 0x7FFFFFFF;
    if( --refCount == 0 )
        delete this;
}

// Registering the addref/release behaviours
r = engine->RegisterObjectBehaviour("gc", asBEHAVE_ADDREF, "void f()", asMETHOD(CGCRef,AddRef), asCALL_THISCALL); assert( r >= 0 );
r = engine->RegisterObjectBehaviour("gc", asBEHAVE_RELEASE, "void f()", asMETHOD(CGCRef,Release), asCALL_THISCALL); assert( r >= 0 );
\endcode








*/
