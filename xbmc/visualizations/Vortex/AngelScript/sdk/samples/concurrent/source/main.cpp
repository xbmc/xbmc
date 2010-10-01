#include <iostream>  // cout
#include <assert.h>  // assert()
#include <conio.h>   // kbhit(), getch()
#include <windows.h> // timeGetTime()
#include <vector>
#include <angelscript.h>
#include "../../../add_on/scriptstring/scriptstring.h"

using namespace std;

// Function prototypes
void ConfigureEngine(asIScriptEngine *engine);
int  CompileScript(asIScriptEngine *engine);
void PrintString(string &str);
void PrintNumber(int num);
void ScriptSleep(UINT milliSeconds);

struct SContextInfo
{
	UINT sleepUntil;
	asIScriptContext *ctx;
};

// This simple manager will let us manage the contexts, 
// in a simple scheme for apparent multithreading
class CContextManager
{
public:
	void AddContext(int funcID);
	void ExecuteScripts();
	void SetSleeping(asIScriptContext *ctx, UINT milliSeconds);
	void AbortAll();

	vector<SContextInfo> contexts;
} contextManager;

class asCOutputStream : public asIOutputStream
{
public:
	void Write(const char *text) 
	{ 
		buffer += text;
		printf(text); 
	}

	string buffer;
};


asIScriptEngine *engine = 0;
int main(int argc, char **argv)
{
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

	contextManager.AddContext(engine->GetFunctionIDByDecl("script1", "void main()"));
	contextManager.AddContext(engine->GetFunctionIDByDecl("script2", "void main()"));
	
	// Print some useful information and start the input loop
	cout << "This sample shows how two scripts can be executed concurrently." << endl; 
	cout << "Both scripts voluntarily give up the control by calling Sleep()." << endl;
	cout << "Press any key to terminate the application." << endl;

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
	r = engine->RegisterGlobalFunction("void Print(int)", asFUNCTION(PrintNumber), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void Sleep(uint)", asFUNCTION(ScriptSleep), asCALL_CDECL); assert( r >= 0 );
}

int CompileScript(asIScriptEngine *engine)
{
	int r;

	// This script prints a message 3 times per second
	const char *script1 = 
	"int count = 0;                     "
	"void main()                        "
	"{                                  "
    "  for(;;)                          "
	"  {                                "
	"    Print(\"A :\");                "
	"    Print(count++);                "
	"    Print(\"\\n\");                "
	"    Sleep(333);                    "
	"  }                                "
	"}                                  ";
	
	// This script prints a message once per second
	const char *script2 =
	"int count = 0;                     "
	"void main()                        "
	"{                                  "
	"  for(;;)                          "
	"  {                                "
	"    Print(\" B:\");                "
	"    Print(count++);                "
	"    Print(\"\\n\");                "
	"    Sleep(1000);                   "
	"  }                                "
	"}                                  ";

	// Build the two script into separate modules. This will make them have
	// separate namespaces, which allows them to use the same name for functions
	// and global variables.
	r = engine->AddScriptSection("script1", "script1", script1, strlen(script1), 0, false);
	if( r < 0 ) 
	{
		cout << "AddScriptSection() failed" << endl;
		return -1;
	}
	
	r = engine->Build("script1");
	if( r < 0 )
	{
		cout << "Build() failed" << endl;
		return -1;
	}

	r = engine->AddScriptSection("script2", "script2", script2, strlen(script2), 0, false);
	if( r < 0 )
	{
		cout << "AddScriptSection() failed" << endl;
		return -1;
	}

	r = engine->Build("script2");
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

void PrintNumber(int num)
{
	cout << num;
}

void ScriptSleep(UINT milliSeconds)
{
	// Get a pointer to the context that is currently being executed
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx )
	{
		// Suspend it's execution. The VM will continue until the current 
		// statement is finished and then return from the Execute() method
		ctx->Suspend();

		// Tell the context manager when the context is to continue execution
		contextManager.SetSleeping(ctx, milliSeconds);
	}
}

void CContextManager::ExecuteScripts()
{
	// Check if the system time is higher than the time set for the contexts
	UINT time = timeGetTime();
	for( int n = 0; n < contexts.size(); n++ )
	{
		if( contexts[n].ctx && contexts[n].sleepUntil < time )
		{
			int r = contexts[n].ctx->Execute();
			if( r != asEXECUTION_SUSPENDED )
			{
				// The context has terminated execution (for one reason or other)
				contexts[n].ctx->Release();
				contexts[n].ctx = 0;
			}
		}
	}
}

void CContextManager::AbortAll()
{
	// Abort all contexts and release them. The script engine will make 
	// sure that all resources held by the scripts are properly released.
	for( int n = 0; n < contexts.size(); n++ )
	{
		if( contexts[n].ctx )
		{
			contexts[n].ctx->Abort();
			contexts[n].ctx->Release();
			contexts[n].ctx = 0;
		}
	}
}

void CContextManager::AddContext(int funcID)
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
	SContextInfo info = {0, ctx};
	contexts.push_back(info);
}

void CContextManager::SetSleeping(asIScriptContext *ctx, UINT milliSeconds)
{
	// Find the context and update the timeStamp  
	// for when the context is to be continued
	for( int n = 0; n < contexts.size(); n++ )
	{
		if( contexts[n].ctx == ctx )
		{
			contexts[n].sleepUntil = timeGetTime() + milliSeconds;
		}
	}
}