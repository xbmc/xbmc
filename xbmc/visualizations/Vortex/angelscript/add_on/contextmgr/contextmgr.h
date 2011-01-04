#ifndef CONTEXTMGR_H
#define CONTEXTMGR_H

// The context manager simplifies the management of multiple concurrent scripts

// More than one context manager can be used, if you wish to control different
// groups of scripts separately, e.g. game object scripts, and GUI scripts.

// OBSERVATION: This class is currently not thread safe.

#include <angelscript.h>
#include <vector>

BEGIN_AS_NAMESPACE

class CScriptAny;

// The internal structure for holding contexts
struct SContextInfo;

// The signature of the get time callback function
typedef asUINT (*TIMEFUNC_t)();

class CContextMgr
{
public:
    CContextMgr();
    ~CContextMgr();

	// Set the function that the manager will use to obtain the time in milliseconds
    void SetGetTimeCallback(TIMEFUNC_t func);

    // Registers the script function
	//
	//  void sleep(uint milliseconds)
	//
    // The application must set the get time callback for this to work
    void RegisterThreadSupport(asIScriptEngine *engine);

	// Registers the script functions
	//
	//  void createCoRoutine(const string &in functionName, any @arg)
	//  void yield()
	void RegisterCoRoutineSupport(asIScriptEngine *engine);

	// Create a new context, prepare it with the function id, then return 
	// it so that the application can pass the argument values. The context
	// will be released by the manager after the execution has completed.
    asIScriptContext *AddContext(asIScriptEngine *engine, int funcId);

	// Create a new context, prepare it with the function id, then return
	// it so that the application can pass the argument values. The context
	// will be added as a co-routine in the same thread as the currCtx.
	asIScriptContext *AddContextForCoRoutine(asIScriptContext *currCtx, int funcId);

	// Execute each script that is not currently sleeping. The function returns after 
	// each script has been executed once. The application should call this function
	// for each iteration of the message pump, or game loop, or whatever.
    void ExecuteScripts();

	// Put a script to sleep for a while
    void SetSleeping(asIScriptContext *ctx, asUINT milliSeconds);

	// Switch the execution to the next co-routine in the group.
	// Returns true if the switch was successful.
	void NextCoRoutine();

	// Abort all scripts
    void AbortAll();

protected:
	std::vector<SContextInfo*> threads;
	std::vector<SContextInfo*> freeThreads;
	asUINT                     currentThread;
    TIMEFUNC_t                 getTimeFunc;
};


END_AS_NAMESPACE

#endif
