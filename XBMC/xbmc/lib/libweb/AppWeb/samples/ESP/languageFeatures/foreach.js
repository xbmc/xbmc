//
//	for in test
//

var cat = new Object();
f="name";
cat[f] = 2;

cat.name = "Felix the Cat";
cat.address = "1286 NE 8th Street";
cat.age = 9;

for (var s in cat) {
	println("  cat." + s + " = " + cat[s]);
}
