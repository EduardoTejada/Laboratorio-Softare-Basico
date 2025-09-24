#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "atv3_sin.tab.h"
#include "processar-requisicoes.h"
// Variáveis globais para comunicação entre parser e main

ComandoNode *lista_comandos_global = NULL;
char *web_space = NULL;
char *arquivo_req = NULL;
char *arquivo_resp = NULL;
char *arquivo_registro = NULL;

// Função para redirecionar saída padrão para arquivo
void redirect_stdout_to_file(const char *filename) {
    fflush(stdout);
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Erro ao abrir arquivo de resposta");
        exit(1);
    }
    dup2(fd, STDOUT_FILENO);
    close(fd);
}

// Função para restaurar saída padrão
void restore_stdout() {
    fflush(stdout);
    dup2(STDOUT_FILENO, 1);
}

// Função para ler arquivo de requisição
char* ler_arquivo_requisicao(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir arquivo de requisição");
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *buffer = malloc(length + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    fclose(file);
    
    return buffer;
}

// Função para registrar requisição/resposta no arquivo de registro
void registrar_requisicao_resposta(const char *req_file, const char *resp_file, 
                                  const char *reg_file, int apenas_cabecalho) {
    FILE *reg = fopen(reg_file, "a");
    if (!reg) {
        perror("Erro ao abrir arquivo de registro");
        return;
    }
    
    fprintf(reg, "\n=== REQUISIÇÃO: %s ===\n", req_file);
    
    // Copiar requisição
    char *req_content = ler_arquivo_requisicao(req_file);
    if (req_content) {
        fprintf(reg, "%s\n", req_content);
        free(req_content);
    }
    
    fprintf(reg, "=== RESPOSTA: %s ===\n", resp_file);
    
    // Copiar resposta (apenas cabeçalho se especificado)
    FILE *resp = fopen(resp_file, "r");
    if (resp) {
        char line[1024];
        int in_header = 1;
        
        while (fgets(line, sizeof(line), resp)) {
            if (apenas_cabecalho) {
                if (strcmp(line, "\r\n") == 0 || strcmp(line, "\n") == 0) {
                    in_header = 0;
                    fprintf(reg, "%s", line);
                    continue;
                }
                
                if (in_header) {
                    fprintf(reg, "%s", line);
                }
            } else {
                fprintf(reg, "%s", line);
            }
        }
        fclose(resp);
    }
    
    fprintf(reg, "=== FIM ===\n\n");
    fclose(reg);
}

// Função para processar requisição usando as funções do processar-requisicoes.c
int processar_requisicao(ComandoNode *comando, const char *web_space, 
                         const char *arquivo_resp) {
    if (!comando || !comando->nome_comando) {
        return HTTP_INTERNAL_ERROR;
    }
    
    // Redirecionar saída para arquivo de resposta
    redirect_stdout_to_file(arquivo_resp);
    
    int status_code;
    char *method = comando->nome_comando;
    char *connection = "close";
    
    // Encontrar o campo Host para determinar o recurso
    char *recurso = "/";
    CampoNode *campo = comando->campos;
    
    while (campo) {
        if (strcasecmp(campo->nome, "Host") == 0 && campo->parametros) {
            // Extrair path do Host se disponível
            char *host_value = campo->parametros->valor;
            char *path_start = strchr(host_value, '/');
            if (path_start) {
                recurso = path_start;
            }
            break;
        }
        campo = campo->proximo;
    }
    
    // Preparar argumentos para as funções de processamento
    char *argv[] = {"servidor", (char*)web_space, (char*)recurso, method};
    
    if (strcmp(method, "GET") == 0) {
        status_code = process_GET(argv, connection);
    } else if (strcmp(method, "HEAD") == 0) {
        status_code = process_HEAD(argv, connection);
    } else if (strcmp(method, "OPTIONS") == 0) {
        status_code = process_OPTIONS(connection);
    } else if (strcmp(method, "TRACE") == 0) {
        status_code = process_TRACE(connection);
    } else if (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0) {
        send_error_page(HTTP_NOT_IMPLEMENTED, connection, method);
        status_code = HTTP_NOT_IMPLEMENTED;
    } else {
        send_error_page(HTTP_METHOD_NOT_ALLOWED, connection, method);
        status_code = HTTP_METHOD_NOT_ALLOWED;
    }
    
    // Restaurar saída padrão
    restore_stdout();
    
    return status_code;
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Uso: %s <Web Space> req_N.txt resp_N.txt registro.txt\n", argv[0]);
        return 1;
    }
    
    web_space = argv[1];
    arquivo_req = argv[2];
    arquivo_resp = argv[3];
    arquivo_registro = argv[4];
    
    // Ler arquivo de requisição
    char *requisicao = ler_arquivo_requisicao(arquivo_req);
    if (!requisicao) {
        return 1;
    }
    
    printf("=== REQUISIÇÃO LIDA DO ARQUIVO ===\n");
    printf("%s\n", requisicao);
    printf("=== FIM DA REQUISIÇÃO ===\n\n");
    
    // Configurar o parser para ler da string
    YY_BUFFER_STATE buffer = yy_scan_string(requisicao);
    
    // Executar parser
    printf("=== PROCESSANDO REQUISIÇÃO COM PARSER ===\n");
    yyparse();
    
    // Limpar buffer do lexer
    yy_delete_buffer(buffer);
    free(requisicao);
    
    // Processar requisição (primeiro comando da lista)
    if (lista_comandos_global) {
        printf("=== EXECUTANDO REQUISIÇÃO ===\n");
        int status = processar_requisicao(lista_comandos_global, web_space, arquivo_resp);
        
        printf("Status da requisição: %d\n", status);
        
        // Registrar no arquivo de registro
        int apenas_cabecalho = (strcmp(lista_comandos_global->nome_comando, "GET") == 0);
        registrar_requisicao_resposta(arquivo_req, arquivo_resp, arquivo_registro, apenas_cabecalho);
        
        printf("Requisição processada e registrada com sucesso!\n");
    } else {
        printf("Nenhuma requisição válida encontrada!\n");
    }
    
    // Liberar memória
    liberarLista(lista_comandos_global);
    yylex_destroy();
    
    return 0;
}