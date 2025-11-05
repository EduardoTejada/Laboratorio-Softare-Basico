/* Compile com: gcc -o cripto cripto.c -lcrypt */
#define _XOPEN_SOURCE
#include <crypt.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv){
    if (argc < 3){
        printf("Uso: cripto senha sal\nonde senha pode ser qualquer dado e sal tem que ser uma das\nseguintes opções:\n - dois caracteres quaisquer (muda se usar mais que dois?) ou\n - $5$sal_qualquer$ (muda se usar mais que dois caracteres?) ou\n - $6$sal_qualquer$ (muda se usar mais que dois caracteres?).\n (lembre de usar escape (\\) antes de $) \n\n");
        exit(0);
    }
    printf("\n( Sal = %s ) + ( Senha = %s ) ==> crypt() ==> %s \n\n",
    argv[2], argv[1], crypt(argv[1], argv[2]));
    return(0);
}