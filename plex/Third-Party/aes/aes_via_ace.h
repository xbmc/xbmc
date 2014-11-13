/*
Copyright (c) 1998-2013, Brian Gladman, Worcester, UK. All rights reserved.

The redistribution and use of this software (with or without changes)
is allowed without the payment of fees or royalties provided that:

  source code distributions include the above copyright notice, this
  list of conditions and the following disclaimer;

  binary distributions include the above copyright notice, this list
  of conditions and the following disclaimer in their documentation.

This software is provided 'as is' with no explicit or implied warranties
in respect of its operation, including, but not limited to, correctness
and fitness for purpose.
---------------------------------------------------------------------------
Issue Date: 20/12/2007
*/

#ifndef AES_VIA_ACE_H
#define AES_VIA_ACE_H

#if defined( _MSC_VER )
#  define INLINE  __inline
#elif defined( __GNUC__ )
#  define INLINE  static inline
#else
#  error VIA ACE requires Microsoft or GNU C
#endif

#define NEH_GENERATE    1
#define NEH_LOAD        2
#define NEH_HYBRID      3

#define MAX_READ_ATTEMPTS   1000

/* VIA Nehemiah RNG and ACE Feature Mask Values */

#define NEH_CPU_IS_VIA      0x00000001
#define NEH_CPU_READ        0x00000010
#define NEH_CPU_MASK        0x00000011

#define NEH_RNG_PRESENT     0x00000004
#define NEH_RNG_ENABLED     0x00000008
#define NEH_ACE_PRESENT     0x00000040
#define NEH_ACE_ENABLED     0x00000080
#define NEH_RNG_FLAGS       (NEH_RNG_PRESENT | NEH_RNG_ENABLED)
#define NEH_ACE_FLAGS       (NEH_ACE_PRESENT | NEH_ACE_ENABLED)
#define NEH_FLAGS_MASK      (NEH_RNG_FLAGS | NEH_ACE_FLAGS)

/* VIA Nehemiah Advanced Cryptography Engine (ACE) Control Word Values  */

#define NEH_GEN_KEY     0x00000000      /* generate key schedule        */
#define NEH_LOAD_KEY    0x00000080      /* load schedule from memory    */
#define NEH_ENCRYPT     0x00000000      /* encryption                   */
#define NEH_DECRYPT     0x00000200      /* decryption                   */
#define NEH_KEY128      0x00000000+0x0a /* 128 bit key                  */
#define NEH_KEY192      0x00000400+0x0c /* 192 bit key                  */
#define NEH_KEY256      0x00000800+0x0e /* 256 bit key                  */

#define NEH_ENC_GEN     (NEH_ENCRYPT | NEH_GEN_KEY)
#define NEH_DEC_GEN     (NEH_DECRYPT | NEH_GEN_KEY)
#define NEH_ENC_LOAD    (NEH_ENCRYPT | NEH_LOAD_KEY)
#define NEH_DEC_LOAD    (NEH_DECRYPT | NEH_LOAD_KEY)

#define NEH_ENC_GEN_DATA {\
    NEH_ENC_GEN | NEH_KEY128, 0, 0, 0,\
    NEH_ENC_GEN | NEH_KEY192, 0, 0, 0,\
    NEH_ENC_GEN | NEH_KEY256, 0, 0, 0 }

#define NEH_ENC_LOAD_DATA {\
    NEH_ENC_LOAD | NEH_KEY128, 0, 0, 0,\
    NEH_ENC_LOAD | NEH_KEY192, 0, 0, 0,\
    NEH_ENC_LOAD | NEH_KEY256, 0, 0, 0 }

#define NEH_ENC_HYBRID_DATA {\
    NEH_ENC_GEN  | NEH_KEY128, 0, 0, 0,\
    NEH_ENC_LOAD | NEH_KEY192, 0, 0, 0,\
    NEH_ENC_LOAD | NEH_KEY256, 0, 0, 0 }

#define NEH_DEC_GEN_DATA {\
    NEH_DEC_GEN | NEH_KEY128, 0, 0, 0,\
    NEH_DEC_GEN | NEH_KEY192, 0, 0, 0,\
    NEH_DEC_GEN | NEH_KEY256, 0, 0, 0 }

#define NEH_DEC_LOAD_DATA {\
    NEH_DEC_LOAD | NEH_KEY128, 0, 0, 0,\
    NEH_DEC_LOAD | NEH_KEY192, 0, 0, 0,\
    NEH_DEC_LOAD | NEH_KEY256, 0, 0, 0 }

#define NEH_DEC_HYBRID_DATA {\
    NEH_DEC_GEN  | NEH_KEY128, 0, 0, 0,\
    NEH_DEC_LOAD | NEH_KEY192, 0, 0, 0,\
    NEH_DEC_LOAD | NEH_KEY256, 0, 0, 0 }

#define neh_enc_gen_key(x)  ((x) == 128 ? (NEH_ENC_GEN | NEH_KEY128) :      \
     (x) == 192 ? (NEH_ENC_GEN | NEH_KEY192) : (NEH_ENC_GEN | NEH_KEY256))

#define neh_enc_load_key(x) ((x) == 128 ? (NEH_ENC_LOAD | NEH_KEY128) :     \
     (x) == 192 ? (NEH_ENC_LOAD | NEH_KEY192) : (NEH_ENC_LOAD | NEH_KEY256))

#define neh_enc_hybrid_key(x)   ((x) == 128 ? (NEH_ENC_GEN | NEH_KEY128) :  \
     (x) == 192 ? (NEH_ENC_LOAD | NEH_KEY192) : (NEH_ENC_LOAD | NEH_KEY256))

#define neh_dec_gen_key(x)  ((x) == 128 ? (NEH_DEC_GEN | NEH_KEY128) :      \
     (x) == 192 ? (NEH_DEC_GEN | NEH_KEY192) : (NEH_DEC_GEN | NEH_KEY256))

#define neh_dec_load_key(x) ((x) == 128 ? (NEH_DEC_LOAD | NEH_KEY128) :     \
     (x) == 192 ? (NEH_DEC_LOAD | NEH_KEY192) : (NEH_DEC_LOAD | NEH_KEY256))

#define neh_dec_hybrid_key(x)   ((x) == 128 ? (NEH_DEC_GEN | NEH_KEY128) :  \
     (x) == 192 ? (NEH_DEC_LOAD | NEH_KEY192) : (NEH_DEC_LOAD | NEH_KEY256))

#if defined( _MSC_VER ) && ( _MSC_VER > 1200 )
#define aligned_auto(type, name, no, stride)  __declspec(align(stride)) type name[no]
#else
#define aligned_auto(type, name, no, stride)                \
    unsigned char _##name[no * sizeof(type) + stride];      \
    type *name = (type*)(16 * ((((unsigned long)(_##name)) + stride - 1) / stride))
#endif

#if defined( _MSC_VER ) && ( _MSC_VER > 1200 )
#define aligned_array(type, name, no, stride) __declspec(align(stride)) type name[no]
#elif defined( __GNUC__ )
#define aligned_array(type, name, no, stride) type name[no] __attribute__ ((aligned(stride)))
#else
#define aligned_array(type, name, no, stride) type name[no]
#endif

/* VIA ACE codeword     */

static unsigned char via_flags = 0;

#if defined ( _MSC_VER ) && ( _MSC_VER > 800 )

#define NEH_REKEY   __asm pushfd __asm popfd
#define NEH_AES     __asm _emit 0xf3 __asm _emit 0x0f __asm _emit 0xa7
#define NEH_ECB     NEH_AES __asm _emit 0xc8
#define NEH_CBC     NEH_AES __asm _emit 0xd0
#define NEH_CFB     NEH_AES __asm _emit 0xe0
#define NEH_OFB     NEH_AES __asm _emit 0xe8
#define NEH_RNG     __asm _emit 0x0f __asm _emit 0xa7 __asm _emit 0xc0

INLINE int has_cpuid(void)
{   char ret_value;
    __asm
    {   pushfd                  /* save EFLAGS register     */
        mov     eax,[esp]       /* copy it to eax           */
        mov     edx,0x00200000  /* CPUID bit position       */
        xor     eax,edx         /* toggle the CPUID bit     */
        push    eax             /* attempt to set EFLAGS to */
        popfd                   /*     the new value        */
        pushfd                  /* get the new EFLAGS value */
        pop     eax             /*     into eax             */
        xor     eax,[esp]       /* xor with original value  */
        and     eax,edx         /* has CPUID bit changed?   */
        setne   al              /* set to 1 if we have been */
        mov     ret_value,al    /*     able to change it    */
        popfd                   /* restore original EFLAGS  */
    }
    return (int)ret_value;
}

INLINE int is_via_cpu(void)
{   char ret_value;
    __asm
    {   push    ebx
        xor     eax,eax         /* use CPUID to get vendor  */
        cpuid                   /* identity string          */
        xor     eax,eax         /* is it "CentaurHauls" ?   */
        sub     ebx,0x746e6543  /* 'Cent'                   */
        or      eax,ebx
        sub     edx,0x48727561  /* 'aurH'                   */
        or      eax,edx
        sub     ecx,0x736c7561  /* 'auls'                   */
        or      eax,ecx
        sete    al              /* set to 1 if it is VIA ID */
        mov     dl,NEH_CPU_READ /* mark CPU type as read    */
        or      dl,al           /* & store result in flags  */
        mov     [via_flags],dl  /* set VIA detected flag    */
        mov     ret_value,al    /*     able to change it    */
        pop     ebx
    }
    return (int)ret_value;
}

INLINE int read_via_flags(void)
{   char ret_value = 0;
    __asm
    {   mov     eax,0xC0000000  /* Centaur extended CPUID   */
        cpuid
        mov     edx,0xc0000001  /* >= 0xc0000001 if support */
        cmp     eax,edx         /* for VIA extended feature */
        jnae    no_rng          /*     flags is available   */
        mov     eax,edx         /* read Centaur extended    */
        cpuid                   /*     feature flags        */
        mov     eax,NEH_FLAGS_MASK  /* mask out and save    */
        and     eax,edx         /*  the RNG and ACE flags   */
        or      [via_flags],al  /* present & enabled flags  */
        mov     ret_value,al    /*     able to change it    */
no_rng:
    }
    return (int)ret_value;
}

INLINE unsigned int via_rng_in(void *buf)
{   char ret_value = 0x1f;
    __asm
    {   push    edi
        mov     edi,buf         /* input buffer address     */
        xor     edx,edx         /* try to fetch 8 bytes     */
        NEH_RNG                 /* do RNG read operation    */
        and     ret_value,al    /* count of bytes returned  */
        pop     edi
    }
    return (int)ret_value;
}

INLINE void via_ecb_op5(
            const void *k, const void *c, const void *s, void *d, int l)
{   __asm
    {   push    ebx
        NEH_REKEY
        mov     ebx, (k)
        mov     edx, (c)
        mov     esi, (s)
        mov     edi, (d)
        mov     ecx, (l)
        NEH_ECB
        pop     ebx
    }
}

INLINE void via_cbc_op6(
            const void *k, const void *c, const void *s, void *d, int l, void *v)
{   __asm
    {   push    ebx
        NEH_REKEY
        mov     ebx, (k)
        mov     edx, (c)
        mov     esi, (s)
        mov     edi, (d)
        mov     ecx, (l)
        mov     eax, (v)
        NEH_CBC
        pop     ebx
    }
}

INLINE void via_cbc_op7(
        const void *k, const void *c, const void *s, void *d, int l, void *v, void *w)
{   __asm
    {   push    ebx
        NEH_REKEY
        mov     ebx, (k)
        mov     edx, (c)
        mov     esi, (s)
        mov     edi, (d)
        mov     ecx, (l)
        mov     eax, (v)
        NEH_CBC
        mov     esi, eax
        mov     edi, (w)
        movsd
        movsd
        movsd
        movsd
        pop     ebx
    }
}

INLINE void via_cfb_op6(
            const void *k, const void *c, const void *s, void *d, int l, void *v)
{   __asm
    {   push    ebx
        NEH_REKEY
        mov     ebx, (k)
        mov     edx, (c)
        mov     esi, (s)
        mov     edi, (d)
        mov     ecx, (l)
        mov     eax, (v)
        NEH_CFB
        pop     ebx
    }
}

INLINE void via_cfb_op7(
        const void *k, const void *c, const void *s, void *d, int l, void *v, void *w)
{   __asm
    {   push    ebx
        NEH_REKEY
        mov     ebx, (k)
        mov     edx, (c)
        mov     esi, (s)
        mov     edi, (d)
        mov     ecx, (l)
        mov     eax, (v)
        NEH_CFB
        mov     esi, eax
        mov     edi, (w)
        movsd
        movsd
        movsd
        movsd
        pop     ebx
    }
}

INLINE void via_ofb_op6(
            const void *k, const void *c, const void *s, void *d, int l, void *v)
{   __asm
    {   push    ebx
        NEH_REKEY
        mov     ebx, (k)
        mov     edx, (c)
        mov     esi, (s)
        mov     edi, (d)
        mov     ecx, (l)
        mov     eax, (v)
        NEH_OFB
        pop     ebx
    }
}

#elif defined( __GNUC__ )

#define NEH_REKEY   asm("pushfl\n popfl\n\t")
#define NEH_ECB     asm(".byte 0xf3, 0x0f, 0xa7, 0xc8\n\t")
#define NEH_CBC     asm(".byte 0xf3, 0x0f, 0xa7, 0xd0\n\t")
#define NEH_CFB     asm(".byte 0xf3, 0x0f, 0xa7, 0xe0\n\t")
#define NEH_OFB     asm(".byte 0xf3, 0x0f, 0xa7, 0xe8\n\t")
#define NEH_RNG     asm(".byte 0x0f, 0xa7, 0xc0\n\t");

INLINE int has_cpuid(void)
{   int val;
    asm("pushfl\n\t");
    asm("movl  0(%esp),%eax\n\t");
    asm("xor   $0x00200000,%eax\n\t");
    asm("pushl %eax\n\t");
    asm("popfl\n\t");
    asm("pushfl\n\t");
    asm("popl  %eax\n\t");
    asm("xorl  0(%esp),%edx\n\t");
    asm("andl  $0x00200000,%eax\n\t");
    asm("movl  %%eax,%0\n\t" : "=m" (val));
    asm("popfl\n\t");
    return val ? 1 : 0;
}

INLINE int is_via_cpu(void)
{   int val;
    asm("pushl %ebx\n\t");
    asm("xorl %eax,%eax\n\t");
    asm("cpuid\n\t");
    asm("xorl %eax,%eax\n\t");
    asm("subl $0x746e6543,%ebx\n\t");
    asm("orl  %ebx,%eax\n\t");
    asm("subl $0x48727561,%edx\n\t");
    asm("orl  %edx,%eax\n\t");
    asm("subl $0x736c7561,%ecx\n\t");
    asm("orl  %ecx,%eax\n\t");
    asm("movl %%eax,%0\n\t" : "=m" (val));
    asm("popl %ebx\n\t");
    val = (val ? 0 : 1);
    via_flags = (val | NEH_CPU_READ);
    return val;
}

INLINE int read_via_flags(void)
{   unsigned char   val;
    asm("movl $0xc0000000,%eax\n\t");
    asm("cpuid\n\t");
    asm("movl $0xc0000001,%edx\n\t");
    asm("cmpl %edx,%eax\n\t");
    asm("setae %al\n\t");
    asm("movb %%al,%0\n\t" : "=m" (val));
    if(!val) return 0;
    asm("movl $0xc0000001,%eax\n\t");
    asm("cpuid\n\t");
    asm("movb %%dl,%0\n\t" : "=m" (val));
    val &= NEH_FLAGS_MASK;
    via_flags |= val;
    return (int) val;
}

INLINE int via_rng_in(void *buf)
{   int val;
    asm("pushl %edi\n\t");
    asm("movl %0,%%edi\n\t" : : "m" (buf));
    asm("xorl %edx,%edx\n\t");
    NEH_RNG
    asm("andl $0x0000001f,%eax\n\t");
    asm("movl %%eax,%0\n\t" : "=m" (val));
    asm("popl %edi\n\t");
    return val;
}

INLINE volatile  void via_ecb_op5(
            const void *k, const void *c, const void *s, void *d, int l)
{
    asm("pushl %ebx\n\t");
    NEH_REKEY;
    asm("movl %0, %%ebx\n\t" : : "m" (k));
    asm("movl %0, %%edx\n\t" : : "m" (c));
    asm("movl %0, %%esi\n\t" : : "m" (s));
    asm("movl %0, %%edi\n\t" : : "m" (d));
    asm("movl %0, %%ecx\n\t" : : "m" (l));
    NEH_ECB;
    asm("popl %ebx\n\t");
}

INLINE volatile  void via_cbc_op6(
            const void *k, const void *c, const void *s, void *d, int l, void *v)
{
    asm("pushl %ebx\n\t");
    NEH_REKEY;
    asm("movl %0, %%ebx\n\t" : : "m" (k));
    asm("movl %0, %%edx\n\t" : : "m" (c));
    asm("movl %0, %%esi\n\t" : : "m" (s));
    asm("movl %0, %%edi\n\t" : : "m" (d));
    asm("movl %0, %%ecx\n\t" : : "m" (l));
    asm("movl %0, %%eax\n\t" : : "m" (v));
    NEH_CBC;
    asm("popl %ebx\n\t");
}

INLINE volatile  void via_cbc_op7(
        const void *k, const void *c, const void *s, void *d, int l, void *v, void *w)
{
    asm("pushl %ebx\n\t");
    NEH_REKEY;
    asm("movl %0, %%ebx\n\t" : : "m" (k));
    asm("movl %0, %%edx\n\t" : : "m" (c));
    asm("movl %0, %%esi\n\t" : : "m" (s));
    asm("movl %0, %%edi\n\t" : : "m" (d));
    asm("movl %0, %%ecx\n\t" : : "m" (l));
    asm("movl %0, %%eax\n\t" : : "m" (v));
    NEH_CBC;
    asm("movl %eax,%esi\n\t");
    asm("movl %0, %%edi\n\t" : : "m" (w));
    asm("movsl; movsl; movsl; movsl\n\t");
    asm("popl %ebx\n\t");
}

INLINE volatile  void via_cfb_op6(
            const void *k, const void *c, const void *s, void *d, int l, void *v)
{
    asm("pushl %ebx\n\t");
    NEH_REKEY;
    asm("movl %0, %%ebx\n\t" : : "m" (k));
    asm("movl %0, %%edx\n\t" : : "m" (c));
    asm("movl %0, %%esi\n\t" : : "m" (s));
    asm("movl %0, %%edi\n\t" : : "m" (d));
    asm("movl %0, %%ecx\n\t" : : "m" (l));
    asm("movl %0, %%eax\n\t" : : "m" (v));
    NEH_CFB;
    asm("popl %ebx\n\t");
}

INLINE volatile  void via_cfb_op7(
        const void *k, const void *c, const void *s, void *d, int l, void *v, void *w)
{
    asm("pushl %ebx\n\t");
    NEH_REKEY;
    asm("movl %0, %%ebx\n\t" : : "m" (k));
    asm("movl %0, %%edx\n\t" : : "m" (c));
    asm("movl %0, %%esi\n\t" : : "m" (s));
    asm("movl %0, %%edi\n\t" : : "m" (d));
    asm("movl %0, %%ecx\n\t" : : "m" (l));
    asm("movl %0, %%eax\n\t" : : "m" (v));
    NEH_CFB;
    asm("movl %eax,%esi\n\t");
    asm("movl %0, %%edi\n\t" : : "m" (w));
    asm("movsl; movsl; movsl; movsl\n\t");
    asm("popl %ebx\n\t");
}

INLINE volatile  void via_ofb_op6(
            const void *k, const void *c, const void *s, void *d, int l, void *v)
{
    asm("pushl %ebx\n\t");
    NEH_REKEY;
    asm("movl %0, %%ebx\n\t" : : "m" (k));
    asm("movl %0, %%edx\n\t" : : "m" (c));
    asm("movl %0, %%esi\n\t" : : "m" (s));
    asm("movl %0, %%edi\n\t" : : "m" (d));
    asm("movl %0, %%ecx\n\t" : : "m" (l));
    asm("movl %0, %%eax\n\t" : : "m" (v));
    NEH_OFB;
    asm("popl %ebx\n\t");
}

#else
#error VIA ACE is not available with this compiler
#endif

INLINE int via_ace_test(void)
{
    return has_cpuid() && is_via_cpu() && ((read_via_flags() & NEH_ACE_FLAGS) == NEH_ACE_FLAGS);
}

#define VIA_ACE_AVAILABLE   (((via_flags & NEH_ACE_FLAGS) == NEH_ACE_FLAGS)         \
    || (via_flags & NEH_CPU_READ) && (via_flags & NEH_CPU_IS_VIA) || via_ace_test())

INLINE int via_rng_test(void)
{
    return has_cpuid() && is_via_cpu() && ((read_via_flags() & NEH_RNG_FLAGS) == NEH_RNG_FLAGS);
}

#define VIA_RNG_AVAILABLE   (((via_flags & NEH_RNG_FLAGS) == NEH_RNG_FLAGS)         \
    || (via_flags & NEH_CPU_READ) && (via_flags & NEH_CPU_IS_VIA) || via_rng_test())

INLINE int read_via_rng(void *buf, int count)
{   int nbr, max_reads, lcnt = count;
    unsigned char *p, *q;
    aligned_auto(unsigned char, bp, 64, 16);

    if(!VIA_RNG_AVAILABLE)
        return 0;

    do
    {
        max_reads = MAX_READ_ATTEMPTS;
        do
            nbr = via_rng_in(bp);
        while
            (nbr == 0 && --max_reads);

        lcnt -= nbr;
        p = (unsigned char*)buf; q = bp;
        while(nbr--)
            *p++ = *q++;
    }
    while
        (lcnt && max_reads);

    return count - lcnt;
}

#endif
