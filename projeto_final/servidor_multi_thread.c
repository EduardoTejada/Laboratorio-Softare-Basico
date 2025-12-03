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
#include <errno.h>
#include <pthread.h>
#include "inc/myString.h"
#include <sys/time.h>
#include <sys/types.h>

/* declarações vindas do Flex */
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char *yy_str);
extern void yy_delete_buffer(YY_BUFFER_STATE b);
extern FILE *yyin;
extern int yyparse();

// Definição das variáveis globais
CampoNode *lista_completa = NULL;
CampoNode **ultimo = &lista_completa;

// Mutex para proteger o parser
pthread_mutex_t parser_mutex = PTHREAD_MUTEX_INITIALIZER;

// Estrutura para passar dados para a thread
typedef struct {
    int soquete_cliente;
    char client_ip[INET_ADDRSTRLEN];
    int client_port;
} thread_data_t;


// Variáveis do programa
char *web_space = "./webspace_para_testes/";
char *arquivo_req = "requisicao.txt";
char *arquivo_resp = "resposta.txt";
char *arquivo_registro = "registro.txt";

// Configuração de multithreading
int max_threads = 64;
int threads_ativas = 0; // Contador de threads ativas
pthread_mutex_t threads_mutex = PTHREAD_MUTEX_INITIALIZER;

// Protótipos das funções do processar-requisicoes.c
void send_error_page(int status, char* connection, char* additional_info);
int process_GET(char* caminhos[], char* connection, CampoNode* lista_campos);
int process_POST(char* caminhos[], char* connection, CampoNode* lista_campos, const char* corpo_post);
int process_HEAD(char* caminhos[], char* connection, CampoNode* lista_campos);
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


// Função auxiliar para extrair corpo da requisição
char* extrair_corpo_requisicao(const char* requisicao) {
    // Encontrar o final dos headers (linha em branco)
    char* fim_headers = strstr(requisicao, "\r\n\r\n");
    if (fim_headers == NULL) {
        fim_headers = strstr(requisicao, "\n\n");
        if (fim_headers == NULL) {
            return NULL; // Sem corpo
        }
        fim_headers += 2; // Pular \n\n
    } else {
        fim_headers += 4; // Pular \r\n\r\n
    }
    
    if (*fim_headers == '\0') {
        return NULL; // Corpo vazio
    }
    
    fprintf(stderr, "DEBUG: Corpo extraído: %s\n", fim_headers);
    return strdup(fim_headers);
}


// Função principal de processamento
int processar_requisicao(char *requisicao, const char *web_space, const char *arquivo_resp, CampoNode* lista_campos) {
    char *metodo, *path;
    
    if (!extrair_metodo_path(requisicao, &metodo, &path)) {
        send_error_page(HTTP_INTERNAL_ERROR, "close", "Falha ao extrair método e path");
        return HTTP_INTERNAL_ERROR;
    }
    
    int status_code;
    char *connection = "keep-alive";
    
    // Preparar argumentos para as funções de processamento
    char *argv[] = {"servidor", (char*)web_space, (char*)path, NULL};
    
    if (strcmp(metodo, "GET") == 0) {
        status_code = process_GET(argv, connection, lista_campos);
    } else if (strcmp(metodo, "HEAD") == 0) {
        status_code = process_HEAD(argv, connection, lista_campos);
    } else if (strcmp(metodo, "OPTIONS") == 0) {
        status_code = process_OPTIONS(connection);
    } else if (strcmp(metodo, "TRACE") == 0) {
        status_code = process_TRACE(connection);
    } else if (strcmp(metodo, "POST") == 0) {
        char* corpo = extrair_corpo_requisicao(requisicao);
        status_code = process_POST(argv, connection, lista_campos, corpo);
        if (corpo) free(corpo);
    } else if (strcmp(metodo, "PUT") == 0) {
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



// Função para processar requisição completa
void processar_requisicao_completa(int soquete_cliente, char *requisicao) {
    printf("[Thread %ld] === REQUISIÇÃO RECEBIDA ===\n", pthread_self());
    printf("%s\n", requisicao);
    printf("[Thread %ld] === FIM DA REQUISIÇÃO ===\n\n", pthread_self());
    
    // Proteger o parser com mutex
    pthread_mutex_lock(&parser_mutex);

    // Configurar o parser para ler da string
    YY_BUFFER_STATE buffer = yy_scan_string(requisicao);
    
    // Executar parser
    printf("[Thread %ld] === PROCESSANDO REQUISIÇÃO COM PARSER ===\n", pthread_self());
    yyparse();

    // Limpar buffer do lexer
    yy_delete_buffer(buffer);
    
    // Processar requisição e gerar resposta em arquivo
    printf("[Thread %ld] === EXECUTANDO REQUISIÇÃO ===\n", pthread_self());
    
    // Usar redirecionamento temporário para arquivo
    int stdout_backup = dup(STDOUT_FILENO);
    FILE *temp_resp = fopen(arquivo_resp, "w");
    if (!temp_resp) {
        perror("Erro ao criar arquivo de resposta temporário");
        close(soquete_cliente);
        return;
    }
    dup2(fileno(temp_resp), STDOUT_FILENO);
    
    // Processar a requisição
    int status = processar_requisicao(requisicao, web_space, arquivo_resp, lista_completa);
    
    // Restaurar stdout
    fflush(stdout);
    dup2(stdout_backup, STDOUT_FILENO);
    close(stdout_backup);
    fclose(temp_resp);
    
    printf("[Thread %ld] Status da requisição: %d\n", pthread_self(), status);

    // Enviar resposta para o cliente
    enviar_resposta_para_cliente(soquete_cliente);
    
    // Limpar lista
    liberarLista(lista_completa);
    lista_completa = NULL;
    ultimo = &lista_completa;
    
    pthread_mutex_unlock(&parser_mutex);
    
    printf("[Thread %ld] Requisição processada!\n", pthread_self());
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
    
    // testa endereço
    if (inet_aton(endereco_ip, &endereco_servidor->sin_addr) == 0) {
        fprintf(stderr, "Endereço IP inválido: %s\n", endereco_ip);
        exit(1);
    }
    printf("Servidor escutando em %s:%d\n", endereco_ip, porta);

    // tentando bind
    if (bind(*soquete, (struct sockaddr*)endereco_servidor, sizeof(*endereco_servidor)) < 0) {
        perror("Erro no bind");
        fprintf(stderr, "IP: %s, Porta: %d\n", endereco_ip, porta);
        exit(1);
    }

    printf("DEBUG: Bind bem-sucedido em %s:%d\n", endereco_ip, porta);

    // começa a escutar a porta
    if (listen(*soquete, 5) < 0) {
        perror("Erro no listen");
        exit(1);
    }
}


// Função executada por cada thread para tratar a conexão
void* thread_trata_conexao(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    int soquete_cliente = data->soquete_cliente;
    
    printf("[Thread %ld] Iniciada para cliente %s:%d\n", 
           pthread_self(), data->client_ip, data->client_port);
    
    char requisicao[8192];
    memset(requisicao, 0, sizeof(requisicao));
    
    struct timeval tv;
    tv.tv_sec = 4;  // 4 segundos de timeout
    tv.tv_usec = 0;
    setsockopt(soquete_cliente, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    // Ler requisição do cliente
    ssize_t bytes_lidos;
    do {
        bytes_lidos = read(soquete_cliente, requisicao, sizeof(requisicao) - 1);
    } while (bytes_lidos == -1 && errno == EINTR); // Proteção EINTR
    
    if (bytes_lidos > 0) {
        requisicao[bytes_lidos] = '\0';
        printf("[Thread %ld] Dados recebidos (%zd bytes) de %s:%d\n", 
               pthread_self(), bytes_lidos, data->client_ip, data->client_port);    
        processar_requisicao_completa(soquete_cliente, requisicao);
    } else {
        printf("[Thread %ld] Erro na leitura dos dados de %s:%d\n", 
               pthread_self(), data->client_ip, data->client_port);
    }
    
    close(soquete_cliente);
    
    // Decrementar contador de threads ativas
    pthread_mutex_lock(&threads_mutex);
    threads_ativas--;
    printf("[Thread %ld] Terminada. Threads ativas: %d/%d\n", 
           pthread_self(), threads_ativas, max_threads);
    pthread_mutex_unlock(&threads_mutex);
    
    free(data);
    pthread_exit(NULL);
}

// Aceitação com preenchimento dos dados do cliente
int aceitar_conexao_completa(int soquete_servidor, struct sockaddr_in *endereco_cliente, char *client_ip, int *client_port) {
    socklen_t tam_endereco = sizeof(*endereco_cliente);
    
    // Configurar socket como não-bloqueante temporariamente
    int flags = fcntl(soquete_servidor, F_GETFL, 0);
    fcntl(soquete_servidor, F_SETFL, flags | O_NONBLOCK);
    
    // Aceitar a conexão com cliente
    int soquete_cliente = accept(soquete_servidor, (struct sockaddr*)endereco_cliente, &tam_endereco);
    
    // Restaurar modo bloqueante
    fcntl(soquete_servidor, F_SETFL, flags);
    
    if (soquete_cliente < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return -1; // Não há conexões pendentes
        }
        perror("Erro no accept");
        return -1;
    }
    
    // Preencher dados do cliente
    inet_ntop(AF_INET, &endereco_cliente->sin_addr, client_ip, INET_ADDRSTRLEN);
    *client_port = ntohs(endereco_cliente->sin_port);
    
    return soquete_cliente;
}

int main(int argc, char *argv[]) {
    if(argc != 4){
        fprintf(stderr, "Uso: %s [endereco IP] [porta] [max_threads]\n", argv[0]);
        exit(1);
    }

    char *endereco_ip = argv[1];
    int porta = atoi(argv[2]);
    max_threads = atoi(argv[3]);

    int soquete_servidor;
    struct sockaddr_in endereco_servidor;
    
    conectar_na_internet(endereco_ip, porta, &soquete_servidor, &endereco_servidor);

    printf("Servidor HTTP multi-thread pronto para receber conexões...\n");
    printf("Webspace: %s\n", web_space);
    printf("Máximo de threads: %d\n", max_threads);

    while (1) {
        //printf("DEBUG: Threads ativas: %d/%d\n", threads_ativas, max_threads);
        struct sockaddr_in endereco_cliente;
        char client_ip[INET_ADDRSTRLEN];
        int client_port;
        
        // Aceitar conexão com dados completos do cliente
        int soquete_cliente = aceitar_conexao_completa(soquete_servidor, &endereco_cliente, client_ip, &client_port);
        
        if (soquete_cliente < 0) {
            // Não há conexão pendente, pausa breve para não consumir 100% CPU
            usleep(10000); // 10ms
            continue;
        }
        
        // conexão com cliente bem-sucedida
        printf("=== NOVA CONEXÃO de %s:%d ===\n", client_ip, client_port);

        // verifica limite de threads
        pthread_mutex_lock(&threads_mutex);
        if (threads_ativas >= max_threads) {
            pthread_mutex_unlock(&threads_mutex);
            // envia resposta para o cliente
            printf("LIMITE ATINGIDO: %d threads. Rejeitando conexão.\n", threads_ativas);
            const char *error_msg = "HTTP/1.1 503 Service Unavailable\r\n\r\n";
            write(soquete_cliente, error_msg, strlen(error_msg));
            close(soquete_cliente);
            continue;
        }
        // não chegou no limite de threads
        threads_ativas++;
        pthread_mutex_unlock(&threads_mutex);

        // Preparar dados para a thread
        thread_data_t* thread_data = malloc(sizeof(thread_data_t));
        if (!thread_data) {
            perror("Erro ao alocar memória para thread");
            pthread_mutex_lock(&threads_mutex);
            threads_ativas--;
            pthread_mutex_unlock(&threads_mutex);
            close(soquete_cliente);
            continue;
        }
        
        // preenche dados para a thread
        thread_data->soquete_cliente = soquete_cliente;
        strncpy(thread_data->client_ip, client_ip, INET_ADDRSTRLEN);
        thread_data->client_port = client_port;
        
        // cria a thread
        pthread_t thread_id;
        int rc = pthread_create(&thread_id, NULL, thread_trata_conexao, thread_data);
        
        if (rc == 0) {
            printf("[Main] Thread %ld criada. Threads ativas: %d/%d\n", 
                   thread_id, threads_ativas, max_threads);
            pthread_detach(thread_id); // evita zombies
        } else {
            perror("Erro ao criar thread");
            pthread_mutex_lock(&threads_mutex);
            threads_ativas--;
            pthread_mutex_unlock(&threads_mutex);
            free(thread_data);
            close(soquete_cliente);
        }
    }

    close(soquete_servidor);
    pthread_mutex_destroy(&parser_mutex);
    pthread_mutex_destroy(&threads_mutex);
    
    return 0;
}