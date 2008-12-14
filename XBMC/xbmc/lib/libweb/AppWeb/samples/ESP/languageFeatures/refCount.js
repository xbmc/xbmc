//
//	refCount test
//

var a = new Object();
assert(refCount(a) > 1);

b = a;
assert(refCount(a) > 2);

c = b;
assert(refCount(a) > 3);

c = 1;
assert(refCount(a) > 2);

b = 1;
assert(refCount(a) > 1);

a = 1;
assert(refCount(a) == 0);
