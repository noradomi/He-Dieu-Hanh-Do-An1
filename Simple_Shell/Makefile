CC=gcc
all:simple_shell
	
simple_shell:simple_shell.o history.o
	$(CC) simple_shell.o history.o -o simple_shell
simple_shell.o:simple_shell.c
	$(CC) -c simple_shell.c
history.o: history.c
	$(CC) -c history.c
clean:
	rm -f *.o main
