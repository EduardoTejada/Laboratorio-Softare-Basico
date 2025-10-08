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

/* declarações vindas do Flex */
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char *yy_str);
extern void yy_delete_buffer(YY_BUFFER_STATE b);
extern FILE *yyin;
extern int yyparse();

// Definição das variáveis globais
CampoNode *lista_completa = NULL;
CampoNode **ultimo = &lista_completa;

// Variáveis do programa - INICIALIZADAS
char *web_space = "./meu-webspace/";
char *arquivo_req = "requisicao.txt";
char *arquivo_resp = "resposta.txt";
char *arquivo_registro = "registro.txt";

// Protótipos das funções do processar-requisicoes.c
void send_error_page(int status, char* connection, char* additional_info);
int process_GET(char* caminhos[], char* connection);
int process_HEAD(char* caminhos[], char* connection);
int process_OPTIONS(char* connection);
int process_TRACE(char* connection);

// Função para enviar resposta pelo socket
void enviar_resposta_para_cliente(int soquete_cliente) {
    FILE *resp_file = fopen(arquivo_resp, "r");
    if (!resp_file) {
        perror("Erro ao abrir arquivo de resposta");
        const char *error_msg = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
        write(soquete_cliente, error_msg, strlen(error_msg));
        return;
    }
    
    char buffer[4096];
    size_t bytes_lidos;
    
    while ((bytes_lidos = fread(buffer, 1, sizeof(buffer), resp_file)) > 0) {
        if (write(soquete_cliente, buffer, bytes_lidos) < 0) {
            perror("Erro ao enviar resposta");
            break;
        }
    }
    
    fclose(resp_file);
}

// Função para processar requisição completa
void processar_requisicao_completa(int soquete_cliente, char *requisicao) {
    printf("=== REQUISIÇÃO RECEBIDA ===\n");
    printf("%s\n", requisicao);
    printf("=== FIM DA REQUISIÇÃO ===\n\n");
    
    // Configurar o parser para ler da string (conforme especificado no roteiro)
    YY_BUFFER_STATE buffer = yy_scan_string(requisicao);
    
    // Executar parser
    printf("=== PROCESSANDO REQUISIÇÃO COM PARSER ===\n");
    yyparse();
    
    // Imprimir resultado da análise
    printf("=== ESTRUTURA DE DADOS DO PARSER ===\n");
    imprimirLista(lista_completa);
    
    // Limpar buffer do lexer
    yy_delete_buffer(buffer);
    
    // Processar requisição e gerar resposta em arquivo
    printf("=== EXECUTANDO REQUISIÇÃO ===\n");
    
    // Usar redirecionamento temporário para arquivo (apenas para esta versão)
    int stdout_backup = dup(STDOUT_FILENO);
    FILE *temp_resp = fopen(arquivo_resp, "w");
    if (!temp_resp) {
        perror("Erro ao criar arquivo de resposta temporário");
        close(soquete_cliente);
        return;
    }
    dup2(fileno(temp_resp), STDOUT_FILENO);
    
    // Processar a requisição (isso vai escrever no arquivo)
    int status = processar_requisicao(requisicao, web_space, arquivo_resp);
    
    // Restaurar stdout
    fflush(stdout);
    dup2(stdout_backup, STDOUT_FILENO);
    close(stdout_backup);
    fclose(temp_resp);
    
    printf("Status da requisição: %d\n", status);
    
    // Enviar resposta para o cliente
    enviar_resposta_para_cliente(soquete_cliente);
    
    // Registrar no arquivo de registro
    char *metodo, *path;
    if (extrair_metodo_path(requisicao, &metodo, &path)) {
        int apenas_cabecalho = (strcmp(metodo, "HEAD") == 0);
        registrar_requisicao_resposta(requisicao, arquivo_resp, arquivo_registro, apenas_cabecalho);
        
        free(metodo);
        free(path);
    }
    
    printf("Requisição processada e resposta enviada!\n");
    
    // Limpar lista para próxima requisição
    liberarLista(lista_completa);
    lista_completa = NULL;
    ultimo = &lista_completa;
}

// Função principal de processamento
int processar_requisicao(char *requisicao, const char *web_space, const char *arquivo_resp) {
    char *metodo, *path;
    
    if (!extrair_metodo_path(requisicao, &metodo, &path)) {
        send_error_page(HTTP_INTERNAL_ERROR, "close", "Falha ao extrair método e path");
        return HTTP_INTERNAL_ERROR;
    }
    
    int status_code;
    char *connection = "close"; // Para esta versão, sempre fechar conexão
    
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
    
    free(metodo);
    free(path);
    return status_code;
}

// Função para configurar conexão de rede
void conectar_na_internet(char *endereco_ip, int porta, int* soquete, struct sockaddr_in* endereco_servidor) {
    // cria socket TCP
    *soquete = socket(AF_INET, SOCK_STREAM, 0);
    if (*soquete < 0) {
        perror("Erro ao criar socket");
        exit(1);
    }

    // permite reuso rápido da porta
    int opt = 1;
    if (setsockopt(*soquete, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Erro no setsockopt");
        exit(1);
    }

    // configura endereço/porta
    memset(endereco_servidor, 0, sizeof(*endereco_servidor));
    endereco_servidor->sin_family = AF_INET;
    endereco_servidor->sin_port = htons(porta);
    
    // Decidir se usa IP específico ou qualquer interface
    if (strcmp(endereco_ip, "0.0.0.0") == 0) {
        endereco_servidor->sin_addr.s_addr = INADDR_ANY;
        printf("Servidor escutando em todas as interfaces na porta %d\n", porta);
    } else {
        if (inet_aton(endereco_ip, &endereco_servidor->sin_addr) == 0) {
            fprintf(stderr, "Endereço IP inválido: %s\n", endereco_ip);
            exit(1);
        }
        printf("Servidor escutando em %s:%d\n", endereco_ip, porta);
    }

    if (bind(*soquete, (struct sockaddr*)endereco_servidor, sizeof(*endereco_servidor)) < 0) {
        perror("Erro no bind");
        fprintf(stderr, "IP: %s, Porta: %d\n", endereco_ip, porta);
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

    int soquete_servidor, soquete_cliente;
    struct sockaddr_in endereco_servidor, endereco_cliente;
    socklen_t tam_endereco = sizeof(endereco_cliente);

    conectar_na_internet(endereco_ip, porta, &soquete_servidor, &endereco_servidor);

    printf("Servidor HTTP mono-thread pronto para receber conexões...\n");
    printf("Webspace: %s\n", web_space);
    printf("Pressione Ctrl+C para parar o servidor\n\n");

    while (1) {
        soquete_cliente = accept(soquete_servidor, (struct sockaddr*)&endereco_cliente, &tam_endereco);
        if (soquete_cliente < 0) {
            perror("Erro no accept");
            continue;
        }

        // Mostrar quem conectou
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &endereco_cliente.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("=== NOVA CONEXÃO de %s:%d ===\n", client_ip, ntohs(endereco_cliente.sin_port));

        char requisicao[8192]; // Buffer maior para requisições completas
        memset(requisicao, 0, sizeof(requisicao));
        
        // Ler requisição do cliente
        ssize_t bytes_lidos = read(soquete_cliente, requisicao, sizeof(requisicao) - 1);
        if (bytes_lidos > 0) {
            requisicao[bytes_lidos] = '\0';
            processar_requisicao_completa(soquete_cliente, requisicao);
        } else {
            perror("Erro na leitura da requisição");
        }
        
        close(soquete_cliente);
        printf("=== CONEXÃO COM %s FECHADA ===\n\n", client_ip);
    }

    close(soquete_servidor);
    return 0;
}