#ifndef _GSL_PRIVATE_H
#define _GSL_PRIVATE_H

/* -- internal use -- */

#include "goomsl.h"

#ifdef USE_JITC_X86
#include "jitc_x86.h"
#endif

#include "goomsl_heap.h"

/* {{{ type of nodes */
#define EMPTY_NODE 0
#define CONST_INT_NODE 1
#define CONST_FLOAT_NODE 2
#define CONST_PTR_NODE 3
#define VAR_NODE 4
#define PARAM_NODE 5
#define READ_PARAM_NODE 6
#define OPR_NODE 7
/* }}} */
/* {{{ type of operations */
#define OPR_SET 1
#define OPR_IF 2
#define OPR_WHILE 3
#define OPR_BLOCK 4
#define OPR_ADD 5
#define OPR_MUL 6
#define OPR_EQU 7
#define OPR_NOT 8
#define OPR_LOW 9
#define OPR_DIV 10
#define OPR_SUB 11
#define OPR_FUNC_INTRO 12
#define OPR_FUNC_OUTRO 13
#define OPR_CALL 14
#define OPR_EXT_CALL 15
#define OPR_PLUS_EQ 16
#define OPR_SUB_EQ 17
#define OPR_MUL_EQ 18
#define OPR_DIV_EQ 19
#define OPR_CALL_EXPR 20
#define OPR_AFFECT_LIST 21
#define OPR_FOREACH 22
#define OPR_VAR_LIST 23

/* }}} */

typedef struct _ConstIntNodeType { /* {{{ */
    int val;
} ConstIntNodeType; /* }}} */
typedef struct _ConstFloatNodeType { /* {{{ */
    float val;
} ConstFloatNodeType; /* }}} */
typedef struct _ConstPtrNodeType { /* {{{ */
    int id;
} ConstPtrNodeType; /* }}} */
typedef struct _OprNodeType { /* {{{ */
    int type;
    int nbOp;
    struct _NODE_TYPE *op[3]; /* maximal number of operand needed */
    struct _NODE_TYPE *next;
} OprNodeType; /* }}} */
typedef struct _NODE_TYPE { /* {{{ */
    int type;
    char *str;
    GoomHash *vnamespace;
    int line_number;
    union {
        ConstIntNodeType constInt;
        ConstFloatNodeType constFloat;
        ConstPtrNodeType constPtr;
        OprNodeType opr;
    } unode;
} NodeType; /* }}} */
typedef struct _INSTRUCTION_DATA { /* {{{ */
  
  union {
    void  *var;
    int   *var_int;
    int   *var_ptr;
    float *var_float;
    int    jump_offset;
    struct _ExternalFunctionStruct *external_function;
  } udest;

  union {
    void  *var;
    int   *var_int;
    int   *var_ptr;
    float *var_float;
    int    value_int;
    int    value_ptr;
    float  value_float;
  } usrc;
} InstructionData;
/* }}} */
typedef struct _INSTRUCTION { /* {{{ */

    int id;
    InstructionData data;
    GoomSL *parent;
    const char *name; /* name of the instruction */

    char     **params; /* parametres de l'instruction */
    GoomHash **vnamespace;
    int       *types;  /* type des parametres de l'instruction */
    int cur_param;
    int nb_param;

    int address;
    char *jump_label;
    char *nop_label;

    int line_number;

} Instruction;
/* }}} */
typedef struct _INSTRUCTION_FLOW { /* {{{ */

    Instruction **instr;
    int number;
    int tabsize;
    GoomHash *labels;
} InstructionFlow;
/* }}} */
typedef struct _FAST_INSTRUCTION { /* {{{ */
  int id;
  InstructionData data;
  Instruction *proto;
} FastInstruction;
/* }}} */
typedef struct _FastInstructionFlow { /* {{{ */
  int number;
  FastInstruction *instr;
  void *mallocedInstr;
} FastInstructionFlow;
/* }}} */
typedef struct _ExternalFunctionStruct { /* {{{ */
  GoomSL_ExternalFunction function;
  GoomHash *vars;
  int is_extern;
} ExternalFunctionStruct;
/* }}} */
typedef struct _Block {
  int    data;
  int    size;
} Block;
typedef struct _GSL_StructField { /* {{{ */
  int  type;
  char name[256];
  int  offsetInStruct; /* Where this field is stored... */
} GSL_StructField;
 /* }}} */
typedef struct _GSL_Struct { /* {{{ */
  int nbFields;
  GSL_StructField *fields[64];
  int size;
  Block iBlock[64];
  Block fBlock[64];
} GSL_Struct;
 /* }}} */
struct _GoomSL { /* {{{ */
    int num_lines;
    Instruction *instr;     /* instruction en cours de construction */

    InstructionFlow     *iflow;  /* flow d'instruction 'normal' */
    FastInstructionFlow *fastiflow; /* flow d'instruction optimise */
    
    GoomHash *vars;         /* table de variables */
    int currentNS;
    GoomHash *namespaces[16];
    
    GoomHash *functions;    /* table des fonctions externes */

    GoomHeap *data_heap; /* GSL Heap-like memory space */
    
    int nbStructID;
    GoomHash   *structIDS;
    GSL_Struct **gsl_struct;
    int gsl_struct_size;
    
    int    nbPtr;
    int    ptrArraySize;
    void **ptrArray;
    
    int compilationOK;
#ifdef USE_JITC_X86
    JitcX86Env *jitc;
    JitcFunc    jitc_func;
#endif
}; /* }}} */

extern GoomSL *currentGoomSL;

Instruction *gsl_instr_init(GoomSL *parent, const char *name, int id, int nb_param, int line_number);
void gsl_instr_add_param(Instruction *_this, char *param, int type);
void gsl_instr_set_namespace(Instruction *_this, GoomHash *ns);

void gsl_declare_task(const char *name);
void gsl_declare_external_task(const char *name);

int gsl_type_of_var(GoomHash *namespace, const char *name);

void gsl_enternamespace(const char *name);
void gsl_reenternamespace(GoomHash *ns);
GoomHash *gsl_leavenamespace(void);
GoomHash *gsl_find_namespace(const char *name);

void gsl_commit_compilation(void);

/* #define TYPE_PARAM    1 */

#define FIRST_RESERVED 0x80000

#define TYPE_INTEGER  0x90001
#define TYPE_FLOAT    0x90002
#define TYPE_VAR      0x90003
#define TYPE_PTR      0x90004
#define TYPE_LABEL    0x90005

#define TYPE_OP_EQUAL 6
#define TYPE_IVAR     0xa0001
#define TYPE_FVAR     0xa0002
#define TYPE_PVAR     0xa0003
#define TYPE_SVAR     0xa0004

#define INSTR_JUMP     6
#define INSTR_JZERO    29
#define INSTR_CALL     36 
#define INSTR_RET      37
#define INSTR_EXT_CALL 38
#define INSTR_JNZERO   40

#define INSTR_SET     0x80001
#define INSTR_INT     0x80002
#define INSTR_FLOAT   0x80003
#define INSTR_PTR     0x80004
#define INSTR_LABEL   0x80005
#define INSTR_ISLOWER 0x80006
#define INSTR_ADD     0x80007
#define INSTR_MUL     0x80008
#define INSTR_DIV     0x80009
#define INSTR_SUB     0x80010
#define INSTR_ISEQUAL 0x80011
#define INSTR_NOT     0x80012


#endif
