all:
	bison -d especifica_12_sin.y
	
	flex especifica_12_lex.l

	gcc especifica_12_sin.tab.c lex.yy.c myString.c myList.c -o especifica_12 -lfl

	./especifica_12 < teste.txt
