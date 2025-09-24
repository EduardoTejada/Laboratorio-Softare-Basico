/* Programa teste_select.c */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int main(void) {
  int n, c;
  //long int tolerancia = 5;
  struct timeval timeout;
  fd_set fds;
  int fd;
  
  fd = 0;              /* O que faz esta linha e as demais? Comente cada uma. */
  FD_ZERO(&fds);       /* Iniciliza/limpa o conjunto de descritores de arquivos */
  FD_SET(fd, &fds);    /* Adciona o descritor de arquivo 0 ao conjunto fds, fazendo com que o programa monitore a atividade desse descritor (0 é a entrada padrão) */
  timeout.tv_sec = 5;  /* Define que o tempo que o select ira esperar é de até 5 segundos */
  timeout.tv_usec = 0; /* Define como zero os microssegundos que adicionais ao timeout já definido em segundos */
  printf("Tempo atual: %ld\n", timeout.tv_sec);
  n = select(1, &fds, (fd_set *)0, (fd_set *)0, &timeout);
  /* 
   * A linha acima checa se o arquivo especificado em fds foi modificado para leitura, escrita, condicoes, excecoes
   * Portanto a funcao select espera um tempo especificado pelo parametro timeout, 1 eh o numero maximo de descritores
   * para monitorar, fds eh o conjunto de descritores para leitura, os outros dois parametros apos sao conjuntos de 
   * descritores para monitorar a escrita e as excecoes, que estao vazios pois nesse caso so iremos monitorar leitura
   * Retorna numero de descritores prontos, 0 se timeout, -1 se erro
   */

  if(n > 0 && FD_ISSET(fd, &fds))   /* Verifica se select retornou descritor pronto e se o descritor 0 está no conjunto de descritores prontos */
    {
      c = getchar();  /* Le o caractere digitado pelo usuario diferente do getchar() normal, a execução nao sera bloqueada pois ja ha um caractere esperando*/
      printf("Caractere %c teclado.\n", c);
    }
  else if(n == 0)    /* Select retornou 0 indicando que o timeout expirou sem que nenhuma entrada fosse detectada */
        {
        printf("Nada foi teclado em %ld s.\n", timeout.tv_sec);
        }
       else perror("Erro em select():");   /* Select retornou -1, indicando um erro, exibe a mensagem de erro correspondente */
  exit(1);   /* Termina o programa com codigo de saida 1 para sucesso */
}

