#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#define FILL_CHARACTER '.'

#define mytoupper(x) (islower(x) ? toupper(x) : (x))

int
main(int argc, char *argv[])
{
  unsigned int letters[0x100];
  unsigned int order[0x100];
  unsigned char *pairs[0x100];
  FILE *f;
  unsigned int i, j, k;
  int first, second;

  /* read letters */
  for (i = 0; i < 0x100; i++) {
    letters[i] = 0;
    pairs[i] = NULL;
  }

  f = fopen(argv[2], "r");
  while (fscanf(f, "%lx", &j) == 1)
    letters[j] = 0xff;
  fclose(f);

  /* read stuff */
  j = 0;
  while ((first = getchar()) != EOF) {
    pairs[first] = (unsigned char*)malloc(0x101);
    order[j] = first;
    letters[first] = j++;
    getchar();  /* : */
    i = 0;
    while ((second = getchar()) != '\n')
      pairs[first][i++] = (unsigned char*)second;
    pairs[first][i] = '\0';
  }
  assert(order[0] == FILL_CHARACTER);

  puts("/* THIS IS A GENERATED TABLE, see data/pairtoc.c. */");
  printf("static const unsigned char LETTER_");
  for (i = 0; argv[1][i]; i++)
    printf("%c", mytoupper(argv[1][i]));
  puts("[] = {");

  for (i = 0; i < 0x100; i++) {
    if (i % 8 == 0)
      printf("  ");
    printf("%3u", letters[i]);
    if (i % 8 == 7)
      printf(",  /* 0x%02x */\n", i-7);
    else
      printf(", ");
  }
  puts("};\n");

  puts("/* THIS IS A GENERATED TABLE, see data/pairtoc.c. */");
  printf("static const unsigned char *PAIR_");
  for (i = 0; argv[1][i]; i++)
    printf("%c", mytoupper(argv[1][i]));
  puts("[] = {\n");

  printf("  (unsigned char*)\"");
  for (k = 0; pairs[FILL_CHARACTER][k]; k++) {
    printf("\\x%02x", pairs[FILL_CHARACTER][k]);
  }
  printf("\",  /* FILLCHAR */\n");

  for (i = 1; i < j; i++) {
    printf("  (unsigned char*)\"");
    for (k = 0; pairs[order[i]][k]; k++) {
      if (pairs[order[i]][k] == FILL_CHARACTER)
        printf(".");
      else
        printf("\\x%02x", pairs[order[i]][k]);
    }
    printf("\",  /* 0x%02x */\n", order[i]);
  }
  puts("};\n");

  return 0;
}
