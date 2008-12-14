//
//	Test methods
//

function MyObj() {
	this.x = 1;
	this.y = 2;
}

function toValue() {
	return 48;
}

var o = new new MyObj();

//MyObj.prototype.z = 7;
//MyObj.prototype.z = 7;


//
//	Assign method
//
o.toValue = toValue;

println("toValue is " + o.toValue());
assert(o.toValue() == 48);
assert(o.x == 1);

x = "abc" + o.toValue();
println("x is " + x);
assert(x == "abc48");
