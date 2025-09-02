/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    NOMEVAR = 258,                 /* NOMEVAR  */
    VALOR = 259,                   /* VALOR  */
    INICIO_MAIN = 260,             /* INICIO_MAIN  */
    TIPO = 261,                    /* TIPO  */
    IGUAL = 262,                   /* IGUAL  */
    OPERA = 263,                   /* OPERA  */
    PONTO_VIRGULA = 264,           /* PONTO_VIRGULA  */
    PARENTESES_ESQ = 265,          /* PARENTESES_ESQ  */
    PARENTESES_DIR = 266,          /* PARENTESES_DIR  */
    VIRGULA = 267,                 /* VIRGULA  */
    CHAVE_ESQ = 268,               /* CHAVE_ESQ  */
    CHAVE_DIR = 269                /* CHAVE_DIR  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define NOMEVAR 258
#define VALOR 259
#define INICIO_MAIN 260
#define TIPO 261
#define IGUAL 262
#define OPERA 263
#define PONTO_VIRGULA 264
#define PARENTESES_ESQ 265
#define PARENTESES_DIR 266
#define VIRGULA 267
#define CHAVE_ESQ 268
#define CHAVE_DIR 269

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
#line 33 "especifica_11.y"
union YYSTYPE
{
#line 33 "especifica_11.y"

    char* str;
    double num;

#line 101 "y.tab.h"

};
#line 33 "especifica_11.y"
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
