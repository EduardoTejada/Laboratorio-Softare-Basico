CC = gcc
CFLAGS = -Wall -g
LEX = flex
YACC = bison
YFLAGS = -d

OBJS = atv3_sin.tab.o lex.yy.o myList.o myString.o processar-requisicoes.o

all: servidor_multi_thread servidor_multi_poll servidor_mono_select servidor_mono_read

thread: servidor_multi_thread

poll: servidor_multi_poll

select: servidor_mono_select

mono: servidor_mono_read

# Servidor Multithread
servidor_multi_thread: servidor_multi_thread.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ servidor_multi_thread.o $(OBJS) -lpthread
	rm -f servidor_multi_thread.o

servidor_multi_thread.o: servidor_multi_thread.c atv3_sin.tab.h inc/myList.h inc/myString.h
	$(CC) $(CFLAGS) -c servidor_multi_thread.c

# Servidor Multiprocesso
servidor_multi_poll: servidor_multi_poll.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ servidor_multi_poll.o $(OBJS)
	rm -f servidor_multi_poll.o

servidor_multi_poll.o: servidor_multi_poll.c atv3_sin.tab.h inc/myList.h inc/myString.h
	$(CC) $(CFLAGS) -c servidor_multi_poll.c

# Servidor Monoprocesso com select
servidor_mono_select: servidor_mono_select.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ servidor_mono_select.o $(OBJS)
	rm -f servidor_mono_select.o

servidor_mono_select.o: servidor_mono_select.c atv3_sin.tab.h inc/myList.h inc/myString.h
	$(CC) $(CFLAGS) -c servidor_mono_select.c

# Servidor Monoprocesso básico
servidor_mono_read: servidor_mono_read.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ servidor_mono_read.o $(OBJS)
	rm -f servidor_mono_read.o

servidor_mono_read.o: servidor_mono_read.c atv3_sin.tab.h inc/myList.h inc/myString.h
	$(CC) $(CFLAGS) -c servidor_mono_read.c

# Regras comuns para objetos compartilhados
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

clean:
	rm -f *.o servidor_mono_read servidor_mono_select servidor_multi_poll servidor_multi_thread atv3_sin.tab.c atv3_sin.tab.h lex.yy.c

test: all
	@echo "=== Compilação concluída ==="
	@echo "Servidores disponíveis:"
	@echo "  servidor_multi_thread  - Versão com threads"
	@echo "  servidor_multi_poll    - Versão com processos"
	@echo "  servidor_mono_select   - Versão monoprocesso com select"
	@echo "  servidor_mono_read     - Versão monoprocesso básica"

.PHONY: all clean test thread poll select mono
