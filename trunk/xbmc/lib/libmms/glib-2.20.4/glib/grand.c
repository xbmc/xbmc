/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* Originally developed and coded by Makoto Matsumoto and Takuji
 * Nishimura.  Please mail <matumoto@math.keio.ac.jp>, if you're using
 * code from this file in your own programs or libraries.
 * Further information on the Mersenne Twister can be found at
 * http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
 * This code was adapted to glib by Sebastian Wilhelmi.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.  
 */

/* 
 * MT safe
 */

#include "config.h"

#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "glib.h"
#include "gthreadprivate.h"
#include "galias.h"

#ifdef G_OS_WIN32
#include <process.h>		/* For getpid() */
#endif

G_LOCK_DEFINE_STATIC (global_random);
static GRand* global_random = NULL;

/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

/* Tempering parameters */   
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

static guint
get_random_version (void)
{
  static gboolean initialized = FALSE;
  static guint random_version;
  
  if (!initialized)
    {
      const gchar *version_string = g_getenv ("G_RANDOM_VERSION");
      if (!version_string || version_string[0] == '\000' || 
	  strcmp (version_string, "2.2") == 0)
	random_version = 22;
      else if (strcmp (version_string, "2.0") == 0)
	random_version = 20;
      else
	{
	  g_warning ("Unknown G_RANDOM_VERSION \"%s\". Using version 2.2.",
		     version_string);
	  random_version = 22;
	}
      initialized = TRUE;
    }
  
  return random_version;
}

/* This is called from g_thread_init(). It's used to
 * initialize some static data in a threadsafe way.
 */
void 
_g_rand_thread_init (void)
{
  (void)get_random_version ();
}

struct _GRand
{
  guint32 mt[N]; /* the array for the state vector  */
  guint mti; 
};

/**
 * g_rand_new_with_seed:
 * @seed: a value to initialize the random number generator.
 * 
 * Creates a new random number generator initialized with @seed.
 * 
 * Return value: the new #GRand.
 **/
GRand*
g_rand_new_with_seed (guint32 seed)
{
  GRand *rand = g_new0 (GRand, 1);
  g_rand_set_seed (rand, seed);
  return rand;
}

/**
 * g_rand_new_with_seed_array:
 * @seed: an array of seeds to initialize the random number generator.
 * @seed_length: an array of seeds to initialize the random number generator.
 * 
 * Creates a new random number generator initialized with @seed.
 * 
 * Return value: the new #GRand.
 *
 * Since: 2.4
 **/
GRand*
g_rand_new_with_seed_array (const guint32 *seed, guint seed_length)
{
  GRand *rand = g_new0 (GRand, 1);
  g_rand_set_seed_array (rand, seed, seed_length);
  return rand;
}

/**
 * g_rand_new:
 * 
 * Creates a new random number generator initialized with a seed taken
 * either from <filename>/dev/urandom</filename> (if existing) or from 
 * the current time (as a fallback).
 * 
 * Return value: the new #GRand.
 **/
GRand* 
g_rand_new (void)
{
  guint32 seed[4];
  GTimeVal now;
#ifdef G_OS_UNIX
  static gboolean dev_urandom_exists = TRUE;

  if (dev_urandom_exists)
    {
      FILE* dev_urandom;

      do
        {
	  errno = 0;
	  dev_urandom = fopen("/dev/urandom", "rb");
	}
      while G_UNLIKELY (errno == EINTR);

      if (dev_urandom)
	{
	  int r;

	  do
	    {
	      errno = 0;
	      r = fread (seed, sizeof (seed), 1, dev_urandom);
	    }
	  while G_UNLIKELY (errno == EINTR);

	  if (r != 1)
	    dev_urandom_exists = FALSE;

	  fclose (dev_urandom);
	}	
      else
	dev_urandom_exists = FALSE;
    }
#else
  static gboolean dev_urandom_exists = FALSE;
#endif

  if (!dev_urandom_exists)
    {  
      g_get_current_time (&now);
      seed[0] = now.tv_sec;
      seed[1] = now.tv_usec;
      seed[2] = getpid ();
#ifdef G_OS_UNIX
      seed[3] = getppid ();
#else
      seed[3] = 0;
#endif
    }

  return g_rand_new_with_seed_array (seed, 4);
}

/**
 * g_rand_free:
 * @rand_: a #GRand.
 *
 * Frees the memory allocated for the #GRand.
 **/
void
g_rand_free (GRand* rand)
{
  g_return_if_fail (rand != NULL);

  g_free (rand);
}

/**
 * g_rand_copy:
 * @rand_: a #GRand.
 *
 * Copies a #GRand into a new one with the same exact state as before.
 * This way you can take a snapshot of the random number generator for
 * replaying later.
 *
 * Return value: the new #GRand.
 *
 * Since: 2.4
 **/
GRand *
g_rand_copy (GRand* rand)
{
  GRand* new_rand;

  g_return_val_if_fail (rand != NULL, NULL);

  new_rand = g_new0 (GRand, 1);
  memcpy (new_rand, rand, sizeof (GRand));

  return new_rand;
}

/**
 * g_rand_set_seed:
 * @rand_: a #GRand.
 * @seed: a value to reinitialize the random number generator.
 *
 * Sets the seed for the random number generator #GRand to @seed.
 **/
void
g_rand_set_seed (GRand* rand, guint32 seed)
{
  g_return_if_fail (rand != NULL);

  switch (get_random_version ())
    {
    case 20:
      /* setting initial seeds to mt[N] using         */
      /* the generator Line 25 of Table 1 in          */
      /* [KNUTH 1981, The Art of Computer Programming */
      /*    Vol. 2 (2nd Ed.), pp102]                  */
      
      if (seed == 0) /* This would make the PRNG procude only zeros */
	seed = 0x6b842128; /* Just set it to another number */
      
      rand->mt[0]= seed;
      for (rand->mti=1; rand->mti<N; rand->mti++)
	rand->mt[rand->mti] = (69069 * rand->mt[rand->mti-1]);
      
      break;
    case 22:
      /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
      /* In the previous version (see above), MSBs of the    */
      /* seed affect only MSBs of the array mt[].            */
      
      rand->mt[0]= seed;
      for (rand->mti=1; rand->mti<N; rand->mti++)
	rand->mt[rand->mti] = 1812433253UL * 
	  (rand->mt[rand->mti-1] ^ (rand->mt[rand->mti-1] >> 30)) + rand->mti; 
      break;
    default:
      g_assert_not_reached ();
    }
}

/**
 * g_rand_set_seed_array:
 * @rand_: a #GRand.
 * @seed: array to initialize with
 * @seed_length: length of array
 *
 * Initializes the random number generator by an array of
 * longs.  Array can be of arbitrary size, though only the
 * first 624 values are taken.  This function is useful
 * if you have many low entropy seeds, or if you require more then
 * 32bits of actual entropy for your application.
 *
 * Since: 2.4
 **/
void
g_rand_set_seed_array (GRand* rand, const guint32 *seed, guint seed_length)
{
  int i, j, k;

  g_return_if_fail (rand != NULL);
  g_return_if_fail (seed_length >= 1);

  g_rand_set_seed (rand, 19650218UL);

  i=1; j=0;
  k = (N>seed_length ? N : seed_length);
  for (; k; k--)
    {
      rand->mt[i] = (rand->mt[i] ^
		     ((rand->mt[i-1] ^ (rand->mt[i-1] >> 30)) * 1664525UL))
	      + seed[j] + j; /* non linear */
      rand->mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
      i++; j++;
      if (i>=N)
        {
	  rand->mt[0] = rand->mt[N-1];
	  i=1;
	}
      if (j>=seed_length)
	j=0;
    }
  for (k=N-1; k; k--)
    {
      rand->mt[i] = (rand->mt[i] ^
		     ((rand->mt[i-1] ^ (rand->mt[i-1] >> 30)) * 1566083941UL))
	      - i; /* non linear */
      rand->mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
      i++;
      if (i>=N)
        {
	  rand->mt[0] = rand->mt[N-1];
	  i=1;
	}
    }

  rand->mt[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */ 
}

/**
 * g_rand_int:
 * @rand_: a #GRand.
 *
 * Returns the next random #guint32 from @rand_ equally distributed over
 * the range [0..2^32-1].
 *
 * Return value: A random number.
 **/
guint32
g_rand_int (GRand* rand)
{
  guint32 y;
  static const guint32 mag01[2]={0x0, MATRIX_A};
  /* mag01[x] = x * MATRIX_A  for x=0,1 */

  g_return_val_if_fail (rand != NULL, 0);

  if (rand->mti >= N) { /* generate N words at one time */
    int kk;
    
    for (kk=0;kk<N-M;kk++) {
      y = (rand->mt[kk]&UPPER_MASK)|(rand->mt[kk+1]&LOWER_MASK);
      rand->mt[kk] = rand->mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
    }
    for (;kk<N-1;kk++) {
      y = (rand->mt[kk]&UPPER_MASK)|(rand->mt[kk+1]&LOWER_MASK);
      rand->mt[kk] = rand->mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
    }
    y = (rand->mt[N-1]&UPPER_MASK)|(rand->mt[0]&LOWER_MASK);
    rand->mt[N-1] = rand->mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];
    
    rand->mti = 0;
  }
  
  y = rand->mt[rand->mti++];
  y ^= TEMPERING_SHIFT_U(y);
  y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
  y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
  y ^= TEMPERING_SHIFT_L(y);
  
  return y; 
}

/* transform [0..2^32] -> [0..1] */
#define G_RAND_DOUBLE_TRANSFORM 2.3283064365386962890625e-10

/**
 * g_rand_int_range:
 * @rand_: a #GRand.
 * @begin: lower closed bound of the interval.
 * @end: upper open bound of the interval.
 *
 * Returns the next random #gint32 from @rand_ equally distributed over
 * the range [@begin..@end-1].
 *
 * Return value: A random number.
 **/
gint32 
g_rand_int_range (GRand* rand, gint32 begin, gint32 end)
{
  guint32 dist = end - begin;
  guint32 random;

  g_return_val_if_fail (rand != NULL, begin);
  g_return_val_if_fail (end > begin, begin);

  switch (get_random_version ())
    {
    case 20:
      if (dist <= 0x10000L) /* 2^16 */
	{
	  /* This method, which only calls g_rand_int once is only good
	   * for (end - begin) <= 2^16, because we only have 32 bits set
	   * from the one call to g_rand_int (). */
	  
	  /* we are using (trans + trans * trans), because g_rand_int only
	   * covers [0..2^32-1] and thus g_rand_int * trans only covers
	   * [0..1-2^-32], but the biggest double < 1 is 1-2^-52. 
	   */
	  
	  gdouble double_rand = g_rand_int (rand) * 
	    (G_RAND_DOUBLE_TRANSFORM +
	     G_RAND_DOUBLE_TRANSFORM * G_RAND_DOUBLE_TRANSFORM);
	  
	  random = (gint32) (double_rand * dist);
	}
      else
	{
	  /* Now we use g_rand_double_range (), which will set 52 bits for
	     us, so that it is safe to round and still get a decent
	     distribution */
	  random = (gint32) g_rand_double_range (rand, 0, dist);
	}
      break;
    case 22:
      if (dist == 0)
	random = 0;
      else 
	{
	  /* maxvalue is set to the predecessor of the greatest
	   * multiple of dist less or equal 2^32. */
	  guint32 maxvalue;
	  if (dist <= 0x80000000u) /* 2^31 */
	    {
	      /* maxvalue = 2^32 - 1 - (2^32 % dist) */
	      guint32 leftover = (0x80000000u % dist) * 2;
	      if (leftover >= dist) leftover -= dist;
	      maxvalue = 0xffffffffu - leftover;
	    }
	  else
	    maxvalue = dist - 1;
	  
	  do
	    random = g_rand_int (rand);
	  while (random > maxvalue);
	  
	  random %= dist;
	}
      break;
    default:
      random = 0;		/* Quiet GCC */
      g_assert_not_reached ();
    }      
 
  return begin + random;
}

/**
 * g_rand_double:
 * @rand_: a #GRand.
 *
 * Returns the next random #gdouble from @rand_ equally distributed over
 * the range [0..1).
 *
 * Return value: A random number.
 **/
gdouble 
g_rand_double (GRand* rand)
{    
  /* We set all 52 bits after the point for this, not only the first
     32. Thats why we need two calls to g_rand_int */
  gdouble retval = g_rand_int (rand) * G_RAND_DOUBLE_TRANSFORM;
  retval = (retval + g_rand_int (rand)) * G_RAND_DOUBLE_TRANSFORM;

  /* The following might happen due to very bad rounding luck, but
   * actually this should be more than rare, we just try again then */
  if (retval >= 1.0) 
    return g_rand_double (rand);

  return retval;
}

/**
 * g_rand_double_range:
 * @rand_: a #GRand.
 * @begin: lower closed bound of the interval.
 * @end: upper open bound of the interval.
 *
 * Returns the next random #gdouble from @rand_ equally distributed over
 * the range [@begin..@end).
 *
 * Return value: A random number.
 **/
gdouble 
g_rand_double_range (GRand* rand, gdouble begin, gdouble end)
{
  return g_rand_double (rand) * (end - begin) + begin;
}

/**
 * g_random_int:
 *
 * Return a random #guint32 equally distributed over the range
 * [0..2^32-1].
 *
 * Return value: A random number.
 **/
guint32
g_random_int (void)
{
  guint32 result;
  G_LOCK (global_random);
  if (!global_random)
    global_random = g_rand_new ();
  
  result = g_rand_int (global_random);
  G_UNLOCK (global_random);
  return result;
}

/**
 * g_random_int_range:
 * @begin: lower closed bound of the interval.
 * @end: upper open bound of the interval.
 *
 * Returns a random #gint32 equally distributed over the range
 * [@begin..@end-1].
 *
 * Return value: A random number.
 **/
gint32 
g_random_int_range (gint32 begin, gint32 end)
{
  gint32 result;
  G_LOCK (global_random);
  if (!global_random)
    global_random = g_rand_new ();
  
  result = g_rand_int_range (global_random, begin, end);
  G_UNLOCK (global_random);
  return result;
}

/**
 * g_random_double:
 *
 * Returns a random #gdouble equally distributed over the range [0..1).
 *
 * Return value: A random number.
 **/
gdouble 
g_random_double (void)
{
  double result;
  G_LOCK (global_random);
  if (!global_random)
    global_random = g_rand_new ();
  
  result = g_rand_double (global_random);
  G_UNLOCK (global_random);
  return result;
}

/**
 * g_random_double_range:
 * @begin: lower closed bound of the interval.
 * @end: upper open bound of the interval.
 *
 * Returns a random #gdouble equally distributed over the range [@begin..@end).
 *
 * Return value: A random number.
 **/
gdouble 
g_random_double_range (gdouble begin, gdouble end)
{
  double result;
  G_LOCK (global_random);
  if (!global_random)
    global_random = g_rand_new ();
 
  result = g_rand_double_range (global_random, begin, end);
  G_UNLOCK (global_random);
  return result;
}

/**
 * g_random_set_seed:
 * @seed: a value to reinitialize the global random number generator.
 * 
 * Sets the seed for the global random number generator, which is used
 * by the <function>g_random_*</function> functions, to @seed.
 **/
void
g_random_set_seed (guint32 seed)
{
  G_LOCK (global_random);
  if (!global_random)
    global_random = g_rand_new_with_seed (seed);
  else
    g_rand_set_seed (global_random, seed);
  G_UNLOCK (global_random);
}


#define __G_RAND_C__
#include "galiasdef.c"
