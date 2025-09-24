%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "atv3_sin.tab.h"
void extern yyerror (char *s);
%}

ESPACO [\s\t]
LINHA_VALIDA {ESPACO}*[a-zA-Z][a-zA-Z0-9_\-]*{ESPACO}*:{ESPACO}*.*
CAMPO_VAZIO \n{ESPACO}*:{ESPACO}*.*
CAMPO_INVALIDO \n{ESPACO}*[^a-zA-Z\n ].*{ESPACO}*:{ESPACO}*.*
LINHA_DUMP \.\/http-dump.*\n
LINHA_MSG \n{ESPACO}*M{ESPACO}*.*{ESPACO}*:{ESPACO}*.*
LINHA_COMANDO (GET|TRACE|PUT|POST|OPTIONS){ESPACO}*.*

%%
#.*                 { /* printf("0- %s\n", yytext); */; }
{LINHA_DUMP}        { /* printf("1- %s\n", yytext); */; }
{LINHA_MSG}         { /* printf("2- %s\n", yytext); */; }
{CAMPO_INVALIDO}    { /* printf("3- %s\n", yytext); */; printf("CAMPO_INVALIDO: <%s>\n", ++yytext);         }
{CAMPO_VAZIO}       { /* printf("4- %s\n", yytext); */; printf("CAMPO_VAZIO: <%s>\n", ++yytext);            }
{LINHA_VALIDA}      { /* printf("5- %s\n", yytext); */; yylval.str = strdup(yytext); return LINHA_VALIDA;   }
{LINHA_COMANDO}     { /* printf("6- %s\n", yytext); */; yylval.str = strdup(yytext); return LINHA_COMANDO;  }
"\n"                { /* printf("7- %s\n", yytext); */; }
.                   { /* printf("8- %s\n", yytext); */; }
%%
