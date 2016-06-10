all: lex.yy.o myshell 

lex.yy.o: lex.yy.c
	cc -c lex.yy.c

lex.yy.c: shell.l
	flex shell.l

myshell: argshell.o
	cc -o myshell argshell.o lex.yy.o -lfl

argshell.o: argshell.c
	cc -g -c argshell.c

clean:
	rm lex.yy.* myshell argshell.o
