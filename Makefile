run:
	make server
	./server.o 8080 

server:
	gcc -o server.o -I . server.c

client:
	gcc -o client.o -I . client.c

build: 
	rm bin/*.o
	make server
	mv server.o bin/server.o
	make client
	mv client.o bin/client.o
