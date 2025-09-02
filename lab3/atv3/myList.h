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

typedef struct ComandoNode {
    char *nome_comando;
    CampoNode *campos;
    struct ComandoNode *proximo;
} ComandoNode;

void addParameterToList(char* parametro, ParametroNode **ultimo);
void addComandToList(ComandoNode* comando, ComandoNode ***ultimo);
void addFieldToList(CampoNode* campo, CampoNode **ultimo);
CampoNode* createFieldNode(char* nome_campo, ParametroNode* parametros);
ComandoNode* createComandNode(char* nome_comando, CampoNode* campos);
void imprimirLista(ComandoNode *lista);
void liberarLista(ComandoNode *lista);