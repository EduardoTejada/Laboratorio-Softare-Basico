#ifndef PROCESSAR_REQUISICOES_H
#define PROCESSAR_REQUISICOES_H

#include <sys/stat.h>

#define HTTP_OK 200
#define HTTP_BAD_REQUEST 400
#define HTTP_NOT_FOUND 404
#define HTTP_FORBIDDEN 403
#define HTTP_METHOD_NOT_ALLOWED 405
#define HTTP_INTERNAL_ERROR 500
#define HTTP_NOT_IMPLEMENTED 501

#ifndef S_IFDIR
#define S_IFDIR 1
#endif

#ifndef S_IFMT
#define S_IFMT 2
#endif

#ifndef S_IFREG
#define S_IFREG 3
#endif

#include "myList.h"

// Declarações das funções
char* get_http_date();
char* get_content_type(char* filename);
void send_error_page(int status, char* connection, char* additional_info);
void send_headers(int status, char* connection, struct stat *statbuf, char* filename, int is_head);
void send_simple_headers(int status, char* connection, long content_length, char* content_type);
int process_OPTIONS(char* connection);
int process_TRACE(char* connection);
int process_GET(char* caminhos[], char* connection, CampoNode* lista_campos);
int process_HEAD(char* caminhos[], char* connection);
int process_POST(char* caminhos[], char* connection, CampoNode* lista_campos, const char* corpo_post);

#endif /* PROCESSAR_REQUISICOES_H */