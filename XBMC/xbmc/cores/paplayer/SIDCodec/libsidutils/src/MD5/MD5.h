/*
 * This code has been derived by Michael Schwendt <mschwendt@yahoo.com>
 * from original work by L. Peter Deutsch <ghost@aladdin.com>.
 * 
 * The original C code (md5.c, md5.h) is available here:
 * ftp://ftp.cs.wisc.edu/ghost/packages/md5.tar.gz
 */

/*
  Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  L. Peter Deutsch
  ghost@aladdin.com
 */

#ifndef MD5_H
#define MD5_H

#include "MD5_Defs.h"

typedef unsigned char md5_byte_t;  // 8-bit byte
typedef unsigned int md5_word_t;   // 32-bit word

class MD5
{
 public:
    // Initialize the algorithm. Reset starting values.
    MD5();
    
    // Append a string to the message.
    void append(const void* data, int nbytes);

    // Finish the message.
    void finish();

    // Return pointer to 16-byte fingerprint.
    const md5_byte_t* getDigest();
    
    // Initialize the algorithm. Reset starting values.
    void reset();

 private:

    /* Define the state of the MD5 Algorithm. */
    md5_word_t count[2];	/* message length in bits, lsw first */
    md5_word_t abcd[4];		/* digest buffer */
    md5_byte_t buf[64];		/* accumulate block */

    md5_byte_t digest[16];
        
    md5_word_t tmpBuf[16];
    const md5_word_t* X;
    
    void
    process(const md5_byte_t data[64]);

    md5_word_t
    ROTATE_LEFT(const md5_word_t x, const int n);
    
    md5_word_t
    F(const md5_word_t x, const md5_word_t y, const md5_word_t z);
        
    md5_word_t
    G(const md5_word_t x, const md5_word_t y, const md5_word_t z);

    md5_word_t
    H(const md5_word_t x, const md5_word_t y, const md5_word_t z);

    md5_word_t
    I(const md5_word_t x, const md5_word_t y, const md5_word_t z);

    typedef md5_word_t (MD5::*md5func)(const md5_word_t x, const md5_word_t y, const md5_word_t z);
        
    void
    SET(md5func func, md5_word_t& a, md5_word_t& b, md5_word_t& c,
        md5_word_t& d, const int k, const int s,
        const md5_word_t Ti);
};

inline md5_word_t
MD5::ROTATE_LEFT(const md5_word_t x, const int n)
{
    return ( (x<<n) | (x>>(32-n)) );
}

inline md5_word_t
MD5::F(const md5_word_t x, const md5_word_t y, const md5_word_t z)
{
    return ( (x&y) | (~x&z) );
}

inline md5_word_t
MD5::G(const md5_word_t x, const md5_word_t y, const md5_word_t z)
{
    return ( (x&z) | (y&~z) );
}

inline md5_word_t
MD5::H(const md5_word_t x, const md5_word_t y, const md5_word_t z)
{
    return ( x^y^z );
}

inline md5_word_t
MD5::I(const md5_word_t x, const md5_word_t y, const md5_word_t z)
{
    return ( y ^ (x|~z) );
}

inline void
MD5::SET(md5func func, md5_word_t& a, md5_word_t& b, md5_word_t& c,
          md5_word_t& d, const int k, const int s,
          const md5_word_t Ti)
{
    md5_word_t t = a + (this->*func)(b,c,d) + X[k] + Ti;
    a = ROTATE_LEFT(t, s) + b;
}

#endif  /* MD5_H */
