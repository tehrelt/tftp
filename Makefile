run:
	make server
	./server.o 8080 .

server:
	gcc -o server.o server.c

client:
	gcc -o client.o client.c
