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
#include <poll.h>
#include <errno.h>
#include <pthread.h>

/* declarações vindas do Flex */
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char *yy_str);
extern void yy_delete_buffer(YY_BUFFER_STATE b);
extern FILE *yyin;
extern int yyparse();

// Definição das variáveis globais
CampoNode *lista_completa = NULL;
CampoNode **ultimo = &lista_completa;
/*typedef struct {
    CampoNode *lista_completa;
    CampoNode **ultimo;
} parser_context_t;*/

// MUTEX PARA PROTEGER O PARSER
pthread_mutex_t parser_mutex = PTHREAD_MUTEX_INITIALIZER;

// Variáveis do programa
char *web_space = "./meu-webspace/";
char *arquivo_req = "requisicao.txt";
char *arquivo_resp = "resposta.txt";
char *arquivo_registro = "registro.txt";

// Configuração de multithreading
//#define MAX_THREADS 64  // Número máximo de threads
int max_threads = 64;
int threads_ativas = 0; // Contador de threads ativas
pthread_mutex_t threads_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    printf("[Thread %ld] === REQUISIÇÃO RECEBIDA ===\n", pthread_self());
    printf("%s\n", requisicao);
    printf("[Thread %ld] === FIM DA REQUISIÇÃO ===\n\n", pthread_self());
    
    // Fazer parsing RÁPIDO protegido por mutex
    pthread_mutex_lock(&parser_mutex);
    YY_BUFFER_STATE buffer = yy_scan_string(requisicao);
    yyparse();
    yy_delete_buffer(buffer);
    pthread_mutex_unlock(&parser_mutex);
    
    /*
    // PROTEGER O PARSER COM MUTEX
    pthread_mutex_lock(&parser_mutex);

    // Configurar o parser para ler da string
    YY_BUFFER_STATE buffer = yy_scan_string(requisicao);
    
    // Executar parser
    printf("[Thread %ld] === PROCESSANDO REQUISIÇÃO COM PARSER ===\n", pthread_self());
    yyparse();
    
    // Imprimir resultado da análise
    printf("[Thread %ld] === ESTRUTURA DE DADOS DO PARSER ===\n", pthread_self());
    imprimirLista(lista_completa);
    
    // Limpar buffer do lexer
    yy_delete_buffer(buffer);*/
    
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
    int status = processar_requisicao(requisicao, web_space, arquivo_resp);
    
    // Restaurar stdout
    fflush(stdout);
    dup2(stdout_backup, STDOUT_FILENO);
    close(stdout_backup);
    fclose(temp_resp);
    
    printf("[Thread %ld] Status da requisição: %d\n", pthread_self(), status);

    // Enviar resposta para o cliente
    enviar_resposta_para_cliente(soquete_cliente);
    
    // Limpar lista (proteger com mutex)
    pthread_mutex_lock(&parser_mutex);
    liberarLista(lista_completa);
    lista_completa = NULL;
    ultimo = &lista_completa;
    pthread_mutex_unlock(&parser_mutex);
    
    printf("[Thread %ld] Requisição processada!\n", pthread_self());
}

// Função principal de processamento
int processar_requisicao(char *requisicao, const char *web_space, const char *arquivo_resp) {
    char *metodo, *path;
    
    if (!extrair_metodo_path(requisicao, &metodo, &path)) {
        send_error_page(HTTP_INTERNAL_ERROR, "close", "Falha ao extrair método e path");
        return HTTP_INTERNAL_ERROR;
    }
    
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
    
    free(metodo);
    free(path);
    return status_code;
}

/*
void processar_requisicao_completa(int soquete_cliente, char *requisicao) {
    printf("[Thread %ld] === REQUISIÇÃO RECEBIDA ===\n", pthread_self());
    printf("%s\n", requisicao);
    printf("[Thread %ld] === FIM DA REQUISIÇÃO ===\n\n", pthread_self());
    
    // PROCESSAMENTO SIMPLIFICADO SEM PARSER
    printf("[Thread %ld] === EXECUTANDO REQUISIÇÃO (SEM PARSER) ===\n", pthread_self());
    
    // Usar redirecionamento temporário para arquivo
    int stdout_backup = dup(STDOUT_FILENO);
    FILE *temp_resp = fopen(arquivo_resp, "w");
    if (!temp_resp) {
        perror("Erro ao criar arquivo de resposta temporário");
        close(soquete_cliente);
        return;
    }
    dup2(fileno(temp_resp), STDOUT_FILENO);
    
    // Processar a requisição SEM o parser
    // Usar uma função simplificada que não depende do parser
    int status = processar_requisicao_simples(requisicao, web_space, arquivo_resp);
    
    // Restaurar stdout
    fflush(stdout);
    dup2(stdout_backup, STDOUT_FILENO);
    close(stdout_backup);
    fclose(temp_resp);
    
    printf("[Thread %ld] Status da requisição: %d\n", pthread_self(), status);

    // Enviar resposta para o cliente
    enviar_resposta_para_cliente(soquete_cliente);
    
    printf("[Thread %ld] Requisição processada e resposta enviada!\n", pthread_self());
}

// Função simplificada sem parser
int processar_requisicao_simples(char *requisicao, const char *web_space, const char *arquivo_resp) {
    // Extrair método e path de forma simples
    char *metodo = "GET"; // Simplificação
    char *path = "/index.html"; // Simplificação
    
    char *argv[] = {"servidor", (char*)web_space, path, NULL};
    
    if (strcmp(metodo, "GET") == 0) {
        return process_GET(argv, "close");
    }
    
    return HTTP_OK;
}*/

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

    printf("DEBUG: Tentando bind em %s:%d\n", endereco_ip, porta);

    if (bind(*soquete, (struct sockaddr*)endereco_servidor, sizeof(*endereco_servidor)) < 0) {
        perror("Erro no bind");
        fprintf(stderr, "IP: %s, Porta: %d\n", endereco_ip, porta);
        exit(1);
    }

    printf("DEBUG: Bind bem-sucedido em %s:%d\n", endereco_ip, porta);

    if (listen(*soquete, 5) < 0) {
        perror("Erro no listen");
        exit(1);
    }
}

// Aceitar conexão com timeout usando POLL
int aceitar_com_timeout_poll(int soquete_servidor, int timeout_segundos) {
    struct pollfd fds[1];
    int resultado;
    
    // Configurar estrutura poll
    fds[0].fd = soquete_servidor;
    fds[0].events = POLLIN;  // Esperar por dados para leitura
    fds[0].revents = 0;
    
    printf("[POLL] Aguardando conexão (timeout: %d segundos)...\n", timeout_segundos);
    
    // Esperar por atividade no socket com timeout
    resultado = poll(fds, 1, timeout_segundos * 1000);
    
    if (resultado == -1) {
        // Verificar se foi interrupção por sinal (EINTR)
        if (errno == EINTR) {
            printf("[POLL] Chamada interrompida por sinal (EINTR), reiniciando...\n");
            return -3; // Código especial para EINTR
        }
        perror("[POLL] Erro no poll");
        return -1;
    } else if (resultado == 0) {
        printf("[POLL] Timeout de %d segundos - nenhuma conexão recebida\n", timeout_segundos);
        return -2; // Código especial para timeout
    }
    
    // Verificar se o socket está realmente pronto para leitura
    if (fds[0].revents & POLLIN) {
        printf("[POLL] Socket pronto para aceitar conexão\n");
        return 1;
    }
    
    return -1;
}

// Estrutura para passar dados para a thread
typedef struct {
    int soquete_cliente;
    char client_ip[INET_ADDRSTRLEN];
    int client_port;
} thread_data_t;

// Função executada por cada thread para tratar a conexão
void* thread_trata_conexao(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    int soquete_cliente = data->soquete_cliente;
    
    printf("[Thread %ld] Iniciada para cliente %s:%d\n", 
           pthread_self(), data->client_ip, data->client_port);
    
    char requisicao[8192];
    memset(requisicao, 0, sizeof(requisicao));
    
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

int main(int argc, char *argv[]) {
    if(argc != 4){
        fprintf(stderr, "Uso: %s [endereco IP] [porta] [max_threads]\n", argv[0]);
        exit(1);
    }

    char *endereco_ip = argv[1];
    int porta = atoi(argv[2]);
    max_threads = atoi(argv[3]);

    int soquete_servidor, soquete_cliente;
    struct sockaddr_in endereco_servidor, endereco_cliente;
    socklen_t tam_endereco = sizeof(endereco_cliente);

    conectar_na_internet(endereco_ip, porta, &soquete_servidor, &endereco_servidor);

    printf("Servidor HTTP multi-thread pronto para receber conexões...\n");
    printf("Webspace: %s\n", web_space);
    printf("Máximo de threads: %d\n", max_threads);

    while (1) {
        printf("\n================\nAguardando próxima requisição\n================\n");
        printf("Threads ativas: %d/%d\n", threads_ativas, max_threads);

        // Verificar limite de threads
        if (threads_ativas >= max_threads) {
            printf("LIMITE ATINGIDO: %d threads ativas. Aguardando...\n", threads_ativas);
            sleep(2);
            continue;
        }
        
        // USAR POLL PARA ACEITAR CONEXÕES COM TIMEOUT E PROTEÇÃO EINTR
        int status_aceitacao;
        do {
            status_aceitacao = aceitar_com_timeout_poll(soquete_servidor, 4);
        } while (status_aceitacao == -3); // Re-tentar se foi EINTR
        
        if (status_aceitacao == -2) {
            // Timeout - continuar loop sem erro
            printf("Continuando após timeout de aceitação\n");
            continue;
        } else if (status_aceitacao == -1) {
            // Erro - pausa antes de continuar
            printf("Erro na aceitação, continuando em 2 segundos\n");
            sleep(2);
            continue;
        }

        // ACEITAR CONEXÃO COM PROTEÇÃO EINTR
        do {
            soquete_cliente = accept(soquete_servidor, (struct sockaddr*)&endereco_cliente, &tam_endereco);
        } while (soquete_cliente == -1 && errno == EINTR);
        
        if (soquete_cliente < 0) {
            perror("Erro no accept após poll");
            continue;
        }
        
        // Mostrar quem conectou
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &endereco_cliente.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(endereco_cliente.sin_port);
        printf("=== NOVA CONEXÃO de %s:%d ===\n", client_ip, client_port);

        // Preparar dados para a thread
        thread_data_t* thread_data = malloc(sizeof(thread_data_t));
        if (!thread_data) {
            perror("Erro ao alocar memória para thread");
            close(soquete_cliente);
            continue;
        }
        
        thread_data->soquete_cliente = soquete_cliente;
        strncpy(thread_data->client_ip, client_ip, INET_ADDRSTRLEN);
        thread_data->client_port = client_port;
        
        // CRIAR THREAD PARA ATENDER REQUISIÇÃO
        pthread_t thread_id;
        int rc = pthread_create(&thread_id, NULL, thread_trata_conexao, thread_data);
        
        if (rc == 0) {
            // Thread criada com sucesso
            pthread_mutex_lock(&threads_mutex);
            threads_ativas++;
            printf("[Main] Thread %ld criada. Threads ativas: %d/%d\n", 
                   thread_id, threads_ativas, max_threads);
            pthread_mutex_unlock(&threads_mutex);
            
            // Usar detach para a thread se auto-liberar ao terminar
            pthread_detach(thread_id);
        } else {
            // Erro na criação da thread
            perror("Erro ao criar thread");
            free(thread_data);
            close(soquete_cliente);
        }
    }

    close(soquete_servidor);

    // DESTRUIR O MUTEX AO FINAL
    pthread_mutex_destroy(&parser_mutex);
    pthread_mutex_destroy(&threads_mutex);
    
    return 0;
}
