/*
 * sym.c -- Symbol Table module
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: sym.c,v 1.3 2002/10/24 14:44:50 bporter Exp $
 */

/******************************** Description *********************************/
/*
 *	This module implements a highly efficient generic symbol table with
 *	update and access routines. Symbols are simple character strings and
 *	the values they take can be flexible types as defined by value_t.
 *	This modules allows multiple symbol tables to be created.
 */

/********************************* Includes ***********************************/

#ifdef UEMF
	#include	"uemf.h"
#else
	#include	"basic/basicInternal.h"
#endif

/********************************* Defines ************************************/

typedef struct {						/* Symbol table descriptor */
	int		inuse;						/* Is this entry in use */
	int		hash_size;					/* Size of the table below */
	sym_t	**hash_table;				/* Allocated at run time */
} sym_tabent_t;

/********************************* Globals ************************************/

static sym_tabent_t	**sym;				/* List of symbol tables */
static int			symMax;				/* One past the max symbol table */
static int			symOpenCount = 0;	/* Count of apps using sym */

static int			htIndex;			/* Current location in table */
static sym_t*		next;				/* Next symbol in iteration */

/**************************** Forward Declarations ****************************/

static int		hashIndex(sym_tabent_t *tp, char_t *name);
static sym_t	*hash(sym_tabent_t *tp, char_t *name);
static int		calcPrime(int size);

/*********************************** Code *************************************/
/*
 *	Open the symbol table subSystem.
 */

int symSubOpen()
{
	if (++symOpenCount == 1) {
		symMax = 0;
		sym = NULL;
	}
	return 0;
}

/******************************************************************************/
/*
 *	Close the symbol table subSystem.
 */

void symSubClose()
{
	if (--symOpenCount <= 0) {
		symOpenCount = 0;
	}
}

/******************************************************************************/
/*
 *	Create a symbol table.
 */

sym_fd_t symOpen(int hash_size)
{
	sym_fd_t		sd;
	sym_tabent_t	*tp;

	a_assert(hash_size > 2);

/*
 *	Create a new handle for this symbol table
 */
	if ((sd = hAlloc((void***) &sym)) < 0) {
		return -1;
	}

/*
 *	Create a new symbol table structure and zero
 */
	if ((tp = (sym_tabent_t*) balloc(B_L, sizeof(sym_tabent_t))) == NULL) {
		symMax = hFree((void***) &sym, sd);
		return -1;
	}
	memset(tp, 0, sizeof(sym_tabent_t));
	if (sd >= symMax) {
		symMax = sd + 1;
	}
	a_assert(0 <= sd && sd < symMax);
	sym[sd] = tp;

/*
 *	Now create the hash table for fast indexing.
 */
	tp->hash_size = calcPrime(hash_size);
	tp->hash_table = (sym_t**) balloc(B_L, tp->hash_size * sizeof(sym_t*));
	a_assert(tp->hash_table);
	memset(tp->hash_table, 0, tp->hash_size * sizeof(sym_t*));

	return sd;
}

/******************************************************************************/
/*
 *	Close this symbol table. Call a cleanup function to allow the caller
 *	to free resources associated with each symbol table entry.
 */

void symClose(sym_fd_t sd)
{
	sym_tabent_t	*tp;
	sym_t			*sp, *forw;
	int				i;

	a_assert(0 <= sd && sd < symMax);
	tp = sym[sd];
	a_assert(tp);

/*
 *	Free all symbols in the hash table, then the hash table itself.
 */
	for (i = 0; i < tp->hash_size; i++) {
		for (sp = tp->hash_table[i]; sp; sp = forw) {
			forw = sp->forw;
			valueFree(&sp->name);
			valueFree(&sp->content);
			bfree(B_L, (void*) sp);
			sp = forw;
		}
	}
	bfree(B_L, (void*) tp->hash_table);

	symMax = hFree((void***) &sym, sd);
	bfree(B_L, (void*) tp);
}

/******************************************************************************/
/*
 *	Return the first symbol in the hashtable if there is one. This call is used
 *	as the first step in traversing the table. A call to symFirst should be
 *	followed by calls to symNext to get all the rest of the entries.
 */

sym_t* symFirst(sym_fd_t sd)
{
	sym_tabent_t	*tp;
	sym_t			*sp, *forw;
	int				i;

	a_assert(0 <= sd && sd < symMax);
	tp = sym[sd];
	a_assert(tp);

/*
 *	Find the first symbol in the hashtable and return a pointer to it.
 */
	for (i = 0; i < tp->hash_size; i++) {
		for (sp = tp->hash_table[i]; sp; sp = forw) {
			forw = sp->forw;

			if (forw == NULL) {
				htIndex = i + 1;
				next = tp->hash_table[htIndex];
			} else {
				htIndex = i;
				next = forw;
			}
			return sp;
		}
	}
	return NULL;
}

/******************************************************************************/
/*
 *	Return the next symbol in the hashtable if there is one. See symFirst.
 */

sym_t* symNext(sym_fd_t sd)
{
	sym_tabent_t	*tp;
	sym_t			*sp, *forw;
	int				i;

	a_assert(0 <= sd && sd < symMax);
	tp = sym[sd];
	a_assert(tp);

/*
 *	Find the first symbol in the hashtable and return a pointer to it.
 */
	for (i = htIndex; i < tp->hash_size; i++) {
		for (sp = next; sp; sp = forw) {
			forw = sp->forw;

			if (forw == NULL) {
				htIndex = i + 1;
				next = tp->hash_table[htIndex];
			} else {
				htIndex = i;
				next = forw;
			}
			return sp;
		}
		next = tp->hash_table[i + 1];
	}
	return NULL;
}

/******************************************************************************/
/*
 *	Lookup a symbol and return a pointer to the symbol entry. If not present
 *	then return a NULL.
 */

sym_t *symLookup(sym_fd_t sd, char_t *name)
{
	sym_tabent_t	*tp;
	sym_t			*sp;
	char_t			*cp;

	a_assert(0 <= sd && sd < symMax);
	if ((tp = sym[sd]) == NULL) {
		return NULL;
	}

	if (name == NULL || *name == '\0') {
		return NULL;
	}

/*
 *	Do an initial hash and then follow the link chain to find the right entry
 */
	for (sp = hash(tp, name); sp; sp = sp->forw) {
		cp = sp->name.value.string;
		if (cp[0] == name[0] && gstrcmp(cp, name) == 0) {
			break;
		}
	}
	return sp;
}

/******************************************************************************/
/*
 *	Enter a symbol into the table. If already there, update its value.
 *	Always succeeds if memory available. We allocate a copy of "name" here
 *	so it can be a volatile variable. The value "v" is just a copy of the
 *	passed in value, so it MUST be persistent.
 */

sym_t *symEnter(sym_fd_t sd, char_t *name, value_t v, int arg)
{
	sym_tabent_t	*tp;
	sym_t			*sp, *last;
	char_t			*cp;
	int				hindex;

	a_assert(name);
	a_assert(0 <= sd && sd < symMax);
	tp = sym[sd];
	a_assert(tp);

/*
 *	Calculate the first daisy-chain from the hash table. If non-zero, then
 *	we have daisy-chain, so scan it and look for the symbol.
 */
	last = NULL;
	hindex = hashIndex(tp, name);
	if ((sp = tp->hash_table[hindex]) != NULL) {
		for (; sp; sp = sp->forw) {
			cp = sp->name.value.string;
			if (cp[0] == name[0] && gstrcmp(cp, name) == 0) {
				break;
			}
			last = sp;
		}
		if (sp) {
/*
 *			Found, so update the value
 *			If the caller stores handles which require freeing, they
 *			will be lost here. It is the callers responsibility to free
 *			resources before overwriting existing contents. We will here
 *			free allocated strings which occur due to value_instring().
 *			We should consider providing the cleanup function on the open rather
 *			than the close and then we could call it here and solve the problem.
 */
			if (sp->content.valid) {
				valueFree(&sp->content);
			}
			sp->content = v;
			sp->arg = arg;
			return sp;
		}
/*
 *		Not found so allocate and append to the daisy-chain
 */
		sp = (sym_t*) balloc(B_L, sizeof(sym_t));
		if (sp == NULL) {
			return NULL;
		}
		sp->name = valueString(name, VALUE_ALLOCATE);
		sp->content = v;
		sp->forw = (sym_t*) NULL;
		sp->arg = arg;
		last->forw = sp;

	} else {
/*
 *		Daisy chain is empty so we need to start the chain
 */
		sp = (sym_t*) balloc(B_L, sizeof(sym_t));
		if (sp == NULL) {
			return NULL;
		}
		tp->hash_table[hindex] = sp;
		tp->hash_table[hashIndex(tp, name)] = sp;

		sp->forw = (sym_t*) NULL;
		sp->content = v;
		sp->arg = arg;
		sp->name = valueString(name, VALUE_ALLOCATE);
	}
	return sp;
}

/******************************************************************************/
/*
 *	Delete a symbol from a table
 */

int symDelete(sym_fd_t sd, char_t *name)
{
	sym_tabent_t	*tp;
	sym_t			*sp, *last;
	char_t			*cp;
	int				hindex;

	a_assert(name && *name);
	a_assert(0 <= sd && sd < symMax);
	tp = sym[sd];
	a_assert(tp);

/*
 *	Calculate the first daisy-chain from the hash table. If non-zero, then
 *	we have daisy-chain, so scan it and look for the symbol.
 */
	last = NULL;
	hindex = hashIndex(tp, name);
	if ((sp = tp->hash_table[hindex]) != NULL) {
		for ( ; sp; sp = sp->forw) {
			cp = sp->name.value.string;
			if (cp[0] == name[0] && gstrcmp(cp, name) == 0) {
				break;
			}
			last = sp;
		}
	}
	if (sp == (sym_t*) NULL) {				/* Not Found */
		return -1;
	}

/*
 *	Unlink and free the symbol. Last will be set if the element to be deleted
 *	is not first in the chain.
 */
	if (last) {
		last->forw = sp->forw;
	} else {
		tp->hash_table[hindex] = sp->forw;
	}
	valueFree(&sp->name);
	valueFree(&sp->content);
	bfree(B_L, (void*) sp);

	return 0;
}

/******************************************************************************/
/*
 *	Hash a symbol and return a pointer to the hash daisy-chain list
 *	All symbols reside on the chain (ie. none stored in the hash table itself)
 */

static sym_t *hash(sym_tabent_t *tp, char_t *name)
{
	a_assert(tp);

	return tp->hash_table[hashIndex(tp, name)];
}

/******************************************************************************/
/*
 *	Compute the hash function and return an index into the hash table
 *	We use a basic additive function that is then made modulo the size of the
 *	table.
 */

static int hashIndex(sym_tabent_t *tp, char_t *name)
{
	unsigned int	sum;
	int				i;

	a_assert(tp);
/*
 *	Add in each character shifted up progressively by 7 bits. The shift
 *	amount is rounded so as to not shift too far. It thus cycles with each
 *	new cycle placing character shifted up by one bit.
 */
	i = 0;
	sum = 0;
	while (*name) {
		sum += (((int) *name++) << i);
		i = (i + 7) % (BITS(int) - BITSPERBYTE);
	}
	return sum % tp->hash_size;
}

/******************************************************************************/
/*
 *	Check if this number is a prime
 */

static int isPrime(int n)
{
	int		i, max;

	a_assert(n > 0);

	max = n / 2;
	for (i = 2; i <= max; i++) {
		if (n % i == 0) {
			return 0;
		}
	}
	return 1;
}

/******************************************************************************/
/*
 *	Calculate the largest prime smaller than size.
 */

static int calcPrime(int size)
{
	int count;

	a_assert(size > 0);

	for (count = size; count > 0; count--) {
		if (isPrime(count)) {
			return count;
		}
	}
	return 1;
}

/******************************************************************************/


