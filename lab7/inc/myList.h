#ifndef MYLIST_H
#define MYLIST_H

// Estruturas para lista ligada
typedef struct ParametroNode {
    char *valor;
    struct ParametroNode *proximo;
} ParametroNode;

typedef struct CampoNode {
    char *nome;
    ParametroNode *parametros;
    struct CampoNode *proximo;
} CampoNode;

void addParameterToList(char* parametro, ParametroNode **ultimo);
CampoNode* createFieldNode(char* nome_campo, ParametroNode* parametros);
void imprimirLista(CampoNode *lista);
void liberarLista(CampoNode *lista);

#endif /* MYLIST_H */