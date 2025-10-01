CC = gcc
CFLAGS = -Wall -g
LEX = flex
YACC = bison
YFLAGS = -d

OBJS = servidor.o atv3_sin.tab.o lex.yy.o myList.o myString.o processar-requisicoes.o

all: servidor

servidor: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

servidor.o: servidor.c atv3_sin.tab.h inc/myList.h inc/myString.h
	$(CC) $(CFLAGS) -c servidor.c

atv3_sin.tab.o: atv3_sin.tab.c atv3_sin.tab.h
	$(CC) $(CFLAGS) -c atv3_sin.tab.c

lex.yy.o: lex.yy.c atv3_sin.tab.h
	$(CC) $(CFLAGS) -c lex.yy.c

atv3_sin.tab.c atv3_sin.tab.h: atv3_sin.y
	$(YACC) $(YFLAGS) atv3_sin.y

lex.yy.c: atv3_lex.l
	$(LEX) atv3_lex.l

myList.o: myList.c inc/myList.h
	$(CC) $(CFLAGS) -c myList.c

myString.o: myString.c inc/myString.h
	$(CC) $(CFLAGS) -c myString.c

processar-requisicoes.o: processar-requisicoes.c
	$(CC) $(CFLAGS) -c processar-requisicoes.c

clean:
	rm -f *.o servidor atv3_sin.tab.c atv3_sin.tab.h lex.yy.c

test: servidor
	chmod +x testar_servidor.sh
	./testar_servidor.sh

.PHONY: all clean test