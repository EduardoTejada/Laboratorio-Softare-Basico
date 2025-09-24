#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

#define HTTP_OK 200
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

// Função para obter a data/hora atual no formato HTTP
char* get_http_date() {
    static char date_str[100];
    time_t now;
    struct tm *tm_info;
    
    time(&now);
    tm_info = gmtime(&now);
    strftime(date_str, sizeof(date_str), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
    
    return date_str;
}

// Função para obter o tipo de conteúdo baseado na extensão do arquivo
char* get_content_type(char* filename) {
    char *ext = strrchr(filename, '.');
    if (ext == NULL) return "text/plain";
    
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0)
        return "text/html";
    else if (strcmp(ext, ".txt") == 0)
        return "text/plain";
    else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
        return "image/jpeg";
    else if (strcmp(ext, ".png") == 0)
        return "image/png";
    else if (strcmp(ext, ".gif") == 0)
        return "image/gif";
    else if (strcmp(ext, ".css") == 0)
        return "text/css";
    else if (strcmp(ext, ".js") == 0)
        return "application/javascript";
    else
        return "application/octet-stream";
}

// Função para gerar páginas de erro HTML
void send_error_page(int status, char* connection, char* additional_info) {
    char status_msg[50];
    char title[100];
    char message[500];
    
    // Definir mensagens baseadas no status
    switch(status) {
        case HTTP_NOT_FOUND:
            strcpy(status_msg, "404 Not Found");
            strcpy(title, "404 - Página Não Encontrada");
            snprintf(message, sizeof(message), 
                    "O recurso solicitado não foi encontrado no servidor.%s%s",
                    additional_info ? "\n<br>" : "",
                    additional_info ? additional_info : "");
            break;
            
        case HTTP_FORBIDDEN:
            strcpy(status_msg, "403 Forbidden");
            strcpy(title, "403 - Acesso Negado");
            snprintf(message, sizeof(message), 
                    "Você não tem permissão para acessar este recurso.%s%s",
                    additional_info ? "\n<br>" : "",
                    additional_info ? additional_info : "");
            break;
            
        case HTTP_METHOD_NOT_ALLOWED:
            strcpy(status_msg, "405 Method Not Allowed");
            strcpy(title, "405 - Método Não Permitido");
            strcpy(message, "O método HTTP utilizado não é suportado para este recurso.");
            break;
            
        case HTTP_NOT_IMPLEMENTED:
            strcpy(status_msg, "501 Not Implemented");
            strcpy(title, "501 - Não Implementado");
            strcpy(message, "O servidor não suporta a funcionalidade solicitada.");
            break;
            
        case HTTP_INTERNAL_ERROR:
        default:
            strcpy(status_msg, "500 Internal Server Error");
            strcpy(title, "500 - Erro Interno do Servidor");
            snprintf(message, sizeof(message), 
                    "Ocorreu um erro interno no servidor.%s%s",
                    additional_info ? "\n<br>" : "",
                    additional_info ? additional_info : "");
            break;
    }
    
    // Gerar página HTML de erro
    char html_content[2048];
    snprintf(html_content, sizeof(html_content),
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>%s</title>\n"
        "</head>\n"
        "<body>\n"
        "    <h1 class=\"error-code\">%d</h1>\n"
        "    <h2 class=\"error-title\">%s</h2>\n"
        "    <div class=\"error-message\">%s</div>\n"
        "    <div class=\"error-info\">\n"
        "        <strong>Data:</strong> %s<br>\n"
        "        <strong>Servidor:</strong> Servidor HTTP ver. 0.1 de Eduardo\n"
        "    </div>\n"
        "</body>\n"
        "</html>",

        title, status, title, message, get_http_date());
    
    int content_length = strlen(html_content);
    
    // Enviar cabeçalhos de erro
    printf("HTTP/1.1 %s\r\n", status_msg);
    printf("Date: %s\r\n", get_http_date());
    printf("Server: Servidor HTTP ver. 0.1 de Eduardo\r\n");
    printf("Connection: %s\r\n", connection);
    printf("Content-Type: text/html; charset=utf-8\r\n");
    printf("Content-Length: %d\r\n", content_length);
    printf("\r\n");
    
    // Enviar corpo HTML apenas se não for HEAD
    if (strcmp(connection, "HEAD") != 0) {
        printf("%s", html_content);
    }
}

// Função para enviar cabeçalhos HTTP normais
void send_headers(int status, char* connection, struct stat *statbuf, char* filename, int is_head) {
    char status_msg[50];
    char* content_type = get_content_type(filename);
    
    // Definir mensagem de status
    switch(status) {
        case HTTP_OK: strcpy(status_msg, "200 OK"); break;
        case HTTP_NOT_FOUND: strcpy(status_msg, "404 Not Found"); break;
        case HTTP_FORBIDDEN: strcpy(status_msg, "403 Forbidden"); break;
        case HTTP_METHOD_NOT_ALLOWED: strcpy(status_msg, "405 Method Not Allowed"); break;
        case HTTP_NOT_IMPLEMENTED: strcpy(status_msg, "501 Not Implemented"); break;
        default: strcpy(status_msg, "500 Internal Server Error"); break;
    }
    
    // Enviar linha de status
    printf("HTTP/1.1 %s\r\n", status_msg);
    
    // Enviar cabeçalhos
    printf("Date: %s\r\n", get_http_date());
    printf("Server: Servidor HTTP ver. 0.1 de Eduardo\r\n");
    printf("Connection: %s\r\n", connection);
    
    if (status == HTTP_OK) {
        char last_modified[100];
        struct tm *tm_info = gmtime(&statbuf->st_mtime);
        strftime(last_modified, sizeof(last_modified), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
        
        printf("Last-Modified: %s\r\n", last_modified);
        printf("Content-Length: %ld\r\n", statbuf->st_size);
        printf("Content-Type: %s\r\n", content_type);
    }
    
    printf("\r\n"); // Linha em branco que separa cabeçalho do corpo
}

// Função para enviar cabeçalhos simples (para OPTIONS e TRACE)
void send_simple_headers(int status, char* connection, long content_length, char* content_type) {
    char status_msg[50];
    
    // Definir mensagem de status
    switch(status) {
        case HTTP_OK: strcpy(status_msg, "200 OK"); break;
        case HTTP_METHOD_NOT_ALLOWED: strcpy(status_msg, "405 Method Not Allowed"); break;
        case HTTP_NOT_IMPLEMENTED: strcpy(status_msg, "501 Not Implemented"); break;
        default: strcpy(status_msg, "500 Internal Server Error"); break;
    }
    
    // Enviar linha de status
    printf("HTTP/1.1 %s\r\n", status_msg);
    
    // Enviar cabeçalhos
    printf("Date: %s\r\n", get_http_date());
    printf("Server: Servidor HTTP ver. 0.1 de Eduardo\r\n");
    printf("Connection: %s\r\n", connection);
    
    if (content_length >= 0) {
        printf("Content-Length: %ld\r\n", content_length);
    }
    
    if (content_type != NULL) {
        printf("Content-Type: %s\r\n", content_type);
    }
    
    printf("\r\n"); // Linha em branco que separa cabeçalho do corpo
}

// Função para processar requisição OPTIONS
int process_OPTIONS(char* connection) {
    send_simple_headers(HTTP_OK, connection, 0, "text/plain");
    return HTTP_OK;
}

// Função para processar requisição TRACE
int process_TRACE(char* connection) {
    send_simple_headers(HTTP_OK, connection, 0, "message/http");
    return HTTP_OK;
}

// Função para processar requisição GET
int process_GET(char* caminhos[], char* connection) {
    char home[4096];
    char target_inicial[4096];
    char target_completo[4096];
    int fd;
    char buf[4096];
    ssize_t bytes_read;
    
    strncpy(home, caminhos[1], 4095);
    home[4095] = '\0';
    strncpy(target_inicial, caminhos[2], 4095);
    target_inicial[4095] = '\0';
    
    snprintf(target_completo, sizeof(target_completo), "%s/%s", home, target_inicial);

    struct stat statbuf;

    if(stat(target_completo, &statbuf) == -1){
        char additional_info[256];
        snprintf(additional_info, sizeof(additional_info), "Arquivo: %s", target_completo);
        send_error_page(HTTP_NOT_FOUND, connection, additional_info);
        return HTTP_NOT_FOUND;
    }
    
    if(access(target_completo, R_OK) != 0){
        char additional_info[256];
        snprintf(additional_info, sizeof(additional_info), "Arquivo: %s (Sem permissão de leitura)", target_completo);
        send_error_page(HTTP_FORBIDDEN, connection, additional_info);
        return HTTP_FORBIDDEN;
    }

    switch (statbuf.st_mode & S_IFMT){
        case S_IFREG: // Arquivo regular
            send_headers(HTTP_OK, connection, &statbuf, target_completo, 0);
            
            if((fd = open(target_completo, O_RDONLY)) == -1) {
                char additional_info[256];
                snprintf(additional_info, sizeof(additional_info), "Arquivo: %s (Erro ao abrir: %s)", target_completo, strerror(errno));
                send_error_page(HTTP_FORBIDDEN, connection, additional_info);
                return HTTP_FORBIDDEN;
            }
            
            while ((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
                write(STDOUT_FILENO, buf, bytes_read);
            }
            close(fd);
            return HTTP_OK;

        case S_IFDIR: // Diretório
            if(access(target_completo, X_OK) != 0){
                char additional_info[256];
                snprintf(additional_info, sizeof(additional_info), "Diretório: %s (Sem permissão de execução)", target_completo);
                send_error_page(HTTP_FORBIDDEN, connection, additional_info);
                return HTTP_FORBIDDEN;
            }

            char target_welcome[256];
            char target_index[256];
            struct stat statbuf_index, statbuf_welcome;
            
            snprintf(target_welcome, sizeof(target_welcome), "%s/welcome.html", target_completo);
            snprintf(target_index, sizeof(target_index), "%s/index.html", target_completo);

            int index_encontrado = 0, welcome_encontrado = 0;
            int index_permitido = 0, welcome_permitido = 0;
            
            if(stat(target_index, &statbuf_index) != -1){
                index_encontrado = 1;
                if(access(target_index, R_OK) == 0)
                    index_permitido = 1;
            }
            if(stat(target_welcome, &statbuf_welcome) != -1){
                welcome_encontrado = 1;
                if(access(target_welcome, R_OK) == 0)
                    welcome_permitido = 1;
            }

            if(index_encontrado == 0 && welcome_encontrado == 0) {
                char additional_info[256];
                snprintf(additional_info, sizeof(additional_info), "Diretório: %s (index.html ou welcome.html não encontrados)", target_completo);
                send_error_page(HTTP_NOT_FOUND, connection, additional_info);
                return HTTP_NOT_FOUND;
            }
            
            if(index_permitido == 0 && welcome_permitido == 0) {
                char additional_info[256];
                snprintf(additional_info, sizeof(additional_info), "Diretório: %s (Sem permissão de leitura nos arquivos índice)", target_completo);
                send_error_page(HTTP_FORBIDDEN, connection, additional_info);
                return HTTP_FORBIDDEN;
            }
            
            // Preferência: index.html > welcome.html
            char* arquivo_escolhido = target_index;
            struct stat* statbuf_escolhido = &statbuf_index;
            
            if(!index_permitido || (welcome_permitido && !index_encontrado)) {
                arquivo_escolhido = target_welcome;
                statbuf_escolhido = &statbuf_welcome;
            }
            
            send_headers(HTTP_OK, connection, statbuf_escolhido, arquivo_escolhido, 0);
            
            if((fd = open(arquivo_escolhido, O_RDONLY)) == -1) {
                char additional_info[256];
                snprintf(additional_info, sizeof(additional_info), "Arquivo: %s (Erro ao abrir: %s)", arquivo_escolhido, strerror(errno));
                send_error_page(HTTP_FORBIDDEN, connection, additional_info);
                return HTTP_FORBIDDEN;
            }
            
            while ((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
                write(STDOUT_FILENO, buf, bytes_read);
            }
            close(fd);
            return HTTP_OK;
        
        default:
            char additional_info[256];
            snprintf(additional_info, sizeof(additional_info), "Recurso: %s (Tipo de arquivo não suportado)", target_completo);
            send_error_page(HTTP_FORBIDDEN, connection, additional_info);
            return HTTP_FORBIDDEN;
    }
}

// Função para processar requisição HEAD (igual ao GET, mas sem corpo)
int process_HEAD(char* caminhos[], char* connection) {
    char home[4096];
    char target_inicial[4096];
    char target_completo[4096];
    
    strncpy(home, caminhos[1], 4095);
    home[4095] = '\0';
    strncpy(target_inicial, caminhos[2], 4095);
    target_inicial[4095] = '\0';
    
    snprintf(target_completo, sizeof(target_completo), "%s/%s", home, target_inicial);

    struct stat statbuf;

    if(stat(target_completo, &statbuf) == -1){
        send_error_page(HTTP_NOT_FOUND, "HEAD", target_completo);
        return HTTP_NOT_FOUND;
    }
    
    if(access(target_completo, R_OK) != 0){
        send_error_page(HTTP_FORBIDDEN, "HEAD", target_completo);
        return HTTP_FORBIDDEN;
    }

    switch (statbuf.st_mode & S_IFMT){
        case S_IFREG: // Arquivo regular
            send_headers(HTTP_OK, connection, &statbuf, target_completo, 1);
            return HTTP_OK;

        case S_IFDIR: // Diretório
            if(access(target_completo, X_OK) != 0){
                send_error_page(HTTP_FORBIDDEN, "HEAD", target_completo);
                return HTTP_FORBIDDEN;
            }

            char target_welcome[256];
            char target_index[256];
            struct stat statbuf_index, statbuf_welcome;
            
            snprintf(target_welcome, sizeof(target_welcome), "%s/welcome.html", target_completo);
            snprintf(target_index, sizeof(target_index), "%s/index.html", target_completo);

            int index_encontrado = 0, welcome_encontrado = 0;
            int index_permitido = 0, welcome_permitido = 0;
            
            if(stat(target_index, &statbuf_index) != -1){
                index_encontrado = 1;
                if(access(target_index, R_OK) == 0)
                    index_permitido = 1;
            }
            if(stat(target_welcome, &statbuf_welcome) != -1){
                welcome_encontrado = 1;
                if(access(target_welcome, R_OK) == 0)
                    welcome_permitido = 1;
            }

            if(index_encontrado == 0 && welcome_encontrado == 0) {
                send_error_page(HTTP_NOT_FOUND, "HEAD", target_completo);
                return HTTP_NOT_FOUND;
            }
            
            if(index_permitido == 0 && welcome_permitido == 0) {
                send_error_page(HTTP_FORBIDDEN, "HEAD", target_completo);
                return HTTP_FORBIDDEN;
            }
            
            // Preferência: index.html > welcome.html
            char* arquivo_escolhido = target_index;
            struct stat* statbuf_escolhido = &statbuf_index;
            
            if(!index_permitido || (welcome_permitido && !index_encontrado)) {
                arquivo_escolhido = target_welcome;
                statbuf_escolhido = &statbuf_welcome;
            }
            
            send_headers(HTTP_OK, connection, statbuf_escolhido, arquivo_escolhido, 1);
            return HTTP_OK;
        
        default:
            send_error_page(HTTP_FORBIDDEN, "HEAD", target_completo);
            return HTTP_FORBIDDEN;
    }
}

// Função principal
int main(int argc, char *argv[]){
    if(argc == 4) {
        char* method = argv[3];
        char* connection = "close";
        
        if(strcmp(method, "GET") == 0) {
            process_GET(argv, connection);
        } else if(strcmp(method, "HEAD") == 0) {
            process_HEAD(argv, connection);
        } else if(strcmp(method, "OPTIONS") == 0) {
            process_OPTIONS(connection);
        } else if(strcmp(method, "TRACE") == 0) {
            process_TRACE(connection);
        } else {
            send_error_page(HTTP_METHOD_NOT_ALLOWED, connection, method);
        }
    } else {
        // Erro de uso - enviar página de erro HTML
        char usage_info[256];
        snprintf(usage_info, sizeof(usage_info), "Uso correto: %s [caminho da raiz] [recurso] [GET|HEAD|OPTIONS|TRACE]", argv[0]);
        send_error_page(HTTP_INTERNAL_ERROR, "close", usage_info);
        return 1;
    }
    
    return 0;
}