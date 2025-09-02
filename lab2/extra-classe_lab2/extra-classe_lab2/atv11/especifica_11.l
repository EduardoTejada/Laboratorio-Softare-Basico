%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "especifica_11.tab.h"
void extern yyerror (char *s);

%}

ESPACO [ \t]
DIGITO [0-9]
LETRA [A-Za-z]
DELIM {ESPACO}+
ID {LETRA}({LETRA}|{DIGITO}|_)*

%%
"int"           { return TIPO; }
{DELIM}		    {  }
"main"          { return INICIO_MAIN; }
"float"         { return TIPO; }
"("             { return PARENTESES_ESQ; }
")"             { return PARENTESES_DIR; }
"{"             { return CHAVE_ESQ; }
"}"             { return CHAVE_DIR; }
"="             { return IGUAL; }
[*+/\-]         { return OPERA; }
";"             { return PONTO_VIRGULA; }
","             { return VIRGULA; }
{ID}            { yylval.str = strdup(yytext); return NOMEVAR; }
{DIGITO}+(\.{DIGITO}+)? { yylval.str = strdup(yytext); return VALOR; }
[ \t\n]         {  }
.               {  }
%%
