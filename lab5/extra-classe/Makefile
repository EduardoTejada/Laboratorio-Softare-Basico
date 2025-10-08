CC = gcc
CFLAGS = -Wall -g
LEX = flex
YACC = bison
YFLAGS = -d

OBJS = atv3_sin.tab.o lex.yy.o myList.o myString.o processar-requisicoes.o

all: servidor_multi_poll

select: servidor_mono_select

mono: servidor_mono_read

servidor_multi_poll: servidor_multi_poll.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ servidor_multi_poll.o $(OBJS)
	rm -f servidor_multi_poll.o $(OBJS)

servidor_multi_poll.o: servidor_multi_poll.c atv3_sin.tab.h inc/myList.h inc/myString.h
	$(CC) $(CFLAGS) -c servidor_multi_poll.c

servidor_mono_select: servidor_mono_select.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ servidor_mono_select.o $(OBJS)
	rm -f servidor_mono_select.o $(OBJS)

servidor_mono_select.o: servidor_mono_select.c atv3_sin.tab.h inc/myList.h inc/myString.h
	$(CC) $(CFLAGS) -c servidor_mono_select.c

servidor_mono_read: servidor_mono_read.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ servidor_mono_read.o $(OBJS)
	rm -f servidor_mono_read.o $(OBJS)

servidor_mono_read.o: servidor_mono_read.c atv3_sin.tab.h inc/myList.h inc/myString.h
	$(CC) $(CFLAGS) -c servidor_mono_read.c

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
	rm -f *.o servidor_mono_read servidor_mono_select atv3_sin.tab.c atv3_sin.tab.h lex.yy.c

.PHONY: all clean test