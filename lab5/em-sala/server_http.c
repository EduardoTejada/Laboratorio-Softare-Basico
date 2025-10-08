#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 8080

int main() {
    int soquete, soquete_msg;
    struct sockaddr_in meu_cliente;
    socklen_t tam_endereco = sizeof(meu_cliente);
    char buffer[2048];

    // cria socket TCP
    soquete = socket(AF_INET, SOCK_STREAM, 0);
    if (soquete < 0) {
        perror("Erro ao criar socket");
        exit(1);
    }

    // permite reuso rápido da porta
    int opt = 1;
    if (setsockopt(soquete, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Erro no setsockopt");
        exit(1);
    }

    // configura endereço/porta
    meu_cliente.sin_family = AF_INET;
    meu_cliente.sin_port = htons(PORT);
    meu_cliente.sin_addr.s_addr = INADDR_ANY; // qualquer interface

    if (bind(soquete, (struct sockaddr*)&meu_cliente, sizeof(meu_cliente)) < 0) {
        perror("Erro no bind");
        exit(1);
    }

    if (listen(soquete, 5) < 0) {
        perror("Erro no listen");
        exit(1);
    }

    while (1) {
        soquete_msg = accept(soquete, (struct sockaddr*)&meu_cliente, &tam_endereco);
        if (soquete_msg < 0) {
            perror("Erro no accept");
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        read(soquete_msg, buffer, sizeof(buffer) - 1);

        printf("%s\n", buffer);

        // resposta HTTP fixa
        const char *resposta =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<html><body><h1>Olá, mundo!</h1></body></html>";

        write(soquete_msg, resposta, strlen(resposta));
        close(soquete_msg);
    }

    close(soquete);
    return 0;
}