all:
	gcc -Wall -c common.c
	gcc -Wall -c lista.c
	gcc -Wall client.c common.o lista.o -o client
	gcc -Wall server.c common.o lista.o -o server -pthread -lm

clean:
	rm common.o lista.o client server
