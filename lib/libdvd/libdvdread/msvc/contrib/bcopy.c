#include <string.h>

void bcopy(const void *IN, void *OUT, size_t N);

void bcopy(const void *IN, void *OUT, size_t N)
{
  memcpy(OUT, IN, N);
}
