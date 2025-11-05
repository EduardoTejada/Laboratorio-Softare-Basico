#include "myList.h"

char* extractParameter(char** ptr, int isQuotes);
void trim(char *str);
char* trimEnd(char *str);
char* trimBegin(char *str);
char* extractFieldName(char* linha, char* dois_pontos);
char* findTwoDots(char* linha);
CampoNode* processarLinha(char *linha);
ParametroNode* processarParametros(char *texto_parametros);
char* extractHTTPCommand(char* linha);
int extrair_metodo_path(char *requisicao, char **metodo, char **path);
void registrar_requisicao_resposta(char *req_content, const char *resp_file, 
                                  const char *reg_file, int apenas_cabecalho);