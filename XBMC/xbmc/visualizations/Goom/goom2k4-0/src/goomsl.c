#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "goomsl.h"
#include "goomsl_private.h"
#include "goomsl_yacc.h"

/*#define TRACE_SCRIPT*/

 /* {{{ definition of the instructions number */
#define INSTR_SETI_VAR_INTEGER     1
#define INSTR_SETI_VAR_VAR         2
#define INSTR_SETF_VAR_FLOAT       3
#define INSTR_SETF_VAR_VAR         4
#define INSTR_NOP                  5
/* #define INSTR_JUMP              6 */
#define INSTR_SETP_VAR_PTR         7
#define INSTR_SETP_VAR_VAR         8
#define INSTR_SUBI_VAR_INTEGER     9
#define INSTR_SUBI_VAR_VAR         10
#define INSTR_SUBF_VAR_FLOAT       11
#define INSTR_SUBF_VAR_VAR         12
#define INSTR_ISLOWERF_VAR_VAR     13
#define INSTR_ISLOWERF_VAR_FLOAT   14
#define INSTR_ISLOWERI_VAR_VAR     15
#define INSTR_ISLOWERI_VAR_INTEGER 16
#define INSTR_ADDI_VAR_INTEGER     17
#define INSTR_ADDI_VAR_VAR         18
#define INSTR_ADDF_VAR_FLOAT       19
#define INSTR_ADDF_VAR_VAR         20
#define INSTR_MULI_VAR_INTEGER     21
#define INSTR_MULI_VAR_VAR         22
#define INSTR_MULF_VAR_FLOAT       23
#define INSTR_MULF_VAR_VAR         24
#define INSTR_DIVI_VAR_INTEGER     25
#define INSTR_DIVI_VAR_VAR         26
#define INSTR_DIVF_VAR_FLOAT       27
#define INSTR_DIVF_VAR_VAR         28
/* #define INSTR_JZERO             29 */
#define INSTR_ISEQUALP_VAR_VAR     30
#define INSTR_ISEQUALP_VAR_PTR     31
#define INSTR_ISEQUALI_VAR_VAR     32
#define INSTR_ISEQUALI_VAR_INTEGER 33
#define INSTR_ISEQUALF_VAR_VAR     34
#define INSTR_ISEQUALF_VAR_FLOAT   35
/* #define INSTR_CALL              36 */
/* #define INSTR_RET               37 */
/* #define INSTR_EXT_CALL          38 */
#define INSTR_NOT_VAR              39
/* #define INSTR_JNZERO            40 */
#define  INSTR_SETS_VAR_VAR        41
#define  INSTR_ISEQUALS_VAR_VAR    42
#define  INSTR_ADDS_VAR_VAR        43
#define  INSTR_SUBS_VAR_VAR        44
#define  INSTR_MULS_VAR_VAR        45
#define  INSTR_DIVS_VAR_VAR        46

 /* }}} */
/* {{{ definition of the validation error types */
static const char *VALIDATE_OK = "ok"; 
#define VALIDATE_ERROR "error while validating "
#define VALIDATE_TODO "todo"
#define VALIDATE_SYNTHAX_ERROR "synthax error"
#define VALIDATE_NO_SUCH_INT "no such integer variable"
#define VALIDATE_NO_SUCH_VAR "no such variable"
#define VALIDATE_NO_SUCH_DEST_VAR "no such destination variable"
#define VALIDATE_NO_SUCH_SRC_VAR "no such src variable"
/* }}} */

  /***********************************/
 /* PROTOTYPE OF INTERNAL FUNCTIONS */
/***********************************/

/* {{{ */
static void gsl_instr_free(Instruction *_this);
static const char *gsl_instr_validate(Instruction *_this);
static void gsl_instr_display(Instruction *_this);

static InstructionFlow *iflow_new(void);
static void iflow_add_instr(InstructionFlow *_this, Instruction *instr);
static void iflow_clean(InstructionFlow *_this);
static void iflow_free(InstructionFlow *_this);
static void iflow_execute(FastInstructionFlow *_this, GoomSL *gsl);
/* }}} */

  /************************************/
 /* DEFINITION OF INTERNAL FUNCTIONS */
/************************************/

void iflow_free(InstructionFlow *_this)
{ /* {{{ */
  free(_this->instr);
  goom_hash_free(_this->labels);
  free(_this); /*TODO: finir cette fonction */
} /* }}} */

void iflow_clean(InstructionFlow *_this)
{ /* {{{ */
  /* TODO: clean chaque instruction du flot */
  _this->number = 0;
  goom_hash_free(_this->labels);
  _this->labels = goom_hash_new();
} /* }}} */

InstructionFlow *iflow_new(void)
{ /* {{{ */
  InstructionFlow *_this = (InstructionFlow*)malloc(sizeof(InstructionFlow));
  _this->number = 0;
  _this->tabsize = 6;
  _this->instr = (Instruction**)malloc(_this->tabsize * sizeof(Instruction*));
  _this->labels = goom_hash_new();

  return _this;
} /* }}} */

void iflow_add_instr(InstructionFlow *_this, Instruction *instr)
{ /* {{{ */
  if (_this->number == _this->tabsize) {
    _this->tabsize *= 2;
    _this->instr = (Instruction**)realloc(_this->instr, _this->tabsize * sizeof(Instruction*));
  }
  _this->instr[_this->number] = instr;
  instr->address = _this->number;
  _this->number++;
} /* }}} */

void gsl_instr_set_namespace(Instruction *_this, GoomHash *ns)
{ /* {{{ */
  if (_this->cur_param <= 0) {
    fprintf(stderr, "ERROR: Line %d, No more params to instructions\n", _this->line_number);
    exit(1);
  }
  _this->vnamespace[_this->cur_param-1] = ns;
} /* }}} */

void gsl_instr_add_param(Instruction *instr, char *param, int type)
{ /* {{{ */
  int len;
  if (instr==NULL)
    return;
  if (instr->cur_param==0)
    return;
  --instr->cur_param;
  len = strlen(param);
  instr->params[instr->cur_param] = (char*)malloc(len+1);
  strcpy(instr->params[instr->cur_param], param);
  instr->types[instr->cur_param] = type;
  if (instr->cur_param == 0) {

    const char *result = gsl_instr_validate(instr);
    if (result != VALIDATE_OK) {
      printf("ERROR: Line %d: ", instr->parent->num_lines + 1);
      gsl_instr_display(instr);
      printf("... %s\n", result);
      instr->parent->compilationOK = 0;
      exit(1);
    }

#if USE_JITC_X86
    iflow_add_instr(instr->parent->iflow, instr);
#else
    if (instr->id != INSTR_NOP)
      iflow_add_instr(instr->parent->iflow, instr);
    else
      gsl_instr_free(instr);
#endif
  }
} /* }}} */

Instruction *gsl_instr_init(GoomSL *parent, const char *name, int id, int nb_param, int line_number)
{ /* {{{ */
  Instruction *instr = (Instruction*)malloc(sizeof(Instruction));
  instr->params     = (char**)malloc(nb_param*sizeof(char*));
  instr->vnamespace = (GoomHash**)malloc(nb_param*sizeof(GoomHash*));
  instr->types = (int*)malloc(nb_param*sizeof(int));
  instr->cur_param = instr->nb_param = nb_param;
  instr->parent = parent;
  instr->id = id;
  instr->name = name;
  instr->jump_label = NULL;
  instr->line_number = line_number;
  return instr;
} /* }}} */

void gsl_instr_free(Instruction *_this)
{ /* {{{ */
  int i;
  free(_this->types);
  for (i=_this->cur_param; i<_this->nb_param; ++i)
    free(_this->params[i]);
  free(_this->params);
  free(_this);
} /* }}} */

void gsl_instr_display(Instruction *_this)
{ /* {{{ */
  int i=_this->nb_param-1;
  printf("%s", _this->name);
  while(i>=_this->cur_param) {
    printf(" %s", _this->params[i]);
    --i;
  }
} /* }}} */

  /****************************************/
 /* VALIDATION OF INSTRUCTION PARAMETERS */
/****************************************/

static const char *validate_v_v(Instruction *_this)
{ /* {{{ */
  HashValue *dest = goom_hash_get(_this->vnamespace[1], _this->params[1]);
  HashValue *src  = goom_hash_get(_this->vnamespace[0], _this->params[0]);

  if (dest == NULL) {
    return VALIDATE_NO_SUCH_DEST_VAR;
  }
  if (src == NULL) {
    return VALIDATE_NO_SUCH_SRC_VAR;
  }
  _this->data.udest.var = dest->ptr;
  _this->data.usrc.var  = src->ptr;
  return VALIDATE_OK;
} /* }}} */

static const char *validate_v_i(Instruction *_this)
{ /* {{{ */
  HashValue *dest            = goom_hash_get(_this->vnamespace[1], _this->params[1]);
  _this->data.usrc.value_int = strtol(_this->params[0],NULL,0);

  if (dest == NULL) {
    return VALIDATE_NO_SUCH_INT;
  }
  _this->data.udest.var = dest->ptr;
  return VALIDATE_OK;
} /* }}} */

static const char *validate_v_p(Instruction *_this)
{ /* {{{ */
  HashValue *dest            = goom_hash_get(_this->vnamespace[1], _this->params[1]);
  _this->data.usrc.value_ptr = strtol(_this->params[0],NULL,0);

  if (dest == NULL) {
    return VALIDATE_NO_SUCH_INT;
  }
  _this->data.udest.var = dest->ptr;
  return VALIDATE_OK;
} /* }}} */

static const char *validate_v_f(Instruction *_this)
{ /* {{{ */
  HashValue *dest            = goom_hash_get(_this->vnamespace[1], _this->params[1]);
  _this->data.usrc.value_float = atof(_this->params[0]);

  if (dest == NULL) {
    return VALIDATE_NO_SUCH_VAR;
  }
  _this->data.udest.var = dest->ptr;
  return VALIDATE_OK;
} /* }}} */

static const char *validate(Instruction *_this,
                            int vf_f_id, int vf_v_id,
                            int vi_i_id, int vi_v_id,
                            int vp_p_id, int vp_v_id,
                            int vs_v_id)
{ /* {{{ */
  if ((_this->types[1] == TYPE_FVAR) && (_this->types[0] == TYPE_FLOAT)) {
    _this->id = vf_f_id;
    return validate_v_f(_this);
  }
  else if ((_this->types[1] == TYPE_FVAR) && (_this->types[0] == TYPE_FVAR)) {
    _this->id = vf_v_id;
    return validate_v_v(_this);
  }
  else if ((_this->types[1] == TYPE_IVAR) && (_this->types[0] == TYPE_INTEGER)) {
    _this->id = vi_i_id;
    return validate_v_i(_this);
  }
  else if ((_this->types[1] == TYPE_IVAR) && (_this->types[0] == TYPE_IVAR)) {
    _this->id = vi_v_id;
    return validate_v_v(_this);
  }
  else if ((_this->types[1] == TYPE_PVAR) && (_this->types[0] == TYPE_PTR)) {
    if (vp_p_id == INSTR_NOP) return VALIDATE_ERROR;
    _this->id = vp_p_id;
    return validate_v_p(_this);
  }
  else if ((_this->types[1] == TYPE_PVAR) && (_this->types[0] == TYPE_PVAR)) {
    _this->id = vp_v_id;
    if (vp_v_id == INSTR_NOP) return VALIDATE_ERROR;
    return validate_v_v(_this);
  }
  else if ((_this->types[1] < FIRST_RESERVED) && (_this->types[1] >= 0) && (_this->types[0] == _this->types[1])) {
    _this->id = vs_v_id;
    if (vs_v_id == INSTR_NOP) return "Impossible operation to perform between two structs";
    return validate_v_v(_this);
  }
  return VALIDATE_ERROR;
} /* }}} */

const char *gsl_instr_validate(Instruction *_this)
{ /* {{{ */
  if (_this->id != INSTR_EXT_CALL) {
    int i = _this->nb_param;
    while (i>0)
    {
      i--;
      if (_this->types[i] == TYPE_VAR) {
        int type = gsl_type_of_var(_this->vnamespace[i], _this->params[i]);

        if (type == INSTR_INT)
          _this->types[i] = TYPE_IVAR;
        else if (type == INSTR_FLOAT)
          _this->types[i] = TYPE_FVAR;
        else if (type == INSTR_PTR)
          _this->types[i] = TYPE_PVAR;
        else if ((type >= 0) && (type < FIRST_RESERVED))
          _this->types[i] = type;
        else fprintf(stderr,"WARNING: Line %d, %s has no namespace\n", _this->line_number, _this->params[i]);
      }
    }
  }

  switch (_this->id) {

    /* set */ 
    case INSTR_SET:
      return validate(_this,
          INSTR_SETF_VAR_FLOAT, INSTR_SETF_VAR_VAR,
          INSTR_SETI_VAR_INTEGER, INSTR_SETI_VAR_VAR,
          INSTR_SETP_VAR_PTR, INSTR_SETP_VAR_VAR,
          INSTR_SETS_VAR_VAR);

      /* extcall */
    case INSTR_EXT_CALL:
      if (_this->types[0] == TYPE_VAR) {
        HashValue *fval = goom_hash_get(_this->parent->functions, _this->params[0]);
        if (fval) {
          _this->data.udest.external_function = (struct _ExternalFunctionStruct*)fval->ptr;
          return VALIDATE_OK;
        }
      }
      return VALIDATE_ERROR;

      /* call */
    case INSTR_CALL:
      if (_this->types[0] == TYPE_LABEL) {
        _this->jump_label = _this->params[0];
        return VALIDATE_OK;
      }
      return VALIDATE_ERROR;

      /* ret */
    case INSTR_RET:
      return VALIDATE_OK;

      /* jump */
    case INSTR_JUMP:

      if (_this->types[0] == TYPE_LABEL) {
        _this->jump_label = _this->params[0];
        return VALIDATE_OK;
      }
      return VALIDATE_ERROR;

      /* jzero / jnzero */
    case INSTR_JZERO:
    case INSTR_JNZERO:

      if (_this->types[0] == TYPE_LABEL) {
        _this->jump_label = _this->params[0];
        return VALIDATE_OK;
      }
      return VALIDATE_ERROR;

      /* label */
    case INSTR_LABEL:

      if (_this->types[0] == TYPE_LABEL) {
        _this->id = INSTR_NOP;
        _this->nop_label = _this->params[0];
        goom_hash_put_int(_this->parent->iflow->labels, _this->params[0], _this->parent->iflow->number);
        return VALIDATE_OK;
      }
      return VALIDATE_ERROR;

      /* isequal */
    case INSTR_ISEQUAL:
      return validate(_this,
          INSTR_ISEQUALF_VAR_FLOAT, INSTR_ISEQUALF_VAR_VAR,
          INSTR_ISEQUALI_VAR_INTEGER, INSTR_ISEQUALI_VAR_VAR,
          INSTR_ISEQUALP_VAR_PTR, INSTR_ISEQUALP_VAR_VAR,
          INSTR_ISEQUALS_VAR_VAR);

      /* not */
    case INSTR_NOT:
      _this->id = INSTR_NOT_VAR;
      return VALIDATE_OK;

      /* islower */
    case INSTR_ISLOWER:
      return validate(_this,
          INSTR_ISLOWERF_VAR_FLOAT, INSTR_ISLOWERF_VAR_VAR,
          INSTR_ISLOWERI_VAR_INTEGER, INSTR_ISLOWERI_VAR_VAR,
          INSTR_NOP, INSTR_NOP, INSTR_NOP);

      /* add */
    case INSTR_ADD:
      return validate(_this,
          INSTR_ADDF_VAR_FLOAT, INSTR_ADDF_VAR_VAR,
          INSTR_ADDI_VAR_INTEGER, INSTR_ADDI_VAR_VAR,
          INSTR_NOP, INSTR_NOP,
          INSTR_ADDS_VAR_VAR);

      /* mul */
    case INSTR_MUL:
      return validate(_this,
          INSTR_MULF_VAR_FLOAT, INSTR_MULF_VAR_VAR,
          INSTR_MULI_VAR_INTEGER, INSTR_MULI_VAR_VAR,
          INSTR_NOP, INSTR_NOP,
          INSTR_MULS_VAR_VAR);

      /* sub */
    case INSTR_SUB:
      return validate(_this,
          INSTR_SUBF_VAR_FLOAT, INSTR_SUBF_VAR_VAR,
          INSTR_SUBI_VAR_INTEGER, INSTR_SUBI_VAR_VAR,
          INSTR_NOP, INSTR_NOP,
          INSTR_SUBS_VAR_VAR);

      /* div */
    case INSTR_DIV:
      return validate(_this,
          INSTR_DIVF_VAR_FLOAT, INSTR_DIVF_VAR_VAR,
          INSTR_DIVI_VAR_INTEGER, INSTR_DIVI_VAR_VAR,
          INSTR_NOP,INSTR_NOP,
          INSTR_DIVS_VAR_VAR);

    default:
      return VALIDATE_TODO;
  }
  return VALIDATE_ERROR;
} /* }}} */

  /*************/
 /* EXECUTION */
/*************/
void iflow_execute(FastInstructionFlow *_this, GoomSL *gsl)
{ /* {{{ */
  int flag = 0;
  int ip = 0;
  FastInstruction *instr = _this->instr;
  int stack[0x10000];
  int stack_pointer = 0;

  stack[stack_pointer++] = -1;

  /* Quelques Macro pour rendre le code plus lisible */
#define pSRC_VAR        instr[ip].data.usrc.var
#define SRC_VAR_INT    *instr[ip].data.usrc.var_int
#define SRC_VAR_FLOAT  *instr[ip].data.usrc.var_float
#define SRC_VAR_PTR    *instr[ip].data.usrc.var_ptr

#define pDEST_VAR       instr[ip].data.udest.var
#define DEST_VAR_INT   *instr[ip].data.udest.var_int
#define DEST_VAR_FLOAT *instr[ip].data.udest.var_float
#define DEST_VAR_PTR   *instr[ip].data.udest.var_ptr

#define VALUE_INT       instr[ip].data.usrc.value_int
#define VALUE_FLOAT     instr[ip].data.usrc.value_float
#define VALUE_PTR       instr[ip].data.usrc.value_ptr

#define JUMP_OFFSET     instr[ip].data.udest.jump_offset

#define SRC_STRUCT_ID  instr[ip].data.usrc.var_int[-1]
#define DEST_STRUCT_ID instr[ip].data.udest.var_int[-1]
#define SRC_STRUCT_IBLOCK(i)  gsl->gsl_struct[SRC_STRUCT_ID]->iBlock[i]
#define SRC_STRUCT_FBLOCK(i)  gsl->gsl_struct[SRC_STRUCT_ID]->fBlock[i]
#define DEST_STRUCT_IBLOCK(i) gsl->gsl_struct[DEST_STRUCT_ID]->iBlock[i]
#define DEST_STRUCT_FBLOCK(i) gsl->gsl_struct[DEST_STRUCT_ID]->fBlock[i]
#define DEST_STRUCT_IBLOCK_VAR(i,j) \
  ((int*)((char*)pDEST_VAR   + gsl->gsl_struct[DEST_STRUCT_ID]->iBlock[i].data))[j]
#define DEST_STRUCT_FBLOCK_VAR(i,j) \
  ((float*)((char*)pDEST_VAR + gsl->gsl_struct[DEST_STRUCT_ID]->fBlock[i].data))[j]
#define SRC_STRUCT_IBLOCK_VAR(i,j) \
  ((int*)((char*)pSRC_VAR    + gsl->gsl_struct[SRC_STRUCT_ID]->iBlock[i].data))[j]
#define SRC_STRUCT_FBLOCK_VAR(i,j) \
  ((float*)((char*)pSRC_VAR  + gsl->gsl_struct[SRC_STRUCT_ID]->fBlock[i].data))[j]
#define DEST_STRUCT_SIZE      gsl->gsl_struct[DEST_STRUCT_ID]->size

  while (1)
  {
    int i;
#ifdef TRACE_SCRIPT 
    printf("execute "); gsl_instr_display(instr[ip].proto); printf("\n");
#endif
    switch (instr[ip].id) {

      /* SET.I */
      case INSTR_SETI_VAR_INTEGER:
        DEST_VAR_INT = VALUE_INT;
        ++ip; break;

      case INSTR_SETI_VAR_VAR:
        DEST_VAR_INT = SRC_VAR_INT;
        ++ip; break;

        /* SET.F */
      case INSTR_SETF_VAR_FLOAT:
        DEST_VAR_FLOAT = VALUE_FLOAT;
        ++ip; break;

      case INSTR_SETF_VAR_VAR:
        DEST_VAR_FLOAT = SRC_VAR_FLOAT;
        ++ip; break;

        /* SET.P */
      case INSTR_SETP_VAR_VAR:
        DEST_VAR_PTR = SRC_VAR_PTR;
        ++ip; break;

      case INSTR_SETP_VAR_PTR:
        DEST_VAR_PTR = VALUE_PTR;
        ++ip; break;

        /* JUMP */
      case INSTR_JUMP:
        ip += JUMP_OFFSET; break;

        /* JZERO */
      case INSTR_JZERO:
        ip += (flag ? 1 : JUMP_OFFSET); break;

      case INSTR_NOP:
        ++ip; break;

        /* ISEQUAL.P */
      case INSTR_ISEQUALP_VAR_VAR:
        flag = (DEST_VAR_PTR == SRC_VAR_PTR);
        ++ip; break;

      case INSTR_ISEQUALP_VAR_PTR:
        flag = (DEST_VAR_PTR == VALUE_PTR);
        ++ip; break;

        /* ISEQUAL.I */
      case INSTR_ISEQUALI_VAR_VAR:
        flag = (DEST_VAR_INT == SRC_VAR_INT);
        ++ip; break;

      case INSTR_ISEQUALI_VAR_INTEGER:
        flag = (DEST_VAR_INT == VALUE_INT);
        ++ip; break;

        /* ISEQUAL.F */
      case INSTR_ISEQUALF_VAR_VAR:
        flag = (DEST_VAR_FLOAT == SRC_VAR_FLOAT);
        ++ip; break;

      case INSTR_ISEQUALF_VAR_FLOAT:
        flag = (DEST_VAR_FLOAT ==  VALUE_FLOAT);
        ++ip; break;

        /* ISLOWER.I */
      case INSTR_ISLOWERI_VAR_VAR:
        flag = (DEST_VAR_INT < SRC_VAR_INT);
        ++ip; break;

      case INSTR_ISLOWERI_VAR_INTEGER:
        flag = (DEST_VAR_INT <  VALUE_INT);
        ++ip; break;

        /* ISLOWER.F */
      case INSTR_ISLOWERF_VAR_VAR:
        flag = (DEST_VAR_FLOAT < SRC_VAR_FLOAT);
        ++ip; break;

      case INSTR_ISLOWERF_VAR_FLOAT:
        flag = (DEST_VAR_FLOAT <  VALUE_FLOAT);
        ++ip; break;

        /* ADD.I */
      case INSTR_ADDI_VAR_VAR:
        DEST_VAR_INT += SRC_VAR_INT;
        ++ip; break;

      case INSTR_ADDI_VAR_INTEGER:
        DEST_VAR_INT += VALUE_INT;
        ++ip; break;

        /* ADD.F */
      case INSTR_ADDF_VAR_VAR:
        DEST_VAR_FLOAT += SRC_VAR_FLOAT;
        ++ip; break;

      case INSTR_ADDF_VAR_FLOAT:
        DEST_VAR_FLOAT += VALUE_FLOAT;
        ++ip; break;

        /* MUL.I */
      case INSTR_MULI_VAR_VAR:
        DEST_VAR_INT *= SRC_VAR_INT;
        ++ip; break;

      case INSTR_MULI_VAR_INTEGER:
        DEST_VAR_INT *= VALUE_INT;
        ++ip; break;

        /* MUL.F */
      case INSTR_MULF_VAR_FLOAT:
        DEST_VAR_FLOAT *= VALUE_FLOAT;
        ++ip; break;

      case INSTR_MULF_VAR_VAR:
        DEST_VAR_FLOAT *= SRC_VAR_FLOAT;
        ++ip; break;

        /* DIV.I */
      case INSTR_DIVI_VAR_VAR:
        DEST_VAR_INT /= SRC_VAR_INT;
        ++ip; break;

      case INSTR_DIVI_VAR_INTEGER:
        DEST_VAR_INT /= VALUE_INT;
        ++ip; break;

        /* DIV.F */
      case INSTR_DIVF_VAR_FLOAT:
        DEST_VAR_FLOAT /= VALUE_FLOAT;
        ++ip; break;

      case INSTR_DIVF_VAR_VAR:
        DEST_VAR_FLOAT /= SRC_VAR_FLOAT;
        ++ip; break;

        /* SUB.I */
      case INSTR_SUBI_VAR_VAR:
        DEST_VAR_INT -= SRC_VAR_INT;
        ++ip; break;

      case INSTR_SUBI_VAR_INTEGER:
        DEST_VAR_INT -= VALUE_INT;
        ++ip; break;

        /* SUB.F */
      case INSTR_SUBF_VAR_FLOAT:
        DEST_VAR_FLOAT -= VALUE_FLOAT;
        ++ip; break;

      case INSTR_SUBF_VAR_VAR:
        DEST_VAR_FLOAT -= SRC_VAR_FLOAT;
        ++ip; break;

        /* CALL */
      case INSTR_CALL:
        stack[stack_pointer++] = ip + 1;
        ip += JUMP_OFFSET; break;

        /* RET */
      case INSTR_RET:
        ip = stack[--stack_pointer];
        if (ip<0) return;
        break;

        /* EXT_CALL */
      case INSTR_EXT_CALL:
        instr[ip].data.udest.external_function->function(gsl, gsl->vars, instr[ip].data.udest.external_function->vars);
        ++ip; break;

        /* NOT */
      case INSTR_NOT_VAR:
        flag = !flag;
        ++ip; break;

        /* JNZERO */
      case INSTR_JNZERO:
        ip += (flag ? JUMP_OFFSET : 1); break;

      case INSTR_SETS_VAR_VAR:
        memcpy(pDEST_VAR, pSRC_VAR, DEST_STRUCT_SIZE);
        ++ip; break;

      case INSTR_ISEQUALS_VAR_VAR:
        break;

      case INSTR_ADDS_VAR_VAR:
        /* process integers */
        i=0;
        while (DEST_STRUCT_IBLOCK(i).size > 0) {
          int j=DEST_STRUCT_IBLOCK(i).size;
          while (j--) {
            DEST_STRUCT_IBLOCK_VAR(i,j) += SRC_STRUCT_IBLOCK_VAR(i,j);
          }
          ++i;
        }
        /* process floats */
        i=0;
        while (DEST_STRUCT_FBLOCK(i).size > 0) {
          int j=DEST_STRUCT_FBLOCK(i).size;
          while (j--) {
            DEST_STRUCT_FBLOCK_VAR(i,j) += SRC_STRUCT_FBLOCK_VAR(i,j);
          }
          ++i;
        }
        ++ip; break;

      case INSTR_SUBS_VAR_VAR:
        /* process integers */
        i=0;
        while (DEST_STRUCT_IBLOCK(i).size > 0) {
          int j=DEST_STRUCT_IBLOCK(i).size;
          while (j--) {
            DEST_STRUCT_IBLOCK_VAR(i,j) -= SRC_STRUCT_IBLOCK_VAR(i,j);
          }
          ++i;
        }
        /* process floats */
        i=0;
        while (DEST_STRUCT_FBLOCK(i).size > 0) {
          int j=DEST_STRUCT_FBLOCK(i).size;
          while (j--) {
            DEST_STRUCT_FBLOCK_VAR(i,j) -= SRC_STRUCT_FBLOCK_VAR(i,j);
          }
          ++i;
        }
        ++ip; break;

      case INSTR_MULS_VAR_VAR:
        /* process integers */
        i=0;
        while (DEST_STRUCT_IBLOCK(i).size > 0) {
          int j=DEST_STRUCT_IBLOCK(i).size;
          while (j--) {
            DEST_STRUCT_IBLOCK_VAR(i,j) *= SRC_STRUCT_IBLOCK_VAR(i,j);
          }
          ++i;
        }
        /* process floats */
        i=0;
        while (DEST_STRUCT_FBLOCK(i).size > 0) {
          int j=DEST_STRUCT_FBLOCK(i).size;
          while (j--) {
            DEST_STRUCT_FBLOCK_VAR(i,j) *= SRC_STRUCT_FBLOCK_VAR(i,j);
          }
          ++i;
        }
        ++ip; break;
        
      case INSTR_DIVS_VAR_VAR:
        /* process integers */
        i=0;
        while (DEST_STRUCT_IBLOCK(i).size > 0) {
          int j=DEST_STRUCT_IBLOCK(i).size;
          while (j--) {
            DEST_STRUCT_IBLOCK_VAR(i,j) /= SRC_STRUCT_IBLOCK_VAR(i,j);
          }
          ++i;
        }
        /* process floats */
        i=0;
        while (DEST_STRUCT_FBLOCK(i).size > 0) {
          int j=DEST_STRUCT_FBLOCK(i).size;
          while (j--) {
            DEST_STRUCT_FBLOCK_VAR(i,j) /= SRC_STRUCT_FBLOCK_VAR(i,j);
          }
          ++i;
        }
        ++ip; break;

      default:
        printf("NOT IMPLEMENTED : %d\n", instr[ip].id);
        ++ip;
        exit(1);
    }
  }
} /* }}} */

int gsl_malloc(GoomSL *_this, int size)
{ /* {{{ */
  if (_this->nbPtr >= _this->ptrArraySize) {
    _this->ptrArraySize *= 2;
    _this->ptrArray = (void**)realloc(_this->ptrArray, sizeof(void*) * _this->ptrArraySize);
  }
  _this->ptrArray[_this->nbPtr] = malloc(size);
  return _this->nbPtr++;
} /* }}} */

void *gsl_get_ptr(GoomSL *_this, int id)
{ /* {{{ */
  if ((id>=0)&&(id<_this->nbPtr))
    return _this->ptrArray[id];
  fprintf(stderr,"INVALID GET PTR 0x%08x\n", id);
  return NULL;
} /* }}} */

void gsl_free_ptr(GoomSL *_this, int id)
{ /* {{{ */
  if ((id>=0)&&(id<_this->nbPtr)) {
    free(_this->ptrArray[id]);
    _this->ptrArray[id] = 0;
  }
} /* }}} */

void gsl_enternamespace(const char *name)
{ /* {{{ */
  HashValue *val = goom_hash_get(currentGoomSL->functions, name);
  if (val) {
    ExternalFunctionStruct *function = (ExternalFunctionStruct*)val->ptr;
    currentGoomSL->currentNS++;
    currentGoomSL->namespaces[currentGoomSL->currentNS] = function->vars;
  }
  else {
    fprintf(stderr, "ERROR: Line %d, Could not find namespace: %s\n", currentGoomSL->num_lines, name);
    exit(1);
  }
} /* }}} */

void gsl_reenternamespace(GoomHash *nsinfo) {
  currentGoomSL->currentNS++;
  currentGoomSL->namespaces[currentGoomSL->currentNS] = nsinfo;
}

GoomHash *gsl_leavenamespace(void)
{ /* {{{ */
  currentGoomSL->currentNS--;
  return currentGoomSL->namespaces[currentGoomSL->currentNS+1];
} /* }}} */

GoomHash *gsl_find_namespace(const char *name)
{ /* {{{ */
  int i;
  for (i=currentGoomSL->currentNS;i>=0;--i) {
    if (goom_hash_get(currentGoomSL->namespaces[i], name))
      return currentGoomSL->namespaces[i];
  }
  return NULL;
} /* }}} */

void gsl_declare_task(const char *name)
{ /* {{{ */
  if (goom_hash_get(currentGoomSL->functions, name)) {
    return;
  }
  else {
    ExternalFunctionStruct *gef = (ExternalFunctionStruct*)malloc(sizeof(ExternalFunctionStruct));
    gef->function = 0;
    gef->vars = goom_hash_new();
    gef->is_extern = 0;
    goom_hash_put_ptr(currentGoomSL->functions, name, (void*)gef);
  }
} /* }}} */

void gsl_declare_external_task(const char *name)
{ /* {{{ */
  if (goom_hash_get(currentGoomSL->functions, name)) {
    fprintf(stderr, "ERROR: Line %d, Duplicate declaration of %s\n", currentGoomSL->num_lines, name);
    return;
  }
  else {
    ExternalFunctionStruct *gef = (ExternalFunctionStruct*)malloc(sizeof(ExternalFunctionStruct));
    gef->function = 0;
    gef->vars = goom_hash_new();
    gef->is_extern = 1;
    goom_hash_put_ptr(currentGoomSL->functions, name, (void*)gef);
  }
} /* }}} */

static void reset_scanner(GoomSL *gss)
{ /* {{{ */
  gss->num_lines = 0;
  gss->instr = NULL;
  iflow_clean(gss->iflow);

  /* reset variables */
  goom_hash_free(gss->vars);
  gss->vars = goom_hash_new();
  gss->currentNS = 0;
  gss->namespaces[0] = gss->vars;

  goom_hash_free(gss->structIDS);
  gss->structIDS  = goom_hash_new();
  
  while (gss->nbStructID > 0) {
    int i;
    gss->nbStructID--;
    for(i=0;i<gss->gsl_struct[gss->nbStructID]->nbFields;++i)
      free(gss->gsl_struct[gss->nbStructID]->fields[i]);
    free(gss->gsl_struct[gss->nbStructID]);
  }

  gss->compilationOK = 1;

  goom_heap_delete(gss->data_heap);
  gss->data_heap = goom_heap_new();
} /* }}} */

static void calculate_labels(InstructionFlow *iflow)
{ /* {{{ */
  int i = 0;
  while (i < iflow->number) {
    Instruction *instr = iflow->instr[i];
    if (instr->jump_label) {
      HashValue *label = goom_hash_get(iflow->labels,instr->jump_label);
      if (label) {
        instr->data.udest.jump_offset = -instr->address + label->i;
      }
      else {
        fprintf(stderr, "ERROR: Line %d, Could not find label %s\n", instr->line_number, instr->jump_label);
        instr->id = INSTR_NOP;
        instr->nop_label = 0;
        exit(1);
      }
    }
    ++i;
  }
} /* }}} */

static int powerOfTwo(int i)
{
  int b;
  for (b=0;b<31;b++)
    if (i == (1<<b))
      return b;
  return 0;
}

/* Cree un flow d'instruction optimise */
static void gsl_create_fast_iflow(void)
{ /* {{{ */
  int number = currentGoomSL->iflow->number;
  int i;
#ifdef USE_JITC_X86

  /* pour compatibilite avec les MACROS servant a execution */
  int ip = 0;
  GoomSL *gsl = currentGoomSL;

  JitcX86Env *jitc;

  if (currentGoomSL->jitc != NULL)
    jitc_x86_delete(currentGoomSL->jitc);
  jitc = currentGoomSL->jitc = jitc_x86_env_new(0xffff);
  currentGoomSL->jitc_func = jitc_prepare_func(jitc);

#if 0  
#define SRC_STRUCT_ID  instr[ip].data.usrc.var_int[-1]
#define DEST_STRUCT_ID instr[ip].data.udest.var_int[-1]
#define SRC_STRUCT_IBLOCK(i)  gsl->gsl_struct[SRC_STRUCT_ID]->iBlock[i]
#define SRC_STRUCT_FBLOCK(i)  gsl->gsl_struct[SRC_STRUCT_ID]->fBlock[i]
#define DEST_STRUCT_IBLOCK(i) gsl->gsl_struct[DEST_STRUCT_ID]->iBlock[i]
#define DEST_STRUCT_FBLOCK(i) gsl->gsl_struct[DEST_STRUCT_ID]->fBlock[i]
#define DEST_STRUCT_IBLOCK_VAR(i,j) \
  ((int*)((char*)pDEST_VAR   + gsl->gsl_struct[DEST_STRUCT_ID]->iBlock[i].data))[j]
#define DEST_STRUCT_FBLOCK_VAR(i,j) \
  ((float*)((char*)pDEST_VAR + gsl->gsl_struct[DEST_STRUCT_ID]->fBlock[i].data))[j]
#define SRC_STRUCT_IBLOCK_VAR(i,j) \
  ((int*)((char*)pSRC_VAR    + gsl->gsl_struct[SRC_STRUCT_ID]->iBlock[i].data))[j]
#define SRC_STRUCT_FBLOCK_VAR(i,j) \
  ((float*)((char*)pSRC_VAR  + gsl->gsl_struct[SRC_STRUCT_ID]->fBlock[i].data))[j]
#define DEST_STRUCT_SIZE      gsl->gsl_struct[DEST_STRUCT_ID]->size
#endif

  JITC_JUMP_LABEL(jitc, "__very_end__");
  JITC_ADD_LABEL (jitc, "__very_start__");
  
  for (i=0;i<number;++i) {
    Instruction *instr = currentGoomSL->iflow->instr[i];
    switch (instr->id) {
      case INSTR_SETI_VAR_INTEGER     :
        jitc_add(jitc, "mov [$d], $d", instr->data.udest.var_int, instr->data.usrc.value_int);
        break;
      case INSTR_SETI_VAR_VAR         :
        jitc_add(jitc, "mov eax, [$d]", instr->data.usrc.var_int);
        jitc_add(jitc, "mov [$d], eax", instr->data.udest.var_int);
        break;
        /* SET.F */
      case INSTR_SETF_VAR_FLOAT       :
        jitc_add(jitc, "mov [$d], $d", instr->data.udest.var_float, *(int*)(&instr->data.usrc.value_float));
        break;
      case INSTR_SETF_VAR_VAR         :
        jitc_add(jitc, "mov eax, [$d]", instr->data.usrc.var_float);
        jitc_add(jitc, "mov [$d], eax", instr->data.udest.var_float);
        break;
      case INSTR_NOP                  :
        if (instr->nop_label != 0)
          JITC_ADD_LABEL(jitc, instr->nop_label);
        break;
      case INSTR_JUMP                 :
        JITC_JUMP_LABEL(jitc,instr->jump_label);
        break;
      case INSTR_SETP_VAR_PTR         :
        jitc_add(jitc, "mov [$d], $d", instr->data.udest.var_ptr, instr->data.usrc.value_ptr);
        break;
      case INSTR_SETP_VAR_VAR         :
        jitc_add(jitc, "mov eax, [$d]", instr->data.usrc.var_ptr);
        jitc_add(jitc, "mov [$d], eax", instr->data.udest.var_ptr);
        break;
      case INSTR_SUBI_VAR_INTEGER     :
        jitc_add(jitc, "add [$d],  $d", instr->data.udest.var_int, -instr->data.usrc.value_int);
        break;
      case INSTR_SUBI_VAR_VAR         :
        jitc_add(jitc, "mov eax, [$d]", instr->data.udest.var_int);
        jitc_add(jitc, "sub eax, [$d]", instr->data.usrc.var_int);
        jitc_add(jitc, "mov [$d], eax", instr->data.udest.var_int);
        break;
      case INSTR_SUBF_VAR_FLOAT       :
        printf("NOT IMPLEMENTED : %d\n", instr->id);
        break;
      case INSTR_SUBF_VAR_VAR         :
        printf("NOT IMPLEMENTED : %d\n", instr->id);
        break;
      case INSTR_ISLOWERF_VAR_VAR:
        printf("NOT IMPLEMENTED : %d\n", instr->id);
        break;
      case INSTR_ISLOWERF_VAR_FLOAT:
        printf("NOT IMPLEMENTED : %d\n", instr->id);
        break;
      case INSTR_ISLOWERI_VAR_VAR:
        jitc_add(jitc,"mov edx, [$d]", instr->data.udest.var_int);
        jitc_add(jitc,"sub edx, [$d]", instr->data.usrc.var_int);
        jitc_add(jitc,"shr edx, $d",   31);
        break;
      case INSTR_ISLOWERI_VAR_INTEGER:
        jitc_add(jitc,"mov edx, [$d]", instr->data.udest.var_int);
        jitc_add(jitc,"sub edx, $d", instr->data.usrc.value_int);
        jitc_add(jitc,"shr edx, $d",   31);
        break;
      case INSTR_ADDI_VAR_INTEGER:
        jitc_add(jitc, "add [$d],  $d", instr->data.udest.var_int, instr->data.usrc.value_int);
        break;
      case INSTR_ADDI_VAR_VAR:
        jitc_add(jitc, "mov eax, [$d]", instr->data.udest.var_int);
        jitc_add(jitc, "add eax, [$d]", instr->data.usrc.var_int);
        jitc_add(jitc, "mov [$d], eax", instr->data.udest.var_int);
        break;
      case INSTR_ADDF_VAR_FLOAT:
        printf("NOT IMPLEMENTED : %d\n", instr->id);
        break;
      case INSTR_ADDF_VAR_VAR:
        printf("NOT IMPLEMENTED : %d\n", instr->id);
        break;
      case INSTR_MULI_VAR_INTEGER:
        if (instr->data.usrc.value_int != 1)
        {
          int po2 = powerOfTwo(instr->data.usrc.value_int);
          if (po2) {
            /* performs (V / 2^n) by doing V >> n */
            jitc_add(jitc, "mov  eax, [$d]",  instr->data.udest.var_int);
            jitc_add(jitc, "sal  eax, $d",    po2);
            jitc_add(jitc, "mov  [$d], eax",  instr->data.udest.var_int);
          }
          else {
            jitc_add(jitc, "mov  eax, [$d]", instr->data.udest.var_int);
            jitc_add(jitc, "imul eax, $d",   instr->data.usrc.value_int);
            jitc_add(jitc, "mov  [$d], eax", instr->data.udest.var_int);
          }
        }
        break;
      case INSTR_MULI_VAR_VAR:
        jitc_add(jitc, "mov  eax,  [$d]", instr->data.udest.var_int);
        jitc_add(jitc, "imul eax,  [$d]", instr->data.usrc.var_int);
        jitc_add(jitc, "mov  [$d], eax",  instr->data.udest.var_int);
        break;
      case INSTR_MULF_VAR_FLOAT:
        printf("NOT IMPLEMENTED : %d\n", instr->id);
        break;
      case INSTR_MULF_VAR_VAR:
        printf("NOT IMPLEMENTED : %d\n", instr->id);
        break;
      case INSTR_DIVI_VAR_INTEGER:
        if ((instr->data.usrc.value_int != 1) && (instr->data.usrc.value_int != 0))
        {
          int po2 = powerOfTwo(instr->data.usrc.value_int);
          if (po2) {
            /* performs (V / 2^n) by doing V >> n */
            jitc_add(jitc, "mov  eax, [$d]",  instr->data.udest.var_int);
            jitc_add(jitc, "sar  eax, $d",    po2);
            jitc_add(jitc, "mov  [$d], eax",  instr->data.udest.var_int);
          }
          else {
            /* performs (V/n) by doing (V*(32^2/n)) */
            long   coef;
            double dcoef = (double)4294967296.0 / (double)instr->data.usrc.value_int;
            if (dcoef < 0.0) dcoef = -dcoef;
            coef   = (long)floor(dcoef);
            dcoef -= floor(dcoef);
            if (dcoef < 0.5) coef += 1;
            
            jitc_add(jitc, "mov  eax, [$d]", instr->data.udest.var_int);
            jitc_add(jitc, "mov  edx, $d",   coef);
            jitc_add(jitc, "imul edx");
            if (instr->data.usrc.value_int < 0)
              jitc_add(jitc, "neg edx");
            jitc_add(jitc, "mov [$d], edx", instr->data.udest.var_int);
          }
        }
        break;
      case INSTR_DIVI_VAR_VAR         :
        jitc_add(jitc, "mov  eax, [$d]", instr->data.udest.var_int);
        jitc_add(jitc, "cdq"); /* sign extend eax into edx */
        jitc_add(jitc, "idiv [$d]", instr->data.usrc.var_int);
        jitc_add(jitc, "mov [$d], eax", instr->data.udest.var_int);
        break;
      case INSTR_DIVF_VAR_FLOAT:
        printf("NOT IMPLEMENTED : %d\n", instr->id);
        break;
      case INSTR_DIVF_VAR_VAR:
        printf("NOT IMPLEMENTED : %d\n", instr->id);
        break;
      case INSTR_JZERO:
        jitc_add(jitc, "cmp edx, $d", 0);
        jitc_add(jitc, "je $s", instr->jump_label);
        break;
      case INSTR_ISEQUALP_VAR_VAR     :
        jitc_add(jitc, "mov eax, [$d]", instr->data.udest.var_ptr);
        jitc_add(jitc, "mov edx, $d",   0);
        jitc_add(jitc, "cmp eax, [$d]", instr->data.usrc.var_ptr);
        jitc_add(jitc, "jne $d",        1);
        jitc_add(jitc, "inc edx");
        break;
      case INSTR_ISEQUALP_VAR_PTR     :
        jitc_add(jitc, "mov eax, [$d]", instr->data.udest.var_ptr);
        jitc_add(jitc, "mov edx, $d",   0);
        jitc_add(jitc, "cmp eax, $d",   instr->data.usrc.value_ptr);
        jitc_add(jitc, "jne $d",        1);
        jitc_add(jitc, "inc edx");
        break;
      case INSTR_ISEQUALI_VAR_VAR     :
        jitc_add(jitc, "mov eax, [$d]", instr->data.udest.var_int);
        jitc_add(jitc, "mov edx, $d",   0);
        jitc_add(jitc, "cmp eax, [$d]", instr->data.usrc.var_int);
        jitc_add(jitc, "jne $d",        1);
        jitc_add(jitc, "inc edx");
        break;
      case INSTR_ISEQUALI_VAR_INTEGER :
        jitc_add(jitc, "mov eax, [$d]", instr->data.udest.var_int);
        jitc_add(jitc, "mov edx, $d",   0);
        jitc_add(jitc, "cmp eax, $d", instr->data.usrc.value_int);
        jitc_add(jitc, "jne $d",        1);
        jitc_add(jitc, "inc edx");
        break;
      case INSTR_ISEQUALF_VAR_VAR     :
        printf("NOT IMPLEMENTED : %d\n", instr->id);
        break;
      case INSTR_ISEQUALF_VAR_FLOAT   :
        printf("NOT IMPLEMENTED : %d\n", instr->id);
        break;
      case INSTR_CALL:
        jitc_add(jitc, "call $s", instr->jump_label);
        break;
      case INSTR_RET:
        jitc_add(jitc, "ret");
        break;
      case INSTR_EXT_CALL:
        jitc_add(jitc, "mov eax, [$d]", &(instr->data.udest.external_function->vars));
        jitc_add(jitc, "push eax");
        jitc_add(jitc, "mov edx, [$d]", &(currentGoomSL->vars));
        jitc_add(jitc, "push edx");
        jitc_add(jitc, "mov eax, [$d]", &(currentGoomSL));
        jitc_add(jitc, "push eax");

        jitc_add(jitc, "mov eax, [$d]",  &(instr->data.udest.external_function));
        jitc_add(jitc, "mov eax, [eax]");
        jitc_add(jitc, "call [eax]");
        jitc_add(jitc, "add esp, $d", 12);
        break;
      case INSTR_NOT_VAR:
        jitc_add(jitc, "mov eax, edx");
        jitc_add(jitc, "mov edx, $d", 1);
        jitc_add(jitc, "sub edx, eax");
        break;
      case INSTR_JNZERO:
        jitc_add(jitc, "cmp edx, $d", 0);
        jitc_add(jitc, "jne $s", instr->jump_label);
        break;
      case INSTR_SETS_VAR_VAR:
        {
          int loop = DEST_STRUCT_SIZE / sizeof(int);
          int dst  = (int)pDEST_VAR;
          int src  = (int)pSRC_VAR;
        
          while (loop--) {
            jitc_add(jitc,"mov eax, [$d]", src);
            jitc_add(jitc,"mov [$d], eax", dst);
            src += 4;
            dst += 4;
          }
        }
        break;
      case INSTR_ISEQUALS_VAR_VAR:
        break;
      case INSTR_ADDS_VAR_VAR:
        {
        /* process integers */
        int i=0;
        while (DEST_STRUCT_IBLOCK(i).size > 0) {
          int j=DEST_STRUCT_IBLOCK(i).size;
          while (j--) { /* TODO interlace 2 */
            jitc_add(jitc, "mov eax, [$d]", &DEST_STRUCT_IBLOCK_VAR(i,j));
            jitc_add(jitc, "add eax, [$d]", &SRC_STRUCT_IBLOCK_VAR(i,j));
            jitc_add(jitc, "mov [$d], eax", &DEST_STRUCT_IBLOCK_VAR(i,j));
          }
          ++i;
        }
        /* process floats */
        i=0;
        while (DEST_STRUCT_FBLOCK(i).size > 0) {
          int j=DEST_STRUCT_FBLOCK(i).size;
          while (j--) {
            /* DEST_STRUCT_FBLOCK_VAR(i,j) += SRC_STRUCT_FBLOCK_VAR(i,j); */
            /* TODO */
          }
          ++i;
        }
        break;
        }
      case INSTR_SUBS_VAR_VAR:
        {
        /* process integers */
        int i=0;
        while (DEST_STRUCT_IBLOCK(i).size > 0) {
          int j=DEST_STRUCT_IBLOCK(i).size;
          while (j--) {
            jitc_add(jitc, "mov eax, [$d]", &DEST_STRUCT_IBLOCK_VAR(i,j));
            jitc_add(jitc, "sub eax, [$d]", &SRC_STRUCT_IBLOCK_VAR(i,j));
            jitc_add(jitc, "mov [$d], eax", &DEST_STRUCT_IBLOCK_VAR(i,j));
          }
          ++i;
        }
        break;
        }
      case INSTR_MULS_VAR_VAR:
        {
        /* process integers */
        int i=0;
        while (DEST_STRUCT_IBLOCK(i).size > 0) {
          int j=DEST_STRUCT_IBLOCK(i).size;
          while (j--) {
            jitc_add(jitc, "mov  eax,  [$d]", &DEST_STRUCT_IBLOCK_VAR(i,j));
            jitc_add(jitc, "imul eax,  [$d]", &SRC_STRUCT_IBLOCK_VAR(i,j));
            jitc_add(jitc, "mov  [$d], eax",  &DEST_STRUCT_IBLOCK_VAR(i,j));
          }
          ++i;
        }
        break;
        }
      case INSTR_DIVS_VAR_VAR:
        {
        /* process integers */
        int i=0;
        while (DEST_STRUCT_IBLOCK(i).size > 0) {
          int j=DEST_STRUCT_IBLOCK(i).size;
          while (j--) {
            jitc_add(jitc, "mov  eax,  [$d]", &DEST_STRUCT_IBLOCK_VAR(i,j));
            jitc_add(jitc, "cdq");
            jitc_add(jitc, "idiv [$d]",       &SRC_STRUCT_IBLOCK_VAR(i,j));
            jitc_add(jitc, "mov  [$d], eax",  &DEST_STRUCT_IBLOCK_VAR(i,j));
          }
          ++i;
        }
        break;
        }
    }
  }

  JITC_ADD_LABEL (jitc, "__very_end__");
  jitc_add(jitc, "call $s", "__very_start__");
  jitc_add(jitc, "mov eax, $d", 0);
  jitc_validate_func(jitc);
#else
  InstructionFlow     *iflow     = currentGoomSL->iflow;
  FastInstructionFlow *fastiflow = (FastInstructionFlow*)malloc(sizeof(FastInstructionFlow));
  fastiflow->mallocedInstr = calloc(number*16, sizeof(FastInstruction));
  /* fastiflow->instr = (FastInstruction*)(((int)fastiflow->mallocedInstr) + 16 - (((int)fastiflow->mallocedInstr)%16)); */
  fastiflow->instr = (FastInstruction*)fastiflow->mallocedInstr;
  fastiflow->number = number;
  for(i=0;i<number;++i) {
    fastiflow->instr[i].id    = iflow->instr[i]->id;
    fastiflow->instr[i].data  = iflow->instr[i]->data;
    fastiflow->instr[i].proto = iflow->instr[i];
  }
  currentGoomSL->fastiflow = fastiflow;
#endif
} /* }}} */

void yy_scan_string(const char *str);
void yyparse(void);

GoomHash *gsl_globals(GoomSL *_this)
{
  return _this->vars;
}


/**
 * Some native external functions
 */
static void ext_charAt(GoomSL *gsl, GoomHash *global, GoomHash *local)
{
  char *string = GSL_LOCAL_PTR(gsl, local, "value");
  int   index  = GSL_LOCAL_INT(gsl, local, "index");
  GSL_GLOBAL_INT(gsl, "charAt") = 0;
  if (string == NULL) {
    return;
  }
  if (index < strlen(string))
    GSL_GLOBAL_INT(gsl, "charAt") = string[index];
}

static void ext_i2f(GoomSL *gsl, GoomHash *global, GoomHash *local)
{
  int i = GSL_LOCAL_INT(gsl, local, "value");
  GSL_GLOBAL_FLOAT(gsl, "i2f") = i;
}

static void ext_f2i(GoomSL *gsl, GoomHash *global, GoomHash *local)
{
  float f = GSL_LOCAL_FLOAT(gsl, local, "value");
  GSL_GLOBAL_INT(gsl, "f2i") = f;
}

/**
 *
 */
void gsl_compile(GoomSL *_currentGoomSL, const char *script)
{ /* {{{ */
  char *script_and_externals;
  static const char *sBinds =
    "external <charAt: string value, int index> : int\n"
    "external <f2i: float value> : int\n"
    "external <i2f: int value> : float\n";

#ifdef VERBOSE
  printf("\n=== Starting Compilation ===\n");
#endif

  script_and_externals = malloc(strlen(script) + strlen(sBinds) + 2);
  strcpy(script_and_externals, sBinds);
  strcat(script_and_externals, script);

  /* 0- reset */
  currentGoomSL = _currentGoomSL;
  reset_scanner(currentGoomSL);

  /* 1- create the syntaxic tree */
  yy_scan_string(script_and_externals);
  yyparse();

  /* 2- generate code */
  gsl_commit_compilation();

  /* 3- resolve symbols */
  calculate_labels(currentGoomSL->iflow);

  /* 4- optimize code */
  gsl_create_fast_iflow();

  /* 5- bind a few internal functions */
  gsl_bind_function(currentGoomSL, "charAt", ext_charAt);
  gsl_bind_function(currentGoomSL, "f2i", ext_f2i);
  gsl_bind_function(currentGoomSL, "i2f", ext_i2f);
  free(script_and_externals);
  
#ifdef VERBOSE
  printf("=== Compilation done. # of lines: %d. # of instr: %d ===\n", currentGoomSL->num_lines, currentGoomSL->iflow->number);
#endif
} /* }}} */

void gsl_execute(GoomSL *scanner)
{ /* {{{ */
  if (scanner->compilationOK) {
#if USE_JITC_X86
    scanner->jitc_func();
#else
    iflow_execute(scanner->fastiflow, scanner);
#endif
  }
} /* }}} */

GoomSL *gsl_new(void)
{ /* {{{ */
  GoomSL *gss = (GoomSL*)malloc(sizeof(GoomSL));

  gss->iflow = iflow_new();
  gss->vars  = goom_hash_new();
  gss->functions = goom_hash_new();
  gss->nbStructID  = 0;
  gss->structIDS   = goom_hash_new();
  gss->gsl_struct_size = 32;
  gss->gsl_struct = (GSL_Struct**)malloc(gss->gsl_struct_size * sizeof(GSL_Struct*));
  gss->currentNS = 0;
  gss->namespaces[0] = gss->vars;
  gss->data_heap = goom_heap_new();

  reset_scanner(gss);

  gss->compilationOK = 0;
  gss->nbPtr=0;
  gss->ptrArraySize=256;
  gss->ptrArray = (void**)malloc(gss->ptrArraySize * sizeof(void*));
#ifdef USE_JITC_X86
  gss->jitc = NULL;
#endif
  return gss;
} /* }}} */

void gsl_bind_function(GoomSL *gss, const char *fname, GoomSL_ExternalFunction func)
{ /* {{{ */
  HashValue *val = goom_hash_get(gss->functions, fname);
  if (val) {
    ExternalFunctionStruct *gef = (ExternalFunctionStruct*)val->ptr;
    gef->function = func;
  }
  else fprintf(stderr, "Unable to bind function %s\n", fname);
} /* }}} */

int gsl_is_compiled(GoomSL *gss)
{ /* {{{ */
  return gss->compilationOK;
} /* }}} */

void gsl_free(GoomSL *gss)
{ /* {{{ */
  iflow_free(gss->iflow);
  goom_hash_free(gss->vars);
  goom_hash_free(gss->functions);
  goom_hash_free(gss->structIDS);
  free(gss->gsl_struct);
  goom_heap_delete(gss->data_heap);
  free(gss->ptrArray);
  free(gss);
} /* }}} */


static int gsl_nb_import;
static char gsl_already_imported[256][256];

char *gsl_init_buffer(const char *fname)
{
    char *fbuffer;
    fbuffer = (char*)malloc(512);
    fbuffer[0]=0;
    gsl_nb_import = 0;
    if (fname)
      gsl_append_file_to_buffer(fname,&fbuffer);
    return fbuffer;
}

static char *gsl_read_file(const char *fname)
{
  FILE *f;
  char *buffer;
  int fsize;
  f = fopen(fname,"rt");
  if (!f) {
    fprintf(stderr, "ERROR: Could not load file %s\n", fname);
    exit(1);
  }
  fseek(f,0,SEEK_END);
  fsize = ftell(f);
  rewind(f);
  buffer = (char*)malloc(fsize+512);
  fread(buffer,1,fsize,f);
  fclose(f);
  buffer[fsize]=0;
  return buffer;
}

void gsl_append_file_to_buffer(const char *fname, char **buffer)
{
    char *fbuffer;
    int size,fsize,i=0;
    char reset_msg[256];
    
    /* look if the file have not been already imported */
    for (i=0;i<gsl_nb_import;++i) {
      if (strcmp(gsl_already_imported[i], fname) == 0)
        return;
    }
    
    /* add fname to the already imported files. */
    strcpy(gsl_already_imported[gsl_nb_import++], fname);

    /* load the file */
    fbuffer = gsl_read_file(fname);
    fsize = strlen(fbuffer);
        
    /* look for #import */
    while (fbuffer[i]) {
      if ((fbuffer[i]=='#') && (fbuffer[i+1]=='i')) {
        char impName[256];
        int j;
        while (fbuffer[i] && (fbuffer[i]!=' '))
          i++;
        i++;
        j=0;
        while (fbuffer[i] && (fbuffer[i]!='\n'))
          impName[j++] = fbuffer[i++];
        impName[j++] = 0;
        gsl_append_file_to_buffer(impName, buffer);
      }
      i++;
    }
    
    sprintf(reset_msg, "\n#FILE %s#\n#RST_LINE#\n", fname);
    strcat(*buffer, reset_msg);
    size=strlen(*buffer);
    *buffer = (char*)realloc(*buffer, size+fsize+256);
    strcat((*buffer)+size, fbuffer);
    free(fbuffer);
}


