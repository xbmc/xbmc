#include <stdio.h>
#include <ctype.h>

#define mytoupper(x) (islower(x) ? toupper(x) : (x))

int
main(int argc, char *argv[])
{
  unsigned int weights[0x100];
  unsigned int i, j;
  unsigned char c;

  for (i = 0; i < 0x100; i++)
    weights[i] = 0;

  while (scanf("%c %u\n", &c, &j) == 2)
    weights[c] = j;

  puts("/* THIS IS A GENERATED TABLE, see data/basetoc.c. */");
  printf("static const unsigned short int RAW_");
  for (i = 0; argv[1][i]; i++)
    printf("%c", mytoupper(argv[1][i]));
  puts("[] = {");

  for (i = 0; i < 0x100; i++) {
    if (i % 8 == 0)
      printf("  ");
    printf("%4u", weights[i]);
    if (i % 8 == 7)
      printf(",  /* 0x%02x */\n", i-7);
    else
      printf(", ");
  }
  puts("};\n");

  return 0;
}
