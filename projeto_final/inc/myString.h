#ifndef MYSTRING_H
#define MYSTRING_H

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
void url_decode(const char* src, char* dest);

#endif /* MYSTRING_H */