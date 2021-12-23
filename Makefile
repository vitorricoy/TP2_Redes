all:
	gcc -Wall -c common.c
	gcc -Wall client.c common.o -g -o client
	gcc -Wall server.c common.o -o server -pthread

clean:
	rm common.o client server1 server2 server3 server4
