//
//	This script should parse correctly and do output as it proceeds
//

/* C style comments are okay too */

var x;		// Comments should work at the end of the line also

//
//	Test compound var declarations
//
var a, b, c;
var d = 1, e = "Sunny Day", f = 3 / 1;

//
//	Test new to create objects
//
var o = new Object();

//
//	Multiple statements on one line
//

var j ; var k ; j = 2 ; var l;

//
//	Test dot notation and array indexing
//
o.a = 7;
o.b = 9;
o.c = "Rainy Day";
o["a"] = 10;

//
//	Test foreach
//
for (f in o) {
	// println("  o." + f + " = " + o[f]);
}

//
//	Test arrays
//
array = new Array();
o.b = array;
o.b[1] = 8;
o.b["index"] = 8;

//
//	Test print obj
//
var cat = new Object();
var breed = new Object();
breed.country = "australia";
breed.pure = "false";
cat.name = 1;
cat.address = "9440 Lake";
cat.heritage = breed;

// printVar(cat);
// printVar(breed);


//
//	Test if
//
x = 1;
if(x==1){
	y = true;
}else{
	y = false;
}
assert(y == true);

x = 2;
assert(x == (1 + 4 / 6 + 2));


//
//	Test for
//

for (i = 0; i < 10; i++) {
	j = i;
}
