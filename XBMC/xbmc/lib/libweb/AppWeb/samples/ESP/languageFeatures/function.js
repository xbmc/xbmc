//
//	Function test
//

function myFunc(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q) {
	assert(q == "last");

	for (i = 0; i < arguments.length; i++) {
		println("arg[" + i + "] = " + arguments[i]);
	}
}

myFunc(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, "last");
