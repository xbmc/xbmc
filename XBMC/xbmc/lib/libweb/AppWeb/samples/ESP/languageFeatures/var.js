//
//	var test
//

var x;
assert(x == undefined);

var a, b, c;
assert(a == undefined);
assert(b == undefined);
assert(c == undefined);
assert(a == undefined && b == undefined && c == undefined);

var d = 1, e = "Sunny Day", f = 3 / 1;
assert(d == 1);
assert(f == 3);
assert(e == "Sunny Day");

x = 4;
assert(x == 4);

var undefVar;
x = undefVar;
assert(x == undefined);
if (undefVar != undefined) {
	assert(0);
}

var x;
assert(x == undefined);

//	Should fail
// 	var o.y;

