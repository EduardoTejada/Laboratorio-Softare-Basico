#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "myString.h"

// Retira os espaços iniciais
char* trimBegin(char *str){
    char *aux = str;
    while (*aux && isspace(*aux)) {
        aux++;
    }

    return aux;
}

// Retira os espaços finais
char* trimEnd(char *str){
    char *aux = str + strlen(str) - 1;
    while (aux > str && isspace(*aux)) {
        *aux = '\0';
        aux--;
    }
    return aux;
}

// Função para remover espaços do início e fim (inspirado no strip do Python)
void trim(char *str) {
    if (str == NULL) return;
    
    // Retira os espaços iniciais
    char *inicio = trimBegin(str);
    
    // Se a string é composta somente de espaços
    if (*inicio == '\0') {
        str[0] = '\0';
        return;
    }
    
    // Retira os espaços finais
    char *fim = trimEnd(inicio);
    
    // Desloca o ponteiro da string caso tenha mudado o início
    if (inicio != str) {
        memmove(str, inicio, strlen(inicio) + 1);
    }
}

char* extractParameter(char** ptr, int isQuotes){
    char *fim;
    if(isQuotes == 1){
        fim = strchr(*ptr, '"');
        // Encontrar aspas de fechamento
        if (fim == NULL) {
            // Aspas não fechadas: tratando como todo restante da lista de parâmetros pertencentes à string
            fim = *ptr + strlen(*ptr);
            printf("Aviso: Detectadas aspas não fechadas dentro dos parâmetros!\n");
        }
    }
    else{
        fim = *ptr;
        while (*fim && *fim != ',') {
            fim++;
        }
    }


    int tamanho = fim - *ptr;
    char *parametro = malloc(tamanho + 1);
    strncpy(parametro, *ptr, tamanho);
    parametro[tamanho] = '\0';

    *ptr = fim;


    if(!isQuotes){
        trim(parametro);
        char* aux = parametro;
        int tem_espaco = 0;
        while (*aux) {
            if(*aux == ' '){
                tem_espaco = 1;
            }
            aux++;
        }
        if(tem_espaco)
            printf("Aviso: Parametro <%s> contém espaço que não está dentro de aspas! Interpretando como string.\n", parametro);
    }

    return parametro;
}


char* extractFieldName(char* linha, char* dois_pontos){
    int nome_campo_len = dois_pontos - linha; // Calcula o tamanho da string até os dois pontos
    char *nome_campo = malloc(nome_campo_len + 1); // Aloca a quantidade de caracteres necessária para a string
    strncpy(nome_campo, linha, nome_campo_len); // Copia somente a quantidade de caracteres correta
    nome_campo[nome_campo_len] = '\0'; // Coloca o caractere final da string
    return nome_campo;
}

char* findTwoDots(char* linha){
    return strchr(linha, ':');
}


// Função principal para processar uma linha
CampoNode* processarLinha(char *linha) {
    if (linha == NULL || *linha == '\0') {
        return NULL; // Linha vazia
    }
    
    // Encontrar o caractere dois pontos ":"
    char *dois_pontos = findTwoDots(linha);
    
    // Extrair nome do campo
    char *nome_campo = extractFieldName(linha, dois_pontos);
    
    trim(nome_campo); // Remove espaços
    
    // Extrair parâmetros (após os dois pontos)
    char *parametros_str = dois_pontos + 1;
    ParametroNode *parametros = processarParametros(parametros_str);
    
    // Criar nó do campo
    CampoNode *campo = createFieldNode(nome_campo, parametros);
    
    return campo;
}


// Função para processar parâmetros
ParametroNode* processarParametros(char *texto_parametros) {
    if (texto_parametros == NULL || *texto_parametros == '\0') {
        return NULL;
    }
    
    ParametroNode *cabeca = NULL;
    ParametroNode **ultimo = &cabeca;
    

    char *ptr = texto_parametros;
    while (*ptr) {
        // Pular espaços iniciais
        ptr = trimBegin(ptr);
        
        if (*ptr == '\0') break;
        
        // Verificar se começa com aspas
        if (*ptr == '"') {
            ptr++; // Pular aspas de abertura
            
            // Parâmetro com aspas vai até a próxima aspas ou fim
            char *parametro = extractParameter(&ptr, 1);

            // Adicionar à lista
            addParameterToList(parametro, ultimo);
            
            // Pular aspas de fechamento
            if (*ptr == '"') ptr++;
        } else {
            // Parâmetro sem aspas vai até a próxima vírgula ou fim
            char *parametro = extractParameter(&ptr, 0);
            
            // Remover espaços (só para parâmetros sem aspas)
            trim(parametro);
            
            // Só adicionar se não for vazio
            if (*parametro != '\0') {
                addParameterToList(parametro, ultimo);
            } else {
                free(parametro);
            }
            
        }
        
        // Pular vírgula e continuar
        if (*ptr == ',') {
            ptr++;
        }
    }
    
    return cabeca;
}