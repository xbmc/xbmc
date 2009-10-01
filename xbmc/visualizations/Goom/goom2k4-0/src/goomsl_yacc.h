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
/* Line 1240 of yacc.c.  */
#line 95 "goomsl_yacc.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



