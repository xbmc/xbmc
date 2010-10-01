#include "utils.h"

namespace TestObject3
{

#define TESTNAME "TestObject3"


struct cFloat
{
	float m_Float;
	cFloat()
	{
		m_Float = 0.0f;
	}
	cFloat(float Float)
	{
		m_Float = Float;
	}
	float operator = (float f);
	operator float()
	{
		return m_Float;
	}
	float operator += (cFloat v );
	float operator += (float v );
	float operator -= (cFloat v );
	float operator -= (float v );
	float operator /= (cFloat v );
	float operator /= (float v );
	float operator *= (cFloat v );
	float operator *= (float v );
};

float cFloat::operator = (float f)
{
	float old = m_Float;
	m_Float = f;
	return m_Float;
}
float cFloat::operator += (cFloat v )
{
	float old = m_Float;
	m_Float += v;
	return m_Float;
}
float cFloat::operator += (float v )
{
	float old = m_Float;
	m_Float += v;
	return m_Float;
}
float cFloat::operator -= (cFloat v )
{
	float old = m_Float;
	m_Float -= v;
	return m_Float;
}
float cFloat::operator -= (float v )
{
	float old = m_Float;
	m_Float -= v;
	return m_Float;
}
float cFloat::operator /= (cFloat v )
{
	float old = m_Float;
	m_Float /= v;
	return m_Float;
}
float cFloat::operator /= (float v )
{
	float old = m_Float;
	m_Float /= v;
	return m_Float;
}
float cFloat::operator *= (cFloat v )
{
	float old = m_Float;
	m_Float *= v;
	return m_Float;
}
float cFloat::operator *= (float v )
{
	float old = m_Float;
	m_Float *= v;
	return m_Float;
}
float __cdecl AssignFloat2Float(float a,cFloat &b)
{
	b=a;
	return b;
}
float  __cdecl OpPlusRR(cFloat *self,  cFloat* other)
{
	return (float)(*self)+(float)(*other);
}
float  __cdecl OpPlusRF(cFloat *self,  float other)
{
	assert(self);
	
	return (float)(*self)+(float)(other);
}
float  __cdecl OpPlusFR(float self,  cFloat* other)
{
	return (self)+(float)(*other);
}

float  __cdecl OpMulRR(cFloat *self,  cFloat* other)
{
	return (float)(*self)*(float)(*other);
}
float  __cdecl OpMulRF(cFloat *self,  float other)
{
	return (float)(*self)*(float)(other);
}
float  __cdecl OpMulFR(float self,  cFloat* other)
{
	return (float)(self)*(float)(*other);
}

bool Register(asIScriptEngine*  pSE)
{
	pSE->RegisterObjectType("Float", sizeof(cFloat), asOBJ_CLASS);
	
	if(pSE->RegisterObjectBehaviour("Float",asBEHAVE_ASSIGNMENT,"Float& f(float )",asFUNCTION(AssignFloat2Float),asCALL_CDECL_OBJLAST))
		return false;
	
	
	// asBEHAVE_ADD
	if(pSE->RegisterGlobalBehaviour(asBEHAVE_ADD,"float  f(Float &in ,Float &in)",asFUNCTION(OpPlusRR),  asCALL_CDECL))
		return false;
	if(pSE->RegisterGlobalBehaviour(asBEHAVE_ADD,"float  f(Float &in ,float)",asFUNCTION(OpPlusRF),  asCALL_CDECL))
		return false;
	if(pSE->RegisterGlobalBehaviour(asBEHAVE_ADD,"float  f(float ,Float &in)",asFUNCTION(OpPlusFR),  asCALL_CDECL))
		return false;
	
	// asBEHAVE_MULTIPLY
	if(pSE->RegisterGlobalBehaviour(asBEHAVE_MULTIPLY, "float  f(Float &in ,Float &in)",asFUNCTION(OpMulRR),  asCALL_CDECL))
		return false;
	if(pSE->RegisterGlobalBehaviour(asBEHAVE_MULTIPLY, "float  f(Float &in ,float)",asFUNCTION(OpMulRF),  asCALL_CDECL))
		return false;
	if(pSE->RegisterGlobalBehaviour(asBEHAVE_MULTIPLY, "float  f(float ,Float &in)",asFUNCTION(OpMulFR),  asCALL_CDECL))
		return false;
	
	return true;
}

cFloat& Get(int index)
{
	static cFloat m_arr[10];
	return m_arr[index];
}
void Print(float f)
{
	assert(f == 30.0f);
//	printf("%f\n", f);
}

bool Test()
{
	asIScriptEngine*        pSE;
	pSE=asCreateScriptEngine(ANGELSCRIPT_VERSION);
	Register(pSE);
	pSE->RegisterGlobalFunction("Float& Get(int32)",asFUNCTION(Get),asCALL_CDECL);
	pSE->RegisterGlobalFunction("void Print(float)",asFUNCTION(Print),asCALL_CDECL);
	
	const char script[]="\
						   float ret=10;\n\
						   Get(0)=10.0f;\n\
						   Get(1)=10.0f;\n\
						   Get(2)=10.0f;\n\
						   ret=Get(0)+(Get(1)*2.0f);\n\
						   Print(ret);\n\
						   \n";
	pSE->ExecuteString("",script);
	
	pSE->Release();
	   
	return false;
}


} // namespace

