#include "jitc_x86.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define PARAM_INT      1
#define PARAM_FLOAT    2
#define PARAM_REG      3
#define PARAM_dispREG  4
#define PARAM_DISP32   5
#define PARAM_LABEL    6
#define PARAM_NONE     666

typedef struct {
  int    id;
  int    i;
  double f;
  int    reg;
  int    disp;
  char   label[256];
} IParam;

struct {
  char *name;
  int   reg;
} RegsName[] = {
  {"eax",EAX}, {"ebx",EBX}, {"ecx",ECX}, {"edx",EDX},
  {"edi",EDI}, {"esi",ESI}, {"ebp",EBP}, {"esp",ESP},
  {"st0",0}, {"st1",1}, {"st2",2}, {"st3",3},
  {"st4",4}, {"st5",5}, {"st6",6}, {"st7",7},
  {"mm0",0}, {"mm1",1}, {"mm2",2}, {"mm3",3},
  {"mm4",4}, {"mm5",5}, {"mm6",6}, {"mm7",7}, {NULL,0}
};

void modrm(JitcX86Env *jitc, int opcode, IParam *iparam)
{
  int dest = 0;
  int src  = 1;
  int direction = 0x0;
  unsigned int byte   = 666;
  unsigned int int32  = 0;
  unsigned int need32 = 0;

  if ((iparam[0].id == PARAM_REG) && (iparam[1].id != PARAM_REG)) {
    dest = 1;
    src  = 0;
    direction = 0x02;
  }

  if (iparam[src].id != PARAM_REG) {
    fprintf(stderr, "JITC_x86: Invalid Instruction Parameters: %d %d.\n", iparam[0].id, iparam[1].id);
    exit(1);
  }

  if (iparam[dest].id == PARAM_REG) {
    byte = ((int)JITC_MOD_REG_REG << 6) | (iparam[src].reg << 3) | (iparam[0].reg);
  }

  else if (iparam[dest].id == PARAM_dispREG)
  {
    if (iparam[dest].disp == 0)
      byte = ((int)JITC_MOD_pREG_REG << 6) | (iparam[src].reg << 3) | (iparam[dest].reg);
  }

  else if (iparam[dest].id == PARAM_DISP32)
  {
    byte   = ((int)JITC_MOD_pREG_REG << 6) | (iparam[src].reg << 3) | JITC_RM_DISP32;
    need32 = 1;
    int32  = iparam[dest].disp;
  }

  if (byte == 666) {
    fprintf(stderr, "JITC_x86: Invalid Instruction Parameters: %d %d.\n", iparam[0].id, iparam[1].id);
    exit(1);
  }
  else {
    if (opcode < 0x100)
      JITC_ADD_UCHAR(jitc, opcode + direction);
    else {
      JITC_ADD_UCHAR(jitc, (opcode>>8)&0xff);
      JITC_ADD_UCHAR(jitc, (opcode&0xff)/* + direction*/);
    }
    JITC_ADD_UCHAR(jitc, byte);
    if (need32)
      JITC_ADD_UINT(jitc, int32);
  }
}
      
static void imul_like_modrm_1param(JitcX86Env *jitc, int opcode, int digit, IParam *iparam)
{
  if (iparam[0].id == PARAM_REG)
  {
    JITC_ADD_UCHAR(jitc, opcode);
    JITC_MODRM(jitc, 0x03, digit, iparam[0].reg);
    return;
  }
  if (iparam[0].id == PARAM_dispREG) {
    JITC_ADD_UCHAR(jitc, opcode);
    if (iparam[0].disp == 0)
    {
      JITC_MODRM(jitc, 0x00, digit, iparam[0].reg);
    }
    else if ((iparam[0].disp & 0xff) == iparam[0].disp)
    {
      JITC_MODRM(jitc, 0x01, digit, iparam[0].reg);
      JITC_ADD_UCHAR(jitc, iparam[0].disp);
    }
    else
    {
      JITC_MODRM(jitc, 0x02, digit, iparam[0].reg);
      JITC_ADD_UINT(jitc, iparam[0].disp);
    }
    return;
  }
  if (iparam[0].id == PARAM_DISP32) {
    JITC_ADD_UCHAR(jitc, opcode);
    JITC_MODRM(jitc, JITC_MOD_pREG_REG, digit, JITC_RM_DISP32);
    JITC_ADD_UINT(jitc, iparam[0].disp);
    return;
  }
}

/* 1 byte encoded opcode including register... imm32 parameter */
#define INSTR_1bReg_IMM32(opcode,dest,src) { \
      JITC_ADD_UCHAR(jitc, opcode + iparam[dest].reg); \
      JITC_ADD_UINT (jitc, (int)iparam[src].i); }

typedef struct {
  char *name;
  int opcode;
  int opcode_reg_int;
  int digit_reg_int;
  int opcode_eax_int;
} AddLikeInstr;

static AddLikeInstr addLike[] = {
  { "add",  0x01,   0x81, 0x00, 0x05 },
  { "and",  0x21,   0x81, 0x04, 0x25 },
  { "or",   0x0B,   0x81, 0x01, 0x0D },
  { "cmp",  0x39,   0x81, 0x07, 0x3D },
  { "imul", 0x0FAF, 0x69, 0x00, 0x10000 },
  { "sub",  0x29,   0x81, 0x05, 0X2D },
  { NULL,       -1,   -1,   -1,   -1 }
};

int checkAddLike(JitcX86Env *jitc, char *op, IParam *iparam, int nbParams)
{
  int i;
  for (i=0;addLike[i].name;++i)
  {
    if (strcmp(op,addLike[i].name) == 0)
    {
      if ((iparam[0].id == PARAM_REG) && (iparam[1].id == PARAM_INT)) {
        if ((iparam[0].reg == EAX) && (addLike[i].opcode_eax_int != 0x10000)) {
          JITC_ADD_UCHAR(jitc, addLike[i].opcode_eax_int);
          JITC_ADD_UINT(jitc,  iparam[1].i);
          return 1;
        }
        else {
          JITC_ADD_UCHAR(jitc, addLike[i].opcode_reg_int);
          JITC_MODRM(jitc,     0x03, addLike[i].digit_reg_int, iparam[0].reg);
          JITC_ADD_UINT(jitc,  iparam[1].i);
          return 1;
        }
      }
      else if ((iparam[0].id == PARAM_dispREG) && (iparam[1].id == PARAM_INT)) {
        JITC_ADD_UCHAR(jitc, addLike[i].opcode_reg_int);
        if ((iparam[0].disp & 0xff) == iparam[0].disp)
        {
          JITC_MODRM(jitc,     0x01, addLike[i].digit_reg_int, iparam[0].reg);
          JITC_ADD_UCHAR(jitc,  iparam[0].disp);
        }
        else
        {
          JITC_MODRM(jitc,     0x00, addLike[i].digit_reg_int, iparam[0].reg);
          JITC_ADD_UINT(jitc,  iparam[0].disp);
        }
        JITC_ADD_UINT(jitc,  iparam[1].i);
        return 1;
      }
      else if ((iparam[0].id == PARAM_DISP32) && (iparam[1].id == PARAM_INT)) {
        JITC_ADD_UCHAR(jitc, addLike[i].opcode_reg_int);
        JITC_MODRM(jitc, 0x00, addLike[i].digit_reg_int, 0x05);
        JITC_ADD_UINT(jitc,  iparam[0].disp);
        JITC_ADD_UINT(jitc,  iparam[1].i);
        return 1;
      }
      else {
        modrm(jitc, addLike[i].opcode, iparam);
        return 1;
      }
    }
  }
  return 0;
}

/**
 * Check all kind of known instruction... perform special optimisations..
 */
static void jitc_add_op(JitcX86Env *jitc, char *op, IParam *iparam, int nbParams)
{
  /* MOV */
  if (strcmp(op,"mov") == 0)
  {
    if ((iparam[0].id == PARAM_REG) && (iparam[1].id == PARAM_INT)) {
      INSTR_1bReg_IMM32(0xb8,0,1);
    }
    else if ((iparam[0].id == PARAM_DISP32) && (iparam[1].id == PARAM_INT)) {
      JITC_ADD_UCHAR(jitc, 0xc7);
      JITC_MODRM(jitc, 0x00, 0x00, 0x05);
      JITC_ADD_UINT(jitc, iparam[0].disp);
      JITC_ADD_UINT(jitc, iparam[1].i);
    }
    else
      modrm(jitc, 0x89, iparam);
    return;
  }

#define IMUL_LIKE(_OP,_opcode,_digit)\
  if (strcmp(op, _OP) == 0) { \
    if (nbParams == 1) { \
      imul_like_modrm_1param(jitc, _opcode, _digit, iparam); \
      return; }}

#define SHIFT_LIKE(_name,_op1,_op2,_digit) \
  if (strcmp(op, _name) == 0) { \
    if (iparam[1].id == PARAM_INT) { \
      if (iparam[1].i == 1) \
        imul_like_modrm_1param(jitc, _op1, _digit, iparam); \
      else { \
        imul_like_modrm_1param(jitc, _op2, _digit, iparam); \
        JITC_ADD_UCHAR(jitc, iparam[1].i); \
      } \
      return; \
    } \
  }

#define POP_LIKE(_OP,_opcode) \
  if (strcmp(op, _OP) == 0) { \
    if (iparam[0].id == PARAM_REG) { \
      JITC_ADD_UCHAR(jitc, _opcode + iparam[0].reg); \
      return; } }

  IMUL_LIKE("neg",  0xf7, 0x03);
  IMUL_LIKE("imul", 0xf7, 0x05);
  IMUL_LIKE("idiv", 0xf7, 0x07);

  POP_LIKE("pop", 0x58);  
  POP_LIKE("push", 0x50);

  SHIFT_LIKE("sal", 0xd1, 0xc1, 0x04);
  SHIFT_LIKE("sar", 0xd1, 0xc1, 0x07);
  SHIFT_LIKE("shl", 0xd1, 0xc1, 0x04);
  SHIFT_LIKE("shr", 0xd1, 0xc1, 0x05);

  /* INC */
  if (strcmp(op, "inc") == 0) {
    if (iparam[0].id == PARAM_REG) {
      JITC_ADD_UCHAR(jitc, 0x40 + iparam[0].reg);
      return;
    }
    imul_like_modrm_1param(jitc, 0xff, 0x00, iparam);
    return;
  }

  /* DEC */
  if (strcmp(op, "dec") == 0) {
    if (iparam[0].id == PARAM_REG) {
      JITC_ADD_UCHAR(jitc, 0x48 + iparam[0].reg);
      return;
    }
    imul_like_modrm_1param(jitc, 0xff, 0x01, iparam);
    return;
  }

  if (strcmp(op, "call") == 0)
  {
    if (iparam[0].id == PARAM_LABEL) {
      jitc_add_used_label(jitc,iparam[0].label,jitc->used+1);
      JITC_CALL(jitc,0);
      return;
    }
    if (iparam[0].id == PARAM_INT) {
      JITC_CALL(jitc,iparam[0].i);
      return;
    }
    if (iparam[0].id == PARAM_dispREG) {
      JITC_ADD_UCHAR(jitc,0xff);
      JITC_ADD_UCHAR(jitc,0xd0+iparam[0].reg);
      return;
    }
  }
  
#define MONOBYTE_INSTR(_OP,_opcode) \
  if (strcmp(op, _OP) == 0) { \
    JITC_ADD_UCHAR(jitc, _opcode); \
    return; }

  MONOBYTE_INSTR("ret", 0xc3);
  MONOBYTE_INSTR("leave", 0xc9);
  MONOBYTE_INSTR("cdq", 0x99);

  /* JNE */
  if (strcmp(op, "jne") == 0) {
    if (iparam[0].id == PARAM_LABEL) {
      JITC_JUMP_COND_LABEL(jitc,COND_NOT_EQUAL,iparam[0].label);
      return;
    }
    if (iparam[0].id == PARAM_INT) {
      JITC_JUMP_COND(jitc,COND_NOT_EQUAL,iparam[0].i);
      return;
    }
  }

  /* JE */
  if (strcmp(op, "je") == 0) {
    if (iparam[0].id == PARAM_LABEL) {
      JITC_JUMP_COND_LABEL(jitc,COND_EQUAL,iparam[0].label);
      return;
    }
    if (iparam[0].id == PARAM_INT) {
      JITC_JUMP_COND(jitc,COND_EQUAL,iparam[0].i);
      return;
    }
  }
    
  /* ADD LIKE */
  if (checkAddLike(jitc, op, iparam, nbParams)) return;

  /* BSWAP : 0F C8+rd */
  
  fprintf(stderr, "JITC_x86: Invalid Operation '%s'\n", op);
  exit(1);
}

/**
 * Adds a new instruction to the just in time compiled function
 */
void jitc_add(JitcX86Env *jitc, const char *_instr, ...)
{
  char instr[256];
  char *op;
  char *sparam[16]; int nbParam=0; int i;
  IParam iparam[16];
  va_list ap;
  strcpy(instr,_instr);

#ifdef DISPLAY_GENCODE
  printf("|");
#endif
  
  op = strtok(instr, " ,");
  if (!op) return;

  /* decoupage en tokens */
  while ((sparam[nbParam] = strtok(NULL, " ,")) != NULL) if (strlen(sparam[nbParam])>0) nbParam++;

  /* Reconnaissance des parametres */
  va_start(ap, _instr);
  for (i=0;i<nbParam;++i)
  {
    int r;
    char regname[256];
    iparam[i].id = PARAM_NONE;
    if (strcmp(sparam[i], "$d") == 0) {
      iparam[i].id = PARAM_INT;
      iparam[i].i  = va_arg(ap, int);
    }
    else if (strcmp(sparam[i], "$f") == 0) {
      iparam[i].id = PARAM_FLOAT;
      iparam[i].f  = va_arg(ap, double);
    }
    else if (strcmp(sparam[i], "[$d]") == 0) {
      iparam[i].id   = PARAM_DISP32;
      iparam[i].disp = va_arg(ap, int);
    }
    else if (strcmp(sparam[i], "$s") == 0) {
      iparam[i].id   = PARAM_LABEL;
      strcpy(iparam[i].label, va_arg(ap, char*));
    }
    else
    for (r=0;RegsName[r].name;r++) {
      if (strcmp(sparam[i], RegsName[r].name) == 0) {
        iparam[i].id  = PARAM_REG;
        iparam[i].reg = RegsName[r].reg;
      }
      else
      {
        if (sscanf(sparam[i], "$d[%3s]", regname) > 0) {
          if (strcmp(regname, RegsName[r].name) == 0) {
            iparam[i].id   = PARAM_dispREG;
            iparam[i].reg  = RegsName[r].reg;
            iparam[i].disp = va_arg(ap, int);
          }
        }
        if (sscanf(sparam[i], "[%3s]", regname) > 0) {
          if (strcmp(regname, RegsName[r].name) == 0) {
            iparam[i].id   = PARAM_dispREG;
            iparam[i].reg  = RegsName[r].reg;
            iparam[i].disp = 0;
          }
        }
      }
    }
    if (iparam[i].id == PARAM_NONE) {
      fprintf(stderr, "JITC_x86: Unrecognized parameter '%s'\n", sparam[i]);
      exit(1);
    }
  }
  va_end(ap);

  jitc_add_op(jitc, op, &(iparam[0]), nbParam);
#ifdef DISPLAY_GENCODE
  printf(" ;;;  %s", op);
  for (i=0;i<nbParam;++i)
  {
    if (iparam[i].id == PARAM_INT)
      printf(" 0x%x", iparam[i].i);
    else if (iparam[i].id == PARAM_DISP32)
      printf(" [0x%x]", iparam[i].disp);
    else if (iparam[i].id == PARAM_LABEL)
      printf(" %s", iparam[i].label);
    else
      printf(" %s", sparam[i]);
  }
  printf("\n");
  
#endif
}

JitcX86Env *jitc_x86_env_new(int memory_size) {

    JitcX86Env *jitc = (JitcX86Env*)malloc(sizeof(JitcX86Env));
    jitc->_memory = (unsigned char*)malloc(memory_size);
    jitc->used    = 0;
    jitc->memory  = (unsigned char*)((int)jitc->_memory + (32-((int)jitc->_memory)%32)%32);

    jitc->nbUsedLabel  = 0;
    jitc->nbKnownLabel = 0;

    jitc->usedLabel  = (LabelAddr*)malloc(sizeof(LabelAddr) * JITC_MAXLABEL);
    jitc->knownLabel = (LabelAddr*)malloc(sizeof(LabelAddr) * JITC_MAXLABEL);
    
    return jitc;
}

void jitc_x86_delete(JitcX86Env *jitc) {
    
    free(jitc->usedLabel);
    free(jitc->knownLabel);
    free(jitc->_memory);
    free(jitc);
}

JitcFunc jitc_prepare_func(JitcX86Env *jitc) {

    JitcFunc ptr = 0;
    jitc->used = (32 - jitc->used%32)%32;
    ptr = (JitcFunc)&(jitc->memory[jitc->used]);

#ifdef DISPLAY_GENCODE
    printf("\n------------------------------------------\n");
    printf("-- Function Intro                       --\n");
    printf("------------------------------------------\n");
#endif

    /* save the state */
    jitc_add(jitc,"push ebp");
    jitc_add(jitc,"mov ebp, esp");
    jitc_add(jitc,"sub esp, $d", 8);
    JITC_PUSH_ALL(jitc);
#ifdef DISPLAY_GENCODE
    printf("\n------------------------------------------\n");
#endif
    return ptr;
}

void jitc_validate_func(JitcX86Env *jitc) {

#ifdef DISPLAY_GENCODE
    printf("\n------------------------------------------\n");
    printf("-- Function Outro                       --\n");
    printf("------------------------------------------\n");
#endif
    /* restore the state */
    JITC_POP_ALL(jitc);
    jitc_add(jitc, "leave");
    jitc_add(jitc, "ret");
    jitc_resolve_labels(jitc);
#ifdef DISPLAY_GENCODE
    printf("\n------------------------------------------\n");
#endif
}

void jitc_add_used_label(JitcX86Env *jitc, char *label, int where) {

    strncpy(jitc->usedLabel[jitc->nbUsedLabel].label, label, JITC_LABEL_SIZE);
    jitc->usedLabel[jitc->nbUsedLabel].address = where;
    jitc->nbUsedLabel++;
}

void jitc_add_known_label(JitcX86Env *jitc, char *label, int where) {

#ifdef DISPLAY_GENCODE
    printf("%s:\n", label);
#endif
    strncpy(jitc->knownLabel[jitc->nbKnownLabel].label, label, JITC_LABEL_SIZE);
    jitc->knownLabel[jitc->nbKnownLabel].address = where;
    jitc->nbKnownLabel++;
}

void jitc_resolve_labels(JitcX86Env *jitc) {

    int i,j;
    for (i=jitc->nbUsedLabel;i-->0;) {

        LabelAddr used = jitc->usedLabel[i];
        for (j=jitc->nbKnownLabel;j-->0;) {

            LabelAddr known = jitc->knownLabel[j];
            if (strcmp(known.label, used.label) == 0) {
                int *offset = (int*)&(jitc->memory[used.address]);
                *offset = known.address - used.address - 4; /* always using long offset */
                break;
            }
        }
    }
    jitc->nbUsedLabel = jitc->nbKnownLabel = 0;
}
