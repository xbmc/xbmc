#ifndef _GOOMTOOLS_H
#define _GOOMTOOLS_H

/**
 * Random number generator wrapper for faster random number.
 */

#ifdef _WIN32PC
#define inline __inline
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define random rand
#define bzero(x,y) memset(x,0,y)
#endif

#define GOOM_NB_RAND 0x10000

typedef struct _GOOM_RANDOM {
	int array[GOOM_NB_RAND];
	unsigned short pos;
} GoomRandom;

GoomRandom *goom_random_init(int i);
void goom_random_free(GoomRandom *grandom);

inline static int goom_random(GoomRandom *grandom) {
	
	grandom->pos++; /* works because pos is an unsigned short */
	return grandom->array[grandom->pos];
}

inline static int goom_irand(GoomRandom *grandom, int i) {

	grandom->pos++;
	return grandom->array[grandom->pos] % i;
}

/* called to change the specified number of value in the array, so that the array does not remain the same*/
void goom_random_update_array(GoomRandom *grandom, int numberOfValuesToChange);

#endif
