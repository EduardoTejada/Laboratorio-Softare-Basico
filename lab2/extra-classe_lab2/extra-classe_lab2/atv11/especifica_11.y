%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylex();
void yyerror(const char *msg);

void yylex_destroy(void);

extern char* yytext;

typedef struct {
    char nome[50];
    char inicializada;
} Variavel;

int tamanho_atual = 20;
Variavel variaveis_lista[20];
int num_variaveis = 0;

void adicionarVariavel(char* nome);
void inicializarVariavel(char* nome);
void verificarInicializacao(char* nome);


%}

%union {
    char* str;
    double num;
}
%token <str> NOMEVAR
%token <str> VALOR
%token INICIO_MAIN  TIPO IGUAL OPERA PONTO_VIRGULA
%token PARENTESES_ESQ PARENTESES_DIR VIRGULA CHAVE_ESQ CHAVE_DIR

%left OPERA
%left PARENTESES_ESQ PARENTESES_DIR

%%
prog_fonte : TIPO INICIO_MAIN PARENTESES_ESQ PARENTESES_DIR CHAVE_ESQ conteudo_prog CHAVE_DIR
;

conteudo_prog : 
    | declaracoes expressoes
;

declaracoes : linha_declara declaracoes
    | linha_declara
;

linha_declara : TIPO variaveis PONTO_VIRGULA
;

variaveis : identificador VIRGULA variaveis
    | identificador
;

identificador : NOMEVAR
    {
      adicionarVariavel($1);
      free($1);
    }
    | NOMEVAR IGUAL VALOR
    {
      adicionarVariavel($1);
      inicializarVariavel($1);
      free($1);
      free($3);
    }
;

expressoes : linha_executavel expressoes
    | linha_executavel
;

linha_executavel : NOMEVAR IGUAL operacoes PONTO_VIRGULA
    {
      inicializarVariavel($1);
      free($1);
    }
;

operacoes : operacoes OPERA operacoes
    | PARENTESES_ESQ operacoes PARENTESES_DIR
    | VALOR { free($1); 
    }
    | NOMEVAR
    {
      verificarInicializacao($1);
      free($1);
    }
;
%%


void yyerror(const char *msg) {
    fprintf(stderr, "Erro: %s\n", msg);
    fprintf(stderr, "Token atual: %s\n", yytext);
}

void adicionarVariavel(char* nome){
    // Verifica se a variável já foi adicionada
    for(int i = 0; i < num_variaveis; i++){
        if(strcmp(variaveis_lista[i].nome, nome) == 0){
            return;
        }
    }
    
    // Adiciona a variável à lista
    strcpy(variaveis_lista[num_variaveis].nome, nome);
    variaveis_lista[num_variaveis].inicializada = 0;
    num_variaveis++;
}

void inicializarVariavel(char* nome){
    // Marca a variável como inicializada
    for(int i = 0; i < num_variaveis; i++){
        if(strcmp(variaveis_lista[i].nome, nome) == 0){
            variaveis_lista[i].inicializada = 1;
            return;
        }
    }
    // Caso ela não esteja na lista ela é adicionada e então inicializada
    adicionarVariavel(nome);
    inicializarVariavel(nome);
}

void verificarInicializacao(char* nome){
    // Busca entre todas as variáveis
    for(int i = 0; i < num_variaveis; i++){
        if(strcmp(variaveis_lista[i].nome, nome) == 0){
            // Caso não esteja inicializada emite um alerta
            if(variaveis_lista[i].inicializada == 0) {
                printf("ALERTA: Variavel <%s> usada mas nao inicializada\n", nome);
            }
            return;
        }
    }
}

int main(){
    yyparse();
    yylex_destroy();
    return 0;
}
