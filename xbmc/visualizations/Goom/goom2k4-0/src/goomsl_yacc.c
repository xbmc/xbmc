/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     LTYPE_INTEGER = 258,
     LTYPE_FLOAT = 259,
     LTYPE_VAR = 260,
     LTYPE_PTR = 261,
     PTR_TK = 262,
     INT_TK = 263,
     FLOAT_TK = 264,
     DECLARE = 265,
     EXTERNAL = 266,
     WHILE = 267,
     DO = 268,
     NOT = 269,
     PLUS_EQ = 270,
     SUB_EQ = 271,
     DIV_EQ = 272,
     MUL_EQ = 273,
     SUP_EQ = 274,
     LOW_EQ = 275,
     NOT_EQ = 276,
     STRUCT = 277,
     FOR = 278,
     IN = 279
   };
#endif
#define LTYPE_INTEGER 258
#define LTYPE_FLOAT 259
#define LTYPE_VAR 260
#define LTYPE_PTR 261
#define PTR_TK 262
#define INT_TK 263
#define FLOAT_TK 264
#define DECLARE 265
#define EXTERNAL 266
#define WHILE 267
#define DO 268
#define NOT 269
#define PLUS_EQ 270
#define SUB_EQ 271
#define DIV_EQ 272
#define MUL_EQ 273
#define SUP_EQ 274
#define LOW_EQ 275
#define NOT_EQ 276
#define STRUCT 277
#define FOR 278
#define IN 279




/* Copy the first part of user declarations.  */
#line 6 "goomsl_yacc.y"

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



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 1199 "goomsl_yacc.y"
typedef union YYSTYPE {
    int intValue;
    float floatValue;
    char charValue;
    char strValue[2048];
    NodeType *nPtr;
    GoomHash *namespace;
    GSL_Struct *gsl_struct;
    GSL_StructField *gsl_struct_field;
  } YYSTYPE;
/* Line 191 of yacc.c.  */
#line 1327 "goomsl_yacc.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 1339 "goomsl_yacc.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   229

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  42
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  30
/* YYNRULES -- Number of rules. */
#define YYNRULES  89
/* YYNRULES -- Number of states. */
#define YYNSTATES  217

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   279

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      25,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      35,    36,    32,    29,    34,    30,     2,    31,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    33,     2,
      27,    26,    28,    37,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    40,     2,    41,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    38,     2,    39,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     7,    10,    19,    30,    39,    50,    53,
      56,    57,    65,    68,    73,    76,    79,    82,    85,    87,
      89,    90,    93,    96,    99,   102,   104,   108,   111,   112,
     116,   122,   130,   131,   132,   137,   142,   147,   152,   154,
     157,   160,   163,   166,   169,   172,   179,   186,   193,   195,
     199,   203,   207,   211,   218,   222,   224,   227,   231,   232,
     234,   236,   240,   244,   248,   252,   255,   259,   261,   265,
     269,   273,   277,   281,   285,   288,   290,   292,   294,   298,
     304,   310,   318,   323,   330,   333,   335,   340,   344,   346
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      43,     0,    -1,    44,    55,    52,    -1,    44,    59,    -1,
      44,    11,    27,    48,    28,    50,    25,    56,    -1,    44,
      11,    27,    48,    33,    51,    28,    50,    25,    56,    -1,
      44,    10,    27,    49,    28,    50,    25,    56,    -1,    44,
      10,    27,    49,    33,    51,    28,    50,    25,    56,    -1,
      44,    45,    -1,    44,    25,    -1,    -1,    22,    27,     5,
      33,    46,    28,    25,    -1,    71,    47,    -1,    46,    34,
      71,    47,    -1,     8,     5,    -1,     9,     5,    -1,     7,
       5,    -1,     5,     5,    -1,     5,    -1,     5,    -1,    -1,
      33,     8,    -1,    33,     9,    -1,    33,     7,    -1,    33,
       5,    -1,    58,    -1,    58,    34,    51,    -1,    52,    53,
      -1,    -1,    54,    44,    55,    -1,    27,    49,    28,    50,
      25,    -1,    27,    49,    33,    51,    28,    50,    25,    -1,
      -1,    -1,     9,     5,    26,    64,    -1,     8,     5,    26,
      64,    -1,     7,     5,    26,    64,    -1,     5,     5,    26,
      64,    -1,    58,    -1,     9,     5,    -1,     8,     5,    -1,
       7,     5,    -1,     5,     5,    -1,    62,    25,    -1,    57,
      25,    -1,    35,    65,    36,    37,    71,    59,    -1,    12,
      65,    71,    13,    71,    59,    -1,    38,    25,    63,    44,
      39,    25,    -1,    67,    -1,     5,    15,    64,    -1,     5,
      16,    64,    -1,     5,    18,    64,    -1,     5,    17,    64,
      -1,    23,     5,    24,    60,    13,    59,    -1,    35,    61,
      36,    -1,     5,    -1,     5,    61,    -1,     5,    26,    64,
      -1,    -1,     5,    -1,    66,    -1,    64,    32,    64,    -1,
      64,    31,    64,    -1,    64,    29,    64,    -1,    64,    30,
      64,    -1,    30,    64,    -1,    35,    64,    36,    -1,    68,
      -1,    64,    26,    64,    -1,    64,    27,    64,    -1,    64,
      28,    64,    -1,    64,    19,    64,    -1,    64,    20,    64,
      -1,    64,    21,    64,    -1,    14,    65,    -1,     4,    -1,
       3,    -1,     6,    -1,    49,    25,    56,    -1,    49,    33,
      69,    25,    56,    -1,    40,    49,    41,    25,    56,    -1,
      40,    49,    33,    69,    41,    25,    56,    -1,    40,    49,
      56,    41,    -1,    40,    49,    33,    69,    41,    56,    -1,
      70,    69,    -1,    70,    -1,     5,    26,    56,    64,    -1,
      33,    56,    64,    -1,    25,    -1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,  1236,  1236,  1238,  1239,  1240,  1241,  1242,  1243,  1244,
    1245,  1250,  1253,  1254,  1257,  1258,  1259,  1260,  1265,  1267,
    1270,  1271,  1272,  1273,  1274,  1277,  1278,  1283,  1284,  1287,
    1289,  1291,  1294,  1296,  1300,  1301,  1302,  1303,  1304,  1307,
    1308,  1309,  1310,  1315,  1316,  1317,  1318,  1319,  1320,  1321,
    1322,  1323,  1324,  1325,  1328,  1330,  1331,  1334,  1336,  1339,
    1340,  1341,  1342,  1343,  1344,  1345,  1346,  1347,  1350,  1351,
    1352,  1353,  1354,  1355,  1356,  1359,  1360,  1361,  1366,  1367,
    1368,  1369,  1373,  1374,  1377,  1378,  1380,  1384,  1393,  1393
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "LTYPE_INTEGER", "LTYPE_FLOAT", 
  "LTYPE_VAR", "LTYPE_PTR", "PTR_TK", "INT_TK", "FLOAT_TK", "DECLARE", 
  "EXTERNAL", "WHILE", "DO", "NOT", "PLUS_EQ", "SUB_EQ", "DIV_EQ", 
  "MUL_EQ", "SUP_EQ", "LOW_EQ", "NOT_EQ", "STRUCT", "FOR", "IN", "'\\n'", 
  "'='", "'<'", "'>'", "'+'", "'-'", "'/'", "'*'", "':'", "','", "'('", 
  "')'", "'?'", "'{'", "'}'", "'['", "']'", "$accept", "gsl", "gsl_code", 
  "struct_declaration", "struct_members", "struct_member", 
  "ext_task_name", "task_name", "return_type", "arglist", 
  "gsl_def_functions", "function", "function_intro", "function_outro", 
  "leave_namespace", "declaration", "empty_declaration", "instruction", 
  "var_list", "var_list_content", "affectation", "start_block", 
  "expression", "test", "constValue", "func_call", "func_call_expression", 
  "affectation_list", "affectation_in_list", "opt_nl", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,    10,    61,    60,    62,    43,
      45,    47,    42,    58,    44,    40,    41,    63,   123,   125,
      91,    93
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    42,    43,    44,    44,    44,    44,    44,    44,    44,
      44,    45,    46,    46,    47,    47,    47,    47,    48,    49,
      50,    50,    50,    50,    50,    51,    51,    52,    52,    53,
      54,    54,    55,    56,    57,    57,    57,    57,    57,    58,
      58,    58,    58,    59,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    60,    61,    61,    62,    63,    64,
      64,    64,    64,    64,    64,    64,    64,    64,    65,    65,
      65,    65,    65,    65,    65,    66,    66,    66,    67,    67,
      67,    67,    68,    68,    69,    69,    70,    70,    71,    71
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     3,     2,     8,    10,     8,    10,     2,     2,
       0,     7,     2,     4,     2,     2,     2,     2,     1,     1,
       0,     2,     2,     2,     2,     1,     3,     2,     0,     3,
       5,     7,     0,     0,     4,     4,     4,     4,     1,     2,
       2,     2,     2,     2,     2,     6,     6,     6,     1,     3,
       3,     3,     3,     6,     3,     1,     2,     3,     0,     1,
       1,     3,     3,     3,     3,     2,     3,     1,     3,     3,
       3,     3,     3,     3,     2,     1,     1,     1,     3,     5,
       5,     7,     4,     6,     2,     1,     4,     3,     1,     0
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
      10,     0,    32,     1,    19,     0,     0,     0,     0,     0,
       0,     0,     0,     9,     0,     0,     0,     8,     0,    28,
       0,    38,     3,     0,    48,    42,     0,     0,     0,     0,
       0,    41,    40,    39,     0,     0,    76,    75,    59,    77,
       0,     0,     0,     0,     0,    89,    60,    67,     0,     0,
       0,    58,    19,     0,    33,     0,     2,    44,    43,     0,
      49,    50,    52,    51,    57,     0,     0,     0,     0,    18,
       0,    74,    65,     0,    33,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    88,     0,     0,     0,     0,
      10,     0,     0,    78,     0,    33,     0,    85,     0,    27,
      10,    37,    36,    35,    34,    20,     0,    20,     0,    66,
       0,     0,    71,    72,    73,    68,    69,    70,    63,    64,
      62,    61,    89,    89,     0,     0,    89,     0,     0,    33,
      33,     0,    33,    84,     0,    32,     0,     0,     0,     0,
       0,     0,     0,    25,     0,     0,     0,    82,     0,     0,
       0,    55,     0,     0,     0,     0,     0,    80,     0,    87,
      79,    20,     0,    29,    24,    23,    21,    22,    33,    42,
      41,    40,    39,    20,     0,    33,    20,    33,    46,     0,
      89,     0,     0,     0,     0,    12,    56,    54,    53,    45,
      47,    33,    86,     0,     0,     6,     0,    26,     4,     0,
      83,    11,     0,    17,    16,    14,    15,    81,    30,    20,
      33,    33,    13,     0,     7,     5,    31
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,     1,     2,    17,   149,   185,    70,    18,   137,   142,
      56,    99,   100,    19,    93,    20,    21,    22,   125,   152,
      23,    90,    44,    45,    46,    24,    47,    96,    97,    86
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -116
static const short yypact[] =
{
    -116,    40,   136,  -116,   103,    39,    66,    68,    61,    65,
       1,    77,   101,  -116,     1,    84,   109,  -116,    12,  -116,
      91,  -116,  -116,    97,  -116,    98,    72,    72,    72,    72,
      72,    99,   104,   113,   109,   130,  -116,  -116,  -116,  -116,
       1,    72,    72,   109,   166,   115,  -116,  -116,   145,   131,
     118,  -116,  -116,   -24,  -116,    -3,   138,  -116,  -116,    72,
     159,   159,   159,   159,   159,    72,    72,    72,    14,  -116,
      51,  -116,    22,   102,   124,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,  -116,   160,   139,   140,   141,
    -116,    -3,   152,  -116,   154,  -116,   156,    -3,   109,  -116,
    -116,   159,   159,   159,   159,   150,    82,   150,    82,  -116,
      -3,   158,   159,   159,   159,   159,   159,   159,    22,    22,
    -116,  -116,   115,   115,   195,   188,   115,    88,   162,  -116,
    -116,    72,  -116,  -116,    52,   136,   155,   177,   199,   200,
     201,   202,   180,   175,   185,   183,   171,  -116,   144,    18,
     161,   195,   178,   144,   144,   190,   191,  -116,    72,   159,
    -116,   150,    82,  -116,  -116,  -116,  -116,  -116,  -116,  -116,
    -116,  -116,  -116,   150,    82,  -116,   150,  -116,  -116,   192,
     115,   208,   213,   214,   215,  -116,  -116,  -116,  -116,  -116,
    -116,  -116,   159,   196,   194,  -116,   198,  -116,  -116,   203,
    -116,  -116,   161,  -116,  -116,  -116,  -116,  -116,  -116,   150,
    -116,  -116,  -116,   204,  -116,  -116,  -116
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
    -116,  -116,   -68,  -116,  -116,    23,  -116,   -15,  -104,   -92,
    -116,  -116,  -116,    89,   -74,  -116,   -88,  -115,  -116,    75,
    -116,  -116,   -16,    -6,  -116,  -116,  -116,   -62,  -116,   -99
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
     111,    53,    94,   144,    36,    37,    38,    39,    50,    91,
      60,    61,    62,    63,    64,    40,   145,    92,   143,    68,
     143,   131,   127,   148,   150,    72,    73,   154,    74,   128,
      95,    41,   135,   178,    71,   133,    42,    54,   188,   189,
       3,    43,   105,   101,    31,    55,   179,   106,   146,   102,
     103,   104,   180,    83,    84,   157,   158,   193,   160,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   196,
     194,    32,   199,    33,   143,    36,    37,    38,    39,   107,
     161,   202,   197,   134,   108,   162,   143,   138,    34,   139,
     140,   141,    35,     4,   195,     5,     6,     7,     8,     9,
      10,   198,    41,   200,    48,   213,    49,    42,    25,    51,
      11,    12,    43,    13,    52,   159,    57,   207,    26,    27,
      28,    29,    58,    14,    59,    65,    15,   155,    16,    30,
      66,    81,    82,    83,    84,    69,   214,   215,   109,    67,
      85,     4,   192,     5,     6,     7,     8,     9,    10,     4,
      87,     5,     6,     7,    89,    88,    10,   110,    11,    12,
     164,    13,   165,   166,   167,    98,   181,    12,   182,   183,
     184,    14,   123,   122,    15,   124,    16,   129,   126,    14,
     130,   132,    15,   136,    16,    75,    76,    77,    81,    82,
      83,    84,    78,    79,    80,    81,    82,    83,    84,   147,
     151,   153,   168,   156,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   203,   187,   190,   191,   201,   204,   205,
     206,   208,   209,   210,   163,   212,   186,     0,   211,   216
};

static const short yycheck[] =
{
      74,    16,     5,   107,     3,     4,     5,     6,    14,    33,
      26,    27,    28,    29,    30,    14,   108,    41,   106,    34,
     108,    95,    90,   122,   123,    41,    42,   126,    43,    91,
      33,    30,   100,   148,    40,    97,    35,    25,   153,   154,
       0,    40,    28,    59,     5,    33,    28,    33,   110,    65,
      66,    67,    34,    31,    32,   129,   130,   161,   132,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,   173,
     162,     5,   176,     5,   162,     3,     4,     5,     6,    28,
      28,   180,   174,    98,    33,    33,   174,     5,    27,     7,
       8,     9,    27,     5,   168,     7,     8,     9,    10,    11,
      12,   175,    30,   177,    27,   209,     5,    35,     5,    25,
      22,    23,    40,    25,     5,   131,    25,   191,    15,    16,
      17,    18,    25,    35,    26,    26,    38,    39,    40,    26,
      26,    29,    30,    31,    32,     5,   210,   211,    36,    26,
      25,     5,   158,     7,     8,     9,    10,    11,    12,     5,
       5,     7,     8,     9,    36,    24,    12,    33,    22,    23,
       5,    25,     7,     8,     9,    27,     5,    23,     7,     8,
       9,    35,    33,    13,    38,    35,    40,    25,    37,    35,
      26,    25,    38,    33,    40,    19,    20,    21,    29,    30,
      31,    32,    26,    27,    28,    29,    30,    31,    32,    41,
       5,    13,    25,    41,     5,     5,     5,     5,    28,    34,
      25,    28,    41,     5,    36,    25,    25,    25,     5,     5,
       5,    25,    28,    25,   135,   202,   151,    -1,    25,    25
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    43,    44,     0,     5,     7,     8,     9,    10,    11,
      12,    22,    23,    25,    35,    38,    40,    45,    49,    55,
      57,    58,    59,    62,    67,     5,    15,    16,    17,    18,
      26,     5,     5,     5,    27,    27,     3,     4,     5,     6,
      14,    30,    35,    40,    64,    65,    66,    68,    27,     5,
      65,    25,     5,    49,    25,    33,    52,    25,    25,    26,
      64,    64,    64,    64,    64,    26,    26,    26,    49,     5,
      48,    65,    64,    64,    49,    19,    20,    21,    26,    27,
      28,    29,    30,    31,    32,    25,    71,     5,    24,    36,
      63,    33,    41,    56,     5,    33,    69,    70,    27,    53,
      54,    64,    64,    64,    64,    28,    33,    28,    33,    36,
      33,    56,    64,    64,    64,    64,    64,    64,    64,    64,
      64,    64,    13,    33,    35,    60,    37,    44,    69,    25,
      26,    56,    25,    69,    49,    44,    33,    50,     5,     7,
       8,     9,    51,    58,    50,    51,    69,    41,    71,    46,
      71,     5,    61,    13,    71,    39,    41,    56,    56,    64,
      56,    28,    33,    55,     5,     7,     8,     9,    25,     5,
       5,     5,     5,    28,    34,    25,    28,    41,    59,    28,
      34,     5,     7,     8,     9,    47,    61,    36,    59,    59,
      25,    25,    64,    50,    51,    56,    50,    51,    56,    50,
      56,    25,    71,     5,     5,     5,     5,    56,    25,    28,
      25,    25,    47,    50,    56,    56,    25
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:
#line 1238 "goomsl_yacc.y"
    { gsl_append(yyvsp[0].nPtr); }
    break;

  case 4:
#line 1239 "goomsl_yacc.y"
    { gsl_declare_global_variable(yyvsp[-2].intValue,yyvsp[-4].strValue); }
    break;

  case 5:
#line 1240 "goomsl_yacc.y"
    { gsl_declare_global_variable(yyvsp[-2].intValue,yyvsp[-6].strValue); }
    break;

  case 6:
#line 1241 "goomsl_yacc.y"
    { gsl_declare_global_variable(yyvsp[-2].intValue,yyvsp[-4].strValue); }
    break;

  case 7:
#line 1242 "goomsl_yacc.y"
    { gsl_declare_global_variable(yyvsp[-2].intValue,yyvsp[-6].strValue); }
    break;

  case 11:
#line 1250 "goomsl_yacc.y"
    { gsl_add_struct(yyvsp[-4].strValue, yyvsp[-2].gsl_struct); }
    break;

  case 12:
#line 1253 "goomsl_yacc.y"
    { yyval.gsl_struct = gsl_new_struct(yyvsp[0].gsl_struct_field);               }
    break;

  case 13:
#line 1254 "goomsl_yacc.y"
    { yyval.gsl_struct = yyvsp[-3].gsl_struct; gsl_add_struct_field(yyvsp[-3].gsl_struct, yyvsp[0].gsl_struct_field); }
    break;

  case 14:
#line 1257 "goomsl_yacc.y"
    { yyval.gsl_struct_field = gsl_new_struct_field(yyvsp[0].strValue, INSTR_INT); }
    break;

  case 15:
#line 1258 "goomsl_yacc.y"
    { yyval.gsl_struct_field = gsl_new_struct_field(yyvsp[0].strValue, INSTR_FLOAT); }
    break;

  case 16:
#line 1259 "goomsl_yacc.y"
    { yyval.gsl_struct_field = gsl_new_struct_field(yyvsp[0].strValue, INSTR_PTR); }
    break;

  case 17:
#line 1260 "goomsl_yacc.y"
    { yyval.gsl_struct_field = gsl_new_struct_field_struct(yyvsp[0].strValue, yyvsp[-1].strValue); }
    break;

  case 18:
#line 1265 "goomsl_yacc.y"
    { gsl_declare_external_task(yyvsp[0].strValue); gsl_enternamespace(yyvsp[0].strValue); strcpy(yyval.strValue,yyvsp[0].strValue); }
    break;

  case 19:
#line 1267 "goomsl_yacc.y"
    { gsl_declare_task(yyvsp[0].strValue); gsl_enternamespace(yyvsp[0].strValue); strcpy(yyval.strValue,yyvsp[0].strValue); strcpy(yyval.strValue,yyvsp[0].strValue); }
    break;

  case 20:
#line 1270 "goomsl_yacc.y"
    { yyval.intValue=-1; }
    break;

  case 21:
#line 1271 "goomsl_yacc.y"
    { yyval.intValue=INT_TK; }
    break;

  case 22:
#line 1272 "goomsl_yacc.y"
    { yyval.intValue=FLOAT_TK; }
    break;

  case 23:
#line 1273 "goomsl_yacc.y"
    { yyval.intValue=PTR_TK; }
    break;

  case 24:
#line 1274 "goomsl_yacc.y"
    { yyval.intValue= 1000 + gsl_get_struct_id(yyvsp[0].strValue); }
    break;

  case 29:
#line 1287 "goomsl_yacc.y"
    { gsl_leavenamespace(); }
    break;

  case 30:
#line 1289 "goomsl_yacc.y"
    { gsl_append(new_function_intro(yyvsp[-3].strValue));
                                                                 gsl_declare_global_variable(yyvsp[-1].intValue,yyvsp[-3].strValue); }
    break;

  case 31:
#line 1291 "goomsl_yacc.y"
    { gsl_append(new_function_intro(yyvsp[-5].strValue));
                                                                 gsl_declare_global_variable(yyvsp[-1].intValue,yyvsp[-5].strValue); }
    break;

  case 32:
#line 1294 "goomsl_yacc.y"
    { gsl_append(new_function_outro()); }
    break;

  case 33:
#line 1296 "goomsl_yacc.y"
    { yyval.namespace = gsl_leavenamespace();   }
    break;

  case 34:
#line 1300 "goomsl_yacc.y"
    { gsl_float_decl_local(yyvsp[-2].strValue); yyval.nPtr = new_set(new_var(yyvsp[-2].strValue,currentGoomSL->num_lines), yyvsp[0].nPtr); }
    break;

  case 35:
#line 1301 "goomsl_yacc.y"
    { gsl_int_decl_local(yyvsp[-2].strValue);   yyval.nPtr = new_set(new_var(yyvsp[-2].strValue,currentGoomSL->num_lines), yyvsp[0].nPtr); }
    break;

  case 36:
#line 1302 "goomsl_yacc.y"
    { gsl_ptr_decl_local(yyvsp[-2].strValue);   yyval.nPtr = new_set(new_var(yyvsp[-2].strValue,currentGoomSL->num_lines), yyvsp[0].nPtr); }
    break;

  case 37:
#line 1303 "goomsl_yacc.y"
    { gsl_struct_decl_local(yyvsp[-3].strValue,yyvsp[-2].strValue); yyval.nPtr = new_set(new_var(yyvsp[-2].strValue,currentGoomSL->num_lines), yyvsp[0].nPtr); }
    break;

  case 38:
#line 1304 "goomsl_yacc.y"
    { yyval.nPtr = 0; }
    break;

  case 39:
#line 1307 "goomsl_yacc.y"
    { gsl_float_decl_local(yyvsp[0].strValue);       }
    break;

  case 40:
#line 1308 "goomsl_yacc.y"
    { gsl_int_decl_local(yyvsp[0].strValue);         }
    break;

  case 41:
#line 1309 "goomsl_yacc.y"
    { gsl_ptr_decl_local(yyvsp[0].strValue);         }
    break;

  case 42:
#line 1310 "goomsl_yacc.y"
    { gsl_struct_decl_local(yyvsp[-1].strValue,yyvsp[0].strValue);   }
    break;

  case 43:
#line 1315 "goomsl_yacc.y"
    { yyval.nPtr = yyvsp[-1].nPtr; }
    break;

  case 44:
#line 1316 "goomsl_yacc.y"
    { yyval.nPtr = yyvsp[-1].nPtr; }
    break;

  case 45:
#line 1317 "goomsl_yacc.y"
    { yyval.nPtr = new_if(yyvsp[-4].nPtr,yyvsp[0].nPtr); }
    break;

  case 46:
#line 1318 "goomsl_yacc.y"
    { yyval.nPtr = new_while(yyvsp[-4].nPtr,yyvsp[0].nPtr); }
    break;

  case 47:
#line 1319 "goomsl_yacc.y"
    { lastNode = yyvsp[-3].nPtr->unode.opr.op[1]; yyval.nPtr=yyvsp[-3].nPtr; }
    break;

  case 48:
#line 1320 "goomsl_yacc.y"
    { yyval.nPtr = yyvsp[0].nPtr; }
    break;

  case 49:
#line 1321 "goomsl_yacc.y"
    { yyval.nPtr = new_plus_eq(new_var(yyvsp[-2].strValue,currentGoomSL->num_lines),yyvsp[0].nPtr); }
    break;

  case 50:
#line 1322 "goomsl_yacc.y"
    { yyval.nPtr = new_sub_eq(new_var(yyvsp[-2].strValue,currentGoomSL->num_lines),yyvsp[0].nPtr); }
    break;

  case 51:
#line 1323 "goomsl_yacc.y"
    { yyval.nPtr = new_mul_eq(new_var(yyvsp[-2].strValue,currentGoomSL->num_lines),yyvsp[0].nPtr); }
    break;

  case 52:
#line 1324 "goomsl_yacc.y"
    { yyval.nPtr = new_div_eq(new_var(yyvsp[-2].strValue,currentGoomSL->num_lines),yyvsp[0].nPtr); }
    break;

  case 53:
#line 1325 "goomsl_yacc.y"
    { yyval.nPtr = new_static_foreach(new_var(yyvsp[-4].strValue, currentGoomSL->num_lines), yyvsp[-2].nPtr, yyvsp[0].nPtr); }
    break;

  case 54:
#line 1328 "goomsl_yacc.y"
    { yyval.nPtr = yyvsp[-1].nPtr; }
    break;

  case 55:
#line 1330 "goomsl_yacc.y"
    { yyval.nPtr = new_var_list(new_var(yyvsp[0].strValue,currentGoomSL->num_lines), NULL); }
    break;

  case 56:
#line 1331 "goomsl_yacc.y"
    { yyval.nPtr = new_var_list(new_var(yyvsp[-1].strValue,currentGoomSL->num_lines), yyvsp[0].nPtr);   }
    break;

  case 57:
#line 1334 "goomsl_yacc.y"
    { yyval.nPtr = new_set(new_var(yyvsp[-2].strValue,currentGoomSL->num_lines),yyvsp[0].nPtr); }
    break;

  case 58:
#line 1336 "goomsl_yacc.y"
    { yyval.nPtr = new_block(lastNode); lastNode = yyval.nPtr->unode.opr.op[0]; }
    break;

  case 59:
#line 1339 "goomsl_yacc.y"
    { yyval.nPtr = new_var(yyvsp[0].strValue,currentGoomSL->num_lines); }
    break;

  case 60:
#line 1340 "goomsl_yacc.y"
    { yyval.nPtr = yyvsp[0].nPtr; }
    break;

  case 61:
#line 1341 "goomsl_yacc.y"
    { yyval.nPtr = new_mul(yyvsp[-2].nPtr,yyvsp[0].nPtr); }
    break;

  case 62:
#line 1342 "goomsl_yacc.y"
    { yyval.nPtr = new_div(yyvsp[-2].nPtr,yyvsp[0].nPtr); }
    break;

  case 63:
#line 1343 "goomsl_yacc.y"
    { yyval.nPtr = new_add(yyvsp[-2].nPtr,yyvsp[0].nPtr); }
    break;

  case 64:
#line 1344 "goomsl_yacc.y"
    { yyval.nPtr = new_sub(yyvsp[-2].nPtr,yyvsp[0].nPtr); }
    break;

  case 65:
#line 1345 "goomsl_yacc.y"
    { yyval.nPtr = new_neg(yyvsp[0].nPtr);    }
    break;

  case 66:
#line 1346 "goomsl_yacc.y"
    { yyval.nPtr = yyvsp[-1].nPtr; }
    break;

  case 67:
#line 1347 "goomsl_yacc.y"
    { yyval.nPtr = yyvsp[0].nPtr; }
    break;

  case 68:
#line 1350 "goomsl_yacc.y"
    { yyval.nPtr = new_equ(yyvsp[-2].nPtr,yyvsp[0].nPtr); }
    break;

  case 69:
#line 1351 "goomsl_yacc.y"
    { yyval.nPtr = new_low(yyvsp[-2].nPtr,yyvsp[0].nPtr); }
    break;

  case 70:
#line 1352 "goomsl_yacc.y"
    { yyval.nPtr = new_low(yyvsp[0].nPtr,yyvsp[-2].nPtr); }
    break;

  case 71:
#line 1353 "goomsl_yacc.y"
    { yyval.nPtr = new_not(new_low(yyvsp[-2].nPtr,yyvsp[0].nPtr)); }
    break;

  case 72:
#line 1354 "goomsl_yacc.y"
    { yyval.nPtr = new_not(new_low(yyvsp[0].nPtr,yyvsp[-2].nPtr)); }
    break;

  case 73:
#line 1355 "goomsl_yacc.y"
    { yyval.nPtr = new_not(new_equ(yyvsp[-2].nPtr,yyvsp[0].nPtr)); }
    break;

  case 74:
#line 1356 "goomsl_yacc.y"
    { yyval.nPtr = new_not(yyvsp[0].nPtr);    }
    break;

  case 75:
#line 1359 "goomsl_yacc.y"
    { yyval.nPtr = new_constFloat(yyvsp[0].strValue,currentGoomSL->num_lines); }
    break;

  case 76:
#line 1360 "goomsl_yacc.y"
    { yyval.nPtr = new_constInt(yyvsp[0].strValue,currentGoomSL->num_lines); }
    break;

  case 77:
#line 1361 "goomsl_yacc.y"
    { yyval.nPtr = new_constPtr(yyvsp[0].strValue,currentGoomSL->num_lines); }
    break;

  case 78:
#line 1366 "goomsl_yacc.y"
    { yyval.nPtr = new_call(yyvsp[-2].strValue,NULL); }
    break;

  case 79:
#line 1367 "goomsl_yacc.y"
    { yyval.nPtr = new_call(yyvsp[-4].strValue,yyvsp[-2].nPtr); }
    break;

  case 80:
#line 1368 "goomsl_yacc.y"
    { yyval.nPtr = new_call(yyvsp[-3].strValue,NULL); }
    break;

  case 81:
#line 1369 "goomsl_yacc.y"
    { yyval.nPtr = new_call(yyvsp[-5].strValue,yyvsp[-3].nPtr); }
    break;

  case 82:
#line 1373 "goomsl_yacc.y"
    { yyval.nPtr = new_call_expr(yyvsp[-2].strValue,NULL); }
    break;

  case 83:
#line 1374 "goomsl_yacc.y"
    { yyval.nPtr = new_call_expr(yyvsp[-4].strValue,yyvsp[-2].nPtr); }
    break;

  case 84:
#line 1377 "goomsl_yacc.y"
    { yyval.nPtr = new_affec_list(yyvsp[-1].nPtr,yyvsp[0].nPtr);   }
    break;

  case 85:
#line 1378 "goomsl_yacc.y"
    { yyval.nPtr = new_affec_list(yyvsp[0].nPtr,NULL); }
    break;

  case 86:
#line 1380 "goomsl_yacc.y"
    {
                              gsl_reenternamespace(yyvsp[-1].namespace);
                              yyval.nPtr = new_set(new_var(yyvsp[-3].strValue,currentGoomSL->num_lines),yyvsp[0].nPtr);
                            }
    break;

  case 87:
#line 1384 "goomsl_yacc.y"
    {
                              gsl_reenternamespace(yyvsp[-1].namespace);
                              yyval.nPtr = new_set(new_var("&this", currentGoomSL->num_lines),yyvsp[0].nPtr);
                            }
    break;


    }

/* Line 999 of yacc.c.  */
#line 2792 "goomsl_yacc.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 1396 "goomsl_yacc.y"



void yyerror(char *str)
{ /* {{{ */
    fprintf(stderr, "ERROR: Line %d, %s\n", currentGoomSL->num_lines, str);
    currentGoomSL->compilationOK = 0;
    exit(1);
} /* }}} */


