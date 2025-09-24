#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 7080

void processar_mensagem(char *entrada, char *saida) {
    sprintf(saida, "Servidor recebeu: %s", entrada);
}   

int main(){
    int soquete, soquete_msg, tamanhobuf = 1024;
    struct sockaddr_in meu_servidor, meu_cliente;
    int tam_endereco = sizeof(meu_cliente);
    char buf_entrada[tamanhobuf], buf_saida[tamanhobuf];
    
    soquete = socket(AF_INET, SOCK_STREAM, 0); // 0 indica “Use protocolo padrão”
    if(soquete < 0){
        perror("Erro ao criar socket");
        exit(1);
    }
    
    meu_servidor.sin_family = AF_INET;
    meu_servidor.sin_port = htons(PORT); // htons() converte para representação de rede
    meu_servidor.sin_addr.s_addr = INADDR_ANY; // qualquer endereço válido

    if (bind(soquete, (struct sockaddr*)&meu_servidor, sizeof(meu_servidor)) < 0) {
        perror("Erro no bind");
        exit(1);
    }
    
    // prepara socket e uma lista para receber até 5 conexões
    if (listen(soquete, 5) < 0) {
        perror("Erro no listen");
        exit(1);
    }
    
    while(1){
        soquete_msg = accept(soquete, (struct sockaddr*)&meu_cliente, &tam_endereco);
        if (soquete_msg < 0) {
            perror("Erro no accept");
            continue;
        }

        int n;
        do{
            memset(buf_entrada, 0, tamanhobuf);
            n = read (soquete_msg, buf_entrada, tamanhobuf);
            if(n > 0){
                buf_entrada[n] = '\0';
                processar_mensagem(buf_entrada, buf_saida);
                write (soquete_msg, buf_saida, strlen(buf_saida));
            }
        } while(n > 0);

        close(soquete_msg);
    }
    close(soquete);
    return 0;
}