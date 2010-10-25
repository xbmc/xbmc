#include <assert.h>
#include <string>

#include "contextmgr.h"

using namespace std;

// TODO: Should have a pool of free asIScriptContext so that new contexts
//       won't be allocated every time. The application must not keep 
//       its own references, instead it must tell the context manager
//       that it is using the context. Otherwise the context manager may
//       think it can reuse the context too early.

// TODO: Need to have a callback for when scripts finishes, so that the 
//       application can receive return values.

BEGIN_AS_NAMESPACE

struct SContextInfo
{
    asUINT sleepUntil;
	vector<asIScriptContext*> coRoutines;
	asUINT currentCoRoutine;
};

static CContextMgr *g_ctxMgr = 0;
static void ScriptSleep(asUINT milliSeconds)
{
	// Get a pointer to the context that is currently being executed
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx && g_ctxMgr )
	{
		// Suspend its execution. The VM will continue until the current 
		// statement is finished and then return from the Execute() method
		ctx->Suspend();

		// Tell the context manager when the context is to continue execution
		g_ctxMgr->SetSleeping(ctx, milliSeconds);
	}
}

static void ScriptYield()
{
	// Get a pointer to the context that is currently being executed
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx && g_ctxMgr )
	{
		// Let the context manager know that it should run the next co-routine
		g_ctxMgr->NextCoRoutine();

		// The current context must be suspended so that VM will return from 
		// the Execute() method where the context manager will continue.
		ctx->Suspend();
	}
}

void ScriptCreateCoRoutine(string &func, CScriptAny *arg)
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx && g_ctxMgr )
	{
		asIScriptEngine *engine = ctx->GetEngine();
		string mod = engine->GetFunctionDescriptorById(ctx->GetCurrentFunction())->GetModuleName();

		// We need to find the function that will be created as the co-routine
		string decl = "void " + func + "(any @)"; 
		int funcId = engine->GetModule(mod.c_str())->GetFunctionIdByDecl(decl.c_str());
		if( funcId < 0 )
		{
			// No function could be found, raise an exception
			ctx->SetException(("Function '" + decl + "' doesn't exist").c_str());
			return;
		}

		// Create a new context for the co-routine
		asIScriptContext *coctx = g_ctxMgr->AddContextForCoRoutine(ctx, funcId);

		// Pass the argument to the context
		coctx->SetArgObject(0, arg);
	}
}

CContextMgr::CContextMgr()
{
    getTimeFunc   = 0;
	currentThread = 0;
}

CContextMgr::~CContextMgr()
{
	asUINT n;

	// Free the memory
	for( n = 0; n < threads.size(); n++ )
	{
		if( threads[n] )
		{
			for( asUINT c = 0; c < threads[n]->coRoutines.size(); c++ )
			{
				if( threads[n]->coRoutines[c] )
					threads[n]->coRoutines[c]->Release();
			}

			delete threads[n];
		}
	}

	for( n = 0; n < freeThreads.size(); n++ )
	{
		if( freeThreads[n] )
		{
			assert( freeThreads[n]->coRoutines.size() == 0 );

			delete freeThreads[n];
		}
	}
}

void CContextMgr::ExecuteScripts()
{
	g_ctxMgr = this; 

	// TODO: Should have a time out. And if not all scripts executed before the 
	//       time out, the next time the function is called the loop should continue
	//       where it left off.

	// TODO: We should have a per thread time out as well. When this is used, the yield
	//       call should resume the next coroutine immediately if there is still time

	// Check if the system time is higher than the time set for the contexts
	asUINT time = getTimeFunc ? getTimeFunc() : asUINT(-1);
	for( currentThread = 0; currentThread < threads.size(); currentThread++ )
	{
		if( threads[currentThread]->sleepUntil < time )
		{
			// TODO: Only allow each thread to execute for a while

			int currentCoRoutine = threads[currentThread]->currentCoRoutine;
			int r = threads[currentThread]->coRoutines[currentCoRoutine]->Execute();
			if( r != asEXECUTION_SUSPENDED )
			{
				// The context has terminated execution (for one reason or other)
				threads[currentThread]->coRoutines[currentCoRoutine]->Release();
				threads[currentThread]->coRoutines[currentCoRoutine] = 0;

				threads[currentThread]->coRoutines.erase(threads[currentThread]->coRoutines.begin() + threads[currentThread]->currentCoRoutine);
				if( threads[currentThread]->currentCoRoutine >= threads[currentThread]->coRoutines.size() )
					threads[currentThread]->currentCoRoutine = 0;

				// If this was the last co-routine terminate the thread
				if( threads[currentThread]->coRoutines.size() == 0 )
				{
					freeThreads.push_back(threads[currentThread]);
					threads.erase(threads.begin() + currentThread);
					currentThread--;
				}
			}
		}
	}

	g_ctxMgr = 0;
}

void CContextMgr::NextCoRoutine()
{
	threads[currentThread]->currentCoRoutine++;
	if( threads[currentThread]->currentCoRoutine >= threads[currentThread]->coRoutines.size() )
		threads[currentThread]->currentCoRoutine = 0;
}

void CContextMgr::AbortAll()
{
	// Abort all contexts and release them. The script engine will make 
	// sure that all resources held by the scripts are properly released.

	for( asUINT n = 0; n < threads.size(); n++ )
	{
		for( asUINT c = 0; c < threads[n]->coRoutines.size(); c++ )
		{
			if( threads[n]->coRoutines[c] )
			{
				threads[n]->coRoutines[c]->Abort();
				threads[n]->coRoutines[c]->Release();
				threads[n]->coRoutines[c] = 0;
			}
		}
		threads[n]->coRoutines.resize(0);

		freeThreads.push_back(threads[n]);
	}

	threads.resize(0);

	currentThread = 0;
}

asIScriptContext *CContextMgr::AddContext(asIScriptEngine *engine, int funcId)
{
	// Create the new context
	asIScriptContext *ctx = engine->CreateContext();
	if( ctx == 0 )
		return 0;

	// Prepare it to execute the function
	int r = ctx->Prepare(funcId);
	if( r < 0 )
	{
		ctx->Release();
		return 0;
	}

	// Add the context to the list for execution
	SContextInfo *info = 0;
	if( freeThreads.size() > 0 )
	{
		info = *freeThreads.rbegin();
		freeThreads.pop_back();
	}
	else
	{
		info = new SContextInfo;
	}

    info->coRoutines.push_back(ctx);
	info->currentCoRoutine = 0;
    info->sleepUntil = 0;
	threads.push_back(info);

	return ctx;
}

asIScriptContext *CContextMgr::AddContextForCoRoutine(asIScriptContext *currCtx, int funcId)
{
	asIScriptEngine *engine = currCtx->GetEngine();
	asIScriptContext *coctx = engine->CreateContext();
	if( coctx == 0 )
	{
		return 0;
	}

	// Prepare the context
	int r = coctx->Prepare(funcId);
	if( r < 0 )
	{
		// Couldn't prepare the context
		coctx->Release();
		return 0;
	}

	// Find the current context thread info
	// TODO: Start with the current thread so that we can find the group faster
	for( asUINT n = 0; n < threads.size(); n++ )
	{
		if( threads[n]->coRoutines[threads[n]->currentCoRoutine] == currCtx )
		{
			// Add the coRoutine to the list
			threads[n]->coRoutines.push_back(coctx);
		}
	}

	return coctx; 
}

void CContextMgr::SetSleeping(asIScriptContext *ctx, asUINT milliSeconds)
{
    assert( getTimeFunc != 0 );
    
	// Find the context and update the timeStamp  
	// for when the context is to be continued

	// TODO: Start with the current thread

	for( asUINT n = 0; n < threads.size(); n++ )
	{
		if( threads[n]->coRoutines[threads[n]->currentCoRoutine] == ctx )
		{
			threads[n]->sleepUntil = (getTimeFunc ? getTimeFunc() : 0) + milliSeconds;
		}
	}
}

void CContextMgr::RegisterThreadSupport(asIScriptEngine *engine)
{
	int r;

    // Must set the get time callback function for this to work
    assert( getTimeFunc != 0 );
    
    // Register the sleep function
    r = engine->RegisterGlobalFunction("void sleep(uint)", asFUNCTION(ScriptSleep), asCALL_CDECL); assert( r >= 0 );

	// TODO: Add support for spawning new threads, waiting for signals, etc
}

void CContextMgr::RegisterCoRoutineSupport(asIScriptEngine *engine)
{
	int r;

	r = engine->RegisterGlobalFunction("void yield()", asFUNCTION(ScriptYield), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void createCoRoutine(const string &in, any @+)", asFUNCTION(ScriptCreateCoRoutine), asCALL_CDECL); assert( r >= 0 );
}

void CContextMgr::SetGetTimeCallback(TIMEFUNC_t func)
{
	getTimeFunc = func;
}

END_AS_NAMESPACE
