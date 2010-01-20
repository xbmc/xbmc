#include "pixeldoubler.h"
#include <stdlib.h>
#include <string.h>

void pixel_doubler (Surface *src, Surface *dest) {
  register int *d; // pointeur sur le pixel courant a marquer
  register int *s; // pointeur sur le pixel coutant en cours de lecture
  int sw;  // nombre d'octet de largeur de ligne de la surface source
  int sw2;
  int fd;  // adresse de la fin du buffer destination
  int fin; // adresse de fin d'une ligne du buffer source

  d = dest->buf;
  s = src->buf;

  sw = src->width << 2;
  sw2 = sw << 1;

  fin = (int)s;
  fd = (int)d + (dest->size<<2);
  
  // tant que tout le buffer source n'est pas remplit
  while ((int)d < fd) {

    // passer a la ligne suivante du buffer source
    fin += sw;

    // l'afficher sur une ligne du buffer destination
    while ((int)s < fin) {
      register int col = *(s++);
      // 2 affichage par point du buffer source (doubling horizontal)
      *(d++) = col; *(d++) = col;
    }

    // puis l'afficher sur une autre ligne (doubling vertical)
    memcpy (d, ((char*)d) - sw2, sw2);
/*    s = (int*)((int)s - sw); // retour au debut de la ligne src
    while ((int)s < fin) {
      register int col = *(s++);
      *(d++) = col; *(d++) = col; // idem (cf plus haut)
    } */
    d = (int*)((char*)d + sw2);
  }
}
