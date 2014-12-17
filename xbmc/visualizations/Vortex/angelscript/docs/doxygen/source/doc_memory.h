/**

\page doc_memory Memory management

This article will explain the memory management in AngelScript in detail.
It's probably more detail than most developers need to know, but some may want
to know exactly how it works in order to evaluate AngelScript better.




\section doc_memory_1 Overview of the memory management

AngelScript uses a hybrid method with both reference counting and garbage
collection, where reference counting is the main method and the garbage
collection is only for backup when circular references needs to be resolved.

This design was chosen because reference counting is the easiest way of
passing objects between script engine and application while still keeping
track of live objects. If pure garbage collection was used, the script engine
would have to know about the entire memory space of the application where
script objects could be stored, otherwise it wouldn't be able to know if the
objects are still alive or not.

The garbage collector is only used for those object types that have a
possibility of forming a circular reference with itself or other objects, e.g.
objects that have a member variable as a handle to the same type as itself.
Whenever such an object is created, the garbage collector is informed so that
it can keep track of the object.

The garbage collector is executed manually because the application
will want to control when that extra processing should be done, usually at
idle moments of the application execution. The garbage collector in
AngelScript is also incremental, so that it can be executed in tiny steps
throughout the application execution.

\see \ref doc_gc




\section doc_memory_2 Reference counting algorithm

All built-in script objects are automatically reference counted, and the
application just need to make sure to call the AddRef and Release methods
appropriately if it is storing any references to the objects outside the
script engine.

Application registered objects may or may not be reference counted. Those
that should be reference counted must implement the ADDREF and RELEASE
behaviours so that AngelScript can notify the objects of the changes to the
reference count. The application needs keep track of the reference counter
itself and free the memory when there are no more references to the object.
This is usually done by having an integer member variable in the object
structure itself to hold the reference count. It can also be implemented with
a separate structure for holding the reference counter, but it adds extra
overhead as the reference counter must be searched for with each change.

\see \ref doc_register_type





\section doc_memory_3 Garbage collector algorithm

The garbage collector, used to handle the scenarios where reference counting
isn't enough, uses the following algorithm.

<ol>

<li><b>Destroy garbage:</b> Free all objects with only one reference. This
    reference is held by the GC itself so therefore it is known that nobody
    else is referencing the object.

<li><b>Clear counters:</b> Clear the GC counter for the remaining objects.
    This counter will be used to count the references to the object reachable
    from the GC. At the same time the objects are also flagged, so that we can
    detect if the references change while the garbage collector is running.
    This flag is automatically cleared when incrementing or decrementing the
    reference counter.

<li><b>Count GC references:</b> For each of the objects in the GC, tell it to
    increment the GC counter for all objects it holds references to. This is
    only done for objects still flagged, because if the flag is no longer set
    we know it has been referenced by the application, thus it is considered
    live along with all objects it holds references to.

<li><b>Mark live objects:</b> Build a list of all objects in the GC that are
    not flagged or that doesn't have the GC count equal to the reference
    counter. These are objects that are considered live. For each of the
    objects in the list, add all of the references in the object to the list
    as well, unless they are already in the list. As the list is traversed it
    will grow with new objects, these objects will also have their references
    added to the list. This way we're following the chain of references so
    we're gathering all objects that are alive.

<li><b>Verify unmarked objects:</b> Make one more pass over the list of
    objects in the GC to see if any of the objects that were not marked as
    alive has had the flag cleared, if it has then the run the 'mark live
    objects' step again.

<li><b>Break circular references:</b> All objects that have not been marked as
    alive by this time are involved in unreachable circular references and must be
    destroyed. The objects are destroyed by telling the objects to release the
    references they hold, thus breaking the circular references. When all
    circular references have been broken, the entire GC routine is restarted
    which will free the objects. If there were no dead objects, then the GC
    is finished.

</ol>

All of the steps, except 'verify unmarked objects' are incremental, i.e. they 
can be interrupted to allow the application and scripts to execute before 
continuing the garbage collection. Step 1 can also be executed individually
at any time during the cycle, this permits to free up memory for objects that
are not involved in cyclic memory without having to wait for the detection cycle
to complete.

The application should ideally invoke the garbage collector every once in
a while to make sure too much garbage isn't accumulated over long periods.

It may also be a good idea to do a complete run of the GC when it doesn't
matter if the application pauses for a little while, for example when in menu
mode.

The garbage collector can also take care of application registered types
if the application registers the appropriate object behaviours.

\see \ref doc_gc_object

\section doc_memory_4 Memory heap

By default AngelScript uses the memory heap accessed through the standard
malloc and free global functions when allocating memory. Usually this means
that AngelScript shares the memory heap with the application, though if
the AngelScript library is compiled as a DLL or a shared object the heap may
actually be separate from the application heap.

Some applications need extra control over the memory heap that the standard
C/C++ library doesn't provide. This is common with video games for consoles,
that have limited memory and cannot afford loosing space due to fragmentation
or many small allocations. AngelScript aids these applications as well as it
is possible to register custom memory allocation routines with the library,
giving the application exact control over the memory used by AngelScript.

\see \ref asSetGlobalMemoryFunctions


*/
