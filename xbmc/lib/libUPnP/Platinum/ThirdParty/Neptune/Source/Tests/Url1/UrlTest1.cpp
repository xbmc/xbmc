/*****************************************************************
|
|      URL Test Program 1
|
|      (c) 2001-2006 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "NptDebug.h"

#if defined(WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#define CHECK(x)                                        \
    do {                                                \
      if (!(x)) {                                       \
        fprintf(stderr, "ERROR line %d \n", __LINE__);  \
      }                                                 \
    } while(0)

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

    // test URL parsing
    NPT_HttpUrl url0;
    CHECK(!url0.IsValid());

    NPT_HttpUrl url1("");
    CHECK(!url1.IsValid());

    NPT_HttpUrl url2("http");
    CHECK(!url2.IsValid());

    NPT_HttpUrl url3("http:");
    CHECK(!url3.IsValid());

    NPT_HttpUrl url4("http:/");
    CHECK(!url4.IsValid());

    NPT_HttpUrl url5("http://");
    CHECK(!url5.IsValid());

    NPT_HttpUrl url;
    
    url = "http://a";
    CHECK(url.IsValid());
    CHECK(url.GetHost() == "a");
    CHECK(url.GetPort() == 80);
    CHECK(url.GetQuery().IsEmpty());
    CHECK(url.GetFragment().IsEmpty());
    CHECK(url.ToString() == "http://a/");

    url = "http://foo.bar";
    CHECK(url.IsValid());
    CHECK(url.GetHost() == "foo.bar");
    CHECK(url.GetPort() == 80);
    CHECK(url.GetPath() == "/");
    CHECK(url.GetQuery().IsEmpty());
    CHECK(url.GetFragment().IsEmpty());
    CHECK(url.ToString() == "http://foo.bar/");

    url = "http://foo.bar:";
    CHECK(!url.IsValid());

    url = "http://foo.bar:156";
    CHECK(url.IsValid());
    CHECK(url.GetHost() == "foo.bar");
    CHECK(url.GetPort() == 156);
    CHECK(url.GetPath() == "/");
    CHECK(url.GetQuery().IsEmpty());
    CHECK(url.GetFragment().IsEmpty());
    CHECK(url.ToString() == "http://foo.bar:156/");

    url = "http://foo.bar:176899";
    CHECK(!url.IsValid());

    url = "http://foo.bar:176a";
    CHECK(!url.IsValid());

    url = "http://foo.bar:176/";
    CHECK(url.IsValid());
    CHECK(url.GetHost() == "foo.bar");
    CHECK(url.GetPort() == 176);
    CHECK(url.GetPath() == "/");
    CHECK(url.GetQuery().IsEmpty());
    CHECK(url.GetFragment().IsEmpty());
    CHECK(url.ToString() == "http://foo.bar:176/");

    url = "http://foo.bar:176/blabla";
    CHECK(url.IsValid());
    CHECK(url.GetHost() == "foo.bar");
    CHECK(url.GetPort() == 176);
    CHECK(url.GetPath() == "/blabla");
    CHECK(url.GetQuery().IsEmpty());
    CHECK(url.GetFragment().IsEmpty());
    CHECK(url.ToString() == "http://foo.bar:176/blabla");

    url = "http://foo.bar/blabla/blibli";
    CHECK(url.IsValid());
    CHECK(url.GetHost() == "foo.bar");
    CHECK(url.GetPort() == 80);
    CHECK(url.GetPath() == "/blabla/blibli");
    CHECK(url.GetQuery().IsEmpty());
    CHECK(url.GetFragment().IsEmpty());
    CHECK(url.ToString() == "http://foo.bar/blabla/blibli");

    url = "http://foo.bar:176/blabla/blibli/";
    CHECK(url.IsValid());
    CHECK(url.GetHost() == "foo.bar");
    CHECK(url.GetPort() == 176);
    CHECK(url.GetPath() == "/blabla/blibli/");
    CHECK(url.GetQuery().IsEmpty());
    CHECK(url.GetFragment().IsEmpty());
    CHECK(url.ToString() == "http://foo.bar:176/blabla/blibli/");

    url = "http://foo.bar/";
    CHECK(url.IsValid());
    CHECK(url.GetHost() == "foo.bar");
    CHECK(url.GetPort() == 80);
    CHECK(url.GetPath() == "/");
    CHECK(url.GetQuery().IsEmpty());
    CHECK(url.GetFragment().IsEmpty());
    CHECK(url.ToString() == "http://foo.bar/");

    url = "http://foo.bar/blabla/blibli/?query";
    CHECK(url.IsValid());
    CHECK(url.GetHost() == "foo.bar");
    CHECK(url.GetPort() == 80);
    CHECK(url.GetPath() == "/blabla/blibli/");
    CHECK(url.GetQuery() == "query");
    CHECK(url.GetFragment().IsEmpty());
    CHECK(url.ToString() == "http://foo.bar/blabla/blibli/?query");

    url = "http://foo.bar/blabla/blibli/?query=1&bla=%20&slash=/&foo=a#fragment";
    CHECK(url.IsValid());
    CHECK(url.GetHost() == "foo.bar");
    CHECK(url.GetPort() == 80);
    CHECK(url.GetPath() == "/blabla/blibli/");
    CHECK(url.GetQuery() == "query=1&bla=%20&slash=/&foo=a");
    CHECK(url.GetFragment() == "fragment");
    CHECK(url.ToString() == "http://foo.bar/blabla/blibli/?query=1&bla=%20&slash=/&foo=a#fragment");

    url = "http://foo.bar/blabla foo/blibli/?query=1&bla=2&slash=/&foo=a#fragment";
    CHECK(url.IsValid());
    CHECK(url.GetHost() == "foo.bar");
    CHECK(url.GetPort() == 80);
    CHECK(url.GetPath() == "/blabla foo/blibli/");
    CHECK(url.GetQuery() == "query=1&bla=2&slash=/&foo=a");
    CHECK(url.GetFragment() == "fragment");
    CHECK(url.ToString(false) == "http://foo.bar/blabla%20foo/blibli/?query=1&bla=2&slash=/&foo=a");

    url = NPT_HttpUrl("http://foo.bar/blabla%20foo/blibli/?query=1&bla=2&slash=/&foo=a#fragment");
    CHECK(url.IsValid());
    CHECK(url.GetHost() == "foo.bar");
    CHECK(url.GetPort() == 80);
    CHECK(url.GetPath() == "/blabla%20foo/blibli/");
    CHECK(url.GetQuery() == "query=1&bla=2&slash=/&foo=a");
    CHECK(url.GetFragment() == "fragment");
    CHECK(url.ToRequestString() == "/blabla%20foo/blibli/?query=1&bla=2&slash=/&foo=a");

    url.SetPathPlus("/bla/foo?query=bar");
    url.SetHost("bar.com:8080");
    CHECK(url.IsValid());
    CHECK(url.GetHost() == "bar.com");
    CHECK(url.GetPort() == 8080);
    CHECK(url.GetPath() == "/bla/foo");
    CHECK(url.GetQuery() == "query=bar");
    
    url.SetPathPlus("bla/foo?query=bar");
    url.SetHost("bar.com:8080");
    CHECK(url.IsValid());
    CHECK(url.GetHost() == "bar.com");
    CHECK(url.GetPort() == 8080);
    CHECK(url.GetPath() == "bla/foo");
    CHECK(url.GetQuery() == "query=bar");

    url.SetPathPlus("*");
    CHECK(url.IsValid());
    CHECK(url.GetPath() == "*");

    url = NPT_HttpUrl("http://foo/?query=1&bla=2&slash=/&foo=a#fragment");
    CHECK(url.IsValid());
    CHECK(url.GetHost() == "foo");
    CHECK(url.GetPort() == 80);
    CHECK(url.GetPath() == "/");
    CHECK(url.GetQuery() == "query=1&bla=2&slash=/&foo=a");
    CHECK(url.GetFragment() == "fragment");
    CHECK(url.ToRequestString() == "/?query=1&bla=2&slash=/&foo=a");

    // url form encoding
    NPT_HttpUrlQuery query;
    query.AddField("url1","http://foo.bar/foo?q=3&bar=+7/3");
    query.AddField("url2","1234");
    CHECK(query.ToString() == "url1=http%3A%2F%2Ffoo.bar%2Ffoo%3Fq%3D3%26bar%3D%2B7%2F3&url2=1234");

    query = "url1=http%3A%2F%2Ffoo.bar%2Ffoo%3Fq%3D3%26bar%3D%2B7%2F3&url2=12+34";
    CHECK(query.ToString() == "url1=http%3A%2F%2Ffoo.bar%2Ffoo%3Fq%3D3%26bar%3D%2B7%2F3&url2=12%2034");

    return 0;
}
