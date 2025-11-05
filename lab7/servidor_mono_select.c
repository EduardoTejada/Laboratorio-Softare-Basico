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

/* declaraçoes vindas do Flex */
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char *yy_str);
extern void yy_delete_buffer(YY_BUFFER_STATE b);
extern FILE *yyin;
extern int yyparse();

// Definiçao das variáveis globais
CampoNode *lista_completa = NULL;
CampoNode **ultimo = &lista_completa;

// Variáveis do programa
char *web_space = "./meu-webspace/";
char *arquivo_req = "requisicao.txt";
char *arquivo_resp = "resposta.txt";
char *arquivo_registro = "registro.txt";

// Prototipos das funçoes do processar-requisicoes.c
void send_error_page(int status, char* connection, char* additional_info);
int process_GET(char* caminhos[], char* connection);
int process_HEAD(char* caminhos[], char* connection);
int process_OPTIONS(char* connection);
int process_TRACE(char* connection);

// Funçao para enviar resposta pelo socket
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

// Funçao para processar requisiçao completa
void processar_requisicao_completa(int soquete_cliente, char *requisicao) {
    printf("=== REQUISIÇaO RECEBIDA ===\n");
    printf("%s\n", requisicao);
    printf("=== FIM DA REQUISIÇaO ===\n\n");
    
    // Configurar o parser para ler da string (conforme especificado no roteiro)
    YY_BUFFER_STATE buffer = yy_scan_string(requisicao);
    
    // Executar parser
    printf("=== PROCESSANDO REQUISIÇaO COM PARSER ===\n");
    yyparse();
    
    // Imprimir resultado da análise
    printf("=== ESTRUTURA DE DADOS DO PARSER ===\n");
    imprimirLista(lista_completa);
    
    // Limpar buffer do lexer
    yy_delete_buffer(buffer);
    
    // Processar requisiçao e gerar resposta em arquivo
    printf("=== EXECUTANDO REQUISIÇaO ===\n");
    
    // Usar redirecionamento temporário para arquivo (apenas para esta versao)
    int stdout_backup = dup(STDOUT_FILENO);
    FILE *temp_resp = fopen(arquivo_resp, "w");
    if (!temp_resp) {
        perror("Erro ao criar arquivo de resposta temporário");
        close(soquete_cliente);
        return;
    }
    dup2(fileno(temp_resp), STDOUT_FILENO);
    
    // Processar a requisiçao (isso vai escrever no arquivo)
    int status = processar_requisicao(requisicao, web_space, arquivo_resp);
    
    // Restaurar stdout
    fflush(stdout);
    dup2(stdout_backup, STDOUT_FILENO);
    close(stdout_backup);
    fclose(temp_resp);
    
    printf("Status da requisiçao: %d\n", status);
    
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
    
    printf("Requisiçao processada e resposta enviada!\n");
    
    // Limpar lista para proxima requisiçao
    liberarLista(lista_completa);
    lista_completa = NULL;
    ultimo = &lista_completa;
}

// Funçao principal de processamento
int processar_requisicao(char *requisicao, const char *web_space, const char *arquivo_resp) {
    char *metodo, *path;
    
    if (!extrair_metodo_path(requisicao, &metodo, &path)) {
        send_error_page(HTTP_INTERNAL_ERROR, "close", "Falha ao extrair metodo e path");
        return HTTP_INTERNAL_ERROR;
    }
    
    int status_code;
    char *connection = "close"; // Para esta versao, sempre fechar conexao
    
    // Preparar argumentos para as funçoes de processamento
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

// Funçao para configurar conexao de rede
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
    
    // Decidir se usa IP especifico ou qualquer interface
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

// Aceitar conexao com timeout usando select
int aceitar_com_timeout(int soquete_servidor, int timeout_segundos) {
    fd_set conj_leitura;
    struct timeval timeout;
    int resultado;
    
    // Inicializar o conjunto de descritores
    FD_ZERO(&conj_leitura);
    FD_SET(soquete_servidor, &conj_leitura);
    
    // Configurar timeout de 4 segundos
    timeout.tv_sec = timeout_segundos;
    timeout.tv_usec = 0;
    
    printf("[SELECT] Aguardando conexao (timeout: %d segundos)...\n", timeout_segundos);
    
    // Esperar por atividade no socket com timeout
    resultado = select(soquete_servidor + 1, &conj_leitura, NULL, NULL, &timeout);
    
    if (resultado == -1) {
        perror(" [SELECT] Erro no select");
        return -1;
    } else if (resultado == 0) {
        printf("[SELECT] Timeout de %d segundos - nenhuma conexao recebida\n", timeout_segundos);
        return -2; // Codigo especial para timeout
    }
    
    // Verificar se o socket está realmente pronto
    if (FD_ISSET(soquete_servidor, &conj_leitura)) {
        printf(" [SELECT] Socket pronto para aceitar conexao\n");
        return 1;
    }
    
    return -1;
}

// Funçao para ler dados do cliente com timeout
int ler_dados_com_timeout(int soquete_cliente, char *buffer, size_t tamanho_buffer, int timeout_segundos) {
    fd_set conj_leitura;
    struct timeval timeout;
    int resultado;
    
    FD_ZERO(&conj_leitura);
    FD_SET(soquete_cliente, &conj_leitura);
    
    timeout.tv_sec = timeout_segundos;
    timeout.tv_usec = 0;
    
    printf("Aguardando dados do cliente (timeout: %d segundos)...\n", timeout_segundos);
    
    resultado = select(soquete_cliente + 1, &conj_leitura, NULL, NULL, &timeout);
    
    if (resultado == -1) {
        perror("Erro no select (leitura)");
        return -1;
    } else if (resultado == 0) {
        printf("Timeout de leitura - cliente nao enviou dados em %d segundos\n", timeout_segundos);
        return -2;
    }
    
    // Socket pronto para leitura
    printf("Socket pronto para leitura de dados\n");
    ssize_t bytes_lidos = read(soquete_cliente, buffer, tamanho_buffer - 1);
    
    if (bytes_lidos > 0) {
        buffer[bytes_lidos] = '\0';
        printf("Dados recebidos (%zd bytes)\n", bytes_lidos);
    }
    
    return bytes_lidos;
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

    printf("Servidor HTTP mono-thread pronto para receber conexoes...\n");
    printf("Webspace: %s\n", web_space);

    while (1) {
        printf("\n================\nAguardando proxima requisiçao\n================\n");
        // USAR SELECT PARA ACEITAR CONEXoES COM TIMEOUT
        int status_aceitacao = aceitar_com_timeout(soquete_servidor, 4);
        
        if (status_aceitacao == -2) {
            // Timeout - continuar loop sem erro
            printf("Continuando apos timeout de aceitacao\n");
            continue;
        } else if (status_aceitacao == -1) {
            // Erro - pausa antes de continuar
            printf("Erro na aceitacao, continuando em 2 segundos\n");
            sleep(2);
            continue;
        }

        soquete_cliente = accept(soquete_servidor, (struct sockaddr*)&endereco_cliente, &tam_endereco);
        if (soquete_cliente < 0) {
            perror("Erro no accept"); 
            continue;
        }

        // Mostrar quem conectou
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &endereco_cliente.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("=== NOVA CONEXaO de %s:%d ===\n", client_ip, ntohs(endereco_cliente.sin_port));

        char requisicao[8192]; // Buffer maior para requisiçoes completas
        memset(requisicao, 0, sizeof(requisicao));
        
        // Ler requisiçao do cliente
        ssize_t bytes_lidos = ler_dados_com_timeout(soquete_cliente, requisicao, sizeof(requisicao), 4);

        if (bytes_lidos == -2) {
            printf("TIMEOUT CRITICO: Cliente conectou mas nao enviou dados em 4 segundos\n");
            const char *msg_timeout = "HTTP/1.1 408 Request Timeout\r\n\r\n<h1>408 Request Timeout</h1>\n";
            write(soquete_cliente, msg_timeout, strlen(msg_timeout));
            close(soquete_cliente);
            continue;
        } else if (bytes_lidos <= 0) {
            // Outro erro na leitura
            printf("Erro na leitura dos dados do cliente\n");
            close(soquete_cliente);
            continue;
        }
        
        processar_requisicao_completa(soquete_cliente, requisicao);
        
        close(soquete_cliente);
        printf("=== CONEXaO COM %s FECHADA ===\n\n", client_ip);
    }

    close(soquete_servidor);
    return 0;
}