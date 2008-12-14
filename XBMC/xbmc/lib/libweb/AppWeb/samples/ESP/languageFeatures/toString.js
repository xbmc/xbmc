//
//	Define a toString method and cast an object to a string
//
function toString() {
	return this.name;
}

var cat = new Object();
cat.name = "Felix the Cat";
cat.toString = toString;

println("Name: " + cat);

