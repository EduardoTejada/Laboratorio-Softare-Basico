%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "myString.h"

int yylex();
void yyerror(const char *msg);
void yylex_destroy(void);

extern char* yytext;

CampoNode *lista_completa = NULL;
CampoNode **ultimo = &lista_completa;

%}

%union {
    char* str;
    double num;
    struct CampoNode* campo_node;
    struct {
        char* comando;
        struct CampoNode* campos;
    } programa_completo;
}

%type <programa_completo> programa
%type <str> comando
%type <campo_node> linhas
%type <campo_node> linha

%token <str> LINHA_VALIDA
%token <str> LINHA_COMANDO

%%

programa : comando linhas
    {
        $$.comando = $1;
        $$.campos = $2;
    }
;

linhas : linhas linha
    {
        // Adiciona à lista ligada
        if ($2 != (CampoNode*) NULL) {
            *ultimo = $2;
            ultimo = &($2->proximo);
        }
        else {
            printf("ERRO!\n");
            exit(1);
        }
        $$ = $1; // Retorna a cabeça da lista
    }
    | linha
    {
        if ($1 != (CampoNode*) NULL) {
            *ultimo = $1;
            ultimo = &($1->proximo);
        }
        else {
            printf("ERRO!\n");
            exit(1);
        }
        $$ = $1; // Retorna o primeiro elemento
    }
;

linha : LINHA_VALIDA
    {
        CampoNode *campo = processarLinha($1);
        if (campo == (CampoNode*) NULL) {
            printf("ERRO!\n");
            free($1);
            exit(1);
        }
        $$ = campo;
        free($1);
    }
;

comando: LINHA_COMANDO
    {
        char* comando = extractHTTPCommand($1);
        printf("Comando HTTP: %s\n", comando);
        free($1);
        free(comando);
    }
;
%%

void yyerror(const char *msg) {
    fprintf(stderr, "Erro: %s\n", msg);
    fprintf(stderr, "Token atual: %s\n", yytext);
}


int main(){
    yyparse();
    imprimirLista(lista_completa);
    liberarLista(lista_completa);

    yylex_destroy();

    return 0;
}