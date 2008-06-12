#include "includes.h"

 int main(void)
{
	fstring dest;
	char *ptr = dest;

	printf("running on valgrind? %d\n", RUNNING_ON_VALGRIND);

	/* Try copying a string into an fstring buffer.  The string
	 * will actually fit, but this is still wrong because you
	 * can't pstrcpy into an fstring.  This should trap in a
	 * developer build. */

#if 0
	/* As of CVS 20030318, this will be trapped at compile time! */
	pstrcpy(dest, "hello");
#endif /* 0 */

	pstrcpy(ptr, "hello!");

	return 0;
}
