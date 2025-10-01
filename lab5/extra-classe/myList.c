#include <stdio.h>
#include <stdlib.h>
#include "inc/myList.h"

void addParameterToList(char* parametro, ParametroNode **ultimo){
    ParametroNode *novo = malloc(sizeof(ParametroNode));
    novo->valor = parametro;
    novo->proximo = NULL;
    
    *ultimo = novo;
    ultimo = &(novo->proximo);
}

CampoNode* createFieldNode(char* nome_campo, ParametroNode* parametros){
    CampoNode *campo = malloc(sizeof(CampoNode));
    campo->nome = nome_campo;
    campo->parametros = parametros;
    campo->proximo = NULL;
    return campo;
}

// Função para imprimir a lista
void imprimirLista(CampoNode *lista) {
    CampoNode *campo = lista;
    while (campo != NULL) {
        printf("CAMPO: '%s'\n", campo->nome);
        
        ParametroNode *param = campo->parametros;
        while (param != NULL) {
            printf("  -> PARAMETRO: '%s'\n", param->valor);
            param = param->proximo;
        }
        
        campo = campo->proximo;
    }
}

// Função para liberar memória
void liberarLista(CampoNode *lista) {
    CampoNode *campo = lista;
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
}