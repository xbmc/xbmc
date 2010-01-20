#include <ctype.h>
#include <stdio.h>

#define isbinary(x) (iscntrl(x) || (x) == 127)

#define FILL_CHARACTER '.'

int
main(void)
{
  unsigned long int count[0x100];
  int i, c;

  for (i = 0; i < 0x100; i++)
    count[i]=0;

  while ((c = getchar()) != EOF)
    count[c]++;

  for (i = 0; i < 0x100; i++) {
    if (count[i]) {
      printf("0x%02x ", i);
      printf("%c %lu\n", isbinary(i) ? '.' : i, count[i]);
    }
  }

  return 0;
}
