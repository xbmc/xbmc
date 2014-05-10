/*****************************************************************
|
|      Threads Test Program 1
|
|      (c) 2001-2012 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Neptune.h"

#if defined(WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#define CHECK(x) {                                  \
    if (!(x)) {                                     \
        NPT_Console::OutputF("TEST FAILED line %d\n", __LINE__);  \
        NPT_ASSERT(0);                              \
    }                                               \
}

/*----------------------------------------------------------------------
|   SharedVariableThread
+---------------------------------------------------------------------*/
class SharedVariableThread : public NPT_Thread
{
public:
	SharedVariableThread(int ping, int pong, NPT_SharedVariable& shared, float wait) : 
	  m_Ping(ping), m_Pong(pong), m_Shared(shared), m_Transitions(0), m_Wait(wait), m_Stop(false), m_Result(NPT_SUCCESS) {}

	void Run() {
		for (;;) {
			if (m_Stop) return;
			NPT_Result result = m_Shared.WaitUntilEquals(m_Ping, 1000);
			if (result != NPT_SUCCESS) {
				NPT_Console::Output("timeout\n");
				m_Result = -1;
				return;
			}
			if (m_Wait != 0.0f) NPT_System::Sleep(m_Wait);
			m_Shared.SetValue(m_Pong);
			++m_Transitions;
		}
	}

	int                 m_Ping;
	int                 m_Pong;
	NPT_SharedVariable& m_Shared;
	unsigned long       m_Transitions;
	float               m_Wait;
	volatile bool       m_Stop;
	NPT_Result          m_Result;
};

/*----------------------------------------------------------------------
|   TestSharedVariables
+---------------------------------------------------------------------*/
static void
TestSharedVariables()
{
	NPT_SharedVariable shared;
	SharedVariableThread t0(1, 2, shared, 0);
	SharedVariableThread t1(2, 1, shared, 0.001f);

	t0.Start();
	t1.Start();

	shared.SetValue(1);
	NPT_Result result = t0.Wait(10000);
	t0.m_Stop = true;
	t1.m_Stop = true;
	t1.Wait();

	NPT_Console::OutputF("T0 transitions=%d, result: %d\n", t0.m_Transitions, t0.m_Result);
	NPT_Console::OutputF("T1 transitions=%d, result: %d\n", t1.m_Transitions, t1.m_Result);

	CHECK(t0.m_Result == NPT_SUCCESS);
	CHECK(t1.m_Result == NPT_SUCCESS);
}

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv) 
{
    NPT_COMPILER_UNUSED(argc);
    NPT_COMPILER_UNUSED(argv);

#if defined(WIN32) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF    |
                   _CRTDBG_CHECK_ALWAYS_DF |
                   _CRTDBG_LEAK_CHECK_DF);
#endif
    
    TestSharedVariables();

    NPT_Debug("- program done -\n");

    return 0;
}






