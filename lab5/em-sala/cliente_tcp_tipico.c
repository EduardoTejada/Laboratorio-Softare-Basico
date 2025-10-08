#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 7080

int main() {
    int soquete, tamanho_buf=1024;
    struct sockaddr_in destino;
    char msg_ida[1024] = "mensagem a enviar\n";
    char msg_volta[1024];

    soquete = socket(AF_INET, SOCK_STREAM, 0);
    if (soquete < 0) {
        perror("Erro ao criar socket");
        exit(1);
    }

    memset(&destino, 0, sizeof(destino));
    destino.sin_family = AF_INET;
    destino.sin_port = htons(SERVER_PORT);

    if (inet_aton(SERVER_IP, &destino.sin_addr) == 0) {
        fprintf(stderr, "Endereço IP inválido\n");
        exit(1);
    }
    if (connect(soquete, (struct sockaddr *)&destino, sizeof(destino)) < 0) {
        perror("Erro no connect");
        exit(1);
    }
    if (write(soquete, msg_ida, strlen(msg_ida)) < 0) {
        perror("Erro no write");
        exit(1);
    }

    int n;
    do {
        memset(msg_volta, 0, tamanho_buf);
        n = read(soquete, msg_volta, tamanho_buf - 1);
        if (n > 0) {
            msg_volta[n] = '\0';
            printf("Resposta do servidor: %s\n", msg_volta);
        }
    } while (n > 0);

    close(soquete);
    return 0;
}
