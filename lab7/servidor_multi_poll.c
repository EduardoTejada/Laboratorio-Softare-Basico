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
#include <signal.h>
#include <sys/wait.h>


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

// Configuração de multiprocessamento
//#define MAX_PROCESSOS_FILHO 3  // Número máximo de processos filhos
int max_processos = 3;
int processos_ativos = 0;      // Contador de processos ativos

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
    printf("[PID %d] === REQUISIÇÃO RECEBIDA ===\n", getpid());
    printf("%s\n", requisicao);
    printf("[PID %d] === FIM DA REQUISIÇÃO ===\n\n", getpid());
    
    // Configurar o parser para ler da string (conforme especificado no roteiro)
    YY_BUFFER_STATE buffer = yy_scan_string(requisicao);
    
    // Executar parser
    printf("[PID %d] === PROCESSANDO REQUISIÇÃO COM PARSER ===\n", getpid());
    yyparse();
    
    // Imprimir resultado da análise
    printf("[PID %d] === ESTRUTURA DE DADOS DO PARSER ===\n", getpid());
    imprimirLista(lista_completa);
    
    // Limpar buffer do lexer
    yy_delete_buffer(buffer);
    
    // Processar requisiçao e gerar resposta em arquivo
    printf("[PID %d] === EXECUTANDO REQUISIÇÃO ===\n", getpid());
    
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
    
    printf("[PID %d] Status da requisição: %d\n", getpid(), status);

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
    
    printf("[PID %d] Requisição processada e resposta enviada!\n", getpid());

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
    char *connection = "close";
    
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

// Aceitar conexão com timeout usando POLL (substitui select)
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

// Manipulador de sinal para processos filhos (evita zombies)
void manipular_sinal_filho(int sig) {
    int status;
    pid_t pid;
    
    // Coletar processos filhos que terminaram sem bloquear
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        processos_ativos--;
        printf("[SINAL] Processo filho %d terminou. Processos ativos: %d/%d\n", 
               pid, processos_ativos, max_processos);
    }
}

int main(int argc, char *argv[]) {
    if(argc != 4){
        fprintf(stderr, "Uso: %s [endereco IP] [porta] [max_processos]\n", argv[0]);
        exit(1);
    }

    char *endereco_ip = argv[1];
    int porta = atoi(argv[2]);
    max_processos = atoi(argv[3]);

    int soquete_servidor, soquete_cliente;
    struct sockaddr_in endereco_servidor, endereco_cliente;
    socklen_t tam_endereco = sizeof(endereco_cliente);
    
    // Configurar manipulador de sinais para processos filhos
    struct sigaction sa;
    sa.sa_handler = manipular_sinal_filho;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("Erro ao configurar manipulador de sinal");
        exit(1);
    }

    conectar_na_internet(endereco_ip, porta, &soquete_servidor, &endereco_servidor);

    printf("Servidor HTTP multi-thread pronto para receber conexoes...\n");
    printf("Webspace: %s\n", web_space);

    while (1) {
        printf("\n================\nAguardando proxima requisiçao\n================\n");
        printf("PROCESSO PRINCIPAL - Processos ativos: %d/%d\n", processos_ativos, max_processos);

        if (processos_ativos >= max_processos) {
            printf("LIMITE ATINGIDO: %d processos ativos. Enviando mensagem de ocupado.\n", processos_ativos);
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
            printf("Continuando apos timeout de aceitacao\n");
            continue;
        } else if (status_aceitacao == -1) {
            // Erro - pausa antes de continuar
            printf("Erro na aceitacao, continuando em 2 segundos\n");
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
        printf("=== NOVA CONEXaO de %s:%d ===\n", client_ip, ntohs(endereco_cliente.sin_port));

        // CRIAR PROCESSO FILHO PARA ATENDER REQUISIÇÃO
        pid_t pid = fork();
        
        if (pid == 0) {
            // PROCESSO FILHO
            close(soquete_servidor); // Filho não precisa do socket principal

            char requisicao[8192]; // Buffer maior para requisiçoes completas
            memset(requisicao, 0, sizeof(requisicao));

            printf("[PID %d] Processo filho criado para atender cliente\n", getpid());
        
            // Ler requisiçao do cliente
            ssize_t bytes_lidos;
            do {
                bytes_lidos = read(soquete_cliente, requisicao, sizeof(requisicao) - 1);
            } while (bytes_lidos == -1 && errno == EINTR); // Proteção EINTR

            if (bytes_lidos > 0) {
                requisicao[bytes_lidos] = '\0';
                printf("[PID %d] Dados recebidos (%zd bytes) de %s\n", 
                       getpid(), bytes_lidos, client_ip);
                
                processar_requisicao_completa(soquete_cliente, requisicao);
            } else {
                printf("[PID %d] Erro na leitura dos dados\n", getpid());
            }

            close(soquete_cliente);
            printf("[PID %d] Processo filho terminando\n", getpid());
            exit(0);
        } else if (pid > 0) {
            // PROCESSO PAI
            processos_ativos++;
            printf("[PAI] Processo filho %d criado. Processos ativos: %d/%d\n", 
                   pid, processos_ativos, max_processos);
            close(soquete_cliente); // Pai não precisa do socket do cliente
        } else {
            // ERRO NO FORK
            perror("Erro no fork");
            close(soquete_cliente);
        }
    }

    close(soquete_servidor);
    return 0;
}