%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "atv3_sin.tab.h"
void extern yyerror (char *s);

int yywrap(void);

%}

ESPACO [\s\t]
CRLF \r\n
DELIM [\s\t\r\n]
LINHA_VALIDA {ESPACO}*[a-zA-Z][a-zA-Z0-9_\-]*{ESPACO}*:{ESPACO}*.*{CRLF}?
CAMPO_VAZIO \n{ESPACO}*:{ESPACO}*.*{CRLF}?
CAMPO_INVALIDO \n{ESPACO}*[^a-zA-Z ].*{ESPACO}*:{ESPACO}*.*{CRLF}?
LINHA_DUMP \.\/http-dump.*{DELIM}*.*
LINHA_MSG Mensagem{ESPACO}recebida:
LINHA_COMANDO (GET|TRACE|PUT|POST|OPTIONS|HEAD){ESPACO}*.*{CRLF}?

%%
{CRLF}             { ; }
#.*                { ; }
{LINHA_DUMP}       { ; }
{LINHA_MSG}        { ; }
{CAMPO_INVALIDO}   { printf("CAMPO_INVALIDO: <%s>\n", yytext); }
{CAMPO_VAZIO}      { printf("CAMPO_VAZIO: <%s>\n", yytext); }
{LINHA_VALIDA}     { 
    // Remover CRLF do final se existir
    char *temp = strdup(yytext);
    char *cr = strchr(temp, '\r');
    if (cr) *cr = '\0';
    yylval.str = temp;
    return LINHA_VALIDA;
}
{LINHA_COMANDO}    { 
    // Remover CRLF do final se existir
    char *temp = strdup(yytext);
    char *cr = strchr(temp, '\r');
    if (cr) *cr = '\0';
    yylval.str = temp;
    return LINHA_COMANDO;
}
[ \t\n]            { ; }
.                  { ; }
%%

int yywrap(void) {
    return 1;
}