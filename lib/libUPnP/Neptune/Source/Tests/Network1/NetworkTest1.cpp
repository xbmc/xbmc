/*****************************************************************
|
|      Network Test Program 1
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

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#define CHECK(x) do { if (!(x)) NPT_Console::OutputF("FAILED line %d\n", __LINE__); } while(0)

/*----------------------------------------------------------------------
|   functions
+---------------------------------------------------------------------*/
static volatile bool NeedToStop = false;

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
class Resolver : public NPT_Thread
{
public:
    Resolver(const char* name, NPT_IpAddress addr) : m_Result(NPT_SUCCESS), m_Name(name), m_Addr(addr) {}
    
    virtual void Run() {
        while (!NeedToStop) {
            NPT_IpAddress addr;
            m_Result = addr.ResolveName(m_Name);
            if (NPT_FAILED(m_Result)) {
                NPT_Console::OutputF("ERROR: ResolveName failed (%d)\n", m_Result);
                return;
            }
            if (!(addr == m_Addr)) {
                m_Result = NPT_FAILURE;
                NPT_Console::OutputF("ERROR: wrong IP address (%s instead of %s for %s)\n", addr.ToString().GetChars(), m_Addr.ToString().GetChars(), m_Name.GetChars());
                return;
            }
        }
    }
    
private:
    NPT_Result    m_Result;
    NPT_String    m_Name;
    NPT_IpAddress m_Addr;
};

/*----------------------------------------------------------------------
|       TestAddresses
+---------------------------------------------------------------------*/
static void
TestAddresses()
{
    NPT_IpAddress a1 = NPT_IpAddress::Loopback;
    CHECK(a1.IsLooppack());
    CHECK(!a1.IsUnspecified());
    NPT_IpAddress a2 = NPT_IpAddress::Any;
    CHECK(a2.IsUnspecified());
#if defined(NPT_CONFIG_ENABLE_IPV6)
    NPT_IpAddress a3;
    a3.ResolveName("::1");
    CHECK(a3.IsLooppack());
    NPT_IpAddress a4;
    a4.ResolveName("127.0.0.1");
    CHECK(a4.IsLooppack());
    
    NPT_IpAddress a6;
    a6.ResolveName("::abcd:1234");
    CHECK(a6.IsV4Compatible());

    NPT_IpAddress a7;
    a7.ResolveName("::ffff:abcd:1234");
    CHECK(a7.IsV4Mapped());

    NPT_IpAddress a5;
    a5.ResolveName("fe80::bae8:56ff:fe45:fc74");
    CHECK(a5.IsLinkLocal());
    
    NPT_IpAddress a8;
    a8.ResolveName("fec3::bae8:56ff:fe45:fc74");
    CHECK(a8.IsSiteLocal());

    NPT_IpAddress a9;
    a9.ResolveName("fd00::bae8:56ff:fe45:fc74");
    CHECK(a9.IsUniqueLocal());

    NPT_IpAddress a10;
    a10.ResolveName("ff05::2");
    CHECK(a10.IsMulticast());
    
#endif
    NPT_IpAddress b1(192, 168, 1, 1);
    CHECK(b1.IsUniqueLocal());
    NPT_IpAddress b2(172, 16, 1, 1);
    CHECK(b2.IsUniqueLocal());
    NPT_IpAddress b3(10, 1, 1, 1);
    CHECK(b3.IsUniqueLocal());
    NPT_IpAddress b4(169, 254, 1, 1);
    CHECK(b4.IsLinkLocal());
    NPT_IpAddress b5(239, 255, 255, 251);
    CHECK(b5.IsMulticast());
}

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int
main(int /*argc*/, char** /*argv*/)
{
    // setup debugging
#if defined(WIN32) && defined(_DEBUG)
    int flags = _crtDbgFlag       | 
        _CRTDBG_ALLOC_MEM_DF      |
        _CRTDBG_DELAY_FREE_MEM_DF |
        _CRTDBG_CHECK_ALWAYS_DF;

    _CrtSetDbgFlag(flags);
    //AllocConsole();
    //freopen("CONOUT$", "w", stdout);
#endif 

    TestAddresses();
    
    NPT_IpAddress addr;
    NPT_Result result;
    
    result = addr.ResolveName("www.perdu.com");
    CHECK(NPT_SUCCEEDED(result));
    Resolver resolver1("www.perdu.com", addr);

    result = addr.ResolveName("zebulon.bok.net");
    CHECK(NPT_SUCCEEDED(result));
    Resolver resolver2("zebulon.bok.net", addr);
    
    resolver1.Start();
    resolver2.Start();
    
    NPT_System::Sleep(10.0);
    NeedToStop = true;
    resolver1.Wait();
    resolver2.Wait();
    
#if defined(WIN32) && defined(_DEBUG)
    _CrtDumpMemoryLeaks();
#endif

    return 0;
}
