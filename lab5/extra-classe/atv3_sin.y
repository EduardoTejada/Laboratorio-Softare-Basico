%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "inc/myString.h"

int yylex();
void yyerror(const char *msg);
void yylex_destroy(void);

extern char* yytext;

// Declaração como extern (definição está em servidor.c)
extern CampoNode *lista_completa;
extern CampoNode **ultimo;

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
        printf("Parser: Comando '%s' processado com sucesso\n", $1);
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
            printf("ERRO: Falha ao adicionar campo à lista\n");
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
            printf("ERRO: Falha ao adicionar campo à lista\n");
            exit(1);
        }
        $$ = $1; // Retorna o primeiro elemento
    }
;

linha : LINHA_VALIDA
    {
        CampoNode *campo = processarLinha($1);
        if (campo == (CampoNode*) NULL) {
            printf("ERRO: Falha ao processar linha '%s'\n", $1);
            free($1);
            exit(1);
        }
        printf("Parser: Campo '%s' processado\n", campo->nome);
        $$ = campo;
        free($1);
    }
;

comando: LINHA_COMANDO
    {
        char* comando = extractHTTPCommand($1);
        printf("Parser: Comando HTTP extraído: %s\n", comando);
        $$ = strdup(comando); // Duplicar para uso posterior
        free($1);
        free(comando);
    }
;
%%

void yyerror(const char *msg) {
    fprintf(stderr, "Erro de parsing: %s\n", msg);
    fprintf(stderr, "Token atual: %s\n", yytext);
}