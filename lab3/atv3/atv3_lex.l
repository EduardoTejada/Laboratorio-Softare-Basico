%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "atv3_sin.tab.h"
void extern yyerror (char *s);

%}

ESPACO [\s\t]
DELIM [\s\t\n]
LINHA_VALIDA {ESPACO}*[a-zA-Z][a-zA-Z0-9_\-]*{ESPACO}*:{ESPACO}*.*
CAMPO_VAZIO \n{ESPACO}*:{ESPACO}*.*
CAMPO_INVALIDO \n{ESPACO}*[^a-zA-Z ].*{ESPACO}*:{ESPACO}*.*
LINHA_DUMP \.\/http-dump.*{DELIM}*.*
LINHA_MSG Mensagem{ESPACO}recebida:
LINHA_COMANDO (GET|TRACE|PUT|POST|OPTIONS){ESPACO}*.*

%%
#.*                 { ; }
{LINHA_DUMP}        { ; }
{LINHA_MSG}         { ; }
{CAMPO_INVALIDO}    { printf("CAMPO_INVALIDO: <%s>\n", ++yytext); }
{CAMPO_VAZIO}       { printf("CAMPO_VAZIO: <%s>\n", ++yytext); }
{LINHA_VALIDA}      { yylval.str = strdup(yytext); return LINHA_VALIDA;}
{LINHA_COMANDO}     { yylval.str = strdup(yytext); return LINHA_COMANDO;}
[ \t\n]             { ; }
.                   { ; }
%%
