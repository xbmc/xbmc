#include <iostream>  // cout
#include <assert.h>  // assert()
#include <conio.h>   // kbhit(), getch()
#include <windows.h> // timeGetTime()
#include <angelscript.h>
#include "../../../add_on/scriptstring/scriptstring.h"

using namespace std;

// Function prototypes
void ConfigureEngine(asIScriptEngine *engine);
int  CompileScript(asIScriptEngine *engine);
void PrintString(string &str);
void LineCallback(asIScriptContext *ctx, DWORD *timeOut);

class asCOutputStream : public asIOutputStream
{
public:
	void Write(const char *text) { printf(text); }
};

int main(int argc, char **argv)
{
	int r;

	// Create the script engine
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
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

	// Create two contexts, one will run the script thread 
	// that executes the main function, the other will other
	// be used to execute the event functions as needed.
	// Note that the global variables declared in the script
	// are shared between the contexts, so they are able to
	// communicate with each other this way.
	asIScriptContext *mainCtx = engine->CreateContext();
	if( mainCtx == 0 ) 
	{
		cout << "Failed to create the context." << endl;
		return -1;
	}

	asIScriptContext *eventCtx = engine->CreateContext();
	if( eventCtx == 0 )
	{
		cout << "Failed to create the context." << endl;
		return -1;
	}

	// Prepare the script context with the function we wish to execute
	r = mainCtx->Prepare(engine->GetFunctionIDByDecl(0, "void main()"));
	if( r < 0 ) 
	{
		cout << "Failed to prepare the context." << endl;
		return -1;
	}

	// Get the function IDs for the event functions already
	int onKeyPressID = engine->GetFunctionIDByDecl(0, "void OnKeyPress()");
	int onQuitID = engine->GetFunctionIDByDecl(0, "void OnQuit()");

	// Set the line callback so that we can suspend the script execution
	// after a certain time. Before executing the script the timeOut variable
	// will be set to the time when the script must stop executing. This
	// way we will be able to do more than one thing, almost at the same time.
	DWORD timeOut;
	r = mainCtx->SetLineCallback(asFUNCTION(LineCallback), &timeOut, asCALL_CDECL);
	if( r < 0 )
	{
		cout << "Failed to set the line callback function." << endl;
		return -1;
	}

	// Print some useful information and start the input loop
	cout << "Sample event driven script execution using AngelScript " << asGetLibraryVersion() << "." << endl;
	cout << "The main script continuosly prints a short string." << endl;
	cout << "Press any key to fire an event that will print another string." << endl;
	cout << "Press ESC to terminate the application." << endl << endl;

	DWORD time = timeGetTime();
	DWORD quitTimeOut = 0;

	for(;;)
	{
		// Check if any key was pressed
		if( kbhit() )
		{
			int key = getch();
			if( key != 27 ) 
			{
				// Fire an event by calling the script function.
				eventCtx->Prepare(onKeyPressID);
				eventCtx->Execute();

				// Note, I'm being a little lazy here, since we don't 
				// verify the return codes. Neither do I add any safeguard 
				// against never ending scripts, which if the script is 
				// badly written could cause the program to hang, I do this 
				// for the main script though.
			}
			else
			{
				// Fire the quit event that will tell the main script to finish
				eventCtx->Prepare(onQuitID);
				eventCtx->Execute();
				
				// Let the script run for at most 1sec more, to give it time 
				// to quit graciously. If it does not finish in time the 
				// script will be aborted.
				quitTimeOut = timeGetTime() + 1000;
			}
		}

		// Allow the long running script to execute for 10ms
		timeOut = timeGetTime() + 10;
		r = mainCtx->Execute();
		if( r != asEXECUTION_SUSPENDED )
		{
			if( quitTimeOut == 0 )
				cout << "The script execution finished early." << endl;
			break;
		}
		
		if( quitTimeOut && quitTimeOut < timeGetTime() )
		{
			// Abort the long running script.
			// AngelScript makes sure everything is correctly freed.
			mainCtx->Abort();
			break;
		}
	}

	// We must release the contexts when no longer using them
	mainCtx->Release();
	eventCtx->Release();

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
	r = engine->RegisterGlobalFunction("uint GetSystemTime()", asFUNCTION(timeGetTime), asCALL_STDCALL); assert( r >= 0 );
}

int CompileScript(asIScriptEngine *engine)
{
	int r;

	// This script prints a char approximately 10 times per second
	const char *scriptMain = 
	"string char = \"-\";               "
	"bool doQuit = false;               "
	"void main()                        "
	"{                                  "
	"  uint time = GetSystemTime();     "
	"  while( !doQuit )                 "
	"  {                                "
	"    uint t = GetSystemTime();      "
	"    if( t - time > 100 )           "
	"    {                              "
	"      time = t;                    "
	"      Print(char);                 "
	"    }                              "
	"  }                                "
	"}                                  ";
	
	const char *scriptEvents =
	"void OnKeyPress()                  "
	"{                                  "
	"  Print(\"A key was pressed\\n\"); "
	"  if( char == \"-\" )              "
	"    char = \"+\";                  "
	"  else                             "
	"    char = \"-\";                  "
	"}                                  "
	"void OnQuit()                      "
	"{                                  "
	"  doQuit = true;                   "
	"}                                  ";

	// Add the script sections that will be compiled into executable code
	r = engine->AddScriptSection(0, "scriptMain", scriptMain, strlen(scriptMain), 0, false);
	if( r < 0 ) 
	{
		cout << "AddScriptSection() failed" << endl;
		return -1;
	}
	
	r = engine->AddScriptSection(0, "scriptEvents", scriptEvents, strlen(scriptEvents), 0, false);
	if( r < 0 )
	{
		cout << "AddScriptSection() failed" << endl;
		return -1;
	}

	// Compile the script
	r = engine->Build(0);
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

void LineCallback(asIScriptContext *ctx, DWORD *timeOut)
{
	// If the time out is reached we suspend the script
	if( *timeOut < timeGetTime() )
		ctx->Suspend();
}

