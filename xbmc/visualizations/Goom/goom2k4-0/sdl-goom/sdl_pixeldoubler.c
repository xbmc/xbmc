#include "pixeldoubler.h"
#include <SDL/SDL.h>
#include <stdlib.h>
#include <string.h>

void sdl_pixel_doubler (Surface *src, SDL_Surface *dest) {
  register int *d; // pointeur sur le pixel courant a marquer
  register int *s; // pointeur sur le pixel coutant en cours de lecture
  int sw;  // nombre d'octet de largeur de ligne de la surface source
  int sw2,swd;
  int fd;  // adresse de la fin du buffer destination
  int fin; // adresse de fin d'une ligne du buffer source

  SDL_LockSurface (dest);

  d = dest->pixels;
  s = src->buf;

  sw = src->width << 2;
  sw2 = dest->pitch;
  swd = sw2 - sw * 2;

  fin = (int)s;
  fd = (int)d + (sw2 * src->height * 2);
  
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
    d = (int*)((char*)d + swd);

    // puis l'afficher sur une autre ligne (doubling vertical)
    memcpy (d, ((char*)d) - sw2, sw2);
/*    s = (int*)((int)s - sw); // retour au debut de la ligne src
    while ((int)s < fin) {
      register int col = *(s++);
      *(d++) = col; *(d++) = col; // idem (cf plus haut)
    } */
    d = (int*)((char*)d + sw2);
  }

  SDL_UnlockSurface (dest);
}
