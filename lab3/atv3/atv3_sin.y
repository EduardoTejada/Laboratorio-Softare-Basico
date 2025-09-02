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

CampoNode *lista_campo = NULL;
CampoNode **ultimo_campo = &lista_campo;

ComandoNode *lista_comandos = NULL;
ComandoNode **ultimo_comando = &lista_comandos;

%}

%union {
    char* str;
    double num;
    struct CampoNode* campo_node;
    struct {
        char* comando;
        struct CampoNode* campos;
    } lista_comando_completo;
}

%type <lista_comando_completo> lista_comando
%type <str> comando
%type <campo_node> linhas
%type <campo_node> linha

%token <str> LINHA_VALIDA
%token <str> LINHA_COMANDO

%%


programa : lista_comando programa
    | lista_comando
;

lista_comando : comando linhas
    {
        ComandoNode *comando = createComandNode($1, lista_campo);
        addComandToList(comando, &ultimo_comando);
        
        if(lista_comandos == NULL) lista_comandos = comando;

        lista_campo = NULL;
        ultimo_campo = &lista_campo;
    }
;

linhas : linhas linha
    {
        // Adiciona à lista ligada
        if ($2 != (CampoNode*) NULL) {
            *ultimo_campo = $2;
            ultimo_campo = &($2->proximo);
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
            *ultimo_campo = $1;
            ultimo_campo = &($1->proximo);
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
        free($1);
        $$ = comando;
    }
;
%%

void yyerror(const char *msg) {
    fprintf(stderr, "Erro: %s\n", msg);
    fprintf(stderr, "Token atual: %s\n", yytext);
}


int main(){
    yyparse();
    imprimirLista(lista_comandos);
    liberarLista(lista_comandos);
    yylex_destroy();

    return 0;
}