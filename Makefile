all:
	gcc -Wall -g -c common.c
	gcc -Wall -g -c lista.c
	gcc -Wall client.c common.o lista.o -g -o client
	gcc -Wall -g server.c common.o lista.o -o server -pthread -lm

clean:
	rm common.o lista.o client server
