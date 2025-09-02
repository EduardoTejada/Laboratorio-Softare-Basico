#include <stdio.h>
#include <stdlib.h>
#include "myList.h"

void addParameterToList(char* parametro, ParametroNode **ultimo){
    ParametroNode *novo = malloc(sizeof(ParametroNode));
    novo->valor = parametro;
    novo->proximo = NULL;
    
    *ultimo = novo;
    ultimo = &(novo->proximo);
}

void addComandToList(ComandoNode* comando, ComandoNode ***ultimo){
    **ultimo = comando;
    *ultimo = &(comando->proximo);
}

CampoNode* createFieldNode(char* nome_campo, ParametroNode* parametros){
    CampoNode *campo = malloc(sizeof(CampoNode));
    campo->nome = nome_campo;
    campo->parametros = parametros;
    campo->proximo = NULL;
    return campo;
}

ComandoNode* createComandNode(char* nome_comando, CampoNode* campos){
    ComandoNode *comando = malloc(sizeof(ComandoNode));
    comando->nome_comando = nome_comando;
    comando->campos = campos;
    comando->proximo = NULL;
    return comando;
}

// Função para imprimir a lista
void imprimirLista(ComandoNode *lista) {
    ComandoNode *comando = lista;
    while (comando != NULL) {
        printf("\nCOMANDO HTTP: '%s'\n", comando->nome_comando);
        CampoNode *campo = comando->campos;
        while (campo != NULL){
            printf(" -> CAMPO: '%s'\n", campo->nome);
            
            ParametroNode *param = campo->parametros;
            while (param != NULL) {
                printf("  --> PARAMETRO: '%s'\n", param->valor);
                param = param->proximo;
            }
            
            campo = campo->proximo;
        }
        comando = comando->proximo;
    }
}

// Função para liberar memória
void liberarLista(ComandoNode *lista) {
    ComandoNode *comando = lista;
    while (comando != NULL){
        ComandoNode *proximo_comando = comando->proximo;

        // Liberar campos
        CampoNode *campo = comando->campos;
        while (campo != NULL) {
            CampoNode *proximo_campo = campo->proximo;
            
            // Liberar parâmetros
            ParametroNode *param = campo->parametros;
            while (param != NULL) {
                ParametroNode *proximo_param = param->proximo;
                free(param->valor);
                free(param);
                param = proximo_param;
            }
            
            free(campo->nome);
            free(campo);
            campo = proximo_campo;
        }

        free(comando->nome_comando);
        free(comando);
        comando = proximo_comando;
    }
}