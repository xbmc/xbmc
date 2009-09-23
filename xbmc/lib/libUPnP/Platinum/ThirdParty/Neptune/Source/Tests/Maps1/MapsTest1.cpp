/*****************************************************************
|
|      Maps Test Program 1
|
|      (c) 2005-2006 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Neptune.h"
#include "NptDebug.h"

/*----------------------------------------------------------------------
|       globals
+---------------------------------------------------------------------*/
static unsigned int A_Count = 0;

/*----------------------------------------------------------------------
|       types
+---------------------------------------------------------------------*/
class A {
public:
    A() : _a(0), _b(0), _c(&_a) {
        printf("A::A()\n");
        A_Count++;
    }
    A(int a, char b) : _a(a), _b(b), _c(&_a) {
        A_Count++;
    }
    A(const A& other) : _a(other._a), _b(other._b), _c(&_a) {
        A_Count++;
    }
    ~A() {
        A_Count--;
    }
    bool Check() { return _c == &_a; }
    bool operator==(const A& other) const {
        return _a == other._a && _b == other._b;
    }
    int _a;
    char _b;
    int* _c;
};

#define CHECK(x) {                                  \
    if (!(x)) {                                     \
        printf("TEST FAILED line %d\n", __LINE__);  \
        return 1;                                   \
    }                                               \
}

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int
main(int /*argc*/, char** /*argv*/)
{
    NPT_Map<NPT_String,A> a_map;
    A* a = NULL;

    CHECK(a_map.GetEntryCount() == 0);
    CHECK(a_map.HasKey("hello") == false);
    CHECK(!a_map.HasValue(A(1,2)));
    CHECK(NPT_FAILED(a_map.Get("bla", a)));
    CHECK(a == NULL);

    a_map.Put("hello", A(1,2));
    CHECK(a_map.GetEntryCount() == 1);
    CHECK(NPT_SUCCEEDED(a_map.Get("hello", a)));
    CHECK(*a == A(1,2));
    CHECK(a_map.HasKey("hello"));
    CHECK(a_map.HasValue(A(1,2)));
    CHECK(a_map["hello"] == A(1,2));
    
    CHECK(a_map["bla"] == A());
    CHECK(a_map.GetEntryCount() == 2);
    a_map["bla"] = A(3,4);
    CHECK(a_map["bla"] == A(3,4));
    CHECK(a_map.GetEntryCount() == 2);

    NPT_Map<NPT_String,A> b_map;
    b_map["hello"] = A(1,2);
    b_map["bla"] = A(3,4);
    CHECK(a_map == b_map);

    NPT_Map<NPT_String,A> c_map = a_map;
    CHECK(c_map["hello"] == a_map["hello"]);
    CHECK(c_map["bla"] == a_map["bla"]);

    CHECK(NPT_SUCCEEDED(a_map.Put("bla", A(5,6))));
    CHECK(NPT_SUCCEEDED(a_map.Get("bla", a)));
    CHECK(*a == A(5,6));
    CHECK(NPT_FAILED(a_map.Get("youyou", a)));

    b_map.Clear();
    CHECK(b_map.GetEntryCount() == 0);

    a_map["youyou"] = A(6,7);
    CHECK(NPT_FAILED(a_map.Erase("coucou")));
    CHECK(NPT_SUCCEEDED(a_map.Erase("bla")));
    CHECK(!a_map.HasKey("bla"));

    CHECK(!(a_map == c_map));
    CHECK(c_map != a_map);

    c_map = a_map;
    NPT_Map<NPT_String,A> d_map(c_map);
    CHECK(d_map == c_map);

    NPT_Map<int,int> i_map;
    i_map[5] = 6;
    i_map[6] = 7;
    i_map[9] = 0;
    CHECK(i_map[0] == 0 || i_map[0] != 0); // unknown value (will cause a valgrind warning)
    CHECK(i_map.GetEntryCount() == 4);

    NPT_Map<NPT_String,A> a1_map;
    NPT_Map<NPT_String,A> a2_map;
    a1_map["hello"] = A(1,2);
    a1_map["bla"]   = A(2,3);
    a1_map["youyou"]= A(3,4);
    a2_map["bla"]   = A(2,3);
    a2_map["youyou"]= A(3,4);
    a2_map["hello"] = A(1,2);
    CHECK(a1_map == a2_map);
    a1_map["foo"] = A(0,0);
    CHECK(a1_map != a2_map);
    a2_map["foo"] = A(0,0);
    CHECK(a1_map == a2_map);
    a2_map["foo"] = A(7,8);
    CHECK(a1_map != a2_map);
    a2_map["foo"] = A(0,0);
    a1_map["bir"] = A(0,0);
    a2_map["bar"] = A(0,0);
    CHECK(a1_map.GetEntryCount() == a2_map.GetEntryCount());
    CHECK(a1_map != a2_map);
    CHECK(!(a1_map == a2_map));

    return 0;
}
