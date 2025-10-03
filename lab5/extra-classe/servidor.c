#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "inc/myList.h"
#include "atv3_sin.tab.h"
#include "inc/processar-requisicoes.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* declarações vindas do Flex */
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char *yy_str);
extern void yy_delete_buffer(YY_BUFFER_STATE b);
extern FILE *yyin;
extern int yyparse();

// Definição das variáveis globais (agora definidas apenas aqui)
CampoNode *lista_completa = NULL;
CampoNode **ultimo = &lista_completa;

// Variáveis do programa
char *web_space = NULL;
char *arquivo_req = NULL;
char *arquivo_resp = NULL;
char *arquivo_registro = NULL;

// Protótipos das funções do processar-requisicoes.c
void send_error_page(int status, char* connection, char* additional_info);
int process_GET(char* caminhos[], char* connection);
int process_HEAD(char* caminhos[], char* connection);
int process_OPTIONS(char* connection);
int process_TRACE(char* connection);

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
    // Para restaurar, precisamos reabrir /dev/stdout
    int fd = open("/dev/stdout", O_WRONLY);
    if (fd != -1) {
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
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

// Função para extrair método e path da requisição
int extrair_metodo_path(char *requisicao, char **metodo, char **path) {
    // Fazer uma cópia para não modificar o original
    char *copia = strdup(requisicao);
    char *linha = strtok(copia, "\n");
    
    if (!linha) {
        free(copia);
        return 0;
    }
    
    // Remover CR se presente no final da linha
    char *cr = strchr(linha, '\r');
    if (cr) *cr = '\0';
    
    // Extrair método
    *metodo = strtok(linha, " ");
    if (!*metodo) {
        free(copia);
        return 0;
    }
    
    // Extrair path
    *path = strtok(NULL, " ");
    if (!*path) {
        free(copia);
        return 0;
    }
    
    // Duplicar as strings para uso posterior
    *metodo = strdup(*metodo);
    *path = strdup(*path);
    
    free(copia);
    return 1;
}

// Função para encontrar campo específico na lista
char* encontrar_campo(CampoNode *lista, const char *nome_campo) {
    CampoNode *campo = lista;
    while (campo != NULL) {
        if (strcasecmp(campo->nome, nome_campo) == 0 && campo->parametros != NULL) {
            return campo->parametros->valor;
        }
        campo = campo->proximo;
    }
    return NULL;
}

// Função principal de processamento
int processar_requisicao(char *requisicao, const char *web_space, 
                         const char *arquivo_resp) {
    char *metodo, *path;
    
    if (!extrair_metodo_path(requisicao, &metodo, &path)) {
        return HTTP_INTERNAL_ERROR;
    }
    
    // Redirecionar saída para arquivo de resposta
    redirect_stdout_to_file(arquivo_resp);
    
    int status_code;
    char *connection = "close";
    
    // Preparar argumentos para as funções de processamento
    char *argv[] = {"servidor", (char*)web_space, (char*)path, NULL};
    
    if (strcmp(metodo, "GET") == 0) {
        status_code = process_GET(argv, connection);
    } else if (strcmp(metodo, "HEAD") == 0) {
        status_code = process_HEAD(argv, connection);
    } else if (strcmp(metodo, "OPTIONS") == 0) {
        status_code = process_OPTIONS(connection);
    } else if (strcmp(metodo, "TRACE") == 0) {
        status_code = process_TRACE(connection);
    } else if (strcmp(metodo, "POST") == 0 || strcmp(metodo, "PUT") == 0) {
        send_error_page(HTTP_NOT_IMPLEMENTED, connection, metodo);
        status_code = HTTP_NOT_IMPLEMENTED;
    } else {
        send_error_page(HTTP_METHOD_NOT_ALLOWED, connection, metodo);
        status_code = HTTP_METHOD_NOT_ALLOWED;
    }
    
    // Restaurar saída padrão
    restore_stdout();
    
    return status_code;
}


void conectar_na_internet(char *endereco_ip, int porta, int* soquete,
                          struct sockaddr_in* meu_cliente,
                          int soquete_msg, socklen_t tam_endereco){
    // cria socket TCP
    *soquete = socket(AF_INET, SOCK_STREAM, 0);
    if (*soquete < 0) {
        perror("Erro ao criar socket");
        exit(1);
    }

    if (inet_aton(endereco_ip, meu_cliente.sin_addr) == 0) {
        printf("Endereço IP inválido\n");
        fprintf(stderr, "Endereço IP inválido\n");
        exit(1);
    }

    // permite reuso rápido da porta
    int opt = 1;
    if (setsockopt(*soquete, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Erro no setsockopt");
        exit(1);
    }

    // configura endereço/porta
    *meu_cliente.sin_family = AF_INET;
    *meu_cliente.sin_port = htons(porta);
    *meu_cliente.sin_addr.s_addr = INADDR_ANY; // qualquer interface

    if (bind(*soquete, (struct sockaddr*)meu_cliente, sizeof(*meu_cliente)) < 0) {
        perror("Erro no bind");
        exit(1);
    }

    if (listen(*soquete, 5) < 0) {
        perror("Erro no listen");
        exit(1);
    }
}


int main(int argc, char *argv[]) {
    if(argc != 3){
        fprintf(stderr, "Uso: %s [endereco IP] [porta]\n", argv[0]);
        exit(1);
    }

    char *endereco_ip = argv[1];
    int porta = atoi(argv[2]);

    int soquete, soquete_msg;
    struct sockaddr_in meu_cliente;
    socklen_t tam_endereco = sizeof(meu_cliente);
    char requisicao[2048];

    conectar_na_rede(endereco_ip, porta, &soquete, &meu_cliente, soquete_msg, tam_endereco);

    while (1) {
        soquete_msg = accept(soquete, (struct sockaddr*)&meu_cliente, &tam_endereco);
        if (soquete_msg < 0) {
            perror("Erro no accept");
            continue;
        }

        memset(requisicao, 0, sizeof(requisicao));
        read(soquete_msg, requisicao, sizeof(requisicao) - 1);

        // CÓDIGO PARA PROCESSAR REQUISIÇÃO
        //printf("%s\n", requisicao);

        // CÓDIGO PARA EXIBIR RESPOSTA
        /*const char *resposta =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<html><body><h1>Olá, mundo!</h1></body></html>";

            */
        write(soquete_msg, resposta, strlen(resposta));
        close(soquete_msg);
    }

    close(soquete);
    
    
    // Ler arquivo de requisição
    /*char *requisicao = ler_arquivo_requisicao(arquivo_req);
    if (!requisicao) {
        return 1;
    }*/
    
    printf("=== REQUISIÇÃO LIDA DA WEB ===\n");
    printf("%s\n", requisicao);
    printf("=== FIM DA REQUISIÇÃO ===\n\n");
    
    /*
    // Configurar o parser para ler da string
    YY_BUFFER_STATE buffer = yy_scan_string(requisicao);
    
    // Executar parser
    printf("=== PROCESSANDO REQUISIÇÃO COM PARSER ===\n");
    yyparse();
    
    // Imprimir resultado da análise
    printf("=== ESTRUTURA DE DADOS DO PARSER ===\n");
    imprimirLista(lista_completa);
    
    // Limpar buffer do lexer
    yy_delete_buffer(buffer);
    
    // Processar requisição
    printf("=== EXECUTANDO REQUISIÇÃO ===\n");
    int status = processar_requisicao(requisicao, web_space, arquivo_resp);
    
    printf("Status da requisição: %d\n", status);
    
    // Registrar no arquivo de registro
    char *metodo, *path;
    extrair_metodo_path(requisicao, &metodo, &path);
    int apenas_cabecalho = (strcmp(metodo, "GET") == 0);
    
    registrar_requisicao_resposta(arquivo_req, arquivo_resp, arquivo_registro, apenas_cabecalho);
    
    printf("Requisição processada e registrada com sucesso!\n");
    
    // Liberar memória
    liberarLista(lista_completa);
    free(requisicao);
    */
    return 0;
}