all:
	gcc -Wall -c common.c
	gcc -Wall client.c common.o -g -o client
	gcc -Wall server1.c common.o -o server1
	gcc -Wall server2.c common.o -o server2
	gcc -Wall server3.c common.o -o server3
	gcc -Wall server4.c common.o -o server4

clean:
	rm common.o client server1 server2 server3 server4
