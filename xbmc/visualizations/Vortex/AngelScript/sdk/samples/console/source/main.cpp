#include <iostream>
#include <string>
#include <assert.h>
#include <math.h>
#include <angelscript.h>
#include "../../../add_on/scriptstring/scriptstring.h"

using namespace std;

// Function prototypes
void PrintHelp();
void ExecString(asIScriptEngine *engine, string &arg);
void ConfigureEngine(asIScriptEngine *engine);
void grab(int);
void grab(asUINT);
void grab(bool);
void grab(float);
void grab(double);
void grab(string&);
void grab(void);

// Some global variables that the script can access
float            g_gravity;
asUINT           p_health;
asUINT           r_fov;
bool             r_shadow;
asCScriptString *p_name = 0;

int main(int argc, char **argv)
{
	// Create the script engine
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	if( engine == 0 )
	{
		cout << "Failed to create script engine." << endl;
		return -1;
	}

	// Allocate a script string for the player name.
	// We must do this because the script string type that we use is  
	// reference counted and cannot be declared as local or global variable.
	p_name = new asCScriptString("player");
	if( p_name == 0 )
	{
		cout << "Failed to allocate script string." << endl;
		return -1;
	}

	// Configure the script engine with all the functions, 
	// and variables that the script should be able to use.
	ConfigureEngine(engine);
	
	// Print some useful information and start the input loop
	cout << "Sample console using AngelScript " << asGetLibraryVersion() << " to perform scripted tasks." << endl;
	cout << "Type 'help' for more information." << endl;

	for(;;)
	{
		string input;
		input.resize(256);
		string cmd, arg;

		cout << "> ";
		cin.getline(&input[0], 256);

		// Trim unused characters
		input.resize(strlen(input.c_str()));

		int pos;
		if( (pos = input.find(" ")) != string::npos )
		{
			cmd = input.substr(0, pos);
			arg = input.substr(pos+1);
		}
		else
		{
			cmd = input;
			arg = "";
		}

		// Interpret the command
		if( cmd == "exec" )
			ExecString(engine, arg);
		else if( cmd == "help" )
			PrintHelp();
		else if( cmd == "quit" )
			break;
		else
			cout << "Unknown command." << endl;
	}

	// Release the engine
	engine->Release();

	// Release the script string
	if( p_name ) p_name->Release();

	return 0;
}

void PrintHelp()
{
	cout << "Commands:" << endl;
	cout << "exec [script] - executes script statement and prints the result" << endl;
	cout << "help          - this command" << endl;
	cout << "quit          - end application" << endl;
	cout << endl;
	cout << "Functions:" << endl;
	cout << "float sin(float) - sinus" << endl;
	cout << "float cos(float) - cosinus" << endl;
	cout << endl;
	cout << "Variables:" << endl;
	cout << "g_gravity (float)  - game gravity factor" << endl;
	cout << "p_health  (uint)   - player health" << endl;
	cout << "p_name    (string) - player name" << endl;
	cout << "r_fov     (uint)   - field of view angle" << endl;
	cout << "r_shadow  (bool)   - toggles shadows on/off" << endl;
}

void ConfigureEngine(asIScriptEngine *engine)
{
	int r;

	// Tell the engine to output any error messages to printf
	engine->SetCommonMessageStream((asOUTPUTFUNC_t)printf, 0);

	// Register the script string type
	// Look at the implementation for this function for more information  
	// on how to register a custom string type, and other object types.
	// The implementation is in "/add_on/scriptstring/scriptstring.cpp"
	RegisterScriptString(engine);

	// Register the global variables
	r = engine->RegisterGlobalProperty("float g_gravity", &g_gravity); assert( r >= 0 );
	r = engine->RegisterGlobalProperty("uint p_health", &p_health);    assert( r >= 0 );
	r = engine->RegisterGlobalProperty("uint r_fov", &r_fov);          assert( r >= 0 );
	r = engine->RegisterGlobalProperty("bool r_shadow", &r_shadow);    assert( r >= 0 );
	r = engine->RegisterGlobalProperty("string p_name", p_name);       assert( r >= 0 );

	// Register some useful functions
	r = engine->RegisterGlobalFunction("float sin(float)", asFUNCTION(sinf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float cos(float)", asFUNCTION(cosf), asCALL_CDECL); assert( r >= 0 );

	// Register special function with overloads to catch any type.
	// This is used by the exec command to output the resulting value from the statement.
	r = engine->RegisterGlobalFunction("void _grab(bool)", asFUNCTIONPR(grab, (bool), void), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void _grab(int)", asFUNCTIONPR(grab, (int), void), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void _grab(uint)", asFUNCTIONPR(grab, (asUINT), void), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void _grab(float)", asFUNCTIONPR(grab, (float), void), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void _grab(double)", asFUNCTIONPR(grab, (double), void), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void _grab()", asFUNCTIONPR(grab, (void), void), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void _grab(string &in)", asFUNCTIONPR(grab, (string&), void), asCALL_CDECL); assert( r >= 0 );

	// Do not output anything else to printf
	engine->SetCommonMessageStream(0);
}

class asCOutputStream : public asIOutputStream
{
public:
	void Write(const char *text) { buffer += text; }

	string buffer;
};

void ExecString(asIScriptEngine *engine, string &arg)
{
	asCOutputStream out;

	string script;

	script = "_grab(" + arg + ")";

	engine->SetCommonMessageStream(&out);
	int r = engine->ExecuteString(0, script.c_str());
	if( r < 0 )
		cout << "Invalid script statement. " << endl;
	else if( r == asEXECUTION_EXCEPTION )
		cout << "A script exception was raised." << endl;

	engine->SetCommonMessageStream(0);
}

void grab(int v)
{
	cout << v << endl;
}

void grab(asUINT v)
{
	cout << v << endl;
}

void grab(bool v)
{
	cout << boolalpha << v << endl;
}

void grab(float v)
{
	cout << v << endl;
}

void grab(double v)
{
	cout << v << endl;
}

void grab(string &v)
{
	cout << v << endl;
}

void grab()
{
	// There is no value
}
