#include <stdio.h>
#include <assert.h>

#define NO_CHR 0xffff

#define FILL_CHARACTER '.'

int
main(int argc, char *argv[])
{
  unsigned int letters[0x100];
  unsigned long int count[0x10000];
  FILE *f;

  int i, killme, c, last;
  unsigned long j, sum;

  /* read letters */
  for (i = 0; i < 0x100; i++)
    letters[i] = 0;

  f = fopen(argv[1], "r");
  while (fscanf(f, "%lx", &j) == 1)
    letters[j] = 1;
  fclose(f);

  /* read stuff */
  for (i = 0; i < 0x10000; i++)
    count[i] = 0;

  killme = 0;
  last = FILL_CHARACTER;
  do {
    c = getchar();
    killme = (c == EOF);
    if (c == EOF || !letters[c])
      c = FILL_CHARACTER;
    count[last*0x100 + c]++;
    last = c;
  } while (!killme);

  /* note we put things into the same array. that's ugly. */
  sum = 0;
  last = 0;
  for (i = 0; i < 0x10000; i++) {
    if (i/0x100 == FILL_CHARACTER && i%0x100 == FILL_CHARACTER)
      count[i] = 0;
    else {
      assert(i >= last);
      if (count[i]) {
        sum += count[last++] = count[i];
        count[last++] = i;
      }
    }
  }

  /* sort by count */
  last /= 2;
  do {
    killme = 1;
    for (i = 1; i < last; i++) {
      if (count[2*i] > count[2*i-2]) {
        killme = 0;
        j = count[2*i];
        count[2*i] = count[2*i-2];
        count[2*i-2] = j;
        j = count[2*i+1];
        count[2*i+1] = count[2*i-1];
        count[2*i-1] = j;
      }
    }
  } while (!killme);

  /* kill small */
  sum = 0.95*sum;
  j = 0;
  for (i = 0; i < last; i++) {
    j += count[2*i];
    if (j > sum)
      break;
  }
  last = i;

  /* sort by first again */
  do {
    killme = 1;
    for (i = 1; i < last; i++) {
      /* note we sort by first letter only, so the second letters will be
       * sorted by frequency, which is exactly what we want */
      if (count[2*i+1]/0x100 < count[2*i-1]/0x100) {
        killme = 0;
        j = count[2*i];
        count[2*i] = count[2*i-2];
        count[2*i-2] = j;
        j = count[2*i+1];
        count[2*i+1] = count[2*i-1];
        count[2*i-1] = j;
      }
    }
  } while (!killme);

  i = 0;
  while (i < last) {
    c = count[2*i+1]/0x100;
    printf("%c:", c);
    while (i < last && count[2*i+1]/0x100 == c) {
      printf("%c", count[2*i+1]%0x100);
      i++;
    }
    printf("\n");
  }
  return 0;
}
/* vim: ts=2
 */
