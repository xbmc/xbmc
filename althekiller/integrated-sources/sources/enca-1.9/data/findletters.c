#include <stdio.h>

int
main(int argc, char *argv[])
{
  unsigned int map[0x100];
  unsigned int letter_map[0x10000];
  unsigned int i;
  FILE *f;

  f = fopen(argv[1], "r");
  for (i = 0; i < 0x100; i++)
    fscanf(f, "%x", map + i);
  fclose(f);

  for (i = 0; i < 0x10000; i++)
    letter_map[i] = 0;

  while (scanf("%x", &i) == 1)
    letter_map[i] = 1;

  for (i = 0; i < 0x100; i++) {
    if (letter_map[map[i]])
      printf("%02x\n", i);
  }

  return 0;
}
