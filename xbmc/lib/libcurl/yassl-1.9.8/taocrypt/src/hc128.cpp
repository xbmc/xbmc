/* hc128.cpp                                
 *
 * Copyright (C) 2003 Sawtooth Consulting Ltd.
 *
 * This file is part of yaSSL, an SSL implementation written by Todd A Ouska
 * (todd at yassl.com, see www.yassl.com).
 *
 * yaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * There are special exceptions to the terms and conditions of the GPL as it
 * is applied to yaSSL. View the full text of the exception in the file
 * FLOSS-EXCEPTIONS in the directory of this software distribution.
 *
 * yaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */


#include "runtime.hpp"
#include "hc128.hpp"



namespace TaoCrypt {




#ifdef BIG_ENDIAN_ORDER
    #define LITTLE32(x) ByteReverse((word32)x)
#else
    #define LITTLE32(x) (x)
#endif


/*h1 function*/
#define h1(x, y) {                              \
     byte a,c;                                  \
     a = (byte) (x);                            \
     c = (byte) ((x) >> 16);                    \
     y = (T_[512+a])+(T_[512+256+c]);           \
}

/*h2 function*/
#define h2(x, y) {                              \
     byte a,c;                                  \
     a = (byte) (x);                            \
     c = (byte) ((x) >> 16);                    \
     y = (T_[a])+(T_[256+c]);                   \
}

/*one step of HC-128, update P and generate 32 bits keystream*/
#define step_P(u,v,a,b,c,d,n){                  \
     word32 tem0,tem1,tem2,tem3;                \
     h1((X_[(d)]),tem3);                        \
     tem0 = rotrFixed((T_[(v)]),23);            \
     tem1 = rotrFixed((X_[(c)]),10);            \
     tem2 = rotrFixed((X_[(b)]),8);             \
     (T_[(u)]) += tem2+(tem0 ^ tem1);           \
     (X_[(a)]) = (T_[(u)]);                     \
     (n) = tem3 ^ (T_[(u)]) ;                   \
}       

/*one step of HC-128, update Q and generate 32 bits keystream*/
#define step_Q(u,v,a,b,c,d,n){                  \
     word32 tem0,tem1,tem2,tem3;                \
     h2((Y_[(d)]),tem3);                        \
     tem0 = rotrFixed((T_[(v)]),(32-23));       \
     tem1 = rotrFixed((Y_[(c)]),(32-10));       \
     tem2 = rotrFixed((Y_[(b)]),(32-8));        \
     (T_[(u)]) += tem2 + (tem0 ^ tem1);         \
     (Y_[(a)]) = (T_[(u)]);                     \
     (n) = tem3 ^ (T_[(u)]) ;                   \
}   


/*16 steps of HC-128, generate 512 bits keystream*/
void HC128::GenerateKeystream(word32* keystream)  
{
   word32 cc,dd;
   cc = counter1024_ & 0x1ff;
   dd = (cc+16)&0x1ff;

   if (counter1024_ < 512)	
   {   		
      counter1024_ = (counter1024_ + 16) & 0x3ff;
      step_P(cc+0, cc+1, 0, 6, 13,4, keystream[0]);
      step_P(cc+1, cc+2, 1, 7, 14,5, keystream[1]);
      step_P(cc+2, cc+3, 2, 8, 15,6, keystream[2]);
      step_P(cc+3, cc+4, 3, 9, 0, 7, keystream[3]);
      step_P(cc+4, cc+5, 4, 10,1, 8, keystream[4]);
      step_P(cc+5, cc+6, 5, 11,2, 9, keystream[5]);
      step_P(cc+6, cc+7, 6, 12,3, 10,keystream[6]);
      step_P(cc+7, cc+8, 7, 13,4, 11,keystream[7]);
      step_P(cc+8, cc+9, 8, 14,5, 12,keystream[8]);
      step_P(cc+9, cc+10,9, 15,6, 13,keystream[9]);
      step_P(cc+10,cc+11,10,0, 7, 14,keystream[10]);
      step_P(cc+11,cc+12,11,1, 8, 15,keystream[11]);
      step_P(cc+12,cc+13,12,2, 9, 0, keystream[12]);
      step_P(cc+13,cc+14,13,3, 10,1, keystream[13]);
      step_P(cc+14,cc+15,14,4, 11,2, keystream[14]);
      step_P(cc+15,dd+0, 15,5, 12,3, keystream[15]);
   }
   else				    
   {
	  counter1024_ = (counter1024_ + 16) & 0x3ff;
      step_Q(512+cc+0, 512+cc+1, 0, 6, 13,4, keystream[0]);
      step_Q(512+cc+1, 512+cc+2, 1, 7, 14,5, keystream[1]);
      step_Q(512+cc+2, 512+cc+3, 2, 8, 15,6, keystream[2]);
      step_Q(512+cc+3, 512+cc+4, 3, 9, 0, 7, keystream[3]);
      step_Q(512+cc+4, 512+cc+5, 4, 10,1, 8, keystream[4]);
      step_Q(512+cc+5, 512+cc+6, 5, 11,2, 9, keystream[5]);
      step_Q(512+cc+6, 512+cc+7, 6, 12,3, 10,keystream[6]);
      step_Q(512+cc+7, 512+cc+8, 7, 13,4, 11,keystream[7]);
      step_Q(512+cc+8, 512+cc+9, 8, 14,5, 12,keystream[8]);
      step_Q(512+cc+9, 512+cc+10,9, 15,6, 13,keystream[9]);
      step_Q(512+cc+10,512+cc+11,10,0, 7, 14,keystream[10]);
      step_Q(512+cc+11,512+cc+12,11,1, 8, 15,keystream[11]);
      step_Q(512+cc+12,512+cc+13,12,2, 9, 0, keystream[12]);
      step_Q(512+cc+13,512+cc+14,13,3, 10,1, keystream[13]);
      step_Q(512+cc+14,512+cc+15,14,4, 11,2, keystream[14]);
      step_Q(512+cc+15,512+dd+0, 15,5, 12,3, keystream[15]);
   }
}


/* The following defines the initialization functions */
#define f1(x)  (rotrFixed((x),7)  ^ rotrFixed((x),18) ^ ((x) >> 3))
#define f2(x)  (rotrFixed((x),17) ^ rotrFixed((x),19) ^ ((x) >> 10))

/*update table P*/
#define update_P(u,v,a,b,c,d){                      \
     word32 tem0,tem1,tem2,tem3;                    \
     tem0 = rotrFixed((T_[(v)]),23);                \
     tem1 = rotrFixed((X_[(c)]),10);                \
     tem2 = rotrFixed((X_[(b)]),8);                 \
     h1((X_[(d)]),tem3);                            \
     (T_[(u)]) = ((T_[(u)]) + tem2+(tem0^tem1)) ^ tem3;     \
     (X_[(a)]) = (T_[(u)]);                         \
}  

/*update table Q*/
#define update_Q(u,v,a,b,c,d){                      \
     word32 tem0,tem1,tem2,tem3;                    \
     tem0 = rotrFixed((T_[(v)]),(32-23));           \
     tem1 = rotrFixed((Y_[(c)]),(32-10));           \
     tem2 = rotrFixed((Y_[(b)]),(32-8));            \
     h2((Y_[(d)]),tem3);                            \
     (T_[(u)]) = ((T_[(u)]) + tem2+(tem0^tem1)) ^ tem3;     \
     (Y_[(a)]) = (T_[(u)]);                         \
}     

/*16 steps of HC-128, without generating keystream, */
/*but use the outputs to update P and Q*/
void HC128::SetupUpdate()  /*each time 16 steps*/
{
   word32 cc,dd;
   cc = counter1024_ & 0x1ff;
   dd = (cc+16)&0x1ff;

   if (counter1024_ < 512)	
   {   		
      counter1024_ = (counter1024_ + 16) & 0x3ff;
      update_P(cc+0, cc+1, 0, 6, 13, 4);
      update_P(cc+1, cc+2, 1, 7, 14, 5);
      update_P(cc+2, cc+3, 2, 8, 15, 6);
      update_P(cc+3, cc+4, 3, 9, 0,  7);
      update_P(cc+4, cc+5, 4, 10,1,  8);
      update_P(cc+5, cc+6, 5, 11,2,  9);
      update_P(cc+6, cc+7, 6, 12,3,  10);
      update_P(cc+7, cc+8, 7, 13,4,  11);
      update_P(cc+8, cc+9, 8, 14,5,  12);
      update_P(cc+9, cc+10,9, 15,6,  13);
      update_P(cc+10,cc+11,10,0, 7,  14);
      update_P(cc+11,cc+12,11,1, 8,  15);
      update_P(cc+12,cc+13,12,2, 9,  0);
      update_P(cc+13,cc+14,13,3, 10, 1);
      update_P(cc+14,cc+15,14,4, 11, 2);
      update_P(cc+15,dd+0, 15,5, 12, 3);   
   }
   else				    
   {
      counter1024_ = (counter1024_ + 16) & 0x3ff;
      update_Q(512+cc+0, 512+cc+1, 0, 6, 13, 4);
      update_Q(512+cc+1, 512+cc+2, 1, 7, 14, 5);
      update_Q(512+cc+2, 512+cc+3, 2, 8, 15, 6);
      update_Q(512+cc+3, 512+cc+4, 3, 9, 0,  7);
      update_Q(512+cc+4, 512+cc+5, 4, 10,1,  8);
      update_Q(512+cc+5, 512+cc+6, 5, 11,2,  9);
      update_Q(512+cc+6, 512+cc+7, 6, 12,3,  10);
      update_Q(512+cc+7, 512+cc+8, 7, 13,4,  11);
      update_Q(512+cc+8, 512+cc+9, 8, 14,5,  12);
      update_Q(512+cc+9, 512+cc+10,9, 15,6,  13);
      update_Q(512+cc+10,512+cc+11,10,0, 7,  14);
      update_Q(512+cc+11,512+cc+12,11,1, 8,  15);
      update_Q(512+cc+12,512+cc+13,12,2, 9,  0);
      update_Q(512+cc+13,512+cc+14,13,3, 10, 1);
      update_Q(512+cc+14,512+cc+15,14,4, 11, 2);
      update_Q(512+cc+15,512+dd+0, 15,5, 12, 3); 
   }       
}


/* for the 128-bit key:  key[0]...key[15]
*  key[0] is the least significant byte of ctx->key[0] (K_0);
*  key[3] is the most significant byte of ctx->key[0]  (K_0);
*  ...
*  key[12] is the least significant byte of ctx->key[3] (K_3)
*  key[15] is the most significant byte of ctx->key[3]  (K_3)
*
*  for the 128-bit iv:  iv[0]...iv[15]
*  iv[0] is the least significant byte of ctx->iv[0] (IV_0);
*  iv[3] is the most significant byte of ctx->iv[0]  (IV_0);
*  ...
*  iv[12] is the least significant byte of ctx->iv[3] (IV_3)
*  iv[15] is the most significant byte of ctx->iv[3]  (IV_3)
*/



void HC128::SetIV(const byte* iv)
{ 
    word32 i;
	
	for (i = 0; i < (128 >> 5); i++)
        iv_[i] = LITTLE32(((word32*)iv)[i]);
	
    for (; i < 8; i++) iv_[i] = iv_[i-4];
  
    /* expand the key and IV into the table T */ 
    /* (expand the key and IV into the table P and Q) */ 
	
	for (i = 0; i < 8;  i++)   T_[i] = key_[i];
	for (i = 8; i < 16; i++)   T_[i] = iv_[i-8];

    for (i = 16; i < (256+16); i++) 
		T_[i] = f2(T_[i-2]) + T_[i-7] + f1(T_[i-15]) + T_[i-16]+i;
    
	for (i = 0; i < 16;  i++)  T_[i] = T_[256+i];

	for (i = 16; i < 1024; i++) 
		T_[i] = f2(T_[i-2]) + T_[i-7] + f1(T_[i-15]) + T_[i-16]+256+i;
    
    /* initialize counter1024, X and Y */
	counter1024_ = 0;
	for (i = 0; i < 16; i++) X_[i] = T_[512-16+i];
    for (i = 0; i < 16; i++) Y_[i] = T_[512+512-16+i];
    
    /* run the cipher 1024 steps before generating the output */
	for (i = 0; i < 64; i++)  SetupUpdate();  
}


void HC128::SetKey(const byte* key, const byte* iv)
{ 
  word32 i;  

  /* Key size in bits 128 */ 
  for (i = 0; i < (128 >> 5); i++)
      key_[i] = LITTLE32(((word32*)key)[i]);
 
  for ( ; i < 8 ; i++) key_[i] = key_[i-4];

  SetIV(iv);
}


/* The following defines the encryption of data stream */
void HC128::Process(byte* output, const byte* input, word32 msglen)
{
  word32 i, keystream[16];

  for ( ; msglen >= 64; msglen -= 64, input += 64, output += 64)
  {
	  GenerateKeystream(keystream);

      /* unroll loop */
	  ((word32*)output)[0]  = ((word32*)input)[0]  ^ LITTLE32(keystream[0]);
	  ((word32*)output)[1]  = ((word32*)input)[1]  ^ LITTLE32(keystream[1]);
	  ((word32*)output)[2]  = ((word32*)input)[2]  ^ LITTLE32(keystream[2]);
	  ((word32*)output)[3]  = ((word32*)input)[3]  ^ LITTLE32(keystream[3]);
	  ((word32*)output)[4]  = ((word32*)input)[4]  ^ LITTLE32(keystream[4]);
	  ((word32*)output)[5]  = ((word32*)input)[5]  ^ LITTLE32(keystream[5]);
	  ((word32*)output)[6]  = ((word32*)input)[6]  ^ LITTLE32(keystream[6]);
	  ((word32*)output)[7]  = ((word32*)input)[7]  ^ LITTLE32(keystream[7]);
	  ((word32*)output)[8]  = ((word32*)input)[8]  ^ LITTLE32(keystream[8]);
	  ((word32*)output)[9]  = ((word32*)input)[9]  ^ LITTLE32(keystream[9]);
	  ((word32*)output)[10] = ((word32*)input)[10] ^ LITTLE32(keystream[10]);
	  ((word32*)output)[11] = ((word32*)input)[11] ^ LITTLE32(keystream[11]);
	  ((word32*)output)[12] = ((word32*)input)[12] ^ LITTLE32(keystream[12]);
	  ((word32*)output)[13] = ((word32*)input)[13] ^ LITTLE32(keystream[13]);
	  ((word32*)output)[14] = ((word32*)input)[14] ^ LITTLE32(keystream[14]);
	  ((word32*)output)[15] = ((word32*)input)[15] ^ LITTLE32(keystream[15]);
  }

  if (msglen > 0)
  {
      GenerateKeystream(keystream);

#ifdef BIG_ENDIAN_ORDER
      {
          word32 wordsLeft = msglen / sizeof(word32);
          if (msglen % sizeof(word32)) wordsLeft++;
          
          ByteReverse(keystream, keystream, wordsLeft * sizeof(word32));
      }
#endif

      for (i = 0; i < msglen; i++)
	      output[i] = input[i] ^ ((byte*)keystream)[i];
  }

}


}  // namespace
