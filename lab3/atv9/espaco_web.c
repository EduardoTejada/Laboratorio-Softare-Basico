#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

#define HTTP_OK 200
#define HTTP_NOT_FOUND 404
#define HTTP_FORBIDDEN 403


#ifndef S_IFDIR
#define S_IFDIR 1
#endif

#ifndef S_IFMT
#define S_IFMT 2
#endif

#ifndef S_IFREG
#define S_IFREG 3
#endif


int searchOnWeb(char* caminhos[]){
    char home[4096];
    strcpy(home, caminhos[1]);
    strncpy(home, caminhos[1], 4095);
    home[4095] = '\0';
    char target_inicial[4096];
    strncpy(target_inicial, caminhos[2], 4095);
    target_inicial[4095] = '\0';
    char target_completo[4096];
    int fd;
    char buf[4096];
    ssize_t bytes_read;
    
    snprintf(target_completo, sizeof(target_completo), "%s/%s", home, target_inicial);

    struct stat statbuf;  /* definicao da estrutura do Inode */

    if(stat(target_completo, &statbuf) == -1){
        return HTTP_NOT_FOUND;
    }
    if(access(target_completo, R_OK) != 0){
        return HTTP_FORBIDDEN;
    }


    switch (statbuf.st_mode & S_IFMT){
        case S_IFREG : // Arquivo regular
            if((fd = open(target_completo, O_RDONLY, 0600)) == -1)
                return HTTP_FORBIDDEN;
            while ((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
                write(STDOUT_FILENO, buf, bytes_read);
            }
            close(fd);
            return HTTP_OK;
            break;

        case S_IFDIR : // Arquivo diretório
            if(access(target_completo, X_OK) != 0){
                return HTTP_FORBIDDEN;
            }

            char target_welcome[256];
            char target_index[256];
            strcpy(target_welcome, target_completo);
            strcpy(target_index, target_completo);
            snprintf(target_welcome, sizeof(target_welcome), "%s/welcome.html", target_completo);
            snprintf(target_index, sizeof(target_index), "%s/index.html", target_completo);

            int index_encontrado = 0, welcome_encontrado = 0, index_permitido = 0, welcome_permitido = 0;
            if(stat(target_index, &statbuf) != -1){
                index_encontrado = 1;
                if(access(target_index, R_OK) == 0)
                    index_permitido = 1;
            }
            if(stat(target_welcome, &statbuf) != -1){
                welcome_encontrado = 1;
                if(access(target_welcome, R_OK) == 0)
                    welcome_permitido = 1;
            }

            if(index_encontrado == 0 && welcome_encontrado == 0) return HTTP_NOT_FOUND;
            if(index_permitido == 0 && welcome_permitido == 0) return HTTP_FORBIDDEN;
            
            if(welcome_permitido == 1 && index_permitido == 0) strcpy(target_index, target_welcome);
            
            if((fd = open(target_index, O_RDONLY, 0600)) == -1)
                return HTTP_FORBIDDEN;
            
            while ((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
                write(STDOUT_FILENO, buf, bytes_read);
            }
            close(fd);
            return HTTP_OK;
        
        default:
            return HTTP_FORBIDDEN;
    }
}

int main(int argc, char *argv[]){
    if(argc == 3){
        switch(searchOnWeb(argv)){
            case HTTP_FORBIDDEN:
                printf("Sem permissao para acessar o arquivo.\n");
                break;
            case HTTP_NOT_FOUND:
                printf("Arquivo nao encontrado.\n");
                break;
            case HTTP_OK:
                printf("===========ARQUIVO ENCONTRADO===========\n");
                break;
        }
    } else
        printf("Erro!\nUso: [caminho da raiz] [recurso desejado com seu caminho]");
    return 0;
}
