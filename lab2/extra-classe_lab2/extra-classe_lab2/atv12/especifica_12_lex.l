%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "especifica_12_sin.tab.h"
void extern yyerror (char *s);

%}

ESPACO [ \t]
LINHA_VALIDA {ESPACO}*[a-zA-Z][a-zA-Z0-9_\-]*{ESPACO}*:{ESPACO}*.*
CAMPO_VAZIO \n{ESPACO}*:{ESPACO}*.*
CAMPO_INVALIDO \n{ESPACO}*[^a-zA-Z ].*{ESPACO}*:{ESPACO}*.*

%%
#.*                 { ; }
{CAMPO_INVALIDO}    { printf("CAMPO_INVALIDO: <%s>\n", ++yytext); }
{CAMPO_VAZIO}       { printf("CAMPO_VAZIO: <%s>\n", ++yytext); }
{LINHA_VALIDA}      { yylval.str = strdup(yytext); return LINHA_VALIDA;}
[ \t\n]             { ; }
.                   { ; }
%%