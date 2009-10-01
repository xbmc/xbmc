/**
 * Copyright (c)2004 Jean-Christophe Hoelt <jeko@ios-software.com>
 */

#include <stdlib.h>
#include <string.h>

#define JITC_MAXLABEL 1024
#define JITC_LABEL_SIZE 64

/**
 * low level macros
 */

 /* {{{ Registres Generaux */
#define EAX 0
#define ECX 1
#define EDX 2
#define EBX 3
#define ESP 4
#define EBP 5
#define ESI 6
#define EDI 7
 /* }}} */
 /* {{{ Registres MMX */
#define MM0 0
#define MM1 1
#define MM2 2
#define MM3 3
#define MM4 4
#define MM5 5
#define MM6 6
#define MM7 7
 /* }}} */
 /* {{{  Registres SSE*/
#define XMM0 0
#define XMM1 1
#define XMM2 2
#define XMM3 3
#define XMM4 4
#define XMM5 5
#define XMM6 6
#define XMM7 7
 /* }}} */
 /* {{{ Alias aux registres */
#define R0 0
#define R1 1
#define R2 2
#define R3 3
#define R4 4
#define R5 5
#define R6 6
#define R7 7
 /* }}} */

 /* {{{ Conditions */
#define COND_OVERFLOW    0
#define COND_NO_OVERFLOW 1
#define COND_BELOW       2
#define COND_NOT_BELOW   3
#define COND_EQUAL       4
#define COND_ZERO        4
#define COND_NOT_EQUAL   5
#define COND_NOT_ZERO    5
#define COND_NOT_ABOVE   6
#define COND_ABOVE       7
#define COND_SIGN        8
#define COND_NOT_SIGN    9
#define COND_EVEN       10
#define COND_ODD        11
#define COND_LESS_THAN  12
#define COND_GREATER_EQUAL 13
#define COND_LESS_EQUAL    14
#define COND_GREATER_THAN  15
 /* }}} */

typedef int (*JitcFunc)(void);

typedef struct _LABEL_ADDR {
  char label[JITC_LABEL_SIZE];
  int  address;
} LabelAddr;

typedef struct _JITC_X86_ENV {
  unsigned char *_memory;
  unsigned char *memory;
  unsigned int  used;

  int nbUsedLabel;
  int nbKnownLabel;
  LabelAddr    *usedLabel;
  LabelAddr    *knownLabel;
} JitcX86Env;

#define DISPLAY_GENCODE
/*#define DISPLAY_GENCODE_HEXA*/

#ifdef DISPLAY_GENCODE_HEXA
  #define JITC_ADD_UCHAR(jitc,op) printf(" 0x%02x", op) && (jitc->memory[jitc->used++]=op)
#else
  #define JITC_ADD_UCHAR(jitc,op) (jitc->memory[jitc->used++]=op)
#endif

#define JITC_ADD_USHORT(jitc,i) { JITC_ADD_UCHAR(jitc,i&0xff); JITC_ADD_UCHAR(jitc,(i>>8)&0xff); } 
#define JITC_ADD_UINT(jitc,i)   { \
    JITC_ADD_UCHAR(jitc,i&0xff); \
    JITC_ADD_UCHAR(jitc,(i>>8)&0xff); \
    JITC_ADD_UCHAR(jitc,(i>>16)&0xff); \
    JITC_ADD_UCHAR(jitc,(i>>24)&0xff); \
}
#define JITC_ADD_2UCHAR(jitc,op1,op2) {JITC_ADD_UCHAR(jitc,op1); JITC_ADD_UCHAR(jitc,op2);}

#define JITC_MODRM(jitc,mod,reg,rm) { JITC_ADD_UCHAR(jitc,((int)mod<<6)|((int)reg<<3)|((int)rm)); }

/* special values for R/M */
#define JITC_RM_DISP32 0x05

#define JITC_MOD_pREG_REG      0x00
#define JITC_MOD_disp8REG_REG  0x01
#define JITC_MOD_disp32REG_REG 0x02
#define JITC_MOD_REG_REG       0x03
/* cf 24319101 p.27 */

#define JITC_OP_REG_REG(jitc,op,dest,src)     { JITC_ADD_UCHAR(jitc,op); JITC_ADD_UCHAR(jitc,0xc0+(src<<3)+dest); }
#define JITC_OP_REG_pREG(jitc,op,dest,src)    { JITC_ADD_UCHAR(jitc,op); JITC_ADD_UCHAR(jitc,(dest<<3)+src); }
#define JITC_OP_pREG_REG(jitc,op,dest,src)    { JITC_ADD_UCHAR(jitc,op); JITC_ADD_UCHAR(jitc,(src<<3)+dest); }

/**
 * "high" level macro
 */

#define JITC_LOAD_REG_IMM32(jitc,reg,imm32) { JITC_ADD_UCHAR  (jitc,0xb8+reg); JITC_ADD_UINT(jitc,(int)(imm32)); }
#define JITC_LOAD_REG_REG(jitc,dest,src)    { JITC_OP_REG_REG (jitc,0x89,dest,src); }

#define JITC_LOAD_REG_pREG(jitc,dest,src)   { JITC_OP_REG_pREG(jitc,0x8b,dest,src); }
#define JITC_LOAD_pREG_REG(jitc,dest,src)   { JITC_OP_pREG_REG(jitc,0x89,dest,src); }

#define JITC_DEC_REG(jitc,reg)              { JITC_ADD_UCHAR  (jitc,0x48+reg); }
#define JITC_INC_REG(jitc,reg)              { JITC_ADD_UCHAR  (jitc,0x40+reg); }

#define JITC_ADD_REG_REG(jitc,dest,src)     { JITC_OP_REG_REG (jitc,0x01,dest,src); }
#define JITC_ADD_REG_IMM32(jitc,reg,imm32)  { JITC_ADD_UCHAR (jitc,0x81);\
                                              JITC_ADD_UCHAR (jitc,0xc0+reg);\
                                              JITC_ADD_UINT  (jitc,(int)imm32); }

#define JITC_AND_REG_REG(jitc,dest,src)     { JITC_OP_REG_REG (jitc,0x21,dest,src); }
#define JITC_CMP_REG_REG(jitc,dest,src)     { JITC_OP_REG_REG (jitc,0x39,dest,src); }
#define JITC_CMP_REG_IMM32(jitc,reg,imm32)  { JITC_ADD_2UCHAR (jitc,0x81,0xf8+reg); \
                                              JITC_ADD_UINT   (jitc,(int)imm32); }

#define JITC_IDIV_EAX_REG(jitc,reg)         { JITC_ADD_2UCHAR(jitc, 0xf7, 0xf8+reg); }
#define JITC_IMUL_EAX_REG(jitc,reg)         { JITC_ADD_2UCHAR(jitc, 0xf7, 0xe8+reg); }

/*#define JITC_SUB_REG_REG(jitc,dest,src)     { JITC_OP_REG_REG (jitc,0x29,dest,src); }
#define JITC_SUB_EAX_IMM32(jitc,imm32)      { JITC_ADD_UCHAR  (jitc,0x2d); JITC_ADD_UINT(jitc,(int)imm32); }
#define JITC_SUB_REG_IMM32(jitc,reg,imm32)  { JITC_ADD_2UCHAR (jitc,0x81, 0xe8+reg);\
                                              JITC_ADD_UINT   (jitc,(int)imm32); }*/
#define JITC_SUB_REG_IMM8(jitc,reg,imm8)    { JITC_ADD_2UCHAR (jitc,0x83, 0xe8+reg);\
                                              JITC_ADD_UCHAR(jitc,imm8); }

/* Floating points */

#define JITC_FLD_pIMM32(jitc,address)       { JITC_ADD_UCHAR  (jitc, 0xD9); \
                                              JITC_MODRM      (jitc, 0x00, 0x00,JITC_RM_DISP32); \
                                              JITC_ADD_UINT(jitc,(int)(address)); }
#define JITC_FLD_STi(jict,reg)              { JITC_ADD_2UCHAR (jitc, 0xD9, 0xC0+reg); }

#define JITC_FST_pIMM32(jitc,address)       { JITC_ADD_UCHAR  (jitc, 0xD9); \
                                              JITC_MODRM      (jitc, 0x00, 0x02,JITC_RM_DISP32); \
                                              JITC_ADD_UINT(jitc,(int)(ADDRess)); }
#define JITC_FST_STi(jict,reg)              { JITC_ADD_2UCHAR (jitc, 0xDD, 0xD0+reg); }

#define JITC_FSTP_pIMM32(jitc,address)      { JITC_ADD_UCHAR  (jitc, 0xD9); \
                                              JITC_MODRM      (jitc, 0x00, 0x03, JITC_RM_DISP32); \
                                              JITC_ADD_UINT(jitc,(int)(address)); }
#define JITC_FSTP_STi(jict,reg)             { JITC_ADD_2UCHAR (jitc, 0xDD, 0xD8+reg); }

#define JITC_FADD

/* Jumps */

#define JITC_ADD_LABEL(jitc,label)          { jitc_add_known_label(jitc,label,jitc->used); }

#define JITC_JUMP(jitc,offset)              { JITC_ADD_UCHAR(jitc,0xe9); JITC_ADD_UINT(jitc,offset); }
#define JITC_JUMP_LABEL(jitc,label)         { jitc_add_used_label(jitc,label,jitc->used+1); JITC_JUMP(jitc,0); }
#define JITC_JUMP_COND(jitc,cond,offset)    { JITC_ADD_UCHAR(jitc,0x0f);\
                                              JITC_ADD_UCHAR(jitc,0x80+cond);\
                                              JITC_ADD_UINT(jitc,offset); }
#define JITC_JUMP_COND_LABEL(jitc,cond,label) { jitc_add_used_label(jitc,label,jitc->used+2); JITC_JUMP_COND(jitc,cond,0); }
#define JITC_CALL(jitc,function)            { int __offset__ = (int)function - (int)(&jitc->memory[jitc->used+5]);\
                                              JITC_ADD_UCHAR(jitc,0xe8); JITC_ADD_UINT(jitc,__offset__); }
/*#define JITC_CALL_pREG(jitc,reg)            { JITC_ADD_UCHAR(jitc,0xff); JITC_ADD_UCHAR(jitc,0xd0+reg); }
#define JITC_CALL_LABEL(jitc,label)         { jitc_add_used_label(jitc,label,jitc->used+1); JITC_CALL(jitc,0); }*/

/* save all registers (except EAX,ESP,EBP) */
#define JITC_PUSH_ALL(jitc) { jitc_add(jitc,"push ecx"); jitc_add(jitc,"push edx"); jitc_add(jitc,"push ebx"); \
                              jitc_add(jitc,"push esi"); jitc_add(jitc,"push edi"); }

/* restore all registers (except EAX,ESP,EBP) */
#define JITC_POP_ALL(jitc)  { jitc_add(jitc,"pop edi"); jitc_add(jitc,"pop esi"); jitc_add(jitc,"pop ebx"); \
                              jitc_add(jitc,"pop edx"); jitc_add(jitc,"pop ecx"); }

/* public methods */
JitcX86Env *jitc_x86_env_new(int memory_size);
JitcFunc    jitc_prepare_func(JitcX86Env *jitc);
void        jitc_add(JitcX86Env *jitc, const char *instr, ...);
void        jitc_validate_func(JitcX86Env *jitc);
void        jitc_x86_delete(JitcX86Env *jitc);


/* private methods */
void jitc_add_used_label(JitcX86Env *jitc, char *label, int where);
void jitc_add_known_label(JitcX86Env *jitc, char *label, int where);
void jitc_resolve_labels(JitcX86Env *jitc);
