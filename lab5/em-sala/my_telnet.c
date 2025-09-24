#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

int main(int argc, char *argv[]){
    int soquete, tamanho_buf=1024;
    struct sockaddr_in destino;
    char msg_ida[1024] = "GET /\r\n\r\n";
    char msg_volta[1024];
    
    if(argc != 3){
        fprintf(stderr, "Uso: %s [endereco IP] [porta]\n", argv[0]);
        exit(1);
    }

    char *endereco_ip = argv[1];
    int porta = atoi(argv[2]);
    
    soquete = socket(AF_INET, SOCK_STREAM, 0);
    if (soquete < 0) {
        perror("Erro ao criar socket");
        exit(1);
    }

    memset(&destino, 0, sizeof(destino));
    destino.sin_family = AF_INET;
    destino.sin_port = htons(porta);

    if (inet_aton(endereco_ip, &destino.sin_addr) == 0) {
        printf("Endereço IP inválido\n");
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

    FILE *saida = fopen("saida.html", "w");
    if (!saida) {
        perror("Erro ao criar arquivo");
        close(soquete);
        exit(1);
    }

    int n;
    do {
        memset(msg_volta, 0, tamanho_buf);
        n = read(soquete, msg_volta, tamanho_buf - 1);
        if (n > 0) {
            printf("%s", msg_volta); // imprime na tela
            fprintf(saida, "%s", msg_volta); // salva no arquivo
        }
    } while (n > 0);

    close(soquete);
    return 0;
}
