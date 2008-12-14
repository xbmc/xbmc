//
//	Array test
//

var o = new Array();
assert(o.length == 0);

o["abc"] = 8;
assert(o["abc"] == 8);
assert(o.length == 1);

o[1] = 7;
assert(o[1] == 7);
assert(o.length == 2);

//
//	This should fail, length is readonly
//
//	o.length = 99;
//
