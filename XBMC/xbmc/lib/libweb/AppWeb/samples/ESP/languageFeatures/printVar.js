//
//	printVar test
//

var cat = new Object();
var breed = new Object();

breed.country = "australia";
breed.pure = false;

cat.name = "fluffy";
cat.address = "9440 Lake";
cat.heritage = breed;

println(cat);
printVars(cat);
printVars(breed);
