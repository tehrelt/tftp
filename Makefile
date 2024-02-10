build:
	# make dir
	# rm bin/*.o
	make server
	mv server.o bin/server.o
	make client
	mv client.o bin/client.o

dir:
	mkdir bin

server:
	gcc -o server.o -I . server.c

client:
	gcc -o client.o -I . client.c

