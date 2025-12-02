CC = gcc
CFLAGS = -Wall -g
LEX = flex
YACC = bison
YFLAGS = -d

OBJS = atv3_sin.tab.o lex.yy.o myList.o myString.o processar-requisicoes.o base64.o

all: servidor_multi_thread

# Servidor Multithread
servidor_multi_thread: servidor_multi_thread.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ servidor_multi_thread.o $(OBJS) -lpthread -lcrypt
	rm -f servidor_multi_thread.o

servidor_multi_thread.o: servidor_multi_thread.c atv3_sin.tab.h inc/myList.h inc/myString.h inc/base64.h
	$(CC) $(CFLAGS) -c servidor_multi_thread.c

atv3_sin.tab.o: atv3_sin.tab.c atv3_sin.tab.h
	$(CC) $(CFLAGS) -c atv3_sin.tab.c

lex.yy.o: lex.yy.c atv3_sin.tab.h
	$(CC) $(CFLAGS) -c lex.yy.c

atv3_sin.tab.c atv3_sin.tab.h: atv3_sin.y
	$(YACC) $(YFLAGS) atv3_sin.y

lex.yy.c: atv3_lex.l
	$(LEX) atv3_lex.l

myList.o: inc/myList.c inc/myList.h
	$(CC) $(CFLAGS) -c inc/myList.c

myString.o: inc/myString.c inc/myString.h
	$(CC) $(CFLAGS) -c inc/myString.c

processar-requisicoes.o: inc/processar-requisicoes.c
	$(CC) $(CFLAGS) -c inc/processar-requisicoes.c

base64.o: inc/base64.c
	$(CC) $(CFLAGS) -c inc/base64.c

clean:
	rm -f *.o servidor_mono_read servidor_multi_thread atv3_sin.tab.c atv3_sin.tab.h lex.yy.c

test: all
	@echo "=== Compilação concluída ==="
	@echo "Servidores disponíveis:"
	@echo "  servidor_multi_thread  - Versão com threads"

.PHONY: all clean test thread
