/**
 * copyright 2004, Jean-Christophe Hoelt <jeko@ios-software.com>
 *
 * This program is released under the terms of the GNU Lesser General Public Licence.
 */
%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "goomsl.h"
    #include "goomsl_private.h"

#define STRUCT_ALIGNMENT 16
/* #define VERBOSE  */

    int yylex(void);
    void yyerror(char *);
    extern GoomSL *currentGoomSL;

    static NodeType *nodeNew(const char *str, int type, int line_number);
    static NodeType *nodeClone(NodeType *node);
    static void nodeFreeInternals(NodeType *node);
    static void nodeFree(NodeType *node);

    static void commit_node(NodeType *node, int releaseIfTemp);
    static void precommit_node(NodeType *node);

    static NodeType *new_constInt(const char *str, int line_number);
    static NodeType *new_constFloat(const char *str, int line_number);
    static NodeType *new_constPtr(const char *str, int line_number);
    static NodeType *new_var(const char *str, int line_number);
    static NodeType *new_nop(const char *str);
    static NodeType *new_op(const char *str, int type, int nbOp);

    static int  allocateLabel();
    static int  allocateTemp();
    static void releaseTemp(int n);
    static void releaseAllTemps();

    static int is_tmp_expr(NodeType *node) {
        if (node->str) {
            return (!strncmp(node->str,"_i_tmp_",7))
              || (!strncmp(node->str,"_f_tmp_",7))
              || (!strncmp(node->str,"_p_tmp",7));
        }
        return 0;
    }
    /* pre: is_tmp_expr(node); */
    static int get_tmp_id(NodeType *node)  { return atoi((node->str)+5); }

    static int is_commutative_expr(int itype)
    { /* {{{ */
        return (itype == INSTR_ADD)
            || (itype == INSTR_MUL)
            || (itype == INSTR_ISEQUAL);
    } /* }}} */

    static void GSL_PUT_LABEL(char *name, int line_number)
    { /* {{{ */
#ifdef VERBOSE
      printf("label %s\n", name);
#endif
      currentGoomSL->instr = gsl_instr_init(currentGoomSL, "label", INSTR_LABEL, 1, line_number);
      gsl_instr_add_param(currentGoomSL->instr, name, TYPE_LABEL);
    } /* }}} */
    static void GSL_PUT_JUMP(char *name, int line_number)
    { /* {{{ */
#ifdef VERBOSE
      printf("jump %s\n", name);
#endif
      currentGoomSL->instr = gsl_instr_init(currentGoomSL, "jump", INSTR_JUMP, 1, line_number);
      gsl_instr_add_param(currentGoomSL->instr, name, TYPE_LABEL);
    } /* }}} */

    static void GSL_PUT_JXXX(char *name, char *iname, int instr_id, int line_number)
    { /* {{{ */
#ifdef VERBOSE
      printf("%s %s\n", iname, name);
#endif
      currentGoomSL->instr = gsl_instr_init(currentGoomSL, iname, instr_id, 1, line_number);
      gsl_instr_add_param(currentGoomSL->instr, name, TYPE_LABEL);
    } /* }}} */
    static void GSL_PUT_JZERO(char *name,int line_number)
    { /* {{{ */
      GSL_PUT_JXXX(name,"jzero.i",INSTR_JZERO,line_number);
    } /* }}} */
    static void GSL_PUT_JNZERO(char *name, int line_number)
    { /* {{{ */
      GSL_PUT_JXXX(name,"jnzero.i",INSTR_JNZERO,line_number);
    } /* }}} */

    /* Structures Management */

#define ALIGN_ADDR(_addr,_align) {\
   if (_align>1) {\
       int _dec = (_addr%_align);\
       if (_dec != 0) _addr += _align - _dec;\
   }}

    /* */
    void gsl_prepare_struct(GSL_Struct *s, int s_align, int i_align, int f_align)
    {
      int i;
      int consumed = 0;
      int iblk=0, fblk=0;

      s->iBlock[0].size = 0;
      s->iBlock[0].data = 0;
      s->fBlock[0].size = 0;
      s->fBlock[0].data = 0;

      /* Prepare sub-struct and calculate space needed for their storage */
      for (i = 0; i < s->nbFields; ++i)
      {
        if (s->fields[i]->type < FIRST_RESERVED)
        {
          int j=0;
          GSL_Struct *substruct = currentGoomSL->gsl_struct[s->fields[i]->type];
          consumed += sizeof(int); /* stocke le prefix */
          ALIGN_ADDR(consumed, s_align);
          s->fields[i]->offsetInStruct = consumed;
          gsl_prepare_struct(substruct, s_align, i_align, f_align);
          for(j=0;substruct->iBlock[j].size>0;++j) {
            s->iBlock[iblk].data = consumed + substruct->iBlock[j].data;
            s->iBlock[iblk].size = substruct->iBlock[j].size;
            iblk++;
          }
          for(j=0;substruct->fBlock[j].size>0;++j) {
            s->fBlock[fblk].data = consumed + substruct->fBlock[j].data;
            s->fBlock[fblk].size = substruct->fBlock[j].size;
            fblk++;
          }
          consumed += substruct->size;
        }
      }

      /* Then prepare integers */
      ALIGN_ADDR(consumed, i_align);
      for (i = 0; i < s->nbFields; ++i)
      {
        if (s->fields[i]->type == INSTR_INT)
        {
          if (s->iBlock[iblk].size == 0) {
            s->iBlock[iblk].size = 1;
            s->iBlock[iblk].data = consumed;
          } else {
            s->iBlock[iblk].size += 1;
          }
          s->fields[i]->offsetInStruct = consumed;
          consumed += sizeof(int);
        }
      }

      iblk++;
      s->iBlock[iblk].size = 0;
      s->iBlock[iblk].data = 0;

      /* Then prepare floats */
      ALIGN_ADDR(consumed, f_align);
      for (i = 0; i < s->nbFields; ++i)
      {
        if (s->fields[i]->type == INSTR_FLOAT)
        {
          if (s->fBlock[fblk].size == 0) {
            s->fBlock[fblk].size = 1;
            s->fBlock[fblk].data = consumed;
          } else {
            s->fBlock[fblk].size += 1;
          }
          s->fields[i]->offsetInStruct = consumed;
          consumed += sizeof(int);
        }
      }

      fblk++;
      s->fBlock[fblk].size = 0;
      s->fBlock[fblk].data = 0;
      
      /* Finally prepare pointers */
      ALIGN_ADDR(consumed, i_align);
      for (i = 0; i < s->nbFields; ++i)
      {
        if (s->fields[i]->type == INSTR_PTR)
        {
          s->fields[i]->offsetInStruct = consumed;
          consumed += sizeof(int);
        }
      }
      s->size = consumed;
    }

    /* Returns the ID of a struct from its name */
    int gsl_get_struct_id(const char *name) /* {{{ */
    {
      HashValue *ret = goom_hash_get(currentGoomSL->structIDS, name);
      if (ret != NULL) return ret->i;
      return -1;
    } /* }}} */

    /* Adds the definition of a struct */
    void gsl_add_struct(const char *name, GSL_Struct *gsl_struct) /* {{{ */
    {
      /* Prepare the struct: ie calculate internal storage format */
      gsl_prepare_struct(gsl_struct, STRUCT_ALIGNMENT, STRUCT_ALIGNMENT, STRUCT_ALIGNMENT);
      
      /* If the struct does not already exists */
      if (gsl_get_struct_id(name) < 0)
      {
        /* adds it */
        int id = currentGoomSL->nbStructID++;
        goom_hash_put_int(currentGoomSL->structIDS, name, id);
        if (currentGoomSL->gsl_struct_size <= id) {
          currentGoomSL->gsl_struct_size *= 2;
          currentGoomSL->gsl_struct = (GSL_Struct**)realloc(currentGoomSL->gsl_struct,
                                                            sizeof(GSL_Struct*) * currentGoomSL->gsl_struct_size);
        }
        currentGoomSL->gsl_struct[id] = gsl_struct;
      }
    } /* }}} */
    
    /* Creates a field for a struct */
    GSL_StructField *gsl_new_struct_field(const char *name, int type)
    {
      GSL_StructField *field = (GSL_StructField*)malloc(sizeof(GSL_StructField));
      strcpy(field->name, name);
      field->type = type;
      return field;
    }
    
    /* Create as field for a struct which will be a struct itself */
    GSL_StructField *gsl_new_struct_field_struct(const char *name, const char *type)
    {
      GSL_StructField *field = gsl_new_struct_field(name, gsl_get_struct_id(type));
      if (field->type < 0) {
        fprintf(stderr, "ERROR: Line %d, Unknown structure: '%s'\n",
                currentGoomSL->num_lines, type);
        exit(1);
      }
      return field;
    }

    /* Creates a Struct */
    GSL_Struct *gsl_new_struct(GSL_StructField *field)
    {
      GSL_Struct *s = (GSL_Struct*)malloc(sizeof(GSL_Struct));
      s->nbFields = 1;
      s->fields[0] = field;
      return s;
    }

    /* Adds a field to a struct */
    void gsl_add_struct_field(GSL_Struct *s, GSL_StructField *field)
    {
      s->fields[s->nbFields++] = field;
    }

    int gsl_type_of_var(GoomHash *ns, const char *name)
    {
        char type_of[256];
        HashValue *hv;
        sprintf(type_of, "__type_of_%s", name);
        hv = goom_hash_get(ns, type_of);
        if (hv != NULL)
          return hv->i;
        fprintf(stderr, "ERROR: Unknown variable type: '%s'\n", name);
        return -1;
    }

    static void gsl_declare_var(GoomHash *ns, const char *name, int type, void *space)
    {
        char type_of[256];
        if (name[0] == '@') { ns = currentGoomSL->vars; }

        if (space == NULL) {
          switch (type) {
            case INSTR_INT:
            case INSTR_FLOAT:
            case INSTR_PTR:
              space = goom_heap_malloc_with_alignment(currentGoomSL->data_heap,
                  sizeof(int), sizeof(int));
            break;
            case -1:
              fprintf(stderr, "What the fuck!\n");
              exit(1);
            default: /* On a un struct_id */
              space = goom_heap_malloc_with_alignment_prefixed(currentGoomSL->data_heap,
                  currentGoomSL->gsl_struct[type]->size, STRUCT_ALIGNMENT, sizeof(int));
          }
        }
        goom_hash_put_ptr(ns, name, (void*)space);
        sprintf(type_of, "__type_of_%s", name);
        goom_hash_put_int(ns, type_of, type);

        /* Ensuite le hack: on ajoute les champs en tant que variables. */
        if (type < FIRST_RESERVED)
        {
          int i;
          GSL_Struct *gsl_struct = currentGoomSL->gsl_struct[type];
          ((int*)space)[-1] = type; /* stockage du type dans le prefixe de structure */
          for (i = 0; i < gsl_struct->nbFields; ++i)
          {
            char full_name[256];
            char *cspace = (char*)space + gsl_struct->fields[i]->offsetInStruct;
            sprintf(full_name, "%s.%s", name, gsl_struct->fields[i]->name);
            gsl_declare_var(ns, full_name, gsl_struct->fields[i]->type, cspace);
          }
       }
    }
    
    /* Declare a variable which will be a struct */
    static void gsl_struct_decl(GoomHash *namespace, const char *struct_name, const char *name)
    {
        int  struct_id = gsl_get_struct_id(struct_name);
        gsl_declare_var(namespace, name, struct_id, NULL);
    }

    static void gsl_float_decl_global(const char *name)
    {
        gsl_declare_var(currentGoomSL->vars, name, INSTR_FLOAT, NULL);
    }
    static void gsl_int_decl_global(const char *name)
    {
        gsl_declare_var(currentGoomSL->vars, name, INSTR_INT, NULL);
    }
    static void gsl_ptr_decl_global(const char *name)
    {
        gsl_declare_var(currentGoomSL->vars, name, INSTR_PTR, NULL);
    }
    static void gsl_struct_decl_global_from_id(const char *name, int id)
    {
        gsl_declare_var(currentGoomSL->vars, name, id, NULL);
    }
    
    /* FLOAT */
    static void gsl_float_decl_local(const char *name)
    {
        gsl_declare_var(currentGoomSL->namespaces[currentGoomSL->currentNS], name, INSTR_FLOAT, NULL);
    }
    /* INT */
    static void gsl_int_decl_local(const char *name)
    {
        gsl_declare_var(currentGoomSL->namespaces[currentGoomSL->currentNS], name, INSTR_INT, NULL);
    }
    /* PTR */
    static void gsl_ptr_decl_local(const char *name)
    {
        gsl_declare_var(currentGoomSL->namespaces[currentGoomSL->currentNS], name, INSTR_PTR, NULL);
    }
    /* STRUCT */
    static void gsl_struct_decl_local(const char *struct_name, const char *name)
    {
        gsl_struct_decl(currentGoomSL->namespaces[currentGoomSL->currentNS],struct_name,name);
    }


    static void commit_test2(NodeType *set,const char *type, int instr);
    static NodeType *new_call(const char *name, NodeType *affect_list);

    /* SETTER */
    static NodeType *new_set(NodeType *lvalue, NodeType *expression)
    { /* {{{ */
        NodeType *set = new_op("set", OPR_SET, 2);
        set->unode.opr.op[0] = lvalue;
        set->unode.opr.op[1] = expression;
        return set;
    } /* }}} */
    static void commit_set(NodeType *set)
    { /* {{{ */
      commit_test2(set,"set",INSTR_SET);
    } /* }}} */

    /* PLUS_EQ */
    static NodeType *new_plus_eq(NodeType *lvalue, NodeType *expression) /* {{{ */
    {
        NodeType *set = new_op("plus_eq", OPR_PLUS_EQ, 2);
        set->unode.opr.op[0] = lvalue;
        set->unode.opr.op[1] = expression;
        return set;
    }
    static void commit_plus_eq(NodeType *set)
    {
        precommit_node(set->unode.opr.op[1]);
#ifdef VERBOSE
        printf("add %s %s\n", set->unode.opr.op[0]->str, set->unode.opr.op[1]->str);
#endif
        currentGoomSL->instr = gsl_instr_init(currentGoomSL, "add", INSTR_ADD, 2, set->line_number);
        commit_node(set->unode.opr.op[0],0);
        commit_node(set->unode.opr.op[1],1);
    } /* }}} */

    /* SUB_EQ */
    static NodeType *new_sub_eq(NodeType *lvalue, NodeType *expression) /* {{{ */
    {
        NodeType *set = new_op("sub_eq", OPR_SUB_EQ, 2);
        set->unode.opr.op[0] = lvalue;
        set->unode.opr.op[1] = expression;
        return set;
    }
    static void commit_sub_eq(NodeType *set)
    {
        precommit_node(set->unode.opr.op[1]);
#ifdef VERBOSE
        printf("sub %s %s\n", set->unode.opr.op[0]->str, set->unode.opr.op[1]->str);
#endif
        currentGoomSL->instr = gsl_instr_init(currentGoomSL, "sub", INSTR_SUB, 2, set->line_number);
        commit_node(set->unode.opr.op[0],0);
        commit_node(set->unode.opr.op[1],1);
    } /* }}} */

    /* MUL_EQ */
    static NodeType *new_mul_eq(NodeType *lvalue, NodeType *expression) /* {{{ */
    {
        NodeType *set = new_op("mul_eq", OPR_MUL_EQ, 2);
        set->unode.opr.op[0] = lvalue;
        set->unode.opr.op[1] = expression;
        return set;
    }
    static void commit_mul_eq(NodeType *set)
    {
        precommit_node(set->unode.opr.op[1]);
#ifdef VERBOSE
        printf("mul %s %s\n", set->unode.opr.op[0]->str, set->unode.opr.op[1]->str);
#endif
        currentGoomSL->instr = gsl_instr_init(currentGoomSL, "mul", INSTR_MUL, 2, set->line_number);
        commit_node(set->unode.opr.op[0],0);
        commit_node(set->unode.opr.op[1],1);
    } /* }}} */

    /* DIV_EQ */
    static NodeType *new_div_eq(NodeType *lvalue, NodeType *expression) /* {{{ */
    {
        NodeType *set = new_op("div_eq", OPR_DIV_EQ, 2);
        set->unode.opr.op[0] = lvalue;
        set->unode.opr.op[1] = expression;
        return set;
    }
    static void commit_div_eq(NodeType *set)
    {
        precommit_node(set->unode.opr.op[1]);
#ifdef VERBOSE
        printf("div %s %s\n", set->unode.opr.op[0]->str, set->unode.opr.op[1]->str);
#endif
        currentGoomSL->instr = gsl_instr_init(currentGoomSL, "div", INSTR_DIV, 2, set->line_number);
        commit_node(set->unode.opr.op[0],0);
        commit_node(set->unode.opr.op[1],1);
    } /* }}} */

    /* commodity method for add, mult, ... */

    static void precommit_expr(NodeType *expr, const char *type, int instr_id)
    { /* {{{ */
        NodeType *tmp, *tmpcpy;
        int toAdd;

        /* compute "left" and "right" */
        switch (expr->unode.opr.nbOp) {
        case 2:
          precommit_node(expr->unode.opr.op[1]);
        case 1:
          precommit_node(expr->unode.opr.op[0]);
        }

        if (is_tmp_expr(expr->unode.opr.op[0])) {
            tmp = expr->unode.opr.op[0];
            toAdd = 1;
        }
        else if (is_commutative_expr(instr_id) && (expr->unode.opr.nbOp==2) && is_tmp_expr(expr->unode.opr.op[1])) {
            tmp = expr->unode.opr.op[1];
            toAdd = 0;
        }
        else {
            char stmp[256];
            /* declare a temporary variable to store the result */
            if (expr->unode.opr.op[0]->type == CONST_INT_NODE) {
                sprintf(stmp,"_i_tmp_%i",allocateTemp());
                gsl_int_decl_global(stmp);
            }
            else if (expr->unode.opr.op[0]->type == CONST_FLOAT_NODE) {
                sprintf(stmp,"_f_tmp%i",allocateTemp());
                gsl_float_decl_global(stmp);
            }
            else if (expr->unode.opr.op[0]->type == CONST_PTR_NODE) {
                sprintf(stmp,"_p_tmp%i",allocateTemp());
                gsl_ptr_decl_global(stmp);
            }
            else {
                int type = gsl_type_of_var(expr->unode.opr.op[0]->vnamespace, expr->unode.opr.op[0]->str);
                if (type == INSTR_FLOAT) {
                    sprintf(stmp,"_f_tmp_%i",allocateTemp());
                    gsl_float_decl_global(stmp);
                }
                else if (type == INSTR_PTR) {
                    sprintf(stmp,"_p_tmp_%i",allocateTemp());
                    gsl_ptr_decl_global(stmp);
                }
                else if (type == INSTR_INT) {
                    sprintf(stmp,"_i_tmp_%i",allocateTemp());
                    gsl_int_decl_global(stmp);
                }
                else if (type == -1) {
                    fprintf(stderr, "ERROR: Line %d, Could not find variable '%s'\n",
                            expr->line_number, expr->unode.opr.op[0]->str);
                    exit(1);
                }
                else { /* type is a struct_id */
                    sprintf(stmp,"_s_tmp_%i",allocateTemp());
                    gsl_struct_decl_global_from_id(stmp,type);
                }
            }
            tmp = new_var(stmp,expr->line_number);

            /* set the tmp to the value of "op1" */
            tmpcpy = nodeClone(tmp);
            commit_node(new_set(tmp,expr->unode.opr.op[0]),0);
            toAdd = 1;

            tmp = tmpcpy;
        }

        /* add op2 to tmp */
#ifdef VERBOSE
        if (expr->unode.opr.nbOp == 2)
          printf("%s %s %s\n", type, tmp->str, expr->unode.opr.op[toAdd]->str);
        else
          printf("%s %s\n", type, tmp->str);
#endif
        currentGoomSL->instr = gsl_instr_init(currentGoomSL, type, instr_id, expr->unode.opr.nbOp, expr->line_number);
        tmpcpy = nodeClone(tmp);
        commit_node(tmp,0);
        if (expr->unode.opr.nbOp == 2) {
          commit_node(expr->unode.opr.op[toAdd],1);
        }
    
        /* redefine the ADD node now as the computed variable */
        nodeFreeInternals(expr);
        *expr = *tmpcpy;
        free(tmpcpy);
    } /* }}} */

    static NodeType *new_expr1(const char *name, int id, NodeType *expr1)
    { /* {{{ */
        NodeType *add = new_op(name, id, 1);
        add->unode.opr.op[0] = expr1;
        return add;
    } /* }}} */

    static NodeType *new_expr2(const char *name, int id, NodeType *expr1, NodeType *expr2)
    { /* {{{ */
        NodeType *add = new_op(name, id, 2);
        add->unode.opr.op[0] = expr1;
        add->unode.opr.op[1] = expr2;
        return add;
    } /* }}} */

    /* ADD */
    static NodeType *new_add(NodeType *expr1, NodeType *expr2) { /* {{{ */
        return new_expr2("add", OPR_ADD, expr1, expr2);
    }
    static void precommit_add(NodeType *add) {
        precommit_expr(add,"add",INSTR_ADD);
    } /* }}} */

    /* SUB */
    static NodeType *new_sub(NodeType *expr1, NodeType *expr2) { /* {{{ */
        return new_expr2("sub", OPR_SUB, expr1, expr2);
    }
    static void precommit_sub(NodeType *sub) {
        precommit_expr(sub,"sub",INSTR_SUB);
    } /* }}} */

    /* NEG */
    static NodeType *new_neg(NodeType *expr) { /* {{{ */
        NodeType *zeroConst = NULL;
        if (expr->type == CONST_INT_NODE)
          zeroConst = new_constInt("0", currentGoomSL->num_lines);
        else if (expr->type == CONST_FLOAT_NODE)
          zeroConst = new_constFloat("0.0", currentGoomSL->num_lines);
        else if (expr->type == CONST_PTR_NODE) {
          fprintf(stderr, "ERROR: Line %d, Could not negate const pointer.\n",
            currentGoomSL->num_lines);
          exit(1);
        }
        else {
            int type = gsl_type_of_var(expr->vnamespace, expr->str);
            if (type == INSTR_FLOAT)
              zeroConst = new_constFloat("0.0", currentGoomSL->num_lines);
            else if (type == INSTR_PTR) {
              fprintf(stderr, "ERROR: Line %d, Could not negate pointer.\n",
                currentGoomSL->num_lines);
              exit(1);
            }
            else if (type == INSTR_INT)
              zeroConst = new_constInt("0", currentGoomSL->num_lines);
            else if (type == -1) {
                fprintf(stderr, "ERROR: Line %d, Could not find variable '%s'\n",
                        expr->line_number, expr->unode.opr.op[0]->str);
                exit(1);
            }
            else { /* type is a struct_id */
                fprintf(stderr, "ERROR: Line %d, Could not negate struct '%s'\n",
                        expr->line_number, expr->str);
                exit(1);
            }
        }
        return new_expr2("sub", OPR_SUB, zeroConst, expr);
    }
    /* }}} */

    /* MUL */
    static NodeType *new_mul(NodeType *expr1, NodeType *expr2) { /* {{{ */
        return new_expr2("mul", OPR_MUL, expr1, expr2);
    }
    static void precommit_mul(NodeType *mul) {
        precommit_expr(mul,"mul",INSTR_MUL);
    } /* }}} */
    
    /* DIV */
    static NodeType *new_div(NodeType *expr1, NodeType *expr2) { /* {{{ */
        return new_expr2("div", OPR_DIV, expr1, expr2);
    }
    static void precommit_div(NodeType *mul) {
        precommit_expr(mul,"div",INSTR_DIV);
    } /* }}} */

    /* CALL EXPRESSION */
    static NodeType *new_call_expr(const char *name, NodeType *affect_list) { /* {{{ */
        NodeType *call = new_call(name,affect_list);
        NodeType *node = new_expr1(name, OPR_CALL_EXPR, call);
        node->vnamespace = gsl_find_namespace(name);
        if (node->vnamespace == NULL)
          fprintf(stderr, "ERROR: Line %d, No return type for: '%s'\n", currentGoomSL->num_lines, name);
        return node;
    }
    static void precommit_call_expr(NodeType *call) {
        char stmp[256];
        NodeType *tmp,*tmpcpy;
        int type = gsl_type_of_var(call->vnamespace, call->str);
        if (type == INSTR_FLOAT) {
          sprintf(stmp,"_f_tmp_%i",allocateTemp());
          gsl_float_decl_global(stmp);
        }
        else if (type == INSTR_PTR) {
          sprintf(stmp,"_p_tmp_%i",allocateTemp());
          gsl_ptr_decl_global(stmp);
        }
        else if (type == INSTR_INT) {
          sprintf(stmp,"_i_tmp_%i",allocateTemp());
          gsl_int_decl_global(stmp);
        }
        else if (type == -1) {
          fprintf(stderr, "ERROR: Line %d, Could not find variable '%s'\n",
                  call->line_number, call->str);
          exit(1);
        }
        else { /* type is a struct_id */
          sprintf(stmp,"_s_tmp_%i",allocateTemp());
          gsl_struct_decl_global_from_id(stmp,type);
        }
        tmp = new_var(stmp,call->line_number);
        commit_node(call->unode.opr.op[0],0);
        tmpcpy = nodeClone(tmp);
        commit_node(new_set(tmp,new_var(call->str,call->line_number)),0);
        
        nodeFreeInternals(call);
        *call = *tmpcpy;
        free(tmpcpy);
    } /* }}} */

    static void commit_test2(NodeType *set,const char *type, int instr)
    { /* {{{ */
        NodeType *tmp;
        char stmp[256];
        precommit_node(set->unode.opr.op[0]);
        precommit_node(set->unode.opr.op[1]);
        tmp = set->unode.opr.op[0];
        
        stmp[0] = 0;
        if (set->unode.opr.op[0]->type == CONST_INT_NODE) {
            sprintf(stmp,"_i_tmp_%i",allocateTemp());
            gsl_int_decl_global(stmp);
        }
        else if (set->unode.opr.op[0]->type == CONST_FLOAT_NODE) {
            sprintf(stmp,"_f_tmp%i",allocateTemp());
            gsl_float_decl_global(stmp);
        }
        else if (set->unode.opr.op[0]->type == CONST_PTR_NODE) {
            sprintf(stmp,"_p_tmp%i",allocateTemp());
            gsl_ptr_decl_global(stmp);
        }
        if (stmp[0]) {
            NodeType *tmpcpy;
            tmp = new_var(stmp, set->line_number);
            tmpcpy = nodeClone(tmp);
            commit_node(new_set(tmp,set->unode.opr.op[0]),0);
            tmp = tmpcpy;
        }

#ifdef VERBOSE
        printf("%s %s %s\n", type, tmp->str, set->unode.opr.op[1]->str);
#endif
        currentGoomSL->instr = gsl_instr_init(currentGoomSL, type, instr, 2, set->line_number);
        commit_node(tmp,instr!=INSTR_SET);
        commit_node(set->unode.opr.op[1],1);
    } /* }}} */
    
    /* NOT */
    static NodeType *new_not(NodeType *expr1) { /* {{{ */
        return new_expr1("not", OPR_NOT, expr1);
    }
    static void commit_not(NodeType *set)
    {
        commit_node(set->unode.opr.op[0],0);
#ifdef VERBOSE
        printf("not\n");
#endif
        currentGoomSL->instr = gsl_instr_init(currentGoomSL, "not", INSTR_NOT, 1, set->line_number);
        gsl_instr_add_param(currentGoomSL->instr, "|dummy|", TYPE_LABEL);
    } /* }}} */
    
    /* EQU */
    static NodeType *new_equ(NodeType *expr1, NodeType *expr2) { /* {{{ */
        return new_expr2("isequal", OPR_EQU, expr1, expr2);
    }
    static void commit_equ(NodeType *mul) {
        commit_test2(mul,"isequal",INSTR_ISEQUAL);
    } /* }}} */
    
    /* INF */
    static NodeType *new_low(NodeType *expr1, NodeType *expr2) { /* {{{ */
        return new_expr2("islower", OPR_LOW, expr1, expr2);
    }
    static void commit_low(NodeType *mul) {
        commit_test2(mul,"islower",INSTR_ISLOWER);
    } /* }}} */

    /* WHILE */
    static NodeType *new_while(NodeType *expression, NodeType *instr) { /* {{{ */
        NodeType *node = new_op("while", OPR_WHILE, 2);
        node->unode.opr.op[0] = expression;
        node->unode.opr.op[1] = instr;
        return node;
    }

    static void commit_while(NodeType *node)
    {
        int lbl = allocateLabel();
        char start_while[1024], test_while[1024];
        sprintf(start_while, "|start_while_%d|", lbl);
        sprintf(test_while, "|test_while_%d|", lbl);
       
        GSL_PUT_JUMP(test_while,node->line_number);
        GSL_PUT_LABEL(start_while,node->line_number);

        /* code */
        commit_node(node->unode.opr.op[1],0);

        GSL_PUT_LABEL(test_while,node->line_number);
        commit_node(node->unode.opr.op[0],0);
        GSL_PUT_JNZERO(start_while,node->line_number);
    } /* }}} */

    /* FOR EACH */
    static NodeType *new_static_foreach(NodeType *var, NodeType *var_list, NodeType *instr) { /* {{{ */
        NodeType *node = new_op("for", OPR_FOREACH, 3);
        node->unode.opr.op[0] = var;
        node->unode.opr.op[1] = var_list;
        node->unode.opr.op[2] = instr;
        node->line_number = currentGoomSL->num_lines;
        return node;
    }
    static void commit_foreach(NodeType *node)
    {
        NodeType *cur = node->unode.opr.op[1];
        char tmp_func[256], tmp_loop[256];
        int lbl = allocateLabel();
        sprintf(tmp_func, "|foreach_func_%d|", lbl);
        sprintf(tmp_loop, "|foreach_loop_%d|", lbl);

        GSL_PUT_JUMP(tmp_loop, node->line_number);
        GSL_PUT_LABEL(tmp_func, node->line_number);

        precommit_node(node->unode.opr.op[2]);
        commit_node(node->unode.opr.op[2], 0);

        currentGoomSL->instr = gsl_instr_init(currentGoomSL, "ret", INSTR_RET, 1, node->line_number);
        gsl_instr_add_param(currentGoomSL->instr, "|dummy|", TYPE_LABEL);
#ifdef VERBOSE
        printf("ret\n");
#endif
        
        GSL_PUT_LABEL(tmp_loop, node->line_number);
        
        while (cur != NULL)
        {
          NodeType *x, *var;

          /* 1: x=var */
          x   = nodeClone(node->unode.opr.op[0]);
          var = nodeClone(cur->unode.opr.op[0]);
          commit_node(new_set(x, var),0);
          
          /* 2: instr */
          currentGoomSL->instr = gsl_instr_init(currentGoomSL, "call", INSTR_CALL, 1, node->line_number);
          gsl_instr_add_param(currentGoomSL->instr, tmp_func, TYPE_LABEL);
#ifdef VERBOSE
          printf("call %s\n", tmp_func);
#endif
          
          /* 3: var=x */
          x   = nodeClone(node->unode.opr.op[0]);
          var = cur->unode.opr.op[0];
          commit_node(new_set(var, x),0);
          cur = cur->unode.opr.op[1];
        }
        nodeFree(node->unode.opr.op[0]);
    } /* }}} */

    /* IF */
    static NodeType *new_if(NodeType *expression, NodeType *instr) { /* {{{ */
        NodeType *node = new_op("if", OPR_IF, 2);
        node->unode.opr.op[0] = expression;
        node->unode.opr.op[1] = instr;
        return node;
    }
    static void commit_if(NodeType *node) {

        char slab[1024];
        sprintf(slab, "|eif%d|", allocateLabel());
        commit_node(node->unode.opr.op[0],0);
        GSL_PUT_JZERO(slab,node->line_number);
        /* code */
        commit_node(node->unode.opr.op[1],0);
        GSL_PUT_LABEL(slab,node->line_number);
    } /* }}} */

    /* BLOCK */
    static NodeType *new_block(NodeType *lastNode) { /* {{{ */
        NodeType *blk = new_op("block", OPR_BLOCK, 2);
        blk->unode.opr.op[0] = new_nop("start_of_block");
        blk->unode.opr.op[1] = lastNode;        
        return blk;
    }
    static void commit_block(NodeType *node) {
        commit_node(node->unode.opr.op[0]->unode.opr.next,0);
    } /* }}} */

    /* FUNCTION INTRO */
    static NodeType *new_function_intro(const char *name) { /* {{{ */
        char stmp[256];
        if (strlen(name) < 200) {
           sprintf(stmp, "|__func_%s|", name);
        }
        return new_op(stmp, OPR_FUNC_INTRO, 0);
    }
    static void commit_function_intro(NodeType *node) {
        currentGoomSL->instr = gsl_instr_init(currentGoomSL, "label", INSTR_LABEL, 1, node->line_number);
        gsl_instr_add_param(currentGoomSL->instr, node->str, TYPE_LABEL);
#ifdef VERBOSE
        printf("label %s\n", node->str);
#endif
    } /* }}} */

    /* FUNCTION OUTRO */
    static NodeType *new_function_outro() { /* {{{ */
        return new_op("ret", OPR_FUNC_OUTRO, 0);
    }
    static void commit_function_outro(NodeType *node) {
        currentGoomSL->instr = gsl_instr_init(currentGoomSL, "ret", INSTR_RET, 1, node->line_number);
        gsl_instr_add_param(currentGoomSL->instr, "|dummy|", TYPE_LABEL);
        releaseAllTemps();
#ifdef VERBOSE
        printf("ret\n");
#endif
    } /* }}} */
    
    /* AFFECTATION LIST */
    static NodeType *new_affec_list(NodeType *set, NodeType *next) /* {{{ */
    {
      NodeType *node = new_op("affect_list", OPR_AFFECT_LIST, 2);
      node->unode.opr.op[0] = set;
      node->unode.opr.op[1] = next;
      return node;
    }
    static NodeType *new_affect_list_after(NodeType *affect_list)
    {
      NodeType *ret  = NULL;
      NodeType *cur  =  affect_list;
      while(cur != NULL) {
        NodeType *set  = cur->unode.opr.op[0];
        NodeType *next = cur->unode.opr.op[1];
        NodeType *lvalue     = set->unode.opr.op[0];
        NodeType *expression = set->unode.opr.op[1];
        if ((lvalue->str[0] == '&') && (expression->type == VAR_NODE)) {
          NodeType *nset = new_set(nodeClone(expression), nodeClone(lvalue));
          ret  = new_affec_list(nset, ret);
        }
        cur = next;
      }
      return ret;
    }
    static void commit_affect_list(NodeType *node)
    {
      NodeType *cur = node;
      while(cur != NULL) {
        NodeType *set = cur->unode.opr.op[0];
        precommit_node(set->unode.opr.op[0]);
        precommit_node(set->unode.opr.op[1]);
        cur = cur->unode.opr.op[1];
      }
      cur = node;
      while(cur != NULL) {
        NodeType *set = cur->unode.opr.op[0];
        commit_node(set,0);
        cur = cur->unode.opr.op[1];
      }
    } /* }}} */

    /* VAR LIST */
    static NodeType *new_var_list(NodeType *var, NodeType *next) /* {{{ */
    {
      NodeType *node = new_op("var_list", OPR_VAR_LIST, 2);
      node->unode.opr.op[0] = var;
      node->unode.opr.op[1] = next;
      return node;
    }
    static void commit_var_list(NodeType *node)
    {
    } /* }}} */

    /* FUNCTION CALL */
    static NodeType *new_call(const char *name, NodeType *affect_list) { /* {{{ */
        HashValue *fval;
        fval = goom_hash_get(currentGoomSL->functions, name);
        if (!fval) {
            gsl_declare_task(name);
            fval = goom_hash_get(currentGoomSL->functions, name);
        }
        if (!fval) {
            fprintf(stderr, "ERROR: Line %d, Could not find function %s\n", currentGoomSL->num_lines, name);
            exit(1);
            return NULL;
        }
        else {
            ExternalFunctionStruct *gef = (ExternalFunctionStruct*)fval->ptr;
            if (gef->is_extern) {
                NodeType *node =  new_op(name, OPR_EXT_CALL, 1);
                node->unode.opr.op[0] = affect_list;
                return node;
            }
            else {
                NodeType *node;
                char stmp[256];
                if (strlen(name) < 200) {
                    sprintf(stmp, "|__func_%s|", name);
                }
                node = new_op(stmp, OPR_CALL, 1);
                node->unode.opr.op[0] = affect_list;
                return node;
            }
        }
    }
    static void commit_ext_call(NodeType *node) {
        NodeType *alafter = new_affect_list_after(node->unode.opr.op[0]);
        commit_node(node->unode.opr.op[0],0);
        currentGoomSL->instr = gsl_instr_init(currentGoomSL, "extcall", INSTR_EXT_CALL, 1, node->line_number);
        gsl_instr_add_param(currentGoomSL->instr, node->str, TYPE_VAR);
#ifdef VERBOSE
        printf("extcall %s\n", node->str);
#endif
        commit_node(alafter,0);
    }
    static void commit_call(NodeType *node) {
        NodeType *alafter = new_affect_list_after(node->unode.opr.op[0]);
        commit_node(node->unode.opr.op[0],0);
        currentGoomSL->instr = gsl_instr_init(currentGoomSL, "call", INSTR_CALL, 1, node->line_number);
        gsl_instr_add_param(currentGoomSL->instr, node->str, TYPE_LABEL);
#ifdef VERBOSE
        printf("call %s\n", node->str);
#endif
        commit_node(alafter,0);
    } /* }}} */

    /** **/

    static NodeType *rootNode = 0; /* TODO: reinitialiser a chaque compilation. */
    static NodeType *lastNode = 0;
    static NodeType *gsl_append(NodeType *curNode) {
        if (curNode == 0) return 0; /* {{{ */
        if (lastNode)
            lastNode->unode.opr.next = curNode;
        lastNode = curNode;
        while(lastNode->unode.opr.next) lastNode = lastNode->unode.opr.next;
        if (rootNode == 0)
            rootNode = curNode;
        return curNode;
    } /* }}} */

#if 1
    int allocateTemp() {
      return allocateLabel();
    }
    void releaseAllTemps() {}
    void releaseTemp(int n) {}
#else
    static int nbTemp = 0;
    static int *tempArray = 0;
    static int tempArraySize = 0;
    int allocateTemp() { /* TODO: allocateITemp, allocateFTemp */
        int i = 0; /* {{{ */
        if (tempArray == 0) {
          tempArraySize = 256;
          tempArray = (int*)malloc(tempArraySize * sizeof(int));
        }
        while (1) {
          int j;
          for (j=0;j<nbTemp;++j) {
            if (tempArray[j] == i) break;
          }
          if (j == nbTemp) {
            if (nbTemp == tempArraySize) {
              tempArraySize *= 2;
              tempArray = (int*)realloc(tempArray,tempArraySize * sizeof(int));
            }
            tempArray[nbTemp++] = i;
            return i;
          }
          i++;
        }
    } /* }}} */
    void releaseAllTemps() {
      nbTemp = 0; /* {{{ */
    } /* }}} */
    void releaseTemp(int n) {
      int j; /* {{{ */
      for (j=0;j<nbTemp;++j) {
        if (tempArray[j] == n) {
          tempArray[j] = tempArray[--nbTemp];
          break;
        }
      }
    } /* }}} */
#endif

    static int lastLabel = 0;
    int allocateLabel() {
        return ++lastLabel; /* {{{ */
    } /* }}} */

    void gsl_commit_compilation()
    { /* {{{ */
        commit_node(rootNode,0);
        rootNode = 0;
        lastNode = 0;
    } /* }}} */
    
    void precommit_node(NodeType *node)
    { /* {{{ */
        /* do here stuff for expression.. for exemple */
        if (node->type == OPR_NODE)
            switch(node->unode.opr.type) {
                case OPR_ADD: precommit_add(node); break;
                case OPR_SUB: precommit_sub(node); break;
                case OPR_MUL: precommit_mul(node); break;
                case OPR_DIV: precommit_div(node); break;
                case OPR_CALL_EXPR: precommit_call_expr(node); break;
            }
    } /* }}} */
    
    void commit_node(NodeType *node, int releaseIfTmp)
    { /* {{{ */
        if (node == 0) return;
        
        switch(node->type) {
            case OPR_NODE:
                switch(node->unode.opr.type) {
                    case OPR_SET:           commit_set(node); break;
                    case OPR_PLUS_EQ:       commit_plus_eq(node); break;
                    case OPR_SUB_EQ:        commit_sub_eq(node); break;
                    case OPR_MUL_EQ:        commit_mul_eq(node); break;
                    case OPR_DIV_EQ:        commit_div_eq(node); break;
                    case OPR_IF:            commit_if(node); break;
                    case OPR_WHILE:         commit_while(node); break;
                    case OPR_BLOCK:         commit_block(node); break;
                    case OPR_FUNC_INTRO:    commit_function_intro(node); break;
                    case OPR_FUNC_OUTRO:    commit_function_outro(node); break;
                    case OPR_CALL:          commit_call(node); break;
                    case OPR_EXT_CALL:      commit_ext_call(node); break;
                    case OPR_EQU:           commit_equ(node); break;
                    case OPR_LOW:           commit_low(node); break;
                    case OPR_NOT:           commit_not(node); break;
                    case OPR_AFFECT_LIST:   commit_affect_list(node); break;
                    case OPR_FOREACH:       commit_foreach(node); break;
                    case OPR_VAR_LIST:      commit_var_list(node); break;
#ifdef VERBOSE
                    case EMPTY_NODE:        printf("NOP\n"); break;
#endif
                }

                commit_node(node->unode.opr.next,0); /* recursive for the moment, maybe better to do something iterative? */
                break;

            case VAR_NODE:         gsl_instr_set_namespace(currentGoomSL->instr, node->vnamespace);
                                   gsl_instr_add_param(currentGoomSL->instr, node->str, TYPE_VAR); break;
            case CONST_INT_NODE:   gsl_instr_add_param(currentGoomSL->instr, node->str, TYPE_INTEGER); break;
            case CONST_FLOAT_NODE: gsl_instr_add_param(currentGoomSL->instr, node->str, TYPE_FLOAT); break;
            case CONST_PTR_NODE:   gsl_instr_add_param(currentGoomSL->instr, node->str, TYPE_PTR); break;
        }
        if (releaseIfTmp && is_tmp_expr(node))
          releaseTemp(get_tmp_id(node));
        
        nodeFree(node);
    } /* }}} */

    NodeType *nodeNew(const char *str, int type, int line_number) {
        NodeType *node = (NodeType*)malloc(sizeof(NodeType)); /* {{{ */
        node->type = type;
        node->str  = (char*)malloc(strlen(str)+1);
        node->vnamespace = NULL;
        node->line_number = line_number;
        strcpy(node->str, str);
        return node;
    } /* }}} */
    static NodeType *nodeClone(NodeType *node) {
        NodeType *ret = nodeNew(node->str, node->type, node->line_number); /* {{{ */
        ret->vnamespace = node->vnamespace;
        ret->unode = node->unode;
        return ret;
    } /* }}} */

    void nodeFreeInternals(NodeType *node) {
        free(node->str); /* {{{ */
    } /* }}} */
    
    void nodeFree(NodeType *node) {
        nodeFreeInternals(node); /* {{{ */
        free(node);
    } /* }}} */

    NodeType *new_constInt(const char *str, int line_number) {
        NodeType *node = nodeNew(str, CONST_INT_NODE, line_number); /* {{{ */
        node->unode.constInt.val = atoi(str);
        return node;
    } /* }}} */

    NodeType *new_constPtr(const char *str, int line_number) {
        NodeType *node = nodeNew(str, CONST_PTR_NODE, line_number); /* {{{ */
        node->unode.constPtr.id = strtol(str,NULL,0);
        return node;
    } /* }}} */

    NodeType *new_constFloat(const char *str, int line_number) {
        NodeType *node = nodeNew(str, CONST_FLOAT_NODE, line_number); /* {{{ */
        node->unode.constFloat.val = atof(str);
        return node;
    } /* }}} */

    NodeType *new_var(const char *str, int line_number) {
        NodeType *node = nodeNew(str, VAR_NODE, line_number); /* {{{ */
        node->vnamespace = gsl_find_namespace(str);
        if (node->vnamespace == 0) {
            fprintf(stderr, "ERROR: Line %d, Variable not found: '%s'\n", line_number, str);
            exit(1);
        }
        return node;
    } /* }}} */
    
    NodeType *new_nop(const char *str) {
        NodeType *node = new_op(str, EMPTY_NODE, 0); /* {{{ */
        return node;
    } /* }}} */
    
    NodeType *new_op(const char *str, int type, int nbOp) {
        int i; /* {{{ */
        NodeType *node = nodeNew(str, OPR_NODE, currentGoomSL->num_lines);
        node->unode.opr.next = 0;
        node->unode.opr.type = type;
        node->unode.opr.nbOp = nbOp;
        for (i=0;i<nbOp;++i) node->unode.opr.op[i] = 0;
        return node;
    } /* }}} */


    void gsl_declare_global_variable(int type, char *name) {
      switch(type){
        case -1: break;
        case FLOAT_TK:gsl_float_decl_global(name);break;
        case INT_TK:  gsl_int_decl_global(name);break;
        case PTR_TK:  gsl_ptr_decl_global(name);break;
        default:
        {
          int id = type - 1000;
          gsl_struct_decl_global_from_id(name,id);
        }
      }
    }

%}

%union {
    int intValue;
    float floatValue;
    char charValue;
    char strValue[2048];
    NodeType *nPtr;
    GoomHash *namespace;
    GSL_Struct *gsl_struct;
    GSL_StructField *gsl_struct_field;
  };
  
%token <strValue>   LTYPE_INTEGER
%token <strValue>   LTYPE_FLOAT
%token <strValue>   LTYPE_VAR
%token <strValue>   LTYPE_PTR

%token PTR_TK INT_TK FLOAT_TK DECLARE EXTERNAL WHILE DO NOT PLUS_EQ SUB_EQ DIV_EQ MUL_EQ SUP_EQ LOW_EQ NOT_EQ STRUCT FOR IN

%type <intValue> return_type
%type <nPtr> expression constValue instruction test func_call func_call_expression
%type <nPtr> start_block affectation_list affectation_in_list affectation declaration
%type <nPtr> var_list_content var_list
%type <strValue> task_name ext_task_name 
%type <namespace> leave_namespace
%type <gsl_struct> struct_members
%type <gsl_struct_field> struct_member
%left '\n'
%left PLUS_EQ SUB_EQ MUL_EQ DIV_EQ
%left NOT
%left '=' '<' '>'
%left '+' '-'
%left '/' '*'

%%

/* -------------- Global architechture of a GSL program ------------*/

gsl: gsl_code function_outro gsl_def_functions ;

gsl_code: gsl_code instruction              { gsl_append($2); }
   | gsl_code EXTERNAL '<' ext_task_name '>' return_type '\n' leave_namespace             { gsl_declare_global_variable($6,$4); }
   | gsl_code EXTERNAL '<' ext_task_name ':' arglist '>' return_type '\n' leave_namespace { gsl_declare_global_variable($8,$4); }
   | gsl_code DECLARE '<' task_name '>' return_type  '\n' leave_namespace                 { gsl_declare_global_variable($6,$4); }
   | gsl_code DECLARE '<' task_name ':' arglist '>' return_type '\n' leave_namespace      { gsl_declare_global_variable($8,$4); }
   | gsl_code struct_declaration
   | gsl_code '\n'
   |
   ;

/* ------------- Declaration of a structure ------------ */

struct_declaration: STRUCT  '<' LTYPE_VAR  ':' struct_members '>' '\n' { gsl_add_struct($3, $5); }
                  ;

struct_members: opt_nl struct_member                    { $$ = gsl_new_struct($2);               }
              | struct_members ',' opt_nl struct_member { $$ = $1; gsl_add_struct_field($1, $4); }
              ;

struct_member: INT_TK    LTYPE_VAR { $$ = gsl_new_struct_field($2, INSTR_INT); }
             | FLOAT_TK  LTYPE_VAR { $$ = gsl_new_struct_field($2, INSTR_FLOAT); }
             | PTR_TK    LTYPE_VAR { $$ = gsl_new_struct_field($2, INSTR_PTR); }
             | LTYPE_VAR LTYPE_VAR { $$ = gsl_new_struct_field_struct($2, $1); }
             ;

/* ------------- Fonction declarations -------------- */

ext_task_name: LTYPE_VAR { gsl_declare_external_task($1); gsl_enternamespace($1); strcpy($$,$1); }
             ;
task_name:     LTYPE_VAR { gsl_declare_task($1); gsl_enternamespace($1); strcpy($$,$1); strcpy($$,$1); }
         ;

return_type:      { $$=-1; }
  | ':' INT_TK    { $$=INT_TK; }
  | ':' FLOAT_TK  { $$=FLOAT_TK; }
  | ':' PTR_TK    { $$=PTR_TK; }
  | ':' LTYPE_VAR { $$= 1000 + gsl_get_struct_id($2); }
  ;

arglist: empty_declaration
       | empty_declaration ',' arglist
       ;

/* ------------- Fonction definition -------------- */

gsl_def_functions: gsl_def_functions function
                 |
                 ;

function: function_intro gsl_code function_outro { gsl_leavenamespace(); }

function_intro: '<' task_name '>' return_type '\n'             { gsl_append(new_function_intro($2));
                                                                 gsl_declare_global_variable($4,$2); }
              | '<' task_name ':' arglist '>' return_type '\n' { gsl_append(new_function_intro($2));
                                                                 gsl_declare_global_variable($6,$2); }
              ;
function_outro: { gsl_append(new_function_outro()); } ;

leave_namespace:      { $$ = gsl_leavenamespace();   };

/* ------------ Variable declaration ---------------- */

declaration: FLOAT_TK LTYPE_VAR '=' expression { gsl_float_decl_local($2); $$ = new_set(new_var($2,currentGoomSL->num_lines), $4); }
           | INT_TK   LTYPE_VAR '=' expression { gsl_int_decl_local($2);   $$ = new_set(new_var($2,currentGoomSL->num_lines), $4); }
           | PTR_TK   LTYPE_VAR '=' expression { gsl_ptr_decl_local($2);   $$ = new_set(new_var($2,currentGoomSL->num_lines), $4); }
           | LTYPE_VAR LTYPE_VAR '=' expression { gsl_struct_decl_local($1,$2); $$ = new_set(new_var($2,currentGoomSL->num_lines), $4); }
           | empty_declaration                { $$ = 0; }
           ;

empty_declaration: FLOAT_TK  LTYPE_VAR { gsl_float_decl_local($2);       }
                 | INT_TK    LTYPE_VAR { gsl_int_decl_local($2);         }
                 | PTR_TK    LTYPE_VAR { gsl_ptr_decl_local($2);         }
                 | LTYPE_VAR LTYPE_VAR { gsl_struct_decl_local($1,$2);   }
                 ;

/* -------------- Instructions and Expressions ------------------ */

instruction: affectation '\n' { $$ = $1; }
           | declaration '\n' { $$ = $1; }
           | '(' test ')' '?' opt_nl instruction     { $$ = new_if($2,$6); }
           | WHILE test opt_nl DO opt_nl instruction { $$ = new_while($2,$6); }
           | '{' '\n' start_block gsl_code '}' '\n'  { lastNode = $3->unode.opr.op[1]; $$=$3; }
           | func_call                               { $$ = $1; }
           | LTYPE_VAR PLUS_EQ expression { $$ = new_plus_eq(new_var($1,currentGoomSL->num_lines),$3); }
           | LTYPE_VAR SUB_EQ expression  { $$ = new_sub_eq(new_var($1,currentGoomSL->num_lines),$3); }
           | LTYPE_VAR MUL_EQ expression  { $$ = new_mul_eq(new_var($1,currentGoomSL->num_lines),$3); }
           | LTYPE_VAR DIV_EQ expression  { $$ = new_div_eq(new_var($1,currentGoomSL->num_lines),$3); }
           | FOR LTYPE_VAR IN var_list DO instruction { $$ = new_static_foreach(new_var($2, currentGoomSL->num_lines), $4, $6); }
           ;

var_list: '(' var_list_content ')'      { $$ = $2; }
        ;
var_list_content: LTYPE_VAR             { $$ = new_var_list(new_var($1,currentGoomSL->num_lines), NULL); }
           | LTYPE_VAR var_list_content { $$ = new_var_list(new_var($1,currentGoomSL->num_lines), $2);   }
           ;

affectation: LTYPE_VAR '=' expression { $$ = new_set(new_var($1,currentGoomSL->num_lines),$3); } ;

start_block: { $$ = new_block(lastNode); lastNode = $$->unode.opr.op[0]; }
           ;

expression: LTYPE_VAR   { $$ = new_var($1,currentGoomSL->num_lines); }
          | constValue  { $$ = $1; }
          | expression '*' expression { $$ = new_mul($1,$3); } 
          | expression '/' expression { $$ = new_div($1,$3); } 
          | expression '+' expression { $$ = new_add($1,$3); } 
          | expression '-' expression { $$ = new_sub($1,$3); } 
          | '-' expression            { $$ = new_neg($2);    }
          | '(' expression ')'        { $$ = $2; }
          | func_call_expression      { $$ = $1; }
          ;

test: expression '=' expression { $$ = new_equ($1,$3); } 
    | expression '<' expression { $$ = new_low($1,$3); } 
    | expression '>' expression { $$ = new_low($3,$1); }
    | expression SUP_EQ expression { $$ = new_not(new_low($1,$3)); }
    | expression LOW_EQ expression { $$ = new_not(new_low($3,$1)); }
    | expression NOT_EQ expression { $$ = new_not(new_equ($1,$3)); }
    | NOT test                  { $$ = new_not($2);    }
    ;

constValue: LTYPE_FLOAT   { $$ = new_constFloat($1,currentGoomSL->num_lines); }
          | LTYPE_INTEGER { $$ = new_constInt($1,currentGoomSL->num_lines); } 
          | LTYPE_PTR     { $$ = new_constPtr($1,currentGoomSL->num_lines); } 
          ;

/* ---------------- Function Calls ------------------ */

func_call:   task_name '\n' leave_namespace                          { $$ = new_call($1,NULL); }
           | task_name ':' affectation_list '\n' leave_namespace         { $$ = new_call($1,$3); }
           | '[' task_name ']' '\n' leave_namespace                  { $$ = new_call($2,NULL); }
           | '[' task_name ':' affectation_list ']' '\n' leave_namespace { $$ = new_call($2,$4); }
           ;

func_call_expression:
            '[' task_name leave_namespace ']'                      { $$ = new_call_expr($2,NULL); }
          | '[' task_name ':' affectation_list ']' leave_namespace { $$ = new_call_expr($2,$4); }
          ;
             
affectation_list: affectation_in_list affectation_list     { $$ = new_affec_list($1,$2);   }
            | affectation_in_list                          { $$ = new_affec_list($1,NULL); }

affectation_in_list: LTYPE_VAR '=' leave_namespace expression {
                              gsl_reenternamespace($3);
                              $$ = new_set(new_var($1,currentGoomSL->num_lines),$4);
                            }
                   | ':' leave_namespace expression {
                              gsl_reenternamespace($2);
                              $$ = new_set(new_var("&this", currentGoomSL->num_lines),$3);
                            }
                   ;


/* ------------ Misc ---------- */

opt_nl: '\n' | ;
           

%%


void yyerror(char *str)
{ /* {{{ */
    fprintf(stderr, "ERROR: Line %d, %s\n", currentGoomSL->num_lines, str);
    currentGoomSL->compilationOK = 0;
    exit(1);
} /* }}} */

