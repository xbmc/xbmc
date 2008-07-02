/*
** 2008 February 16
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This file implements an object that represents a fixed-length
** bitmap.  Bits are numbered starting with 1.
**
** A bitmap is used to record what pages a database file have been
** journalled during a transaction.  Usually only a few pages are
** journalled.  So the bitmap is usually sparse and has low cardinality.
** But sometimes (for example when during a DROP of a large table) most
** or all of the pages get journalled.  In those cases, the bitmap becomes
** dense.  The algorithm needs to handle both cases well.
**
** The size of the bitmap is fixed when the object is created.
**
** All bits are clear when the bitmap is created.  Individual bits
** may be set or cleared one at a time.
**
** Test operations are about 100 times more common that set operations.
** Clear operations are exceedingly rare.  There are usually between
** 5 and 500 set operations per Bitvec object, though the number of sets can
** sometimes grow into tens of thousands or larger.  The size of the
** Bitvec object is the number of pages in the database file at the
** start of a transaction, and is thus usually less than a few thousand,
** but can be as large as 2 billion for a really big database.
**
** @(#) $Id: bitvec.c,v 1.5 2008/05/13 13:27:34 drh Exp $
*/
#include "sqliteInt.h"

#define BITVEC_SZ        512
/* Round the union size down to the nearest pointer boundary, since that's how 
** it will be aligned within the Bitvec struct. */
#define BITVEC_USIZE     (((BITVEC_SZ-12)/sizeof(Bitvec*))*sizeof(Bitvec*))
#define BITVEC_NCHAR     BITVEC_USIZE
#define BITVEC_NBIT      (BITVEC_NCHAR*8)
#define BITVEC_NINT      (BITVEC_USIZE/4)
#define BITVEC_MXHASH    (BITVEC_NINT/2)
#define BITVEC_NPTR      (BITVEC_USIZE/sizeof(Bitvec *))

#define BITVEC_HASH(X)   (((X)*37)%BITVEC_NINT)

/*
** A bitmap is an instance of the following structure.
**
** This bitmap records the existance of zero or more bits
** with values between 1 and iSize, inclusive.
**
** There are three possible representations of the bitmap.
** If iSize<=BITVEC_NBIT, then Bitvec.u.aBitmap[] is a straight
** bitmap.  The least significant bit is bit 1.
**
** If iSize>BITVEC_NBIT and iDivisor==0 then Bitvec.u.aHash[] is
** a hash table that will hold up to BITVEC_MXHASH distinct values.
**
** Otherwise, the value i is redirected into one of BITVEC_NPTR
** sub-bitmaps pointed to by Bitvec.u.apSub[].  Each subbitmap
** handles up to iDivisor separate values of i.  apSub[0] holds
** values between 1 and iDivisor.  apSub[1] holds values between
** iDivisor+1 and 2*iDivisor.  apSub[N] holds values between
** N*iDivisor+1 and (N+1)*iDivisor.  Each subbitmap is normalized
** to hold deal with values between 1 and iDivisor.
*/
struct Bitvec {
  u32 iSize;      /* Maximum bit index */
  u32 nSet;       /* Number of bits that are set */
  u32 iDivisor;   /* Number of bits handled by each apSub[] entry */
  union {
    u8 aBitmap[BITVEC_NCHAR];    /* Bitmap representation */
    u32 aHash[BITVEC_NINT];      /* Hash table representation */
    Bitvec *apSub[BITVEC_NPTR];  /* Recursive representation */
  } u;
};

/*
** Create a new bitmap object able to handle bits between 0 and iSize,
** inclusive.  Return a pointer to the new object.  Return NULL if 
** malloc fails.
*/
Bitvec *sqlite3BitvecCreate(u32 iSize){
  Bitvec *p;
  assert( sizeof(*p)==BITVEC_SZ );
  p = sqlite3MallocZero( sizeof(*p) );
  if( p ){
    p->iSize = iSize;
  }
  return p;
}

/*
** Check to see if the i-th bit is set.  Return true or false.
** If p is NULL (if the bitmap has not been created) or if
** i is out of range, then return false.
*/
int sqlite3BitvecTest(Bitvec *p, u32 i){
  if( p==0 ) return 0;
  if( i>p->iSize || i==0 ) return 0;
  if( p->iSize<=BITVEC_NBIT ){
    i--;
    return (p->u.aBitmap[i/8] & (1<<(i&7)))!=0;
  }
  if( p->iDivisor>0 ){
    u32 bin = (i-1)/p->iDivisor;
    i = (i-1)%p->iDivisor + 1;
    return sqlite3BitvecTest(p->u.apSub[bin], i);
  }else{
    u32 h = BITVEC_HASH(i);
    while( p->u.aHash[h] ){
      if( p->u.aHash[h]==i ) return 1;
      h++;
      if( h>=BITVEC_NINT ) h = 0;
    }
    return 0;
  }
}

/*
** Set the i-th bit.  Return 0 on success and an error code if
** anything goes wrong.
*/
int sqlite3BitvecSet(Bitvec *p, u32 i){
  u32 h;
  assert( p!=0 );
  assert( i>0 );
  assert( i<=p->iSize );
  if( p->iSize<=BITVEC_NBIT ){
    i--;
    p->u.aBitmap[i/8] |= 1 << (i&7);
    return SQLITE_OK;
  }
  if( p->iDivisor ){
    u32 bin = (i-1)/p->iDivisor;
    i = (i-1)%p->iDivisor + 1;
    if( p->u.apSub[bin]==0 ){
      sqlite3FaultBeginBenign(SQLITE_FAULTINJECTOR_MALLOC);
      p->u.apSub[bin] = sqlite3BitvecCreate( p->iDivisor );
      sqlite3FaultEndBenign(SQLITE_FAULTINJECTOR_MALLOC);
      if( p->u.apSub[bin]==0 ) return SQLITE_NOMEM;
    }
    return sqlite3BitvecSet(p->u.apSub[bin], i);
  }
  h = BITVEC_HASH(i);
  while( p->u.aHash[h] ){
    if( p->u.aHash[h]==i ) return SQLITE_OK;
    h++;
    if( h==BITVEC_NINT ) h = 0;
  }
  p->nSet++;
  if( p->nSet>=BITVEC_MXHASH ){
    int j, rc;
    u32 aiValues[BITVEC_NINT];
    memcpy(aiValues, p->u.aHash, sizeof(aiValues));
    memset(p->u.apSub, 0, sizeof(p->u.apSub[0])*BITVEC_NPTR);
    p->iDivisor = (p->iSize + BITVEC_NPTR - 1)/BITVEC_NPTR;
    rc = sqlite3BitvecSet(p, i);
    for(j=0; j<BITVEC_NINT; j++){
      if( aiValues[j] ) rc |= sqlite3BitvecSet(p, aiValues[j]);
    }
    return rc;
  }
  p->u.aHash[h] = i;
  return SQLITE_OK;
}

/*
** Clear the i-th bit.  Return 0 on success and an error code if
** anything goes wrong.
*/
void sqlite3BitvecClear(Bitvec *p, u32 i){
  assert( p!=0 );
  assert( i>0 );
  if( p->iSize<=BITVEC_NBIT ){
    i--;
    p->u.aBitmap[i/8] &= ~(1 << (i&7));
  }else if( p->iDivisor ){
    u32 bin = (i-1)/p->iDivisor;
    i = (i-1)%p->iDivisor + 1;
    if( p->u.apSub[bin] ){
      sqlite3BitvecClear(p->u.apSub[bin], i);
    }
  }else{
    int j;
    u32 aiValues[BITVEC_NINT];
    memcpy(aiValues, p->u.aHash, sizeof(aiValues));
    memset(p->u.aHash, 0, sizeof(p->u.aHash[0])*BITVEC_NINT);
    p->nSet = 0;
    for(j=0; j<BITVEC_NINT; j++){
      if( aiValues[j] && aiValues[j]!=i ){
        sqlite3BitvecSet(p, aiValues[j]);
      }
    }
  }
}

/*
** Destroy a bitmap object.  Reclaim all memory used.
*/
void sqlite3BitvecDestroy(Bitvec *p){
  if( p==0 ) return;
  if( p->iDivisor ){
    int i;
    for(i=0; i<BITVEC_NPTR; i++){
      sqlite3BitvecDestroy(p->u.apSub[i]);
    }
  }
  sqlite3_free(p);
}

#ifndef SQLITE_OMIT_BUILTIN_TEST
/*
** Let V[] be an array of unsigned characters sufficient to hold
** up to N bits.  Let I be an integer between 0 and N.  0<=I<N.
** Then the following macros can be used to set, clear, or test
** individual bits within V.
*/
#define SETBIT(V,I)      V[I>>3] |= (1<<(I&7))
#define CLEARBIT(V,I)    V[I>>3] &= ~(1<<(I&7))
#define TESTBIT(V,I)     (V[I>>3]&(1<<(I&7)))!=0

/*
** This routine runs an extensive test of the Bitvec code.
**
** The input is an array of integers that acts as a program
** to test the Bitvec.  The integers are opcodes followed
** by 0, 1, or 3 operands, depending on the opcode.  Another
** opcode follows immediately after the last operand.
**
** There are 6 opcodes numbered from 0 through 5.  0 is the
** "halt" opcode and causes the test to end.
**
**    0          Halt and return the number of errors
**    1 N S X    Set N bits beginning with S and incrementing by X
**    2 N S X    Clear N bits beginning with S and incrementing by X
**    3 N        Set N randomly chosen bits
**    4 N        Clear N randomly chosen bits
**    5 N S X    Set N bits from S increment X in array only, not in bitvec
**
** The opcodes 1 through 4 perform set and clear operations are performed
** on both a Bitvec object and on a linear array of bits obtained from malloc.
** Opcode 5 works on the linear array only, not on the Bitvec.
** Opcode 5 is used to deliberately induce a fault in order to
** confirm that error detection works.
**
** At the conclusion of the test the linear array is compared
** against the Bitvec object.  If there are any differences,
** an error is returned.  If they are the same, zero is returned.
**
** If a memory allocation error occurs, return -1.
*/
int sqlite3BitvecBuiltinTest(int sz, int *aOp){
  Bitvec *pBitvec = 0;
  unsigned char *pV = 0;
  int rc = -1;
  int i, nx, pc, op;

  /* Allocate the Bitvec to be tested and a linear array of
  ** bits to act as the reference */
  pBitvec = sqlite3BitvecCreate( sz );
  pV = sqlite3_malloc( (sz+7)/8 + 1 );
  if( pBitvec==0 || pV==0 ) goto bitvec_end;
  memset(pV, 0, (sz+7)/8 + 1);

  /* Run the program */
  pc = 0;
  while( (op = aOp[pc])!=0 ){
    switch( op ){
      case 1:
      case 2:
      case 5: {
        nx = 4;
        i = aOp[pc+2] - 1;
        aOp[pc+2] += aOp[pc+3];
        break;
      }
      case 3:
      case 4: 
      default: {
        nx = 2;
        sqlite3_randomness(sizeof(i), &i);
        break;
      }
    }
    if( (--aOp[pc+1]) > 0 ) nx = 0;
    pc += nx;
    i = (i & 0x7fffffff)%sz;
    if( (op & 1)!=0 ){
      SETBIT(pV, (i+1));
      if( op!=5 ){
        if( sqlite3BitvecSet(pBitvec, i+1) ) goto bitvec_end;
      }
    }else{
      CLEARBIT(pV, (i+1));
      sqlite3BitvecClear(pBitvec, i+1);
    }
  }

  /* Test to make sure the linear array exactly matches the
  ** Bitvec object.  Start with the assumption that they do
  ** match (rc==0).  Change rc to non-zero if a discrepancy
  ** is found.
  */
  rc = sqlite3BitvecTest(0,0) + sqlite3BitvecTest(pBitvec, sz+1)
          + sqlite3BitvecTest(pBitvec, 0);
  for(i=1; i<=sz; i++){
    if(  (TESTBIT(pV,i))!=sqlite3BitvecTest(pBitvec,i) ){
      rc = i;
      break;
    }
  }

  /* Free allocated structure */
bitvec_end:
  sqlite3_free(pV);
  sqlite3BitvecDestroy(pBitvec);
  return rc;
}
#endif /* SQLITE_OMIT_BUILTIN_TEST */
