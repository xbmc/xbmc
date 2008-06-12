/*
 * ringq.c -- Ring queue buffering module
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: ringq.c,v 1.4 2003/09/17 14:45:03 bporter Exp $
 */

/******************************** Description *********************************/

/*
 *	A ring queue allows maximum utilization of memory for data storage and is
 *	ideal for input/output buffering.  This module provides a highly efficient
 *	implementation and a vehicle for dynamic strings.
 *
 *	WARNING:  This is a public implementation and callers have full access to
 *	the queue structure and pointers.  Change this module very carefully.
 *
 *	This module follows the open/close model.
 *
 *	Operation of a ringq where rq is a pointer to a ringq :
 *
 *		rq->buflen contains the size of the buffer.
 *		rq->buf will point to the start of the buffer.
 *		rq->servp will point to the first (un-consumed) data byte.
 *		rq->endp will point to the next free location to which new data is added
 *		rq->endbuf will point to one past the end of the buffer.
 *
 *	Eg. If the ringq contains the data "abcdef", it might look like :
 *
 *	+-------------------------------------------------------------------+
 *  |   |   |   |   |   |   |   | a | b | c | d | e | f |   |   |   |   |
 *	+-------------------------------------------------------------------+
 *    ^                           ^                       ^               ^
 *    |                           |                       |               |
 *  rq->buf                    rq->servp               rq->endp      rq->enduf
 *     
 *	The queue is empty when servp == endp.  This means that the queue will hold
 *	at most rq->buflen -1 bytes.  It is the filler's responsibility to ensure
 *	the ringq is never filled such that servp == endp.
 *
 *	It is the filler's responsibility to "wrap" the endp back to point to
 *	rq->buf when the pointer steps past the end.  Correspondingly it is the
 *	consumers responsibility to "wrap" the servp when it steps to rq->endbuf.
 *	The ringqPutc and ringqGetc routines will do this automatically.
 */

/********************************* Includes ***********************************/

#ifdef UEMF
	#include	"uemf.h"
#else
	#include	"basic/basicInternal.h"
#endif

/*********************************** Defines **********************************/
/*
 *	Faster than a function call
 */

#define RINGQ_LEN(rq) \
	((rq->servp > rq->endp) ? \
		(rq->buflen + (rq->endp - rq->servp)) : \
		(rq->endp - rq->servp))

/***************************** Forward Declarations ***************************/

static int	ringqGrow(ringq_t *rq);
static int	getBinBlockSize(int size);

int			ringqGrowCalls = 0;

/*********************************** Code *************************************/
/*
 *	Create a new ringq. "increment" is the amount to increase the size of the
 *	ringq should it need to grow to accomodate data being added. "maxsize" is
 *	an upper limit (sanity level) beyond which the q must not grow. Set maxsize
 *	to -1 to imply no upper limit. The buffer for the ringq is always 
 *	dynamically allocated. Set maxsize
 */

int ringqOpen(ringq_t *rq, int initSize, int maxsize)
{
	int	increment;

	a_assert(rq);
	a_assert(initSize >= 0);

	increment = getBinBlockSize(initSize);
	if ((rq->buf = balloc(B_L, (increment))) == NULL) {
		return -1;
	}
	rq->maxsize = maxsize;
	rq->buflen = increment;
	rq->increment = increment;
	rq->endbuf = &rq->buf[rq->buflen];
	rq->servp = rq->buf;
	rq->endp = rq->buf;
	*rq->servp = '\0';
	return 0;
}

/******************************************************************************/
/*
 *	Delete a ringq and free the ringq buffer.
 */

void ringqClose(ringq_t *rq)
{
	a_assert(rq);
	a_assert(rq->buflen == (rq->endbuf - rq->buf));

	if (rq == NULL) {
		return;
	}

	ringqFlush(rq);
	bfree(B_L, (char*) rq->buf);
	rq->buf = NULL;
}

/******************************************************************************/
/*
 *	Return the length of the data in the ringq. Users must fill the queue to 
 *	a high water mark of at most one less than the queue size.
 */

int ringqLen(ringq_t *rq)
{
	a_assert(rq);
	a_assert(rq->buflen == (rq->endbuf - rq->buf));

	if (rq->servp > rq->endp) {
		return rq->buflen + rq->endp - rq->servp;
	} else {
		return rq->endp - rq->servp;
	}
}

/******************************************************************************/
/*
 *	Get a byte from the queue
 */

int ringqGetc(ringq_t *rq)
{
	char_t	c;
	char_t*	cp;

	a_assert(rq);
	a_assert(rq->buflen == (rq->endbuf - rq->buf));

	if (rq->servp == rq->endp) {
		return -1;
	}

	cp = (char_t*) rq->servp;
	c = *cp++;
	rq->servp = (unsigned char *) cp;
	if (rq->servp >= rq->endbuf) {
		rq->servp = rq->buf;
	}
   /*
    * 17 Sep 03 BgP -- using the implicit conversion from signed char to
    * signed int in the return below makes this function work incorrectly when
    * dealing with UTF-8 encoded text. UTF-8 may include characters that are >
    * 127, which a signed char treats as negative. When we return a 'negative'
    * value from this function, it gets converted to a negative 
    * integer, instead of a small positive integer, which is what we want. 
    * So, we cast to (unsigned char) before returning, and the problem goes
    * away...
    */
	return (int) ((unsigned char) c);
}

/******************************************************************************/
/*
 *	Add a char to the queue. Note if being used to store wide strings 
 *	this does not add a trailing '\0'. Grow the q as required.
 */

int ringqPutc(ringq_t *rq, char_t c)
{
	char_t *cp;

	a_assert(rq);
	a_assert(rq->buflen == (rq->endbuf - rq->buf));

	if ((ringqPutBlkMax(rq) < (int) sizeof(char_t)) && !ringqGrow(rq)) {
		return -1;
	}

	cp = (char_t*) rq->endp;
	*cp++ = (char_t) c;
	rq->endp = (unsigned char *) cp;
	if (rq->endp >= rq->endbuf) {
		rq->endp = rq->buf;
	}
	return 0;
}

/******************************************************************************/
/*
 *	Insert a wide character at the front of the queue
 */

int ringqInsertc(ringq_t *rq, char_t c)
{
	char_t *cp;

	a_assert(rq);
	a_assert(rq->buflen == (rq->endbuf - rq->buf));

	if (ringqPutBlkMax(rq) < (int) sizeof(char_t) && !ringqGrow(rq)) {
		return -1;
	}
	if (rq->servp <= rq->buf) {
		rq->servp = rq->endbuf;
	}
	cp = (char_t*) rq->servp;
	*--cp = (char_t) c;
	rq->servp = (unsigned char *) cp;
	return 0;
}

/******************************************************************************/
/*
 *	Add a string to the queue. Add a trailing null (maybe two nulls)
 */

int ringqPutStr(ringq_t *rq, char_t *str)
{
	int		rc;

	a_assert(rq);
	a_assert(str);
	a_assert(rq->buflen == (rq->endbuf - rq->buf));

	rc = ringqPutBlk(rq, (unsigned char*) str, gstrlen(str) * sizeof(char_t));
	*((char_t*) rq->endp) = (char_t) '\0';
	return rc;
}

/******************************************************************************/
/*
 *	Add a null terminator. This does NOT increase the size of the queue
 */

void ringqAddNull(ringq_t *rq)
{
	a_assert(rq);
	a_assert(rq->buflen == (rq->endbuf - rq->buf));

	*((char_t*) rq->endp) = (char_t) '\0';
}

/******************************************************************************/
#ifdef UNICODE
/*
 *	Get a byte from the queue
 */

int ringqGetcA(ringq_t *rq)
{
	unsigned char	c;

	a_assert(rq);
	a_assert(rq->buflen == (rq->endbuf - rq->buf));

	if (rq->servp == rq->endp) {
		return -1;
	}

	c = *rq->servp++;
	if (rq->servp >= rq->endbuf) {
		rq->servp = rq->buf;
	}
	return c;
}

/******************************************************************************/
/*
 *	Add a byte to the queue. Note if being used to store strings this does not
 *	add a trailing '\0'. Grow the q as required.
 */

int ringqPutcA(ringq_t *rq, char c)
{
	a_assert(rq);
	a_assert(rq->buflen == (rq->endbuf - rq->buf));

	if (ringqPutBlkMax(rq) == 0 && !ringqGrow(rq)) {
		return -1;
	}

	*rq->endp++ = (unsigned char) c;
	if (rq->endp >= rq->endbuf) {
		rq->endp = rq->buf;
	}
	return 0;
}

/******************************************************************************/
/*
 *	Insert a byte at the front of the queue
 */

int ringqInsertcA(ringq_t *rq, char c)
{
	a_assert(rq);
	a_assert(rq->buflen == (rq->endbuf - rq->buf));

	if (ringqPutBlkMax(rq) == 0 && !ringqGrow(rq)) {
		return -1;
	}
	if (rq->servp <= rq->buf) {
		rq->servp = rq->endbuf;
	}
	*--rq->servp = (unsigned char) c;
	return 0;
}

/******************************************************************************/
/*
 *	Add a string to the queue. Add a trailing null (not really in the q).
 *	ie. beyond the last valid byte.
 */

int ringqPutStrA(ringq_t *rq, char *str)
{
	int		rc;

	a_assert(rq);
	a_assert(str);
	a_assert(rq->buflen == (rq->endbuf - rq->buf));

	rc = ringqPutBlk(rq, (unsigned char*) str, strlen(str));
	rq->endp[0] = '\0';
	return rc;
}

#endif /* UNICODE */
/******************************************************************************/
/*
 *	Add a block of data to the ringq. Return the number of bytes added.
 *	Grow the q as required.
 */

int ringqPutBlk(ringq_t *rq, unsigned char *buf, int size)
{
	int		this, bytes_put;

	a_assert(rq);
	a_assert(rq->buflen == (rq->endbuf - rq->buf));
	a_assert(buf);
	a_assert(0 <= size);

/*
 *	Loop adding the maximum bytes we can add in a single straight line copy
 */
	bytes_put = 0;
	while (size > 0) {
		this = min(ringqPutBlkMax(rq), size);
		if (this <= 0) {
			if (! ringqGrow(rq)) {
				break;
			}
			this = min(ringqPutBlkMax(rq), size);
		}

		memcpy(rq->endp, buf, this);
		buf += this;
		rq->endp += this;
		size -= this;
		bytes_put += this;

		if (rq->endp >= rq->endbuf) {
			rq->endp = rq->buf;
		}
	}
	return bytes_put;
}

/******************************************************************************/
/*
 *	Get a block of data from the ringq. Return the number of bytes returned.
 */

int ringqGetBlk(ringq_t *rq, unsigned char *buf, int size)
{
	int		this, bytes_read;

	a_assert(rq);
	a_assert(rq->buflen == (rq->endbuf - rq->buf));
	a_assert(buf);
	a_assert(0 <= size && size < rq->buflen);

/*
 *	Loop getting the maximum bytes we can get in a single straight line copy
 */
	bytes_read = 0;
	while (size > 0) {
		this = ringqGetBlkMax(rq);
		this = min(this, size);
		if (this <= 0) {
			break;
		}

		memcpy(buf, rq->servp, this);
		buf += this;
		rq->servp += this;
		size -= this;
		bytes_read += this;

		if (rq->servp >= rq->endbuf) {
			rq->servp = rq->buf;
		}
	}
	return bytes_read;
}

/******************************************************************************/
/*
 *	Return the maximum number of bytes the ring q can accept via a single 
 *	block copy. Useful if the user is doing their own data insertion.
 */

int ringqPutBlkMax(ringq_t *rq)
{
	int		space, in_a_line;

	a_assert(rq);
	a_assert(rq->buflen == (rq->endbuf - rq->buf));
	
	space = rq->buflen - RINGQ_LEN(rq) - 1;
	in_a_line = rq->endbuf - rq->endp;

	return min(in_a_line, space);
}

/******************************************************************************/
/*
 *	Return the maximum number of bytes the ring q can provide via a single 
 *	block copy. Useful if the user is doing their own data retrieval.
 */

int ringqGetBlkMax(ringq_t *rq)
{
	int		len, in_a_line;

	a_assert(rq);
	a_assert(rq->buflen == (rq->endbuf - rq->buf));

	len = RINGQ_LEN(rq);
	in_a_line = rq->endbuf - rq->servp;

	return min(in_a_line, len);
}

/******************************************************************************/
/*
 *	Adjust the endp pointer after the user has copied data into the queue.
 */

void ringqPutBlkAdj(ringq_t *rq, int size)
{
	a_assert(rq);
	a_assert(rq->buflen == (rq->endbuf - rq->buf));
	a_assert(0 <= size && size < rq->buflen);

	rq->endp += size;
	if (rq->endp >= rq->endbuf) {
		rq->endp -= rq->buflen;
	}
/*
 *	Flush the queue if the endp pointer is corrupted via a bad size
 */
	if (rq->endp >= rq->endbuf) {
		error(E_L, E_LOG, T("Bad end pointer"));
		ringqFlush(rq);
	}
}

/******************************************************************************/
/*
 *	Adjust the servp pointer after the user has copied data from the queue.
 */

void ringqGetBlkAdj(ringq_t *rq, int size)
{
	a_assert(rq);
	a_assert(rq->buflen == (rq->endbuf - rq->buf));
	a_assert(0 < size && size < rq->buflen);

	rq->servp += size;
	if (rq->servp >= rq->endbuf) {
		rq->servp -= rq->buflen;
	}
/*
 *	Flush the queue if the servp pointer is corrupted via a bad size
 */
	if (rq->servp >= rq->endbuf) {
		error(E_L, E_LOG, T("Bad serv pointer"));
		ringqFlush(rq);
	}
}

/******************************************************************************/
/*
 *	Flush all data in a ring q.  Reset the pointers.
 */

void ringqFlush(ringq_t *rq)
{
	a_assert(rq);
	a_assert(rq->servp);

	rq->servp = rq->buf;
	rq->endp = rq->buf;
	if (rq->servp) {
		*rq->servp = '\0';
	}
}

/******************************************************************************/
/*
 *	Grow the buffer. Return true if the buffer can be grown. Grow using
 *	the increment size specified when opening the ringq. Don't grow beyond
 *	the maximum possible size.
 */

static int ringqGrow(ringq_t *rq)
{
	unsigned char	*newbuf;
	int 			len;

	a_assert(rq);

	if (rq->maxsize >= 0 && rq->buflen >= rq->maxsize) {
		return 0;
	}

	len = ringqLen(rq);

	if ((newbuf = balloc(B_L, rq->buflen + rq->increment)) == NULL) {
		return 0;
	}
	ringqGetBlk(rq, newbuf, ringqLen(rq));
	bfree(B_L, (char*) rq->buf);

#ifdef OLD
	rq->endp = &newbuf[endp];
	rq->servp = &newbuf[servp];
	rq->endbuf = &newbuf[rq->buflen];
	rq->buf = newbuf;
#endif

	rq->buflen += rq->increment;
	rq->endp = newbuf;
	rq->servp = newbuf;
	rq->buf = newbuf;
	rq->endbuf = &rq->buf[rq->buflen];

	ringqPutBlk(rq, newbuf, len);

/*
 *	Double the increment so the next grow will line up with balloc'ed memory
 */
	rq->increment = getBinBlockSize(2 * rq->increment);

	return 1;
}

/******************************************************************************/
/*
 *	Find the smallest binary memory size that "size" will fit into.  This
 *	makes the ringq and ringqGrow routines much more efficient.  The balloc
 *	routine likes powers of 2 minus 1.
 */

static int	getBinBlockSize(int size)
{
	int	q;

	size = size >> B_SHIFT;
	for (q = 0; size; size >>= 1) {
		q++;
	}
	return (1 << (B_SHIFT + q));
}

/******************************************************************************/

