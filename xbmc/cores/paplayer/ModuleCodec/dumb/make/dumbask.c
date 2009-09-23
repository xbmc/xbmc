#include <stdio.h>
#include <ctype.h>


int main(int argc, const char *const argv[])
{
	const char *message = argv[1];
	const char *options;

	if (!message) {
		fprintf(stderr,
			"dumbask: asks the user a question.\n"
			"Specify a message as the first argument (quoted!).\n"
			"You may optionally specify the choices as the second argument.\n"
			"Default choices are YN. Exit code is 0 for first, 1 for second, etc.\n");
		return 0;
	}

	options = argv[2] ? : "YN"; /* I _had_ to use a GNU Extension _somewhere_! */

	printf("%s", argv[1]);

	for (;;) {
		char c = getchar();
		if (c == EOF) return 0;
		c = toupper(c);
		int i;
		for (i = 0; options[i]; i++)
			if (c == toupper(options[i]))
				return i;
	}
}
