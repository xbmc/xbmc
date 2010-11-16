/**

\page doc_gc Garbage collection

Though AngelScript uses reference counting for memory management, there is still need for a garbage collector to 
take care of the few cases where circular referencing between objects prevents the reference counter from reaching zero.
As the application wants to guarantee responsiveness of the application, AngelScript doesn't invoke the garbage collector
automatically. For that reason it is important that the application do this manually at convenient times.

The garbage collector implemented in AngelScript is incremental, so it can be executed for short periods of time without
requiring the entire application to halt. For this reason, it is recommended that a call to \ref 
asIScriptEngine::GarbageCollect "GarbageCollect"(\ref asGC_ONE_STEP) is made at least once during the normal event processing. This will
execute one step in the incremental process, eventually detecting and destroying objects being kept alive due to circular 
references.

This may not be enough for all applications though, as some script may produce more garbage than others. Performing the 
garbage collection only one step at a time may not be fast enough to free the old garbage before the new garbage is generated.
For this reason it is recommended that the application monitor the statistics for the garbage collector and adjust the 
frequency of the calls as necessary. The statistics is obtained through a call to \ref asIScriptEngine::GetGCStatistics "GetGCStatistics",
which returns the number of objects currently known to the garbage collector as well as the number of objects that have been
destroyed and the number of object that have been detected as garbage with circular references. 

If the scripts produce a lot of garbage but only a low number of garbage in circular references, the application can make a 
call to \ref 
asIScriptEngine::GarbageCollect "GarbageCollect"(\ref asGC_FULL_CYCLE | \ref asGC_DESTROY_GARBAGE), which will only destroy the known garbage without trying
to detect circular references. This call is relatively fast as the garbage collector only has to make a trivial local check
to determine if an object is garbage without circular references.

Finally, if the application goes into a state where responsiveness is not so critical, it might be a good idea to do a full
cycle on the garbage collector, thus cleaning up all garbage at once. To do this, call \ref 
asIScriptEngine::GarbageCollect "GarbageCollect"(\ref asGC_FULL_CYCLE).

\see \ref doc_memory



*/
