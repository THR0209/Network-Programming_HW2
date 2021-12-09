all:
	gcc -g -pthread server.c -o server
	gcc -g -pthread client.c -o client
clean:
	rm server
	rm client
