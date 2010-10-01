// This sample shows how to use co-routines with AngelScript. Co-routines
// are threads that work together. When one yields the next one takes over.
// This way they are always synchronized, which makes them much easier to
// use than threads that run in parallel.

#include <iostream>  // cout
#include <assert.h>  // assert()
#include <conio.h>   // kbhit(), getch()
#include <windows.h> // timeGetTime()
#include <crtdbg.h>  // debugging routines
#include <list>
#include <angelscript.h>
#include "../../../add_on/scriptstring/scriptstring.h"

using namespace std;

// Function prototypes
void ConfigureEngine(asIScriptEngine *engine);
int  CompileScript(asIScriptEngine *engine);
void PrintString(string &str);
void ScriptCreateCoRoutine(string &func, asIScriptAny *arg);
void ScriptYield();

// This simple manager will let us manage the contexts, 
// in a simple scheme for apparent multithreading
class CContextManager
{
public:
	CContextManager();

	void SetRunningContext(int funcID);
	void AddCoRoutine(asIScriptContext *ctx);
	void NextCoRoutine();
	void ExecuteScripts();
	void AbortAll();

	list<asIScriptContext*> coRoutines;
	list<asIScriptContext*>::iterator currentCtx;
} contextManager;

class asCOutputStream : public asIOutputStream
{
public:
	void Write(const char *text) { printf(text); }
};


asIScriptEngine *engine = 0;
int main(int argc, char **argv)
{
	// Perform memory leak validation in debug mode
	#if defined(_MSC_VER)
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF);
	_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
	#endif

	int r;

	// Create the script engine
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	if( engine == 0 )
	{
		cout << "Failed to create script engine." << endl;
		return -1;
	}

	// The script compiler will send any compiler messages to the outstream
	asCOutputStream out;
	engine->SetCommonMessageStream(&out);

	// Configure the script engine with all the functions, 
	// and variables that the script should be able to use.
	ConfigureEngine(engine);

	// Compile the script code
	r = CompileScript(engine);
	if( r < 0 ) return -1;

	contextManager.SetRunningContext(engine->GetFunctionIDByDecl("script", "void main()"));
	
	// Print some useful information and start the input loop
	cout << "This sample shows how to use co-routines with AngelScript. Co-routines" << endl; 
	cout << "are threads that work together. When one yields the next one takes over." << endl;
	cout << "This way they are always synchronized, which makes them much easier to" << endl;
	cout << "use than threads that run in parallel." << endl;
	cout << "Press any key to abort execution." << endl;

	for(;;)
	{
		// Check if any key was pressed
		if( kbhit() )
		{
			contextManager.AbortAll();
			break;
		}

		// Allow the contextManager to determine which script to execute next
		contextManager.ExecuteScripts();

		// Slow it down a little so that we can see what is happening
		Sleep(100);
	}

	// Release the engine
	engine->Release();

	return 0;
}

void ConfigureEngine(asIScriptEngine *engine)
{
	int r;

	// Register the script string type
	// Look at the implementation for this function for more information  
	// on how to register a custom string type, and other object types.
	// The implementation is in "/add_on/scriptstring/scriptstring.cpp"
	RegisterScriptString(engine);

	// Register the functions that the scripts will be allowed to use
	r = engine->RegisterGlobalFunction("void Print(string &in)", asFUNCTION(PrintString), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void CreateCoRoutine(string &in, any &in)", asFUNCTION(ScriptCreateCoRoutine), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void Yield()", asFUNCTION(ScriptYield), asCALL_CDECL); assert( r >= 0 );
}

int CompileScript(asIScriptEngine *engine)
{
	int r;

	const char *script = 
	"struct ThreadArg                           \n"
	"{                                          \n"
	"  int count;                               \n"
	"  string str;                              \n"
	"};                                         \n"
	"void main()                                \n"
	"{                                          \n"
    "  for(;;)                                  \n"
	"  {                                        \n"
	"    int count = 10;                        \n"
	"    ThreadArg a;                           \n"
	"    a.count = 3;                           \n"
	"    a.str = \" B\";                        \n"
	"    CreateCoRoutine(\"thread2\", any(@a)); \n"
	"    while( count-- > 0 )                   \n"
	"    {                                      \n"
	"      Print(\"A :\" + count + \"\\n\");    \n"
	"      Yield();                             \n"
	"    }                                      \n"
	"  }                                        \n"
	"}                                          \n"
	"void thread2(any &in arg)                  \n"
	"{                                          \n"
	"  ThreadArg @a;                            \n"
	"  arg.retrieve(@a);                        \n"
	"  int count = a.count;                     \n"
	"  string str = a.str;                      \n"
	"  while( count-- > 0 )                     \n"
	"  {                                        \n"
	"    Print(str + \":\" + count + \"\\n\"); \n"
	"    Yield();                               \n"
	"  }                                        \n"
	"}                                          \n";
	
	// Build the two script into separate modules. This will make them have
	// separate namespaces, which allows them to use the same name for functions
	// and global variables.
	r = engine->AddScriptSection("script", "script", script, strlen(script), 0, false);
	if( r < 0 ) 
	{
		cout << "AddScriptSection() failed" << endl;
		return -1;
	}
	
	r = engine->Build("script");
	if( r < 0 )
	{
		cout << "Build() failed" << endl;
		return -1;
	}

	return 0;
}

void PrintString(string &str)
{
	cout << str;
}

void ScriptCreateCoRoutine(string &func, asIScriptAny *arg)
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx )
	{
		asIScriptEngine *engine = ctx->GetEngine();
		string mod = engine->GetModuleNameFromIndex(ctx->GetCurrentFunction()>>16);

		// We need to find the function that will be created as the co-routine
		string decl = "void " + func + "(any &in)"; 
		int funcId = engine->GetFunctionIDByDecl(mod.c_str(), decl.c_str());
		if( funcId < 0 )
		{
			// No function could be found, raise an exception
			ctx->SetException(("Function '" + decl + "' doesn't exist").c_str());
			return;
		}

		// Create a new context for the co-routine
		asIScriptContext *coctx = engine->CreateContext();
		if( ctx == 0 )
		{
			// No context created, raise an exception
			ctx->SetException("Failed to create context for co-routine");
			return;
		}

		// Prepare the context
		int r = coctx->Prepare(funcId);
		if( r < 0 )
		{
			// Couldn't prepare the context
			ctx->SetException("Failed to prepare the context for co-routine");
			coctx->Release();
			return;
		}

		// Pass the argument to the context
		coctx->SetArgObject(0, arg);

		// Join the current context with the new one, so that they can work together
		contextManager.AddCoRoutine(coctx);
	}
}

void ScriptYield()
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx )
	{
		// Suspend this context
		ctx->Suspend();

		// Let the context manager know that it should run the next co-routine
		contextManager.NextCoRoutine();
	}
}

void CContextManager::ExecuteScripts()
{
	if( currentCtx != coRoutines.end() && *currentCtx != 0 )
	{
		int r = (*currentCtx)->Execute();
		if( r != asEXECUTION_SUSPENDED )
		{
			// The context has terminated execution (for one reason or other)

			// Set the next co-routine as the active one
			list<asIScriptContext*>::iterator it = currentCtx;
			NextCoRoutine();
			if( currentCtx == it )
				currentCtx = coRoutines.end();

			// Release the context that finished
			(*it)->Release();
			coRoutines.erase(it);
		}
	}
}

void CContextManager::AbortAll()
{
	// Abort all contexts and release them. The script engine will make 
	// sure that all resources held by the scripts are properly released.
	list<asIScriptContext*>::iterator it;
	for( it = coRoutines.begin(); it != coRoutines.end(); it = coRoutines.begin() )
	{
		if( *it )
		{
			(*it)->Abort();
			(*it)->Release();
			coRoutines.erase(it);
		}
	}
}

void CContextManager::SetRunningContext(int funcID)
{
	// Create the new context
	asIScriptContext *ctx = engine->CreateContext();
	if( ctx == 0 )
	{
		cout << "Failed to create context" << endl;
		return;
	}

	// Prepare it to execute the function
	int r = ctx->Prepare(funcID);
	if( r < 0 )
	{
		cout << "Failed to prepare the context" << endl;
		return;
	}

	// Add the context to the list for execution
	coRoutines.push_front(ctx);
	currentCtx = coRoutines.begin();
}

void CContextManager::AddCoRoutine(asIScriptContext *ctx)
{
	// Put the context for the co-routine in the queue together with the ctx
	coRoutines.insert(currentCtx, ctx);
}

void CContextManager::NextCoRoutine()
{
	currentCtx++;
	if( currentCtx == coRoutines.end() )
		currentCtx = coRoutines.begin();
}

CContextManager::CContextManager()
{
	currentCtx = coRoutines.end();
}

