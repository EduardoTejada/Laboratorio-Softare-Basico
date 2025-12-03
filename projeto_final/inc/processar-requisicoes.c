#include "processar-requisicoes.h"
#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <crypt.h>
#include "base64.h"

typedef struct {
    char* nomeusuario;
    char* senha_antiga;
    char* senha_nova1;
    char* senha_nova2;
    char* botao_de_envio;
} DadosTrocaSenha;

void liberar_dados_troca_senha(DadosTrocaSenha* dados) {
    if (dados == NULL) return;
    free(dados->nomeusuario);
    free(dados->senha_antiga);
    free(dados->senha_nova1);
    free(dados->senha_nova2);
    free(dados->botao_de_envio);
    free(dados);
}

DadosTrocaSenha* parsear_dados_post(const char* corpo) {
    if (corpo == NULL || strlen(corpo) == 0) {
        fprintf(stderr, "DEBUG: Corpo POST vazio ou nulo\n");
        return NULL;
    }
    
    fprintf(stderr, "DEBUG: Parseando corpo POST: %s\n", corpo);
    
    DadosTrocaSenha* dados = calloc(1, sizeof(DadosTrocaSenha));
    if (dados == NULL) return NULL;
    
    char* copia = strdup(corpo);
    char* token = strtok(copia, "&");
    
    while (token != NULL) {
        fprintf(stderr, "DEBUG: Token: %s\n", token);
        
        char* separator = strchr(token, '=');
        if (separator != NULL) {
            *separator = '\0';
            char* chave = token;
            char* valor = separator + 1;
            
            // Decodificar URL encoding
            char* valor_decodificado = malloc(strlen(valor) + 1);
            url_decode(valor, valor_decodificado);
                        
            if (strcmp(chave, "nomeusuario") == 0) {
                dados->nomeusuario = strdup(valor_decodificado);
            } else if (strcmp(chave, "senha_antiga") == 0) {
                dados->senha_antiga = strdup(valor_decodificado);
            } else if (strcmp(chave, "senha_nova1") == 0) {
                dados->senha_nova1 = strdup(valor_decodificado);
            } else if (strcmp(chave, "senha_nova2") == 0) {
                dados->senha_nova2 = strdup(valor_decodificado);
            } else if (strcmp(chave, "botao_de_envio") == 0) {
                dados->botao_de_envio = strdup(valor_decodificado);
            }
            
            free(valor_decodificado);
        }
        token = strtok(NULL, "&");
    }
    
    free(copia);
    return dados;
}

int trocar_senha_htpasswd(const char* htpasswd_path, const char* usuario, const char* senha_antiga, const char* senha_nova) {
    FILE* arquivo = fopen(htpasswd_path, "r");
    if (arquivo == NULL) {
        fprintf(stderr, "DEBUG: Erro ao abrir arquivo para leitura: %s\n", htpasswd_path);
        return 0;
    }
    
    char linhas[100][256]; // Supondo máximo 100 linhas
    int num_linhas = 0;
    int usuario_encontrado = 0;
    
    while (fgets(linhas[num_linhas], sizeof(linhas[0]), arquivo) && num_linhas < 100) {
        linhas[num_linhas][strcspn(linhas[num_linhas], "\r\n")] = '\0';
        char copia[1024];
        strcpy(copia, linhas[num_linhas]);        
    
        char* separator = strchr(copia, ':');
        if (separator != NULL) {
            *separator = '\0';
            char* user_file = copia;
            char* pass_crypt_file = separator + 1;
            
            if (strcmp(user_file, usuario) == 0) {
                usuario_encontrado = 1;
                
                fprintf(stderr, "DEBUG: senha antiga %s\n", senha_antiga);
                fprintf(stderr, "DEBUG: pass_crypt_file %s\n", pass_crypt_file);

                // Verificar senha antiga
                char* senha_antiga_cripto = crypt(senha_antiga, pass_crypt_file);

                fprintf(stderr, "DEBUG: senha antiga crito %s\n", senha_antiga_cripto);

                if (senha_antiga_cripto == NULL || strcmp(senha_antiga_cripto, pass_crypt_file) != 0) {
                    fclose(arquivo);
                    fprintf(stderr, "DEBUG: Senha antiga (%s) não confere (%s)\n", senha_antiga_cripto, pass_crypt_file);
                    return 0;
                }
                
                // Gerar nova senha criptografada com o mesmo salt
                char* senha_nova_cripto = crypt(senha_nova, pass_crypt_file);
                if (senha_nova_cripto == NULL) {
                    fclose(arquivo);
                    fprintf(stderr, "DEBUG: Erro ao criptografar nova senha\n");
                    return 0;
                }
                
                // Atualizar a linha
                snprintf(linhas[num_linhas], sizeof(linhas[0]), "%s:%s", usuario, senha_nova_cripto);
                fprintf(stderr, "DEBUG: Nova linha: %s\n", linhas[num_linhas]);
            }
        }
        num_linhas++;
    }
    fclose(arquivo);
    
    if (!usuario_encontrado) {
        fprintf(stderr, "DEBUG: Usuário não encontrado no arquivo\n");
        return 0;
    }
    
    // Reescrever o arquivo
    arquivo = fopen(htpasswd_path, "w");
    if (arquivo == NULL) {
        fprintf(stderr, "DEBUG: Erro ao abrir arquivo para escrita: %s\n", htpasswd_path);
        return 0;
    }
    
    for (int i = 0; i < num_linhas; i++) {
        fprintf(arquivo, "%s\n", linhas[i]);
    }
    fclose(arquivo);
    
    fprintf(stderr, "DEBUG: Senha alterada com sucesso\n");
    return 1;
}


int process_POST(char* caminhos[], char* connection, CampoNode* lista_campos, const char* corpo_post) {
    fprintf(stderr, "DEBUG: Processando requisição POST\n");
    
    char home[4096];
    char target_inicial[4096];
    char target_completo[4096];
    
    strncpy(home, caminhos[1], 4095);
    home[4095] = '\0';
    strncpy(target_inicial, caminhos[2], 4095);
    target_inicial[4095] = '\0';
    
    // Normalizar target
    if (target_inicial[0] == '/') {
        memmove(target_inicial, target_inicial + 1, strlen(target_inicial));
    }

    snprintf(target_completo, sizeof(target_completo), "%s/%s", home, target_inicial);
    
    // Remover barras duplas
    char* double_slash;
    while ((double_slash = strstr(target_completo, "//")) != NULL) {
        memmove(double_slash, double_slash + 1, strlen(double_slash));
    }

    fprintf(stderr, "DEBUG: POST para: %s\n", target_completo);
    
    // Verificar se é o formulário de troca de senha
    if (strstr(target_completo, "nova_senha.html") != NULL) {
        fprintf(stderr, "DEBUG: Detectado formulário de troca de senha\n");
        return processar_troca_senha(target_completo, caminhos, connection, lista_campos, corpo_post);
    }
    
    // Outros tipos de POST podem ser implementados aqui
    send_error_page(HTTP_NOT_IMPLEMENTED, connection, "Funcionalidade POST não implementada");
    return HTTP_NOT_IMPLEMENTED;
}


void enviar_pagina_sucesso_troca_senha(char* connection) {
    char* html = 
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>Senha Alterada</title>\n"
        "</head>\n"
        "<body>\n"
        "    <h1>Senha Alterada com Sucesso!</h1>\n"
        "    <p>Sua senha foi alterada com sucesso.</p>\n"
        "    <p><a href=\"/\">Voltar à página inicial</a></p>\n"
        "</body>\n"
        "</html>";
    
    printf("HTTP/1.1 200 OK\r\n");
    printf("Date: %s\r\n", get_http_date());
    printf("Server: Servidor HTTP ver. 0.1 de Eduardo\r\n");
    printf("Connection: %s\r\n", connection);
    printf("Content-Type: text/html; charset=utf-8\r\n");
    printf("Content-Length: %ld\r\n", strlen(html));
    printf("\r\n");
    printf("%s", html);
}

void enviar_pagina_erro_troca_senha(char* mensagem, char* connection) {
    char html[1024];
    snprintf(html, sizeof(html),
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>Erro na Troca de Senha</title>\n"
        "</head>\n"
        "<body>\n"
        "    <h1>Erro na Troca de Senha</h1>\n"
        "    <p>%s</p>\n"
        "    <p><a href=\"javascript:history.back()\">Tentar novamente</a></p>\n"
        "</body>\n"
        "</html>", mensagem);
    
    printf("HTTP/1.1 200 OK\r\n");
    printf("Date: %s\r\n", get_http_date());
    printf("Server: Servidor HTTP ver. 0.1 de Eduardo\r\n");
    printf("Connection: %s\r\n", connection);
    printf("Content-Type: text/html; charset=utf-8\r\n");
    printf("Content-Length: %ld\r\n", strlen(html));
    printf("\r\n");
    printf("%s", html);
}


int processar_troca_senha(char* target_completo, char* caminhos[], char* connection, CampoNode* lista_campos, const char* corpo_post) {
    fprintf(stderr, "DEBUG: Processando troca de senha\n");
    
    char home[4096];
    strncpy(home, caminhos[1], 4095);
    home[4095] = '\0';
    
    char* realm = NULL;
    char* htpasswd_path = NULL;
    
    fprintf(stderr, "DEBUG: Arquivo troca de senha %s\n", caminhos[2]);
    
    int protegido = verificar_protecao(target_completo, home, &realm, &htpasswd_path);
    
    if (!protegido) {
        fprintf(stderr, "DEBUG: Recurso não protegido - não pode trocar senha\n");
        send_error_page(HTTP_FORBIDDEN, connection, "Recurso não protegido");
        return HTTP_FORBIDDEN;
    }
    
    fprintf(stderr, "DEBUG: Usando arquivo de senhas: %s\n", htpasswd_path);

    // Parsear dados do formulário
    DadosTrocaSenha* dados = parsear_dados_post(corpo_post);
    if (dados == NULL) {
        fprintf(stderr, "DEBUG: Erro ao parsear dados do formulário\n");
        send_error_page(HTTP_BAD_REQUEST, connection, "Dados do formulário inválidos");
        free(realm);
        free(htpasswd_path);
        return HTTP_BAD_REQUEST;
    }
    
    // Validar dados
    if (dados->nomeusuario == NULL || dados->senha_antiga == NULL || 
        dados->senha_nova1 == NULL || dados->senha_nova2 == NULL) {
        fprintf(stderr, "DEBUG: Campos obrigatórios faltando\n");
        enviar_pagina_erro_troca_senha("Campos obrigatórios não preenchidos", connection);
        liberar_dados_troca_senha(dados);
        free(realm);
        free(htpasswd_path);
        return HTTP_BAD_REQUEST;
    }
    
    fprintf(stderr, "DEBUG: Dados recebidos - Usuário: %s, Senha antiga: %s, Nova1: %s, Nova2: %s\n",
           dados->nomeusuario, dados->senha_antiga, dados->senha_nova1, dados->senha_nova2);
    
    // Verificar se as novas senhas coincidem
    if (strcmp(dados->senha_nova1, dados->senha_nova2) != 0) {
        fprintf(stderr, "DEBUG: Novas senhas não coincidem (%s) (%s)\n", dados->senha_nova1, dados->senha_nova2);
        enviar_pagina_erro_troca_senha("As novas senhas não coincidem", connection);
        liberar_dados_troca_senha(dados);
        free(realm);
        free(htpasswd_path);
        return HTTP_BAD_REQUEST;
    }
    
    // Verificar se o arquivo de senhas existe e é gravável
    if (access(htpasswd_path, F_OK) != 0) {
        fprintf(stderr, "DEBUG: Arquivo de senhas não existe: %s\n", htpasswd_path);
        enviar_pagina_erro_troca_senha("Arquivo de senhas não encontrado", connection);
        liberar_dados_troca_senha(dados);
        free(realm);
        free(htpasswd_path);
        return HTTP_INTERNAL_ERROR;
    }
    
    if (access(htpasswd_path, W_OK) != 0) {
        fprintf(stderr, "DEBUG: Sem permissão para escrever no arquivo: %s\n", htpasswd_path);
        enviar_pagina_erro_troca_senha("Sem permissão para alterar senhas", connection);
        liberar_dados_troca_senha(dados);
        free(realm);
        free(htpasswd_path);
        return HTTP_FORBIDDEN;
    }
    
    // Trocar a senha
    if (trocar_senha_htpasswd(htpasswd_path, dados->nomeusuario, dados->senha_antiga, dados->senha_nova1)) {
        fprintf(stderr, "DEBUG: Senha alterada com sucesso\n");
        enviar_pagina_sucesso_troca_senha(connection);
    } else {
        fprintf(stderr, "DEBUG: Falha na troca de senha\n");
        enviar_pagina_erro_troca_senha("Falha na autenticação ou usuário não encontrado", connection);
    }
    
    liberar_dados_troca_senha(dados);
    free(realm);
    free(htpasswd_path);
    return HTTP_OK;
}

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
    else if (strcmp(ext, ".tif") == 0)
        return "image/tif";
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

// Verifica a existência e retorna o caminho de um arquivo .htaccess dado um diretório específico
int verificar_htaccess(const char* diretorio, char** realm, char** htpasswd_path) {
    char htaccess_path[4096];
    snprintf(htaccess_path, sizeof(htaccess_path), "%s/.htaccess", diretorio);
    
    FILE* htaccess_file = fopen(htaccess_path, "r");
    if (htaccess_file == NULL) {
        return 0; // .htaccess não existe
    }
    
    // Ler o conteúdo do .htaccess (caminho para o arquivo de senhas)
    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), htaccess_file) != NULL) {
        // Remover quebra de linha
        buffer[strcspn(buffer, "\r\n")] = '\0';
        
        char* separator = strchr(buffer, ':');
        if (separator != NULL) {
            *separator = '\0';
            *realm = strdup(buffer);
            
            // Montando caminho completo
            char* htpasswd_relative = separator + 1;
            char resolved_path[4096];
            
            snprintf(resolved_path, sizeof(resolved_path), "%s/%s", diretorio, htpasswd_relative);
            *htpasswd_path = strdup(resolved_path);
            
            fprintf(stderr, "DEBUG: realm extraído='%s', htpasswd_path resolvido='%s'\n", *realm, *htpasswd_path);
        } else {
            // Formato simples: apenas o caminho para htpasswd
            *realm = strdup("realm");
            *htpasswd_path = strdup(buffer);

            // Montando caminho completo
            if (buffer[0] == '/') {
                // Caminho absoluto
                *htpasswd_path = strdup(buffer);
            } else {
                // Caminho relativo
                char resolved_path[4096];
                snprintf(resolved_path, sizeof(resolved_path), "%s/%s", diretorio, buffer);
                *htpasswd_path = strdup(resolved_path);
            }
            
            fprintf(stderr, "DEBUG: Formato simples - realm='%s', htpasswd_path='%s'\n", *realm, *htpasswd_path);

            
        }
    } else {
        fprintf(stderr, "DEBUG: .htaccess vazio ou erro de leitura\n");
        fclose(htaccess_file);
        return 0;
    
    fclose(htaccess_file);
    return 1; // .htaccess encontrado
    }
}

int buscar_htaccess_recursivo(const char* caminho_completo, const char* webspace, char** realm, char** htpasswd_path) {
    //fprintf(stderr, "DEBUG: caminho completo %s\n", caminho_completo);
    //fprintf(stderr, "DEBUG: webspace %s\n", webspace);
    
    // monta o caminho completo para oo recurso
    char caminho[4096];
    strncpy(caminho, caminho_completo, sizeof(caminho));
    caminho[sizeof(caminho)-1] = '\0';
    //fprintf(stderr, "DEBUG: caminho %s\n", caminho);
    
    // Deixar somente o caminho para o diretório
    char* ultima_barra = strrchr(caminho, '/');
    if (ultima_barra != NULL) {
        *ultima_barra = '\0';
    }

    // Remove a barra no final caso houver
    ultima_barra = strrchr(webspace, '/');
    if (ultima_barra != NULL) {
        *ultima_barra = '\0';
    }

    //fprintf(stderr, "DEBUG: caminho %s\n", caminho);
    
    // Fora do webspace
    if (strlen(caminho) < strlen(webspace)) {
        return 0;
    }

    // Procurar .htaccess no diretório atual
    if (verificar_htaccess(caminho, realm, htpasswd_path)) {
        return 1;
    }

    // Se chegamos à raiz do webspace, parar a busca
    if (strcmp(caminho, webspace) == 0) {
        return 0; // Nenhum .htaccess encontrado
    }
    
    // Subir um nível no diretório
    char* barra_anterior = strrchr(caminho, '/');
    if (barra_anterior != NULL) {
        *barra_anterior = '\0';
        
        // Continuar busca recursivamente
        return buscar_htaccess_recursivo(caminho, webspace, realm, htpasswd_path);
    }
    
    return 0; // Nenhum .htaccess encontrado
}

// Verifica se um recurso está ou não protegido
int verificar_protecao(const char* caminho_completo, const char* webspace, char** realm, char** htpasswd_path) {
    
    *realm = NULL;
    *htpasswd_path = NULL;
    
    return buscar_htaccess_recursivo(caminho_completo, webspace, realm, htpasswd_path);
}

void send_autentication_page(char* realm, char* connection) {
    char html_content[1024];
    snprintf(html_content, sizeof(html_content),
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>401 Authorization Required</title>\n"
        "</head>\n"
        "<body>\n"
        "    <h1>401 Authorization Required</h1>\n"
        "    <p><em>Servidor HTTP ver. 0.1 de Eduardo</em></p>\n"
        "</body>\n"
        "</html>");
    
    int content_length = strlen(html_content);
    
    printf("HTTP/1.1 401 Authorization Required\r\n");
    printf("Date: %s\r\n", get_http_date());
    printf("Server: Servidor HTTP ver. 0.1 de Eduardo\r\n");
    printf("WWW-Authenticate: Basic realm=\"%s\"\r\n", realm);
    printf("Connection: %s\r\n", connection);
    printf("Content-Type: text/html; charset=utf-8\r\n");
    printf("Content-Length: %d\r\n", content_length);
    printf("\r\n");
    
    if (strcmp(connection, "HEAD") != 0) {
        printf("%s", html_content);
    }
    
    fflush(stdout);
}

// Extrai os dados (usuário e senha) preenchidos com Basic Authentication da lista ligada do parser
int extrair_credenciais_da_lista(CampoNode* lista_campos, char** usuario, char** senha) {
    CampoNode* campo = lista_campos;
    
    while (campo != NULL) {
        fprintf(stderr, "DEBUG: Verificando campo: %s\n", campo->nome);
        
        if (strcasecmp(campo->nome, "Authorization") == 0) {
            // Encontramos o header Authorization
            ParametroNode* param = campo->parametros;
            if (param != NULL && param->valor != NULL) {
                fprintf(stderr, "DEBUG: Valor do Authorization: %s\n", param->valor);
                
                // Verificar se é Basic Authentication
                if (strncasecmp(param->valor, "Basic ", 6) == 0) {
                    char* base64_str = param->valor + 6; // Pular "Basic "
                    
                    fprintf(stderr, "DEBUG: String base64: %s\n", base64_str);
                    
                    // Decodificar base64
                    char* decoded = malloc(strlen(base64_str) * 3 / 4 + 1);
                    if (base64_decode(base64_str, strlen(base64_str), decoded) != 0) {
                        fprintf(stderr, "DEBUG: Erro na decodificação base64\n");
                        free(decoded);
                        return 0;
                    }
                    
                    fprintf(stderr, "DEBUG: String decodificada: %s\n", decoded);
                    
                    // Separar usuário e senha (formato: usuario:senha)
                    char* separator = strchr(decoded, ':');
                    if (separator == NULL) {
                        fprintf(stderr, "DEBUG: Formato inválido - separador ':' não encontrado\n");
                        free(decoded);
                        return 0;
                    }
                    
                    *separator = '\0';
                    *usuario = strdup(decoded);
                    *senha = strdup(separator + 1);
                    
                    fprintf(stderr, "DEBUG: Usuário: %s, Senha: %s\n", *usuario, *senha);
                    
                    free(decoded);
                    return 1;
                }
            }
        }
        campo = campo->proximo;
    }
    
    fprintf(stderr, "DEBUG: Header Authorization não encontrado na lista\n");
    return 0;
}

int verificar_credenciais(const char* usuario, const char* senha, const char* htpasswd_path) {
    fprintf(stderr, "DEBUG verificar_credenciais: Iniciando verificação\n");
    fprintf(stderr, "DEBUG: Usuário recebido: '%s'\n", usuario);
    fprintf(stderr, "DEBUG: Senha recebida: '%s'\n", senha);
    fprintf(stderr, "DEBUG: Caminho htpasswd: '%s'\n", htpasswd_path);
    
    // Verificar se o arquivo existe e é legível
    if (access(htpasswd_path, F_OK) != 0) {
        fprintf(stderr, "DEBUG: Arquivo htpasswd não existe: %s\n", htpasswd_path);
        return 0;
    }
    if (access(htpasswd_path, R_OK) != 0) {
        fprintf(stderr, "DEBUG: Sem permissão de leitura no arquivo: %s\n", htpasswd_path);
        return 0;
    }
    
    FILE* htpasswd_file = fopen(htpasswd_path, "r");
    if (htpasswd_file == NULL) {
        fprintf(stderr, "DEBUG: Erro ao abrir arquivo htpasswd: %s (errno: %d - %s)\n", 
                htpasswd_path, errno, strerror(errno));
        return 0;
    }
    
    fprintf(stderr, "DEBUG: Arquivo htpasswd aberto com sucesso\n");
    
    char linha[256];
    
    // Itera pelas linhas do .htpasswd
    while (fgets(linha, sizeof(linha), htpasswd_file)) {
        // Remover quebra de linha
        linha[strcspn(linha, "\r\n")] = '\0';
        
        // Separar usuário e senha criptografada
        char* separator = strchr(linha, ':');
        if (separator == NULL) continue;
        
        *separator = '\0';
        char* user_file = linha;                // nome de usuário da linha atual
        char* pass_crypt_file = separator + 1;  // senha criptografada
        
        fprintf(stderr, "DEBUG: Verificando usuário: %s\n", user_file);
        
        // Verifica encontrou o usuario
        if (strcmp(user_file, usuario) == 0) {
            // Criptografar a senha recebida com o mesmo salt do arquivo
            char* senha_criptografada = crypt(senha, pass_crypt_file);
            
            fprintf(stderr, "DEBUG: Senha criptografada: %s\n", senha_criptografada);
            fprintf(stderr, "DEBUG: Senha no arquivo: %s\n", pass_crypt_file);
            
            // Verifica se as senhas coincidem
            if (senha_criptografada != NULL && strcmp(senha_criptografada, pass_crypt_file) == 0) {
                fclose(htpasswd_file);
                return 1;
            } else {
                fclose(htpasswd_file);
                return 0;
            }
        }
    }
    
    fclose(htpasswd_file);
    return 0; // Usuário não encontrado
}


// Função para processar requisição GET
int process_GET(char* caminhos[], char* connection, CampoNode* lista_campos){
    char home[4096];
    char target_inicial[4096];
    char target_completo[4096];
    int fd;
    char buf[4096];
    ssize_t bytes_read;
    
    // Monta o caminho completo para o recurso: webspace + path
    strncpy(home, caminhos[1], 4095);
    home[4095] = '\0';
    strncpy(target_inicial, caminhos[2], 4095);
    target_inicial[4095] = '\0';
    
    snprintf(target_completo, sizeof(target_completo), "%s/%s", home, target_inicial);

    char* realm = NULL;
    char* htpasswd_path = NULL;
    
    int protegido = verificar_protecao(target_completo, home, &realm, &htpasswd_path);
    if(protegido){
        char* usuario = NULL;
        char* senha = NULL;

        // Procura o campo de credenciais na lista liaga do parser
        if (extrair_credenciais_da_lista(lista_campos, &usuario, &senha)) {
            fprintf(stderr, "DEBUG: Credenciais encontradas na lista - Verificando...\n");
            
            // Se achou verifica se o usuário existe e se a senha está correta
            if (verificar_credenciais(usuario, senha, htpasswd_path)) {
                fprintf(stderr, "DEBUG: Autenticação BEM-SUCEDIDA para usuário: %s\n", usuario);
                // Autenticação bem-sucedida - continuar com o processamento normal
                free(usuario);
                free(senha);
            } else {
                fprintf(stderr, "DEBUG: Autenticação FALHOU para usuário: %s\n", usuario);
                // Credenciais inválidas
                send_autentication_page(realm, connection);
                free(usuario);
                free(senha);
                free(realm);
                free(htpasswd_path);
                return 401;
            }
        } else {
            fprintf(stderr, "DEBUG: Nenhuma credencial encontrada na lista - solicitando autenticação\n");
            // Nenhuma credencial enviada - solicitar autenticação
            send_autentication_page(realm, connection);
            free(realm);
            free(htpasswd_path);
            return 401;
        }
    } else {
        fprintf(stderr, "DEBUG: Recurso NÃO protegido por .htaccess\n");
    }

    if (protegido) {
        free(realm);
        free(htpasswd_path);
    }

    struct stat statbuf;

    // verifica se o arquivo existe
    if(stat(target_completo, &statbuf) == -1){
        char additional_info[256];
        snprintf(additional_info, sizeof(additional_info), "Arquivo: %s", target_completo);
        send_error_page(HTTP_NOT_FOUND, connection, additional_info);
        return HTTP_NOT_FOUND;
    }
    
    // verifica o acesso ao arquivo
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
            
            // Escreve a resposta até que tenha enviado tudo
            while ((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
                write(STDOUT_FILENO, buf, bytes_read);
            }
            close(fd);
            return HTTP_OK;

        case S_IFDIR: // Diretório
            // Verifica de execução
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
            
            // verifica se existe um index.html
            if(stat(target_index, &statbuf_index) != -1){
                index_encontrado = 1;
                if(access(target_index, R_OK) == 0)
                    index_permitido = 1;
            }
            // verifica se existe um welcome.html
            if(stat(target_welcome, &statbuf_welcome) != -1){
                welcome_encontrado = 1;
                if(access(target_welcome, R_OK) == 0)
                    welcome_permitido = 1;
            }

            // se não existir nenhum dos dois retorna erro 404
            if(index_encontrado == 0 && welcome_encontrado == 0) {
                char additional_info[256];
                snprintf(additional_info, sizeof(additional_info), "Diretório: %s (index.html ou welcome.html não encontrados)", target_completo);
                send_error_page(HTTP_NOT_FOUND, connection, additional_info);
                return HTTP_NOT_FOUND;
            }
            
            // se existir algum mas não houver permissão retorna erro 403
            if(index_permitido == 0 && welcome_permitido == 0) {
                char additional_info[256];
                snprintf(additional_info, sizeof(additional_info), "Diretório: %s (Sem permissão de leitura nos arquivos índice)", target_completo);
                send_error_page(HTTP_FORBIDDEN, connection, additional_info);
                return HTTP_FORBIDDEN;
            }
            
            // declaração da preferência index.html > welcome.html
            char* arquivo_escolhido = target_index;
            struct stat* statbuf_escolhido = &statbuf_index;
            
            if(!index_permitido || (welcome_permitido && !index_encontrado)) {
                arquivo_escolhido = target_welcome;
                statbuf_escolhido = &statbuf_welcome;
            }
            
            // envia o cabeçalho da resposta
            send_headers(HTTP_OK, connection, statbuf_escolhido, arquivo_escolhido, 0);
            
            if((fd = open(arquivo_escolhido, O_RDONLY)) == -1) {
                char additional_info[256];
                snprintf(additional_info, sizeof(additional_info), "Arquivo: %s (Erro ao abrir: %s)", arquivo_escolhido, strerror(errno));
                send_error_page(HTTP_FORBIDDEN, connection, additional_info);
                return HTTP_FORBIDDEN;
            }
            
            // envia o corpo da resposta no loop até que tenha enviado tudo
            while ((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
                write(STDOUT_FILENO, buf, bytes_read);
            }
            close(fd);
            return HTTP_OK;
        
        default: // nem arquivo regular nem diretório
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
    
    // Monta o caminho completo para o recurso: webspace + path
    strncpy(home, caminhos[1], 4095);
    home[4095] = '\0';
    strncpy(target_inicial, caminhos[2], 4095);
    target_inicial[4095] = '\0';
    
    if (strcmp(target_inicial, "") == 0 || strcmp(target_inicial, "/") == 0) {
        strcpy(target_inicial, "index.html");
    }
    else if (target_inicial[0] == '/') {
        memmove(target_inicial, target_inicial + 1, strlen(target_inicial));
    }
    
    int is_directory = 0;
    if (target_inicial[strlen(target_inicial)-1] == '/') {
        is_directory = 1;
        target_inicial[strlen(target_inicial)-1] = '\0'; // Remover barra final
    }
    
    snprintf(target_completo, sizeof(target_completo), "%s/%s", home, target_inicial);
    
    printf("DEBUG process_HEAD: Home='%s', Target='%s', Completo='%s', IsDir=%d\n", 
           home, target_inicial, target_completo, is_directory);

    struct stat statbuf;

    if(stat(target_completo, &statbuf) == -1){
        if (is_directory) {
            char target_index[4096];
            snprintf(target_index, sizeof(target_index), "%s/index.html", target_completo);
            if(stat(target_index, &statbuf) != -1){
                // Encontrou index.html no diretório
                strcpy(target_completo, target_index);
            } else {
                char target_welcome[4096];
                snprintf(target_welcome, sizeof(target_welcome), "%s/welcome.html", target_completo);
                if(stat(target_welcome, &statbuf) != -1){
                    // Encontrou welcome.html no diretório
                    strcpy(target_completo, target_welcome);
                } else {
                    send_error_page(HTTP_NOT_FOUND, connection, target_completo);
                    return HTTP_NOT_FOUND;
                }
            }
        } else {
            send_error_page(HTTP_NOT_FOUND, connection, target_completo);
            return HTTP_NOT_FOUND;
        }
    }
    
    if(access(target_completo, R_OK) != 0){
        send_error_page(HTTP_FORBIDDEN, connection, target_completo);
        return HTTP_FORBIDDEN;
    }

    if (is_directory && S_ISREG(statbuf.st_mode)) {
        // Já encontramos um arquivo regular, usar ele
    } else if (S_ISDIR(statbuf.st_mode)) {
        // É um diretório, procurar index.html ou welcome.html
        char target_index[4096];
        char target_welcome[4096];
        
        snprintf(target_index, sizeof(target_index), "%s/index.html", target_completo);
        snprintf(target_welcome, sizeof(target_welcome), "%s/welcome.html", target_completo);
        
        struct stat statbuf_index, statbuf_welcome;
        int index_encontrado = 0, welcome_encontrado = 0;
        
        if(stat(target_index, &statbuf_index) != -1 && access(target_index, R_OK) == 0){
            strcpy(target_completo, target_index);
            statbuf = statbuf_index;
        } else if(stat(target_welcome, &statbuf_welcome) != -1 && access(target_welcome, R_OK) == 0){
            strcpy(target_completo, target_welcome);
            statbuf = statbuf_welcome;
        } else {
            send_error_page(HTTP_NOT_FOUND, connection, target_completo);
            return HTTP_NOT_FOUND;
        }
    }

    send_headers(HTTP_OK, connection, &statbuf, target_completo, 1);
    return HTTP_OK;
}