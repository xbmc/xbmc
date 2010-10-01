#include "utils.h"

namespace TestConstObject
{

#define TESTNAME "TestConstObject"

static const char *script2 = 
"const string URL_SITE = \"http://www.sharkbaitgames.com\";                    \n"
"const string URL_GET_HISCORES = URL_SITE + \"/get_hiscores.php\";             \n"
"const string URL_CAN_SUBMIT_HISCORE = URL_SITE + \"/can_submit_hiscore.php\"; \n"
"const string URL_SUBMIT_HISCORE = URL_SITE + \"/submit_hiscore.php\";         \n";

static const char *script = 
"void Test(obj@ o) { }";

class CObj
{
public: 
	CObj() {refCount = 1; val = 0; next = 0;}
	~CObj() {if( next ) next->Release();}

	CObj &operator=(CObj &other) 
	{
		val = other.val; 
		if( next ) 
			next->Release(); 
		next = other.next; 
		if( next ) 
			next->AddRef();
		return *this; 
	};

	void AddRef() {refCount++;}
	void Release() {if( --refCount == 0 ) delete this;}

	void SetVal(int val) {this->val = val;}
	int GetVal() const {return val;}

	int &operator[](int) {return val;}
	const int &operator[](int) const {return val;}

	int refCount;

	int val;

	CObj *next;
};

void CObj_Construct(CObj *o)
{
	new(o) CObj();
}

CObj c_obj;

bool Test()
{
	int r;
	bool fail = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	// Register an object type
	r = engine->RegisterObjectType("obj", sizeof(CObj), asOBJ_CLASS_CDA); assert( r>=0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(CObj_Construct), asCALL_CDECL_OBJLAST); assert( r>=0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_ADDREF, "void f()", asMETHOD(CObj,AddRef), asCALL_THISCALL); assert( r>=0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_RELEASE, "void f()", asMETHOD(CObj,Release), asCALL_THISCALL); assert( r>=0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_ASSIGNMENT, "obj &f(const obj &in)", asMETHOD(CObj,operator=), asCALL_THISCALL); assert( r>=0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_INDEX, "int &f(int)", asMETHODPR(CObj, operator[], (int), int&), asCALL_THISCALL); assert( r>=0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_INDEX, "const int &f(int) const", asMETHODPR(CObj, operator[], (int) const, const int&), asCALL_THISCALL); assert( r>=0 );

	r = engine->RegisterObjectType("prop", sizeof(int), asOBJ_PRIMITIVE); assert( r>=0 );
	r = engine->RegisterObjectProperty("prop", "int val", 0); assert( r>=0 );

	r = engine->RegisterObjectMethod("obj", "void SetVal(int)", asMETHOD(CObj, SetVal), asCALL_THISCALL); assert( r>=0 );
	r = engine->RegisterObjectMethod("obj", "int GetVal() const", asMETHOD(CObj, GetVal), asCALL_THISCALL); assert( r>=0 );
	r = engine->RegisterObjectProperty("obj", "int val", offsetof(CObj,val)); assert( r>=0 );
	r = engine->RegisterObjectProperty("obj", "obj@ next", offsetof(CObj,next)); assert( r>=0 );
	r = engine->RegisterObjectProperty("obj", "prop p", offsetof(CObj,val)); assert( r>=0 );

	r = engine->RegisterGlobalProperty("const obj c_obj", &c_obj); assert( r>=0 );
	r = engine->RegisterGlobalProperty("obj g_obj", &c_obj); assert( r>= 0 );

	RegisterScriptString(engine);

	COutStream out;
	engine->SetCommonMessageStream(&out);
	engine->AddScriptSection(0, "script1", script2, strlen(script2), 0, false);
	r = engine->Build(0);
	if( r < 0 ) fail = true;


	CBufferedOutStream bout;

	// TODO:
	// A member array of a const object is also const

	// TODO:
	// Parameters registered as &in and not const must make a copy of the object (even for operators)

	// A member object of a const object is also const
	bout.buffer = "";
	engine->SetCommonMessageStream(&bout);
	r = engine->ExecuteString(0, "c_obj.p.val = 1;");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 13) : Error   : Reference is read-only\n" ) fail = true;

	c_obj.val = 0;
	engine->SetCommonMessageStream(&out);
	r = engine->ExecuteString(0, "g_obj.p.val = 1;");
	if( r < 0 ) fail = true;
	if( c_obj.val != 1 ) fail = true;

	// Allow overloading on const.
	r = engine->ExecuteString(0, "obj o; o[0] = 1;");
	if( r < 0 ) fail = true;

	// Allow return of const ref
	r = engine->ExecuteString(0, "int a = c_obj[0];");
	if( r < 0 ) fail = true;

	// Do not allow the script to call object behaviour that is not const on a const object
	bout.buffer = "";
	engine->SetCommonMessageStream(&bout);
	r = engine->ExecuteString(0, "c_obj[0] = 1;");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 10) : Error   : Reference is read-only\n" ) fail = true;

	// Do not allow the script to take a non-const handle to a const object
	bout.buffer = "";
	r = engine->ExecuteString(0, "obj@ o = @c_obj;");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 10) : Error   : Can't implicitly convert from 'const obj@' to 'obj@&'.\n" )
		fail = true;

	// Do not allow the script to pass a const obj@ to a parameter that is not a const obj@
	engine->AddScriptSection(0, "script", script, strlen(script));
	engine->SetCommonMessageStream(&out);
	engine->Build(0);
	
	bout.buffer = "";
	engine->SetCommonMessageStream(&bout);
	r = engine->ExecuteString(0, "Test(@c_obj);");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 1) : Error   : No matching signatures to 'Test(const obj@const&)'\n" )
		fail = true;

	// Do not allow the script to assign the object handle member of a const object
	bout.buffer = "";
	r = engine->ExecuteString(0, "@c_obj.next = @obj();");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 13) : Error   : Reference is read-only\n" )
		fail = true;

	// Allow the script to change the object the handle points to
	engine->SetCommonMessageStream(&out);
	r = engine->ExecuteString(0, "c_obj.next.val = 1;");
	if( r != 3 ) fail = true;

	// Allow the script take a handle to a non const object handle in a const object
	r = engine->ExecuteString(0, "obj @a = @c_obj.next;");
	if( r < 0 ) fail = true;

	// Allow the script to take a const handle to a const object
	r = engine->ExecuteString(0, "const obj@ o = @c_obj;");
	if( r < 0 ) fail = true;

	bout.buffer = "";
	engine->SetCommonMessageStream(&bout);
	r = engine->ExecuteString(0, "obj @a; const obj @b; @a = @b;");
	if( r >= 0 ) fail = true;
	if(bout.buffer != "ExecuteString (1, 28) : Error   : Can't implicitly convert from 'const obj@&' to 'obj@&'.\n" )
		fail = true;

	// Allow a non-const handle to be assigned to a const handle
	engine->SetCommonMessageStream(&out);
	r = engine->ExecuteString(0, "obj @a; const obj @b; @b = @a;");
	if( r < 0 ) fail = true;

	// Do not allow the script to alter properties of a const object
	bout.buffer = "";
	engine->SetCommonMessageStream(&bout);
	r = engine->ExecuteString(0, "c_obj.val = 1;");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 11) : Error   : Reference is read-only\n" )
		fail = true;

	// Do not allow the script to call non-const methods on a const object
	bout.buffer = "";
	r = engine->ExecuteString(0, "c_obj.SetVal(1);");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 7) : Error   : No matching signatures to 'SetVal(const uint) const'\n" )
		fail = true;

	// Allow the script to call const methods on a const object
	engine->SetCommonMessageStream(&out);
	r = engine->ExecuteString(0, "c_obj.GetVal();");
	if( r < 0 ) fail = true;

	engine->Release();

	if( fail )
		printf("%s: failed\n", TESTNAME);

	// Success
	return fail;
}

} // namespace

