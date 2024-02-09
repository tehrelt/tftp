run:
	make server
	./server.o 8080 .

server:
	gcc -o server.o -I . server.c

client:
	gcc -o client.o -I . client.c
