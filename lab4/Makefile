all:
	bison -d atv3_sin.y
	
	flex atv3_lex.l

	gcc atv3_sin.tab.c lex.yy.c myString.c myList.c -o atv3 -lfl

	./atv3 < completo.txt
