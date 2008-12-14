//
//	object test
//

var o = new Object();

o.a = 7;
assert(o.a == 7);
o.a = 9;

assert(o.a == 9);

o.b = new Array();
assert(o.b.length == 0);

o.b.c = 8;
o.b.d = 9;
assert(o.b.c == 8);
assert(o.b.length == 2);

delete o.b.d;
assert(o.b.length == 1);

